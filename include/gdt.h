#ifndef INCLUDE_GDT_H_
#define INCLUDE_GDT_H_

#include "types.h"

// 64 位全局段描述符类型
typedef
struct glb_desc_t {
	uint16_t limit_low;      // 段限长  15 - 0
	uint16_t base_low;       // 段基址  15 - 0
	uint8_t  base_middle;    // 段基址  23 - 16
	uint8_t  access;         // 段存在位、描述符特权级、描述符类型、描述符子类别
	uint8_t  granularity;    // 其他标志、段限长  19 - 16
	uint8_t  base_high;      // 段基址  31 - 24
} __attribute__((packed)) glb_desc_t;

// GDTR
typedef
struct gdtr_t {
	uint16_t limit;     // 限长
	uint32_t base;      // 基址
} __attribute__((packed)) gdtr_t;

// 设置全局描述符表
void setup_gdt();

#endif  // INCLUDE_GDT_H_