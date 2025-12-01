#pragma once
#include <stdint.h>
#include "fs/blockdev.h"

/*
 * Create a ramdisk “disk” of `size_bytes` and return it as a block_device_t.
 * Sector size = 512 bytes.
 *
 * NOTE: Physically you should pick something like 8MB or 16MB,
 * not 16GB, to avoid exhausting your RAM.
 */
block_device_t *ramdisk_create(uint64_t size_bytes);
