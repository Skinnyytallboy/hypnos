TARGET      := kernel.elf
BUILD       := build
ISO         := $(BUILD)/hypnos.iso
ISODIR      := $(BUILD)/isodir

CC32        := gcc -m32
LD32        := ld -m elf_i386

CFLAGS      := -std=c11 -ffreestanding -fno-stack-protector -fno-pie -O2 -Wall -Wextra -Ikernel
ASFLAGS     := -m32 -ffreestanding -fno-pie -O2
LDFLAGS     := -nostdlib

OBJS := \
	$(BUILD)/entry.o \
	$(BUILD)/context_switch.o \
	$(BUILD)/gdt.o \
	$(BUILD)/gdt_flush.o \
	$(BUILD)/idt.o \
	$(BUILD)/isr_c.o \
	$(BUILD)/isr_s.o \
	$(BUILD)/irq_c.o \
	$(BUILD)/irq_s.o \
	$(BUILD)/tss_flush.o \
	$(BUILD)/keyboard.o \
	$(BUILD)/timer.o \
	$(BUILD)/kmalloc.o \
	$(BUILD)/paging.o \
	$(BUILD)/physmem.o \
	$(BUILD)/crypto.o \
	$(BUILD)/fs.o \
	$(BUILD)/task.o \
	$(BUILD)/shell.o \
	$(BUILD)/editor.o \
	$(BUILD)/kernel_main.o \
	$(BUILD)/security.o \
	$(BUILD)/user_mode.o \
	$(BUILD)/user_program.o \
	$(BUILD)/log.o \
	$(BUILD)/debugcon.o
# 	$(BUILD)/map_user_pages.o \



.PHONY: all run iso clean run-gtk run-sdl run-curses

all: iso

# $(BUILD)/map_user_pages.o: kernel/user/map_user_pages.c
# 	@mkdir -p $(BUILD)
# 	$(CC32) $(CFLAGS) -c $< -o $@

$(BUILD)/user_program.o: kernel/user/user_program.c
	@mkdir -p $(BUILD)
	$(CC32) $(CFLAGS) -c $< -o $@

$(BUILD)/user_mode.o: kernel/arch/i386/start/user_mode.S
	@mkdir -p $(BUILD)/kernel/arch/i386/start
	$(CC32) $(ASFLAGS) -c $< -o $@

$(BUILD)/tss_flush.o: kernel/arch/i386/cpu/tss_flush.S
	@mkdir -p $(BUILD)
	$(CC32) $(ASFLAGS) -c $< -o $@

$(BUILD)/security.o: kernel/security.c
	@mkdir -p $(BUILD)
	$(CC32) $(CFLAGS) -c $< -o $@

$(BUILD)/log.o: kernel/log.c
	@mkdir -p $(BUILD)
	$(CC32) $(CFLAGS) -c $< -o $@

$(BUILD)/entry.o: kernel/arch/i386/start/entry.S
	@mkdir -p $(BUILD)
	$(CC32) $(ASFLAGS) -c $< -o $@

$(BUILD)/context_switch.o: kernel/arch/i386/start/context_switch.S
	@mkdir -p $(BUILD)
	$(CC32) $(ASFLAGS) -c $< -o $@


$(BUILD)/debugcon.o: kernel/arch/i386/drivers/debugcon.c
	@mkdir -p $(BUILD)
	$(CC32) $(CFLAGS) -c $< -o $@


$(BUILD)/gdt.o: kernel/arch/i386/cpu/gdt.c
	@mkdir -p $(BUILD)
	$(CC32) $(CFLAGS) -c $< -o $@

$(BUILD)/gdt_flush.o: kernel/arch/i386/cpu/gdt_flush.S
	@mkdir -p $(BUILD)
	$(CC32) $(ASFLAGS) -c $< -o $@

$(BUILD)/idt.o: kernel/arch/i386/cpu/idt.c
	@mkdir -p $(BUILD)
	$(CC32) $(CFLAGS) -c $< -o $@

$(BUILD)/isr_c.o: kernel/arch/i386/cpu/isr.c
	@mkdir -p $(BUILD)
	$(CC32) $(CFLAGS) -c $< -o $@

$(BUILD)/irq_c.o: kernel/arch/i386/cpu/irq.c
	@mkdir -p $(BUILD)
	$(CC32) $(CFLAGS) -c $< -o $@

$(BUILD)/isr_s.o: kernel/arch/i386/cpu/isr.S
	@mkdir -p $(BUILD)
	$(CC32) $(ASFLAGS) -c $< -o $@

$(BUILD)/irq_s.o: kernel/arch/i386/cpu/irq.S
	@mkdir -p $(BUILD)
	$(CC32) $(ASFLAGS) -c $< -o $@

$(BUILD)/keyboard.o: kernel/arch/i386/drivers/keyboard.c
	@mkdir -p $(BUILD)
	$(CC32) $(CFLAGS) -c $< -o $@

$(BUILD)/timer.o: kernel/arch/i386/drivers/timer.c
	@mkdir -p $(BUILD)
	$(CC32) $(CFLAGS) -c $< -o $@

$(BUILD)/kmalloc.o: kernel/arch/i386/mm/kmalloc.c
	@mkdir -p $(BUILD)
	$(CC32) $(CFLAGS) -c $< -o $@

$(BUILD)/paging.o: kernel/arch/i386/mm/paging.c
	@mkdir -p $(BUILD)
	$(CC32) $(CFLAGS) -c $< -o $@

$(BUILD)/physmem.o: kernel/arch/i386/mm/physmem.c
	@mkdir -p $(BUILD)
	$(CC32) $(CFLAGS) -c $< -o $@

$(BUILD)/crypto.o: kernel/fs/crypto.c
	@mkdir -p $(BUILD)
	$(CC32) $(CFLAGS) -c $< -o $@

$(BUILD)/fs.o: kernel/fs/fs.c
	@mkdir -p $(BUILD)
	$(CC32) $(CFLAGS) -c $< -o $@

$(BUILD)/task.o: kernel/sched/task.c
	@mkdir -p $(BUILD)
	$(CC32) $(CFLAGS) -c $< -o $@

$(BUILD)/shell.o: kernel/shell/shell.c
	@mkdir -p $(BUILD)
	$(CC32) $(CFLAGS) -c $< -o $@

$(BUILD)/editor.o: kernel/shell/editor.c
	@mkdir -p $(BUILD)
	$(CC32) $(CFLAGS) -c $< -o $@

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
	mkdir -p logs
	rm -f logs/all.log
	qemu-system-i386 -cdrom $(ISO) -m 256 \
	    -debugcon file:logs/all.log \
	    -no-reboot -no-shutdown


run-gtk: iso
	qemu-system-i386 -cdrom $(ISO) -m 256 -display gtk -no-reboot -no-shutdown

run-sdl: iso
	qemu-system-i386 -cdrom $(ISO) -m 256 -display sdl -no-reboot -no-shutdown

run-curses: iso
	mkdir -p logs
	rm -f logs/all.log
	qemu-system-i386 -cdrom $(ISO) -m 256 -display curses \
	    -debugcon file:logs/all.log \
	    -no-reboot -no-shutdown


clean:
	rm -rf $(BUILD)
