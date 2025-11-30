#include "log.h"
#include "console.h"
#include <stdint.h>

#define LOG_MAX_LINES 128
#define LOG_LINE_LEN  80

static char log_buf[LOG_MAX_LINES][LOG_LINE_LEN];
static int  log_head = 0;
static int  log_count = 0;

static int l_strlen(const char* s) {
    int n = 0;
    while (s && s[n]) n++;
    return n;
}

static void l_strncpy(char* dst, const char* src, int max_len) {
    int i = 0;
    if (!dst || !src || max_len <= 0) return;
    while (src[i] && i < max_len - 1) {
        dst[i] = src[i];
        i++;
    }
    dst[i] = 0;
}

void log_init(void)
{
    log_head = 0;
    log_count = 0;
}

void log_event(const char* msg)
{
    if (!msg) return;
    l_strncpy(log_buf[log_head], msg, LOG_LINE_LEN);
    log_head = (log_head + 1) % LOG_MAX_LINES;
    if (log_count < LOG_MAX_LINES) log_count++;
}

void log_dump(void)
{
    console_write("=== Hypnos audit log ===\n");
    int start = (log_head - log_count + LOG_MAX_LINES) % LOG_MAX_LINES;
    for (int i = 0; i < log_count; i++) {
        int idx = (start + i) % LOG_MAX_LINES;
        console_write(log_buf[idx]);
        console_write("\n");
    }
    console_write("=== end ===\n");
}
