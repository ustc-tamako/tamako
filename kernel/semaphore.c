#include "semaphore.h"

void __sem_wait(semaphore_t * sem)
{
	sched_ops->dequeue(current);
	current->stat = TASK_WAITING;
	current->wait_for = sem;
	tq_enqueue(&sem->queue, current);

	schedule();
}

void __sem_post(semaphore_t * sem)
{
	task_t * task = tq_pick_next(&sem->queue);
	tq_dequeue(&sem->queue, task);
	task->stat = TASK_READY;
	task->wait_for = NULL;
	sched_ops->enqueue(task);
}
