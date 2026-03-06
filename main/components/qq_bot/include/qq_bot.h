/**
 * @file qq_bot.h
 * @brief QQ Bot Integration for ESP32 NanoBot
 *
 * Uses QQ Open Platform API with WebSocket connection
 */

#ifndef QQ_BOT_H
#define QQ_BOT_H

#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Initialize QQ Bot
 * @return ESP_OK on success
 */
esp_err_t qq_bot_init(void);

/**
 * @brief QQ Bot main task
 * @param pvParameters Task parameters
 */
void qq_bot_task(void *pvParameters);

/**
 * @brief Send text message via QQ
 * @param openid Receiver's OpenID
 * @param message Text message to send
 * @return ESP_OK on success
 */
esp_err_t qq_bot_send_text(const char *openid, const char *message);

/**
 * @brief Send message with markdown formatting
 * @param openid Receiver's OpenID
 * @param markdown Markdown formatted message
 * @return ESP_OK on success
 */
esp_err_t qq_bot_send_markdown(const char *openid, const char *markdown);

#ifdef __cplusplus
}
#endif

#endif // QQ_BOT_H
