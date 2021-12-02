#include "types.h"

int kern_entry()
{
    uint8_t *input = (uint8_t *)0xB8000;
    uint8_t color = (0 << 4) | (15 & 0x0F);

    *input++ = 'H'; *input++ = color;
    *input++ = 'e'; *input++ = color;
    *input++ = 'l'; *input++ = color;
    *input++ = 'l'; *input++ = color;
    *input++ = 'o'; *input++ = color;
    *input++ = ','; *input++ = color;
    *input++ = ' '; *input++ = color;
    *input++ = 'W'; *input++ = color;
    *input++ = 'o'; *input++ = color;
    *input++ = 'r'; *input++ = color;
    *input++ = 'l'; *input++ = color;
    *input++ = 'd'; *input++ = color;
    *input++ = '!'; *input++ = color;
    return 0;
}