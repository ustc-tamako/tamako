#include "mm.h"
#include "printk.h"
#include "debug.h"
#include "spinlock.h"

typedef
struct bucket_desc {
	void               * page;      // 存放对象的页
	struct bucket_desc * next;      // 循环链表
	void               * free_ptr;	// 指向第一个空闲对象
	uint16_t             ref_cnt;	// 该桶中已被分配的对象数
	uint16_t             obj_size;	// 桶中对象的大小
} bucket_desc; // 16B

typedef
struct _bucket_desc {
	uint16_t      obj_size;
	uint16_t      list_len;
	bucket_desc * chain;
	spinlock_t    lock;
} _bucket_desc;

// 桶目录
static _bucket_desc bucket_dir[] = {
	{8,	0,	(bucket_desc *)NULL, SPINLOCK_FREE}, // 8B
	{16,	0,	(bucket_desc *)NULL, SPINLOCK_FREE}, // 16B
	{32,	0,	(bucket_desc *)NULL, SPINLOCK_FREE}, // 32B
	{64,	0,	(bucket_desc *)NULL, SPINLOCK_FREE}, // 64B
	{128,	0,	(bucket_desc *)NULL, SPINLOCK_FREE}, // 128B
	{256,	0,	(bucket_desc *)NULL, SPINLOCK_FREE}, // 256B
	{512,	0,	(bucket_desc *)NULL, SPINLOCK_FREE}, // 512B
	{1024,	0,	(bucket_desc *)NULL, SPINLOCK_FREE}, // 1024B
	{2048,	0,	(bucket_desc *)NULL, SPINLOCK_FREE}, // 2048B
	{4096,	0,	(bucket_desc *)NULL, SPINLOCK_FREE}, // 4096B
	{(uint16_t)-1,	0,	(bucket_desc *)NULL, SPINLOCK_FREE}
};

static list_node free_bucket_frames_head = list_empty_head(free_bucket_frames_head);
// 空闲桶描述符链表头
static bucket_desc * free_bucket_desc = (bucket_desc *)NULL;
static spinlock_t free_bucket_lock = SPINLOCK_FREE;

typedef
struct cache_desc {
	uint16_t   obj_size;
	uint16_t   list_len;
	uint32_t * object;
	spinlock_t lock;
} cache_desc;

// 对象缓存目录
static cache_desc cache_dir[] = {
	{8,	0,	(uint32_t *)NULL, SPINLOCK_FREE}, // 8B
	{16,	0,	(uint32_t *)NULL, SPINLOCK_FREE}, // 16B
	{32,	0,	(uint32_t *)NULL, SPINLOCK_FREE}, // 32B
	{64,	0,	(uint32_t *)NULL, SPINLOCK_FREE}, // 64B
	{128,	0,	(uint32_t *)NULL, SPINLOCK_FREE}, // 128B
	{256,	0,	(uint32_t *)NULL, SPINLOCK_FREE}, // 256B
	{512,	0,	(uint32_t *)NULL, SPINLOCK_FREE}, // 512B
	{1024,	0,	(uint32_t *)NULL, SPINLOCK_FREE}, // 1024B
	{2048,	0,	(uint32_t *)NULL, SPINLOCK_FREE}, // 2048B
	{4096,	0,	(uint32_t *)NULL, SPINLOCK_FREE}, // 4096B
	{(uint16_t)-1,	0,	(uint32_t *)NULL, SPINLOCK_FREE}
};

// 缓存大小上限, 当某个对象缓存的大小达到阀值后, 释放链表中 3/4 的对象
#define CACHE_THRESHOLD	0 //  64KB

/*
 * 用于对象缓存链表中查找前驱和后继节点的宏
 * 每个空闲对象用其头部的 8B 空间保存两个指针
 */
#define prev(c) (*(c))
#define next(c) (*((c)+1))

static void bucket_init()
{
	// 申请一个页框, 用来存放空的桶描述符
	bucket_desc * bkt_desc = (bucket_desc *)alloc_page();
	if (bkt_desc == NULL) {
		error_log("Kmalloc Init", "No more page for bucket description.");
		panic("Memory Error");
	}
	// 设置空闲桶描述符链表
	free_bucket_desc = bkt_desc;
	list_add_tail(&to_fr(to_paddr(bkt_desc))->chain, &free_bucket_frames_head);
	// 最后一个描述符的 next 指针赋为空
	bkt_desc += PAGE_SIZE/sizeof(bucket_desc) - 1;
	bkt_desc->next = NULL;
	// 依次连接链表
	while (--bkt_desc >= free_bucket_desc) {
		bkt_desc->next = bkt_desc + 1;
	}
}

void * kmalloc(size_t len)
{
	uint32_t * ret;
	int i = 0;
	_bucket_desc * bkt_dir = bucket_dir;
	while (bkt_dir[i].obj_size < len) {
		i++;
	}
	cache_desc * ch_desc = cache_dir + i;
	// 首先在对象缓存中寻找
	spin_lock(&ch_desc->lock);
	if (ch_desc->list_len > 0) {
		ret = ch_desc->object;
		// ret 是链表中的最后一个对象
		if (--ch_desc->list_len == 0) {
			ch_desc->object = NULL;
		}
		else {
			// 将 ret 从双链表中取出, 重新连接链表
			next((uint32_t *)prev(ret)) = next(ret);
			prev((uint32_t *)next(ret)) = prev(ret);
			ch_desc->object = (uint32_t *)next(ret);
		}
		spin_unlock(&ch_desc->lock);
		// 清空对象头部的两个指针
		*ret = NULL;
		*(ret+1) = NULL;
		return (void *)ret;
	}
	spin_unlock(&ch_desc->lock);

	// 对象缓存中未找到, 则在对象存储桶中查找
	bkt_dir = bucket_dir + i;
	if (bkt_dir->obj_size == (uint16_t)-1) {
		error_log("Kmalloc", "Try to alloc block larger than 4KB.");
		return NULL;
	}
	spin_lock(&bkt_dir->lock);
	bucket_desc * bkt_desc = bkt_dir->chain;
	// 从桶描述符链表中查找第一个有空闲区域的项
	if (bkt_desc != NULL) {
		while (bkt_desc->free_ptr == NULL && bkt_desc->next != bkt_dir->chain) {
			bkt_desc = bkt_desc->next;
		}
	}
	// 没有找到空闲的描述符, 就从空闲桶描述符链表中取一个
	if (bkt_desc == NULL || bkt_desc->free_ptr == NULL) {
		// 空闲桶描述符链表为空, 则调用初始化函数
		spin_lock(&free_bucket_lock);
		if (free_bucket_desc == NULL) {
			bucket_init();
		}
		// 取空闲桶描述符链表的头节点
		bkt_desc = free_bucket_desc;
		free_bucket_desc = free_bucket_desc->next;
		spin_unlock(&free_bucket_lock);

		uint32_t fr_idx;
		bucket_desc * bkt_next = bkt_dir->chain;
		if (bkt_next != NULL) {
			// 头节点不为空, 则将头节点的数据拷贝到新节点中, 再将新节点插入头节点后
			*bkt_desc = *bkt_next;
			bkt_next->next = bkt_desc;
			// 更新页框管理表
			fr_idx = to_fr_idx(to_paddr(bkt_next->page));
			spin_lock(&frame_tab[fr_idx].lock);
			frame_tab[fr_idx].bkt_desc = (void *)bkt_desc;
			spin_unlock(&frame_tab[fr_idx].lock);
			bkt_desc = bkt_next;
		}
		else {
			// 头节点为空, 则设置循环指针, 将 chain 指向新节点
			bkt_desc->next = bkt_desc;
			bkt_dir->chain = bkt_desc;
		}
		bkt_dir->list_len++;
		// 给桶描述符分配一个页框
		bkt_desc->page = alloc_page();
		bkt_desc->free_ptr = bkt_desc->page;
		bkt_desc->ref_cnt = 0;
		bkt_desc->obj_size = bkt_dir->obj_size;
		// 设置页框表的对应项, 标识该页框对应的桶描述符
		fr_idx = to_fr_idx(to_paddr(bkt_desc->page));
		frame_tab[fr_idx].bkt_desc = (void *)bkt_desc;
		// 对分配到的页框进行初始化
		uint32_t * obj_p = bkt_desc->page;
		for (int i = 0; i < PAGE_SIZE/bkt_dir->obj_size - 1; i++) {
			*obj_p = (uint32_t)(obj_p + (bkt_dir->obj_size >> 2));
			obj_p += (bkt_dir->obj_size >> 2);
		}
		// 最后一个对象的指针赋为空
		*obj_p = NULL;
	}
	// 从存储桶中获取第一个空闲对象, 并修改桶的空闲对象链表和引用计数
	ret = bkt_desc->free_ptr;
	bkt_desc->free_ptr = (void *)*ret;
	bkt_desc->ref_cnt++;
	spin_unlock(&bkt_dir->lock);
	// 将空闲对象的指针清零返回
	*ret = 0;
	return (void *)ret;
}

void kfree(void * vaddr)
{
	// 根据虚拟地址找到页框号, 查找页框管理表中对应项, 得到 bucket 对象大小
	uint32_t * va = (uint32_t *)vaddr;
	uint32_t fr_idx = to_fr_idx(to_paddr(va));
	bucket_desc * bkt_desc = (bucket_desc *)frame_tab[fr_idx].bkt_desc;
	size_t size = bkt_desc->obj_size;
	// 找到该对象大小相应的缓存描述符
	int i = 0;
	cache_desc * ch_desc;
	while (cache_dir[i].obj_size < size) {
		i++;
	}
	ch_desc = cache_dir + i;
	if (ch_desc->obj_size == (uint16_t)-1) {
		error_log("Kfree", "Invalid obj_size, seems something wrong in frame_tab.");
		return;
	}
	spin_lock(&ch_desc->lock);
	// 把要释放的对象插入缓存链表中
	uint32_t * obj = ch_desc->object;
	// 该对象是双链表中的第一个节点
	if (obj == NULL) {
		next(va) = (uint32_t)va;
		prev(va) = (uint32_t)va;
	}
	else {
		next(va) = (uint32_t)obj;
		prev(va) = prev(obj);
		next((uint32_t *)prev(obj)) = (uint32_t)va;
		prev(obj) = (uint32_t)va;
	}
	// 更改链表头
	ch_desc->object = va;
	ch_desc->list_len++;
	spin_unlock(&ch_desc->lock);
}

void free_cache()
{
	cache_desc * ch_desc = cache_dir;
	while (ch_desc->obj_size != (uint16_t)-1) {
		// 当缓存对象链表的总大小超过阀值时, 释放 3/4 的对象放回存储桶中
		spin_lock(&ch_desc->lock);
		if (ch_desc->list_len * ch_desc->obj_size > CACHE_THRESHOLD) {
			while (ch_desc->list_len * ch_desc->obj_size > CACHE_THRESHOLD>>2) {
				// 从缓存对象链表中删除尾部节点, 即优先回收最早被释放的对象
				uint32_t * obj = (uint32_t *)prev(ch_desc->object);
				next((uint32_t *)prev(obj)) = (uint32_t)ch_desc->object;
				prev(ch_desc->object) = (uint32_t)prev(obj);
				ch_desc->list_len--;
				if (ch_desc->list_len == 0) {
					ch_desc->object = NULL;
				}
				// 根据对象所在的页找到相应的桶描述符
				uint32_t fr_idx = to_fr_idx(to_paddr(obj));
				bucket_desc * bkt_desc = (bucket_desc *)frame_tab[fr_idx].bkt_desc;
				_bucket_desc * bkt_dir = bucket_dir + (ch_desc - cache_dir);
				spin_lock(&bkt_dir->lock);
				// 将 obj 插入到空闲对象链表的头部
				*obj = (uint32_t)bkt_desc->free_ptr;
				bkt_desc->free_ptr = (void *)*obj;
				bkt_desc->ref_cnt--;
				// 当前页面为空, 将其释放
				if (bkt_desc->ref_cnt == 0) {
					free_page(bkt_desc->page);
					if (bkt_desc->next != bkt_desc) {
						// 链表节点数大于 1, 则将下一个节点的内容拷贝到当前节点, 并释放下一个节点
						bucket_desc * bkt_next = bkt_desc->next;
						*bkt_desc = *bkt_next;
						if (bkt_dir->chain == bkt_next) {
							bkt_dir->chain = bkt_desc;
						}
						// 更新页框管理表
						fr_idx = to_fr_idx(to_paddr(bkt_next->page));
						spin_lock(&frame_tab[fr_idx].lock);
						frame_tab[fr_idx].bkt_desc = (void *)bkt_desc;
						spin_unlock(&frame_tab[fr_idx].lock);
						bkt_desc = bkt_next;
					}
					else {
						bkt_dir->chain = NULL;
					}
					bkt_dir->list_len--;
					spin_lock(&free_bucket_lock);
					bkt_desc->next = free_bucket_desc;
					free_bucket_desc = bkt_desc;
					spin_unlock(&free_bucket_lock);
				}
				spin_unlock(&bkt_dir->lock);
			} // while
		} // if
		spin_unlock(&ch_desc->lock);
		ch_desc++;
	} // while
}


/**************************************************
 *                                                *
 *                    Test Code                   *
 *                                                *
 **************************************************/

void km_print()
{
	info_log("Kmalloc", "");
	int i = 0;
	bucket_desc * bkt_desc = free_bucket_desc;
	int len = 0;
	while (bkt_desc != NULL) {
		len++;
		bkt_desc = bkt_desc->next;
	}
	printk("\033[31mFree_bucket:%4d\033[0m\n", len);
	frame_t * fr = NULL;
	list_for_each_entry(fr, &free_bucket_frames_head, chain) {
		fr_print(fr);
		printk("\n");
	}
	while (bucket_dir[i].obj_size < (uint16_t)-1) {
		bkt_desc = bucket_dir[i].chain;
		if (bkt_desc != NULL) {
			printk("\033[31mSize:%4d\tCache_length:%3d\tBucket_length:%3d\033[0m\n", bucket_dir[i].obj_size, cache_dir[i].list_len, bucket_dir[i].list_len);
			do {
				fr_print(to_fr(to_paddr(bkt_desc->page)));
				printk("N_%03d|\n", bkt_desc->ref_cnt);
				bkt_desc = bkt_desc->next;
			} while (bkt_desc != bucket_dir[i].chain);
		}
		i++;
	}
}

void kmalloc_test()
{
	typedef
 	struct km_test_t {
 		uint32_t a, b, c, d, e;
 		uint32_t t[100];
 	} km_test_t;

	int * a = (int *)kmalloc(sizeof(int));
 	int * b = (int *)kmalloc(sizeof(int));
 	int * c = (int *)kmalloc(sizeof(int));
	km_print();
 	kfree(a);
 	kfree(b);
 	kfree(c);

 	km_print();
	km_test_t * test_arr[64];
	for (int i = 0; i < 64; i++) {
 		test_arr[i] = (km_test_t *)kmalloc(sizeof(km_test_t));
 	}
 	for (int i = 0; i < 32; i++) {
 		kfree(test_arr[i]);
 	}
 	test_arr[33]->c = 19;
 	for (int i = 0; i < 32; i++) {
 		test_arr[i] = (km_test_t *)kmalloc(sizeof(km_test_t));
 	}
 	for (int i = 0; i < 16; i++) {
 		kfree(test_arr[i]);
 	}
 	for (int i = 0; i < 8; i++) {
 		test_arr[i] = (km_test_t *)kmalloc(sizeof(km_test_t));
 	}
 	test_arr[5]->b = 3;
 	for (int i = 32; i < 64; i++) {
 		kfree(test_arr[i]);
 	}
 	int * d = (int *)kmalloc(sizeof(int) * 111);
 	kfree(d);
 	for (int i = 0; i < 8; i++) {
 		kfree(test_arr[i]);
 	}
 	// km_print();
 	for (int i = 16; i < 32; i++) {
 		kfree(test_arr[i]);
 	}
 	km_print();
}
