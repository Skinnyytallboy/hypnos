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
    struct fs_node* child;
    struct fs_node* sibling;
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
        return 0;
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

    crypto_encrypt((const uint8_t*)data, (uint8_t*)buf, len);
    buf[len] = 0;

    node->data = buf;
    node->size = (uint32_t)len;

    return 0;
}

int fs_write_cwd(const char* name, const char* data)
{
    if (!name || !name[0] || !data)
        return -1;
    if (!fs_cwd || !fs_cwd->is_dir)
        return -1;
    fs_node_t* node = fs_find_in_dir(fs_cwd, name);
    if (!node) {
        node = fs_new_node(name, 0);
        if (!node) return -1;
        node->parent  = fs_cwd;
        node->sibling = fs_cwd->child;
        fs_cwd->child = node;
    }

    if (node->is_dir)
        return -1;
    size_t len = kstrlen(data);

    char* buf = (char*)kmalloc(len + 1);
    if (!buf) return -1;

    crypto_encrypt((const uint8_t*)data, (uint8_t*)buf, len);
    buf[len] = 0;

    node->data = buf;
    node->size = (uint32_t)len;

    return 0;
}


const char* fs_read(const char* path)
{
    fs_node_t* node = fs_resolve(path, 0, NULL);
    if (!node || node->is_dir || !node->data || node->size == 0)
        return NULL;

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

static const char* kstrstr(const char* hay, const char* needle)
{
    if (!hay || !needle) return NULL;
    size_t n = kstrlen(needle);
    if (n == 0) return hay;
    for (size_t i = 0; hay[i]; i++) {
        if (kstrncmp(hay + i, needle, n) == 0)
            return hay + i;
    }
    return NULL;
}

/* detach node from its parent's child list (does not free memory) */
static void fs_detach_from_parent(fs_node_t* node)
{
    if (!node || !node->parent) return;
    fs_node_t* parent = node->parent;
    fs_node_t** curp = &parent->child;
    while (*curp) {
        if (*curp == node) {
            *curp = node->sibling;
            node->sibling = NULL;
            node->parent = NULL;
            return;
        }
        curp = &(*curp)->sibling;
    }
}

int fs_unlink(const char *path)
{
    if (!path || !*path) return -1;
    fs_node_t* node = fs_resolve(path, 0, NULL);
    if (!node) return -1;
    if (node->is_dir) return -1;
    if (!node->parent) return -1;

    fs_detach_from_parent(node);
    return 0;
}

int fs_rmdir(const char *path)
{
    if (!path || !*path) return -1;
    fs_node_t* node = fs_resolve(path, 0, NULL);
    if (!node) return -1;
    if (!node->is_dir) return -1;
    if (node->child) return -1;

    if (node == fs_root) return -1;

    fs_detach_from_parent(node);
    return 0;
}

int fs_rename(const char *oldpath, const char *newpath)
{
    if (!oldpath || !newpath) return -1;

    fs_node_t* src = fs_resolve(oldpath, 0, NULL);
    if (!src) return -1;

    char last[MAX_NAME_LEN];
    fs_node_t* dst_parent = fs_resolve(newpath, 1, last);
    if (!dst_parent || !dst_parent->is_dir) return -1;
    if (!last[0]) return -1;

    if (fs_find_in_dir(dst_parent, last))
        return -1;

    if (src == fs_root) return -1;

    fs_detach_from_parent(src);
    kstrncpy(src->name, last, MAX_NAME_LEN);
    src->parent = dst_parent;
    src->sibling = dst_parent->child;
    dst_parent->child = src;

    return 0;
}

int fs_copy(const char *src_path, const char *dst_path)
{
    if (!src_path || !dst_path) return -1;

    fs_node_t* src = fs_resolve(src_path, 0, NULL);
    if (!src) return -1;
    if (src->is_dir) return -1; /* only files supported */

    /* resolve destination parent and name */
    char last[MAX_NAME_LEN];
    fs_node_t* dst_parent = fs_resolve(dst_path, 1, last);
    if (!dst_parent || !dst_parent->is_dir) return -1;
    if (!last[0]) return -1;

    if (fs_find_in_dir(dst_parent, last))
        return -1;

    fs_node_t* n = fs_new_node(last, 0);
    if (!n) return -1;
    n->parent = dst_parent;
    n->sibling = dst_parent->child;
    dst_parent->child = n;

    if (src->data && src->size > 0) {
        char* buf = (char*)kmalloc(src->size + 1);
        if (!buf) return -1;
        for (uint32_t i = 0; i < src->size; i++)
            buf[i] = src->data[i];
        buf[src->size] = 0;
        n->data = buf;
        n->size = src->size;
    }

    return 0;
}

static void fs_find_walk(fs_node_t* node, char* pathbuf, size_t pathlen, const char* needle)
{
    while (node) {
        size_t orig_len = pathlen;
        if (pathlen == 1 && pathbuf[0] == '/') {
            size_t i = 1;
            for (size_t j = 0; node->name[j] && i + j + 1 < MAX_PATH_LEN; j++)
                pathbuf[i + j] = node->name[j];
            pathlen = orig_len + kstrlen(node->name);
            pathbuf[pathlen] = 0;
        } else {
            if (pathlen + 1 < MAX_PATH_LEN) {
                pathbuf[pathlen++] = '/';
                pathbuf[pathlen] = 0;
            }
            for (size_t j = 0; node->name[j] && pathlen + j + 1 < MAX_PATH_LEN; j++)
                pathbuf[pathlen + j] = node->name[j];
            pathlen += kstrlen(node->name);
            pathbuf[pathlen] = 0;
        }

        if (kstrstr(node->name, needle) != NULL) {
            console_write(pathbuf);
            console_write("\n");
        }

        if (node->is_dir && node->child) {
            fs_find_walk(node->child, pathbuf, pathlen, needle);
        }

        pathbuf[orig_len] = 0;
        pathlen = orig_len;

        node = node->sibling;
    }
}

int fs_find(const char *name)
{
    if (!name || !*name) return -1;
    if (!fs_root) return -1;

    char buf[MAX_PATH_LEN];
    buf[0] = '/';
    buf[1] = 0;
    if (fs_root->child)
        fs_find_walk(fs_root->child, buf, 1, name);
    return 0;
}

static void fs_tree_walk(fs_node_t* node, fs_tree_cb cb, int depth)
{
    while (node) {
        cb(node->name, node->is_dir, depth);

        if (node->is_dir && node->child) {
            fs_tree_walk(node->child, cb, depth + 1);
        }

        node = node->sibling;
    }
}

void fs_tree_cwd(fs_tree_cb cb)
{
    if (!cb || !fs_cwd || !fs_cwd->is_dir)
        return;

    fs_tree_walk(fs_cwd->child, cb, 0);
}
