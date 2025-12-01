#include <stdint.h>
#include "user/syscall.h"

void user_program_main(void)
{
    sys_puts("[USER] Hello...\n");

    for (;;) {
        sys_puts(".");
        for (volatile int i = 0; i < 5000000; i++) { }
    }
    // sys_puts("[USER] Hello from Ring 3 via syscall!!!\n");
    // uint32_t last = 0;

    // for (;;) {
    // uint32_t t = sys_get_ticks();
    // if (t - last >= 100) {
    //     last = t;
    //     sys_puts("[USER] one more second in Ring 3...\n");
    // }
}
    // for (;;) {
    //     uint32_t t = sys_get_ticks();

    //     // DEBUG:
    //     if (t % 50 == 0) {  // print sometimes
    //         char buf[64];
    //         // tiny integer to string
    //         int i = 0;
    //         uint32_t x = t;
    //         char tmp[16];
    //         if (x == 0) { sys_puts("[USER] t=0\n"); }
    //         else {
    //             while (x > 0 && i < 10) {
    //                 tmp[i++] = '0' + (x % 10);
    //                 x /= 10;
    //             }
    //             for (int j = i-1; j >= 0; j--) {
    //                 char c[2] = {tmp[j], 0};
    //                 sys_puts(c);
    //             }
    //             sys_puts("\n");
    //         }
    //     }
    // }
// }




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
