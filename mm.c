#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <unistd.h>

#include "memlib.h"
#include "mm.h"

#define UNSCALED_POINTER_ADD(p, x) ((void*)((char*)(p) + (x)))
#define UNSCALED_POINTER_SUB(p, x) ((void*)((char*)(p) - (x)))

typedef struct _BlockInfo {
  long int size;
  struct _Block* prev;
} BlockInfo;


typedef struct _FreeBlockInfo {
  struct _Block* nextFree;
  struct _Block* prevFree;
} FreeBlockInfo;

typedef struct _Block {
  BlockInfo info;
  FreeBlockInfo freeNode;
} Block;

static Block* free_list_head = NULL;
static Block* malloc_list_tail = NULL;

static size_t heap_size = 0;

#define WORD_SIZE sizeof(void*)
#define ALIGNMENT (sizeof(FreeBlockInfo))

void* requestMoreSpace(size_t reqSize);
Block* first_block();
Block* next_block(Block* block);
void examine_heap();
int check_heap();

//  Look for a free block that can fit the given amount of space exhaustively
Block* searchList(size_t reqSize) {
  Block* ptrFreeBlock = first_block();

  //check if the Block is free or not, if not go to the next Block
  while (ptrFreeBlock) { 
    if (ptrFreeBlock->info.size < 0 && (-ptrFreeBlock->info.size) >= reqSize) {
      return ptrFreeBlock;
    }
    ptrFreeBlock = next_block(ptrFreeBlock);
  }

  return NULL;  // no block found
}

/* Find a free block of at least the requested size in the free list.  Returns
   NULL if no free block is large enough. */
Block* searchFreeList(size_t reqSize) {
  Block* ptrFreeBlock = free_list_head;

  while (ptrFreeBlock) {
    if (-(ptrFreeBlock->info.size) >= reqSize) {  
      return ptrFreeBlock;
    }
    ptrFreeBlock = ptrFreeBlock->freeNode.nextFree;  
  }
  
  return NULL;
}

// Add block to the Free List
void addToFreeList(Block* block) {
  block->freeNode.nextFree = free_list_head;
  block->freeNode.prevFree = NULL;
  if (free_list_head) {
    // Update free list head if necessary
    free_list_head->freeNode.prevFree = block;
  }
  // Update free list head if necessary
  free_list_head = block;
}

// Remove block from the Free List
void removeFromFreeList(Block* block) {
  if (block->freeNode.prevFree) {
    block->freeNode.prevFree->freeNode.nextFree = block->freeNode.nextFree;
  } else {
      // Update free list head if necessary
    free_list_head = block->freeNode.nextFree;
  }

  if (block->freeNode.nextFree) {
    block->freeNode.nextFree->freeNode.prevFree = block->freeNode.prevFree;
  }
}
// TOP-LEVEL ALLOCATOR INTERFACE ------------------------------------

/* Allocate a block of size size and return a pointer to it. If size is zero,
 * returns null.
 */
void* mm_malloc(size_t size) {
  if (size == 0) {
    return NULL;  // Explicitly return NULL for size 0
  }
  long int reqSize = size;
  reqSize = ALIGNMENT * ((reqSize + ALIGNMENT - 1) / ALIGNMENT); 

  // Find a free block from free list
  Block* block = searchFreeList(reqSize);
  if (block) {
    // Mark as Allocated
    block->info.size = -(block->info.size);

    // Split
    long int excessSize = block->info.size - reqSize - sizeof(BlockInfo);
    if (excessSize >= (long int)sizeof(Block)) {

      // new block is the block we splited out, which is free
      Block* newBlock = (Block*) UNSCALED_POINTER_ADD(block, reqSize + sizeof(BlockInfo));
      newBlock->info.size = -excessSize;
      newBlock->info.prev = block;

      // Let next block point to the new block
      Block* nextBlock = next_block(block);
      if (nextBlock) {
        nextBlock->info.prev = newBlock;
      }

      // Update the block's size
      block->info.size = reqSize;

      // if block is head, new block become the new tail
      if (block == malloc_list_tail) {
        malloc_list_tail = newBlock;
      }
      
      // new block is free, so add to free list
      addToFreeList(newBlock);
    }

    // block is no longer free
    removeFromFreeList(block);
    return UNSCALED_POINTER_ADD(block, sizeof(BlockInfo));

  } else {
    // No free block found, request more space
    block = (Block*) requestMoreSpace(reqSize + sizeof(BlockInfo));
    if (!block) {
      return NULL; // Check if requestMoreSpace failed
    }

    // Update the block size
    block->info.size = reqSize;
    if (malloc_list_tail) {
      block->info.prev = malloc_list_tail;
    }

    // Update tail
    malloc_list_tail = block;
    return UNSCALED_POINTER_ADD(block, sizeof(BlockInfo));
  }
}

void coalesce(Block* block) {

  if (block == NULL || block->info.size > 0) {
    // Either the block is NULL or it's not free, no coalescing needed.
    return;
  }

  // totalSize is postive
  long int totalSize = -(block->info.size);

  Block* prevBlock = block->info.prev;
  Block* nextBlock = next_block(block);

  if (prevBlock && prevBlock->info.size < 0) {
    
    //Previous block is free, remove it from the free list and merge it
    removeFromFreeList(block);

    // Increase total size by the size of the previous block
    totalSize += (-(prevBlock->info.size) + sizeof(BlockInfo));

    if (nextBlock) {
      nextBlock->info.prev = prevBlock;
    }
      // Update the block pointer to the previous block
    if (block == malloc_list_tail) {
      malloc_list_tail = prevBlock;
    }
    block = prevBlock;
  }

  // Check if the next block is free

  if (nextBlock && nextBlock->info.size < 0) {

    // Next block is free, remove it from the free list and merge it
    removeFromFreeList(nextBlock);

    // Increase total size by the size of the next block
    totalSize += (-(nextBlock->info.size) + sizeof(BlockInfo));

    Block* nextNextBlock = next_block(nextBlock);

    // check if there is a block after next block
    if (nextNextBlock) {
      nextNextBlock->info.prev = block;
    } else if (nextBlock == malloc_list_tail) {
      malloc_list_tail = block;
    }
  }

  // Update the free list if necessary
  block->info.size = -totalSize;
  if (!free_list_head) {
    free_list_head = block;
  }
}

/* Free the block referenced by ptr. */
void mm_free(void* ptr) {
  Block* block = (Block*) UNSCALED_POINTER_SUB(ptr, sizeof(BlockInfo));
  block->info.size = -block->info.size;  // Mark the block as free

  addToFreeList(block);  
  coalesce(block);
}

// PROVIDED FUNCTIONS -----------------------------------------------
//
// You do not need to modify these, but they might be helpful to read
// over.

/* Get more heap space of exact size reqSize. */
void* requestMoreSpace(size_t reqSize) {
  void* ret = UNSCALED_POINTER_ADD(mem_heap_lo(), heap_size);
  heap_size += reqSize;

  void* mem_sbrk_result = mem_sbrk(reqSize);
  if ((size_t)mem_sbrk_result == -1) {
    printf("ERROR: mem_sbrk failed in requestMoreSpace\n");
    exit(0);
  }

  return ret;
}

/* Initialize the allocator. */
int mm_init() {
  free_list_head = NULL;
  malloc_list_tail = NULL;
  heap_size = 0;

  return 0;
}

/* Gets the first block in the heap or returns NULL if there is not one. */
Block* first_block() {
  Block* first = (Block*)mem_heap_lo();
  if (heap_size == 0) {
    return NULL;
  }

  return first;
}

/* Gets the adjacent block or returns NULL if there is not one. */
Block* next_block(Block* block) {
  size_t distance = (block->info.size > 0) ? block->info.size : -block->info.size;

  Block* end = (Block*)UNSCALED_POINTER_ADD(mem_heap_lo(), heap_size);
  Block* next = (Block*)UNSCALED_POINTER_ADD(block, sizeof(BlockInfo) + distance);
  if (next >= end) {
    return NULL;
  }

  return next;
}

/* Print the heap by iterating through it as an implicit free list. */
void examine_heap() {
  /* print to stderr so output isn't buffered and not output if we crash */
  Block* curr = (Block*)mem_heap_lo();
  Block* end = (Block*)UNSCALED_POINTER_ADD(mem_heap_lo(), heap_size);
  fprintf(stderr, "heap size:\t0x%lx\n", heap_size);
  fprintf(stderr, "heap start:\t%p\n", curr);
  fprintf(stderr, "heap end:\t%p\n", end);

  fprintf(stderr, "free_list_head: %p\n", (void*)free_list_head);

  fprintf(stderr, "malloc_list_tail: %p\n", (void*)malloc_list_tail);

  while(curr && curr < end) {
    /* print out common block attributes */
    fprintf(stderr, "%p: %ld\t", (void*)curr, curr->info.size);

    /* and allocated/free specific data */
    if (curr->info.size > 0) {
      fprintf(stderr, "ALLOCATED\tprev: %p\n", (void*)curr->info.prev);
    } else {
      fprintf(stderr, "FREE\tnextFree: %p, prevFree: %p, prev: %p\n", (void*)curr->freeNode.nextFree, (void*)curr->freeNode.prevFree, (void*)curr->info.prev);
    }

    curr = next_block(curr);
  }
  fprintf(stderr, "END OF HEAP\n\n");

  curr = free_list_head;
  fprintf(stderr, "Head ");
  while(curr) {
    fprintf(stderr, "-> %p ", curr);
    curr = curr->freeNode.nextFree;
  }
  fprintf(stderr, "\n");
}

/* Checks the heap data structure for consistency. */
int check_heap() {
  Block* curr = (Block*)mem_heap_lo();
  Block* end = (Block*)UNSCALED_POINTER_ADD(mem_heap_lo(), heap_size);
  Block* last = NULL;
  long int free_count = 0;

  while(curr && curr < end) {
    if (curr->info.prev != last) {
      fprintf(stderr, "check_heap: Error: previous link not correct.\n");
      examine_heap();
    }

    if (curr->info.size <= 0) {
      // Free
      free_count++;
    }

    last = curr;
    curr = next_block(curr);
  }

  curr = free_list_head;
  last = NULL;
  while(curr) {
    if (curr == last) {
      fprintf(stderr, "check_heap: Error: free list is circular.\n");
      examine_heap();
    }
    last = curr;
    curr = curr->freeNode.nextFree;
    if (free_count == 0) {
      fprintf(stderr, "check_heap: Error: free list has more items than expected.\n");
      examine_heap();
    }
    free_count--;
  }

  return 0;
}
