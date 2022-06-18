#include "sched.h"
#include "mm.h"
#include "printk.h"
#include "debug.h"
#include "pid.h"
#include "task_queue.h"

#define STACK_SIZE	PAGE_SIZE

task_t * task_tbl[NR_TASKS];

// 内核栈, 按页对齐
static uint32_t kern_stack[STACK_SIZE>>2] __attribute__((aligned(STACK_SIZE)));
uint32_t * stack_bottom = &kern_stack[STACK_SIZE>>2];

task_t * const task_idle = (task_t *)kern_stack;

extern scheduler_t const rr_scheduler;
#define scheduler	(&rr_scheduler)

static task_queue_t _sleep_queue;
#define sleep_que	(&_sleep_queue)

static task_queue_t _zombie_queue;
#define zombie_que	(&_zombie_queue)

// 获取当前线程
#define current get_current()

static inline task_t * get_current()
{
	task_t * cur;
	__asm__ __volatile__ ("and %%esp, %0" : "=r" (cur) : "0" (~(STACK_SIZE-1))); // task_t 存放在栈的低地址处, 通过 esp & 0xFFFFF000 得到
	return cur;
}

static inline void task_init(task_t * task, uint8_t prio)
{
	task->pid = alloc_pid();
	task_tbl[task->pid] = task;
	task->prio = prio;
	task->time_ticks = prio + 1;
	task->rest_ticks = task->time_ticks;
	task->total_ticks = 0;
	task->sleep_ticks = 0;
}

void sched_init()
{
	scheduler->init();
	tq_init(sleep_que);
	tq_init(zombie_que);

	// idle
	task_init(task_idle, NR_PRIO - 1);
	scheduler->enqueue(task_idle);
	task_idle->stat = TASK_READY;
}

// 在 kernel/asm/sched.s 定义
extern void switch_to(task_t * prev, task_t * next);

void schedule()
{
	cli();
	task_t * next = scheduler->pick_next();
	if (current != next) {
		switch_to(current, next);
	}
	sti();
}

void sched_tick()
{
	int need_schedule = 0;
	current->rest_ticks--;
	current->total_ticks++;
	if (current->rest_ticks == 0) {
		current->rest_ticks = current->time_ticks;
		need_schedule = 1;
	}

	task_t * task = NULL;
	task_t temp;
	list_for_each_entry(task, &sleep_que->head, chain) {
		task->sleep_ticks--;
		if (task->sleep_ticks == 0) {
			tq_dequeue(sleep_que, task);
			task->stat = TASK_READY;
			temp = *task;
			scheduler->enqueue(task);
			task = &temp;
			need_schedule = 1;
		}
	}

	if (need_schedule) {
		schedule();
	}
}

// 内核线程退出
static void kthread_exit(int stat)
{
	if (stat == 0) {
		scheduler->dequeue(current);
		tq_enqueue(zombie_que, current);
		current->stat = TASK_ZOMBIE;
		schedule();
	}
}

// 内核线程执行
static void kexec(int (* fn)(void *), void * args)
{
	int ret = fn(args);	// 执行指定的函数
	kthread_exit(ret);	// 执行完成后退出
}

// 创建内核线程
uint32_t kernel_thread(int (* fn)(void *), void * args, uint8_t prio)
{
	// 给线程栈分配一个页, task_t 位于页的低地址处
	task_t * new_task = (task_t *)alloc_page();

	// 初始化栈
	uint32_t * ebp = (uint32_t *)((uint32_t)new_task + STACK_SIZE);
	uint32_t * esp = ebp;
	// 压入两个参数
	*(--esp) = (uint32_t)args;
	*(--esp) = (uint32_t)fn;
	*(--esp) = (uint32_t)ebp;	// ebp
	*(--esp) = (uint32_t)kexec;	// switch_to() 返回时跳转到该函数执行
	*(--esp) = 0x200;	// eflags
	for (int i = 0; i < 8; i++) {	// 8 个寄存器值
		*(--esp) = 0;
	}
	new_task->esp = esp;

	task_init(new_task, prio);

	// 将新任务加入就绪队列
	scheduler->enqueue(new_task);
	new_task->stat = TASK_READY;

	schedule();

	return 0;
}

// 负责回收僵尸线程资源的线程
static int kkill_zombie()
{
	while (1) {
		// 依次释放僵尸线程的资源
		task_t * task = tq_pick_next(zombie_que);
		if (task == current || task == NULL) {
			schedule();
		}
		else {
			tq_dequeue(zombie_que, task);
			task_tbl[task->pid] = NULL;
			free_pid(task->pid);
			free_page(task);	// 回收该线程的栈
		}
	}

	return 0;
}

static int kfree_km_cache()
{
	while (1) {
		free_cache();
		schedule();
	}
	return 0;
}

int init()
{
	kernel_thread(kkill_zombie, NULL, NR_PRIO - 2);
	kernel_thread(kfree_km_cache, NULL, NR_PRIO - 2);
	kernel_thread(test, NULL, 60);

	return 0;
}

void sleep(uint32_t ticks)
{
	scheduler->dequeue(current);
	tq_enqueue(sleep_que, current);
	current->sleep_ticks = ticks;
	current->stat = TASK_SLEEP;
	schedule();
}

/**************************************************
 *                                                *
 *                    Test Code                   *
 *                                                *
 **************************************************/

void task_print()
{
	info_log("Task", "");
	for (int i = 0; i < NR_TASKS; i++) {
		if (task_tbl[i] != NULL) {
			printk("Task\033[032m<%4d>\033[0m\t|ID_%02d|PR_%02d|\n", to_fr_idx(to_paddr(task_tbl[i])), task_tbl[i]->pid, task_tbl[i]->prio);
		}
	}
}

static int flag = 0;

static int test_thread1()
{
	sleep(10);
	int cnt = 0;
	while (1) {
		if (!(flag & 1)) {
			printk("A");
			flag = 1;
			cnt++;
		}
		if (cnt == 10) {
			break;
		}
	}
	return 0;
}

static int test_thread2()
{
	sleep(10);
	int cnt = 0;
	while (1) {
		if (flag & 1) {
			printk("B\n");
			flag = 0;
			cnt++;
		}
		if (cnt == 10) {
			break;
		}
	}
	return 0;
}

void sched_test()
{
	kernel_thread(test_thread1, NULL, 33);
	kernel_thread(test_thread2, NULL, 33);
	task_print();
}
