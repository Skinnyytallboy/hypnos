#include "arch/i386/drivers/ata_pio.h"
#include "console.h"

#define ATA_PRIMARY_IO     0x1F0
#define ATA_PRIMARY_CTRL   0x3F6

#define ATA_REG_DATA       0x00
#define ATA_REG_SECCOUNT0  0x02
#define ATA_REG_LBA0       0x03
#define ATA_REG_LBA1       0x04
#define ATA_REG_LBA2       0x05
#define ATA_REG_HDDEVSEL   0x06
#define ATA_REG_COMMAND    0x07
#define ATA_REG_STATUS     0x07

#define ATA_CMD_READ_PIO   0x20
#define ATA_CMD_WRITE_PIO  0x30

#define ATA_SR_BSY         0x80
#define ATA_SR_DRQ         0x08
#define ATA_SR_ERR         0x01

#define SECTOR_SIZE 512

/* I/O helpers */
static inline void outb(uint16_t port, uint8_t val) {
    __asm__ volatile ("outb %0, %1" : : "a"(val), "Nd"(port));
}

static inline uint8_t inb(uint16_t port) {
    uint8_t ret;
    __asm__ volatile ("inb %1, %0" : "=a"(ret) : "Nd"(port));
    return ret;
}

static inline void outw(uint16_t port, uint16_t val) {
    __asm__ volatile ("outw %0, %1" : : "a"(val), "Nd"(port));
}

static inline uint16_t inw(uint16_t port) {
    uint16_t ret;
    __asm__ volatile ("inw %1, %0" : "=a"(ret) : "Nd"(port));
    return ret;
}

/* Busy wait until BSY=0, then DRQ=1 or error */
static int ata_wait(void)
{
    uint8_t status;

    // Wait for BSY clear
    do {
        status = inb(ATA_PRIMARY_IO + ATA_REG_STATUS);
    } while (status & ATA_SR_BSY);

    // Wait for DRQ set or error
    while (!(status & (ATA_SR_DRQ | ATA_SR_ERR))) {
        status = inb(ATA_PRIMARY_IO + ATA_REG_STATUS);
    }

    if (status & ATA_SR_ERR)
        return -1;
    return 0;
}

typedef struct ata_dev {
    block_device_t dev;  // must be first
} ata_dev_t;


/* LBA28 read/write, blocking, count <= 255 */
static int ata_read_sectors(uint32_t lba, uint8_t count, void *buffer)
{
    if (count == 0) return 0;

    outb(ATA_PRIMARY_CTRL, 0x00); // disable IRQs for simplicity

    outb(ATA_PRIMARY_IO + ATA_REG_HDDEVSEL, 0xE0 | ((lba >> 24) & 0x0F));
    outb(ATA_PRIMARY_IO + ATA_REG_SECCOUNT0, count);
    outb(ATA_PRIMARY_IO + ATA_REG_LBA0, (uint8_t)(lba & 0xFF));
    outb(ATA_PRIMARY_IO + ATA_REG_LBA1, (uint8_t)((lba >> 8) & 0xFF));
    outb(ATA_PRIMARY_IO + ATA_REG_LBA2, (uint8_t)((lba >> 16) & 0xFF));
    outb(ATA_PRIMARY_IO + ATA_REG_COMMAND, ATA_CMD_READ_PIO);

    uint16_t *buf = (uint16_t *)buffer;

    for (uint8_t s = 0; s < count; ++s) {
        if (ata_wait() != 0) return -1;

        for (int i = 0; i < SECTOR_SIZE / 2; ++i) {
            buf[s * (SECTOR_SIZE / 2) + i] = inw(ATA_PRIMARY_IO + ATA_REG_DATA);
        }
    }

    return 0;
}

static int ata_write_sectors(uint32_t lba, uint8_t count, const void *buffer)
{
    if (count == 0) return 0;

    outb(ATA_PRIMARY_CTRL, 0x00); // disable IRQs for simplicity

    outb(ATA_PRIMARY_IO + ATA_REG_HDDEVSEL, 0xE0 | ((lba >> 24) & 0x0F));
    outb(ATA_PRIMARY_IO + ATA_REG_SECCOUNT0, count);
    outb(ATA_PRIMARY_IO + ATA_REG_LBA0, (uint8_t)(lba & 0xFF));
    outb(ATA_PRIMARY_IO + ATA_REG_LBA1, (uint8_t)((lba >> 8) & 0xFF));
    outb(ATA_PRIMARY_IO + ATA_REG_LBA2, (uint8_t)((lba >> 16) & 0xFF));
    outb(ATA_PRIMARY_IO + ATA_REG_COMMAND, ATA_CMD_WRITE_PIO);

    const uint16_t *buf = (const uint16_t *)buffer;

    for (uint8_t s = 0; s < count; ++s) {
        if (ata_wait() != 0) return -1;

        for (int i = 0; i < SECTOR_SIZE / 2; ++i) {
            outw(ATA_PRIMARY_IO + ATA_REG_DATA,
                 buf[s * (SECTOR_SIZE / 2) + i]);
        }
    }

    return 0;
}

/* blockdev glue */

static int ata_block_read(block_device_t *dev,
                          uint64_t lba,
                          uint32_t count,
                          void    *buffer)
{
    (void)dev;
    if (count == 0) return 0;
    if (lba > 0x0FFFFFFF) return -1; // beyond LBA28

    if (count > 255) count = 255;
    return ata_read_sectors((uint32_t)lba, (uint8_t)count, buffer);
}

static int ata_block_write(block_device_t *dev,
                           uint64_t lba,
                           uint32_t count,
                           const void *buffer)
{
    (void)dev;
    if (count == 0) return 0;
    if (lba > 0x0FFFFFFF) return -1;

    if (count > 255) count = 255;
    return ata_write_sectors((uint32_t)lba, (uint8_t)count, buffer);
}

block_device_t *ata_pio_init(void)
{
    static ata_dev_t dev;

    // Very dumb "probe": just assume disk present.
    dev.dev.name        = "ata0";
    dev.dev.num_sectors = (8192ULL * 1024ULL * 1024ULL) / SECTOR_SIZE;
    dev.dev.read        = ata_block_read;
    dev.dev.write       = ata_block_write;

    console_write("ata_pio: primary disk attached as /dev/ata0\n");
    return &dev.dev;
}
