#include "task_queue.h"
#include "sched.h"

void tq_init(task_queue_t * tq)
{
	list_node_init(&tq->head);
	tq->n_tasks = 0;
	tq->lock = SPINLOCK_FREE;
}

void tq_enqueue(task_queue_t * tq, task_t * task)
{
	spin_lock(&tq->lock);
	list_add_tail(&task->queue_node, &tq->head);
	tq->n_tasks++;
	spin_unlock(&tq->lock);
}

void tq_dequeue(task_queue_t * tq, task_t * task)
{
	spin_lock(&tq->lock);
	list_del(&task->queue_node);
	tq->n_tasks--;
	spin_unlock(&tq->lock);
}

task_t * tq_pick_next(task_queue_t * tq)
{
	task_t * next = NULL;
	spin_lock(&tq->lock);
	if (tq->n_tasks > 0) {
		next = container_of(list_first(&tq->head), task_t, queue_node);
	}
	spin_unlock(&tq->lock);
	return next;
}
