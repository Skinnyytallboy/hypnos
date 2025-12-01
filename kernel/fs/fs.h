#pragma once
#include <stdint.h>

typedef struct fs_node fs_node_t;
typedef void (*fs_tree_cb)(const char* name, int is_dir, int depth);

void fs_init(void);

/* file & dir operations */
int fs_touch(const char* path);
int fs_write(const char* path, const char* data);
const char* fs_read(const char* path);

/* directory operations */
int fs_mkdir(const char* path);
int fs_chdir(const char* path);
const char* fs_getcwd(void);

/* listing */
typedef void (*fs_list_cb)(const char* name, int is_dir);
void fs_list(fs_list_cb cb);

typedef void (*fs_snap_list_cb)(const char* name);

int  fs_snap_create(const char* name);      /* 0 = ok, -1 error */
int  fs_snap_restore(const char* name);     /* restores root + cwd */
void fs_snap_list(fs_snap_list_cb cb);

int fs_write_cwd(const char* name, const char* data);


void fs_tree_cwd(fs_tree_cb cb);


int fs_unlink(const char *path);

int fs_rmdir(const char *path);


int fs_rename(const char *oldpath, const char *newpath);


int fs_copy(const char *src, const char *dst);



 
int fs_find(const char *name);

