#include "fs/ramdisk.h"
#include "arch/i386/mm/kmalloc.h"
#include "console.h"

#define SECTOR_SIZE 512

typedef struct ramdisk {
    block_device_t dev;   // must be first
    uint8_t       *data;  // pointer to allocated memory
    uint64_t       size_bytes;
} ramdisk_t;

static int ramdisk_read(block_device_t *bdev,
                        uint64_t lba,
                        uint32_t count,
                        void    *buffer)
{
    ramdisk_t *rd = (ramdisk_t *)bdev;

    uint64_t offset = lba * SECTOR_SIZE;
    uint64_t bytes  = (uint64_t)count * SECTOR_SIZE;

    if (offset + bytes > rd->size_bytes)
        return -1; // out of range

    uint8_t *src = rd->data + offset;
    uint8_t *dst = (uint8_t *)buffer;

    for (uint64_t i = 0; i < bytes; ++i)
        dst[i] = src[i];

    return 0;
}

static int ramdisk_write(block_device_t *bdev,
                         uint64_t lba,
                         uint32_t count,
                         const void *buffer)
{
    ramdisk_t *rd = (ramdisk_t *)bdev;

    uint64_t offset = lba * SECTOR_SIZE;
    uint64_t bytes  = (uint64_t)count * SECTOR_SIZE;

    if (offset + bytes > rd->size_bytes)
        return -1; // out of range

    const uint8_t *src = (const uint8_t *)buffer;
    uint8_t       *dst = rd->data + offset;

    for (uint64_t i = 0; i < bytes; ++i)
        dst[i] = src[i];

    return 0;
}

block_device_t *ramdisk_create(uint64_t size_bytes)
{
    ramdisk_t *rd = (ramdisk_t *)kmalloc(sizeof(ramdisk_t));
    if (!rd) {
        console_write("ramdisk_create: kmalloc ramdisk_t failed\n");
        return 0;
    }

    rd->size_bytes = size_bytes;
    rd->data       = (uint8_t *)kmalloc((size_t)size_bytes);
    if (!rd->data) {
        console_write("ramdisk_create: kmalloc data failed\n");
        return 0;
    }

    // zero the disk
    for (uint64_t i = 0; i < size_bytes; ++i)
        rd->data[i] = 0;

    rd->dev.name        = "ram0";
    rd->dev.num_sectors = size_bytes / SECTOR_SIZE;
    rd->dev.read        = ramdisk_read;
    rd->dev.write       = ramdisk_write;

    console_write("ramdisk: created ");
    // you have kprint_u32 for 32-bit, so just print in MB-ish
    // (size_bytes is <= a few tens of MB in practice)
    return &rd->dev;
}
