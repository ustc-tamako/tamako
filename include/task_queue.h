#ifndef INCLUDE_TASK_QUEUE_H_
#define INCLUDE_TASK_QUEUE_H_

#include "sched.h"
#include "debug.h"
#include "spinlock.h"

typedef
struct task_queue_t
{
	list_node  head;
	uint32_t   n_tasks;
	spinlock_t lock;
} task_queue_t;

static void tq_init(task_queue_t * tq)
{
	list_node_init(&tq->head);
	tq->n_tasks = 0;
	tq->lock = SPINLOCK_FREE;
}

static void tq_enqueue(task_queue_t * tq, task_t * task)
{
	spin_lock(&tq->lock);
	list_add_tail(&task->chain, &tq->head);
	tq->n_tasks++;
	spin_unlock(&tq->lock);
}

static void tq_dequeue(task_queue_t * tq, task_t * task)
{
	spin_lock(&tq->lock);
	list_del(&task->chain);
	tq->n_tasks--;
	spin_unlock(&tq->lock);
}

static task_t * tq_pick_next(task_queue_t * tq)
{
	task_t * next = NULL;
	spin_lock(&tq->lock);
	if (tq->n_tasks > 0) {
		next = container_of(list_first(&tq->head), task_t, chain);
	}
	spin_unlock(&tq->lock);
	return next;
}

#endif  // INCLUDE_TASK_QUEUE_H_
