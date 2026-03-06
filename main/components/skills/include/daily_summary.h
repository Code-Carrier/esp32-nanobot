/**
 * @file daily_summary.h
 * @brief Daily Work Summary Skill
 *
 * Fetches git commits from today and uses AI to generate a work summary
 */

#ifndef DAILY_SUMMARY_H
#define DAILY_SUMMARY_H

#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Initialize Daily Summary skill
 * @return ESP_OK on success
 */
esp_err_t daily_summary_init(void);

/**
 * @brief Daily Summary main task
 * @param pvParameters Task parameters
 */
void daily_summary_task(void *pvParameters);

/**
 * @brief Generate daily summary from git commits
 * @param summary_buffer Buffer to store the summary
 * @param buffer_size Size of the buffer
 * @return ESP_OK on success
 */
esp_err_t daily_summary_generate(char *summary_buffer, size_t buffer_size);

/**
 * @brief Send daily summary via configured channel (QQ or Feishu)
 * @param recipient_id Recipient's ID (OpenID for QQ, open_id for Feishu)
 * @param channel Channel to use ("qq" or "feishu")
 * @return ESP_OK on success
 */
esp_err_t daily_summary_send(const char *recipient_id, const char *channel);

#ifdef __cplusplus
}
#endif

#endif // DAILY_SUMMARY_H
