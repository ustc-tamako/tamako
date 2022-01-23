#include "isr.h"
#include "pic.h"
#include "printk.h"

#ifndef IDT_LENGTH
	#define IDT_LENGTH 256
#endif

// 全局中断处理函数表
intr_handler_t intr_handler_table[IDT_LENGTH];

void register_intr_handler(uint8_t idx, intr_handler_t handler)
{
	intr_handler_table[idx] = handler;
}

void isr_handler(pt_regs_t * regs)
{
	uint32_t intr_no = regs->intr_no;
	if (intr_handler_table[intr_no] != NULL) {
		intr_handler_table[intr_no](regs);
	}
	else {
		printk("Unhandled Interrupt: %d\n", intr_no);
	}
}

void irq_handler(pt_regs_t * regs)
{
	uint32_t intr_no = regs->intr_no;

	clear_irq(intr_no);

	if (intr_handler_table[intr_no] != NULL) {
		intr_handler_table[intr_no](regs);
	}
	else {
		printk("Unhandled Interrupt: %d\n", intr_no);
	}
}