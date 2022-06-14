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

extern void buddy_main();
extern void km_main();

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

	km_main();
	buddy_main();

	init();

	char * s = (char *)kmalloc(sizeof(char)*128);
	ugets(s);
	uputs(s);
	kfree(s);

	no_bug_please();

	while (1);

	return 0;
}
