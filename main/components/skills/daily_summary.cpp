/**
 * @file daily_summary.cpp
 * @brief Daily Work Summary Skill Implementation
 */

#include "daily_summary.h"
#include "git_commits.h"
#include "ai_provider.h"
#include "qq_bot.h"
#include "feishu_bot.h"
#include "esp_log.h"
#include "esp_system.h"
#include "esp_http_client.h"
#include "cJSON.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

static const char *TAG = "daily_summary";

#ifndef CONFIG_GITHUB_API_ENABLED
#define CONFIG_GITHUB_API_ENABLED 0
#endif

#ifndef CONFIG_GITHUB_OWNER
#define CONFIG_GITHUB_OWNER ""
#endif

#ifndef CONFIG_GITHUB_REPO
#define CONFIG_GITHUB_REPO ""
#endif

#ifndef CONFIG_GITHUB_TOKEN
#define CONFIG_GITHUB_TOKEN ""
#endif

#ifndef CONFIG_GITHUB_AUTHOR
#define CONFIG_GITHUB_AUTHOR ""
#endif

// Configuration
static char s_recipient_id[128] = {0};  // User's QQ OpenID or Feishu open_id
static char s_channel[16] = {0};         // "qq" or "feishu"
static char s_send_hour[8] = {0};        // Hour to send (HH)
static char s_send_minute[8] = {0};      // Minute to send (MM)

/**
 * @brief Get today's date string
 */
static void get_today_date(char *buffer, size_t size)
{
    time_t now;
    struct tm timeinfo;
    time(&now);
    localtime_r(&now, &timeinfo);
    strftime(buffer, size, "%Y-%m-%d", &timeinfo);
}

/**
 * @brief Check if current time matches send time
 */
static bool is_send_time(void)
{
    time_t now;
    struct tm timeinfo;
    time(&now);
    localtime_r(&now, &timeinfo);

    int current_hour = timeinfo.tm_hour;
    int current_min = timeinfo.tm_min;

    int send_h = atoi(s_send_hour);
    int send_m = atoi(s_send_minute);

    return (current_hour == send_h && current_min == send_m);
}

/**
 * @brief Build GitHub commits API query time range for current local day
 */
static void build_today_iso_range(char *since, size_t since_size, char *until, size_t until_size)
{
    time_t now;
    struct tm local_tm = {0};
    time(&now);
    localtime_r(&now, &local_tm);

    struct tm start_local = local_tm;
    start_local.tm_hour = 0;
    start_local.tm_min = 0;
    start_local.tm_sec = 0;

    struct tm end_local = local_tm;
    end_local.tm_hour = 23;
    end_local.tm_min = 59;
    end_local.tm_sec = 59;

    time_t start_t = mktime(&start_local);
    time_t end_t = mktime(&end_local);

    struct tm start_utc = {0};
    struct tm end_utc = {0};
    gmtime_r(&start_t, &start_utc);
    gmtime_r(&end_t, &end_utc);

    strftime(since, since_size, "%Y-%m-%dT%H:%M:%SZ", &start_utc);
    strftime(until, until_size, "%Y-%m-%dT%H:%M:%SZ", &end_utc);
}

static esp_err_t fetch_today_commits_from_github(git_commit_t *commits, int max_commits, int *num_commits)
{
    if (!commits || max_commits <= 0 || !num_commits) {
        return ESP_ERR_INVALID_ARG;
    }

    *num_commits = 0;

#if CONFIG_GITHUB_API_ENABLED
    if (strlen(CONFIG_GITHUB_OWNER) == 0 || strlen(CONFIG_GITHUB_REPO) == 0) {
        ESP_LOGW(TAG, "GitHub API enabled but owner/repo not configured");
        return ESP_ERR_INVALID_STATE;
    }

    char since[32] = {0};
    char until[32] = {0};
    build_today_iso_range(since, sizeof(since), until, sizeof(until));

    char url[512] = {0};
    if (strlen(CONFIG_GITHUB_AUTHOR) > 0) {
        snprintf(url, sizeof(url),
                 "https://api.github.com/repos/%s/%s/commits?since=%s&until=%s&author=%s&per_page=%d",
                 CONFIG_GITHUB_OWNER, CONFIG_GITHUB_REPO, since, until, CONFIG_GITHUB_AUTHOR, max_commits);
    } else {
        snprintf(url, sizeof(url),
                 "https://api.github.com/repos/%s/%s/commits?since=%s&until=%s&per_page=%d",
                 CONFIG_GITHUB_OWNER, CONFIG_GITHUB_REPO, since, until, max_commits);
    }

    esp_http_client_config_t config = {
        .url = url,
        .method = HTTP_METHOD_GET,
        .timeout_ms = 10000,
    };

    esp_http_client_handle_t client = esp_http_client_init(&config);
    if (!client) {
        return ESP_FAIL;
    }

    esp_http_client_set_header(client, "Accept", "application/vnd.github+json");
    esp_http_client_set_header(client, "User-Agent", "esp32-nanobot");

    char auth_header[192] = {0};
    if (strlen(CONFIG_GITHUB_TOKEN) > 0) {
        snprintf(auth_header, sizeof(auth_header), "Bearer %s", CONFIG_GITHUB_TOKEN);
        esp_http_client_set_header(client, "Authorization", auth_header);
    }

    esp_err_t err = esp_http_client_perform(client);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "GitHub commits request failed: %s", esp_err_to_name(err));
        esp_http_client_cleanup(client);
        return err;
    }

    int status = esp_http_client_get_status_code(client);
    if (status != 200) {
        ESP_LOGW(TAG, "GitHub commits API status=%d", status);
        esp_http_client_cleanup(client);
        return ESP_FAIL;
    }

    int content_len = esp_http_client_get_content_length(client);
    if (content_len <= 0 || content_len > 8192) {
        ESP_LOGW(TAG, "Unexpected GitHub response length: %d", content_len);
        esp_http_client_cleanup(client);
        return ESP_FAIL;
    }

    char *response = (char *)malloc(content_len + 1);
    if (!response) {
        esp_http_client_cleanup(client);
        return ESP_ERR_NO_MEM;
    }

    int read_len = esp_http_client_read_response(client, response, content_len);
    response[(read_len > 0) ? read_len : 0] = '\0';
    esp_http_client_cleanup(client);

    if (read_len <= 0) {
        free(response);
        return ESP_FAIL;
    }

    cJSON *root = cJSON_Parse(response);
    free(response);
    if (!root || !cJSON_IsArray(root)) {
        if (root) cJSON_Delete(root);
        return ESP_FAIL;
    }

    int count = cJSON_GetArraySize(root);
    for (int i = 0; i < count && i < max_commits; i++) {
        cJSON *item = cJSON_GetArrayItem(root, i);
        cJSON *sha = cJSON_GetObjectItem(item, "sha");
        cJSON *commit = cJSON_GetObjectItem(item, "commit");
        cJSON *author_obj = commit ? cJSON_GetObjectItem(commit, "author") : NULL;
        cJSON *message = commit ? cJSON_GetObjectItem(commit, "message") : NULL;
        cJSON *author_name = author_obj ? cJSON_GetObjectItem(author_obj, "name") : NULL;
        cJSON *author_date = author_obj ? cJSON_GetObjectItem(author_obj, "date") : NULL;

        if (!cJSON_IsString(sha) || !cJSON_IsString(message)) {
            continue;
        }

        git_commit_t *dst = &commits[*num_commits];
        memset(dst, 0, sizeof(git_commit_t));

        strncpy(dst->hash, sha->valuestring, sizeof(dst->hash) - 1);
        dst->hash[8] = '\0';
        strncpy(dst->message, message->valuestring, sizeof(dst->message) - 1);

        if (cJSON_IsString(author_name)) {
            strncpy(dst->author, author_name->valuestring, sizeof(dst->author) - 1);
        }
        if (cJSON_IsString(author_date)) {
            strncpy(dst->date, author_date->valuestring, sizeof(dst->date) - 1);
        }

        (*num_commits)++;
        if (*num_commits >= max_commits) {
            break;
        }
    }

    cJSON_Delete(root);

    ESP_LOGI(TAG, "Fetched %d commits from GitHub API", *num_commits);
    return (*num_commits > 0) ? ESP_OK : ESP_ERR_NOT_FOUND;
#else
    return ESP_ERR_NOT_SUPPORTED;
#endif
}

/**
 * @brief Get today's commits
 *
 * Tries GitHub API first (if configured), then falls back to simulated data.
 */
static esp_err_t get_today_commits(git_commit_t *commits, int max_commits, int *num_commits)
{
    esp_err_t err = fetch_today_commits_from_github(commits, max_commits, num_commits);
    if (err == ESP_OK || err == ESP_ERR_NOT_FOUND) {
        return err;
    }

    ESP_LOGW(TAG, "Using simulated commits (GitHub API unavailable or not configured)");

    const char *simulated_log =
        "commit a1b2c3d4\n"
        "Author: Developer\n"
        "Date: " __DATE__ "\n"
        "\n"
        "    Fixed bug in login module\n"
        "\n"
        "commit e5f6g7h8\n"
        "Author: Developer\n"
        "Date: " __DATE__ "\n"
        "\n"
        "    Added new feature for user profile\n"
        "\n"
        "commit i9j0k1l2\n"
        "Author: Developer\n"
        "Date: " __DATE__ "\n"
        "\n"
        "    Updated documentation\n";

    *num_commits = git_commits_parse(simulated_log, commits, max_commits);
    ESP_LOGI(TAG, "Found %d commits today", *num_commits);

    return (*num_commits > 0) ? ESP_OK : ESP_ERR_NOT_FOUND;
}

esp_err_t daily_summary_generate(char *summary_buffer, size_t buffer_size)
{
    if (!summary_buffer || buffer_size == 0) {
        return ESP_ERR_INVALID_ARG;
    }

    // Get today's commits
    git_commit_t commits[20];
    int num_commits = 0;
    esp_err_t err = get_today_commits(commits, 20, &num_commits);

    if (err != ESP_OK || num_commits == 0) {
        snprintf(summary_buffer, buffer_size, "今天没有提交记录。");
        return ESP_OK;
    }

    // Format commits for AI prompt
    char commits_prompt[2048];
    err = git_commits_format_prompt(commits, num_commits, commits_prompt, sizeof(commits_prompt));
    if (err != ESP_OK) {
        return err;
    }

    // Build AI prompt
    char date_str[32];
    get_today_date(date_str, sizeof(date_str));

    ai_message_t messages[] = {
        {
            AI_ROLE_SYSTEM,
            "你是一个工作总结助手。请根据用户的 git commit 记录，生成一份简洁专业的每日工作总结。"
            "总结应该包括：\n"
            "1. 今日完成的主要工作（按重要性排序）\n"
            "2. 技术亮点或突破\n"
            "3. 明日计划建议\n"
            "请使用简洁的中文，分点列出，总字数控制在 300 字以内。"
        },
        {
            AI_ROLE_USER,
            commits_prompt
        }
    };

    ESP_LOGI(TAG, "Generating summary with AI...");

    // Call AI provider
    err = ai_provider_chat(messages, 2, summary_buffer, buffer_size);

    if (err == ESP_OK) {
        ESP_LOGI(TAG, "Summary generated successfully");
    } else {
        ESP_LOGE(TAG, "Failed to generate summary");
        snprintf(summary_buffer, buffer_size, "生成总结失败，请稍后重试。");
    }

    return err;
}

esp_err_t daily_summary_send(const char *recipient_id, const char *channel)
{
    if (!recipient_id || !channel) {
        return ESP_ERR_INVALID_ARG;
    }

    char summary[4096];
    esp_err_t err = daily_summary_generate(summary, sizeof(summary));

    if (err != ESP_OK) {
        return err;
    }

    // Add header
    char date_str[32];
    get_today_date(date_str, sizeof(date_str));

    char full_message[4096];
    snprintf(full_message, sizeof(full_message),
             "📅 每日工作总结\n"
             "日期：%s\n"
             "━━━━━━━━━━━━━━━━\n\n"
             "%s\n\n"
             "━━━━━━━━━━━━━━━━\n"
             "明天继续加油！💪",
             date_str, summary);

    // Send via configured channel
    if (strcmp(channel, "qq") == 0) {
        err = qq_bot_send_markdown(recipient_id, full_message);
    } else if (strcmp(channel, "feishu") == 0) {
        err = feishu_bot_send_text(recipient_id, full_message);
    } else {
        ESP_LOGE(TAG, "Unknown channel: %s", channel);
        return ESP_ERR_INVALID_ARG;
    }

    return err;
}

esp_err_t daily_summary_init(void)
{
    // Parse send time from config (format: HH:MM)
    strncpy(s_send_hour, CONFIG_SUMMARY_SEND_TIME, 2);
    s_send_hour[2] = '\0';
    strncpy(s_send_minute, CONFIG_SUMMARY_SEND_TIME + 3, 2);
    s_send_minute[2] = '\0';

    // Default channel preference
#ifdef CONFIG_QQ_BOT_ENABLED
    strcpy(s_channel, "qq");
    // Note: In production, recipient_id should be stored in NVS or configured
    strcpy(s_recipient_id, "user_openid_placeholder");
#elif defined(CONFIG_FEISHU_BOT_ENABLED)
    strcpy(s_channel, "feishu");
    strcpy(s_recipient_id, "user_open_id_placeholder");
#endif

    ESP_LOGI(TAG, "Daily Summary initialized");
    ESP_LOGI(TAG, "  Send time: %s:%s", s_send_hour, s_send_minute);
    ESP_LOGI(TAG, "  Channel: %s", s_channel);

    return ESP_OK;
}

void daily_summary_task(void *pvParameters)
{
    ESP_LOGI(TAG, "Daily Summary task started");

    while (1) {
        // Check if it's send time
        if (is_send_time() && strlen(s_recipient_id) > 0) {
            ESP_LOGI(TAG, "Sending daily summary...");

            esp_err_t err = daily_summary_send(s_recipient_id, s_channel);

            if (err == ESP_OK) {
                ESP_LOGI(TAG, "Daily summary sent successfully");
            } else {
                ESP_LOGE(TAG, "Failed to send daily summary: %s", esp_err_to_name(err));
            }

            // Wait a minute to avoid sending multiple times
            vTaskDelay(pdMS_TO_TICKS(60000));
        }

        // Check every 30 seconds
        vTaskDelay(pdMS_TO_TICKS(30000));
    }
}
