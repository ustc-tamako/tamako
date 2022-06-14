#ifndef INCLUDE_TYPES_H_
#define INCLUDE_TYPES_H_

#ifndef NULL
	#define NULL 0
#endif

#ifndef TRUE
	#define TRUE  1
	#define FALSE 0
#endif

/*
 * =============================================================
 *
 *                         32bits - System
 *
 *               char :  1B                 short :  2B
 *      unsigned char :  1B        unsigned short :  2B
 *             char * :  4B                  long :  4B
 *                int :  4B         unsigned long :  4B
 *       unsigned int :  4B             long long :  8B
 *
 * =============================================================
 */

typedef unsigned int   uint32_t;
typedef          int   int32_t;
typedef unsigned short uint16_t;
typedef          short int16_t;
typedef unsigned char  uint8_t;
typedef          char  int8_t;

typedef unsigned int   size_t;

typedef
struct pt_regs_t {
	// 中断处理函数汇编头中压入
	uint32_t ds;		// 用户数据段描述符
	uint32_t edi;
	uint32_t esi;
	uint32_t ebp;
	uint32_t esp;
	uint32_t ebx;
	uint32_t edx;
	uint32_t ecx;
	uint32_t eax;

	uint32_t intr_no;	// 中断号
	uint32_t err_code;	// 错误代码

	// 处理器自动压入
	uint32_t eip;
	uint32_t cs;
	uint32_t eflags;
	uint32_t usr_esp;
	uint32_t ss;        // 堆栈段寄存器
} pt_regs_t;

#endif  // INCLUDE_TYPES_H_
