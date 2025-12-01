#pragma once
#include <stdint.h>
#include "fs/blockdev.h"

/* Initialize primary ATA PIO disk (if present).
 * Returns block_device_t* or NULL if nothing detected.
 */
block_device_t *ata_pio_init(void);
