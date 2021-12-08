#ifndef INCLUDE_DEBUG_H_
#define INCLUDE_DEBUG_H_

#define assert(x, info) (    \
    do {                     \
        if (!(x)) {          \
            panic(info);     \
        }                    \
    } while(0)               \
)

// 初始化Debug 信息
void debug_init();

// 打印当前函数调用栈信息
void panic(const char * msg);

// 打印当前的段寄存器值
void print_cur_status();

#endif  // INCLUDE_DEBUG_H_