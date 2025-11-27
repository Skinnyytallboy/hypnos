#include <stdint.h>
#include <stddef.h>
#include "console.h"
#include "keyboard.h"
#include "timer.h"

static int kstrcmp(const char* a, const char* b) {
    while (*a && (*a == *b)) {
        a++;
        b++;
    }
    return (unsigned char)*a - (unsigned char)*b;
}

static int kstrncmp(const char* a, const char* b, size_t n) {
    while (n && *a && (*a == *b)) {
        a++;
        b++;
        n--;
    }
    if (n == 0) return 0;
    return (unsigned char)*a - (unsigned char)*b;
}

#define SHELL_INPUT_MAX 128

static char input_buffer[SHELL_INPUT_MAX];
static size_t input_len = 0;

extern volatile uint32_t timer_ticks;

static void shell_execute(const char* cmd);

void shell_keypress(char c)
{
    if (c == '\b') {
        if (input_len > 0) {
            input_len--;
            console_write("\b");
        }
        return;
    }

    if (c == '\n') {
        console_write("\n");
        input_buffer[input_len] = 0;
        shell_execute(input_buffer);
        input_len = 0;
        console_write("> ");
        return;
    }

    if (input_len < SHELL_INPUT_MAX - 1) {
        input_buffer[input_len++] = c;
        char s[2] = {c, 0};
        console_write(s);
    }
}

static void cmd_help(void) {
    console_write("Available commands:\n");
    console_write("  help     - show this message\n");
    console_write("  clear    - clear the screen\n");
    console_write("  uptime   - show ticks since boot\n");
    console_write("  echo X   - print X\n");
    console_write("  panic    - cause an exception\n");
}

static void cmd_clear(void) { console_clear(); }

static void cmd_uptime(void)
{
    char buf[64];
    uint32_t ticks = timer_ticks;

    int len = 0;
    uint32_t t = ticks;
    char tmp[32];
    if (t == 0) {
        tmp[len++] = '0';
    } else {
        while (t > 0) {
            tmp[len++] = '0' + (t % 10);
            t /= 10;
        }
    }
    for (int i = 0; i < len; i++)
        buf[i] = tmp[len - 1 - i];

    buf[len] = 0;

    console_write("Uptime ticks: ");
    console_write(buf);
    console_write("\n");
}

static void cmd_echo(const char* msg) {
    console_write(msg);
    console_write("\n");
}

static void cmd_panic(void) {
    console_write("Triggering division by zero...\n");
    int x = 1 / 0;
    (void)x;
}

static void shell_execute(const char* cmd)
{
    if (cmd[0] == 0)
        return;
    if (!kstrcmp(cmd, "help")) cmd_help();
    else if (!kstrcmp(cmd, "clear")) cmd_clear();
    else if (!kstrcmp(cmd, "uptime")) cmd_uptime();
    else if (!kstrncmp(cmd, "echo ", 5)) cmd_echo(cmd + 5);
    else if (!kstrcmp(cmd, "panic")) cmd_panic();
    else console_write("Unknown command. Type 'help'.\n");
}

void shell_init(void) {
    input_len = 0;
    console_write("> ");
}

void shell_run(void)
{
    for (;;) {
        __asm__ volatile ("hlt");
    }
}
