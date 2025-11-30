#include "editor.h"
#include "console.h"
#include "fs/fs.h"

/* Very small, dumb text editor.
 *
 * - edit <file>      : enter editor
 * - type text        : appended to buffer
 * - Enter            : newline
 * - Backspace        : delete last char
 * - ESC              : save file & exit back to shell
 */

#define EDITOR_BUF_SIZE 4096
#define EDITOR_NAME_MAX 32

static char   editor_buf[EDITOR_BUF_SIZE];
static size_t editor_len    = 0;
static char   editor_name[EDITOR_NAME_MAX];
static int    editor_active = 0;

/* Draw a simple status line on the last row. */
static void editor_draw_status(void)
{
    size_t old_row, old_col;
    console_get_cursor(&old_row, &old_col);

    int row = VGA_HEIGHT - 1;
    console_set_color(COLOR_BLACK, COLOR_LIGHT_GREY);

    char line[80];
    int pos = 0;

    const char* msg1 = "[ESC] save & exit | editing: ";
    const char* p = msg1;
    while (*p && pos < 79) line[pos++] = *p++;

    p = editor_name;
    while (*p && pos < 79) line[pos++] = *p++;

    while (pos < 79) line[pos++] = ' ';
    line[pos] = 0;

    for (int x = 0; x < 79; x++) {
        console_put_at(line[x], x, row);
    }

    console_set_theme_default();
    console_set_cursor(old_row, old_col);
}

/* Save current buffer to file in the current directory. */
static void editor_save_file(void)
{
    editor_buf[editor_len] = 0; /* ensure null-terminated */

    /* If buffer is empty, just ensure file exists and call it saved */
    if (editor_len == 0) {
        if (fs_touch(editor_name) == 0) {
            console_write("\n[editor] Saved (empty file).\n");
        } else {
            console_write("\n[editor] Error saving empty file.\n");
        }
        return;
    }

    /* Non-empty: write into current directory using simple helper */
    if (fs_write_cwd(editor_name, editor_buf) == 0) {
        console_write("\n[editor] Saved.\n");
    } else {
        console_write("\n[editor] Error saving file.\n");
    }
}

void editor_start(const char* filename)
{
    if (!filename || !filename[0])
        return;

    /* Copy filename into our own static buffer */
    int i = 0;
    while (filename[i] && i < EDITOR_NAME_MAX - 1) {
        editor_name[i] = filename[i];
        i++;
    }
    editor_name[i] = 0;

    /* Always start with an empty buffer (we're not loading file contents) */
    editor_len    = 0;
    editor_buf[0] = 0;

    editor_active = 1;

    /* Clear screen and print header */
    console_clear();
    console_set_theme_banner();
    console_write("Hypnos editor - editing: ");
    console_write(editor_name);
    console_write("\n\n");
    console_set_theme_default();

    editor_draw_status();
}

void editor_handle_key(char c)
{
    if (!editor_active)
        return;

    /* ESC (27) => save + exit */
    if ((unsigned char)c == 27) {
        editor_save_file();
        editor_active = 0;
        return;
    }

    /* Backspace */
    if (c == '\b') {
        if (editor_len > 0) {
            editor_len--;
            console_write("\b");
        }
        return;
    }

    /* Newline */
    if (c == '\n') {
        if (editor_len < EDITOR_BUF_SIZE - 1) {
            editor_buf[editor_len++] = '\n';
            console_write("\n");
        }
        return;
    }

    /* Printable ASCII */
    if (c >= ' ' && c <= '~') {
        if (editor_len < EDITOR_BUF_SIZE - 1) {
            editor_buf[editor_len++] = c;
            char s[2] = { c, 0 };
            console_write(s);
        }
        return;
    }

    /* ignore anything else */
}

int editor_is_active(void)
{
    return editor_active;
}
