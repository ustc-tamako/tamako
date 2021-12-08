#include "pic.h"
#include "common.h"

// Master - command: 0x20, data: 0x21
//  Slave - command: 0xA0, data: 0xA1
#define PIC1_CMD    (0x20)
#define PIC2_CMD    (0xA0)
#define PIC1_DATA   (0x21)
#define PIC2_DATA   (0xA1)

void pic_init()
{
    // ICW1 通知 8259A 初始化开始
    outb(PIC1_CMD, 0x11);
    outb(PIC2_CMD, 0x11);

    // ICW2 设置中断向量号的前五位，后三位由芯片自己产生
    outb(PIC1_DATA, 0x20);
    outb(PIC2_DATA, 0x28);

    // ICW3 级联设置
    outb(PIC1_DATA, 0x04);   // 第二位为 1，表示级联在主芯片的 IRQ2 
    outb(PIC2_DATA, 0x02);   // 前五位默认为 0，后三位表示级联在主芯片的 IRQ2

    // ICW4 设置 8086 的默认工作模式
    outb(PIC1_DATA, 0x01);
    outb(PIC2_DATA, 0x01);

    // OCW1 允许中断
    outb(PIC1_DATA, 0x00);
    outb(PIC2_DATA, 0x00);
}

void clear_irq(uint32_t intr_no) {
    // OCW2 设置中断结束
    if (intr_no >= 40) {
        outb(PIC2_CMD, 0x20);
    }
    outb(PIC1_CMD, 0x20);
}