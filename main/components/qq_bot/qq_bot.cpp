/**
 * @file qq_bot.cpp
 * @brief QQ Bot Implementation
 */

#include "qq_bot.h"
#include "esp_log.h"
#include "esp_http_client.h"
#include "esp_timer.h"
#include "cJSON.h"
#include <string.h>
#include <stdio.h>

static const char *TAG = "qq_bot";

// QQ API endpoints
static const char *QQ_API_BASE = "https://api.sgroup.qq.com";
static const char *QQ_TOKEN_URL = "https://bots.qq.com/app/getAppAccessToken";

// Access token (valid for 2 hours)
static char s_access_token[256] = {0};
static int64_t s_token_expire_time = 0;

// Bot configuration
static char s_app_id[64] = {0};
static char s_app_secret[64] = {0};

/**
 * @brief Get QQ access token
 */
static esp_err_t qq_bot_get_token(void)
{
    int64_t now = esp_timer_get_time() / 1000000;
    if (now < s_token_expire_time - 300) {
        // Token still valid (5 min buffer)
        return ESP_OK;
    }

    // Build token request
    cJSON *req = cJSON_CreateObject();
    cJSON_AddStringToObject(req, "appId", s_app_id);
    cJSON_AddStringToObject(req, "clientSecret", s_app_secret);
    char *post_data = cJSON_PrintUnformatted(req);
    cJSON_Delete(req);

    esp_http_client_config_t config = {
        .url = QQ_TOKEN_URL,
        .method = HTTP_METHOD_POST,
        .timeout_ms = 10000,
    };

    esp_http_client_handle_t client = esp_http_client_init(&config);
    if (!client) {
        free(post_data);
        return ESP_FAIL;
    }

    esp_http_client_set_header(client, "Content-Type", "application/json");
    esp_http_client_set_post_field(client, post_data, strlen(post_data));

    char response[512] = {0};
    esp_err_t err = esp_http_client_perform(client);

    if (err == ESP_OK) {
        int status = esp_http_client_get_status_code(client);
        int content_len = esp_http_client_get_content_length(client);

        if (content_len < sizeof(response)) {
            esp_http_client_read(client, response, content_len);
            ESP_LOGD(TAG, "Token response: %s", response);

            cJSON *resp_json = cJSON_Parse(response);
            if (resp_json) {
                cJSON *token_item = cJSON_GetObjectItem(resp_json, "access_token");
                cJSON *expires_item = cJSON_GetObjectItem(resp_json, "expires_in");

                if (token_item && cJSON_IsString(token_item)) {
                    strncpy(s_access_token, token_item->valuestring, sizeof(s_access_token) - 1);

                    int expires_in = expires_item ? expires_item->valueint : 7200;
                    s_token_expire_time = now + expires_in;

                    ESP_LOGI(TAG, "QQ access token refreshed (expires in %ds)", expires_in);
                }

                cJSON_Delete(resp_json);
            }
        }
    }

    esp_http_client_cleanup(client);
    free(post_data);

    return (strlen(s_access_token) > 0) ? ESP_OK : ESP_FAIL;
}

/**
 * @brief Send QQ message API request
 */
static esp_err_t qq_bot_send_message_api(const char *openid, const char *content, int msg_type)
{
    esp_err_t err = qq_bot_get_token();
    if (err != ESP_OK) {
        return err;
    }

    // Build API URL
    char url[256];
    snprintf(url, sizeof(url), "%s/v2/users/%s/messages", QQ_API_BASE, openid);

    // Build message payload
    cJSON *req = cJSON_CreateObject();
    cJSON_AddStringToObject(req, "content", content);
    cJSON_AddNumberToObject(req, "msg_type", msg_type);
    cJSON_AddNullToObject(req, "media");

    char *post_data = cJSON_PrintUnformatted(req);
    cJSON_Delete(req);

    esp_http_client_config_t config = {
        .url = url,
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
    snprintf(auth_header, sizeof(auth_header), "QQBot %s", s_access_token);
    esp_http_client_set_header(client, "Authorization", auth_header);
    esp_http_client_set_header(client, "Content-Type", "application/json");

    esp_http_client_set_post_field(client, post_data, strlen(post_data));

    char response[256] = {0};
    err = esp_http_client_perform(client);

    if (err == ESP_OK) {
        int status = esp_http_client_get_status_code(client);
        ESP_LOGD(TAG, "QQ message status: %d", status);

        if (status != 200 && status != 202) {
            int len = esp_http_client_read(client, response, sizeof(response) - 1);
            ESP_LOGE(TAG, "QQ send failed: %s", response);
            err = ESP_FAIL;
        }
    }

    esp_http_client_cleanup(client);
    free(post_data);

    return err;
}

esp_err_t qq_bot_init_with_config(const char *app_id, const char *app_secret)
{
    if (!app_id || !app_secret || strlen(app_id) == 0 || strlen(app_secret) == 0) {
        ESP_LOGE(TAG, "QQ App ID or Secret not configured");
        return ESP_ERR_INVALID_ARG;
    }

    strncpy(s_app_id, app_id, sizeof(s_app_id) - 1);
    s_app_id[sizeof(s_app_id) - 1] = '\0';
    strncpy(s_app_secret, app_secret, sizeof(s_app_secret) - 1);
    s_app_secret[sizeof(s_app_secret) - 1] = '\0';

    size_t id_len = strlen(s_app_id);
    const char *mask = id_len >= 4 ? s_app_id + id_len - 4 : s_app_id;
    ESP_LOGI(TAG, "QQ Bot initialized (AppID: %s****)", mask);

    // Initial token fetch
    return qq_bot_get_token();
}

esp_err_t qq_bot_init(void)
{
    return qq_bot_init_with_config(CONFIG_QQ_APP_ID, CONFIG_QQ_APP_SECRET);
}

void qq_bot_task(void *pvParameters)
{
    ESP_LOGI(TAG, "QQ Bot task started");

    // Main loop - will receive messages via polling or WebSocket
    // For now, just keep token fresh
    while (1) {
        qq_bot_get_token();
        vTaskDelay(pdMS_TO_TICKS(60000)); // Check every minute
    }
}

esp_err_t qq_bot_send_text(const char *openid, const char *message)
{
    if (!openid || !message) {
        return ESP_ERR_INVALID_ARG;
    }

    ESP_LOGI(TAG, "Sending QQ text to %s: %s", openid, message);
    return qq_bot_send_message_api(openid, message, 0); // 0 = text message
}

esp_err_t qq_bot_send_markdown(const char *openid, const char *markdown)
{
    if (!openid || !markdown) {
        return ESP_ERR_INVALID_ARG;
    }

    ESP_LOGI(TAG, "Sending QQ markdown to %s", openid);
    return qq_bot_send_message_api(openid, markdown, 2); // 2 = markdown message
}
