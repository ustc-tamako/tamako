#ifndef INCLUDE_LIST_H_
#define INCLUDE_LIST_H_

#include "common.h"

typedef
struct list_node
{
	struct list_node * prev;
	struct list_node * next;
} list_node;

static inline void list_node_init(list_node * nd)
{
	nd->prev = nd;
	nd->next = nd;
}

static inline void __list_add(list_node * nd, list_node * prev, list_node * next)
{
	prev->next = nd;
	next->prev = nd;
	nd->prev = prev;
	nd->next = next;
}

static inline void __list_del(list_node * prev, list_node * next)
{
	prev->next = next;
	next->prev = prev;
}

#define list_add(nd, pos)		__list_add(nd, pos, (pos)->next)
#define list_add_tail(nd, pos)	__list_add(nd, (pos)->prev, pos)
#define list_del(nd)			__list_del((nd)->prev, (nd)->next)

#define list_empty_head(nd)		{&(nd), &(nd)}
#define list_is_empty(nd)		((nd) == (nd)->next)
#define list_first(head)		((head)->next)
#define list_last(head)			((head)->prev)

#define list_for_each(p, head) \
	for ((p) = list_first(head); (p) != (head); (p) = (p)->next)

#define list_for_each_entry(p, head, member) \
	for ((p) = container_of(list_first(head), typeof(*p), member); &(p)->member != (head); (p) = container_of((p)->member.next, typeof(*p), member))

#endif  // INCLUDE_LIST_H_