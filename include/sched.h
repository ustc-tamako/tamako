#ifndef INCLUDE_SCHED_H_
#define INCLUDE_SCHED_H_

#include "types.h"

uint32_t kernel_thread(int (* fn)(void *), void * args);

void schedule();

void sched_init();

void init();

#endif  // INCLUDE_SCHED_H_
