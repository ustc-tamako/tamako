#include "vargs.h"
#include "console.h"
#include "debug.h"

extern int vsprintf(char * buf, const char * fmt, va_list args);

void cprintk(const char * fmt, ...)
{
    static char buf[1024];
    va_list args;
    int i;

    va_start(args, fmt);
    i = vsprintf(buf, fmt, args);
    va_end(args);

    buf[i] = '\0';

    console_write(buf);
}

void printk(const char * fmt, ...)
{
    static char buf[1024];
    va_list args;
    int i;

    va_start(args, fmt);
    i = vsprintf(buf, fmt, args);
    va_end(args);

    buf[i] = '\0';

    console_write(buf);
}