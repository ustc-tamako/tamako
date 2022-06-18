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

extern sched_operations const rr_operations;
#define sched_ops	(&rr_operations)

static task_queue_t _sleep_queue;
#define sleep_que	(&_sleep_queue)

static task_queue_t _zombie_queue;
#define zombie_que	(&_zombie_queue)

#define IDEL_PRIO	(NR_PRIO - 1)
#define INIT_PRIO	(NR_PRIO - 2)
#define DAEMON_PRIO	(NR_PRIO - 4)

// 获取当前线程
#define current get_current()

static inline task_t * get_current()
{
	task_t * cur;
	__asm__ __volatile__ ("and %%esp, %0" : "=r" (cur) : "0" (~(STACK_SIZE-1))); // task_t 存放在栈的低地址处, 通过 esp & 0xFFFFF000 得到
	return cur;
}

static void task_init(task_t * task, uint8_t prio, char * name)
{
	task->pid = alloc_pid();
	task_tbl[task->pid] = task;
	task->prio = prio;
	task->name = name == NULL ? current->name : name;

	spin_lock(&current->lock);
	task->parent = current;
	list_node_init(&task->children);
	list_add_tail(&task->sibling, &current->children);
	spin_unlock(&current->lock);

	task->time_ticks = prio + 1;
	task->rest_ticks = task->time_ticks;
	task->total_ticks = 0;
	task->sleep_ticks = 0;

	task->wait_for = NULL;
	task->lock = SPINLOCK_FREE;
}

static void task_exit(task_t * task)
{
	task_t * parent = task->parent;
	spin_lock(&parent->lock);
	list_del(&task->sibling);

	spin_lock(&task->lock);
	task_t * child = NULL;
	task_t temp;
	list_for_each_entry(child, &task->children, sibling) {
		list_del(&child->sibling);
		temp = *child;
		list_add_tail(&child->sibling, &parent->children);
		child->parent = parent;
		child = &temp;
	}

	if (parent->stat == TASK_WAITING
		&& parent->wait_for == NULL
		&& list_is_empty(&parent->children)) {
		parent->stat = TASK_READY;
		sched_ops->enqueue(parent);
	}

	spin_unlock(&task->lock);
	spin_unlock(&parent->lock);
}

static void task_kill(task_t * task)
{
	task_tbl[task->pid] = NULL;
	free_pid(task->pid);
	free_page(task);	// 回收该线程的栈
}

static void task_reset_prio(task_t * task, uint8_t prio)
{
	spin_lock(&task->lock);
	if (task->stat == TASK_READY) {
		sched_ops->dequeue(task);
	}
	task->prio = prio;
	task->time_ticks = prio + 1;
	if (task->stat == TASK_READY) {
		sched_ops->enqueue(task);
	}
	spin_unlock(&task->lock);
	if (task == current) {
		schedule();
	}
}

void sched_init()
{
	sched_ops->init();
	tq_init(sleep_que);
	tq_init(zombie_que);

	// idle
	task_init(task_idle, IDEL_PRIO, "idle");
	task_idle->parent = NULL;
	list_node_init(&task_idle->children);
	list_node_init(&task_idle->sibling);
	sched_ops->enqueue(task_idle);
	task_idle->stat = TASK_READY;
}

// 在 kernel/asm/sched.s 定义
extern void switch_to(task_t * prev, task_t * next);

void schedule()
{
	cli();
	task_t * next = sched_ops->pick_next();
	if (current != next) {
		switch_to(current, next);
	}
	sti();
}

void sched_tick()
{
	bool need_schedule = FALSE;
	current->rest_ticks--;
	current->total_ticks++;
	if (current->rest_ticks == 0) {
		current->rest_ticks = current->time_ticks;
		need_schedule = TRUE;
	}

	task_t * task = NULL;
	task_t temp;
	list_for_each_entry(task, &sleep_que->head, queue_node) {
		task->sleep_ticks--;
		if (task->sleep_ticks == 0) {
			tq_dequeue(sleep_que, task);
			task->stat = TASK_READY;
			temp = *task;
			sched_ops->enqueue(task);
			task = &temp;
			need_schedule = TRUE;
		}
	}

	if (need_schedule) {
		schedule();
	}
}

void sleep(uint32_t ticks)
{
	sched_ops->dequeue(current);
	current->sleep_ticks = ticks;
	current->stat = TASK_SLEEP;
	tq_enqueue(sleep_que, current);
	schedule();
}

void wakeup(task_t * task)
{
	tq_dequeue(sleep_que, task);
	task->sleep_ticks = 0;
	task->stat = TASK_READY;
	sched_ops->enqueue(task);
	schedule();
}

void wait()
{
	sched_ops->dequeue(current);
	current->stat = TASK_WAITING;
	schedule();
}

// 内核线程退出
static void kthread_exit(int stat)
{
	if (stat == 0) {
		sched_ops->dequeue(current);
		task_exit(current);
		current->stat = TASK_ZOMBIE;
		tq_enqueue(zombie_que, current);
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
uint32_t kernel_thread(int (* fn)(void *), void * args, uint8_t prio, char * name)
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

	task_init(new_task, prio, name);

	// 将新任务加入就绪队列
	new_task->stat = TASK_READY;
	sched_ops->enqueue(new_task);

	schedule();

	return new_task->pid;
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
			task_kill(task);
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
	kernel_thread(kkill_zombie, NULL, DAEMON_PRIO, "kkill_zombie");
	kernel_thread(kfree_km_cache, NULL, DAEMON_PRIO, "kfree_km_cache");

	kernel_thread(test, NULL, 32, "test_thread");

	task_reset_prio(current, INIT_PRIO);

	while (1);

	return 0;
}

/**************************************************
 *                                                *
 *                    Test Code                   *
 *                                                *
 **************************************************/

void task_print()
{
	char * task_stat_strtab[] = {"RD", "SL", "WA", "ZB"};
	info_log("Task", "");
	printk("\033[01m%2s  %14s    %2s     %4s     %2s    %2s    %5s\033[0m\n", "ID", "NAME", "ST", "STK", "PR", "PA", "TIME");
	for (int i = 0; i < NR_TASKS; i++) {
		if (task_tbl[i] != NULL) {
			printk("%02d  %14s    %2s    \033[032m<%4d>\033[0m    %02d    %02d    %5d\n", task_tbl[i]->pid,
			task_tbl[i]->name,
			task_stat_strtab[task_tbl[i]->stat],
			to_fr_idx(to_paddr(task_tbl[i])),
			task_tbl[i]->prio,
			task_tbl[i]->parent == NULL ? 0 : task_tbl[i]->parent->pid,
			task_tbl[i]->total_ticks);
		}
	}
}

static int flag = 0;

static int test_thread1()
{
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
	kernel_thread(test_thread1, NULL, 33, NULL);
	kernel_thread(test_thread2, NULL, 33, NULL);
	task_print();

	wait();
	task_print();

	sleep(10);
	task_print();
}
