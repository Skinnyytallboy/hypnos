#pragma once
#include <stdint.h>
#include <stddef.h>

typedef struct block_device block_device_t;

struct block_device {
    const char *name;
    uint64_t    num_sectors;   // logical size (512-byte sectors)

    int (*read)(block_device_t *dev,
                uint64_t lba,
                uint32_t count,
                void    *buffer);

    int (*write)(block_device_t *dev,
                 uint64_t lba,
                 uint32_t count,
                 const void *buffer);
};

/* For now we’ll just have a single global device, e.g. /dev/ram0 */
block_device_t *blockdev_get_root(void);
void            blockdev_set_root(block_device_t *dev);
