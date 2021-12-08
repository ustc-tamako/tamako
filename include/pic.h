#ifndef INCLUDE_PIC_H_
#define INCLUDE_PIC_H_

#include "types.h"

// 初始化 PIC
void pic_init();

// 清除中断标志
void clear_irq(uint32_t intr_no);

#endif  // INCLUDE_PIC_H_