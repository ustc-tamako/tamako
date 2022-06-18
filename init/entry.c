#include "console.h"
#include "uart.h"
#include "debug.h"
#include "clock.h"
#include "idt.h"
#include "gdt.h"
#include "mm.h"
#include "fs.h"
#include "sched.h"
#include "common.h"

int kern_entry()
{
	setup_gdt();
	setup_idt();

	debug_init();
	uart_init();
	clk_init();
	mm_init();
	sched_init();

	sti();

	kernel_thread(init, NULL, 0, "init");

	while (1) {
		__asm__ __volatile__ ("pause\n");
	}

	return 0;
}
