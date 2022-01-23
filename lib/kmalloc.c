#include "mm.h"
#include "printk.h"
#include "debug.h"

typedef
struct bucket_desc {
	uint32_t              * page;		// 存放对象的页
	struct bucket_desc    * next; 		// 循环链表
	uint32_t              * free_ptr;	// 指向第一个空闲对象
	uint16_t                ref_cnt;	// 该桶中已被分配的对象数
	uint16_t				obj_size;	// 桶中对象的大小
} bucket_desc; // 16B

typedef
struct _bucket_desc {
	size_t          size;
	bucket_desc   * chain;
} _bucket_desc; // 8B

// 桶目录
_bucket_desc bucket_dir[] = {
	{8,     (bucket_desc *)NULL}, // 8B
	{16,    (bucket_desc *)NULL}, // 16B
	{32,    (bucket_desc *)NULL}, // 32B
	{64,    (bucket_desc *)NULL}, // 64B
	{128,   (bucket_desc *)NULL}, // 128B
	{256,   (bucket_desc *)NULL}, // 256B
	{512,   (bucket_desc *)NULL}, // 512B
	{1024,  (bucket_desc *)NULL}, // 1024B
	{2048,  (bucket_desc *)NULL}, // 2048B
	{4096,  (bucket_desc *)NULL}, // 4096B
	{(size_t)-1,(bucket_desc *)NULL}
};
// 空闲桶描述符链表头
bucket_desc * free_bucket_desc = (bucket_desc *)NULL;

typedef 
struct cache_desc {
	uint16_t	obj_size;
	uint16_t	list_len;
	uint32_t  * object;
} cache_desc; // 8B

// 对象缓存目录
cache_desc cache_dir[] = {
	{8,     0, (uint32_t *)NULL}, // 8B
	{16,    0, (uint32_t *)NULL}, // 16B
	{32,    0, (uint32_t *)NULL}, // 32B
	{64,    0, (uint32_t *)NULL}, // 64B
	{128,   0, (uint32_t *)NULL}, // 128B
	{256,   0, (uint32_t *)NULL}, // 256B
	{512,   0, (uint32_t *)NULL}, // 512B
	{1024,  0, (uint32_t *)NULL}, // 1024B
	{2048,  0, (uint32_t *)NULL}, // 2048B
	{4096,  0, (uint32_t *)NULL}, // 4096B
	{(uint16_t)-1, 0, (uint32_t *)NULL}
};

// 缓存大小上限，当某个对象缓存的大小达到阀值后，释放链表中 3/4 的对象
#define CACHE_THRESHOLD	0x10000 //  64KB

/* 
 * 用于对象缓存链表中查找前驱和后继节点的宏
 * 每个空闲对象用其头部的 8B 空间保存两个指针
 */
#define prev(c) (*(c))
#define next(c) (*((c)+1))

extern frame_t frame_tab[MAX_FRAME_NUM];

void bucket_init()
{
	// 申请一个页框，用来存放空的桶描述符
	bucket_desc * bkt_desc = (bucket_desc *)alloc_page();
	if (bkt_desc == NULL) {
		panic("Memory Error: Heap init failed, no more page for bucket description.");
	}
	// 设置空闲桶描述符链表
	free_bucket_desc = bkt_desc;
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
	while (bkt_dir[i].size < len) {
		i++;
	}
	cache_desc * ch_desc = cache_dir + i;
	// 首先在对象缓存中寻找
	if (ch_desc->list_len > 0) {
		ret = ch_desc->object;
		// ret 是链表中的最后一个对象
		if (--ch_desc->list_len == 0) {
			ch_desc->object = NULL;
		}
		else {
			// 将 ret 从双链表中取出，重新连接链表
			next((uint32_t *)prev(ret)) = next(ret);
			prev((uint32_t *)next(ret)) = prev(ret);
			ch_desc->object = (uint32_t *)next(ret);
		}
		// 清空对象头部的两个指针
		*ret = NULL;
		*(ret+1) = NULL;
		return (void *)ret;
	}

	// 对象缓存中未找到，则在对象存储桶中查找
	bkt_dir = bucket_dir + i;
	if (bkt_dir->size == (size_t)-1) {
		panic("Memory Error: Try to alloc block larger than 4KB.");
	}
	bucket_desc * bkt_desc = bkt_dir->chain;
	// 从桶描述符链表中查找第一个有空闲区域的项
	if (bkt_desc != NULL) {
		while (bkt_desc->free_ptr == NULL && bkt_desc->next != bkt_dir->chain) {
			bkt_desc = bkt_desc->next;
		}
	}
	// 没有找到空闲的描述符，就从空闲桶描述符链表中取一个
	if (bkt_desc == NULL || bkt_desc->free_ptr == NULL) {
		// 空闲桶描述符链表为空，则调用初始化函数
		if (free_bucket_desc == NULL) {
			bucket_init();
		}
		uint32_t fr_idx;
		// 取空闲桶描述符链表的头节点
		bkt_desc = free_bucket_desc;
		free_bucket_desc = free_bucket_desc->next;
		bucket_desc * bkt_next = bkt_dir->chain;
		if (bkt_next != NULL) {
			// 头节点不为空，则将头节点的数据拷贝到新节点中，再将新节点插入头节点后
			*bkt_desc = *bkt_next;
			bkt_next->next = bkt_desc;
			// 更新页框管理表
			fr_idx = pg_to_frame(to_paddr((uint32_t)bkt_next->page));
			frame_tab[fr_idx].bkt_desc = (uint32_t *)bkt_desc;
			bkt_desc = bkt_next;
		}
		else {
			// 头节点为空，则设置循环指针，将 chain 指向新节点
			bkt_desc->next = bkt_desc;
			bkt_dir->chain = bkt_desc;
		}
		// 给桶描述符分配一个页框
		bkt_desc->page = (uint32_t *)alloc_page();
		bkt_desc->free_ptr = bkt_desc->page;
		bkt_desc->ref_cnt = 0;
		bkt_desc->obj_size = bkt_dir->size;
		// 设置页框表的对应项，标识该页框对应的桶描述符
		fr_idx = pg_to_frame(to_paddr((uint32_t)bkt_desc->page));
		frame_tab[fr_idx].bkt_desc = (uint32_t *)bkt_desc;
		// 对分配到的页框进行初始化
		uint32_t * obj_p = bkt_desc->page;
		for (int i = 0; i < PAGE_SIZE/bkt_dir->size - 1; i++) {
			*obj_p = (uint32_t)(obj_p + (bkt_dir->size >> 2));
			obj_p += (bkt_dir->size >> 2);
		}
		// 最后一个对象的指针赋为空
		*obj_p = NULL;
	}
	// 从存储桶中获取第一个空闲对象，并修改桶的空闲对象链表和引用计数
	ret = bkt_desc->free_ptr;
	bkt_desc->free_ptr = (uint32_t *)*ret;
	bkt_desc->ref_cnt++;
	// 将空闲对象的指针清零返回
	*ret = 0;
	return (void *)ret;
}

void kfree(void * vaddr)
{
	// 根据虚拟地址找到页框号，查找页框管理表中对应项，得到 bucket 对象大小
	uint32_t * va = (uint32_t *)vaddr;
	uint32_t fr_idx = pg_to_frame(to_paddr((uint32_t)va));
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
		panic("Memory Error: Invalid obj_size, seems something wrong in frame_tab.");
	}
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

	// 当缓存对象链表的总大小超过阀值时，释放 3/4 的对象放回存储桶中
	if (ch_desc->list_len * ch_desc->obj_size >= CACHE_THRESHOLD) {
		bucket_desc * bkt_desc;
		while (ch_desc->list_len * ch_desc->obj_size > CACHE_THRESHOLD>>2) {
			// 从缓存对象链表中删除尾部节点，即优先回收最早被释放的对象
			obj = (uint32_t *)prev(ch_desc->object);
			next((uint32_t *)prev(obj)) = (uint32_t)ch_desc->object;
			prev(ch_desc->object) = (uint32_t)prev(obj);
			ch_desc->list_len--;
			// 根据对象所在的页找到相应的桶描述符
			fr_idx = pg_to_frame(to_paddr((uint32_t)obj));
			bkt_desc = (bucket_desc *)frame_tab[fr_idx].bkt_desc;
			// 将 obj 插入到空闲对象链表的头部
			*obj = (uint32_t)bkt_desc->free_ptr;
			bkt_desc->free_ptr = (uint32_t *)*obj;
			bkt_desc->ref_cnt--;
			// 当前页面为空，将其释放
			if (bkt_desc->ref_cnt == 0) {
				free_page(bkt_desc->page);
				_bucket_desc * bkt_dir = bucket_dir + i;
				if (bkt_desc->next != bkt_desc) {
					// 链表节点数大于 1，则将下一个节点的内容拷贝到当前节点，并释放下一个节点
					bucket_desc * bkt_next = bkt_desc->next;
					*bkt_desc = *bkt_next;
					if (bkt_dir->chain == bkt_next) {
						bkt_dir->chain = bkt_desc;
					}
					// 更新页框管理表
					fr_idx = pg_to_frame(to_paddr((uint32_t)bkt_next->page));
					frame_tab[fr_idx].bkt_desc = (uint32_t *)bkt_desc;
					bkt_desc = bkt_next;
				}
				else {
					bkt_dir->chain = NULL;
				}
				bkt_desc->next = free_bucket_desc;
				free_bucket_desc = bkt_desc;
			}
		}
	}
}

void km_print()
{
	int i = 0;
	int len;
	_bucket_desc * bkt_dir = bucket_dir;
	bucket_desc * bkt_desc;
	while (bkt_dir[i].size < (size_t)-1) {
		bkt_desc = bkt_dir[i].chain;
		len = 0;
		if (bkt_desc != NULL) {
			len++;
			while (bkt_desc->next != bkt_dir[i].chain) {
				len++;
				bkt_desc = bkt_desc->next;
			}
		}
		printk("Bucket %d: %d\n", bkt_dir[i++].size, len);
	}

	bkt_desc = free_bucket_desc;
	len = 0;
	while (bkt_desc != NULL) {
		len++;
		bkt_desc = bkt_desc->next;
	}
	printk("Free bucket: %d\n", len);
}