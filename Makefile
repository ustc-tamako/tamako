#!Makefile

C_SOURCES = $(shell find . -name "*.c")
C_OBJECTS = $(patsubst %.c, %.o, $(C_SOURCES))
S_SOURCES = $(shell find . -name "*.s")
S_OBJECTS = $(patsubst %.s, %.o, $(S_SOURCES))

CC = gcc
LD = ld
ASM = nasm

C_FLAGS = -c -Wall -m32 -ggdb -gstabs+ -nostdinc -fno-pic -fno-builtin -fno-stack-protector -I include
ASM_FLAGS = -f elf -g -F stabs
LD_FLAGS = -T scripts/kernel.ld -m elf_i386 -nostdlib

all: $(S_OBJECTS) $(C_OBJECTS) link update_image

.c.o:
	@echo [  C  ] $<
	$(CC) $(C_FLAGS) $< -o $@

.s.o:
	@echo [ ASM ] $<
	$(ASM) $(ASM_FLAGS) $<

link:
	@echo [ LD  ] 
	$(LD) $(LD_FLAGS) $(S_OBJECTS) $(C_OBJECTS) -o tamako_kernel

.PHONY:clean
clean:
	$(RM) $(S_OBJECTS) $(C_OBJECTS) tamako_kernel

.PHONY:update_image
update_image:
	sudo mount floppy.img /mnt/kernel
	sudo cp tamako_kernel /mnt/kernel/tamako_kernel
	sleep 1
	sudo umount /mnt/kernel

.PHONY:mount_image
mount_image:
	sudo mount floppy.img /mnt/kernel

.PHONY:umount_image
umount_image:
	sudo umount /mnt/kernel

.PHONY:qemu
qemu:
	qemu -fda floppy.img -boot a

.PHONY:bochs
bochs:
	bochs -f tools/.bochsrc

.PHONY:dqemu
dqemu:
	qemu -S -s -fda floppy.img -boot a &

.PHONY:dbochs
dbochs:
	bochs-gdb -f tools/.dbochsrc
	