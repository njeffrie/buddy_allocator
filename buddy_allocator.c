#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>

#define BLOCK_SIZE 1024
#define MAX_ORDER 11
#define NUM_BLOCKS (1 << MAX_ORDER)
#define LIST_LEN (NUM_BLOCKS * 2)
#define HEAP_SIZE (NUM_BLOCKS * BLOCK_SIZE)
#define MAGIC 0xFEED

typedef struct list_node {
	int is_leaf;
	int is_allocated;
	int order;
	int debug;
} list_node_t;

void *heap_start_ = NULL;
list_node_t *tree_list_;

void dump_tree(int position) {
	list_node_t *node = &tree_list_[position];
	if (!node->is_leaf) {
		dump_tree(position*2);
		dump_tree(position*2 + 1);
	}
	for (int i=0; i<node->order; i++) {
		printf(" ");
	}
	printf("[%d]: is_leaf:%d is_allocated:%d order:%d debug:%d\n", position,
			node->is_leaf, node->is_allocated, node->order, node->debug);
}

int get_order(size_t size) {
	if (size <= 1) return 0;
	size = (size - 1) / BLOCK_SIZE;
	// fls gives order + 1 for exactly 2^n sizes
	int order = fls(size) - ((!(size ^ (1 << (fls(size) - 1)))) ? 1 : 0);
	printf("size %x gives order %d\n", (int)size, order);
	return order;
}

// Returns the array position of a free block with exactly the order specified,
// or NULL if none is found.
int get_exact_fit(int order, int position) {
	//printf("getting exact fit position %d\n", position);
	list_node_t *node = &tree_list_[position];
	if (!node->is_leaf) {
		int result = get_exact_fit(order, position * 2);
		if (!result) {
			result = get_exact_fit(order, position * 2 + 1);
		}
		return result;
	} else if (node -> is_allocated) {
		return 0;
	} else if (node->order == order) {
		assert(node->is_leaf);
		assert(node->debug == MAGIC);
		return position;
	}
	return 0;
}

// Returns the array position with closest fit free block or NULL if no suitable
// block is found.
int get_closest_fit(int order, int position) {
	//printf("getting closest fit\n");
	list_node_t *node = &tree_list_[position];
	if (!node->is_leaf) {
		int result1 = get_closest_fit(order, position * 2);
		int result2 = get_closest_fit(order, position * 2 + 1);
		if (!result1 && !result2) {
			return 0;
		} else if (result1 && !result2) {
			return result1;
		} else if (!result1 && result2) {
			return result2;
		} else {
			list_node_t *node1 = &tree_list_[result1];
			list_node_t *node2 = &tree_list_[result2];
			return (node1->order <= node2->order) ? result1 : result2;
		}
	}
	assert(node->is_allocated || node->order != order);
	return (node->order > order && !node->is_allocated) ? position : 0;
}

// Splits the leaf node in the position specified into two new child leaf nodes.
void split(int position) {
	//printf("splitting at %d\n", position);
	assert(position < LIST_LEN);
	list_node_t *node = &tree_list_[position];
	assert(node->is_leaf);
	assert(!node->is_allocated);

	// 1. no longer leaf since it has children
	node->is_leaf = 0;
	list_node_t *node_l = &tree_list_[position * 2];
	list_node_t *node_r = &tree_list_[position * 2 + 1];
	// 2. initialize child nodes
	node_l->is_leaf = 1;
	node_l->is_allocated = 0;
	node_l->order = node->order - 1;
	node_l->debug = MAGIC;
	node_r->is_leaf = 1;
	node_r->is_allocated = 0;
	node_r->order = node->order - 1;
	node_r->debug = MAGIC;
}

// Combines two leaf nodes which are children of the specified destination node.
void coalesce(int dest) {
	//printf("coalescing to %d\n", dest);
	assert(dest < LIST_LEN / 2);
	list_node_t *node = &tree_list_[dest];
	list_node_t *node_l = &tree_list_[dest * 2];
	list_node_t *node_r = &tree_list_[dest * 2 + 1];
	assert(!node->is_leaf);
	assert(node_l->is_leaf && node_r->is_leaf);
	assert(!node_l->is_allocated && !node_r->is_allocated);
	node->is_leaf = 1;
}

// Gets the heap address corresponding to an element at the position in the free
// tree specified.
void *get_addr(int position) {
	list_node_t *node = &tree_list_[position];
	int order_start_elem = 1 << (MAX_ORDER - node->order);
	int order_index = position - order_start_elem;
	int heap_offset = order_index * (1 << node->order) * BLOCK_SIZE;
	//printf("position %d maps to offset %d\n", position, heap_offset);
	return heap_start_ + heap_offset;
}

int get_position(void *addr, int start) {
	//printf("getting offset %d starting at %d\n", (int)(addr - heap_start_), start);
	list_node_t *node = &tree_list_[start];
	int left_range = (1 << (node->order - 1)) * BLOCK_SIZE;
	//printf("order:%d max block delta:%d\n", node->order, left_range / BLOCK_SIZE);
	if (node->is_leaf) {
		return (get_addr(start) == addr) ? start : 0;
	} else if (addr < get_addr(start) + left_range) {
		return get_position(addr, start * 2);
	} else {
		return get_position(addr, start * 2 + 1);
	}
}

void *buddy_alloc(size_t size) {
	if (!heap_start_) {
		int overhead = 2 * NUM_BLOCKS * sizeof(list_node_t);
		heap_start_ = sbrk(HEAP_SIZE + overhead);
		tree_list_ = (list_node_t *)(heap_start_ + HEAP_SIZE);
		tree_list_[1].is_leaf = 1;
		tree_list_[1].is_allocated = 0;
		tree_list_[1].order = MAX_ORDER;
		tree_list_[1].debug = MAGIC;
	}

	if (size <= 0) {
		return NULL;
		printf("failure due to 0 size allocation\n");
	}
	int order = get_order(size);
	if (order >= MAX_ORDER) {
		printf("failure due to too large a size\n");
		return NULL;
	}
	int result = get_exact_fit(order, 1);
	if (result) {
		tree_list_[result].is_allocated = 1;
		return get_addr(result);
	}
	int nearest = get_closest_fit(order, 1);
	if (!nearest) {
		printf("failure - unable to allocate\n");
		return NULL;
	}
	int order_diff = tree_list_[nearest].order - order;
	for (int i=0; i<order_diff; i++) {
		split(nearest);
		nearest = nearest * 2;
	}
	tree_list_[nearest].is_allocated = 1;
	return get_addr(nearest);
}

void buddy_free(void *ptr) {
	if (ptr < heap_start_ || ptr >= heap_start_ + HEAP_SIZE) {
		return;
	}
	int position = get_position(ptr, 1);
	assert(position);
	tree_list_[position].is_allocated = 0;
	int even_position = position & (~1);
	list_node_t *node_l = &tree_list_[even_position];
	list_node_t *node_r = &tree_list_[even_position + 1];
	while (!node_l->is_allocated && node_l->is_leaf && !node_r->is_allocated &&
			node_r->is_leaf && node_l->order < MAX_ORDER) {
		even_position /= 2;
		coalesce(even_position);
		even_position = even_position & (~1);
		node_l = &tree_list_[even_position];
		node_r = &tree_list_[even_position + 1];
	}
}
