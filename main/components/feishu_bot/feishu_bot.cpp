/**
 * @file feishu_bot.cpp
 * @brief Feishu (飞书) Bot Implementation
 */

#include "feishu_bot.h"
#include "esp_log.h"
#include "esp_http_client.h"
#include "cJSON.h"
#include "esp_timer.h"
#include <string.h>
#include <stdio.h>

static const char *TAG = "feishu_bot";

// Feishu API endpoints
static const char *FEISHU_TOKEN_URL = "https://open.feishu.cn/open-apis/auth/v3/tenant_access_token/internal";
static const char *FEISHU_SEND_URL = "https://open.feishu.cn/open-apis/im/v1/messages";

// Access token (valid for 2 hours)
static char s_tenant_token[256] = {0};
static int64_t s_token_expire_time = 0;

// Bot configuration
static char s_app_id[64] = {0};
static char s_app_secret[128] = {0};

/**
 * @brief Get Feishu tenant access token
 */
static esp_err_t feishu_bot_get_token(void)
{
    int64_t now = esp_timer_get_time() / 1000000;
    if (now < s_token_expire_time - 300) {
        // Token still valid (5 min buffer)
        return ESP_OK;
    }

    // Build token request URL with query parameters
    char url[512];
    snprintf(url, sizeof(url), "%s?app_id=%s&app_secret=%s",
             FEISHU_TOKEN_URL, s_app_id, s_app_secret);

    esp_http_client_config_t config = {
        .url = url,
        .method = HTTP_METHOD_POST,
        .timeout_ms = 10000,
    };

    esp_http_client_handle_t client = esp_http_client_init(&config);
    if (!client) {
        return ESP_FAIL;
    }

    esp_http_client_set_header(client, "Content-Type", "application/json");

    char response[512] = {0};
    esp_err_t err = esp_http_client_perform(client);

    if (err == ESP_OK) {
        esp_http_client_get_status_code(client);
        int content_len = esp_http_client_get_content_length(client);

        if (content_len < sizeof(response)) {
            esp_http_client_read(client, response, content_len);
            ESP_LOGD(TAG, "Token response: %s", response);

            cJSON *resp_json = cJSON_Parse(response);
            if (resp_json) {
                cJSON *data = cJSON_GetObjectItem(resp_json, "data");
                if (data && cJSON_IsObject(data)) {
                    cJSON *token_item = cJSON_GetObjectItem(data, "tenant_access_token");
                    cJSON *expires_item = cJSON_GetObjectItem(data, "expire");

                    if (token_item && cJSON_IsString(token_item)) {
                        strncpy(s_tenant_token, token_item->valuestring, sizeof(s_tenant_token) - 1);

                        int expires_in = expires_item ? expires_item->valueint : 7200;
                        s_token_expire_time = now + expires_in;

                        ESP_LOGI(TAG, "Feishu tenant token refreshed (expires in %ds)", expires_in);
                    }
                }

                cJSON *code_item = cJSON_GetObjectItem(resp_json, "code");
                if (code_item && code_item->valueint != 0) {
                    cJSON *msg_item = cJSON_GetObjectItem(resp_json, "msg");
                    ESP_LOGE(TAG, "Feishu token error: %s", msg_item ? msg_item->valuestring : "unknown");
                }

                cJSON_Delete(resp_json);
            }
        }
    }

    esp_http_client_cleanup(client);

    return (strlen(s_tenant_token) > 0) ? ESP_OK : ESP_FAIL;
}

/**
 * @brief Send Feishu message API request
 */
static esp_err_t feishu_bot_send_message_api(const char *open_id, const char *content, const char *msg_type)
{
    esp_err_t err = feishu_bot_get_token();
    if (err != ESP_OK) {
        return err;
    }

    // Build message payload
    cJSON *req = cJSON_CreateObject();
    cJSON_AddStringToObject(req, "receive_id", open_id);

    cJSON *msg_body = cJSON_CreateObject();
    cJSON_AddStringToObject(msg_body, "text", content);
    cJSON *content_json = cJSON_Duplicate(msg_body, true);
    cJSON_AddItemToObject(req, "content", content_json);
    cJSON_Delete(msg_body);

    cJSON_AddStringToObject(req, "msg_type", msg_type);

    char *post_data = cJSON_PrintUnformatted(req);
    cJSON_Delete(req);

    esp_http_client_config_t config = {
        .url = FEISHU_SEND_URL,
        .method = HTTP_METHOD_POST,
        .timeout_ms = 10000,
    };

    esp_http_client_handle_t client = esp_http_client_init(&config);
    if (!client) {
        free(post_data);
        return ESP_FAIL;
    }

    // Set headers
    char auth_header[128];
    snprintf(auth_header, sizeof(auth_header), "Bearer %s", s_tenant_token);
    esp_http_client_set_header(client, "Authorization", auth_header);
    esp_http_client_set_header(client, "Content-Type", "application/json");

    // Query parameter for open_id type
    esp_http_client_set_header(client, "X-Target-Id-Type", "open_id");

    esp_http_client_set_post_field(client, post_data, strlen(post_data));

    char response[512] = {0};
    err = esp_http_client_perform(client);

    if (err == ESP_OK) {
        int status = esp_http_client_get_status_code(client);
        ESP_LOGD(TAG, "Feishu message status: %d", status);

        int len = esp_http_client_read(client, response, sizeof(response) - 1);
        response[len] = '\0';

        cJSON *resp_json = cJSON_Parse(response);
        if (resp_json) {
            cJSON *code_item = cJSON_GetObjectItem(resp_json, "code");
            if (code_item && code_item->valueint != 0) {
                cJSON *msg_item = cJSON_GetObjectItem(resp_json, "msg");
                ESP_LOGE(TAG, "Feishu send error: %s", msg_item ? msg_item->valuestring : "unknown");
                err = ESP_FAIL;
            } else {
                ESP_LOGI(TAG, "Feishu message sent successfully");
            }
            cJSON_Delete(resp_json);
        }
    }

    esp_http_client_cleanup(client);
    free(post_data);

    return err;
}

esp_err_t feishu_bot_init_with_config(const char *app_id, const char *app_secret)
{
    if (!app_id || !app_secret || strlen(app_id) == 0 || strlen(app_secret) == 0) {
        ESP_LOGE(TAG, "Feishu App ID or Secret not configured");
        return ESP_ERR_INVALID_ARG;
    }

    strncpy(s_app_id, app_id, sizeof(s_app_id) - 1);
    s_app_id[sizeof(s_app_id) - 1] = '\0';
    strncpy(s_app_secret, app_secret, sizeof(s_app_secret) - 1);
    s_app_secret[sizeof(s_app_secret) - 1] = '\0';

    size_t id_len = strlen(s_app_id);
    const char *mask = id_len >= 4 ? s_app_id + id_len - 4 : s_app_id;
    ESP_LOGI(TAG, "Feishu Bot initialized (AppID: %s****)", mask);

    // Initial token fetch
    return feishu_bot_get_token();
}

esp_err_t feishu_bot_init(void)
{
    return feishu_bot_init_with_config(CONFIG_FEISHU_APP_ID, CONFIG_FEISHU_APP_SECRET);
}

void feishu_bot_task(void *pvParameters)
{
    ESP_LOGI(TAG, "Feishu Bot task started");

    // Main loop - keep token fresh
    while (1) {
        feishu_bot_get_token();
        vTaskDelay(pdMS_TO_TICKS(60000)); // Check every minute
    }
}

esp_err_t feishu_bot_send_text(const char *open_id, const char *message)
{
    if (!open_id || !message) {
        return ESP_ERR_INVALID_ARG;
    }

    ESP_LOGI(TAG, "Sending Feishu text to %s: %s", open_id, message);
    return feishu_bot_send_message_api(open_id, message, "text");
}

esp_err_t feishu_bot_send_markdown(const char *open_id, const char *markdown)
{
    if (!open_id || !markdown) {
        return ESP_ERR_INVALID_ARG;
    }

    ESP_LOGI(TAG, "Sending Feishu markdown to %s", open_id);
    // Feishu uses "interactive" message type for rich content including markdown
    return feishu_bot_send_message_api(open_id, markdown, "interactive");
}

esp_err_t feishu_bot_send_interactive(const char *open_id, const char *content)
{
    if (!open_id || !content) {
        return ESP_ERR_INVALID_ARG;
    }

    ESP_LOGI(TAG, "Sending Feishu interactive card to %s", open_id);
    return feishu_bot_send_message_api(open_id, content, "interactive");
}
