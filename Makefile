TARGET      := kernel.elf
BUILD       := build
ISO         := $(BUILD)/hypnos.iso
ISODIR      := $(BUILD)/isodir

CC32        := gcc -m32
LD32        := ld -m elf_i386

CFLAGS      := -std=c11 -ffreestanding -fno-stack-protector -fno-pie -O2 -Wall -Wextra
ASFLAGS     := -m32 -ffreestanding -fno-pie -O2
LDFLAGS     := -nostdlib

OBJS        := $(BUILD)/entry.o $(BUILD)/kernel_main.o

.PHONY: all run iso clean

all: iso

$(BUILD)/entry.o: kernel/entry.S
	@mkdir -p $(BUILD)
	$(CC32) -ffreestanding -fno-pie -O2 -c $< -o $@

$(BUILD)/kernel_main.o: kernel/kernel_main.c
	@mkdir -p $(BUILD)
	$(CC32) $(CFLAGS) -c $< -o $@

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
	qemu-system-i386 -cdrom build/hypnos.iso -m 256 -display gtk -no-reboot -no-shutdown

run-sdl: iso
	qemu-system-i386 -cdrom build/hypnos.iso -m 256 -display sdl -no-reboot -no-shutdown

run-curses: iso
	qemu-system-i386 -cdrom $(ISO) -m 256 -display curses -no-reboot -no-shutdown

clean:
	rm -rf $(BUILD)
