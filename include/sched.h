#ifndef INCLUDE_SCHED_H_
#define INCLUDE_SCHED_H_

#include "types.h"
#include "list.h"
#include "spinlock.h"

typedef
enum task_stat_t
{
	TASK_READY = 0,
	TASK_SLEEP = 1,
	TASK_WAITING = 2,
	TASK_ZOMBIE = 3,
} task_stat_t;

struct semaphore_t;

typedef
struct task_t
{
	uint32_t           * esp;
	spinlock_t           lock;
	uint8_t		         pid;
	uint8_t              prio;
	char               * name;
	struct task_t      * parent;
	list_node            children;		// 子任务链表头
	list_node            sibling;		// 兄弟任务链表节点
	uint32_t             time_ticks;
	uint32_t             rest_ticks;
	uint32_t             total_ticks;
	uint32_t             sleep_ticks;
	task_stat_t          stat;
	struct semaphore_t * wait_for;
	list_node            queue_node;	// 任务队列链表节点
} task_t;

#define NR_PRIO		64
#define NR_TASKS	256
extern task_t * task_tbl[NR_TASKS];

#define STACK_SIZE	0x1000
// 获取当前线程
#define current get_current()

static inline task_t * get_current()
{
	task_t * cur;
	__asm__ __volatile__ ("and %%esp, %0" : "=r" (cur) : "0" (~(STACK_SIZE-1))); // task_t 存放在栈的低地址处, 通过 esp & 0xFFFFF000 得到
	return cur;
}

typedef
struct sched_operations
{
	void     (* init)();
	void     (* enqueue)(task_t *);
	void     (* dequeue)(task_t *);
	task_t * (* pick_next)();
} sched_operations;

extern sched_operations const rr_operations;
#define sched_ops	(&rr_operations)

void sched_init();

void schedule();

void sched_tick();

uint32_t kernel_thread(int (* fn)(void *),
                       void * args,
					   uint8_t prio,
					   char * name);

void sleep(uint32_t ticks);

void wakeup(task_t * task);

void wait();

int init();

#endif  // INCLUDE_SCHED_H_
