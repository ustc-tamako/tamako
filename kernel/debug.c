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

	printk("===============================================================================\n");
	printk("Kernel Panic\n");
	printk("%s\n", msg);

	while (ebp != 0) {
		eip = ebp + 1;
		printk("[0x%08x] %s\n", *eip, elf_lookup_symbol(*eip, &kernel_elf)); // 根据代码地址查找符号表，找到所属的函数名
		ebp = (uint32_t *) *ebp; // 当前函数的 ebp 里存放了上一个函数的 ebp
	}

	printk("===============================================================================\n");

	while(1);
}

void print_sreg()
{
	static int round = 0;
	uint16_t cs, ds, es, ss;

	__asm__ __volatile__ (
		"mov %%cs, %0\n"
		"mov %%ds, %1\n"
		"mov %%es, %2\n"
		"mov %%ss, %3\n"
		: "=m"(cs), "=m"(ds), "=m"(es), "=m"(ss)
	);

	printk("%d:  cs = 0x%04x\n", round, cs);
	printk("%d:  ds = 0x%04x\n", round, ds);
	printk("%d:  es = 0x%04x\n", round, es);
	printk("%d:  ss = 0x%04x\n", round, ss);

	++round;
}

void print_mm_map()
{
	mmap_entry_t * mmap_start_addr = (mmap_entry_t *)glb_mboot_ptr->mmap_addr;
	mmap_entry_t * mmap_end_addr = (mmap_entry_t *)(glb_mboot_ptr->mmap_addr + glb_mboot_ptr->mmap_length);
	
	printk("Memory Map:\n");

	for (mmap_entry_t * mmap = mmap_start_addr; mmap < mmap_end_addr; mmap++) {
		printk("Base_addr = 0x%X%08X, Length = 0x%X%08X, Type = 0x%X\n",
		mmap->base_addr_high, mmap->base_addr_low,
		mmap->length_high, mmap->length_low,
		mmap->type);
	}
}

void no_bug_please()
{
	printk("\n\n%52s\n", "\[0bTamako daisuki! Dozo!");
	while(1);
}