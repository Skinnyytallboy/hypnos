#include <stdint.h>
#include "user/syscall.h"

static void u_itoa(uint32_t v, char *out)
{
    char tmp[16];
    int i = 0;

    if (v == 0) {
        out[0] = '0';
        out[1] = 0;
        return;
    }

    while (v && i < 15) {
        tmp[i++] = '0' + (v % 10);
        v /= 10;
    }

    int j = 0;
    while (i > 0) {
        out[j++] = tmp[--i];
    }
    out[j] = 0;
}

void user_program_main(void)
{
    sys_puts("\n[USER] Hello from Ring 3 via syscall!!!\n");

    uint32_t ticks = sys_get_ticks();
    char buf[32];
    u_itoa(ticks, buf);

    sys_puts("[USER] Kernel says ticks = ");
    sys_puts(buf);
    sys_puts("\n");

    /* For now, just spin in user mode.
       Later we'll add SYS_EXIT or scheduling to return to shell. */
    while (1) {
        __asm__ volatile("hlt");
    }
}



// void user_program_main(void)
// {
//     sys_write("\n[USER] Hello from Ring 3 via syscalls!\n");
//     sys_write("[USER] I am running in user mode with restricted privileges.\n");
//     sys_write("[USER] Trying to run privileged instructions will crash me.\n");

//     uint32_t last = 0;

//     while (1) {
//         uint32_t t = sys_get_ticks();

//         /* Print once per second (100 Hz timer) */
//         if (t / 100 != last) {
//             last = t / 100;

//             sys_write("[USER] tick second: ");
//             // cheap decimal print: build string on stack
//             char buf[16];
//             uint32_t v = last;
//             int i = 0;
//             if (v == 0) {
//                 buf[i++] = '0';
//             } else {
//                 char tmp[16];
//                 int j = 0;
//                 while (v > 0 && j < 15) {
//                     tmp[j++] = '0' + (v % 10);
//                     v /= 10;
//                 }
//                 while (j > 0) {
//                     buf[i++] = tmp[--j];
//                 }
//             }
//             buf[i++] = '\n';
//             buf[i]   = 0;
//             sys_write(buf);
//         }
//     }
// }
