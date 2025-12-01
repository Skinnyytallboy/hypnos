# Hypnos OS

*A from-scratch x86 Operating System with Paging, Filesystem, ATA Storage, Shell, Security, User Mode, and Much More*

> “Writing an OS from scratch is like performing heart surgery on yourself:
> you can do it, but you will question your life choices along the way.”

---

# Table of Contents

1. [Introduction](#introduction)
2. [Features Overview](#features-overview)
3. [System Architecture](#system-architecture)
4. [Project Directory Structure](#project-directory-structure)
5. [Build & Run Instructions](#build--run-instructions)
6. [Boot Process Explained](#boot-process-explained)
7. [Memory Management](#memory-management)
8. [Filesystem & Disk Layer](#filesystem--disk-layer)
9. [Shell & User Tools](#shell--user-tools)
10. [Security Subsystem](#security-subsystem)
11. [Syscalls & User Mode](#syscalls--user-mode)
12. [Scheduler & Multitasking](#scheduler--multitasking)
13. [What Works / What’s Broken](#what-works--whats-broken)
14. [Roadmap](#roadmap)
15. [License](#license)

---

# Introduction

Hypnos is a custom-built x86 operating system designed as an academic, research, and experimental kernel. What began as a simple bootloader experiment quickly escalated into:

* A *real* kernel
* With *real* drivers
* With *real* persistent storage
* With *real* paging
* With *real* memory allocation
* And a *real* filesystem

This project is now feature-rich enough to resemble an early UNIX-like OS while still being small enough to understand.

The philosophy behind Hypnos is simple:
**Everything is built manually. No shortcuts, no libc, no POSIX, no BIOS calls beyond boot.**

---

# Features Overview

A high-level summary of what Hypnos supports today:

### Boot & CPU Setup

* Multiboot-compliant entry point
* GDT with proper code/data segments
* Protected mode switching
* IDT with all 256 entries
* Exception handlers for vectors 0–31
* PIC remapping
* IRQ handling (timer and keyboard)

### Memory Management

* **2 GB physical memory map** using a bitmap allocator
* Identity-mapped **2 GB virtual memory**
* Paging enabled from the first instruction in C
* Kernel heap (1 MB bump allocator)
* Frame allocation for additional mappings

### Storage & Filesystem

* **8 GB virtual disk** (ATA PIO mode)
* **RAM disk** for testing
* Block device abstraction layer
* Custom filesystem:

  * Directories
  * Files
  * Permissions hooks
  * Snapshots
  * Path traversal
  * Persistent metadata

### Shell

A fully interactive shell with:

* Prompt
* Colors & themes
* Status bar with uptime, user, cwd, FS label
* Commands: `ls, cd, pwd, mkdir, touch, write, cat, edit, whoami, login, snap-*`
* Built-in text editor

### Users & Security

* User accounts (`root`, `student`, ...)
* Login system
* Simple permission hooks
* Audit logging subsystem

### User Mode

* Ring 3 execution
* Separate user stack
* syscalls (`int 0x80`) partially implemented
* Example user program running in Ring 3

### Scheduler (Cooperative for now)

* Kernel threads
* Round-robin cooperative switching
* Shell as a task
* Background tasks (demo task)
* Timer tick system

---

# System Architecture

```
+---------------------------------------------------------------+
|                           User Mode                           |
|                          (Ring 3)                             |
|                syscalls → INT 0x80 → kernel                   |
+------------------------^-------------------+------------------+
                         |                   |
+------------------------|-------------------v------------------+
|                     Kernel Mode (Ring 0)                      |
|  +---------------+  +---------------+  +--------------------+ |
|  |  Scheduler    |  | Syscall Layer |  |   Virtual Memory   | |
|  +---------------+  +---------------+  +--------------------+ |
|  +---------------+  +---------------+  +--------------------+ |
|  |   Filesystem  |  |  Block Layer  |  |  Physical Memory   | |
|  +---------------+  +---------------+  +--------------------+ |
|  +--------------+   +---------------+   +-------------------+ |
|  |  IRQ/IDT/GDT |   |  Drivers      |   | Kernel Heap       | |
+---------------------------------------------------------------+
```

---

# Project Directory Structure

```
hypnos/
├── Makefile
├── boot/
│   └── grub/
│       └── grub.cfg
├── kernel/
│   ├── arch/i386/
│   │   ├── cpu/        # GDT, IDT, ISR, IRQ, TSS, syscall stub
│   │   ├── drivers/    # Keyboard, timer, ATA PIO
│   │   ├── mm/         # Paging, physmem, kmalloc
│   │   └── start/      # Multiboot entry, context_switch, user_mode
│   ├── fs/             # Filesystem, blockdev, crypto, ramdisk
│   ├── sched/          # Task scheduler (cooperative)
│   ├── shell/          # Shell + editor
│   ├── user/           # User-mode programs + syscall wrappers
│   ├── log.c           # Audit log
│   ├── security.c      # User accounts & permissions
│   ├── syscall.c       # Kernel syscall dispatcher
│   ├── console.h       # VGA console driver
│   └── kernel_main.c   # Kernel entry point in C
├── linker.ld           # Kernel linker script
└── setup-wsl.sh        # WSL build helper
```

---

# Build & Run Instructions

## Requirements

* GCC cross-compiler for i386 (optional)
* GNU Make
* QEMU

### Build

```
make clean
make
```

### Run (QEMU)

```
make run
```

By default:

* RAM is set to **2 GB**
* Disk is **8 GB**
* Bootloader is GRUB2 (multiboot)

---

# Boot Process Explained

The full boot sequence:

1. **GRUB loads our kernel** (multiboot)
2. `entry.S` sets up:

   * stack
   * multiboot info
   * calls `kernel_main`
3. `kernel_main` initializes:

   * GDT
   * IDT
   * IRQs
   * Paging
   * Physical allocator
   * Heap
   * Disk device (ATA)
   * Filesystem
   * Keyboard, timer
   * User subsystem
4. Displays ASCII banner
5. Creates tasks (shell)
6. Starts task scheduler
7. Shell appears

---

# Memory Management

### Physical Memory (2 GB)

* Bitmap allocator
* 1 bit per frame
* Total frames = 2GB / 4KB = 524,288 frames

### Paging (Full Identity Map)

* PDE count for 2GB = 2048MB / 4MB = 512 PDEs
* Each PDE maps a page table of 1024 entries (4KB each)
* Flags:

  * `PRESENT`
  * `RW`
  * `USER`

### Kernel Heap

* Located at 16 MB
* Simple bump allocator (1 MB)

---

# Filesystem & Disk Layer

The FS is split into layers:

### Block Device Layer

```
blockdev_t {
    name
    num_sectors
    read()
    write()
}
```

### Supported Devices

* **RAMDISK** – used for fallback
* **ATA PIO** – real persistent storage (8 GB)

### Filesystem Features

* Directories
* Files
* Snapshots
* Encryption hooks
* Permissions hooks
* Persistent structure on disk

### Auto-Created FS (first boot)

```
/
├── bin/
├── home/
│   ├── root/
│   └── student/
├── etc/
│   └── motd
└── var/
    └── log/
```

---

# Shell & User Tools

### Supports:

* Navigation: `ls`, `cd`, `pwd`
* File ops: `cat`, `write`, `touch`, `mkdir`
* Snapshots: `snap-create`, `snap-restore`, `snap-list`
* Security: `whoami`, `login`
* Logging: `log`
* Text editor: `edit file`
* Uptime, memory, prompt, colors

Looks like:

```
root@hypnos/home> 
```

Bottom row is status bar:

```
 user: root | uptime: 53s | fs: RAM-FS | cwd: /home/root
```

---

# Security Subsystem

### Users:

* root (default)
* Additional users can be added

### Features:

* Login system
* User permissions
* Audit logging (`log_event`)

---

# Syscalls & User Mode

User processes run in **Ring 3**.

### Syscall Flow

```
user → int 0x80 → isr80_syscall → syscall_handler()
```

### Working Syscalls

* `SYS_PUTS` – print string
* `SYS_GET_TICKS` – return uptime

### Example User Program

```
[USER] Hello from Ring 3!
[USER] one more second in Ring 3...
```

(Still unstable, work pending.)

---

# Scheduler & Multitasking

### Current State

* Cooperative scheduler
* Simple task struct:

  * Register save area
  * Stack
  * Next pointer
* Round-robin switching
* Shell runs as its own task

### Missing Features

* Preemptive switching
* Kernel → user trapframes
* Sleep, wait, blocking I/O
* Per-task paging

---

# What Works / What’s Broken

## Works

* Boot
* Paging
* Heap
* Physical memory
* ATA disk
* Filesystem (persistent)
* Shell
* Logging
* User accounts
* Status bar
* Cooperative tasks
* Basic syscalls

## Broken / Pending

* Syscall numbers sometimes corrupted (mysterious 16, 0)
* User mode freezing after first syscall
* Preemptive scheduler not implemented
* No ELF loader
* No userland processes
* No memory protection between processes

---

# Roadmap

### Phase 1 — Base System (Done)

* Boot, GDT, IDT
* Paging
* Physical allocator
* Shell
* ATA disk
* Filesystem

### Phase 2 — Stable Syscalls (Pending)

* Fix argument extraction
* Implement write/read/open/close/exit

### Phase 3 — Preemptive Scheduler

* Timer-driven context switching
* User stack frames
* Killable tasks

### Phase 4 — ELF Loader

* Load programs from `/bin`
* exec(), fork()

### Phase 5 — Device Drivers

* VGA accelerated text
* Mouse
* PCI scanning

### Phase 6 — Networking (optional)

* NE2000 / RTL8139
* Basic TCP/IP stack

---
