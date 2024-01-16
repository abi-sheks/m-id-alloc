# (Mid)alloc  
Pretty self explanatory. Wanted to implement a memory allocator as described in [Operating Systems : Three Easy Pieces](https://pages.cs.wisc.edu/~remzi/OSTEP/)'s chapter on [free memory management](https://pages.cs.wisc.edu/~remzi/OSTEP/vm-freespace.pdf)  

## Implementation  
The program uses a simple ```mmap``` syscall to get a page aligned pointer of the designated amount, and presents this as the heap to the user.  
I've currently implemented a first-fit style algorithm that iterates over a free list of chunks and allocates the requested space, segments the rest to ***prevent internal fragmentation***. The header for each chunk lives in the ***actual allocated region*** and not some separate structure, which has lead to some arcane pointer arithmetic. I've sprinkled in comments where i can to ease this.  

## Limitations
Currently not implemented ***coalescing***, leading to fragmentation. This implementation is absolutely ***not memory safe***, you can infact write invalid parts of memory and mess things up. ```sbrk``` not used in case of large requests, program simply returns with error.