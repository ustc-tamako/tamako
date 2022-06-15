#ifndef INCLUDE_DEBUG_H_
#define INCLUDE_DEBUG_H_

// 初始化 Debug 信息
void debug_init();

void panic(const char * msg);

void info_log(const char * type, const char * msg);

void warning_log(const char * type, const char * msg);

void error_log(const char * type, const char * msg);

void test();

#endif  // INCLUDE_DEBUG_H_
