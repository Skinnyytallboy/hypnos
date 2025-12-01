#include <stdint.h>
#include "physmem.h"
#include "console.h"

#define PAGE_SIZE       4096
#define MAX_MEM_BYTES   (2048u * 1024u * 1024u)   // 2GB as *unsigned*
#define MAX_FRAMES      (MAX_MEM_BYTES / PAGE_SIZE)

// bitmap: 1 bit per frame
static uint32_t frame_bitmap[MAX_FRAMES / 32];

extern uint32_t kernel_start;
extern uint32_t kernel_end;

static inline void set_frame(uint32_t frame) {
    frame_bitmap[frame / 32] |=  (1u << (frame % 32));
}

static inline void clear_frame(uint32_t frame) {
    frame_bitmap[frame / 32] &= ~(1u << (frame % 32));
}

static inline int test_frame(uint32_t frame) {
    return frame_bitmap[frame / 32] & (1u << (frame % 32));
}

static uint32_t first_free_frame(void)
{
    for (uint32_t i = 0; i < MAX_FRAMES / 32; i++) {
        if (frame_bitmap[i] != 0xFFFFFFFFu) {
            // there is at least one free bit
            for (uint32_t j = 0; j < 32; j++) {
                uint32_t frame = i * 32 + j;
                if (frame < MAX_FRAMES && !test_frame(frame)) {
                    return frame;
                }
            }
        }
    }
    return (uint32_t)-1;
}

void phys_init(void)
{
    console_write("Initializing physical memory allocator...\n");

    // clear bitmap
    for (uint32_t i = 0; i < MAX_FRAMES / 32; i++) {
        frame_bitmap[i] = 0;
    }

    // mark frames up to end of kernel as used
    uint32_t kernel_end_addr = (uint32_t)&kernel_end;
    uint32_t first_free_addr = (kernel_end_addr + PAGE_SIZE - 1) & ~(PAGE_SIZE - 1);

    uint32_t first_free_frame_idx = first_free_addr / PAGE_SIZE;

    for (uint32_t i = 0; i < first_free_frame_idx; i++) {
        set_frame(i);
    }

    console_write("Physical memory allocator ready.\n");
}

uint32_t phys_alloc_frame(void)
{
    uint32_t frame = first_free_frame();
    if (frame == (uint32_t)-1) {
        console_write("phys_alloc_frame: OUT OF MEMORY!\n");
        return 0;
    }

    set_frame(frame);
    return frame * PAGE_SIZE;
}

void phys_free_frame(uint32_t addr)
{
    if (addr == 0) return;
    uint32_t frame = addr / PAGE_SIZE;
    if (frame < MAX_FRAMES) {
        clear_frame(frame);
    }
}
