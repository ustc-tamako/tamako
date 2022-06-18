#include "debug.h"
#include "elf.h"
#include "console.h"
#include "printk.h"

static elf_t kernel_elf;

// 声明全局的 multiboot_t * 指针
multiboot_t * glb_mboot_ptr;

void debug_init()
{
	// 从 GRUB 提供的信息中获取到内核符号表和代码地址信息
	kernel_elf = elf_from_multiboot(glb_mboot_ptr);
}

void panic(const char * msg)
{
	uint32_t * ebp;
	uint32_t * eip;

	__asm__ __volatile__ (
		"mov %%ebp, %0"
		: "=r"(ebp)
	);

	error_log("Kernel Panic", msg);

	while (ebp != 0) {
		eip = ebp + 1;
		printk("[0x%08x] %s\n", *eip, elf_lookup_symbol(*eip, &kernel_elf)); // 根据代码地址查找符号表, 找到所属的函数名
		ebp = (uint32_t *) *ebp; // 当前函数的 ebp 里存放了上一个函数的 ebp
	}

	while(1);
}

void info_log(const char * type, const char * msg)
{
	printk("\033[01m[%s Info]\033[0m  %s\n", type, msg);
}

void warning_log(const char * type, const char * msg)
{
	printk("\033[01m\033[34m[%s Warning]\033[0m  %s\n", type, msg);
}

void error_log(const char * type, const char * msg)
{
	printk("\033[01m\033[31m[%s Error]\033[0m  %s\n", type, msg);
}

static void no_bug_please()
{
	printk("\n\n%64s\n", "\033[01m\033[36mTamako daisuki! Dozo!\033[0m");
}

extern void buddy_test();
extern void kmalloc_test();
extern void sched_test();

int test()
{
	kmalloc_test();
	buddy_test();
	sched_test();

	no_bug_please();

	return 0;
}
