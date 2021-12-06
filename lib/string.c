#include "string.h"

inline void memcpy(void * dest, const void * src, size_t len)
{
    char * dest_b = (char *)dest;
    char * src_b = (char *)src;

    if ( (((uint32_t)dest_b | (uint32_t)src_b) & 0x03) == 0) {
        unsigned long * dest_w = (unsigned long *)dest_b;
        unsigned long * src_w = (unsigned long *)src_b;
        while (len >= sizeof(unsigned long)) {
            *dest_w++ = *src_w++;
            len -= sizeof(unsigned long);
        }
    }

    while (len-- != 0) {
        *dest_b++ = *src_b++;
    }
}

inline void memset(void * dest, uint8_t val, size_t len)
{   
    char * dest_b = (char *)dest;

    while (len-- != 0) {
        *dest_b++ = val;
    }
}

inline void bzero(void * dest, size_t len)
{
    memset(dest, 0, len);
}

inline int strcmp(const char * str1, const char * str2)
{
    int ret = 0;
    while (1) {
        ret = *(const unsigned char *)str1 - *(const unsigned char *)str2;
        if (ret != 0 || *str1 == '\0') {
            break;
        }
        str1++;
        str2++;
    }

    return ret;
}

inline void strcpy(char * dest, const char * src)
{
    while ((*dest++ = *src++) != '\0');
}

inline void strcat(char * dest, const char * src)
{
    while (*dest != '\0') {
        dest++;
    }

    while ((*dest++ = *src++) != '\0');
}

inline size_t strlen(const char * src)
{
    size_t len = 0;
    while (*src != '\0') {
        src++;
        len++;
    }

    return len;
}