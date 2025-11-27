extern void shell_keypress(char c);

#include <stdint.h>
#include "irq.h"
#include "console.h"

static inline uint8_t inb(uint16_t port) {
    uint8_t ret;
    __asm__ volatile ("inb %1, %0" : "=a"(ret) : "Nd"(port));
    return ret;
}

/* very simple US QWERTY scancode set 1 map, no shift/caps yet */
static const char keymap[128] = {
    0,  27, '1','2','3','4','5','6','7','8','9','0','-','=', '\b',   // 0x00
    '\t',                       // 0x0F
    'q','w','e','r','t','y','u','i','o','p','[',']','\n',            // 0x10
    0,                          // control
    'a','s','d','f','g','h','j','k','l',';','\'','`',                // 0x1E
    0, '\\','z','x','c','v','b','n','m',',','.','/', 0, '*',         // 0x2C
    0, ' ',                                                         // 0x39 space
    /* rest unused here */
};

static void keyboard_callback(void)
{
    uint8_t scancode = inb(0x60);

    /* ignore key releases (high bit set) for now */
    if (scancode & 0x80)
        return;
    char c = 0;
    if (scancode < 128)
        c = keymap[scancode];
    if (c) shell_keypress(c);
    // if (c) {
    //     if (c == '\b') {
    //         console_write("\b");
    //     } else if (c == '\n') {
    //         console_write("\n");
    //     } else {
    //         char s[2] = { c, 0 };
    //         console_write(s);
    //     }
    // }
}

void keyboard_install(void) {
    /* IRQ1 is keyboard */
    irq_register_handler(1, keyboard_callback);
    console_write("Keyboard driver installed.\n");
}
