#ifndef INCLUDE_COMMON_H_
#define INCLUDE_COMMON_H_

#include "types.h"

// 端口写一个字节
static inline void outb(uint16_t port, uint8_t value)
{
    __asm__ __volatile__ ("outb %1, %0" : : "dN" (port), "a" (value));
}

// 端口读一个字节
static inline uint8_t inb(uint16_t port)
{
    uint8_t ret;
    __asm__ __volatile__("inb %1, %0" : "=a" (ret) : "dN" (port));
    return ret;
}

static inline void cli()
{
    __asm__ __volatile__ ("cli");
}

static inline void sti()
{
    __asm__ __volatile__ ("sti");
}

#endif  // INCLUDE_COMMON_H_