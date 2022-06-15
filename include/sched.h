#ifndef INCLUDE_SCHED_H_
#define INCLUDE_SCHED_H_

#include "types.h"
#include "list.h"

typedef
struct task_t
{
	uint32_t * esp;
	uint8_t	   pid;
	list_node  chain;	// 链表节点
} task_t;

#define NR_TASKS	256
extern task_t * task_tbl[NR_TASKS];

uint32_t kernel_thread(int (* fn)(void *), void * args);

void schedule();

void sched_init();

void init();

#endif  // INCLUDE_SCHED_H_
