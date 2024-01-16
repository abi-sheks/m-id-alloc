#define _GNU_SOURCE
#include "stdio.h"
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <sys/mman.h>

size_t MAX_SIZE = 1024;

struct heapblock
{
	uint32_t blocksize;
	// NULL if inuse, otherwise points to next block on free list.
	struct heapblock *next;
};

// head node of free list
void *fl_head = NULL;

typedef struct
{
	uint32_t available;
	struct heapblock *start;
} heapmeta;

heapmeta main_heap;
void heap_init(void **start)
{
	*start = (struct heapblock *)mmap(NULL, MAX_SIZE, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANON, -1, 0);
	if (*start == ((void *)-1))
	{
		fprintf(stderr, "ERROR : Bad allocation");
		exit(1);
	}
	// initialize heap data struct
	main_heap.available = MAX_SIZE - sizeof(struct heapblock *) - sizeof(uint32_t);
	main_heap.start = *start;
	// make space for header
	// first size of block stored, then next pointer stored.
	*((uint32_t *)main_heap.start) = main_heap.available;
	*((struct heapblock **)(main_heap.start + sizeof(uint32_t))) = NULL;
	fl_head = main_heap.start;
	// so valid space region for user is from main_heap.start + sizeof(uint32_t) + sizeof(struct heapblock *)
	// block alloc check will compare header space + requested space against the block sizes
	// headerspace size = sizeof(uint32_t) + sizeof(struct heapblock *)
}

void mid_free(void *ptr)
{
}

void *mid_alloc(size_t len)
{
	// traverse free list and look for block to allot
	void *itr = fl_head;
	// this pointer stores the LOCATION of previous chunk next pointer
	struct heapblock **old_next = NULL;
	while (itr != NULL)
	{
		// perform lookahead and check if available size is enough
		// whole block including header will be cordoned off on alloc
		uint32_t avail = *(uint32_t *)(itr);
		struct heapblock *curr_next = *(struct heapblock **)(itr + sizeof(uint32_t));
		if (avail >= len)
		{
			// check next free chunk if there is no space to allocate a header for next free chunk
			if (avail - len < sizeof(struct heapblock **) + sizeof(uint32_t))
			{
				continue;
			}
			// if itr is at head, move head ahead by allocated area
			if (itr == fl_head)
			{
				// fill out the allocated block details.
				*((uint32_t *)(itr)) = len;
				*((struct heapblock **)(itr + sizeof(uint32_t))) = NULL; // marks as allocated, no longer on free list.

				//"push down" free head parameters
				fl_head += sizeof(uint32_t) + sizeof(struct heapblock *) + len;
				*((uint32_t *)(fl_head)) = avail - sizeof(uint32_t) - sizeof(struct heapblock *) - len;
				*((struct heapblock **)(fl_head + sizeof(uint32_t))) = curr_next;
			}
			// if itr is not head, look behind
			// and change the next pointer of previous free block to point to the new free block
			// rest of the operations are the same
			else
			{
				// get address of new free chunk
				void *new_free = itr + sizeof(struct heapblock **) + sizeof(uint32_t) + len;

				// make prev chunks next pointer point there
				*old_next = (struct heapblock *)new_free;

				// make the new free chunk point to old free chunks next and give it proper avail
				*((uint32_t *)new_free) = avail - len - sizeof(uint32_t) - sizeof(struct heapblock **);
				*((struct heapblock **)(new_free + sizeof(uint32_t))) = curr_next;

				// fill out the allocated block details.
				*((uint32_t *)(itr)) = len;
				*((struct heapblock **)(itr + sizeof(uint32_t))) = NULL; // marks as allocated, no longer on free list.
			}

			// fetch the pointer to be returned
			void *sptr = itr + sizeof(struct heapblock *) + sizeof(uint32_t);
			return sptr;
		}
		// update prev chunks next pointer (for lookbehind)
		old_next = (struct heapblock **)(itr + sizeof(uint32_t));
		// update iterator
		itr = curr_next;
	}
	// no match
	return (void *)-1;
}

int main()
{
	void *start = NULL;
	heap_init(&start);
	fprintf(stdout, "Allocated heap starts at : %p \n", start);
	void *allocated_addr = mid_alloc(20);
	fprintf(stdout, "Allocated block at : %p \n", allocated_addr);
	void*next_addr = mid_alloc(12);
	fprintf(stdout, "Allocated block at : %p \n", next_addr);
	return 0;
}
