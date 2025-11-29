TARGET      := kernel.elf
BUILD       := build
ISO         := $(BUILD)/hypnos.iso
ISODIR      := $(BUILD)/isodir

CC32        := gcc -m32
LD32        := ld -m elf_i386

CFLAGS      := -std=c11 -ffreestanding -fno-stack-protector -fno-pie -O2 -Wall -Wextra -Ikernel
ASFLAGS     := -m32 -ffreestanding -fno-pie -O2
LDFLAGS     := -nostdlib

OBJS        := \
	$(BUILD)/entry.o \
	$(BUILD)/kernel_main.o \
	$(BUILD)/gdt.o \
	$(BUILD)/gdt_flush.o \
	$(BUILD)/idt.o \
	$(BUILD)/isr.o \
	$(BUILD)/isr_s.o \
	$(BUILD)/irq.o \
	$(BUILD)/irq_s.o \
	$(BUILD)/timer.o \
	$(BUILD)/keyboard.o \
	$(BUILD)/shell.o \
	$(BUILD)/paging.o \
	$(BUILD)/kmalloc.o \
	$(BUILD)/physmem.o \
	${BUILD}/fs.o
# 	$(BUILD)/task.o \
# 	$(BUILD)/context_switch.o
	

.PHONY: all run iso clean run-gtk run-sdl run-curses

all: iso

$(BUILD)/fs.o: kernel/fs.c
	@mkdir -p $(BUILD)
	$(CC32) $(CFLAGS) -c $< -o $@

# $(BUILD)/task.o: kernel/task.c
# 	$(CC32) $(CFLAGS) -c $< -o $@

# $(BUILD)/context_switch.o: kernel/context_switch.S
# 	$(CC32) $(ASFLAGS) -c $< -o $@

$(BUILD)/physmem.o: kernel/physmem.c
	@mkdir -p $(BUILD)
	$(CC32) $(CFLAGS) -c $< -o $@

$(BUILD)/paging.o: kernel/paging.c
	@mkdir -p $(BUILD)
	$(CC32) $(CFLAGS) -c $< -o $@

$(BUILD)/kmalloc.o: kernel/kmalloc.c
	@mkdir -p $(BUILD)
	$(CC32) $(CFLAGS) -c $< -o $@

$(BUILD)/shell.o: kernel/shell.c
	@mkdir -p $(BUILD)
	$(CC32) $(CFLAGS) -c $< -o $@

$(BUILD)/keyboard.o: kernel/keyboard.c
	@mkdir -p $(BUILD)
	$(CC32) $(CFLAGS) -c $< -o $@

$(BUILD)/irq.o: kernel/irq.c
	@mkdir -p $(BUILD)
	$(CC32) $(CFLAGS) -c $< -o $@

$(BUILD)/irq_s.o: kernel/irq.S
	@mkdir -p $(BUILD)
	$(CC32) $(ASFLAGS) -c $< -o $@

$(BUILD)/timer.o: kernel/timer.c
	@mkdir -p $(BUILD)
	$(CC32) $(CFLAGS) -c $< -o $@

$(BUILD)/entry.o: kernel/entry.S
	@mkdir -p $(BUILD)
	$(CC32) $(ASFLAGS) -c $< -o $@

$(BUILD)/kernel_main.o: kernel/kernel_main.c
	@mkdir -p $(BUILD)
	$(CC32) $(CFLAGS) -c $< -o $@

$(BUILD)/gdt.o: kernel/gdt.c
	@mkdir -p $(BUILD)
	$(CC32) $(CFLAGS) -c $< -o $@

$(BUILD)/gdt_flush.o: kernel/gdt_flush.S
	@mkdir -p $(BUILD)
	$(CC32) $(ASFLAGS) -c $< -o $@

$(BUILD)/idt.o: kernel/idt.c
	@mkdir -p $(BUILD)
	$(CC32) $(CFLAGS) -c $< -o $@

$(BUILD)/isr.o: kernel/isr.c
	@mkdir -p $(BUILD)
	$(CC32) $(CFLAGS) -c $< -o $@

$(BUILD)/isr_s.o: kernel/isr.S
	@mkdir -p $(BUILD)
	$(CC32) $(ASFLAGS) -c $< -o $@

$(BUILD)/$(TARGET): $(OBJS) linker.ld
	$(LD32) -T linker.ld -o $@ $(OBJS)

iso: $(BUILD)/$(TARGET) boot/grub/grub.cfg
	@mkdir -p $(ISODIR)/boot/grub
	@cp $(BUILD)/$(TARGET) $(ISODIR)/boot/kernel.elf
	@cp boot/grub/grub.cfg $(ISODIR)/boot/grub/grub.cfg
	grub-mkrescue -o $(ISO) $(ISODIR)

run: iso
	qemu-system-i386 -cdrom $(ISO) -m 256

run-gtk: iso
	qemu-system-i386 -cdrom $(ISO) -m 256 -display gtk -no-reboot -no-shutdown

run-sdl: iso
	qemu-system-i386 -cdrom $(ISO) -m 256 -display sdl -no-reboot -no-shutdown

run-curses: iso
	qemu-system-i386 -cdrom $(ISO) -m 256 -display curses -no-reboot -no-shutdown

clean:
	rm -rf $(BUILD)
