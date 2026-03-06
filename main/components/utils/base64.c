/*
 * Base64 encoding/decoding utility
 * Public domain implementation
 */

#include <string.h>
#include <stdint.h>

static const char base64_table[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

/**
 * @brief Base64 encode
 */
int base64_encode(const unsigned char *input, int input_len, char *output, int output_len)
{
    int i, j;
    int output_idx = 0;

    if (input_len < 0 || output_len < ((input_len + 2) / 3 * 4 + 1)) {
        return -1;
    }

    for (i = 0; i < input_len; i += 3) {
        uint32_t octet_a = i < input_len ? input[i++] : 0;
        uint32_t octet_b = i < input_len ? input[i++] : 0;
        uint32_t octet_c = i < input_len ? input[i++] : 0;

        uint32_t triple = (octet_a << 16) + (octet_b << 8) + octet_c;

        for (j = 0; j < 4; j++) {
            if (output_idx >= output_len) {
                return -1;
            }

            int index = (triple >> (18 - j * 6)) & 0x3F;
            output[output_idx++] = base64_table[index];
        }
    }

    // Add padding
    int mod = input_len % 3;
    if (mod > 0) {
        for (i = 0; i < 3 - mod; i++) {
            if (output_idx >= output_len) {
                return -1;
            }
            output[output_idx - 1 - i] = '=';
        }
    }

    if (output_idx < output_len) {
        output[output_idx] = '\0';
    }

    return output_idx;
}

/**
 * @brief Base64 decode
 */
int base64_decode(const char *input, int input_len, unsigned char *output, int output_len)
{
    static const unsigned char base64_map[256] = {
        255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
        255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
        255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,  62, 255, 255, 255,  63,
         52,  53,  54,  55,  56,  57,  58,  59,  60,  61, 255, 255, 255, 255, 255, 255,
        255,   0,   1,   2,   3,   4,   5,   6,   7,   8,   9,  10,  11,  12,  13,  14,
         15,  16,  17,  18,  19,  20,  21,  22,  23,  24,  25, 255, 255, 255, 255, 255,
        255,  26,  27,  28,  29,  30,  31,  32,  33,  34,  35,  36,  37,  38,  39,  40,
         41,  42,  43,  44,  45,  46,  47,  48,  49,  50,  51, 255, 255, 255, 255, 255,
        255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
        255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
        255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
        255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
        255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
        255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
        255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
        255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
    };

    int i, j;
    int output_idx = 0;

    if (input_len < 0 || output_len < (input_len / 4 * 3)) {
        return -1;
    }

    for (i = 0, j = 0; i < input_len; i += 4) {
        uint32_t sextet_a = input[i] == '=' ? 0 : base64_map[(unsigned char)input[i]];
        uint32_t sextet_b = i + 1 < input_len && input[i + 1] != '=' ? base64_map[(unsigned char)input[i + 1]] : 0;
        uint32_t sextet_c = i + 2 < input_len && input[i + 2] != '=' ? base64_map[(unsigned char)input[i + 2]] : 0;
        uint32_t sextet_d = i + 3 < input_len && input[i + 3] != '=' ? base64_map[(unsigned char)input[i + 3]] : 0;

        uint32_t triple = (sextet_a << 18) + (sextet_b << 12) + (sextet_c << 6) + sextet_d;

        if (output_idx < output_len) {
            output[output_idx++] = (triple >> 16) & 0xFF;
        }
        if (output_idx < output_len && i + 2 < input_len && input[i + 2] != '=') {
            output[output_idx++] = (triple >> 8) & 0xFF;
        }
        if (output_idx < output_len && i + 3 < input_len && input[i + 3] != '=') {
            output[output_idx++] = triple & 0xFF;
        }
    }

    return output_idx;
}
