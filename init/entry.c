#include "console.h"
#include "uart.h"
#include "debug.h"
#include "clock.h"
#include "idt.h"
#include "gdt.h"
#include "mm.h"
#include "fs.h"
#include "common.h"

uint32_t kern_stack[PAGE_SIZE<<1];
uint32_t * stack_bottom = &kern_stack[PAGE_SIZE<<1];

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

	sti();

	km_main();
	buddy_main();

	char * s = (char *)kmalloc(sizeof(char)*128);
	ugets(s);
	uputs(s);
	kfree(s);

	no_bug_please();

	while (1);

	return 0;
}