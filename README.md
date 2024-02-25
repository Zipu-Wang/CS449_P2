# CS449_P2
Luis OliveiraGeneral InfoSyllabusScheduleResourcesFor FunLabsProjects
Project 2 - Malloc: Memory allocators

Here, we will use our power over pointers to implement a linked list memory allocator.

Warning
This project is known for creating massive office hours lines. Make sure you start early to avoid them.

Also…

Note: DEFINITELY start early enough to get through the more troublesome parts of debugging this one!

The beginning of this project, often leads to small mistakes that result in segmentation faults. Statistically, you will need help! So START EARLY.

Introduction
In this assignment, you will implement a basic memory allocator much like the one discussed in lecture. We will provide you with much of the boilerplate code to start from.

This time we cannot use malloc and free to manage the data for our memory allocator. We are implementing them ourselves!!

In this case, the two functions you will implement are mm_malloc and mm_free and you will implement them in three phases. The first phase is the most naive implementation which will be a bit inefficient. We will then improve this implementation to gain better performance.

Downloading
To get the boilerplate code and the testing harness for this assignment, use the following command on thoth:

wget  https://cs0449.gitlab.io/sp2024/projects/02/malloclab-handout.zip 
This will download the malloclab-handout.zip file to your (private) directory on the thoth Linux machine (make sure you are logged into this!) in which you will do your work.

Then issue the command:

unzip malloclab-handout.zip
This will cause a number of files to be unpacked in the directory malloclab-handout. Navigate to that directory and look at the files. The file you will be modifying is mm.c. The mm.c file contains a skeleton for the functions that interact with the data structures that manage the state of memory. It also contains the definition of the data structures we will use to represent the blocks on the heap. The mm.h file contains just the function prototypes.

Consult the README file for a more thorough description of the files and how to use them.

Implementation
The mm.c file contains declarations which define the following structures, which you do not need to modify:

typedef struct _BlockInfo {
  // Size of the block and whether or not the block is in use or free.
  // When the size is negative, the block is currently free.
  long int size;
  // Pointer to the previous block in the list.
  struct _Block* prev;
} BlockInfo;

typedef struct _FreeBlockInfo {
  // Pointer to the next free block in the list.
  struct _Block* nextFree;
  // Pointer to the previous free block in the list.
  struct _Block* prevFree;
} FreeBlockInfo;

typedef struct _Block {
  BlockInfo info;
  FreeBlockInfo freeNode;
} Block;

/* Pointer to the first FreeBlockInfo in the free list, the list's head. */
static Block* free_list_head = NULL;
static Block* malloc_list_tail = NULL;
The important structure that you will use is the Block structure. This contains the metadata for your block.

The info field of the structure is the metadata that is a part of every block on the heap, free and allocated alike. The freeNode field is metadata that only a free block has.

As a space optimization, you will allow the user program to write over the freeNode information. In memory, the info struct comes before the freeNode section in memory. If you placed the Block structure in memory, then if you add sizeof(BlockInfo) to a pointer to a Block structure, it will be the address of the FreeBlockInfo structure. It is this address that should be returned by mm_malloc to make the best use of space.

You can ignore pointer arithmetic and add directly to a pointer using the UNSCALED_POINTER_ADD macro we provided.

In this case, if you have a pointer to a Block called ptrFreeBlock and you want mm_malloc to return the first address that a program can safely write to, you can write:

return UNSCALED_POINTER_ADD(ptrFreeBlock, sizeof(BlockInfo))
Free Blocks
In this assignment, we are going to treat free blocks as being any Block that has a negative size. An allocated block, then, is a Block that has a positive size.

Here is some code that will determine if a Block is free:

Block* ptrBlock = first_block();

if (ptrBlock->info.size < 0) {
    // Free Block
}
Notice the ptrBlock->info arrow syntax dereferencing the pointer to a Block structure. Yet, since the info field is not a pointer, we directly refer to the fields within. Therefore, info.size uses a dot syntax. Funky!

Your turn
Your task is to modify both the mm.c file to fully implement the following functions:

mm_malloc - Allocate a block of memory of the given size.
mm_free - Deallocate the given pointer that was previously allocated by mm_malloc.
In the process of implementing these, you should find implementing these helper functions useful:

coalesce - Given a free block, coalesce with its neighbors if they are also free.
searchList - Look for a free block that can fit the given amount of space exhaustively.
searchFreeList - Look for a free block that can fit the given amount of space by using the faster free list data structure.
We have provided quite a few helpful functions that will help you:

requestMoreSpace - Will expand the heap by the given number of bytes.
mm_init - This will reset the heap. You should not modify this.
first_block - Will return a pointer to the first Block structure on the heap. It just assumes there is a Block structure written to the first bytes of the heap.
next_block - When given a Block pointer, it will use the metadata to give you the next Block in memory. This assumes a properly managed heap where each Block is directly after the last one.
examine_heap - Prints out the state of the heap assuming it is properly formed.
check_heap - Checks your heap data structures for improper state. Can be useful for early debugging efforts.
Refer to the comments within mm.c for any guidance and rules for special cases for each function. The implementations of provided functions can help you determine how to make use of other functions and how to properly read through your heap.

Phase 1 - By end of week 1
In order to lessen the burden, it is recommended to implement this assignment in phases. The first phase will be our least performant implementation. It will be very naive and not use an explicit free list. That is, we only use the BlockInfo metadata (size and prev) and ignore FreeBlockInfo altogether. Do not worry about the free list just yet.

Also, do not worry about splitBlock or coalesce either. Not doing these makes the memory manager very inefficient, but it will still give a correct answer.

For this, when you allocate you only need to exhaustively find a block using searchList and then, if you cannot find one, allocate more space to create a free block of the exact size you need.

When you have such a block, you can then allocate it (adjust the size field such that it is considered ‘allocated’).

Your mm_free implementation can just… mark it free again. How would we do that?

Phase 2 - By middle of week 2
We are currently MASSEVELY overallocating memory. So now, if we find that the block is large enough to split into two, then you should do so.

It should be helpful to maintain malloc_list_tail to always point to the Block at the end of the heap (largest address.) You only need to worry about free_list_head in phase 3.

At this point, you should have a malloc implementation that passes all the tests, but performs just miserably. Let’s improve it, now.

Implement coalesce() such that adjacent free blocks are merged when mm_free is called.

Remember: When you split a block, it needs enough space for all of the metadata!

Remember: When you merge a block, the final block gains not only that block’s free space, but also the space taken up by the metadata!

Phase 3 - By deadline
Adding coalesce should still result in a slightly better performing mm_malloc implementation, and it should still pass all of its tests.

When you have a working implementation, we can add the final step: the free list.

The free list means making use of the FreeBlockInfo structure that is part of every Block. This contains a nextFree and prevFree field. With this, we can implement a single-ended doubly linked list to maintain a list of only the free blocks.

In your mm_malloc, you will need to remove the node from the free list and also carefully ensure the free block from a split is in the list.

In your mm_free, you will need to add a node to the head of the free list. When you coalesce, you will need to ensure that the logic of the linked list remains intact.

In the end, you should see a vast improvement in performance.

Note/Hint: These nodes do not have to be in any particular order.

Note2: write functions to insert and remove nodes from the list!! :)

Testing
We have graciously provided a Makefile for this lab. As such, you can compile your work as so:

make
If there are no errors, the compiler will generate several programs. One program is mdriver which will test your implementation against some exhaustive program traces. Documentation on commands can be seen by running the mdriver with the -h flag:

./mdriver -h
The following file (traces/short1-bal.rep) illustrates an example trace:

20000
6
12
1
a 0 2040
a 1 2040
f 1
a 2 48
a 3 4072
f 3
a 4 4072
f 0
f 2
a 5 4072
f 4
f 5
You can mostly ignore the first few lines. Lines that start with a will call mm_malloc with the size passed being the last number of that line. The lines starting with f will call mm_free with the address returned by the corresponding mm_malloc call. That is, the prior a line of the trace with the same identifying number (the second token on every a or f line).

You can write your own trace by copying an existing one. To use any of the provided traces, just use mdriver like this:

./mdriver -f traces/short1-bal.rep
If there are no errors, it will print only one line which shows the relative performance of your implementation.

You can run all traces using:

./mdriver
Or verbosely using:

./mdriver -v
Generally, with the initial phase, you will be in the 40s running all traces. Your final implementation in the final phase should be in the high 60s when running all traces. It may deviate due to the server load.

Evaluation
Your program will be evaluated using the thirteen traces described above found in the traces directory of your handout package.

Submission
You only need to upload a single file mm.c to Gradescope for this assignment. You may submit as many times as you like until the due date.

Reflection
It is good to start this assignment early. C, like many things in life, takes practice. This assignment stresses the importance of careful and deliberate programming. Write short statements, do less on each line, and comment your code very liberally.

If you find yourself frustrated, take an hour away from it. Come back to it with a fresh point of view. Read the code over and what it does. Remember that the computer is going to do exactly what it is told, so do not read back what you think you wrote. Read what is actually there and if that is what you intended.

It may help to draw out a diagram of the data structure as you trace your own code. You might find yourself, as I did, going,

“Ah, right, I can’t assign cur = cur->next after I have done cur->next = last! I need to keep that pointer in a temp variable somewhere.”

Mastery does not mean you always get it right on the first try.

CS 449
CS 449
loliveira@pitt.edu
 luisfnqoliveira
CS 449;
