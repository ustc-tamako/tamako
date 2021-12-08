#ifndef INCLUDE_IDT_H_
#define INCLUDE_IDT_H_

#include "types.h"

// 64 位中断描述符类型
typedef
struct intr_desc_t {
    uint16_t base_low;      // 中断处理函数地址  15 - 0
    uint16_t sel;           // 段选择子
    uint8_t  always0;        // 赋值为0的部分
    uint8_t  flags;          // 标志位
    uint16_t base_high;     // 中断处理函数地址  31 - 16
} __attribute__((packed)) intr_desc_t;

// IDTR
typedef
struct idtr_t {
    uint16_t limit;     // 限长
    uint32_t base;      // 基址
} __attribute__((packed)) idtr_t;

// 设置中断描述符表
void setup_idt();

// 0 - 19
void isr0();        // 0 #DE 除 0 异常 
void isr1();        // 1 #DB 调试异常 
void isr2();        // 2 NMI 
void isr3();        // 3 BP 断点异常 
void isr4();        // 4 #OF 溢出 
void isr5();        // 5 #BR 对数组的引用超出边界 
void isr6();        // 6 #UD 无效或未定义的操作码 
void isr7();        // 7 #NM 设备不可用(无数学协处理器) 
void isr8();        // 8 #DF 双重故障(有错误代码) 
void isr9();        // 9 协处理器跨段操作 
void isr10();       // 10 #TS 无效TSS(有错误代码) 
void isr11();       // 11 #NP 段不存在(有错误代码) 
void isr12();       // 12 #SS 栈错误(有错误代码) 
void isr13();       // 13 #GP 常规保护(有错误代码) 
void isr14();       // 14 #PF 页故障(有错误代码) 
void isr15();       // 15 CPU 保留 
void isr16();       // 16 #MF 浮点处理单元错误 
void isr17();       // 17 #AC 对齐检查 
void isr18();       // 18 #MC 机器检查 
void isr19();       // 19 #XM SIMD(单指令多数据)浮点异常

// 20 - 31
void isr20();
void isr21();
void isr22();
void isr23();
void isr24();
void isr25();
void isr26();
void isr27();
void isr28();
void isr29();
void isr30();
void isr31();

// 32 - 47 
void irq0();        // 32 系统计时器
void irq1();        // 33 键盘
void irq2();        // 34 与 IRQ9 相接，MPU-401 MD 使用
void irq3();        // 35 串口设备
void irq4();        // 36 串口设备
void irq5();        // 37 建议声卡使用
void irq6();        // 38 软驱传输控制使用
void irq7();        // 39 打印机传输控制使用
void irq8();        // 40 即时时钟
void irq9();        // 41 与 IRQ2 相接，可设定给其他硬件
void irq10();       // 42 建议网卡使用
void irq11();       // 43 建议 AGP 显卡使用
void irq12();       // 44 接 PS/2 鼠标，也可设定给其他硬件
void irq13();       // 45 协处理器使用
void irq14();       // 46 IDE0 传输控制使用
void irq15();       // 47 IDE1 传输控制使用

#endif  // INCLUDE_IDT_H_