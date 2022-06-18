#ifndef INCLUDE_SCHED_H_
#define INCLUDE_SCHED_H_

#include "types.h"
#include "list.h"

typedef
enum task_stat_t
{
	TASK_READY,
	TASK_SLEEP,
	TASK_ZOMBIE,
} task_stat_t;

typedef
struct task_t
{
	uint32_t     * esp;
	uint8_t		   pid;
	uint8_t        prio;
	uint32_t       time_ticks;
	uint32_t       rest_ticks;
	uint32_t       total_ticks;
	uint32_t       sleep_ticks;
	task_stat_t    stat;
	list_node      chain;	// 链表节点
} task_t;

#define NR_PRIO		64
#define NR_TASKS	256
extern task_t * task_tbl[NR_TASKS];

extern task_t * const task_idle;

typedef
struct scheduler_t
{
	void     (* init)();
	void     (* enqueue)(task_t *);
	void     (* dequeue)(task_t *);
	task_t * (* pick_next)();
} scheduler_t;

void sched_init();

void schedule();

void sched_tick();

uint32_t kernel_thread(int (* fn)(void *), void * args, uint8_t prio);

int init();

void sleep(uint32_t ticks);

#endif  // INCLUDE_SCHED_H_
