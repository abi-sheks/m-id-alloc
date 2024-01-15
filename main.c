#define _GNU_SOURCE
#include "stdio.h"
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <sys/mman.h>

size_t MAX_SIZE = 1024;

typedef struct heapblock {
	uint32_t blocksize;
	bool in_use;
	//NULL if inuse, otherwise points to next block on free list.
	heapblock*next;
} heapblock;

heapblock first;

typedef struct
{
	uint32_t available;
	heapblock *start;
} heapmeta;



heapmeta main_heap;
void heap_init(void **start)
{
	*start = mmap(NULL, MAX_SIZE, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANON, -1, 0);
	if (*start == ((void *)-1))
	{
		fprintf(stderr, "ERROR : Bad allocation");
		exit(1);
	}
	//initialize heap data struct
	main_heap.available = MAX_SIZE;
	main_heap.start = *start;
	//initialize first heapblock, whole heap is free
	first.blocksize = main_heap.available;
	first.in_use = false;
	first.next = main_heap.start;
	main_heap.start->next = NULL;
}

void *mid_alloc(size_t len)
{
	//for now, no increase in size
	if(len > main_heap.available)
	{
		fprintf(stdout, "ERROR : Not enough space");
		return (void*)(-1);
	}
	//else, allocate a new block from free list
	heapblock*itr = first.next;
	while(itr != NULL)
	{
		if(itr->next == NULL)
		{
			
		}
		if(itr->next->blocksize >= len)
		{
			//remove from linked list
			main_heap.available -= len;
			heapblock*old_next = itr->next->next;
			heapblock*final_addr = itr->next;
			itr->next->in_use = true;
			itr->next->next = NULL;
			itr->next = old_next;
			//return 
			return (void*)final_addr;
		}
	}
	//no free block found
	return (void*)-1;
}

int main()
{
	void *start = NULL;
	heap_init(&start);
	void*alloced_addr = mid_alloc(10);
	void*next_addr = mid_alloc()
	return 0;
}
