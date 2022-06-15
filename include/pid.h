#ifndef INCLUDE_PID_H_
#define INCLUDE_PID_H_

#include "types.h"

#define INVALID_PID	0

uint8_t alloc_pid();

void free_pid(uint8_t pid);

#endif  // INCLUDE_PID_H_
