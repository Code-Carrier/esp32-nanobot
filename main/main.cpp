/**
 * @file main.cpp
 * @brief Main entry point for ESP32 NanoBot
 *
 * Features:
 * - Web-based configuration (AP mode)
 * - Serial CLI configuration
 * - TFT display support
 * - QQ/Feishu bot integration
 * - AI-powered daily summary
 */

#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "esp_system.h"
#include "nvs_flash.h"
#include "esp_netif.h"
#include "esp_event.h"
#include "esp_timer.h"
#include "driver/gpio.h"

#include "wifi_manager.h"
#include "config_manager.h"
#include "config_web.h"
#include "ai_provider.h"
#include "qq_bot.h"
#include "feishu_bot.h"
#include "daily_summary.h"
#include "tft_display.h"

static const char *TAG = "NanoBot";

// Task handles
static TaskHandle_t s_config_task_handle = NULL;
static TaskHandle_t s_wifi_task_handle = NULL;
static TaskHandle_t s_qq_bot_task_handle = NULL;
static TaskHandle_t s_feishu_bot_task_handle = NULL;
static TaskHandle_t s_daily_summary_task_handle = NULL;
static TaskHandle_t s_display_task_handle = NULL;

// Button pin for entering AP configuration mode
#define CONFIG_BUTTON_PIN   GPIO_NUM_0  // BOOT button on most ESP32-S3 boards
#define BUTTON_HOLD_TIME_MS 3000

/**
 * @brief Check if config button is pressed during startup
 * @return true if button is held
 */
static bool check_config_button(void)
{
    gpio_config_t io_conf = {
        .mode = GPIO_MODE_INPUT,
        .pin_bit_mask = (1ULL << CONFIG_BUTTON_PIN),
        .pull_up_en = GPIO_PULLUP_ENABLE,
    };
    gpio_config(&io_conf);

    // Wait for button press
    vTaskDelay(pdMS_TO_TICKS(500));

    int hold_count = 0;
    for (int i = 0; i < 50; i++) {  // 5 seconds max
        if (gpio_get_level(CONFIG_BUTTON_PIN) == 0) {
            hold_count++;
            if (hold_count * 100 >= CONFIG_HOLD_TIME_MS) {
                return true;
            }
        } else {
            hold_count = 0;
        }
        vTaskDelay(pdMS_TO_TICKS(100));
    }
    return false;
}

/**
 * @brief Initialize NVS flash storage
 */
static esp_err_t init_nvs(void)
{
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_LOGW(TAG, "NVS partition truncated, erasing...");
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    return ret;
}

/**
 * @brief WiFi connection task
 */
static void wifi_task(void *pvParameters)
{
    device_config_t *config = (device_config_t *)pvParameters;

    ESP_ERROR_CHECK(wifi_manager_init());

    // Wait for WiFi connection with timeout
    TickType_t timeout = pdMS_TO_TICKS(30000);
    TickType_t start = xTaskGetTickCount();

    while (!wifi_manager_is_connected()) {
        if (xTaskGetTickCount() - start > timeout) {
            ESP_LOGE(TAG, "WiFi connection timeout");
            if (s_display_task_handle) {
                tft_show_wifi_status(false, NULL);
            }
            vTaskDelete(NULL);
        }
        vTaskDelay(pdMS_TO_TICKS(500));
    }

    ESP_LOGI(TAG, "WiFi connected successfully");

    // Print IP address
    char ip_str[16];
    if (wifi_manager_get_ip(ip_str, sizeof(ip_str)) == ESP_OK) {
        ESP_LOGI(TAG, "IP Address: %s", ip_str);
        if (s_display_task_handle) {
            tft_show_wifi_status(true, ip_str);
        }
    }

    vTaskDelete(NULL);
}

/**
 * @brief Display status task
 */
static void display_task(void *pvParameters)
{
    ESP_LOGI(TAG, "Display task started");

    // Initialize display
    if (tft_display_init() != ESP_OK) {
        ESP_LOGE(TAG, "Failed to initialize TFT display");
        vTaskDelete(NULL);
    }

    // Show boot screen
    tft_fill_screen(TFT_COLOR_NAVY);
    tft_print_centered(50, "ESP32 NanoBot", TFT_COLOR_WHITE, 3);
    tft_print_centered(120, "v1.0.0", TFT_COLOR_WHITE, 2);
    tft_print_centered(180, "Starting...", TFT_COLOR_GREEN, 1);

    while (1) {
        // Update display with system status
        static char last_ip[16] = {0};
        char ip_str[16];

        if (wifi_manager_get_ip(ip_str, sizeof(ip_str)) == ESP_OK) {
            if (strcmp(ip_str, last_ip) != 0) {
                strncpy(last_ip, ip_str, sizeof(last_ip) - 1);
                tft_show_wifi_status(true, ip_str);
            }
        }

        vTaskDelay(pdMS_TO_TICKS(5000));
    }
}

/**
 * @brief Configuration mode task (AP + Web Server)
 */
static void config_mode_task(void *pvParameters)
{
    ESP_LOGI(TAG, "Starting configuration mode...");

    // Show configuration screen
    tft_show_status("Setup Mode", "Connect to WiFi: NanoBot-Setup\nPassword: 12345678\nThen open: 192.168.4.1", 0);

    // Start web server
    config_web_server_start();

    ESP_LOGI(TAG, "Configuration mode started");
    ESP_LOGI(TAG, "Connect to: NanoBot-Setup");
    ESP_LOGI(TAG, "Password: 12345678");
    ESP_LOGI(TAG, "Web: http://192.168.4.1");

    // Keep running until configured
    while (!config_manager_is_configured()) {
        vTaskDelay(pdMS_TO_TICKS(1000));
    }

    ESP_LOGI(TAG, "Configuration complete, restarting...");
    vTaskDelay(pdMS_TO_TICKS(2000));
    esp_restart();
}

/**
 * @brief Main application entry point
 */
extern "C" void app_main(void)
{
    ESP_LOGI(TAG, "=======================================");
    ESP_LOGI(TAG, "   ESP32 NanoBot v1.0.0");
    ESP_LOGI(TAG, "   Ultra-Lightweight AI Assistant");
    ESP_LOGI(TAG, "=======================================");

    // Print chip information
    ESP_LOGI(TAG, "Chip: ESP32-S3");
    ESP_LOGI(TAG, "Flash: 16MB, PSRAM: 8MB");

    // Initialize NVS
    ESP_ERROR_CHECK(init_nvs());
    ESP_LOGI(TAG, "NVS initialized");

    // Initialize network interface
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());

    // Initialize configuration manager
    ESP_ERROR_CHECK(config_manager_init());

    // Initialize serial CLI
    ESP_ERROR_CHECK(config_cli_init());

    // Get current configuration
    device_config_t *config = config_manager_get_current();

    // Check if we should enter AP configuration mode
    bool enter_ap_mode = !config_manager_is_configured();

#ifdef CONFIG_ENTER_AP_MODE_ALWAYS
    enter_ap_mode = true;
#endif

    if (enter_ap_mode) {
        ESP_LOGI(TAG, "No configuration found, entering AP mode...");

        // Start display task
        xTaskCreate(display_task, "display_task", 4096, NULL, 4, &s_display_task_handle);
        vTaskDelay(pdMS_TO_TICKS(1000));

        // Start AP mode task
        xTaskCreate(config_mode_task, "config_mode_task", 8192, NULL, 5, &s_config_task_handle);

        // Wait forever (will reboot after configuration)
        while (1) {
            vTaskDelay(pdMS_TO_TICKS(1000));
        }
    }

    // Normal mode - connect to configured WiFi
    ESP_LOGI(TAG, "Loading configuration...");
    ESP_LOGI(TAG, "  WiFi SSID: %s", config->wifi_ssid);
    ESP_LOGI(TAG, "  AI Provider: %s", config->ai_provider_type);

    // Initialize AI Provider with saved config
    ai_provider_config_t ai_config = {
        .type = AI_PROVIDER_DEEPSEEK,
        .api_key = config->ai_provider_api_key,
        .base_url = config->ai_provider_base_url,
        .model = config->ai_provider_model,
        .timeout_ms = 30000,
        .max_tokens = 2048,
        .temperature = 0.7f,
    };

    // Parse provider type
    if (strcmp(config->ai_provider_type, "moonshot") == 0) ai_config.type = AI_PROVIDER_MOONSHOT;
    else if (strcmp(config->ai_provider_type, "dashscope") == 0) ai_config.type = AI_PROVIDER_DASHSCOPE;
    else if (strcmp(config->ai_provider_type, "siliconflow") == 0) ai_config.type = AI_PROVIDER_SILICONFLOW;
    else if (strcmp(config->ai_provider_type, "volcengine") == 0) ai_config.type = AI_PROVIDER_VOLCENGINE;
    else if (strcmp(config->ai_provider_type, "zhipu") == 0) ai_config.type = AI_PROVIDER_ZHIPU;

    if (ai_provider_init() == ESP_OK) {
        ESP_LOGI(TAG, "AI Provider initialized");
    }

    // Start display task
    xTaskCreate(display_task, "display_task", 4096, NULL, 4, &s_display_task_handle);

    // Wait for display to initialize
    vTaskDelay(pdMS_TO_TICKS(1000));

    // Show AI status
    tft_show_ai_status(true);
    vTaskDelay(pdMS_TO_TICKS(2000));

    // Start WiFi task
    xTaskCreate(wifi_task, "wifi_task", 4096, config, 5, &s_wifi_task_handle);

    // Wait for WiFi to connect
    vTaskDelay(pdMS_TO_TICKS(5000));

    // Initialize QQ Bot (if enabled)
    if (config->qq_bot_enabled) {
        if (qq_bot_init() == ESP_OK) {
            ESP_LOGI(TAG, "QQ Bot initialized");
            xTaskCreate(qq_bot_task, "qq_bot_task", 8192, NULL, 5, &s_qq_bot_task_handle);
        }
    }

    // Initialize Feishu Bot (if enabled)
    if (config->feishu_bot_enabled) {
        if (feishu_bot_init() == ESP_OK) {
            ESP_LOGI(TAG, "Feishu Bot initialized");
            xTaskCreate(feishu_bot_task, "feishu_bot_task", 8192, NULL, 5, &s_feishu_bot_task_handle);
        }
    }

    // Initialize Daily Summary Skill
    if (config->qq_bot_enabled || config->feishu_bot_enabled) {
        if (daily_summary_init() == ESP_OK) {
            ESP_LOGI(TAG, "Daily Summary Skill initialized");
            xTaskCreate(daily_summary_task, "daily_summary_task", 6144, NULL, 4, &s_daily_summary_task_handle);
        }
    }

    ESP_LOGI(TAG, "=======================================");
    ESP_LOGI(TAG, "   NanoBot started successfully!");
    ESP_LOGI(TAG, "=======================================");
    ESP_LOGI(TAG, "Free heap: %lu bytes", esp_get_free_heap_size());

    // Main loop - monitor system status
    while (1) {
        ESP_LOGI(TAG, "Free heap: %lu, PSRAM: %lu",
                 esp_get_free_heap_size(),
                 heap_caps_get_free_size(MALLOC_CAP_SPIRAM));
        vTaskDelay(pdMS_TO_TICKS(60000));
    }
}
