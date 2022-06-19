#ifndef INCLUDE_TASK_QUEUE_H_
#define INCLUDE_TASK_QUEUE_H_

#include "list.h"
#include "spinlock.h"

typedef
struct task_queue_t
{
	list_node  head;
	uint32_t   n_tasks;
	spinlock_t lock;
} task_queue_t;

struct task_t;
typedef struct task_t task_t;

void tq_init(task_queue_t * tq);

void tq_enqueue(task_queue_t * tq, task_t * task);

void tq_dequeue(task_queue_t * tq, task_t * task);

task_t * tq_pick_next(task_queue_t * tq);

#endif  // INCLUDE_TASK_QUEUE_H_
