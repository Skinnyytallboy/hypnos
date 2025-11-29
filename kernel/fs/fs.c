#include "fs/fs.h"
#include "fs/crypto.h"
#include "arch/i386/mm/kmalloc.h"
#include "console.h"
#include <stddef.h>

#define MAX_NAME_LEN  32
#define MAX_PATH_LEN  128

struct fs_node {
    char name[MAX_NAME_LEN];
    int is_dir;
    char* data;
    uint32_t size;
    struct fs_node* parent;
    struct fs_node* child;   /* first child */
    struct fs_node* sibling; /* next sibling in same dir */
};

typedef struct fs_node fs_node_t;

static fs_node_t* fs_root = NULL;
static fs_node_t* fs_cwd  = NULL;

#define MAX_SNAP_NAME 32

typedef struct snap {
    char name[MAX_SNAP_NAME];
    fs_node_t* root_copy;
    struct snap* next;
} snap_t;

static snap_t* snap_head = NULL;

/* --- tiny string helpers (no libc) --- */

static int kstrcmp(const char* a, const char* b) {
    while (*a && (*a == *b)) { a++; b++; }
    return (unsigned char)*a - (unsigned char)*b;
}

static int kstrncmp(const char* a, const char* b, size_t n) {
    while (n && *a && (*a == *b)) { a++; b++; n--; }
    if (n == 0) return 0;
    return (unsigned char)*a - (unsigned char)*b;
}

static size_t kstrlen(const char* s) {
    size_t n = 0;
    while (s[n]) n++;
    return n;
}

static void kstrncpy(char* dst, const char* src, size_t n) {
    size_t i = 0;
    for (; i < n && src[i]; i++) dst[i] = src[i];
    if (i < n) dst[i] = 0;
    else dst[n-1] = 0;
}

/* --- basic node utilities --- */

static fs_node_t* fs_new_node(const char* name, int is_dir)
{
    fs_node_t* n = (fs_node_t*)kmalloc(sizeof(fs_node_t));
    if (!n) return NULL;

    kstrncpy(n->name, name, MAX_NAME_LEN);
    n->is_dir = is_dir;
    n->data   = NULL;
    n->size   = 0;
    n->parent = NULL;
    n->child  = NULL;
    n->sibling= NULL;
    return n;
}

static fs_node_t* fs_clone_tree(fs_node_t* n, fs_node_t* parent)
{
    if (!n) return NULL;

    fs_node_t* copy = fs_new_node(n->name, n->is_dir);
    if (!copy) return NULL;

    copy->parent = parent;
    copy->size   = n->size;

    if (!n->is_dir && n->data && n->size > 0) {
        /* copy file data */
        char* buf = (char*)kmalloc(n->size + 1);
        if (buf) {
            for (uint32_t i = 0; i < n->size; i++)
                buf[i] = n->data[i];
            buf[n->size] = 0;
            copy->data = buf;
        }
    }

    if (n->child) {
        fs_node_t* child_copy = fs_clone_tree(n->child, copy);
        copy->child = child_copy;
    }

    if (n->sibling) {
        fs_node_t* sib_copy = fs_clone_tree(n->sibling, parent);
        copy->sibling = sib_copy;
    }

    return copy;
}


static fs_node_t* fs_find_in_dir(fs_node_t* dir, const char* name)
{
    fs_node_t* cur = dir->child;
    while (cur) {
        if (!kstrcmp(cur->name, name))
            return cur;
        cur = cur->sibling;
    }
    return NULL;
}

/* split path into components, return next component and advance *p */
static int fs_next_component(const char** p, char* out, size_t out_sz)
{
    const char* s = *p;

    while (*s == '/') s++;
    if (!*s) {
        *p = s;
        return 0;         
    }

    size_t i = 0;
    while (*s && *s != '/' && i + 1 < out_sz) {
        out[i++] = *s++;
    }
    out[i] = 0;
    while (*s == '/') s++;
    *p = s;
    return 1;
}

static fs_node_t* fs_resolve(const char* path, int want_parent, char* last_name)
{
    if (!path || !*path) return NULL;
    fs_node_t* cur;
    if (path[0] == '/') cur = fs_root;
    else cur = fs_cwd;
    const char* p = path;
    char comp[MAX_NAME_LEN];
    fs_node_t* parent = NULL;
    while (fs_next_component(&p, comp, sizeof(comp))) {
        parent = cur;
        fs_node_t* next = fs_find_in_dir(cur, comp);
        if (!next) {
            if (want_parent && *p == 0) {
                if (last_name)
                    kstrncpy(last_name, comp, MAX_NAME_LEN);
                return cur;
            }
            return NULL;
        }
        cur = next;
    }
    if (want_parent) {
        if (last_name) last_name[0] = 0;
        return parent ? parent : cur;
    }
    return cur;
}

void fs_init(void)
{
    fs_root = fs_new_node("/", 1);
    fs_root->parent = fs_root; 
    fs_cwd  = fs_root;
    console_write("RAM filesystem with directories initialized.\n");
}

int fs_mkdir(const char* path)
{
    if (!path || !*path) return -1;
    if (kstrlen(path) >= MAX_PATH_LEN) return -1;

    char last[MAX_NAME_LEN];
    fs_node_t* parent = fs_resolve(path, 1, last);
    if (!parent || !parent->is_dir) return -1;
    if (!last[0]) return -1;

    if (fs_find_in_dir(parent, last))
        return -1; 
    fs_node_t* n = fs_new_node(last, 1);
    if (!n) return -1;
    n->parent = parent;
    n->sibling = parent->child;
    parent->child = n;
    return 0;
}

int fs_touch(const char* path)
{
    if (!path || !*path) return -1;
    if (kstrlen(path) >= MAX_PATH_LEN) return -1;

    char last[MAX_NAME_LEN];
    fs_node_t* parent = fs_resolve(path, 1, last);
    if (!parent || !parent->is_dir) return -1;
    if (!last[0]) return -1;

    fs_node_t* node = fs_find_in_dir(parent, last);
    if (node) {
        if (node->is_dir) return -1;
        return 0;  /* file already exists */
    }

    node = fs_new_node(last, 0);
    if (!node) return -1;
    node->parent = parent;
    node->sibling = parent->child;
    parent->child = node;
    return 0;
}

int fs_write(const char* path, const char* data)
{
    if (!path || !data) return -1;

    char last[MAX_NAME_LEN];
    fs_node_t* parent = fs_resolve(path, 1, last);
    if (!parent || !parent->is_dir) return -1;
    if (!last[0]) return -1;

    fs_node_t* node = fs_find_in_dir(parent, last);
    if (!node) {
        if (fs_touch(path) != 0) return -1;
        node = fs_find_in_dir(parent, last);
        if (!node) return -1;
    }

    if (node->is_dir) return -1;

    size_t len = kstrlen(data);
    char* buf = (char*)kmalloc(len + 1);
    if (!buf) return -1;

    /* encrypt plaintext into node->data */
    crypto_encrypt((const uint8_t*)data, (uint8_t*)buf, len);
    buf[len] = 0;              /* keep it C-string-like, even if encrypted */

    node->data = buf;
    node->size = (uint32_t)len;

    return 0;
}


const char* fs_read(const char* path)
{
    fs_node_t* node = fs_resolve(path, 0, NULL);
    if (!node || node->is_dir || !node->data || node->size == 0)
        return NULL;

    /* allocate buffer for decrypted data */
    char* buf = (char*)kmalloc(node->size + 1);
    if (!buf) return NULL;

    crypto_decrypt((const uint8_t*)node->data, (uint8_t*)buf, node->size);
    buf[node->size] = 0;

    return buf;   /* caller (cat) just prints it; we leak a little, which is fine for now */
}


int fs_chdir(const char* path)
{
    if (!path || !*path) return -1;

    fs_node_t* n = fs_resolve(path, 0, NULL);
    if (!n || !n->is_dir) return -1;

    fs_cwd = n;
    return 0;
}

const char* fs_getcwd(void)
{
    static char buf[MAX_PATH_LEN];
    char tmp[MAX_PATH_LEN];
    size_t pos = 0;

    fs_node_t* n = fs_cwd;
    if (n == fs_root) {
        buf[0] = '/';
        buf[1] = 0;
        return buf;
    }

    tmp[0] = 0;
    while (n && n != fs_root) {
        size_t name_len = kstrlen(n->name);
        if (pos + name_len + 1 >= sizeof(tmp)) break;
        for (size_t i = pos + name_len + 1; i-- > name_len + 1; )
            tmp[i] = tmp[i - name_len - 1];
        tmp[0] = '/';
        for (size_t i = 0; i < name_len; i++)
            tmp[1 + i] = n->name[i];
        pos += name_len + 1;
        tmp[pos] = 0;
        n = n->parent;
    }

    if (pos == 0) { buf[0] = '/'; buf[1] = 0; }
    else {
        for (size_t i = 0; i <= pos; i++)
            buf[i] = tmp[i];
    }
    return buf;
}

void fs_list(fs_list_cb cb)
{
    fs_node_t* cur = fs_cwd->child;
    while (cur) {
        cb(cur->name, cur->is_dir);
        cur = cur->sibling;
    }
}

int fs_snap_create(const char* name)
{
    if (!name || !name[0]) return -1;
    if (kstrlen(name) >= MAX_SNAP_NAME) return -1;
    snap_t* cur = snap_head;
    while (cur) {
        if (!kstrcmp(cur->name, name))
            return -1;
        cur = cur->next;
    }

    fs_node_t* root_copy = fs_clone_tree(fs_root, NULL);
    if (!root_copy) return -1;
    root_copy->parent = root_copy;
    snap_t* s = (snap_t*)kmalloc(sizeof(snap_t));
    if (!s) return -1;
    kstrncpy(s->name, name, MAX_SNAP_NAME);
    s->root_copy = root_copy;
    s->next = snap_head;
    snap_head = s;

    return 0;
}

int fs_snap_restore(const char* name)
{
    if (!name || !name[0]) return -1;

    snap_t* cur = snap_head;
    while (cur) {
        if (!kstrcmp(cur->name, name)) {
            /* for now: just switch root & cwd to this snapshot copy */
            fs_root = cur->root_copy;
            fs_cwd  = fs_root;
            return 0;
        }
        cur = cur->next;
    }
    return -1;
}
void fs_snap_list(fs_snap_list_cb cb)
{
    snap_t* cur = snap_head;
    while (cur) {
        cb(cur->name);
        cur = cur->next;
    }
}
