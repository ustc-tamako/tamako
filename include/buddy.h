#ifndef INCLUDE_BUDDY_H_
#define INCLUDE_BUDDY_H_

#include "types.h"

int buddy_init(void * addr, size_t n);

void * buddy_alloc_frames(size_t n);

int buddy_free_frames(void * addr, size_t n);

#endif  // INCLUDE_BUDDY_H_