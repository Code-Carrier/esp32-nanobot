/**
 * @file json_parser.c
 * @brief JSON Parser Utilities Implementation
 */

#include "json_parser.h"
#include <string.h>
#include <stdbool.h>

const char* json_get_string(cJSON *obj, const char *key, const char *default_value)
{
    if (!obj || !key) {
        return default_value;
    }

    cJSON *item = cJSON_GetObjectItem(obj, key);
    if (item && cJSON_IsString(item)) {
        return item->valuestring;
    }

    return default_value;
}

int json_get_int(cJSON *obj, const char *key, int default_value)
{
    if (!obj || !key) {
        return default_value;
    }

    cJSON *item = cJSON_GetObjectItem(obj, key);
    if (item && cJSON_IsNumber(item)) {
        return item->valueint;
    }

    return default_value;
}

bool json_get_bool(cJSON *obj, const char *key, bool default_value)
{
    if (!obj || !key) {
        return default_value;
    }

    cJSON *item = cJSON_GetObjectItem(obj, key);
    if (item && cJSON_IsBool(item)) {
        return item->valueint == 1;
    }

    return default_value;
}

cJSON* json_create_string_obj(const char *key, const char *value)
{
    cJSON *obj = cJSON_CreateObject();
    if (obj && key && value) {
        cJSON_AddStringToObject(obj, key, value);
    }
    return obj;
}
