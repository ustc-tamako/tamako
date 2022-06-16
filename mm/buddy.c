#include "mm.h"
#include "list.h"
#include "printk.h"
#include "debug.h"

typedef
struct buddy_desc
{
	uint16_t  order;
	uint16_t  list_len;
	list_node head;
} buddy_desc;

// 伙伴算法分配的连续页框数从 2^0 - 2^10 个
#define MAX_ORDER	10
static buddy_desc buddy_dir[MAX_ORDER+1];
static spinlock_t buddy_dir_lock = SPINLOCK_FREE;

// 使用伙伴算法管理的物理地址区间
static uint32_t buddy_start_addr;
static uint32_t buddy_end_addr;
// 判断一个物理地址是否在伙伴算法的管理地址区间内
#define buddy_valid_addr(pa)	(buddy_start_addr <= (uint32_t)(pa) && (uint32_t)(pa) < buddy_end_addr)

// frame_t 类型中 flag 成员各位的意义
// 第 5 位标记该页框是否用于伙伴系统
#define BUDDY_VALID		0x10

static inline int buddy_is_valid(frame_t * fr)
{
	return fr->flag & BUDDY_VALID;
}

// 第 6 位标记该页框是否空闲
#define BUDDY_FREE		0x20

static inline int buddy_is_free(frame_t * fr)
{
	return fr->flag & BUDDY_FREE;
}

// 第 3-0 位标记该页框的 order
static inline uint32_t buddy_order(frame_t * fr)
{
	return fr->flag & 0xF;
}

static inline void buddy_set_order(frame_t * fr, uint32_t od)
{
	fr->flag = (fr->flag & ~0xF) | (od & 0xF);
}

// 获取指定页框的伙伴
static inline uint32_t buddy_index(uint32_t fr_idx, uint32_t od)
{
	return fr_idx ^ (1 << od);
}

static inline frame_t * buddy_frame(frame_t * fr, uint32_t od)
{
	return frame_tab + buddy_index(fr_index(fr), od);
}

/**
 * @brief 将输入页框数 n 向上转换为 2 的幂次数, 输出 order 是满足 2^order >= n * 的最小整数, 当 n 的值超出 MAX_ORDER 时, 返回 MAX_ORDER+1.
 *
 * @param n 输入页框数
 * @return uint32_t 输出的 order 值
 */
static inline uint32_t ceil_order(size_t n)
{
	uint32_t order = 0;
	while (order <= MAX_ORDER+1 && (1<<order) < n) {
		order++;
	}
	return order;
}

static int buddy_init(void * addr, size_t n)
{
	if (((uint32_t)addr & (PAGE_SIZE-1)) != 0) {
		error_log("Buddy Init", "Input address not aligned by frame size.");
		return FALSE;
	}

	buddy_start_addr = (uint32_t)addr;
	buddy_end_addr = (uint32_t)addr + (n<<12);

	// 对伙伴系统目录进行初始化
	uint32_t od = 0;
	for (; od <= MAX_ORDER; od++) {
		buddy_dir[od].order = od;
		buddy_dir[od].list_len = 0;
		list_node_init(&buddy_dir[od].head);
	}

	// 对属于伙伴系统管理的所有页框进行初始化
	uint32_t fr_idx = to_fr_idx(addr);
	frame_t * fr = frame_tab + fr_idx;
	uint32_t i = 0;
	while (i < n) {
		fr->flag |= BUDDY_VALID;
		fr->flag |= BUDDY_FREE;
		buddy_set_order(fr, -1);
		i++;
		fr++;
	}

	/**
	 * 由于给出的地址范围可能并未按照 1K 个页框对齐, 在初始化时先将地址范围头部的页框
	 * 依此插入对应 order 的 buddy 链表中.
	 */
	uint32_t ni = (1<<MAX_ORDER) - (fr_idx&((1<<MAX_ORDER)-1));
	n -= ni;
	fr = frame_tab + fr_idx;
	od = 0;
	uint32_t sz = 1;
	while (ni > 0) {
		if ((ni & sz) != 0) {
			fr->flag |= BUDDY_FREE;
			buddy_set_order(fr, od);
			list_add_tail(&fr->chain, &buddy_dir[od].head);
			buddy_dir[od].list_len++;
			fr += sz;
			ni -= sz;
		}
		od++;
		sz <<= 1;
	}

	/**
	 * 处理完头部的地址空间后, 将按照 1K 个页对齐的地址插入 MAX_ORDER 的链表中, 若
	 * 之后还存在多余的页框, 依此将其插入对应 order 的链表中.
	 */
	od = MAX_ORDER;
	sz = 1<<od;
	while (n > 0) {
		while (n >= sz) {
			fr->flag |= BUDDY_VALID;
			fr->flag |= BUDDY_FREE;
			buddy_set_order(fr, od);
			list_add_tail(&fr->chain, &buddy_dir[od].head);
			buddy_dir[od].list_len++;
			fr += sz;
			n -= sz;
		}
		od--;
		sz >>= 1;
	}

	return TRUE;
}

static frame_t * __buddy_alloc(uint32_t order)
{
	spin_lock(&buddy_dir_lock);
	// 查找第一个符合分配要求的链表
	uint32_t od = order;
	while (od <= MAX_ORDER && list_is_empty(&buddy_dir[od].head)) {
		od++;
	}

	// 分配失败的情况, 输入的 order 太大, 或者没有找到符合条件的非空链表
	if (od > MAX_ORDER) {
		error_log("Buddy Alloction", "No more free frames.");
		spin_unlock(&buddy_dir_lock);
		return NULL;
	}

	// 取出链表的头节点
	list_node * nd = list_first(&buddy_dir[od].head);
	frame_t * fr = container_of(nd, frame_t, chain);
	spin_lock(&fr->lock);
	fr->flag &= ~BUDDY_FREE;
	buddy_set_order(fr, order);
	list_del(nd);
	buddy_dir[od].list_len--;
	spin_unlock(&fr->lock);

	// 将多余的连续块分解为多个不同 order 的块, 分别放回伙伴系统相应的链表中
	while (od > order) {
		od--;
		frame_t * buddy = fr + (1<<od);
		spin_lock(&buddy->lock);
		buddy->flag |= BUDDY_FREE;
		buddy_set_order(buddy, od);
		list_add_tail(&buddy->chain, &buddy_dir[od].head);
		buddy_dir[od].list_len++;
		spin_unlock(&buddy->lock);
	}
	spin_unlock(&buddy_dir_lock);

	return fr;
}

static void * buddy_alloc_frames(size_t n)
{
	uint32_t order = ceil_order(n);
	frame_t * fr = __buddy_alloc(order);

	if (fr == NULL) {
		error_log("Buddy Alloction", "Frames alloc failed, return NULL.");
		return NULL;
	}

	/**
	 * 由于输入的 n 可能不是 2 的幂次倍, 但由于 n 必然能够表示成多个 2 的幂次数之
	 * 和, 因此对得到的连续页框进一步拆分, 将头部的 n 个页框返回, 而剩余部分回收到
	 * 伙伴系统中.
	 * 如申请 10 个连续块时, 调用下层接口得到的是 16 个连续块, 接着将前 10 个块分
	 * 解为 8+2 个连续块返回.而剩下的 6 个连续块则被分为 2+4 个连续块回收到伙伴
	 * 系统中.
	 */
	if (n != 1<<order) {
		int ni = n;
		uint32_t od = order - 1;
		uint32_t sz = 1<<od;
		frame_t * f = fr;
		while (ni > 0) {
			if ((ni & sz) != 0) {
				spin_lock(&f->lock);
				f->flag &= ~BUDDY_FREE;
				buddy_set_order(f, od);
				spin_unlock(&f->lock);
				f += sz;
				ni -= sz;
			}
			od--;
			sz >>= 1;
		}

		ni = (1<<order) - n;
		od = 0;
		sz = 1;
		spin_lock(&buddy_dir_lock);
		while (ni > 0) {
			if ((ni & sz) != 0) {
				spin_lock(&f->lock);
				f->flag |= BUDDY_FREE;
				buddy_set_order(f, od);
				list_add_tail(&f->chain, &buddy_dir[od].head);
				buddy_dir[od].list_len++;
				spin_unlock(&f->lock);
				f += sz;
				ni -= sz;
			}
			od++;
			sz <<= 1;
		}
		spin_unlock(&buddy_dir_lock);
	}

	return fr_paddr(fr);
}

static void __buddy_free(frame_t * fr)
{
	uint32_t fr_idx = fr_index(fr);
	uint32_t order = buddy_order(fr);
	uint32_t bd_idx;
	frame_t * buddy = NULL;

	spin_lock(&buddy_dir_lock);
	while (1) {
		spin_lock(&fr->lock);
		fr->flag |= BUDDY_FREE;
		spin_unlock(&fr->lock);
		bd_idx = buddy_index(fr_idx, order);
		buddy = frame_tab + bd_idx;

		/**
		 * 页框能够与其伙伴合并的条件
		 * 1. 页框的 order 有效且与其伙伴的 order 相等
		 * 2. 其伙伴应该属于伙伴系统管理, 且未被分配
		 */
		spin_lock(&buddy->lock);
		if ( ! (order < MAX_ORDER
			&& (order == buddy_order(buddy))
			&& buddy_is_valid(buddy)
			&& buddy_is_free(buddy)) ) {
			spin_unlock(&buddy->lock);
			break;
		}
		list_del(&buddy->chain);
		buddy_dir[order].list_len--;
		spin_unlock(&buddy->lock);

		// 得到合并后新块的首个页框
		fr_idx &= bd_idx;
		fr = frame_tab + fr_idx;
		// 将新块在原 order 下的伙伴标记为无效
		buddy = buddy_frame(fr, order);
		spin_lock(&buddy->lock);
		buddy->flag &= ~BUDDY_FREE;
		buddy_set_order(buddy, -1);
		spin_unlock(&buddy->lock);
		// 给新块设置 order
		order++;
		spin_lock(&fr->lock);
		buddy_set_order(fr, order);
		spin_unlock(&fr->lock);
	}

	spin_lock(&fr->lock);
	list_add_tail(&fr->chain, &buddy_dir[order].head);
	buddy_dir[order].list_len++;
	spin_unlock(&fr->lock);

	spin_unlock(&buddy_dir_lock);
}

static int buddy_free_frames(void * addr, size_t n)
{
	if (!buddy_valid_addr(addr)) {
		error_log("Buddy Free", "Input address invalid.");
		return FALSE;
	}
	if (((uint32_t)addr & (PAGE_SIZE-1)) != 0) {
		error_log("Buddy Free", "Input address not aligned by frame size.");
		return FALSE;
	}
	if (ceil_order(n) > MAX_ORDER) {
		error_log("Buddy Free", "Input n_frames larger than 2^MAX_ORDER");
		return FALSE;
	}

	/**
	 * 与分配时类似, 这里输入 n 也可以被写成多个 2 的幂次数之和, 由于在分配时取的是
	 * 头部块, 因此回收时也只需要依次回收到对应 order 的链表中即可.
	 * 如回收 10 个连续块时, 将其分解为 8+2 个连续块依此回收到对应链表中, 如果各个
	 * 块的伙伴可以合并, 则与之合并成更大的连续块.
	 */
	int ni = n;
	frame_t * fr = to_fr(addr);
	uint32_t od = ceil_order(n);
	uint32_t sz = 1<<od;
	while (ni > 0) {
		if ((ni & sz) != 0) {
			__buddy_free(fr);
			fr += sz;
			ni -= sz;
		}
		od--;
		sz >>= 1;
	}

	return TRUE;
}

// 伙伴算法的页框管理操作集合
mm_operations const buddy_operations = {
	buddy_init,
	buddy_alloc_frames,
	buddy_free_frames
};


/**************************************************
 *                                                *
 *                    Test Code                   *
 *                                                *
 **************************************************/

void fr_print(frame_t * fr)
{
	printk("\033[032m<%4d>\033[0m\t|OD_%02d", fr_index(fr), buddy_order(fr));
	printk("|%5s", buddy_is_valid(fr) ? "BD_VA" : "");
	printk("|%5s", buddy_is_free(fr) ? "BD_FR" : "");
	printk("|");
}

void buddy_print()
{
	info_log("Buddy System", "");
	buddy_desc * bd_desc = NULL;
	for (int i = 0;	i <= MAX_ORDER; i++) {
		bd_desc = buddy_dir + i;
		if (bd_desc->list_len == 0) {
			continue;
		}
		printk("\033[31mOrder:%2d\tList_length:%4d\033[0m\n", bd_desc->order, bd_desc->list_len);

		frame_t * fr = NULL;
		frame_t * bd = NULL;
		// frame_t * prev = NULL;
		// frame_t * next = NULL;
		list_for_each_entry(fr, &bd_desc->head, chain) {
			printk("Frame");
			fr_print(fr);
			printk("\t");

			bd = buddy_frame(fr, buddy_order(fr));
			printk("Buddy");
			fr_print(bd);
			printk("\n");

			// if (fr->chain.prev != &bd_desc->head) {
			// 	prev = container_of(fr->chain.prev, frame_t, chain);
			// }
			// else {
			// 	prev = container_of(fr->chain.prev->prev, frame_t, chain);
			// }
			// printk("Prev:%4d\tBase_addr:0x%08X\tOrder:%2d\n", fr_index(prev), (uint32_t)fr_paddr(prev), prev->flag & 0xF);
			// if (fr->chain.next != &bd_desc->head) {
			// 	next = container_of(fr->chain.next, frame_t, chain);
			// }
			// else {
			// 	next = container_of(fr->chain.next->next, frame_t, chain);
			// }
			// printk("Next:%4d\tBase_addr:0x%08X\tOrder:%2d\n", fr_index(next), (uint32_t)fr_paddr(next), next->flag & 0xF);
		}
	}
}

void buddy_test()
{
	// buddy_init((void *)0x13000, MAX_FRAME_NUM-227);

	buddy_print();

	void * frames[10];

	frames[0] = buddy_alloc_frames(1);
	frames[1] = buddy_alloc_frames(2);
	frames[2] = buddy_alloc_frames(1);
	frames[3] = buddy_alloc_frames(1);
	frames[4] = buddy_alloc_frames(1);
	frames[5] = buddy_alloc_frames(1);
	frames[6] = buddy_alloc_frames(1);
	frames[7] = buddy_alloc_frames(1);

	// buddy_print();

	buddy_free_frames(frames[0], 1);
	buddy_free_frames(frames[1], 2);
	buddy_free_frames(frames[2], 1);
	buddy_free_frames(frames[3], 1);
	buddy_free_frames(frames[4], 1);
	buddy_free_frames(frames[5], 1);
	buddy_free_frames(frames[6], 1);
	buddy_free_frames(frames[7], 1);

	// buddy_print();

	frames[0] = buddy_alloc_frames(1);
	frames[1] = buddy_alloc_frames(1);
	frames[2] = buddy_alloc_frames(1);
	frames[3] = buddy_alloc_frames(8);
	frames[4] = buddy_alloc_frames(16);
	frames[5] = buddy_alloc_frames(18);
	frames[6] = buddy_alloc_frames(27);
	frames[7] = buddy_alloc_frames(32);
	frames[8] = buddy_alloc_frames(129);
	frames[9] = buddy_alloc_frames(547);

	// buddy_print();

	buddy_free_frames(frames[0], 1);
	buddy_free_frames(frames[1], 1);
	buddy_free_frames(frames[2], 1);
	buddy_free_frames(frames[3], 8);
    buddy_free_frames(frames[4], 16);
    buddy_free_frames(frames[5], 18);
    buddy_free_frames(frames[6], 27);
    buddy_free_frames(frames[7], 32);
    buddy_free_frames(frames[8], 129);
    buddy_free_frames(frames[9], 547);

	buddy_print();
}
