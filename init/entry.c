#include "console.h"
#include "debug.h"
#include "printk.h"
#include "idt.h"
#include "gdt.h"

int kern_entry()
{
    setup_idt();
    setup_gdt();

    debug_init();

    console_clear();

    print_cur_status();

    asm volatile ("int $0x3");
    asm volatile ("int $0x4");

    return 0;
}