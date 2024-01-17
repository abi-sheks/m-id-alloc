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
	// 1 if in use, 0 if free
	char inuse;
	// NULL if inuse or at end of free list, otherwise points to next block on free list.
	struct heapblock *next;
};

// head node of free list
void *fl_head = NULL;

typedef struct
{
	uint32_t available;
	void *start;
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
	// initialize heap data struct
	main_heap.available = MAX_SIZE - sizeof(struct heapblock *) - sizeof(uint32_t) - sizeof(char);
	main_heap.start = *start;
	// make space for header
	// first size of block stored, then next pointer stored.
	fl_head = *start;
	*((uint32_t *)fl_head) = main_heap.available;
	*((char *)(fl_head + sizeof(uint32_t))) = '0';
	*((struct heapblock **)(fl_head + sizeof(uint32_t) + sizeof(char))) = NULL;
	// so valid space region for user is from main_heap.start + sizeof(uint32_t) + sizeof(struct heapblock *) + sizeof(char)
	// block alloc check will compare header space + requested space against the block sizes
	// headerspace size = sizeof(uint32_t) + sizeof(struct heapblock *)
}

// currently, you can pass an invalid pointer and screw up the entire heap
void mid_free(void *ptr)
{
	// insert the newly freed block at the head of the free list

	// get the current head element
	void *curr_head = fl_head;

	// mark the newly freed block by lookbehind
	*((char *)(ptr - sizeof(char) - sizeof(struct heapblock *))) = '0';
	*((struct heapblock **)(ptr - sizeof(struct heapblock *))) = (struct heapblock *)curr_head;

	// make the head pointer point to the newly freed block
	fl_head = (void *)(ptr - sizeof(struct heapblock *) - sizeof(char) - sizeof(uint32_t));
	fprintf(stdout, "Freed! \n");
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
		struct heapblock *curr_next = *(struct heapblock **)(itr + sizeof(uint32_t) + sizeof(char));
		if (avail >= len)
		{
			// check next free chunk if there is no space to allocate a header for to be created free chunk
			if (avail - len < sizeof(struct heapblock *) + sizeof(uint32_t) + sizeof(char))
			{
				old_next = (struct heapblock **)(itr + sizeof(uint32_t) + sizeof(char));
				itr = curr_next;
				continue;
			}
			// if itr is at head, move head ahead by allocated area
			if (itr == fl_head)
			{
				// fill out the allocated block details.
				*((uint32_t *)(itr)) = len;
				*((char *)(itr + sizeof(uint32_t))) = '1';
				*((struct heapblock **)(itr + sizeof(uint32_t))) = NULL; // marks as allocated, no longer on free list.

				//"push down" free head parameters
				fl_head += sizeof(uint32_t) + sizeof(struct heapblock *) + sizeof(char) + len;
				*((uint32_t *)(fl_head)) = avail - sizeof(uint32_t) - sizeof(struct heapblock *) - sizeof(char) - len;
				*((char *)(fl_head + sizeof(uint32_t))) = '0';
				*((struct heapblock **)(fl_head + sizeof(uint32_t) + sizeof(char))) = curr_next;
			}
			// if itr is not head, look behind
			// and change the next pointer of previous free block to point to the new free block
			// rest of the operations are the same
			else
			{
				// get address of new free chunk
				void *new_free = itr + sizeof(struct heapblock *) + sizeof(uint32_t) + sizeof(char) + len;

				// make prev chunks next pointer point there
				*old_next = (struct heapblock *)new_free;

				// make the new free chunk point to old free chunks next and give it proper avail
				*((uint32_t *)new_free) = avail - len - sizeof(uint32_t) - sizeof(struct heapblock *) - sizeof(char);
				*((char *)(new_free + sizeof(uint32_t))) = '0';
				*((struct heapblock **)(new_free + sizeof(uint32_t) + sizeof(char))) = curr_next;

				// fill out the allocated block details.
				*((uint32_t *)(itr)) = len;
				*((char *)(itr + sizeof(char))) = '1';
				*((struct heapblock **)(itr + sizeof(uint32_t) + sizeof(char))) = NULL; // marks as allocated, no longer on free list.
			}

			// fetch the pointer to be returned
			void *sptr = itr + sizeof(struct heapblock *) + sizeof(uint32_t) + sizeof(char);
			return sptr;
		}
		// update prev chunks next pointer (for lookbehind)
		old_next = (struct heapblock **)(itr + sizeof(uint32_t) + sizeof(char));
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
	mid_free(allocated_addr);
	void *next_addr = mid_alloc(2);
	fprintf(stdout, "Allocated block at : %p \n", next_addr);
	return 0;
}
