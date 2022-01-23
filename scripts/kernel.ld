/*
 *      kernel.ld -- 针对 kernel 格式所写的链接脚本
 */

PAGE_OFFSET = 0xC0000000;

ENTRY(start)
SECTIONS
{
	/* 段起始位置 */

	. = 0x100000;

	.init :
	{
		*(.init)
		. = ALIGN(4096);
	}

	. += PAGE_OFFSET;

	.text : AT( ADDR(.text) - PAGE_OFFSET )
	{
		*(.text)
		. = ALIGN(4096);
	}

	.rodata : AT ( ADDR(.rodata) - PAGE_OFFSET )
	{
		*(.rodata)
		. = ALIGN(4096);
	}

	.data : AT ( ADDR(.data) - PAGE_OFFSET )
	{
		*(.data)
		. = ALIGN(4096);
	}

	.bss : AT ( ADDR(.bss) - PAGE_OFFSET )
	{
		*(.bss)
		. = ALIGN(4096);
	}

	.stab : AT( ADDR(.stab) - PAGE_OFFSET )
	{
		*(.stab)
		. = ALIGN(4096);
	}

	.stabstr : AT( ADDR(.stabstr) - PAGE_OFFSET )
	{
		*(.stabstr)
		. = ALIGN(4096);
	}

	_kern_end = .;

	/DISCARD/ : { *(.comment) *(.eh_frame) }
}