#ifndef MACH_STUPID_H
#define MACH_STUPID_H
#define RB_BLACK 1

static inline void rb_root_init(struct rb_root *root, struct rb_node *node)
{
	root->rb_node = node;
	if (node) {
		node->__rb_parent_color = RB_BLACK; /* black, no parent */
		node->rb_left  = NULL;
		node->rb_right = NULL;
	}
}
#endif // MACH_STUPID_H
