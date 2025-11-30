#include "fs/fs.h"
#include "security.h"
#include "log.h"
#include "editor.h"
#include <stddef.h>
#include <stdint.h>
#include "console.h"
#include "sched/task.h"
#include "arch/i386/drivers/keyboard.h"
#include "arch/i386/drivers/timer.h"

extern void switch_to_user_mode(void);
static uint32_t last_bar_second = 0;

static void ui_itoa(uint32_t v, char* out) {
    char tmp[16];
    int i = 0;
    if (v == 0) {
        out[0] = '0';
        out[1] = 0;
        return;
    }
    while (v > 0 && i < 15) {
        tmp[i++] = '0' + (v % 10);
        v /= 10;
    }
    int j = 0;
    while (i > 0) {
        out[j++] = tmp[--i];
    }
    out[j] = 0;
}

static int kstrcmp(const char *a, const char *b)
{
    while (*a && (*a == *b))
    {
        a++;
        b++;
    }
    return (unsigned char)*a - (unsigned char)*b;
}

static int kstrncmp(const char *a, const char *b, size_t n)
{
    while (n && *a && (*a == *b))
    {
        a++;
        b++;
        n--;
    }
    if (n == 0)
        return 0;
    return (unsigned char)*a - (unsigned char)*b;
}

static void shell_draw_status_bar(void)
{
    uint32_t sec        = timer_get_seconds();
    const char* user    = sec_get_current_username();
    const char* cwd     = fs_getcwd();

    char buf[80];
    int pos = 0;

    const char* p = " user: ";
    while (*p && pos < 79) buf[pos++] = *p++;
    p = user;
    while (*p && pos < 79) buf[pos++] = *p++;

    const char* mid = " | uptime: ";
    p = mid;
    while (*p && pos < 79) buf[pos++] = *p++;
    char num[16];
    ui_itoa(sec, num);
    p = num;
    while (*p && pos < 79) buf[pos++] = *p++;
    if (pos < 79) buf[pos++] = 's';

    const char* fsinfo = " | fs: RAM-FS";
    p = fsinfo;
    while (*p && pos < 79) buf[pos++] = *p++;

    const char* cwd_label = " | cwd: ";
    p = cwd_label;
    while (*p && pos < 79) buf[pos++] = *p++;
    p = cwd;
    while (*p && pos < 79) buf[pos++] = *p++;

    while (pos < 79) buf[pos++] = ' ';
    buf[pos] = 0;

    size_t old_row, old_col;
    console_get_cursor(&old_row, &old_col);

    int bar_row = VGA_HEIGHT - 1;

    console_set_color(COLOR_BLACK, COLOR_LIGHT_GREY); 
    for (int x = 0; x < 79; x++) {
        console_put_at(buf[x], x, bar_row);
    }
    console_set_theme_default();
    console_set_cursor(old_row, old_col);
}


static void shell_print_prompt(void)
{
    shell_draw_status_bar();
    const char* cwd = fs_getcwd(); 

    console_set_color(COLOR_LIGHT_GREEN, COLOR_BLACK);
    console_write(sec_get_current_username());
    console_write("@hypnos");

    console_set_color(COLOR_MAGENTA, COLOR_BLACK);
    console_write(cwd);
    console_set_color(COLOR_LIGHT_GREEN, COLOR_BLACK);
    console_write(">");
    console_set_theme_default();
}

#define SHELL_INPUT_MAX 128

static char   input_buffer[SHELL_INPUT_MAX];
static size_t input_len = 0;

extern volatile uint32_t timer_ticks;

static void shell_execute(const char *cmd);

static void ls_printer(const char *name, int is_dir)
{
    console_write("  ");
    console_write(name);
    if (is_dir)
        console_write("/");
    console_write("\n");
}

static void snap_list_printer(const char *name)
{
    console_write("  ");
    console_write(name);
    console_write("\n");
}

static void cmd_whoami(void)
{
    console_write("Current user: ");
    console_write(sec_get_current_username());
    console_write("\n");
}

static void cmd_users(void)
{
    user_t list[8];
    int n = sec_list_users(list, 8);

    console_write("Users:\n");
    for (int i = 0; i < n; i++)
    {
        console_write("  ");
        console_write(list[i].name);
        if (i == sec_get_current_user()->id)
            console_write(" (current)");
        console_write("\n");
    }
}

static void cmd_login(const char *name)
{
    if (!name || !name[0])
    {
        console_write("Usage: login <username>\n");
        return;
    }
    sec_login(name);
}

static void cmd_log_show(void) { log_dump(); }

static void cmd_snap_list(void)
{
    console_write("Snapshots:\n");
    fs_snap_list(snap_list_printer);
}

static void cmd_snap_create(const char *name)
{
    if (!name || !name[0])
    {
        console_write("Usage: snap-create <name>\n");
        return;
    }
    if (fs_snap_create(name) == 0)
        console_write("Snapshot created.\n");
    else
        console_write("snap-create: error (maybe name exists?).\n");
}

static void cmd_snap_restore(const char *name)
{
    if (!name || !name[0])
    {
        console_write("Usage: snap-restore <name>\n");
        return;
    }
    if (fs_snap_restore(name) == 0)
    {
        console_write("Snapshot restored.\n");
        console_write("CWD is now ");
        console_write(fs_getcwd());
        console_write("\n");
    }
    else
    {
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

static void cmd_cd(const char *path)
{
    if (!path || !path[0])
    {
        console_write("Usage: cd <path>\n");
        return;
    }

    /* "cd.." should trigger this, "cd .." should NOT */
    if (!kstrcmp(path, ".."))
    {
        const char *cwd = fs_getcwd();
        char buf[128];

        int i = 0;
        while (cwd[i] && i < (int)sizeof(buf) - 1) {
            buf[i] = cwd[i];
            i++;
        }
        buf[i] = 0;

        /* already at root? stay there */
        if (buf[0] == '/' && buf[1] == 0) {
            /* do nothing */
        } else {
            /* remove trailing slash */
            if (i > 1 && buf[i - 1] == '/') {
                buf[i - 1] = 0;
                i--;
            }

            /* remove the last path segment */
            while (i > 0 && buf[i - 1] != '/') {
                buf[i - 1] = 0;
                i--;
            }

            if (i == 0) {
                buf[0] = '/';
                buf[1] = 0;
            }
        }

        if (fs_chdir(buf) == 0)
            cmd_pwd();
        else
            console_write("cd: error.\n");

        return;
    }

    /* normal "cd folder" */
    if (fs_chdir(path) == 0)
        cmd_pwd();
    else
        console_write("cd: no such directory.\n");
}



static void cmd_mkdir(const char *name)
{
    if (!name || !name[0])
    {
        console_write("Usage: mkdir <name>\n");
        return;
    }
    if (fs_mkdir(name) == 0)
        console_write("Directory created.\n");
    else
        console_write("mkdir: error.\n");
}

static void cmd_touch(const char *arg)
{
    if (!arg || !arg[0])
    {
        console_write("Usage: touch <name>\n");
        return;
    }
    if (fs_touch(arg) == 0)
        console_write("Created/updated file.\n");
    else
        console_write("touch: error.\n");
}

static void cmd_write(const char *name, const char *text)
{
    if (fs_write(name, text) == 0)
        console_write("File written.\n");
    else
        console_write("write: error.\n");
}

static void cmd_cat(const char *name)
{
    if (!name || !name[0])
    {
        console_write("Usage: cat <name>\n");
        return;
    }
    const char *data = fs_read(name);
    if (!data)
    {
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
    uint32_t ticks = timer_get_ticks();

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


static void cmd_echo(const char *msg)
{
    console_write(msg);
    console_write("\n");
}

static void cmd_panic(void)
{
    console_write("Triggering division by zero...\n");
    int zero = 0;
    int x = 1 / zero;
    (void)x;
}

/* split "cmd arg" into arg (returns pointer inside original string or NULL) */
static const char *skip_word(const char *s)
{
    while (*s && *s != ' ')
        s++;
    while (*s == ' ')
        s++;
    return s;
}

static void shell_execute(const char *cmd)
{
    if (cmd[0] == 0)
        return;

    if (!kstrcmp(cmd, "help"))
    {
        console_write("Available commands:\n");
        console_write("  help          - show this message\n");
        console_write("  clear         - clear the screen\n");
        console_write("  uptime        - show ticks since boot\n");
        console_write("  echo X        - print X\n");
        console_write("  panic         - cause an exception\n");
        console_write("  ls            - list directory\n");
        console_write("  pwd           - print working directory\n");
        console_write("  cd <path>     - change directory\n");
        console_write("  mkdir <name>  - make directory\n");
        console_write("  touch <name>  - create/update file\n");
        console_write("  write f txt   - write text to file\n");
        console_write("  cat <name>    - show file contents\n");
        console_write("  edit <file>   - simple text editor\n");
        console_write("  snap-*        - snapshot commands\n");
        console_write("  whoami/users  - security info\n");
        console_write("  login <user>  - switch user\n");
        console_write("  log           - show audit log\n");
    }
    else if (!kstrcmp(cmd, "clear"))
        cmd_clear();
    else if (!kstrcmp(cmd, "uptime"))
        cmd_uptime();
    else if (!kstrncmp(cmd, "echo ", 5))
        cmd_echo(cmd + 5);
    else if (!kstrcmp(cmd, "panic"))
        cmd_panic();
    else if (!kstrcmp(cmd, "ls"))
        cmd_ls();
    else if (!kstrcmp(cmd, "pwd"))
        cmd_pwd();
    else if (!kstrcmp(cmd, "cd.."))
        cmd_cd("..");
    else if (!kstrncmp(cmd, "cd ", 3))
        cmd_cd(cmd + 3);
    else if (!kstrncmp(cmd, "edit ", 5))
    {
        const char *name = cmd + 5;
        while (*name == ' ')
            name++;

        if (!name[0])
            console_write("Usage: edit <filename>\n");
        else
            editor_start(name);
    }
    else if (!kstrncmp(cmd, "mkdir ", 6))
    {
        const char *name = cmd + 6;
        while (*name == ' ')
            name++;

        if (sec_require_perm(PERM_WRITE, "create directory") != 0)
            return;

        if (fs_mkdir(name) == 0)
        {
            console_write("Directory created.\n");
            log_event("fs: mkdir");
        }
        else
        {
            console_write("mkdir: error.\n");
            log_event("fs: mkdir error");
        }
    }
    else if (!kstrncmp(cmd, "touch ", 6))
        cmd_touch(cmd + 6);
    else if (!kstrcmp(cmd, "runuser")) {
        console_write("Launching first user program in Ring 3...\n");
        switch_to_user_mode();
    }
    else if (!kstrncmp(cmd, "cat ", 4))
    {
        const char *name = cmd + 4;
        while (*name == ' ')
            name++;

        if (sec_require_perm(PERM_READ, "read file") != 0)
            return;

        const char *data = fs_read(name);
        if (!data)
        {
            console_write("cat: no such file or empty.\n");
            log_event("fs: read fail");
            return;
        }
        console_write(data);
        console_write("\n");
        log_event("fs: read");
    }
    else if (!kstrcmp(cmd, "snap-list"))
        cmd_snap_list();
    else if (!kstrncmp(cmd, "snap-create ", 12))
    {
        const char *name = cmd + 12;
        while (*name == ' ')
            name++;

        if (sec_require_perm(PERM_SNAP, "snapshot create") != 0)
            return;

        if (fs_snap_create(name) == 0)
        {
            console_write("Snapshot created.\n");
            log_event("fs: snapshot create");
        }
        else
        {
            console_write("snap-create: error.\n");
            log_event("fs: snapshot create error");
        }
    }
    else if (!kstrncmp(cmd, "snap-restore ", 13))
    {
        const char *name = cmd + 13;
        while (*name == ' ')
            name++;

        if (sec_require_perm(PERM_SNAP, "snapshot restore") != 0)
            return;

        if (fs_snap_restore(name) == 0)
        {
            console_write("Snapshot restored.\n");
            log_event("fs: snapshot restore");
        }
        else
        {
            console_write("snap-restore: error.\n");
            log_event("fs: snapshot restore error");
        }
    }
    else if (!kstrcmp(cmd, "whoami"))
        cmd_whoami();
    else if (!kstrcmp(cmd, "users"))
        cmd_users();
    else if (!kstrncmp(cmd, "login ", 6))
    {
        const char *name = cmd + 6;
        while (*name == ' ')
            name++;

        cmd_login(name);
    }
    else if (!kstrcmp(cmd, "log"))
        cmd_log_show();
    else if (!kstrncmp(cmd, "write ", 6))
    {
        const char *p = cmd + 6;

        while (*p == ' ')
            p++;

        char name[32];
        int i = 0;

        while (*p && *p != ' ' && i < (int)sizeof(name) - 1)
            name[i++] = *p++;
        name[i] = '\0';

        while (*p == ' ')
            p++;

        if (!name[0] || !*p)
        {
            console_write("Usage: write <name> <text>\n");
            return;
        }

        if (sec_require_perm(PERM_WRITE, "write file") != 0)
            return;

        if (fs_write(name, p) == 0)
        {
            console_write("File written.\n");
            log_event("fs: write");
        }
        else
        {
            console_write("write: error.\n");
            log_event("fs: write error");
        }
    }
    else
        console_write("Unknown command. Type 'help'.\n");
}


void shell_keypress(char c) {
    
    if (editor_is_active()) {
        int was_active = editor_is_active();
        editor_handle_key(c);
        if (was_active && !editor_is_active()) {
            shell_print_prompt();
        }
        return;
    }

    /* Normal shell command-line handling */
    if (c == '\b')
    {
        if (input_len > 0)
        {
            input_len--;
            console_write("\b");
        }
        return;
    }
   if (c == '\n')
    {
        console_write("\n");
        input_buffer[input_len] = 0;

        shell_execute(input_buffer);
        input_len = 0;

        /* If edit <file> started the editor, do NOT print prompt now */
        if (!editor_is_active()) {
            shell_print_prompt();
        }
        return;
    }
    if (input_len < SHELL_INPUT_MAX - 1)
    {
        input_buffer[input_len++] = c;
        char s[2] = {c, 0};
        console_write(s);
    }
}

void shell_tick(void) {
    static int counter = 0;

    counter++;
    if (counter >= 100) {
        counter = 0;
        shell_draw_status_bar();
    }
}

void shell_init(void) { input_len = 0; }

void shell_run(void)
{
    shell_print_prompt();

    for (;;) {
        /* sleep until any interrupt (timer / keyboard) */
        __asm__ volatile("hlt");
        /* give other tasks a chance to run */
        task_yield();
    }
}