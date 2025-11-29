#include "security.h"
#include "console.h"
#include "log.h"
#include <stdint.h>

#define MAX_USERS 8

static user_t users[MAX_USERS];
static int    user_count = 0;
static int    current_user_idx = -1;

/* local string helpers to avoid conflicts */
static int s_strlen(const char* s) {
    int n = 0;
    while (s && s[n]) n++;
    return n;
}

static int s_strcmp(const char* a, const char* b) {
    int i = 0;
    if (!a || !b) return (a == b) ? 0 : 1;
    while (a[i] && b[i] && a[i] == b[i]) i++;
    return (unsigned char)a[i] - (unsigned char)b[i];
}

static void s_strncpy(char* dst, const char* src, int max_len) {
    int i = 0;
    if (!dst || !src || max_len <= 0) return;
    while (src[i] && i < max_len - 1) {
        dst[i] = src[i];
        i++;
    }
    dst[i] = 0;
}

void sec_init(void)
{
    user_count = 0;
    current_user_idx = -1;

    /* default users */
    int root_id = sec_add_user("root",
        PERM_READ | PERM_WRITE | PERM_EXEC | PERM_SNAP | PERM_ADMIN);
    int guest_id = sec_add_user("guest", PERM_READ);

    (void)guest_id; /* unused warning*/

    if (root_id >= 0) {
        current_user_idx = 0; // root
        log_event("security: created users root, guest; logged in as root");
    } else {
        log_event("security: failed to create initial users");
    }
}

user_t* sec_get_current_user(void)
{
    if (current_user_idx < 0 || current_user_idx >= user_count)
        return 0;
    return &users[current_user_idx];
}

const char* sec_get_current_username(void)
{
    user_t* u = sec_get_current_user();
    if (!u) return "<none>";
    return u->name;
}

int sec_add_user(const char* name, uint32_t perms)
{
    if (!name || !name[0]) return -1;
    if (user_count >= MAX_USERS) return -1;

    /* no duplicates */
    for (int i = 0; i < user_count; i++) {
        if (s_strcmp(users[i].name, name) == 0)
            return -1;
    }

    user_t* u = &users[user_count];
    u->id = user_count;
    s_strncpy(u->name, name, (int)sizeof(u->name));
    u->perms = perms;
    user_count++;

    log_event("security: added user");
    return u->id;
}

int sec_login(const char* name)
{
    if (!name || !name[0]) return -1;

    for (int i = 0; i < user_count; i++) {
        if (s_strcmp(users[i].name, name) == 0) {
            current_user_idx = i;
            log_event("security: user login");
            console_write("Logged in as ");
            console_write(users[i].name);
            console_write("\n");
            return 0;
        }
    }

    console_write("No such user.\n");
    log_event("security: failed login attempt");
    return -1;
}

int sec_list_users(user_t* out, int max)
{
    if (!out || max <= 0) return 0;
    int n = (user_count < max) ? user_count : max;
    for (int i = 0; i < n; i++) {
        out[i] = users[i];
    }
    return n;
}

int sec_check_perm(uint32_t required)
{
    user_t* u = sec_get_current_user();
    if (!u) return 0;
    return ((u->perms & required) == required) ? 1 : 0;
}

int sec_require_perm(uint32_t required, const char* action)
{
    if (sec_check_perm(required))
        return 0;

    console_write("Permission denied: ");
    if (action) console_write(action);
    console_write("\n");
    log_event("security: permission denied");
    return -1;
}
