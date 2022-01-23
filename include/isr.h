#ifndef INCLUDE_ISR_H_
#define INCLUDE_ISR_H_

#include "types.h"

// 中断处理函数指针类型
typedef void (* intr_handler_t)(pt_regs_t *);

// 注册一个中断处理函数
void register_intr_handler(uint8_t idx, intr_handler_t handler);

// 异常处理函数
void isr_handler(pt_regs_t * regs);

// IRQ 处理函数
void irq_handler(pt_regs_t * regs);

#endif  // INCLUDE_ISR_H_