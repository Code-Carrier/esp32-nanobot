/**
 * @file ai_provider.cpp
 * @brief AI Provider Implementation for Chinese LLM APIs
 */

#include "ai_provider.h"
#include "esp_log.h"
#include "esp_http_client.h"
#include "cJSON.h"
#include <string.h>
#include <stdio.h>

static const char *TAG = "ai_provider";

// Provider configurations
static const char *PROVIDER_NAMES[] = {
    "deepseek", "moonshot", "dashscope", "siliconflow", "volcengine", "zhipu", "openrouter"
};

static const char *DEFAULT_BASE_URLS[] = {
    "https://api.deepseek.com",
    "https://api.moonshot.cn",
    "https://dashscope.aliyuncs.com",
    "https://api.siliconflow.cn",
    "https://ark.cn-beijing.volces.com",
    "https://open.bigmodel.cn",
    "https://openrouter.ai"
};

static ai_provider_config_t s_config = {0};
static bool s_initialized = false;

// Get API endpoint path based on provider type
static const char* get_chat_endpoint(ai_provider_type_t type)
{
    switch (type) {
        case AI_PROVIDER_DASHSCOPE:
            return "/compatible-mode/v1/chat/completions";
        case AI_PROVIDER_ZHIPU:
            return "/api/paas/v4/chat/completions";
        default:
            return "/v1/chat/completions";
    }
}

// Get model name based on provider type
static const char* get_model_name(ai_provider_type_t type, const char *custom_model)
{
    if (custom_model && strlen(custom_model) > 0) {
        return custom_model;
    }

    switch (type) {
        case AI_PROVIDER_DEEPSEEK:
            return "deepseek-chat";
        case AI_PROVIDER_MOONSHOT:
            return "moonshot-v1-8k";
        case AI_PROVIDER_DASHSCOPE:
            return "qwen-plus";
        case AI_PROVIDER_SILICONFLOW:
            return "Qwen/Qwen2.5-72B-Instruct";
        case AI_PROVIDER_VOLCENGINE:
            return "Doubao-pro-4k";
        case AI_PROVIDER_ZHIPU:
            return "glm-4";
        case AI_PROVIDER_OPENROUTER:
            return "deepseek/deepseek-chat";
        default:
            return "deepseek-chat";
    }
}

// Get content from JSON response
static char* parse_response(const char *json_response, size_t *content_len)
{
    cJSON *root = cJSON_Parse(json_response);
    if (!root) {
        ESP_LOGE(TAG, "Failed to parse JSON response");
        return NULL;
    }

    char *content = NULL;

    // Try different response formats
    cJSON *choices = cJSON_GetObjectItem(root, "choices");
    if (choices && cJSON_IsArray(choices) && cJSON_GetArraySize(choices) > 0) {
        cJSON *first_choice = cJSON_GetArrayItem(choices, 0);
        cJSON *message = cJSON_GetObjectItem(first_choice, "message");
        if (message) {
            cJSON *content_item = cJSON_GetObjectItem(message, "content");
            if (content_item && cJSON_IsString(content_item)) {
                content = strdup(content_item->valuestring);
            }
        }
    }

    // Check for error
    if (!content) {
        cJSON *error = cJSON_GetObjectItem(root, "error");
        if (error) {
            char *error_str = cJSON_PrintUnformatted(error);
            ESP_LOGE(TAG, "API Error: %s", error_str);
            free(error_str);
        }
    }

    cJSON_Delete(root);

    if (content && content_len) {
        *content_len = strlen(content);
    }

    return content;
}

// HTTP client event handler
static esp_err_t http_event_handler(esp_http_client_event_t *evt)
{
    static size_t response_len = 0;
    char *response_buffer = (char *)evt->user_data;

    switch (evt->event_id) {
        case HTTP_EVENT_ON_DATA:
            if (response_len + evt->data_len < 4096) {
                memcpy(response_buffer + response_len, evt->data, evt->data_len);
                response_len += evt->data_len;
            }
            break;

        case HTTP_EVENT_ON_FINISH:
            ESP_LOGD(TAG, "HTTP request finished");
            break;

        default:
            break;
    }

    return ESP_OK;
}

esp_err_t ai_provider_init_with_config(const ai_provider_config_t *config)
{
    if (!config || !config->api_key || strlen(config->api_key) == 0) {
        ESP_LOGE(TAG, "API key not configured!");
        return ESP_ERR_INVALID_ARG;
    }

    s_config = *config;
    if (!s_config.base_url || strlen(s_config.base_url) == 0) {
        s_config.base_url = DEFAULT_BASE_URLS[s_config.type];
    }
    if (s_config.timeout_ms <= 0) s_config.timeout_ms = 30000;
    if (s_config.max_tokens <= 0) s_config.max_tokens = 2048;
    if (s_config.temperature <= 0.0f) s_config.temperature = 0.7f;

    ESP_LOGI(TAG, "AI Provider initialized: %s", PROVIDER_NAMES[s_config.type]);
    ESP_LOGI(TAG, "  Base URL: %s", s_config.base_url);
    ESP_LOGI(TAG, "  Model: %s", s_config.model ? s_config.model : "(default)");

    s_initialized = true;
    return ESP_OK;
}

esp_err_t ai_provider_init(void)
{
    ai_provider_config_t cfg = {
        .type = AI_PROVIDER_DEEPSEEK,
        .api_key = CONFIG_AI_PROVIDER_API_KEY,
        .base_url = CONFIG_AI_PROVIDER_BASE_URL,
        .model = CONFIG_AI_PROVIDER_MODEL,
        .timeout_ms = 30000,
        .max_tokens = 2048,
        .temperature = 0.7f,
    };

    const char *type_str = CONFIG_AI_PROVIDER_TYPE;
    for (int i = 0; i < sizeof(PROVIDER_NAMES) / sizeof(PROVIDER_NAMES[0]); i++) {
        if (strcmp(type_str, PROVIDER_NAMES[i]) == 0) {
            cfg.type = (ai_provider_type_t)i;
            break;
        }
    }

    return ai_provider_init_with_config(&cfg);
}

esp_err_t ai_provider_deinit(void)
{
    s_initialized = false;
    return ESP_OK;
}

bool ai_provider_is_ready(void)
{
    return s_initialized;
}

esp_err_t ai_provider_chat(
    const ai_message_t *messages,
    size_t num_messages,
    char *response_buffer,
    size_t buffer_size
)
{
    if (!s_initialized) {
        ESP_LOGE(TAG, "AI Provider not initialized");
        return ESP_ERR_INVALID_STATE;
    }

    if (!messages || num_messages == 0 || !response_buffer || buffer_size == 0) {
        return ESP_ERR_INVALID_ARG;
    }

    // Build request JSON
    cJSON *root = cJSON_CreateObject();
    cJSON_AddStringToObject(root, "model", get_model_name(s_config.type, s_config.model));

    // Add messages array
    cJSON *messages_array = cJSON_CreateArray();
    for (size_t i = 0; i < num_messages; i++) {
        cJSON *msg = cJSON_CreateObject();
        const char *role_str;
        switch (messages[i].role) {
            case AI_ROLE_SYSTEM: role_str = "system"; break;
            case AI_ROLE_USER: role_str = "user"; break;
            case AI_ROLE_ASSISTANT: role_str = "assistant"; break;
            default: role_str = "user";
        }
        cJSON_AddStringToObject(msg, "role", role_str);
        cJSON_AddStringToObject(msg, "content", messages[i].content);
        cJSON_AddItemToArray(messages_array, msg);
    }
    cJSON_AddItemToObject(root, "messages", messages_array);

    // Add parameters
    cJSON_AddNumberToObject(root, "temperature", s_config.temperature);
    cJSON_AddNumberToObject(root, "max_tokens", s_config.max_tokens);
    cJSON_AddBoolToObject(root, "stream", false);

    char *request_body = cJSON_PrintUnformatted(root);
    cJSON_Delete(root);

    ESP_LOGD(TAG, "Request: %s", request_body);

    // Configure HTTP client
    char url[256];
    snprintf(url, sizeof(url), "%s%s", s_config.base_url, get_chat_endpoint(s_config.type));

    esp_http_client_config_t http_config = {
        .url = url,
        .method = HTTP_METHOD_POST,
        .timeout_ms = s_config.timeout_ms,
        .buffer_size = 2048,
        .user_data = response_buffer,
    };

    esp_http_client_handle_t client = esp_http_client_init(&http_config);
    if (!client) {
        free(request_body);
        ESP_LOGE(TAG, "Failed to initialize HTTP client");
        return ESP_FAIL;
    }

    // Set headers
    char auth_header[128];
    snprintf(auth_header, sizeof(auth_header), "Bearer %s", s_config.api_key);
    esp_http_client_set_header(client, "Authorization", auth_header);
    esp_http_client_set_header(client, "Content-Type", "application/json");
    esp_http_client_set_header(client, "Accept", "application/json");

    // Set POST data
    esp_http_client_set_post_field(client, request_body, strlen(request_body));

    // Perform request
    memset(response_buffer, 0, buffer_size);
    esp_err_t err = esp_http_client_perform(client);

    if (err == ESP_OK) {
        int status_code = esp_http_client_get_status_code(client);
        ESP_LOGD(TAG, "HTTP Status: %d", status_code);

        if (status_code == 200) {
            // Parse response
            size_t content_len = 0;
            char *content = parse_response(response_buffer, &content_len);
            if (content) {
                strncpy(response_buffer, content, buffer_size - 1);
                response_buffer[buffer_size - 1] = '\0';
                free(content);
                ESP_LOGD(TAG, "Response: %s", response_buffer);
            }
        } else {
            ESP_LOGE(TAG, "API request failed with status %d", status_code);
            err = ESP_FAIL;
        }
    } else {
        ESP_LOGE(TAG, "HTTP request failed: %s", esp_err_to_name(err));
    }

    esp_http_client_cleanup(client);
    free(request_body);

    return err;
}

esp_err_t ai_provider_chat_stream(
    const ai_message_t *messages,
    size_t num_messages,
    ai_response_callback_t callback,
    void *user_data
)
{
    // Streaming not fully implemented in C version
    // Use blocking version instead
    char buffer[4096];
    esp_err_t err = ai_provider_chat(messages, num_messages, buffer, sizeof(buffer));

    if (err == ESP_OK && callback) {
        callback(buffer, user_data);
    }

    return err;
}
