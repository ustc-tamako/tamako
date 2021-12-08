#include "console.h"
#include "debug.h"
#include "clock.h"
#include "printk.h"
#include "idt.h"
#include "gdt.h"

int kern_entry()
{
    setup_idt();
    setup_gdt();

    debug_init();
    clk_init();

    console_clear();

    print_sreg();

    asm volatile ("int $0x03");
    asm volatile ("int $0x04");

    asm volatile ("sti");

    return 0;
}