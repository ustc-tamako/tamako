#ifndef INCLUDE_MM_H_
#define INCLUDE_MM_H_

#include "types.h"

// 内存页(框)大小 4KB
#define FRAME_SIZE      0x1000
#define PAGE_SIZE       0x1000
// 最大物理内存大小 128MB
#define MAX_MM_SIZE     0x08000000
// 最多支持的页框个数 32K
#define MAX_FRAME_NUM   (MAX_MM_SIZE / FRAME_SIZE)
// 当页未分配页框时页表项的默认值
#define PG_NULL         0xFFFFF000

typedef
struct frame_t {
	uint8_t 	ref_cnt;
	uint32_t  * bkt_desc;
} frame_t;

/*
 *  page_t
 *
 *  *-----------------*----------------*-------*-------*---------*
 *  |    31  -  12    |    11  -  3    |   2   |   1   |    0    |
 *  |       addr      |    reserved    |  user | write | present |
 *  *-----------------*----------------*-------*-------*---------*
 * 
 */
typedef unsigned int page_t;

// 页表项 Flag
#define PG_PRESENT      0x1
#define PG_WRITE        0x2
#define PG_USER         0x4

// 虚拟地址偏移量 3GB
#define PAGE_OFFSET     0xC0000000
// 页表项和页框号的相互转换
#define pg_to_frame(p)  ((p)>>12)
#define frame_to_pg(f)  ((f)<<12)
// 直接映射区域内的物理地址与虚拟地址转换
#define to_vaddr(pa)    ((pa) + PAGE_OFFSET)
#define to_paddr(va)    ((va) - PAGE_OFFSET)

void mm_init();

void * alloc_page();

void free_page(void * vaddr);

void * kmalloc(size_t len);

void kfree(void * vaddr);

void km_print();

#endif  // INCLUDE_MM_H_
