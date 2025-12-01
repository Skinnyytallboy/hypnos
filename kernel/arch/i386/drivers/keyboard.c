extern void shell_keypress(char c);

#include <stdint.h>
#include "arch/i386/cpu/irq.h"
#include "console.h"

static inline uint8_t inb(uint16_t port) {
    uint8_t ret;
    __asm__ volatile ("inb %1, %0" : "=a"(ret) : "Nd"(port));
    return ret;
}

/* very simple US QWERTY scancode set 1 map, no numpad */
static const char keymap[128] = {
    0,  27, '1','2','3','4','5','6','7','8','9','0','-','=', '\b',   // 0x00–0x0E
    '\t',                       // 0x0F
    'q','w','e','r','t','y','u','i','o','p','[',']','\n',            // 0x10–0x1C
    0,                          // 0x1D control
    'a','s','d','f','g','h','j','k','l',';','\'','`',                // 0x1E–0x29
    0, '\\','z','x','c','v','b','n','m',',','.','/', 0, '*',         // 0x2A–0x37
    0, ' ',                                                         // 0x38–0x39 (alt, space)
    /* rest unused here */
};

/* Shifted version (when Shift is held) */
static const char keymap_shift[128] = {
    0,  27, '!', '@', '#', '$', '%', '^', '&', '*', '(', ')', '_', '+', '\b', // 0x00–0x0E
    '\t',                       // 0x0F
    'Q','W','E','R','T','Y','U','I','O','P','{','}','\n',            // 0x10–0x1C
    0,                          // 0x1D control
    'A','S','D','F','G','H','J','K','L',':','"','~',                 // 0x1E–0x29
    0, '|','Z','X','C','V','B','N','M','<','>','?', 0, '*',          // 0x2A–0x37
    0, ' ',                                                         // 0x38–0x39
};

/* modifier state */
static int shift_pressed = 0;
static int ctrl_pressed  = 0;
static int alt_pressed   = 0;
static int caps_lock     = 0;

/* scancode constants (set 1) */
#define SC_LSHIFT   0x2A
#define SC_RSHIFT   0x36
#define SC_CTRL     0x1D
#define SC_ALT      0x38
#define SC_CAPS     0x3A

static int is_letter(char c)
{
    return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z');
}

static void keyboard_callback(void)
{
    uint8_t scancode = inb(0x60);

    /* key release? (high bit set) */
    if (scancode & 0x80) {
        uint8_t code = scancode & 0x7F;

        if (code == SC_LSHIFT || code == SC_RSHIFT)
            shift_pressed = 0;
        else if (code == SC_CTRL)
            ctrl_pressed = 0;
        else if (code == SC_ALT)
            alt_pressed = 0;

        return;
    }

    if (scancode == SC_LSHIFT || scancode == SC_RSHIFT) {
        shift_pressed = 1;
        return;
    }
    if (scancode == SC_CTRL) {
        ctrl_pressed = 1;
        return;
    }
    if (scancode == SC_ALT) {
        alt_pressed = 1;
        return;
    }
    if (scancode == SC_CAPS) {
        caps_lock = !caps_lock;
        return;
    }

    char c = 0;
    if (scancode < 128) {
        char base = keymap[scancode];

        if (!base)
            return;

        int use_shift = shift_pressed;

        if (is_letter(base)) {
            if (caps_lock)
                use_shift = !use_shift;

            if (use_shift) {
                if (base >= 'a' && base <= 'z')
                    base = (char)(base - 'a' + 'A');
            } else {
                if (base >= 'A' && base <= 'Z')
                    base = (char)(base - 'A' + 'a');
            }
            c = base;
        } else {
            if (use_shift)
                c = keymap_shift[scancode] ? keymap_shift[scancode] : base;
            else
                c = base;
        }
    }

    if (c) {
        shell_keypress(c);
    }
}

void keyboard_install(void) {
    /* IRQ1 is keyboard */
    irq_register_handler(1, keyboard_callback);
    console_write("Keyboard driver installed.\n");
}
