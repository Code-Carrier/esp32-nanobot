/**
 * @file config_cli.c
 * @brief Serial Command Line Interface for Configuration
 *
 * Commands:
 * - help: Show available commands
 * - status: Show current configuration status
 * - wifi <ssid> <password>: Set WiFi credentials
 * - ai <provider> <api_key> <model>: Set AI provider
 * - qq <appid> <secret> <openid>: Set QQ bot
 * - feishu <appid> <secret> <openid>: Set Feishu bot
 * - reset: Reset all configuration
 * - save: Save configuration to NVS
 * - scan: Scan available WiFi networks
 */

#include "config_manager.h"
#include "esp_log.h"
#include "driver/uart.h"
#include "string.h"
#include "stdio.h"
#include "esp_wifi.h"
#include "esp_netif.h"

static const char *TAG = "config_cli";

#define UART_BUF_SIZE (256)
#define CLI_TASK_STACK_SIZE (4096)
#define CLI_TASK_PRIORITY (5)

// CLI task handle
static TaskHandle_t s_cli_task_handle = NULL;

/**
 * @brief Trim whitespace from string
 */
static char* trim(char *str)
{
    if (!str) return NULL;
    while (*str == ' ' || *str == '\t' || *str == '\n' || *str == '\r') str++;
    if (*str == 0) return str;

    char *end = str + strlen(str) - 1;
    while (end > str && (*end == ' ' || *end == '\t' || *end == '\n' || *end == '\r')) end--;
    *(end + 1) = '\0';

    return str;
}

/**
 * @brief Parse command line arguments
 */
static int parse_args(char *line, char **args, int max_args)
{
    int count = 0;
    char *token = strtok(line, " \t");

    while (token && count < max_args) {
        args[count++] = token;
        token = strtok(NULL, " \t");
    }

    return count;
}

/**
 * @brief Show help message
 */
static void cli_show_help(void)
{
    printf("\n=== ESP32 NanoBot Configuration CLI ===\n\n");
    printf("Available commands:\n");
    printf("  help                              - Show this help message\n");
    printf("  status                            - Show current configuration\n");
    printf("  wifi <ssid> <password>            - Set WiFi credentials\n");
    printf("  ai <provider> <api_key> [model]   - Set AI provider\n");
    printf("     provider: deepseek, moonshot, dashscope, etc.\n");
    printf("  qq <appid> <secret> [openid]      - Set QQ bot\n");
    printf("  feishu <appid> <secret> [openid]  - Set Feishu bot\n");
    printf("  summary <HH:MM> <channel>         - Set daily summary (channel: qq/feishu)\n");
    printf("  scan                              - Scan available WiFi networks\n");
    printf("  reset                             - Reset all configuration\n");
    printf("  save                              - Save configuration to NVS\n");
    printf("  reboot                            - Reboot device\n");
    printf("\nExamples:\n");
    printf("  wifi MyWiFi MyPassword123\n");
    printf("  ai deepseek sk-xxxxxxxxx deepseek-chat\n");
    printf("  qq 12345678 abcdefghijk\n");
    printf("  feishu cli_xxxxxxxx xxxxxxxxxxxx\n");
    printf("  summary 18:00 qq\n");
    printf("\n");
}

/**
 * @brief Show current configuration status
 */
static void cli_show_status(void)
{
    device_config_t *config = config_manager_get_current();

    printf("\n=== Current Configuration ===\n\n");

    printf("WiFi:\n");
    printf("  SSID: %s\n", config->wifi_ssid[0] ? config->wifi_ssid : "(not configured)");
    printf("  Status: %s\n", config->wifi_configured ? "configured" : "not configured");

    printf("\nAI Provider:\n");
    printf("  Type: %s\n", config->ai_provider_type[0] ? config->ai_provider_type : "(not configured)");
    printf("  Model: %s\n", config->ai_provider_model[0] ? config->ai_provider_model : "(not configured)");
    printf("  URL: %s\n", config->ai_provider_base_url[0] ? config->ai_provider_base_url : "(not configured)");
    printf("  API Key: %s\n", config->ai_provider_api_key[0] ? "******" : "(not configured)");
    printf("  Status: %s\n", config->ai_configured ? "configured" : "not configured");

    printf("\nQQ Bot:\n");
    printf("  Enabled: %s\n", config->qq_bot_enabled ? "yes" : "no");
    printf("  App ID: %s\n", config->qq_app_id[0] ? config->qq_app_id : "(not configured)");

    printf("\nFeishu Bot:\n");
    printf("  Enabled: %s\n", config->feishu_bot_enabled ? "yes" : "no");
    printf("  App ID: %s\n", config->feishu_app_id[0] ? config->feishu_app_id : "(not configured)");

    printf("\nDaily Summary:\n");
    printf("  Time: %s\n", config->summary_send_time[0] ? config->summary_send_time : "18:00");
    printf("  Channel: %s\n", config->summary_channel[0] ? config->summary_channel : "qq");

    printf("\nDisplay:\n");
    printf("  Enabled: %s\n", config->display_enabled ? "yes" : "no");
    printf("  Brightness: %d%%\n", config->display_brightness);

    printf("\n================================\n\n");
}

/**
 * @brief Scan available WiFi networks
 */
static esp_err_t cli_wifi_scan(void)
{
    printf("\nScanning WiFi networks...\n");

    wifi_scan_config_t scan_config = {
        .ssid = NULL,
        .bssid = NULL,
        .channel = 0,
        .scan_type = WIFI_SCAN_TYPE_ACTIVE,
        .scan_time.active.min = 100,
        .scan_time.active.max = 200,
    };

    ESP_ERROR_CHECK(esp_wifi_scan_start(&scan_config, true));

    uint16_t ap_count = 0;
    ESP_ERROR_CHECK(esp_wifi_scan_get_ap_num(&ap_count));

    if (ap_count == 0) {
        printf("No networks found.\n");
        return ESP_OK;
    }

    wifi_ap_record_t *ap_records = malloc(ap_count * sizeof(wifi_ap_record_t));
    if (!ap_records) {
        return ESP_ERR_NO_MEM;
    }

    ESP_ERROR_CHECK(esp_wifi_scan_get_ap_records(&ap_count, ap_records));

    printf("\nFound %d networks:\n\n", ap_count);
    printf("  %-32s %-18s %s\n", "SSID", "BSSID", "RSSI");
    printf("  %-32s %-18s %s\n", "--------------------------------", "------------------", "----");

    for (int i = 0; i < ap_count; i++) {
        printf("  %-32s %-18s %d\n",
               (char *)ap_records[i].ssid,
               (char *)ap_records[i].bssid,
               ap_records[i].rssi);
    }

    free(ap_records);
    printf("\n");

    return ESP_OK;
}

/**
 * @brief CLI main loop
 */
static void cli_task(void *pvParameters)
{
    char uart_buf[UART_BUF_SIZE];
    char cmd_buf[UART_BUF_SIZE];
    int cmd_idx = 0;

    printf("\n");
    printf("╔═══════════════════════════════════════════╗\n");
    printf("║   ESP32 NanoBot Configuration CLI v1.0    ║\n");
    printf("║   Type 'help' for available commands      ║\n");
    printf("╚═══════════════════════════════════════════╝\n");
    printf("\nnanobot> ");

    while (1) {
        int len = uart_read_bytes(UART_NUM_0, uart_buf, UART_BUF_SIZE, pdMS_TO_TICKS(100));

        for (int i = 0; i < len; i++) {
            char c = uart_buf[i];

            if (c == '\r' || c == '\n') {
                if (cmd_idx > 0) {
                    cmd_buf[cmd_idx] = '\0';
                    printf("\n");

                    // Parse command
                    char *args[10];
                    int arg_count = parse_args(cmd_buf, args, 10);

                    if (arg_count > 0) {
                        char *cmd = args[0];

                        if (strcmp(cmd, "help") == 0) {
                            cli_show_help();
                        }
                        else if (strcmp(cmd, "status") == 0) {
                            cli_show_status();
                        }
                        else if (strcmp(cmd, "scan") == 0) {
                            cli_wifi_scan();
                        }
                        else if (strcmp(cmd, "reset") == 0) {
                            printf("Resetting configuration...\n");
                            config_manager_reset();
                            printf("Configuration reset. Type 'reboot' to restart.\n");
                        }
                        else if (strcmp(cmd, "save") == 0) {
                            device_config_t *config = config_manager_get_current();
                            if (config_manager_save(config) == ESP_OK) {
                                printf("Configuration saved to NVS.\n");
                            } else {
                                printf("Failed to save configuration.\n");
                            }
                        }
                        else if (strcmp(cmd, "reboot") == 0) {
                            printf("Rebooting...\n");
                            vTaskDelay(pdMS_TO_TICKS(1000));
                            esp_restart();
                        }
                        else if (strcmp(cmd, "wifi") == 0 && arg_count >= 3) {
                            device_config_t *config = config_manager_get_current();
                            strncpy(config->wifi_ssid, args[1], sizeof(config->wifi_ssid) - 1);
                            strncpy(config->wifi_password, args[2], sizeof(config->wifi_password) - 1);
                            config->wifi_configured = true;
                            printf("WiFi credentials set. Type 'save' to save.\n");
                        }
                        else if (strcmp(cmd, "ai") == 0 && arg_count >= 3) {
                            device_config_t *config = config_manager_get_current();
                            strncpy(config->ai_provider_type, args[1], sizeof(config->ai_provider_type) - 1);
                            strncpy(config->ai_provider_api_key, args[2], sizeof(config->ai_provider_api_key) - 1);
                            if (arg_count >= 4) {
                                strncpy(config->ai_provider_model, args[3], sizeof(config->ai_provider_model) - 1);
                            }
                            config->ai_configured = true;
                            printf("AI provider set. Type 'save' to save.\n");
                        }
                        else if (strcmp(cmd, "qq") == 0 && arg_count >= 3) {
                            device_config_t *config = config_manager_get_current();
                            config->qq_bot_enabled = true;
                            strncpy(config->qq_app_id, args[1], sizeof(config->qq_app_id) - 1);
                            strncpy(config->qq_app_secret, args[2], sizeof(config->qq_app_secret) - 1);
                            if (arg_count >= 4) {
                                strncpy(config->qq_recipient_openid, args[3], sizeof(config->qq_recipient_openid) - 1);
                            }
                            printf("QQ bot set. Type 'save' to save.\n");
                        }
                        else if (strcmp(cmd, "feishu") == 0 && arg_count >= 3) {
                            device_config_t *config = config_manager_get_current();
                            config->feishu_bot_enabled = true;
                            strncpy(config->feishu_app_id, args[1], sizeof(config->feishu_app_id) - 1);
                            strncpy(config->feishu_app_secret, args[2], sizeof(config->feishu_app_secret) - 1);
                            if (arg_count >= 4) {
                                strncpy(config->feishu_recipient_openid, args[3], sizeof(config->feishu_recipient_openid) - 1);
                            }
                            printf("Feishu bot set. Type 'save' to save.\n");
                        }
                        else if (strcmp(cmd, "summary") == 0 && arg_count >= 3) {
                            device_config_t *config = config_manager_get_current();
                            strncpy(config->summary_send_time, args[1], sizeof(config->summary_send_time) - 1);
                            strncpy(config->summary_channel, args[2], sizeof(config->summary_channel) - 1);
                            printf("Daily summary set. Type 'save' to save.\n");
                        }
                        else {
                            printf("Unknown command: %s\n", cmd);
                            printf("Type 'help' for available commands.\n");
                        }
                    }

                    cmd_idx = 0;
                    printf("\nnanobot> ");
                } else {
                    printf("\nnanobot> ");
                }
            }
            else if (c == '\b' || c == 127) {
                if (cmd_idx > 0) {
                    cmd_idx--;
                    printf("\b \b");
                }
            }
            else if (c >= 32 && cmd_idx < UART_BUF_SIZE - 1) {
                cmd_buf[cmd_idx++] = c;
                putchar(c);
            }
        }
    }
}

esp_err_t config_cli_init(void)
{
    // Configure UART parameters
    uart_config_t uart_config = {
        .baud_rate = 115200,
        .data_bits = UART_DATA_8_BITS,
        .parity = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
        .source_clk = UART_SCLK_DEFAULT,
    };

    ESP_ERROR_CHECK(uart_param_config(UART_NUM_0, &uart_config));

    // Create CLI task
    BaseType_t ret = xTaskCreate(cli_task, "config_cli", CLI_TASK_STACK_SIZE,
                                  NULL, CLI_TASK_PRIORITY, &s_cli_task_handle);

    if (ret != pdPASS) {
        ESP_LOGE(TAG, "Failed to create CLI task");
        return ESP_FAIL;
    }

    ESP_LOGI(TAG, "Configuration CLI initialized");
    return ESP_OK;
}
