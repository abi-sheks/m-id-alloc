#define _GNU_SOURCE
#include "stdio.h"
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <sys/mman.h>

//logical definition, in reality not used.
struct heapblock
{
	uint32_t available;
	// 1 if in use, 0 if free
	char inuse;
	// NULL if inuse or at end of free list, otherwise points to next block on free list.
	struct heapblock *next;
};


//constants
//as header items are laid out in block as size -> inuse -> nextptr
static const size_t MAX_SIZE = 1024;
static const size_t SIZE_OFF = 0;
static const size_t INUSE_OFF = sizeof(uint32_t);
static const size_t NEXT_OFF = sizeof(uint32_t) + sizeof(char);
static const size_t HEADER_OFF = sizeof(struct heapblock*) + sizeof(uint32_t) + sizeof(char);
typedef struct heapblock *nextchunk;


// head node of free list
static void *fl_head = NULL;

void heap_init(void **start)
{
	*start = mmap(NULL, MAX_SIZE, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANON, -1, 0);
	if (*start == ((void *)-1))
	{
		fprintf(stderr, "ERROR : Bad allocation");
		exit(1);
	}
	// initialize heap data struct
	uint32_t available = MAX_SIZE - HEADER_OFF;
	// make space for header
	// first size of block stored, then next pointer stored.
	fl_head = *start;
	*((uint32_t *)fl_head) = available;
	*((char *)(fl_head + INUSE_OFF)) = '0';
	*((nextchunk*)(fl_head + NEXT_OFF)) = NULL;
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
	*((char *)(ptr - sizeof(char) - sizeof(nextchunk))) = '0';
	*((nextchunk*)(ptr - sizeof(nextchunk))) = (nextchunk)curr_head;

	// make the head pointer point to the newly freed block
	fl_head = (void *)(ptr - HEADER_OFF);
	fprintf(stdout, "Freed! \n");
}

void *mid_alloc(size_t len)
{
	// traverse free list and look for block to allot
	void *itr = fl_head;
	// this pointer stores the LOCATION of previous chunk next pointer
	nextchunk *old_next = NULL;
	while (itr != NULL)
	{
		// perform lookahead and check if available size is enough
		// whole block including header will be cordoned off on alloc
		uint32_t avail = *(uint32_t *)(itr);
		nextchunk curr_next = *(nextchunk *)(itr + NEXT_OFF);
		if (avail >= len)
		{
			// check next free chunk if there is no space to allocate a header for to be created free chunk
			if (avail - len < HEADER_OFF)
			{
				old_next = (nextchunk *)(itr + NEXT_OFF);
				itr = curr_next;
				continue;
			}
			// if itr is at head, move head ahead by allocated area
			if (itr == fl_head)
			{
				// fill out the allocated block details.
				*((uint32_t *)(itr)) = len;
				*((char *)(itr + sizeof(uint32_t))) = '1';
				*((nextchunk*)(itr + NEXT_OFF)) = NULL; // marks as allocated, no longer on free list.

				//"push down" free head parameters
				fl_head += HEADER_OFF + len;
				*((uint32_t *)(fl_head)) = avail - len - HEADER_OFF;
				*((char *)(fl_head + INUSE_OFF)) = '0';
				*((nextchunk*)(fl_head + NEXT_OFF)) = curr_next;
			}
			// if itr is not head, look behind
			// and change the next pointer of previous free block to point to the new free block
			// rest of the operations are the same
			else
			{
				// get address of new free chunk
				void *new_free = itr + HEADER_OFF + len;

				// make prev chunks next pointer point there
				*old_next = (nextchunk)new_free;

				// make the new free chunk point to old free chunks next and give it proper avail
				*((uint32_t *)new_free) = avail - len - HEADER_OFF;
				*((char *)(new_free + INUSE_OFF)) = '0';
				*((nextchunk*)(new_free + NEXT_OFF)) = curr_next;

				// fill out the allocated block details.
				*((uint32_t *)(itr)) = len;
				*((char *)(itr + INUSE_OFF)) = '1';
				*((nextchunk*)(itr + NEXT_OFF)) = NULL; // marks as allocated, no longer on free list.
			}

			// fetch the pointer to be returned
			void *sptr = itr + HEADER_OFF;
			return sptr;
		}
		// update prev chunks next pointer (for lookbehind)
		old_next = (nextchunk*)(itr + NEXT_OFF);
		// update iterator
		itr = curr_next;
	}
	// no match
	return (void *)-1;
}


int main()
{
	void*start = NULL;
    heap_init(&start);
    void *allocated_addr = mid_alloc(20);
    fprintf(stdout, "Allocated block at : %p \n", allocated_addr);
    mid_free(allocated_addr);
    void *next_addr = mid_alloc(2);
    fprintf(stdout, "Allocated block at : %p \n", next_addr);
    return 0;
}