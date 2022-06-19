#ifndef INCLUDE_SEMAPHORE_H_
#define INCLUDE_SEMAPHORE_H_

#include "types.h"
#include "task_queue.h"

typedef
struct semaphore_t
{
	size_t       count;
	task_queue_t queue;
} semaphore_t;

static inline void sem_init(semaphore_t * sem, size_t count)
{
	sem->count = count;
	tq_init(&sem->queue);
}

void __sem_wait(semaphore_t * sem);
void __sem_post(semaphore_t * sem);

static inline void sem_wait(semaphore_t * sem)
{
	__asm__ __volatile__ (
		"pushl %%edx\n\t"
		"movl %0, %%edx\n\t"
		"lock; decl (%%edx)\n\t"
		"jns 1f\n\t"
		"pushl %%edx\n\t"
		"call __sem_wait\n\t"
		"popl %%edx\n\t"
		"1:\n\t"
		"popl %%edx"
		: "+m" (sem)
		:
		: "memory"
	);
}

static inline void sem_post(semaphore_t * sem)
{
	__asm__ __volatile__ (
		"pushl %%edx\n\t"
		"movl %0, %%edx\n\t"
		"lock; incl (%%edx)\n\t"
		"jg 2f\n\t"
		"pushl %%edx\n\t"
		"call __sem_post\n\t"
		"popl %%edx\n\t"
		"2:\n\t"
		"popl %%edx"
		: "+m" (sem)
		:
		: "memory"
	);
}

#endif  // INCLUDE_SEMAPHORE_H_
