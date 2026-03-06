/**
 * @file git_commits.h
 * @brief Git Commit Log Parser
 */

#ifndef GIT_COMMITS_H
#define GIT_COMMITS_H

#include <stddef.h>
#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Git commit structure
 */
typedef struct {
    char hash[16];
    char author[64];
    char date[32];
    char message[256];
    char files_changed[512];
} git_commit_t;

/**
 * @brief Parse git log output
 * @param log_output Raw git log output
 * @param commits Array to store parsed commits
 * @param max_commits Maximum number of commits to parse
 * @return Number of commits parsed
 */
int git_commits_parse(const char *log_output, git_commit_t *commits, int max_commits);

/**
 * @brief Format commits for AI prompt
 * @param commits Array of parsed commits
 * @param num_commits Number of commits
 * @param buffer Output buffer
 * @param buffer_size Size of output buffer
 * @return ESP_OK on success
 */
esp_err_t git_commits_format_prompt(
    const git_commit_t *commits,
    int num_commits,
    char *buffer,
    size_t buffer_size
);

#ifdef __cplusplus
}
#endif

#endif // GIT_COMMITS_H
