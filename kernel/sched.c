#include "sched.h"
#include "mm.h"
#include "list.h"
#include "printk.h"

#define STACK_SIZE	PAGE_SIZE

typedef
struct task_t
{
	uint32_t * esp;
	list_node  chain;	// 链表节点
} task_t;

// 内核栈, 按页对齐
static uint32_t kern_stack[STACK_SIZE>>2] __attribute__((aligned(STACK_SIZE)));
uint32_t * stack_bottom = &kern_stack[STACK_SIZE>>2];

// 就绪队列和僵尸队列
static list_node ready_queue = list_empty_head(ready_queue);
static list_node zombie_queue = list_empty_head(zombie_queue);

// 在 kernel/asm/sched.s 定义
extern void switch_to(task_t * prev, task_t * next);

// 获取当前线程
#define current get_current()

static inline task_t * get_current()
{
	task_t * cur;
	__asm__ __volatile__ ("and %%esp, %0" : "=r" (cur) : "0" (~(STACK_SIZE-1))); // task_t 存放在栈的低地址处, 通过 esp & 0xFFFFF000 得到
	return cur;
}

// 内核线程退出
static void kthread_exit(int stat)
{
	if (stat == 0) {
		list_del(&current->chain);	// 从就绪队列中删除
		list_add_tail(&current->chain, &zombie_queue);	// 添加到僵尸队列
		schedule();
	}
}

// 内核线程执行
static void kexec(int (* fn)(void *), void * args)
{
	int ret = fn(args);	// 执行指定的函数
	kthread_exit(ret);	// 执行完成后退出
}

// 负责回收僵尸线程资源的线程
static int kkill_zombie()
{
	while (1) {
		// 依次释放僵尸线程的资源
		while (!list_is_empty(&zombie_queue)) {
			task_t * task = container_of(list_first(&zombie_queue), task_t, chain);	// 取僵尸链表的第一个 task
			if (task != current) {
				list_del(&task->chain);
				free_page(task);	// 回收该线程的栈
			}
			else {
				break;
			}
		}
		schedule();
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

// 创建内核线程
uint32_t kernel_thread(int (* fn)(void *), void * args)
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
	list_add_tail(&new_task->chain, &ready_queue);	// 插入就绪队列
	return 0;
}

void schedule()
{
	if (!list_is_empty(&ready_queue)) {
		task_t * next = container_of(list_first(&ready_queue), task_t, chain);	// 取就绪队列的第一个 task
		list_del(&next->chain);
		list_add_tail(&next->chain, &ready_queue); // 将该 task 插入到尾部
		if (current != next) {
			switch_to(current, next);
		}
	}
}

void sched_init()
{
	task_t * idle = (task_t *)kern_stack;	// 创建 idle 线程
	list_add_tail(&idle->chain, &ready_queue);
}

static int flag = 0;

static int thread1()
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

static int thread2()
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

void init()
{
	kernel_thread(kkill_zombie, NULL);	// 创建僵尸回收线程
	kernel_thread(kfree_km_cache, NULL);
	kernel_thread(thread1, NULL);
	kernel_thread(thread2, NULL);
}
