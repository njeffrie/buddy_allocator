#ifndef BUDDY_ALLOCATOR_H_
#define BUDDY_ALLOCATOR_H_

void *buddy_alloc(size_t size);
void buddy_free(void *ptr);

//debug
void dump_tree(int position);

#endif //BUDDY_ALLOCATOR_H_
