#include <stdio.h>
#include <time.h>
#include <stdlib.h>
#include "buddy_allocator.h"

#define SIZE 16
#define MAX_RAND 1024 * 64
int main(int argv, char *argc[]) {
	void *array[SIZE];
	printf("testing buddy allocator\n");
	int failure_count = 0;
	for (int i=0; i<100; i++) {
		for (int i=0; i<SIZE; i++) {
			int size = rand() % MAX_RAND;
			//printf("allocating size %d\n", size);
			array[i] = buddy_alloc(size);
			if (array[i] == NULL) {
				failure_count ++;
			}
		}
		//dump_tree(1);
		int remaining = SIZE;
		for (int i=0; i<SIZE; i++) {
			int idx = rand() % remaining;
			buddy_free(array[idx]);
			array[idx] = array[remaining - 1];
			remaining --;
		}
		dump_tree(1);
	}
	printf("failure count:%d\n", failure_count);
	return 0;
}
