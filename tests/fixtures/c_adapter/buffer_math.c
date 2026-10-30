#include <stdint.h>
#include <stddef.h>

int64_t c_buffer_weighted_sum(const char *bytes, int64_t bytes_len, int64_t weight) {
    int64_t sum = 0;
    int64_t i;
    if (bytes_len < 0) {
        return -1;
    }
    if (bytes == NULL && bytes_len != 0) {
        return -2;
    }
    for (i = 0; i < bytes_len; ++i) {
        sum += (int64_t)(unsigned char)bytes[i];
    }
    return sum * weight + bytes_len;
}

int64_t c_text_checksum(const char *text) {
    int64_t sum = 0;
    int64_t i = 0;
    if (text == NULL) {
        return -1;
    }
    while (text[i] != '\0') {
        sum += (i + 1) * (int64_t)(unsigned char)text[i];
        ++i;
    }
    return sum;
}
