#pragma once
#include <stdint.h>
#include "fs/blockdev.h"

/*
 * Create a ramdisk “disk” of `size_bytes` and return it as a block_device_t.
 * Sector size = 512 bytes.
 */
block_device_t *ramdisk_create(uint64_t size_bytes);
