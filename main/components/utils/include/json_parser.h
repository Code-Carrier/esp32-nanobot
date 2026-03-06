/**
 * @file json_parser.h
 * @brief Simple JSON Parser Utilities
 */

#ifndef JSON_PARSER_H
#define JSON_PARSER_H

#include "esp_err.h"
#include "cJSON.h"
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Get string value from JSON object
 * @param obj JSON object
 * @param key Key name
 * @param default_value Default value if not found
 * @return String value or default
 */
const char* json_get_string(cJSON *obj, const char *key, const char *default_value);

/**
 * @brief Get integer value from JSON object
 * @param obj JSON object
 * @param key Key name
 * @param default_value Default value if not found
 * @return Integer value or default
 */
int json_get_int(cJSON *obj, const char *key, int default_value);

/**
 * @brief Get boolean value from JSON object
 * @param obj JSON object
 * @param key Key name
 * @param default_value Default value if not found
 * @return Boolean value or default
 */
bool json_get_bool(cJSON *obj, const char *key, bool default_value);

/**
 * @brief Create JSON object with string
 * @param key Key name
 * @param value String value
 * @return JSON object
 */
cJSON* json_create_string_obj(const char *key, const char *value);

#ifdef __cplusplus
}
#endif

#endif // JSON_PARSER_H
