#ifndef INCLUDE_LIST_H_
#define INCLUDE_LIST_H_

#include "common.h"

typedef
struct list_node
{
	struct list_node * prev;
	struct list_node * next;
} list_node;

#define list_empty_head(nd)		{&(nd), &(nd)}

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

static inline void list_add(list_node * nd, list_node * pos)
{
	__list_add(nd, pos, (pos)->next);
}

static inline void list_add_tail(list_node * nd, list_node * pos)
{
	__list_add(nd, (pos)->prev, pos);
}

static inline void list_del(list_node * nd)
{
	__list_del((nd)->prev, (nd)->next);
}

static inline int list_is_empty(list_node * nd)
{
	return (nd == nd->next);
}

static inline list_node * list_first(list_node * head)
{
	return head->next;
}

static inline list_node * list_last(list_node * head)
{
	return head->prev;
}

#define list_for_each(p, head) \
	for ((p) = list_first(head); (p) != (head); (p) = (p)->next)

#define list_for_each_entry(p, head, member) \
	for ((p) = container_of(list_first(head), typeof(*p), member); &(p)->member != (head); (p) = container_of((p)->member.next, typeof(*p), member))

#endif  // INCLUDE_LIST_H_
