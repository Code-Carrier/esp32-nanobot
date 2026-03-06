/**
 * @file config_manager.h
 * @brief Configuration Manager with NVS Storage
 *
 * Provides:
 * - WiFi credentials storage
 * - AI Provider settings
 * - QQ/Feishu bot credentials
 * - Serial command line interface
 */

#ifndef CONFIG_MANAGER_H
#define CONFIG_MANAGER_H

#include "esp_err.h"
#include <stddef.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

#define CONFIG_MAX_STRING_LEN   128
#define CONFIG_MAX_KEY_LEN      32

/**
 * @brief Configuration data structure
 */
typedef struct {
    // WiFi Settings
    char wifi_ssid[CONFIG_MAX_STRING_LEN];
    char wifi_password[CONFIG_MAX_STRING_LEN];
    bool wifi_configured;

    // AI Provider Settings
    char ai_provider_type[CONFIG_MAX_STRING_LEN];
    char ai_provider_api_key[CONFIG_MAX_STRING_LEN];
    char ai_provider_model[CONFIG_MAX_STRING_LEN];
    char ai_provider_base_url[CONFIG_MAX_STRING_LEN];
    bool ai_configured;

    // QQ Bot Settings
    bool qq_bot_enabled;
    char qq_app_id[CONFIG_MAX_STRING_LEN];
    char qq_app_secret[CONFIG_MAX_STRING_LEN];
    char qq_recipient_openid[CONFIG_MAX_STRING_LEN];

    // Feishu Bot Settings
    bool feishu_bot_enabled;
    char feishu_app_id[CONFIG_MAX_STRING_LEN];
    char feishu_app_secret[CONFIG_MAX_STRING_LEN];
    char feishu_recipient_openid[CONFIG_MAX_STRING_LEN];

    // Daily Summary Settings
    char summary_send_time[16];  // Format: HH:MM
    char summary_channel[16];    // "qq" or "feishu"

    // Display Settings
    bool display_enabled;
    int display_brightness;      // 0-100
} device_config_t;

/**
 * @brief Initialize configuration manager
 * @return ESP_OK on success
 */
esp_err_t config_manager_init(void);

/**
 * @brief Deinitialize configuration manager
 * @return ESP_OK on success
 */
esp_err_t config_manager_deinit(void);

/**
 * @brief Load configuration from NVS
 * @param config Pointer to config structure to fill
 * @return ESP_OK on success
 */
esp_err_t config_manager_load(device_config_t *config);

/**
 * @brief Save configuration to NVS
 * @param config Pointer to config structure to save
 * @return ESP_OK on success
 */
esp_err_t config_manager_save(const device_config_t *config);

/**
 * @brief Reset all configuration to defaults
 * @return ESP_OK on success
 */
esp_err_t config_manager_reset(void);

/**
 * @brief Get current configuration (singleton)
 * @return Pointer to current configuration
 */
device_config_t* config_manager_get_current(void);

/**
 * @brief Serial CLI initialization
 * @return ESP_OK on success
 */
esp_err_t config_cli_init(void);

/**
 * @brief Start WiFi AP mode for configuration
 * @return ESP_OK on success
 */
esp_err_t config_wifi_ap_start(void);

/**
 * @brief Stop WiFi AP mode
 * @return ESP_OK on success
 */
esp_err_t config_wifi_ap_stop(void);

/**
 * @brief Check if configuration is complete
 * @return true if WiFi and AI Provider are configured
 */
bool config_manager_is_configured(void);

#ifdef __cplusplus
}
#endif

#endif // CONFIG_MANAGER_H
