#include "rbtree.h"

rb_node const __rb_null = {NULL|RB_BLACK, NULL, NULL};

static void rb_left_rotate(rb_tree * rb, rb_node * ndx)
{
	rb_node * ndy = ndx->right;
	ndx->right = ndy->left;
	if (ndy->left != rb_nullptr) {
		rb_set_parent(ndy->left, ndx);
	}
	rb_set_parent(ndy, rb_parent(ndx));
	if (rb_parent(ndx) == rb_nullptr) {
		rb->root = ndy;
	}
	else if (ndx == rb_parent(ndx)->left) {
		rb_parent(ndx)->left = ndy;
	}
	else {
		rb_parent(ndx)->right = ndy;
	}
	ndy->left = ndx;
	rb_set_parent(ndx, ndy);
}

static void rb_right_rotate(rb_tree * rb, rb_node * ndx)
{
	rb_node * ndy = ndx->left;
	ndx->left = ndy->right;
	if (ndy->right != rb_nullptr) {
		rb_set_parent(ndy->right, ndx);
	}
	rb_set_parent(ndy, rb_parent(ndx));
	if (rb_parent(ndx) == rb_nullptr) {
		rb->root = ndy;
	}
	else if (ndx == rb_parent(ndx)->left) {
		rb_parent(ndx)->left = ndy;
	}
	else {
		rb_parent(ndx)->right = ndy;
	}
	ndy->right = ndx;
	rb_set_parent(ndx, ndy);
}

static inline rb_node * rb_left_most(rb_node * nd)
{
	while (nd->left != rb_nullptr) {
		nd = nd->left;
	}
	return nd;
}

static inline rb_node * rb_right_most(rb_node * nd)
{
	while (nd->right != rb_nullptr) {
		nd = nd->right;
	}
	return nd;
}

static void rb_transplant(rb_tree * rb, rb_node * ndx, rb_node * ndy)
{
	if (rb_parent(ndx) == rb_nullptr) {
		rb->root = ndy;
	}
	else if (ndx == rb_parent(ndx)->left) {
		rb_parent(ndx)->left = ndy;
	}
	else {
		rb_parent(ndx)->right = ndy;
	}
	rb_set_parent(ndy, rb_parent(ndx));
}

static void rb_insert_fixup(rb_tree * rb, rb_node * nd)
{
	while (rb_is_red(rb_parent(nd))) {
		if (rb_parent(nd) == rb_pparent(nd)->left) {
			rb_node * uncle = rb_pparent(nd)->right;
			if (rb_is_red(uncle)) {
				rb_set_color(rb_parent(nd), RB_BLACK);
				rb_set_color(uncle, RB_BLACK);
				rb_set_color(rb_pparent(nd), RB_RED);
				nd = rb_pparent(nd);
			}
			else {
				if (nd == rb_parent(nd)->right) {
					nd = rb_parent(nd);
					rb_left_rotate(rb, nd);
				}
				rb_set_color(rb_parent(nd), RB_BLACK);
				rb_set_color(rb_pparent(nd), RB_RED);
				rb_right_rotate(rb, rb_pparent(nd));
			}
		}
		else {
			rb_node * uncle = rb_pparent(nd)->left;
			if (rb_is_red(uncle)) {
				rb_set_color(rb_parent(nd), RB_BLACK);
				rb_set_color(uncle, RB_BLACK);
				rb_set_color(rb_pparent(nd), RB_RED);
				nd = rb_pparent(nd);
			}
			else {
				if (nd == rb_parent(nd)->left) {
					nd = rb_parent(nd);
					rb_right_rotate(rb, nd);
				}
				rb_set_color(rb_parent(nd), RB_BLACK);
				rb_set_color(rb_pparent(nd), RB_RED);
				rb_left_rotate(rb, rb_pparent(nd));
			}
		}
	}
	rb_set_color(rb->root, RB_BLACK);
}

void rb_insert(rb_tree * rb, rb_node * nd)
{
	rb_node * pre = rb_nullptr;
	rb_node * p = rb->root;
	while (p != rb_nullptr) {
		pre = p;
		if (rb->compare(nd, p) < 0) {
			p = p->left;
		}
		else {
			p = p->right;
		}
	}
	if (pre == rb_nullptr) {
		rb->root = nd;
	}
	else if (rb->compare(nd, pre) < 0) {
		pre->left = nd;
	}
	else {
		pre->right = nd;
	}
	nd->left = rb_nullptr;
	nd->right = rb_nullptr;
	rb_set_parent_color(nd, pre, RB_RED);
	rb_insert_fixup(rb, nd);
}

static void rb_delete_fixup(rb_tree * rb, rb_node * nd)
{
	while (nd != rb->root && rb_is_black(nd)) {
		if (nd == rb_parent(nd)->left) {
			rb_node * bro = rb_parent(nd)->right;
			if (rb_is_red(bro)) {
				rb_set_color(bro, RB_BLACK);
				rb_set_color(rb_parent(nd), RB_RED);
				rb_left_rotate(rb, rb_parent(nd));
				bro = rb_parent(nd)->right;
			}
			if (rb_is_black(bro->left) && rb_is_black(bro->right)) {
				rb_set_color(bro, RB_RED);
				nd = rb_parent(nd);
			}
			else {
				if (rb_is_black(bro->right)) {
					rb_set_color(bro->left, RB_BLACK);
					rb_set_color(bro, RB_RED);
					rb_right_rotate(rb, bro);
					bro = rb_parent(nd)->right;
				}
				rb_set_color(bro, rb_color(rb_parent(nd)));
				rb_set_color(rb_parent(nd), RB_BLACK);
				rb_set_color(bro->right, RB_BLACK);
				rb_left_rotate(rb, rb_parent(nd));
				nd = rb->root;
			}
		}
		else {
			rb_node * bro = rb_parent(nd)->left;
			if (rb_is_red(bro)) {
				rb_set_color(bro, RB_BLACK);
				rb_set_color(rb_parent(nd), RB_RED);
				rb_right_rotate(rb, rb_parent(nd));
				bro = rb_parent(nd)->left;
			}
			if (rb_is_black(bro->left) && rb_is_black(bro->right)) {
				rb_set_color(bro, RB_RED);
				nd = rb_parent(nd);
			}
			else {
				if (rb_is_black(bro->left)) {
					rb_set_color(bro->right, RB_BLACK);
					rb_set_color(bro, RB_RED);
					rb_left_rotate(rb, bro);
					bro = rb_parent(nd)->left;
				}
				rb_set_color(bro, rb_color(rb_parent(nd)));
				rb_set_color(rb_parent(nd), RB_BLACK);
				rb_set_color(bro->left, RB_BLACK);
				rb_right_rotate(rb, rb_parent(nd));
				nd = rb->root;
			}
		}
	}
	rb_set_color(nd, RB_BLACK);
}

void rb_delete(rb_tree * rb, rb_node * nd)
{
	rb_node * ndx = rb_nullptr;
	rb_node * ndy = nd;
	int clr_y = rb_color(ndy);
	if (nd->left == rb_nullptr) {
		ndx = nd->right;
		rb_transplant(rb, nd, nd->right);
	}
	else if (nd->right == rb_nullptr) {
		ndx = nd->left;
		rb_transplant(rb, nd, nd->left);
	}
	else {
		ndy = rb_left_most(nd->right);
		clr_y = rb_color(ndy);
		ndx = ndy->right;
		if (nd == rb_parent(ndy)) {
			rb_set_parent(ndx, ndy);
		}
		else {
			rb_transplant(rb, ndy, ndy->right);
			ndy->right = nd->right;
			rb_set_parent(ndy->right, ndy);
		}
		rb_transplant(rb, nd, ndy);
		ndy->left = nd->left;
		rb_set_parent(ndy->left, ndy);
		rb_set_color(ndy, rb_color(nd));
	}
	if (clr_y == RB_BLACK) {
		rb_delete_fixup(rb, ndx);
	}
}

rb_node * rb_prev(rb_node * nd)
{
	if (nd->left != rb_nullptr) {
		return rb_right_most(nd->left);
	}
	rb_node * pa = rb_parent(nd);
	while (pa != rb_nullptr && nd == pa->left) {
		nd = pa;
		pa = rb_parent(nd);
	}
	return pa;
}

rb_node * rb_next(rb_node * nd)
{
	if (nd->right != rb_nullptr) {
		return rb_left_most(nd->right);
	}
	rb_node * pa = rb_parent(nd);
	while (pa != rb_nullptr && nd == pa->right) {
		nd = pa;
		pa = rb_parent(nd);
	}
	return pa;
}

rb_node * rb_first(rb_tree * rb)
{
	if (rb->root != rb_nullptr) {
		return rb_left_most(rb->root);
	}
	return rb_nullptr;
}

rb_node * rb_last(rb_tree * rb)
{
	if (rb->root != rb_nullptr) {
		return rb_right_most(rb->root);
	}
	return rb_nullptr;
}

/**
 * Example code

#include "common.h"
#include "mm.h"
#include "printk.h"

typedef struct node
{
	rb_node rb_nd;
	int key;
} node;

int rb_cmp(rb_node * ndx, rb_node * ndy)
{
	node * x = container_of(ndx, node, rb_nd);
	node * y = container_of(ndy, node, rb_nd);
	return x->key < y->key;
}

void rb_print(rb_tree * rb)
{
	rb_node * p = rb_nullptr;
	rb_node * que[20];
	int front = 0;
	int rear = 0;
	if (rb->root != rb_nullptr) {
		que[rear++] = rb->root;
	}
	while (front != rear) {
		int n = rear-front;
		while (n--) {
			p = que[front++];
			node * t = container_of(p, node, rb_nd);
			if (rb_is_red(p)) {
				printk("\033[31m%d\033[0m", t->key);
			}
			else {
				printk("%d", t->key);
			}
			if (rb_parent(p) != rb_nullptr) {
				t = container_of(rb_parent(p), node, rb_nd);
				printk("(%d)  ", t->key);
			}
			if (p->left != rb_nullptr) {
				que[rear++] = p->left;
			}
			if (p->right != rb_nullptr) {
				que[rear++] = p->right;
			}
		}
		printk("\n");
	}
	printk("\n");
}

void rb_test()
{
	rb_tree rb;
	rb.root = rb_nullptr;
	rb.compare = rb_cmp;
	node node_arr[12];
	int keys[12] = {1,5,6,7,8,9,10,11,12,13,14,15};
	for (int i = 0; i < 12; i++) {
		rb_init_node(&node_arr[i].rb_nd);
		node_arr[i].key = keys[i];
		rb_insert(&rb, &node_arr[i].rb_nd);
		rb_print(&rb);
	}

	rb_node * p = rb_first(&rb);
	while (p != rb_nullptr) {
		node * t = container_of(p, node, rb_nd);
		printk("%d ", t->key);
		p = rb_next(p);
	}
	printk("\n\n");

	int del_idx[3] = {10, 5, 1};
	for (int i = 0; i < 3; i++) {
		rb_delete(&rb, &node_arr[del_idx[i]].rb_nd);
		rb_print(&rb);
	}

	p = rb_last(&rb);
	while (p != rb_nullptr) {
		node * t = container_of(p, node, rb_nd);
		printk("%d ", t->key);
		p = rb_prev(p);
	}
	printk("\n");
}

 */
