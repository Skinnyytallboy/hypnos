#include <stdint.h>
#include "arch/i386/drivers/keyboard.h"
#include "console.h"

#define VGA_TEXT_BASE 0xB8000

void user_program_main(void)
{
    console_write("\n[USER] Hello from Ring 3!\n");
    console_write("[USER] I am running in user mode with restricted privileges.\n");
    console_write("[USER] Trying to run privileged instructions will crash me.\n");

    while (1);
}
