/**
 * @file config_manager.c
 * @brief Configuration Manager Implementation with NVS
 */

#include "config_manager.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "nvs.h"
#include <string.h>
#include <stdio.h>

static const char *TAG = "config_manager";

// NVS namespace
#define NVS_NAMESPACE "nanobot_config"

// Singleton instance
static device_config_t s_current_config = {0};
static bool s_initialized = false;

/**
 * @brief Initialize default configuration
 */
static void config_init_defaults(device_config_t *config)
{
    memset(config, 0, sizeof(device_config_t));

    // Defaults
    strncpy(config->ai_provider_type, "deepseek", CONFIG_MAX_STRING_LEN - 1);
    strncpy(config->ai_provider_model, "deepseek-chat", CONFIG_MAX_STRING_LEN - 1);
    strncpy(config->ai_provider_base_url, "https://api.deepseek.com", CONFIG_MAX_STRING_LEN - 1);
    strncpy(config->summary_send_time, "18:00", sizeof(config->summary_send_time) - 1);
    strncpy(config->summary_channel, "qq", sizeof(config->summary_channel) - 1);
    config->display_brightness = 80;
}

esp_err_t config_manager_init(void)
{
    if (s_initialized) {
        return ESP_OK;
    }

    ESP_LOGI(TAG, "Initializing configuration manager...");

    // Initialize NVS
    esp_err_t err = nvs_flash_init();
    if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_LOGW(TAG, "NVS partition truncated, erasing...");
        ESP_ERROR_CHECK(nvs_flash_erase());
        err = nvs_flash_init();
    }

    if (err != ESP_OK) {
        ESP_LOGE(TAG, "NVS init failed: %s", esp_err_to_name(err));
        return err;
    }

    // Load existing configuration
    config_init_defaults(&s_current_config);
    config_manager_load(&s_current_config);

    s_initialized = true;
    ESP_LOGI(TAG, "Configuration manager initialized");

    // Print configuration status
    ESP_LOGI(TAG, "WiFi configured: %s", s_current_config.wifi_configured ? "yes" : "no");
    ESP_LOGI(TAG, "AI Provider configured: %s", s_current_config.ai_configured ? "yes" : "no");

    return ESP_OK;
}

esp_err_t config_manager_deinit(void)
{
    s_initialized = false;
    return nvs_flash_deinit();
}

esp_err_t config_manager_load(device_config_t *config)
{
    if (!config) {
        return ESP_ERR_INVALID_ARG;
    }

    nvs_handle_t nvs_handle;
    esp_err_t err = nvs_open(NVS_NAMESPACE, NVS_READWRITE, &nvs_handle);
    if (err != ESP_OK) {
        ESP_LOGD(TAG, "No saved configuration found");
        return err;
    }

    // WiFi Settings
    size_t len = sizeof(config->wifi_ssid);
    if (nvs_get_str(nvs_handle, "wifi_ssid", config->wifi_ssid, &len) == ESP_OK) {
        config->wifi_configured = true;
    }

    len = sizeof(config->wifi_password);
    nvs_get_str(nvs_handle, "wifi_password", config->wifi_password, &len);

    // AI Provider Settings
    len = sizeof(config->ai_provider_type);
    if (nvs_get_str(nvs_handle, "ai_type", config->ai_provider_type, &len) == ESP_OK) {
        config->ai_configured = true;
    }

    len = sizeof(config->ai_provider_api_key);
    nvs_get_str(nvs_handle, "ai_key", config->ai_provider_api_key, &len);

    len = sizeof(config->ai_provider_model);
    nvs_get_str(nvs_handle, "ai_model", config->ai_provider_model, &len);

    len = sizeof(config->ai_provider_base_url);
    nvs_get_str(nvs_handle, "ai_url", config->ai_provider_base_url, &len);

    // QQ Bot Settings
    config->qq_bot_enabled = (nvs_get_u8(nvs_handle, "qq_enabled", (uint8_t*)&config->qq_bot_enabled) == ESP_OK);

    len = sizeof(config->qq_app_id);
    nvs_get_str(nvs_handle, "qq_id", config->qq_app_id, &len);

    len = sizeof(config->qq_app_secret);
    nvs_get_str(nvs_handle, "qq_secret", config->qq_app_secret, &len);

    len = sizeof(config->qq_recipient_openid);
    nvs_get_str(nvs_handle, "qq_oid", config->qq_recipient_openid, &len);

    // Feishu Bot Settings
    config->feishu_bot_enabled = (nvs_get_u8(nvs_handle, "fs_enabled", (uint8_t*)&config->feishu_bot_enabled) == ESP_OK);

    len = sizeof(config->feishu_app_id);
    nvs_get_str(nvs_handle, "fs_id", config->feishu_app_id, &len);

    len = sizeof(config->feishu_app_secret);
    nvs_get_str(nvs_handle, "fs_secret", config->feishu_app_secret, &len);

    len = sizeof(config->feishu_recipient_openid);
    nvs_get_str(nvs_handle, "fs_oid", config->feishu_recipient_openid, &len);

    // Daily Summary Settings
    len = sizeof(config->summary_send_time);
    nvs_get_str(nvs_handle, "sum_time", config->summary_send_time, &len);

    len = sizeof(config->summary_channel);
    nvs_get_str(nvs_handle, "sum_channel", config->summary_channel, &len);

    // Display Settings
    config->display_enabled = (nvs_get_u8(nvs_handle, "disp_en", (uint8_t*)&config->display_enabled) == ESP_OK);
    uint8_t brightness = (uint8_t)config->display_brightness;
    if (nvs_get_u8(nvs_handle, "disp_bright", &brightness) == ESP_OK) {
        config->display_brightness = brightness;
    }

    nvs_close(nvs_handle);
    return ESP_OK;
}

esp_err_t config_manager_save(const device_config_t *config)
{
    if (!config) {
        return ESP_ERR_INVALID_ARG;
    }

    nvs_handle_t nvs_handle;
    esp_err_t err = nvs_open(NVS_NAMESPACE, NVS_READWRITE, &nvs_handle);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "NVS open failed: %s", esp_err_to_name(err));
        return err;
    }

    // WiFi Settings
    if (strlen(config->wifi_ssid) > 0) {
        nvs_set_str(nvs_handle, "wifi_ssid", config->wifi_ssid);
        nvs_set_str(nvs_handle, "wifi_password", config->wifi_password);
    }

    // AI Provider Settings
    if (strlen(config->ai_provider_api_key) > 0) {
        nvs_set_str(nvs_handle, "ai_type", config->ai_provider_type);
        nvs_set_str(nvs_handle, "ai_key", config->ai_provider_api_key);
        nvs_set_str(nvs_handle, "ai_model", config->ai_provider_model);
        nvs_set_str(nvs_handle, "ai_url", config->ai_provider_base_url);
    }

    // QQ Bot Settings
    nvs_set_u8(nvs_handle, "qq_enabled", config->qq_bot_enabled);
    if (strlen(config->qq_app_id) > 0) {
        nvs_set_str(nvs_handle, "qq_id", config->qq_app_id);
        nvs_set_str(nvs_handle, "qq_secret", config->qq_app_secret);
        nvs_set_str(nvs_handle, "qq_oid", config->qq_recipient_openid);
    }

    // Feishu Bot Settings
    nvs_set_u8(nvs_handle, "fs_enabled", config->feishu_bot_enabled);
    if (strlen(config->feishu_app_id) > 0) {
        nvs_set_str(nvs_handle, "fs_id", config->feishu_app_id);
        nvs_set_str(nvs_handle, "fs_secret", config->feishu_app_secret);
        nvs_set_str(nvs_handle, "fs_oid", config->feishu_recipient_openid);
    }

    // Daily Summary Settings
    nvs_set_str(nvs_handle, "sum_time", config->summary_send_time);
    nvs_set_str(nvs_handle, "sum_channel", config->summary_channel);

    // Display Settings
    nvs_set_u8(nvs_handle, "disp_en", config->display_enabled);
    nvs_set_u8(nvs_handle, "disp_bright", config->display_brightness);

    err = nvs_commit(nvs_handle);
    nvs_close(nvs_handle);

    if (err == ESP_OK) {
        memcpy(&s_current_config, config, sizeof(device_config_t));
        ESP_LOGI(TAG, "Configuration saved successfully");
    }

    return err;
}

esp_err_t config_manager_reset(void)
{
    nvs_handle_t nvs_handle;
    esp_err_t err = nvs_open(NVS_NAMESPACE, NVS_READWRITE, &nvs_handle);
    if (err != ESP_OK) {
        return err;
    }

    err = nvs_erase_all(nvs_handle);
    if (err == ESP_OK) {
        err = nvs_commit(nvs_handle);
    }
    nvs_close(nvs_handle);

    if (err == ESP_OK) {
        config_init_defaults(&s_current_config);
        ESP_LOGI(TAG, "Configuration reset to defaults");
    }

    return err;
}

device_config_t* config_manager_get_current(void)
{
    return &s_current_config;
}

bool config_manager_is_configured(void)
{
    return s_current_config.wifi_configured && s_current_config.ai_configured;
}
