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
	@echo "\033[1m\033[32m   CC   \033[0m \033[1m$<\033[0m"
	@$(CC) $(C_FLAGS) $< -o $@

.s.o:
	@echo "\033[1m\033[32m   AS   \033[0m \033[1m$<\033[0m"
	@$(ASM) $(ASM_FLAGS) $<

link:
	@echo "\033[1m\033[32m   LD   \033[0m \033[1m*.o => tamago\033[0m"
	@$(LD) $(LD_FLAGS) $(S_OBJECTS) $(C_OBJECTS) -o tamago

.PHONY:clean
clean:
	@echo "\033[1m\033[32m   RM   \033[0m \033[1mall\033[0m"
	@$(RM) $(S_OBJECTS) $(C_OBJECTS) tamago

.PHONY:install
install:
	sudo mount tamako.img /mnt/kernel
	sudo cp tamago /mnt/kernel/tamako_kernel
	sleep 1
	sudo umount /mnt/kernel

.PHONY:mac_install
mac_install:
	@cp tamago /Volumes/TAMAKO
	@echo "\033[1m\033[32m        \033[0m \033[1mCopy Done\033[0m"