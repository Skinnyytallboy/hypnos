// kernel/fs_bootstrap.c
#include "fs/fs.h"
#include "console.h"

/*
 * Create an initial directory structure on the root filesystem
 * the FIRST time the OS boots on a fresh disk.
 *
 * We detect "first time" using a sentinel file: /.hypnos_root_initialized
 */
void fs_bootstrap(void)
{
    // Save current working directory
    const char *old_cwd = fs_getcwd();
    char saved_cwd[128];
    int i = 0;
    while (old_cwd[i] && i < (int)sizeof(saved_cwd) - 1) {
        saved_cwd[i] = old_cwd[i];
        i++;
    }
    saved_cwd[i] = 0;

    // Work from root
    fs_chdir("/");

    // If sentinel exists, assume filesystem already initialized
    const char *sentinel = fs_read(".hypnos_root_initialized");
    if (sentinel) {
        console_write("fs_bootstrap: filesystem already initialized.\n");
        fs_chdir(saved_cwd);
        return;
    }

    console_write("fs_bootstrap: creating initial filesystem layout...\n");

    // --- top-level dirs ---
    fs_mkdir("bin");
    fs_mkdir("home");
    fs_mkdir("etc");
    fs_mkdir("var");

    // /var subdirs
    fs_chdir("/var");
    fs_mkdir("log");
    fs_chdir("/");

    // /home/root and /home/student
    fs_chdir("/home");
    fs_mkdir("root");
    fs_mkdir("student");
    fs_chdir("/");

    // --- files ---

    // /etc/motd
    fs_chdir("/etc");
    fs_write("motd",
        "Welcome to Hypnos OS!\n"
        "This is a demo filesystem stored on an 8GB virtual disk.\n"
        "Commands: ls, cd, pwd, mkdir, touch, write, cat, edit, snap-*, log, whoami, users ...\n"
    );
    fs_chdir("/");

    // /home/root/readme.txt
    fs_chdir("/home/root");
    fs_write("readme.txt",
        "Hypnos root README\n"
        "-------------------\n"
        "This directory is meant for the 'root' user.\n"
        "Try creating files here, editing them, and rebooting to\n"
        "confirm that the filesystem is persistent on disk.\n"
    );
    fs_chdir("/");

    // /home/student/readme.txt
    fs_chdir("/home/student");
    fs_write("readme.txt",
        "Hypnos student README\n"
        "----------------------\n"
        "Welcome! You can practice shell commands here.\n"
        "Try: 'ls', 'pwd', 'mkdir demo', 'cd demo', 'edit notes.txt'.\n"
    );
    fs_chdir("/");

    // Sentinel to mark that bootstrap already ran
    fs_chdir("/");
    fs_write(".hypnos_root_initialized", "1");

    console_write("fs_bootstrap: initial filesystem created.\n");

    // restore previous working directory
    fs_chdir(saved_cwd);
}
