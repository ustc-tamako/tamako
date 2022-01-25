#ifndef INCLUDE_PRINTK_H_
#define INCLUDE_PRINTK_H_

// 内核打印函数
void printk(const char * fmt, ...);

void cprintk(const char * fmt, ...);

void uprintk(const char * fmt, ...);

#endif  // INCLUDE_PRINTK_H_