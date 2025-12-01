#include "fs/blockdev.h"

static block_device_t *g_root_dev = 0;

block_device_t *blockdev_get_root(void)
{
    return g_root_dev;
}

void blockdev_set_root(block_device_t *dev)
{
    g_root_dev = dev;
}
