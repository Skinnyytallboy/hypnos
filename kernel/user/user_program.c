#include <stdint.h>
#include "user/syscall.h"

void user_program_main(void)
{
    sys_puts("\n[USER] Hello from Ring 3 via syscall!!\n");
    sys_puts("[USER] This string is printed with INT 0x80.\n");

    while (1) {
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
