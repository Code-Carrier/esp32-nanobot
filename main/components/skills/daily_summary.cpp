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
#include <string.h>
#include <stdio.h>
#include <time.h>

static const char *TAG = "daily_summary";

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
 * @brief Generate git log command for today's commits
 *
 * Simulates git log output for demo purposes
 * In production, this would read from SD card or call git API
 */
static esp_err_t get_today_commits(git_commit_t *commits, int max_commits, int *num_commits)
{
    // For ESP32, we simulate git commits since we can't run git directly
    // In production, this would:
    // 1. Mount SD card with git repository
    // 2. Parse .git/logs or use libgit2
    // 3. Or call a remote git API

    ESP_LOGI(TAG, "Fetching today's commits (simulated)...");

    // Simulated commits - replace with actual git parsing
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
