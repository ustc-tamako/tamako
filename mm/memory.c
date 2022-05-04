#include "multiboot.h"
#include "debug.h"
#include "printk.h"
#include "string.h"
#include "mm.h"
#include "isr.h"

// 管理所有页框的表
frame_t frame_tab[MAX_FRAME_NUM];

// 页目录地址, 位于物理地址的 0x0 处
#define PG_DIR		to_vaddr(0x0)
// 内核页表区域, 位于物理地址的 0x1000 开始处
#define PG_TAB_K	to_vaddr(0x1000)

// 内核静态区域的结束地址
extern uint8_t _kern_end[];
// 内核只读部分的开始地址
extern uint8_t _ro_start[];
// 内核只读部分的结束地址
extern uint8_t _ro_end[];

extern multiboot_t * glb_mboot_ptr;

void page_fault(pt_regs_t * regs)
{
	uint32_t cr2;
	__asm__ __volatile__ ("mov %%cr2, %0" : "=r" (cr2));

	printk("Page fault at 0x%x, virtual faulting address 0x%x\n", regs->eip, cr2);
	printk("Error code: %x\n", regs->err_code);

	// bit 0 为 0 指页面不存在内存里
	if (!(regs->err_code & 0x1)) {
		printk("Page wasn't present.\n");
	}
	// bit 1 为 0 表示读错误, 为 1 表示写错误
	if (regs->err_code & 0x2) {
		printk("Write error.\n");
	} else {
		printk("Read error.\n");
	}
	// bit 2 为 1 表示在用户模式打断的, 为 0 是在内核模式打断的
	if (regs->err_code & 0x4) {
		printk("In user mode.\n");
	} else {
		printk("In kernel mode.\n");
	}
	// bit 3 为 1 表示错误是由保留位覆盖造成的
	if (regs->err_code & 0x8) {
		printk("Reserved bits being overwritten.\n");
	}
	// bit 4 为 1 表示错误发生在取指令的时候
	if (regs->err_code & 0x10) {
		printk("The fault occurred during an instruction fetch.\n");
	}

	while (1);
}

void mm_init()
{
	uint32_t kern_start_pa = 1<<20;
	uint32_t kern_end_pa = to_paddr((uint32_t)_kern_end);
	uint32_t ro_start_pa = to_paddr((uint32_t)_ro_start);
	uint32_t ro_end_pa = to_paddr((uint32_t)_ro_end);

	/* 
	 * 内核页表存放在内存的固定区域 0x1000 开始的 32KB 空间
	 * 由于此时还未修改页目录, 仍然可以使用在启动代码中设置的前 4MB 线性地址空间
	 */
	uint32_t * pg_tab = (uint32_t *)PG_TAB_K;
	uint32_t i;
	// 内核已使用的页框置位
	for (i = 0; i < to_fr_idx(ro_start_pa); i++) {
		pg_tab[i] = (i << 12) | PG_PRESENT | PG_WRITE;
	}
	for (; i < to_fr_idx(ro_end_pa); i++) {
		pg_tab[i] = (i << 12) | PG_PRESENT;
	}
	for (; i < MAX_FRAME_NUM; i++) {
		pg_tab[i] = (i << 12) | PG_PRESENT | PG_WRITE;
	}

	// 设置页目录表
	pg_tab = (uint32_t *)PG_DIR;
	// 内核页目录项的开始索引 0x300
	i = vaddr_to_dir(PAGE_OFFSET);
	// 0xC0000000 开始的 32MB
	for (int k = 0; k < MAX_FRAME_NUM>>10/* 8 */; k++, i++) {
		pg_tab[i] = to_paddr(PG_TAB_K + (k<<12)) | PG_PRESENT | PG_WRITE;
	}

	for (int i = 0; i < MAX_FRAME_NUM; i++) {
		frame_tab[i].ref_cnt = 0;	// 暂不使用
		frame_tab[i].flag = 0xF;
		frame_tab[i].bkt_desc = NULL;
		list_node_init(&frame_tab[i].chain);
	}

	mmap_entry_t * mmap_start_addr = (mmap_entry_t *)glb_mboot_ptr->mmap_addr;
	mmap_entry_t * mmap_end_addr = (mmap_entry_t *)(glb_mboot_ptr->mmap_addr + glb_mboot_ptr->mmap_length);
	
	info_log("Memory Map", "");

	for (mmap_entry_t * mmap = mmap_start_addr; mmap < mmap_end_addr; mmap++) {
		printk("Base_addr:0x%08X\tLength:0x%08X\tType = 0x%X\n", mmap->base_addr_low, mmap->length_low, mmap->type);
		if (mmap->base_addr_low == kern_start_pa) {
			int n = mmap->length_low - (kern_end_pa - kern_start_pa);
			mm_ops->init((void *)kern_end_pa, n>>12);
		}
	}

	register_intr_handler(14, &page_fault);
}