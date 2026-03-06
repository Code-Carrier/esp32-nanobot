/**
 * @file config_web.h
 * @brief Web Server for WiFi AP Configuration Mode
 */

#ifndef CONFIG_WEB_H
#define CONFIG_WEB_H

#include "esp_err.h"
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Start web server for configuration
 * @return ESP_OK on success
 */
esp_err_t config_web_server_start(void);

/**
 * @brief Stop web server
 * @return ESP_OK on success
 */
esp_err_t config_web_server_stop(void);

/**
 * @brief Check if web server is running
 * @return true if running
 */
bool config_web_server_is_running(void);

#ifdef __cplusplus
}
#endif

#endif // CONFIG_WEB_H
