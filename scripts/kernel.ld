/*
 *      kernel.ld -- 针对 kernel 格式所写的链接脚本
 */

PAGE_OFFSET = 0xC0000000;

ENTRY(start)

SECTIONS
{
	/* 段起始位置 */

	. = 0x100000;

	_kern_start = . ;

	.init :
	{
		*(.init)
		. = ALIGN(4096);
	}

	/* . += PAGE_OFFSET; */
	_ro_start = . ;

	.text :
	{
		*(.text)
		. = ALIGN(4096);
	}

	.rodata :
	{
		*(.rodata)
		. = ALIGN(4096);
	}

	_ro_end = . ;

	.data :
	{
		*(.data)
		. = ALIGN(4096);
	}

	.bss :
	{
		*(.bss)
		. = ALIGN(4096);
	}

	.stab :
	{
		*(.stab)
		. = ALIGN(4096);
	}

	.stabstr :
	{
		*(.stabstr)
		. = ALIGN(4096);
	}

	_kern_end = . ;

	/DISCARD/ : { *(.comment) *(.eh_frame) }
}