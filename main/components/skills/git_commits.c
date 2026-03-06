/**
 * @file git_commits.c
 * @brief Git Commit Log Parser Implementation
 */

#include "git_commits.h"
#include "esp_err.h"
#include <string.h>
#include <stdio.h>
#include <ctype.h>

#define MAX_LINE_LEN 512

/**
 * @brief Trim whitespace from string
 */
static char* trim(char *str)
{
    if (!str) return NULL;

    // Trim leading space
    while (isspace((unsigned char)*str)) str++;

    if (*str == 0) return str;

    // Trim trailing space
    char *end = str + strlen(str) - 1;
    while (end > str && isspace((unsigned char)*end)) end--;
    end[1] = '\0';

    return str;
}

int git_commits_parse(const char *log_output, git_commit_t *commits, int max_commits)
{
    if (!log_output || !commits || max_commits <= 0) {
        return 0;
    }

    int num_commits = 0;
    char line[MAX_LINE_LEN];
    const char *ptr = log_output;
    git_commit_t *current = NULL;
    enum {
        STATE_COMMIT,
        STATE_AUTHOR,
        STATE_DATE,
        STATE_MESSAGE,
        STATE_FILES
    } state = STATE_COMMIT;

    while (*ptr && num_commits < max_commits) {
        // Read line
        int i = 0;
        while (*ptr && *ptr != '\n' && i < MAX_LINE_LEN - 1) {
            line[i++] = *ptr++;
        }
        line[i] = '\0';
        ptr++; // Skip newline

        // Check for commit start
        if (strncmp(line, "commit ", 7) == 0) {
            if (current && num_commits < max_commits) {
                num_commits++;
            }
            if (num_commits < max_commits) {
                current = &commits[num_commits];
                memset(current, 0, sizeof(git_commit_t));
                strncpy(current->hash, trim(line + 7), sizeof(current->hash) - 1);
                state = STATE_AUTHOR;
            }
        } else if (state == STATE_AUTHOR && strncmp(line, "Author: ", 8) == 0) {
            strncpy(current->author, trim(line + 8), sizeof(current->author) - 1);
            state = STATE_DATE;
        } else if (state == STATE_DATE && strncmp(line, "Date: ", 6) == 0) {
            strncpy(current->date, trim(line + 6), sizeof(current->date) - 1);
            state = STATE_MESSAGE;
        } else if (state == STATE_MESSAGE) {
            char *trimmed = trim(line);
            if (strlen(trimmed) > 0) {
                // Append to message
                if (strlen(current->message) > 0) {
                    strncat(current->message, " ", sizeof(current->message) - strlen(current->message) - 1);
                }
                strncat(current->message, trimmed, sizeof(current->message) - strlen(current->message) - 1);
            }
        } else if (strncmp(line, "Files: ", 7) == 0) {
            strncpy(current->files_changed, trim(line + 7), sizeof(current->files_changed) - 1);
            state = STATE_FILES;
        }
    }

    // Count last commit
    if (current && num_commits < max_commits) {
        num_commits++;
    }

    return num_commits;
}

esp_err_t git_commits_format_prompt(
    const git_commit_t *commits,
    int num_commits,
    char *buffer,
    size_t buffer_size
)
{
    if (!commits || !buffer || num_commits <= 0) {
        return ESP_ERR_INVALID_ARG;
    }

    size_t offset = 0;

    // Header
    offset += snprintf(buffer + offset, buffer_size - offset,
                       "以下是我今天的 git 提交记录，共 %d 条：\n\n", num_commits);

    // List commits
    for (int i = 0; i < num_commits; i++) {
        offset += snprintf(buffer + offset, buffer_size - offset,
                           "%d. [%s] %s\n",
                           i + 1,
                           commits[i].hash,
                           commits[i].message);

        if (strlen(commits[i].files_changed) > 0) {
            offset += snprintf(buffer + offset, buffer_size - offset,
                               "   修改文件：%s\n", commits[i].files_changed);
        }
    }

    // Prompt
    offset += snprintf(buffer + offset, buffer_size - offset,
                       "\n请根据以上提交记录生成工作总结。");

    return ESP_OK;
}
