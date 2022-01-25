#include "console.h"
#include "uart.h"
#include "debug.h"
#include "clock.h"
#include "idt.h"
#include "gdt.h"
#include "mm.h"
#include "common.h"

uint32_t kern_stack[PAGE_SIZE<<2];
uint32_t * stack_bottom = &kern_stack[PAGE_SIZE<<2];

int kern_entry()
{
	setup_gdt();
	setup_idt();

	debug_init();

	console_init();
	uart_init();

	clk_init();

	mm_init();

	sti();

	char * s = (char *)kmalloc(sizeof(char)*128);
	ugets(s);
	uputs(s);

	no_bug_please();

	return 0;
}