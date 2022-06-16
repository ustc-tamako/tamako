#ifndef INCLUDE_SPINLOCK_H_
#define INCLUDE_SPINLOCK_H_

#include "types.h"

#define barrier()	__asm__ __volatile__ ("" : : : "memory")
#define cpu_relax()	__asm__ __volatile__ ("pause\n" : : : "memory")

static inline uint8_t xchg_8(void * ptr, uint8_t x)
{
	__asm__ __volatile__ ("xchgb %0, %1"
						: "=r" (x)
						: "m" (*(__volatile__ uint8_t *)ptr), "0" (x)
						: "memory");
	return x;
}

typedef uint8_t spinlock_t;

#define SPINLOCK_FREE 0
#define SPINLOCK_BUSY 1

static inline void spin_lock(spinlock_t * lock)
{
	while (1) {
		if (!xchg_8(lock, SPINLOCK_BUSY)) {
			return;
		}
		while (*lock == SPINLOCK_BUSY) {
			cpu_relax();
		}
	}
}

static inline void spin_unlock(spinlock_t * lock)
{
	barrier();
	*lock = SPINLOCK_FREE;
}

#endif  // INCLUDE_SPINLOCK_H_
