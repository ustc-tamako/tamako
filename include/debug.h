#ifndef INCLUDE_DEBUG_H_
#define INCLUDE_DEBUG_H_

// 初始化Debug 信息
void debug_init();

// 打印当前函数调用栈信息
void panic(const char * msg);

// 打印当前的段寄存器值
void print_sreg();

// 打印物理内存段
void print_mm_map();

void no_bug_please();

#endif  // INCLUDE_DEBUG_H_