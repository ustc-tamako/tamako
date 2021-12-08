#include "isr.h"
#include "pic.h"
#include "printk.h"

#ifndef IDT_LENGTH
    #define IDT_LENGTH 256
#endif

// 全局中断处理函数表
interrupt_handler_t int_handler_table[IDT_LENGTH];

void register_interrupt_handler(uint8_t idx, interrupt_handler_t handler)
{
    int_handler_table[idx] = handler;
}

void isr_handler(pt_regs_t * regs)
{
    uint32_t int_no = regs->int_no;
    if (int_handler_table[int_no] != NULL) {
        int_handler_table[int_no](regs);
    }
    else {
        printk("Unhandled Interrupt: %d\n", int_no);
    }
}

void irq_handler(pt_regs_t * regs)
{
    uint32_t int_no = regs->int_no;

    clear_irq(int_no);

    if (int_handler_table[int_no] != NULL) {
        int_handler_table[int_no](regs);
    }
    else {
        printk("Unhandled Interrupt: %d\n", int_no);
    }
}