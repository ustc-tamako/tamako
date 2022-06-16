#ifndef INCLUDE_MM_H_
#define INCLUDE_MM_H_

#include "types.h"
#include "list.h"
#include "spinlock.h"

// 内存页(框)大小 4KB
#define FRAME_SIZE		0x1000
#define PAGE_SIZE		0x1000
// 最大物理内存大小 32MB
#define MAX_MM_SIZE		0x02000000
// 最多支持的页框个数 8K
#define MAX_FRAME_NUM	(MAX_MM_SIZE / FRAME_SIZE)
// 最多支持的页个数
#define MAX_PAGE_NUM	(MAX_VM_SIZE / PAGE_SIZE)
// 当页未分配页框时页表项的默认值
#define PG_NULL			0

/**
 * @brief 用于物理页框管理的结构体.
 */
typedef
struct frame_t {
	uint16_t   ref_cnt;		// 页框的引用计数
	/**
	 * 页框的标志位集合
	 * 3-0:	伙伴系统中使用的 order 值, 范围在 0-10 之间, 值为 15 表示无效
	 *   4: 标记该页框是否被用于伙伴系统
	 *   5:	标记该页框是否空闲
	 */
	uint16_t   flag;
	void     * bkt_desc;	// kmalloc 存储桶中使用, 指向桶相应的描述符地址
	list_node  chain;		// buddy 链表中使用
	spinlock_t lock;		// 互斥锁
} frame_t;

// 管理所有物理页框的数组
extern frame_t frame_tab[MAX_FRAME_NUM];

// 获取 frame_t 对象在 frame_tab 中的索引
#define fr_index(fr)	((fr) - frame_tab)
// 从 frame_t 对象获取其对应的页框首地址
#define fr_paddr(fr)	(fr_index(fr) << 12)
// 物理地址到页框的转换
#define to_fr_idx(pa)	((uint32_t)(pa) >> 12)
#define to_fr(pa)		(frame_tab + to_fr_idx(pa))

/**
 *  @brief page_t
 *
 *  *-----------------*----------------*-------*-------*---------*
 *  |    31  -  12    |    11  -  3    |   2   |   1   |    0    |
 *  |       addr      |    reserved    |  user | write | present |
 *  *-----------------*----------------*-------*-------*---------*
 *
 */
typedef unsigned int page_t;

// 页表项 Flag
#define PG_PRESENT		0x1
#define PG_WRITE		0x2
#define PG_USER			0x4
// 页是否分配页框
#define pg_mapped(pa)	((pa)&PG_PRESENT)

// 虚拟地址偏移量 3GB
#define PAGE_OFFSET		0xC0000000
// 直接映射区域内的物理地址与虚拟地址转换
#define to_vaddr(pa)	((uint32_t)(pa) + PAGE_OFFSET)
#define to_paddr(va)	((uint32_t)(va) - PAGE_OFFSET)

// 从虚拟地址获取页目录号
#define vaddr_to_dir(va)	((va)>>22)
// 从虚拟地址获取页号
#define vaddr_to_pg(va)		(((va)>>12) & 0x3FF)
// 从虚拟地址获取页偏移量
#define vaddr_to_off(va)	((va)&0xFFF)

/**
 * @brief 页框管理算法的操作结构体, 定义了必要的几种页框操作.
 */
typedef
struct mm_operations {
	int 	(* init)(void * addr, size_t n); // 页框管理初始化
	void *	(* alloc_frames)(size_t n); // 分配 n 个连续页框
	int		(* free_frames)(void * addr, size_t n); // 回收指定地址开始的 n 个连续页框
} mm_operations;

// 伙伴系统的页框管理操作
extern mm_operations const buddy_operations;

#define mm_ops	(&buddy_operations)

// 分配和回收页框/页的宏
#define alloc_frames(n)		mm_ops->alloc_frames(n)
#define alloc_frame()		alloc_frames(1)
#define alloc_pages(n)		(void *)to_vaddr(alloc_frames(n))
#define alloc_page()		alloc_pages(1)
#define free_frames(pa, n)	mm_ops->free_frames(pa, n)
#define free_frame(pa)		free_frames(pa, 1)
#define free_pages(va, n)	free_frames((void *)to_paddr(va), n)
#define free_page(va)		free_pages(va, 1)

void mm_init();

void * kmalloc(size_t len);

void kfree(void * vaddr);

void free_cache();

// 打印物理页框的信息
void fr_print(frame_t * fr);

#endif  // INCLUDE_MM_H_
