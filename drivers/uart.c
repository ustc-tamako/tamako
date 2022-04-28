#include "types.h"
#include "common.h"
#include "uart.h"
#include "isr.h"

#define PORT		0x03F8
#define BUF_SIZE	1024

typedef
struct uart_t {
	uint16_t	base;
	uint8_t		id;
	uint8_t		tx_on;
	uint16_t	ilen;
	uint16_t	ihead;
	uint16_t	itail;
	uint16_t	olen;
	uint16_t	ohead;
	uint16_t	otail;
	char		ibuf[BUF_SIZE];
	char		obuf[BUF_SIZE];
} uart_t;

static uart_t uart;

void uputc(char c)
{
	if (uart.tx_on == 1) {
		uart.obuf[uart.otail++] = c;
		uart.otail %= BUF_SIZE;
		uart.olen++;
	}
	else {
		uart.tx_on = 1;
		outb(uart.base+1, inb(uart.base+1)|0x02);
		outb(uart.base, c);
	}
}

void uputs(char * str)
{
	while (*str != '\0') {
		uputc(*str++);
	}
}

char ugetc()
{
	while (uart.ilen <= 0);
	char c;
	c = uart.ibuf[uart.ihead++];
	uart.ihead %= BUF_SIZE;
	uart.ilen--;
	return c;
}

void ugets(char * str)
{
	outb(uart.base+1, inb(uart.base+1)|0x01);
	char * p = str;
	while ((*p = ugetc()) != '\r') {
		if (p > str && *p == '\177') {
			p--;
		}
		else {
			p++;
		}
	}
	*p = '\0';
	outb(uart.base+1, inb(uart.base+1)&0xFE);
}

void uart_intr(pt_regs_t * regs)
{
	// 获取中断标识寄存器的值
	uint8_t iir = inb(uart.base+2);
	// 一个字节发送完成
	if ((iir & 0x06) == 0x02) {
		if (uart.olen <= 0) {
			// 发送队列为空，发送结束
			uart.tx_on = 0;
			outb(uart.base+1, inb(uart.base+1)&0xFD);
		}
		else {
			// 发送队列不为空，则取队首字符发送
			char c = uart.obuf[uart.ohead++];
			uart.ohead %= BUF_SIZE;
			uart.olen--;
			outb(uart.base, c);
		}
	}
	// 接收到数据
	if ((iir & 0x06) == 0x04) {
		char c = inb(uart.base);
		uart.ibuf[uart.itail++] = c;
		uart.itail %= BUF_SIZE;
		uart.ilen++;
		switch (c) {
		case '\r':
			uputc('\n');
			break;
		case '\177':
			uputc('\b');
			uputc(' ');
			uputc('\b');
			break;
		default:
			uputc(c);
			break;
		}
	}
}

void uart_init()
{
	uart.base = PORT;
	uart.id = 1;
	uart.tx_on = 0;
	uart.ilen = 0;
	uart.ihead = 0;
	uart.itail = 0;
	uart.olen = 0;
	uart.ohead = 0;
	uart.otail = 0;

	// 清零
	outb(uart.base+1, 0x00);
	// DLAB = 1
	outb(uart.base+3, 0x80);
	// 设置波特率 2400bps
	outb(uart.base, 0x30);
	outb(uart.base+1, 0x00);

	// DLAB = 0, 设置数据位长度
	outb(uart.base+3, 0x03);
	// 中断使能
	outb(uart.base+1, 0x06);
	// 当前无中断
	outb(uart.base+2, 0x01);
	
	register_intr_handler(36, uart_intr);
}