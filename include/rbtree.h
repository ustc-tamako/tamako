#ifndef INCLUDE_RBTREE_H_
#define INCLUDE_RBTREE_H_

#include "types.h"

typedef
struct rb_node
{
	uint32_t         __color_parent;  // 该成员最后 2 位用来保存颜色
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

#define rb_nullptr	(rb_node *)&__rb_null

static inline rb_node * rb_parent(rb_node * nd)
{
	return (rb_node *)((nd)->__color_parent & ~3);
}

static inline rb_node * rb_pparent(rb_node * nd)
{
	return rb_parent(rb_parent(nd));
}

static inline int rb_color(rb_node * nd)
{
	return nd->__color_parent & 1;
}

static inline int rb_is_red(rb_node * nd)
{
	return !rb_color(nd);
}

static inline int rb_is_black(rb_node * nd)
{
	return rb_color(nd);
}

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

static inline void rb_init_node(rb_node * nd)
{
	rb_set_parent_color(nd, rb_nullptr, RB_RED);
	nd->left = rb_nullptr;
	nd->right = rb_nullptr;
}

void rb_insert(rb_tree * rb, rb_node * nd);
void rb_delete(rb_tree * rb, rb_node * nd);

rb_node * rb_prev(rb_node * nd);
rb_node * rb_next(rb_node * nd);
rb_node * rb_first(rb_tree * rb);
rb_node * rb_last(rb_tree * rb);

#endif  // INCLUDE_RBTREE_H_
