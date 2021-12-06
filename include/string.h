#ifndef INCLUDE_STRING_H_
#define INCLUDE_STRING_H_

#include "types.h"

void memcpy(void * dest, const void * src, size_t len);

void memset(void * dest, uint8_t val, size_t len);

void bzero(void * dest, size_t len);

int strcmp(const char * str1, const char * str2);

void strcpy(char * dest, const char * src);

void strcat(char * dest, const char * src);

size_t strlen(const char * src);

#endif  // INCLUDE_STRING_H_