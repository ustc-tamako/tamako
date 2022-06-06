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

#include "printk.h"

extern void buddy_main();
extern void km_main();

int flag = 0;

int thread()
{
	int cnt = 0;
	while (1) {
		if (flag & 1) {
			printk("B\n");
			flag = 0;
			cnt++;
		}
		if (cnt == 10) {
			break;
		}
	}
	return 0;
}

int kern_entry()
{
	setup_gdt();
	setup_idt();

	debug_init();
	uart_init();
	clk_init();
	mm_init();
	sched_init();

	kernel_thread(thread, NULL);

	sti();

	int cnt = 0;
	while (1) {
		if (!(flag & 1)) {
			printk("A");
			flag = 1;
			cnt++;
		}
		if (cnt == 10) {
			break;
		}
	}

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
