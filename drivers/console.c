#include "common.h"
#include "console.h"

#define CON_ROWS 25
#define CON_COLS 80

static uint16_t * video_memory = (uint16_t *)0xC00B8000;

static uint8_t cursor_x = 0;
static uint8_t cursor_y = 0;

#define xtoi(c) ({ \
	int _i = 0; \
	if ('0' <= c && c <= '9') _i = c - '0'; \
	if ('a' <= c && c <= 'f') _i = c - 'a' + 10; \
	if ('A' <= c && c <= 'F') _i = c - 'A' + 10; \
	_i; \
})

static void move_cursor()
{
	uint16_t cursor_location = cursor_y * CON_COLS + cursor_x;

	// 在这里用到的两个内部寄存器的编号为14与15，分别表示光标位置
	// 的高8位与低8位。

	outb(0x3D4, 14);                    
	outb(0x3D5, cursor_location >> 8);
	outb(0x3D4, 15);
	outb(0x3D5, cursor_location);
}

static void scroll()
{
	uint8_t attribute_byte = (0 /*black*/ << 4) | (15 /*white*/ & 0x0F);
	uint16_t blank = 0x20 /*space*/ | (attribute_byte << 8);

	if (cursor_y >= CON_ROWS) {
		int i;

		for (i = 0 * CON_COLS; i < (CON_ROWS-1) * CON_COLS; i++) {
			video_memory[i] = video_memory[i + CON_COLS];
		}

		for (; i < CON_ROWS * CON_COLS; i++) {
			video_memory[i] = blank;
		}

		cursor_y = CON_ROWS-1;
	}
}

void console_init()
{
	console_clear();
}

void console_clear()
{
	uint8_t attribute_byte = (0 /*black*/ << 4) | (15 /*white*/ & 0x0F);
	uint16_t blank = 0x20 /*space*/ | (attribute_byte << 8);

	 for (int i = 0; i < CON_ROWS * CON_COLS; i++) {
		 video_memory[i] = blank;
	 }

	 cursor_x = 0;
	 cursor_y = 0;
	 move_cursor();
}

void console_putc_color(char c, real_color_t back, real_color_t fore)
{
	uint8_t back_color = (uint8_t)back;
	uint8_t fore_color = (uint8_t)fore;

	uint8_t attribute_byte = (back_color << 4) | (fore_color & 0x0F);
	uint16_t attribute = attribute_byte << 8;

	if (c == 0x08 /*退格*/ && cursor_x) {
		cursor_x--;
	} 
	else if (c == 0x09 /*tab*/) {
		cursor_x = (cursor_x+4) & ~(4-1);
	}
	else if (c == '\r') {
		cursor_x = 0;
	}
	else if (c == '\n') {
		cursor_x = 0;
		cursor_y++;
	} 
	else if (c >= ' ') {
		video_memory[cursor_y * CON_COLS + cursor_x] = c | attribute;
		cursor_x++;
	}

	if (cursor_x >= CON_COLS) {
		cursor_x = 0;
		cursor_y++;
	}

	scroll();

	move_cursor();
}

void console_write(char * cstr)
{
	real_color_t back = rc_black;
	real_color_t fore = rc_white;
	while (*cstr != '\0') {
		if (*cstr == '\[' && *(cstr+1) != '\0' && *(cstr+2) != '\0') {
			back = xtoi(*(cstr+1));
			fore = xtoi(*(cstr+2));
			cstr += 3;
		}
		console_putc_color(*cstr++, back, fore);
	}
}