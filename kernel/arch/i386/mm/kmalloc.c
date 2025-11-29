#include "kmalloc.h"
#include "console.h"

#define KHEAP_START 0x1000000   // 16MB mark (safe unused RAM)
#define KHEAP_SIZE  0x100000    // 1MB heap

static uint8_t* heap = (uint8_t*)KHEAP_START;
static uint32_t heap_offset = 0;

void kmalloc_init(void)
{
    console_write("Kernel heap initialized.\n");
    heap_offset = 0;
}

void* kmalloc(uint32_t size)
{
    if (heap_offset + size >= KHEAP_SIZE) {
        console_write("kmalloc: OUT OF MEMORY!\n");
        return 0;
    }

    void* ptr = heap + heap_offset;
    heap_offset += size;

    return ptr;
}
