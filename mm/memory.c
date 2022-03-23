#include "multiboot.h"
#include "debug.h"
#include "printk.h"
#include "string.h"
#include "mm.h"
#include "isr.h"

// 页框号与位图行号的相互转换
#define idx_to_row(i)   ((i)>>5)
#define row_to_idx(i)   ((i)<<5)
// 空闲页框总数
static uint32_t n_free_frames;
// 管理页框分配的位图 1KB
uint32_t frame_map[idx_to_row(MAX_FRAME_NUM)];
// 管理所有页框的表 40KB
frame_t frame_tab[MAX_FRAME_NUM];
// 计算空闲物理内存大小
#define calc_free_mm()  (n_free_frames << 12)

// 从虚拟地址获取页目录号
#define vaddr_to_dir(v) ((v)>>22)
// 从虚拟地址获取页号
#define vaddr_to_pg(v)  (((v)>>12) & 0x3FF)
// 从虚拟地址获取页偏移量
#define vaddr_to_off(v) ((v)&0xFFF)
// 页是否分配页框
#define pg_mapped(p)    ((p)&PG_PRESENT)

// 内核只读部分的开始地址
extern uint8_t _ro_start[];
// 内核只读部分的结束地址
extern uint8_t _ro_end[];
// 内核保留区域的尾部地址
extern uint8_t _kern_end[];

extern multiboot_t * glb_mboot_ptr;

// 页目录地址，位于物理地址的 0x0 处
#define PG_DIR		to_vaddr(0x0)
// 内核页表区域，位于物理地址的 0x1000 开始处
#define PG_TAB_K	to_vaddr(0x1000)

#define set_frame(idx) \
	do { \
		if (frame_tab[idx].ref_cnt++ == 0) { \
			frame_map[idx_to_row(idx)] |= (0x1 << (idx & 0x1F)); \
			n_free_frames--; \
		} \
	} while(0); \

#define clear_frame(idx) \
	do { \
		if (--frame_tab[idx].ref_cnt == 0) { \
			frame_map[idx_to_row(idx)] &= ~(0x1 << (idx & 0x1F)); \
			n_free_frames++; \
		} \
	} while(0); \

// 获取一个空闲页框，返回其页框号
uint32_t get_free_frame()
{
	// 从未被内核保留的内存区开始搜索
	uint32_t i = idx_to_row(pg_to_frame(to_paddr((uint32_t)_kern_end)) + 1);
	uint32_t wd = 0;
	uint32_t j;
	uint32_t frame = NULL;
	while (i < idx_to_row(MAX_FRAME_NUM)) {
		// 查找位图项中第一个不为 1 的位
		wd = ~frame_map[i] & (frame_map[i]+1); 
		if (wd != 0) {
			j = 0;
			if ((wd & 0xFFFF) == 0) {
				j += 16;
				wd >>= 16;
			}
			if ((wd & 0xFF) == 0) {
				j += 8;
				wd >>= 8;
			}
			if ((wd & 0xF) == 0) {
				j += 4;
				wd >>= 4;
			}
			if ((wd & 0x3) == 0) {
				j += 2;
				wd >>= 2;
			}
			if ((wd & 0x1) == 0) {
				j += 1;
			}
			frame = row_to_idx(i) + j;
			break;
		}
		i++;
	}
	if (frame == NULL) {
		panic("Memory Error: No more free page.");
	}
	return frame;
}

// 分配一个页，并返回其虚拟地址
void * alloc_page()
{
	uint32_t frame = get_free_frame();
	set_frame(frame);
	uint32_t paddr = frame << 12;
	uint32_t vaddr = to_vaddr(paddr);
	// 获取页框所在的页目录项
	page_t * pg_tab = (page_t *)PG_DIR + vaddr_to_dir(vaddr);
	// 页目录项未被映射
	if (!pg_mapped(*pg_tab)) {
		panic("Memory Error: Out of memory! Invalid virtual address.");
	}
	// 获取页框所在的页表首地址
	pg_tab = (page_t *)to_vaddr(*pg_tab & 0xFFFFF000);
	// 如果该页框对应的页表项已经分配，则发出一条警告
	if (pg_mapped(pg_tab[vaddr_to_pg(vaddr)])) {
		printk("Memory Warning: Try to alloc an already present page.");
	}
	// 设置该页表项
	pg_tab[vaddr_to_pg(vaddr)] = paddr | PG_PRESENT | PG_WRITE;
	return (void *)vaddr;
}

// 释放虚拟地址对应的页，并回收其分配的页框
void free_page(void * vaddr)
{
	uint32_t va = (uint32_t)vaddr;
	page_t * pg_tab = (page_t *)PG_DIR + vaddr_to_dir(va);
	pg_tab = (page_t *)to_vaddr(*pg_tab & 0xFFFFF000);
	uint32_t frame = pg_to_frame(pg_tab[vaddr_to_pg(va)]);
	if (!pg_mapped(pg_tab[vaddr_to_pg(va)])) {
		printk("Memory Warning: Try to free a NULL page.");
	}
	pg_tab[vaddr_to_pg(va)] = PG_NULL;
	clear_frame(frame);
}

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
	// bit 1 为 0 表示读错误，为 1 表示写错误
	if (regs->err_code & 0x2) {
		printk("Write error.\n");
	} else {
		printk("Read error.\n");
	}
	// bit 2 为 1 表示在用户模式打断的，为 0 是在内核模式打断的
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
	printk("===============================================================================\n");
	printk("MM INIT\n");
	print_mm_map();

	bzero(frame_map, MAX_FRAME_NUM >> 3);
	n_free_frames = MAX_FRAME_NUM;

	/* 
	 * 内核页表存放在内存的固定区域 0x1000 开始的 8KB 空间
	 * 由于此时还未修改页目录，仍然可以使用在启动代码中设置的前 1MB 线性地址空间
	 */
	page_t * pg_tab = (page_t *)PG_TAB_K;
	uint32_t i;
	// 内核已使用的页框置位
	for (i = 0; i < pg_to_frame(to_paddr((uint32_t)_ro_start)); i++) {
		pg_tab[i] = (i << 12) | PG_PRESENT | PG_WRITE;
		set_frame(i);
	}
	for (; i < pg_to_frame(to_paddr((uint32_t)_ro_end)); i++) {
		pg_tab[i] = (i << 12) | PG_PRESENT;
		set_frame(i);
	}
	for (; i < pg_to_frame(to_paddr((uint32_t)_kern_end)); i++) {
		pg_tab[i] = (i << 12) | PG_PRESENT | PG_WRITE;
		set_frame(i);
	}
	// 剩余部分只进行页表初始化
	for (; i < MAX_FRAME_NUM; i++) {
		pg_tab[i] = PG_NULL;
	}

	// 对由 GRUB 加载入内存的模块部分页表进行初始化
	int mods_cnt = glb_mboot_ptr->mods_count;
	while (mods_cnt--) {
		uint32_t mods_start = *((uint32_t *)glb_mboot_ptr->mods_addr);
		uint32_t mods_end = *(uint32_t *)(glb_mboot_ptr->mods_addr+4);
		while (mods_start < mods_end) {
			i = pg_to_frame(mods_start);
			pg_tab[i] = (i << 12) | PG_PRESENT;
			set_frame(i);
			mods_start += 0x1000;
		}
	}

	// 设置页目录表
	pg_tab = (page_t *)PG_DIR;
	// 内核页目录项的开始索引 0x300
	uint32_t pgk_idx = vaddr_to_dir(PAGE_OFFSET); 
	// 3G 以下线性空间
	for (i = 0; i < pgk_idx; i++) {
		pg_tab[i] = PG_NULL;
	}
	// 0xC0000000 开始的 32MB
	for (int k = 0; k < MAX_FRAME_NUM>>10/* 8 */; k++, i++) {
		pg_tab[i] = to_paddr(PG_TAB_K + (k<<12)) | PG_PRESENT | PG_WRITE;
	}
	// 剩余未进行映射的空间
	for (; i < 0x400/* FRAME_SIZE / 4 */; i++) {
		pg_tab[i] = PG_NULL;
	}

	register_intr_handler(14, &page_fault);

	printk("Free Memory: 0x%08X Bytes\n", calc_free_mm());

	printk("===============================================================================\n");
}