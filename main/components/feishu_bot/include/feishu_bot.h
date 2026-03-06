/**
 * @file feishu_bot.h
 * @brief Feishu (飞书) Bot Integration for ESP32 NanoBot
 *
 * Uses Feishu Open Platform API with WebSocket connection
 */

#ifndef FEISHU_BOT_H
#define FEISHU_BOT_H

#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Initialize Feishu Bot
 * @return ESP_OK on success
 */
esp_err_t feishu_bot_init(void);

/**
 * @brief Feishu Bot main task
 * @param pvParameters Task parameters
 */
void feishu_bot_task(void *pvParameters);

/**
 * @brief Send text message via Feishu
 * @param open_id Receiver's Open ID
 * @param message Text message to send
 * @return ESP_OK on success
 */
esp_err_t feishu_bot_send_text(const char *open_id, const char *message);

/**
 * @brief Send message with rich text (interactive card)
 * @param open_id Receiver's Open ID
 * @param content JSON formatted interactive card content
 * @return ESP_OK on success
 */
esp_err_t feishu_bot_send_interactive(const char *open_id, const char *content);

/**
 * @brief Send message with markdown format
 * @param open_id Receiver's Open ID
 * @param markdown Markdown formatted message (Feishu supports limited markdown)
 * @return ESP_OK on success
 */
esp_err_t feishu_bot_send_markdown(const char *open_id, const char *markdown);

#ifdef __cplusplus
}
#endif

#endif // FEISHU_BOT_H
