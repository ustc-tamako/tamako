#include "common.h"

inline void outb(uint16_t port, uint8_t value)
{
    __asm__ __volatile__ ("out %1, %0" : : "dN" (port), "a" (value));
}

inline uint8_t inb(uint16_t port)
{
    uint8_t ret;
    __asm__ __volatile__ ("inb %1, %0" : "=a" (ret) : "dN" (port));
    return ret;
}

inline uint16_t inw(uint16_t port)
{
    uint16_t ret;
    __asm__ __volatile__ ("inw %1, %0" : "=a" (ret) : "dN" (port));
    return ret;
}