#ifndef INCLUDE_STRING_H_
#define INCLUDE_STRING_H_

#include "types.h"

static inline void memcpy(void * dest, const void * src, size_t len)
{
	char * dest_b = (char *)dest;
	char * src_b = (char *)src;

	if ( (((uint32_t)dest_b | (uint32_t)src_b) & 0x03) == 0 ) {
		uint32_t * dest_w = (uint32_t *)dest_b;
		uint32_t * src_w = (uint32_t *)src_b;
		while (len >= sizeof(uint32_t)) {
			*dest_w++ = *src_w++;
			len -= sizeof(uint32_t);
		}
		dest_b = (char *)dest_w;
		src_b = (char *)src_w;
	}

	while (len-- != 0) {
		*dest_b++ = *src_b++;
	}
}

static inline void memset(void * dest, uint8_t val, size_t len)
{   
	char * dest_b = (char *)dest;

	while (len-- != 0) {
		*dest_b++ = val;
	}
}

static inline void bzero(void * dest, size_t len)
{
	memset(dest, 0, len);
}

static inline int strcmp(const char * str1, const char * str2)
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

static inline void strcpy(char * dest, const char * src)
{
	while ((*dest++ = *src++) != '\0');
}

static inline void strcat(char * dest, const char * src)
{
	while (*dest != '\0') {
		dest++;
	}

	while ((*dest++ = *src++) != '\0');
}

static inline size_t strlen(const char * src)
{
	size_t len = 0;
	while (*src != '\0') {
		src++;
		len++;
	}

	return len;
}

#endif  // INCLUDE_STRING_H_