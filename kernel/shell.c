#include "fs.h"
#include <stddef.h>
#include <stdint.h>
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

static void ls_printer(const char* name, int is_dir)
{
    console_write("  ");
    console_write(name);
    if (is_dir) console_write("/");
    console_write("\n");
}
static void snap_list_printer(const char* name)
{
    console_write("  ");
    console_write(name);
    console_write("\n");
}

static void cmd_snap_list(void)
{
    console_write("Snapshots:\n");
    fs_snap_list(snap_list_printer);
}

static void cmd_snap_create(const char* name)
{
    if (!name || !name[0]) {
        console_write("Usage: snap-create <name>\n");
        return;
    }
    if (fs_snap_create(name) == 0)
        console_write("Snapshot created.\n");
    else
        console_write("snap-create: error (maybe name exists?).\n");
}

static void cmd_snap_restore(const char* name)
{
    if (!name || !name[0]) {
        console_write("Usage: snap-restore <name>\n");
        return;
    }
    if (fs_snap_restore(name) == 0) {
        console_write("Snapshot restored.\n");
        console_write("CWD is now ");
        console_write(fs_getcwd());
        console_write("\n");
    } else {
        console_write("snap-restore: no such snapshot.\n");
    }
}

static void cmd_ls(void)
{
    console_write("Listing ");
    console_write(fs_getcwd());
    console_write(":\n");
    fs_list(ls_printer);
}

static void cmd_pwd(void)
{
    console_write(fs_getcwd());
    console_write("\n");
}

static void cmd_cd(const char* path)
{
    if (!path || !path[0]) {
        console_write("Usage: cd <path>\n");
        return;
    }
    if (fs_chdir(path) == 0)
        cmd_pwd();
    else
        console_write("cd: no such directory.\n");
}

static void cmd_mkdir(const char* name)
{
    if (!name || !name[0]) {
        console_write("Usage: mkdir <name>\n");
        return;
    }
    if (fs_mkdir(name) == 0)
        console_write("Directory created.\n");
    else
        console_write("mkdir: error.\n");
}


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

// static void ls_printer(const char* name, int is_dir)
// {
//     (void)is_dir;
//     console_write("  ");
//     console_write(name);
//     console_write("\n");
// }

// static void cmd_ls(void)
// {
//     console_write("Files in / :\n");
//     fs_list(ls_printer);
// }


static void cmd_touch(const char* arg)
{
    if (!arg || !arg[0]) {
        console_write("Usage: touch <name>\n");
        return;
    }
    if (fs_touch(arg) == 0)
        console_write("Created/updated file.\n");
    else
        console_write("touch: error.\n");
}

static void cmd_write(const char* name, const char* text)
{
    if (fs_write(name, text) == 0)
        console_write("File written.\n");
    else
        console_write("write: error.\n");
}

static void cmd_cat(const char* name)
{
    if (!name || !name[0]) {
        console_write("Usage: cat <name>\n");
        return;
    }
    const char* data = fs_read(name);
    if (!data) {
        console_write("cat: no such file or empty.\n");
        return;
    }
    console_write(data);
    console_write("\n");
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
    int zero = 0;
    int x = 1 / zero;
    (void)x;
}

/* split "cmd arg" into arg (returns pointer inside original string or NULL) */
static const char* skip_word(const char* s) {
    while (*s && *s != ' ') s++;
    while (*s == ' ') s++;
    return s;
}

/* copies next word into dst, returns 0 on success, -1 if no word */
static int next_word(const char* s, char* dst, size_t dst_size) {
    while (*s == ' ') s++;
    if (!*s) return -1;
    size_t i = 0;
    while (*s && *s != ' ' && i + 1 < dst_size) {
        dst[i++] = *s++;
    }
    dst[i] = 0;
    return 0;
}

// static void shell_execute(const char* cmd)
// {
//     if (cmd[0] == 0)
//         return;
//     if (!kstrcmp(cmd, "help")) cmd_help();
//     else if (!kstrcmp(cmd, "clear")) cmd_clear();
//     else if (!kstrcmp(cmd, "uptime")) cmd_uptime();
//     else if (!kstrncmp(cmd, "echo ", 5)) cmd_echo(cmd + 5);
//     else if (!kstrcmp(cmd, "panic")) cmd_panic();
//     else console_write("Unknown command. Type 'help'.\n");
// }
static void shell_execute(const char* cmd)
{
    if (cmd[0] == 0)
        return;

    if (!kstrcmp(cmd, "help")) {
        cmd_help();
    }
    else if (!kstrcmp(cmd, "clear")) {
        cmd_clear();
    }
    else if (!kstrcmp(cmd, "uptime")) {
        cmd_uptime();
    }
    else if (!kstrncmp(cmd, "echo ", 5)) {
        cmd_echo(cmd + 5);
    }
    else if (!kstrcmp(cmd, "panic")) {
        cmd_panic();
    }
    else if (!kstrcmp(cmd, "ls")) {
        cmd_ls();
    }
    else if (!kstrcmp(cmd, "pwd")) {
        cmd_pwd();
    }
    else if (!kstrncmp(cmd, "cd ", 3)) {
        cmd_cd(cmd + 3);
    }
    else if (!kstrncmp(cmd, "mkdir ", 6)) {
        cmd_mkdir(cmd + 6);
    }
    else if (!kstrncmp(cmd, "touch ", 6)) {
        cmd_touch(cmd + 6);
    }
    else if (!kstrncmp(cmd, "cat ", 4)) {
    const char* name = cmd + 4;
    while (*name == ' ') name++;
    cmd_cat(name);
}

        else if (!kstrcmp(cmd, "snap-list")) {
        cmd_snap_list();
    }
    else if (!kstrncmp(cmd, "snap-create ", 12)) {
        cmd_snap_create(cmd + 12);
    }
    else if (!kstrncmp(cmd, "snap-restore ", 13)) {
        cmd_snap_restore(cmd + 13);
    }
else if (!kstrncmp(cmd, "write ", 6)) {
    const char* p = cmd + 6;

    // 1) skip spaces before filename
    while (*p == ' ') p++;

    // 2) extract filename
    char name[32];
    int i = 0;
    while (*p && *p != ' ' && i < (int)sizeof(name) - 1) {
        name[i++] = *p++;
    }
    name[i] = '\0';

    // 3) skip spaces before text
    while (*p == ' ') p++;

    // now p points to the text (can be multi-word)
    if (!name[0] || !*p) {
        console_write("Usage: write <name> <text>\n");
        return;
    }

    // 4) just pass the rest of the line as-is
    if (fs_write(name, p) == 0)
        console_write("File written.\n");
    else
        console_write("write: error.\n");
}


    else {
        console_write("Unknown command. Type 'help'.\n");
    }
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
