#include "sched.h"
#include "spinlock.h"
#include "string.h"

static uint8_t const rr_map[8] = {
	0x01, 0x02, 0x04, 0x08,
	0x10, 0x20, 0x40, 0x80,
};

static uint8_t const rr_unmap[256] = {
    0, 0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0, /* 0x00 to 0x0F */
    4, 0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0, /* 0x10 to 0x1F */
    5, 0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0, /* 0x20 to 0x2F */
    4, 0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0, /* 0x30 to 0x3F */
    6, 0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0, /* 0x40 to 0x4F */
    4, 0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0, /* 0x50 to 0x5F */
    5, 0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0, /* 0x60 to 0x6F */
    4, 0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0, /* 0x70 to 0x7F */
    7, 0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0, /* 0x80 to 0x8F */
    4, 0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0, /* 0x90 to 0x9F */
    5, 0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0, /* 0xA0 to 0xAF */
    4, 0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0, /* 0xB0 to 0xBF */
    6, 0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0, /* 0xC0 to 0xCF */
    4, 0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0, /* 0xD0 to 0xDF */
    5, 0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0, /* 0xE0 to 0xEF */
    4, 0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0, /* 0xF0 to 0xFF */
};

typedef
struct rr_scheduler_t
{
	list_node  queue[NR_PRIO];
	uint32_t   n_tasks;
	uint8_t    bit_tbl[8];
	uint8_t    bit_grp;
	spinlock_t lock;
} rr_scheduler_t;

static rr_scheduler_t rr;

static void rr_init()
{
	for (int i = 0; i < NR_PRIO; i++) {
		list_node_init(&rr.queue[i]);
	}
	rr.n_tasks = 0;
	rr.bit_grp = 0;
	bzero(rr.bit_tbl, 8);
	rr.lock = SPINLOCK_FREE;
}

static void rr_enqueue(task_t * task)
{
	spin_lock(&rr.lock);
	uint8_t prio = task->prio;
	list_add_tail(&task->chain, &rr.queue[prio]);
	rr.n_tasks++;
	rr.bit_tbl[prio >> 3] |= rr_map[prio & 0x07];
	rr.bit_grp |= rr_map[prio >> 3];
	spin_unlock(&rr.lock);
}

static void rr_dequeue(task_t * task)
{
	spin_lock(&rr.lock);
	uint8_t prio = task->prio;
	list_del(&task->chain);
	rr.n_tasks--;
	if (list_is_empty(&rr.queue[prio])) {
		rr.bit_tbl[prio >> 3] &= ~rr_map[prio & 0x07];
		if (rr.bit_tbl[prio >> 3] == 0) {
			rr.bit_grp &= ~rr_map[prio >> 3];
		}
	}
	spin_unlock(&rr.lock);
}

static task_t * rr_pick_next()
{
	spin_lock(&rr.lock);
	uint8_t y = rr_unmap[rr.bit_grp];
	uint8_t x = rr_unmap[rr.bit_tbl[y]];
	uint8_t prio = (y << 3) + x;
	task_t * next = container_of(list_first(&rr.queue[prio]), task_t, chain);
	list_del(&next->chain);
	list_add_tail(&next->chain, &rr.queue[prio]);
	spin_unlock(&rr.lock);

	return next;
}

scheduler_t const rr_scheduler = {
	rr_init,
	rr_enqueue,
	rr_dequeue,
	rr_pick_next
};
