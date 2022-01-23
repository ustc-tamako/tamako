#include "console.h"
#include "debug.h"
#include "clock.h"
#include "printk.h"
#include "idt.h"
#include "gdt.h"
#include "mm.h"
#include "elf.h"


uint32_t kern_stack[PAGE_SIZE<<2];
uint32_t * stack_bottom = &kern_stack[PAGE_SIZE<<2];

int kern_entry()
{
	typedef
	struct mm_test_t {
		uint32_t a, b, c, d, e;
		uint32_t t[1000];
	} mm_test_t;

	console_init();

	setup_idt();
	setup_gdt();

	debug_init();
	clk_init();
	mm_init();

	__asm__ __volatile__ ("sti");

	int * a = (int *)kmalloc(sizeof(int));
	int * b = (int *)kmalloc(sizeof(int));
	int * c = (int *)kmalloc(sizeof(int));
	kfree(a);
	kfree(b);
	kfree(c);

	mm_test_t * t[4096];

	for (int i = 0; i < 4096; i++) {
		t[i] = (mm_test_t *)kmalloc(sizeof(mm_test_t));
	}

	km_print();

	for (int i = 0; i < 2048; i++) {
		kfree(t[i]);
	}

	t[3333]->c = 19;

	for (int i = 0; i < 2048; i++) {
		t[i] = (mm_test_t *)kmalloc(sizeof(mm_test_t));
	}

	for (int i = 0; i < 1024; i++) {
		kfree(t[i]);
	}

	for (int i = 0; i < 512; i++) {
		t[i] = (mm_test_t *)kmalloc(sizeof(mm_test_t));
	}

	t[12]->b = 3;

	for (int i = 1024; i < 2048; i++) {
		kfree(t[i]);
	}

	int * d = (int *)kmalloc(sizeof(int) * 111);
	kfree(d);

	for (int i = 0; i < 512; i++) {
		kfree(t[i]);
	}

	t[3333]->a = 12;

	km_print();

	for (int i = 2048; i < 4096; i++) {
		kfree(t[i]);
	}

	km_print();

	no_bug_please();

	return 0;
}