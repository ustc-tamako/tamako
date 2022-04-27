#ifndef INCLUDE_RBTREE_H_
#define INCLUDE_RBTREE_H_

#include "types.h"

typedef
struct rb_node
{
	uint32_t		 __color_parent;	// 该成员最后 2 位用来保存颜色
	struct rb_node * left;
	struct rb_node * right;
} __attribute__((packed)) rb_node;

typedef
struct rb_tree
{
	struct rb_node * root;
	int (* compare)(rb_node * ndx, rb_node * ndy);
} rb_tree;

#define RB_RED		0
#define RB_BLACK	1

extern rb_node const	__rb_null;

#define rb_nullptr		(rb_node *)&__rb_null

#define rb_parent(nd)	((rb_node *)((nd)->__color_parent & ~3))
#define rb_pparent(nd)	rb_parent(rb_parent(nd))
#define rb_color(nd)	((nd)->__color_parent & 1)
#define rb_is_red(nd)	(!rb_color(nd))
#define rb_is_black(nd)	rb_color(nd)

#define rb_init_node(nd) \
	do { \
		rb_set_parent_color(nd, rb_nullptr, RB_RED); \
		(nd)->left = rb_nullptr; \
		(nd)->right = rb_nullptr; \
	} while (0); \

static inline void rb_set_parent(rb_node * nd, rb_node * pa)
{
	nd->__color_parent = (uint32_t)pa | (nd->__color_parent & 3);
}

static inline void rb_set_color(rb_node * nd, int clr)
{
	nd->__color_parent = (nd->__color_parent & ~1) | (clr & 1);
}

static inline void rb_set_parent_color(rb_node * nd, rb_node * pa, int clr)
{
	nd->__color_parent = (uint32_t)pa | (clr & 1);
}

void rb_insert(rb_tree * rb, rb_node * nd);
void rb_delete(rb_tree * rb, rb_node * nd);

rb_node * rb_prev(rb_node * nd);
rb_node * rb_next(rb_node * nd);
rb_node * rb_first(rb_tree * rb);
rb_node * rb_last(rb_tree * rb);

#endif  // INCLUDE_RBTREE_H_