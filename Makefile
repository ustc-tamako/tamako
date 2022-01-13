#!Makefile

C_SOURCES = $(shell find . -name "*.c")
C_OBJECTS = $(patsubst %.c, %.o, $(C_SOURCES))
S_SOURCES = $(shell find . -name "*.s")
S_OBJECTS = $(patsubst %.s, %.o, $(S_SOURCES))

ARCH			?= x86_64
CROSS_COMPILE	?=

CC = $(CROSS_COMPILE)gcc
LD = $(CROSS_COMPILE)ld
ASM = nasm

C_FLAGS = -c -Wall -m32 -ggdb -gstabs+ -nostdinc -fno-pic -fno-builtin -fno-stack-protector -I include
ASM_FLAGS = -f elf -g -F stabs
LD_FLAGS = -T scripts/kernel.ld -m elf_i386 -nostdlib

all: $(S_OBJECTS) $(C_OBJECTS) link

.c.o:
	@echo [  CC  ] $<
	$(CC) $(C_FLAGS) $< -o $@

.s.o:
	@echo [  AS  ] $<
	$(ASM) $(ASM_FLAGS) $<

link:
	@echo [  LD  ] 
	$(LD) $(LD_FLAGS) $(S_OBJECTS) $(C_OBJECTS) -o tamago

.PHONY:clean
clean:
	$(RM) $(S_OBJECTS) $(C_OBJECTS) tamago

.PHONY:install
install:
	sudo mount tamako.img /mnt/kernel
	sudo cp tamago /mnt/kernel/tamako_kernel
	sleep 1
	sudo umount /mnt/kernel

.PHONY:mac_install
mac_install:
	sudo cp tamago /Volumes/TAMAKO
	