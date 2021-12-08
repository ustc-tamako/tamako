#include "clock.h"
#include "isr.h"
#include "common.h"
#include "printk.h"

#define FREQ_IN     1193180
#define FREQ_OUT    100

// 8253/8254 PIT 寄存器
#define PIT_TIMER   0x40
#define PIT_MODE    0x43

void clk_intr(pt_regs_t * regs)
{
    static uint32_t tick = 0;
    printk("Tick: %d\n", tick++);
}

void clk_init()
{
    register_intr_handler(32, clk_intr);
    uint32_t divisor = FREQ_IN / FREQ_OUT;

    // 设置 8253/8254 芯片工作模式
    outb(PIT_MODE, 0x36);

    uint8_t low = (uint8_t)(divisor & 0xFF);
    uint8_t high = (uint8_t)((divisor >> 8) & 0xFF);

    // 设置分频
    outb(PIT_TIMER, low);
    outb(PIT_TIMER, high);
}