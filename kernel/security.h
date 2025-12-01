#pragma once
#include <stdint.h>

typedef struct user {
    int       id;
    char      name[16];
    uint32_t  perms;
} user_t;

enum {
    PERM_READ   = 1 << 0,
    PERM_WRITE  = 1 << 1,
    PERM_EXEC   = 1 << 2,
    PERM_SNAP   = 1 << 3,
    PERM_ADMIN  = 1 << 4,
};

void      sec_init(void);
const char* sec_get_current_username(void);
user_t*   sec_get_current_user(void);

int sec_add_user(const char* name, uint32_t perms);
int sec_login(const char* name);
int sec_list_users(user_t* out, int max);

int sec_check_perm(uint32_t required);
int sec_require_perm(uint32_t required, const char* action);