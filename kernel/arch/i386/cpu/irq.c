#include "irq.h"
#include "idt.h"
#include "console.h"

#define PIC1        0x20
#define PIC2        0xA0
#define PIC1_COMMAND PIC1
#define PIC1_DATA    (PIC1+1)
#define PIC2_COMMAND PIC2
#define PIC2_DATA    (PIC2+1)

#define PIC_EOI     0x20

static irq_handler_t irq_handlers[16] = {0};

static inline void outb(uint16_t port, uint8_t value) {
    __asm__ volatile ("outb %0, %1" : : "a"(value), "Nd"(port));
}

static inline uint8_t inb(uint16_t port) {
    uint8_t ret;
    __asm__ volatile ("inb %1, %0" : "=a"(ret) : "Nd"(port));
    return ret;
}

void irq_handler_c(int irq_no)
{
    if (irq_no < 16 && irq_handlers[irq_no]) {
        irq_handlers[irq_no]();
    }

    /* send EOI to PICs */
    if (irq_no >= 8) {
        outb(PIC2_COMMAND, PIC_EOI);
    }
    outb(PIC1_COMMAND, PIC_EOI);
}

void irq_register_handler(int irq, irq_handler_t handler)
{
    if (irq >= 0 && irq < 16)
        irq_handlers[irq] = handler;
}

void irq_install(void)
{
    uint8_t a1 = inb(PIC1_DATA);
    uint8_t a2 = inb(PIC2_DATA);

    /* starts the initialization sequence (in cascade mode) */
    outb(PIC1_COMMAND, 0x11);
    outb(PIC2_COMMAND, 0x11);
    /* ICW2: Master PIC vector offset */
    outb(PIC1_DATA, 0x20);    // IRQ0–7 -> IDT 32–39
    /* ICW2: Slave PIC vector offset */
    outb(PIC2_DATA, 0x28);    // IRQ8–15 -> IDT 40–47
    /* ICW3: Cascading setup */
    outb(PIC1_DATA, 0x04);    // master has a slave at IRQ2
    outb(PIC2_DATA, 0x02);    // slave identity
    /* ICW4: Environment info */
    outb(PIC1_DATA, 0x01);
    outb(PIC2_DATA, 0x01);
    /* restore masks */
    outb(PIC1_DATA, a1);
    outb(PIC2_DATA, a2);

    extern void irq0();
    extern void irq1();
    extern void irq2();
    extern void irq3();
    extern void irq4();
    extern void irq5();
    extern void irq6();
    extern void irq7();
    extern void irq8();
    extern void irq9();
    extern void irq10();
    extern void irq11();
    extern void irq12();
    extern void irq13();
    extern void irq14();
    extern void irq15();

    idt_set_gate(32, (uint32_t)irq0,  0x08, 0x8E);
    idt_set_gate(33, (uint32_t)irq1,  0x08, 0x8E);
    idt_set_gate(34, (uint32_t)irq2,  0x08, 0x8E);
    idt_set_gate(35, (uint32_t)irq3,  0x08, 0x8E);
    idt_set_gate(36, (uint32_t)irq4,  0x08, 0x8E);
    idt_set_gate(37, (uint32_t)irq5,  0x08, 0x8E);
    idt_set_gate(38, (uint32_t)irq6,  0x08, 0x8E);
    idt_set_gate(39, (uint32_t)irq7,  0x08, 0x8E);
    idt_set_gate(40, (uint32_t)irq8,  0x08, 0x8E);
    idt_set_gate(41, (uint32_t)irq9,  0x08, 0x8E);
    idt_set_gate(42, (uint32_t)irq10, 0x08, 0x8E);
    idt_set_gate(43, (uint32_t)irq11, 0x08, 0x8E);
    idt_set_gate(44, (uint32_t)irq12, 0x08, 0x8E);
    idt_set_gate(45, (uint32_t)irq13, 0x08, 0x8E);
    idt_set_gate(46, (uint32_t)irq14, 0x08, 0x8E);
    idt_set_gate(47, (uint32_t)irq15, 0x08, 0x8E);
}
