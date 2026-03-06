/**
 * @file ai_provider.h
 * @brief AI Provider Interface for Chinese LLM APIs
 *
 * Supports:
 * - DeepSeek (深智AI)
 * - Moonshot/Kimi (月之暗面)
 * - DashScope/通义千问 (阿里云)
 * - SiliconFlow (硅基流动)
 * - VolcEngine (火山引擎)
 * - Zhipu/智谱 AI
 */

#ifndef AI_PROVIDER_H
#define AI_PROVIDER_H

#include "esp_err.h"
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief AI Provider types
 */
typedef enum {
    AI_PROVIDER_DEEPSEEK,
    AI_PROVIDER_MOONSHOT,
    AI_PROVIDER_DASHSCOPE,
    AI_PROVIDER_SILICONFLOW,
    AI_PROVIDER_VOLCENGINE,
    AI_PROVIDER_ZHIPU,
    AI_PROVIDER_OPENROUTER,
} ai_provider_type_t;

/**
 * @brief AI Provider configuration
 */
typedef struct {
    ai_provider_type_t type;
    const char *api_key;
    const char *base_url;
    const char *model;
    int timeout_ms;
    int max_tokens;
    float temperature;
} ai_provider_config_t;

/**
 * @brief Chat message role
 */
typedef enum {
    AI_ROLE_SYSTEM,
    AI_ROLE_USER,
    AI_ROLE_ASSISTANT,
} ai_role_t;

/**
 * @brief Chat message structure
 */
typedef struct {
    ai_role_t role;
    const char *content;
} ai_message_t;

/**
 * @brief AI Provider response callback
 * @param content Response content
 * @param user_data User data pointer
 */
typedef void (*ai_response_callback_t)(const char *content, void *user_data);

/**
 * @brief Initialize AI Provider
 * @return ESP_OK on success
 */
esp_err_t ai_provider_init(void);

/**
 * @brief Deinitialize AI Provider
 * @return ESP_OK on success
 */
esp_err_t ai_provider_deinit(void);

/**
 * @brief Send chat completion request (blocking)
 * @param messages Array of chat messages
 * @param num_messages Number of messages
 * @param response_buffer Buffer to store response
 * @param buffer_size Size of response buffer
 * @return ESP_OK on success
 */
esp_err_t ai_provider_chat(
    const ai_message_t *messages,
    size_t num_messages,
    char *response_buffer,
    size_t buffer_size
);

/**
 * @brief Send chat completion request (streaming)
 * @param messages Array of chat messages
 * @param num_messages Number of messages
 * @param callback Callback function for streaming response
 * @param user_data User data pointer
 * @return ESP_OK on success
 */
esp_err_t ai_provider_chat_stream(
    const ai_message_t *messages,
    size_t num_messages,
    ai_response_callback_t callback,
    void *user_data
);

/**
 * @brief Check if AI Provider is initialized
 * @return true if initialized
 */
bool ai_provider_is_ready(void);

#ifdef __cplusplus
}
#endif

#endif // AI_PROVIDER_H
