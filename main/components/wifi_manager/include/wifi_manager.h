/**
 * @file wifi_manager.h
 * @brief WiFi Connection Manager for ESP32 NanoBot
 */

#ifndef WIFI_MANAGER_H
#define WIFI_MANAGER_H

#include "esp_err.h"
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Initialize WiFi manager
 * @return ESP_OK on success
 */
esp_err_t wifi_manager_init(void);

/**
 * @brief Check if WiFi is connected
 * @return true if connected, false otherwise
 */
bool wifi_manager_is_connected(void);

/**
 * @brief Get current IP address as string
 * @param buf Buffer to store IP address (min 16 bytes)
 * @param len Length of buffer
 * @return ESP_OK on success
 */
esp_err_t wifi_manager_get_ip(char *buf, size_t len);

#ifdef __cplusplus
}
#endif

#endif // WIFI_MANAGER_H
