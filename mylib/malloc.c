/* File: malloc.c
 * --------------
 * Author: Javier Garcia Nieto <jgnieto@stanford.edu>
 * Simple recycling heap allocator for Mango Pi. Includes
 * malloc() and free() and a heap_dump() for debugging.
 */

#include "malloc.h"
#include "printf.h"
#include <stddef.h> // for NULL
#include "strings.h"
#include "backtrace.h"

/*
 * Data variables private to this module used to track
 * statistics for debugging/validate heap:
 *    count_allocs, count_frees, total_bytes_requested
 */
static int count_allocs, count_frees, total_bytes_requested;

#define HEADER_SIZE (sizeof(struct header))
#define FOOTER_SIZE (sizeof(struct footer))
#define OVERHEAD (HEADER_SIZE + FOOTER_SIZE)
#define ALIGNMENT 8

#define N_FRAMES 3

#define REDZONE_VALUE 0x666666

#define FREE   0
#define IN_USE 1

// Copied from: https://cs107e.github.io/assignments/assign4/block_headers/
struct header {
    unsigned int payload_size;
    unsigned int redzone_prefix;        // should be REDZONE_VALUE
};

struct footer {
    unsigned int redzone_suffix;        // should be REDZONE_VALUE
    unsigned int status;                // 0 if free, 1 if in use
    frame_t frames[N_FRAMES];           // 3 frames of backtrace
};

void memory_report(void);
void report_damaged_redzone(void *ptr);

/*
 * The segment of memory available for the heap runs from &__heap_start
 * to &__heap_max (symbols from memmap.ld establish these boundaries)
 *
 * The variable cur_head_end is initialized to &__heap_start and this
 * address is adjusted upward as in-use portion of heap segment
 * enlarges. Because cur_head_end is qualified as static, this variable
 * is not stored in stack frame, instead variable is located in data segment.
 * The one variable is shared by all and retains its value between calls.
 */
void *sbrk(size_t nbytes) {
    extern unsigned char __heap_start, __heap_max; // symbols in linker script memmap.ld
    static void *cur_heap_end =  &__heap_start;     // IMPORTANT: static

    void *new_heap_end = (char *)cur_heap_end + nbytes;
    if (new_heap_end > (void *)&__heap_max)    // if request would extend beyond heap max
        return NULL;                // reject
    void *prev_heap_end = cur_heap_end;
    cur_heap_end = new_heap_end;
    return prev_heap_end;
}

// Simple macro to round up x to multiple of n.
// Requires n be a power of 2.
#define roundup(x,n) (((x)+((n)-1))&(~((n)-1)))

#define next_header(hdr) ((struct header *)((char *)hdr + hdr->payload_size + OVERHEAD))
#define footer_ptr(hdr) ((struct footer *)((char *)hdr + hdr->payload_size + HEADER_SIZE))

/*
 * This malloc attempts to find a free block of memory that can fit nbytes
 * (aligned to ALIGNMENT) and return a pointer to it. It will create a block
 * with the remaining memory if it is large enough. If no such block is
 * found, it will expand the heap and return a pointer to the new block.
*/
void *malloc(size_t nbytes) {
    if (nbytes == 0) return NULL; // special case

    // keep stats
    count_allocs++;
    total_bytes_requested += nbytes;

    // round up to align bytes
    nbytes = roundup(nbytes, ALIGNMENT);

    // useful constants
    extern unsigned char __heap_start;
    const void *heap_end = sbrk(0);

    // find freed memory block that can fit our data
    struct header *candidate = (struct header *)&__heap_start;
    struct header *block_header = NULL;

    while ((void *)candidate < heap_end) {
        struct footer *candidate_footer = footer_ptr(candidate);

        if (candidate_footer->status == FREE && candidate->payload_size >= nbytes) {
            // found free block that will suit us, great!
            block_header = candidate;
            break;
        }

        // look at next block
        candidate = next_header(candidate);
    }

    if (block_header == NULL) {
        // didn't find free memory, expand heap
        block_header = (struct header *)sbrk(nbytes + OVERHEAD);

        if (block_header == NULL) return NULL; // if sbrk fails 

        // this is one of the cases we want to make payload_size = nbytes
        block_header->payload_size = nbytes;
    } else {
        // only take up the amount of memory we need, but only create new
        // free block if there are enough bytes for the header and the smallest
        // possible aligned block
        if (block_header->payload_size - nbytes > OVERHEAD + ALIGNMENT) {
            // create free block (cannot use macro because we add nbytes, special case)
            struct header *new_block_header = (struct header *)((char *)block_header + nbytes + OVERHEAD);
            new_block_header->payload_size = block_header->payload_size - OVERHEAD - nbytes;
            new_block_header->redzone_prefix = REDZONE_VALUE;

            struct footer *new_block_footer = footer_ptr(new_block_header);
            new_block_footer->status = FREE;
            new_block_footer->redzone_suffix = REDZONE_VALUE;

            // this is the other case we want to make payload_size = nbytes
            block_header->payload_size = nbytes;
        }
    }

    // payload_size is not updated here because we don't want to change it
    // if we are reusing a block but need to keep its original size because
    // we cannot fit a new one
    struct footer *block_footer = footer_ptr(block_header);
    block_footer->status = IN_USE;

    block_header->redzone_prefix = REDZONE_VALUE;
    block_footer->redzone_suffix = REDZONE_VALUE;

    // backtrace
    backtrace_gather_frames(block_footer->frames, N_FRAMES);

    // payload starts after header
    return block_header + 1;
}

/*
 * This free function will mark the block of memory pointed to by ptr as free.
 * It will also coalesce the block with any successive free blocks.
*/
void free(void *ptr) {
    if (ptr == NULL) return; // special case

    // update stats
    count_frees++;

    // useful constant
    const void *heap_end = sbrk(0);

    // subtract to find the header
    struct header *block_header = (struct header *)((char *)ptr - HEADER_SIZE);
    struct footer *block_footer = footer_ptr(block_header);

    // check if the redzones are damaged
    if (block_header->redzone_prefix != REDZONE_VALUE || block_footer->redzone_suffix != REDZONE_VALUE) {
        report_damaged_redzone(ptr);
        return;
    }

    // set block to free
    block_footer->status = FREE;

    // coalesce by merging with succeeding free blocks
    unsigned long coalesce_size = block_header->payload_size;
    struct header *coalesce_header = next_header(block_header);

    while ((void *)coalesce_header < heap_end) {
        struct footer *coalesce_footer = footer_ptr(coalesce_header);
        // when we find an in-use block we have to stop, as we cannot touch it
        if (coalesce_footer->status != FREE)
            break;
        
        coalesce_size += coalesce_header->payload_size + OVERHEAD;

        // look at next block
        coalesce_header = next_header(coalesce_header);
    }

    // set the payload_size to the corresponding size after finding nearby free
    // blocks. we do not need to remove the headers of the coalesced blocks since
    // memory is always assumed to contain garbage.
    block_header->payload_size = coalesce_size;
}

/*
 * Prints blocks of memory in a pretty format.
*/
void heap_dump(const char *label) {
    // useful constants
    extern unsigned char __heap_start;
    const void *heap_end = sbrk(0);

    // header
    printf("\n---------- HEAP DUMP (%s) ----------\n", label);
    printf("Heap segment at %p - %p\n", &__heap_start, heap_end);

    struct header *block_header = (struct header *)&__heap_start;;
    int count = 0;

    while ((void *)block_header < heap_end) {
        struct footer *block_footer = footer_ptr(block_header);

        // our format keeps the rows mostly aligned, nice!
        if (block_footer->status == IN_USE) {
            printf("#%d USED %p (%d bytes)\n", count, block_header + 1, block_header->payload_size);
        } else if (block_footer->status == FREE) {
            printf("#%d FREE %p (%d bytes)\n", count, block_header + 1, block_header->payload_size);
        } else {
            printf("#%d ???? %p (%d bytes)\n", count, block_header + 1, block_header->payload_size);
        }
        count++;
        block_header = next_header(block_header);
    }

    // footer
    printf("----------  END DUMP (%s) ----------\n", label);
    printf("Stats: %d in-use (%d allocs, %d frees), %d total bytes requested\n\n",
        count_allocs - count_frees, count_allocs, count_frees, total_bytes_requested);
}

void memory_report(void) {
    // useful constants
    extern unsigned char __heap_start;
    const void *heap_end = sbrk(0);

    printf("\n=============================================\n");
    printf(  "         Mini-Valgrind Memory Report         \n");
    printf(  "=============================================\n");
    printf("final stats: %d allocs, %d frees, %d total bytes requested\n\n", count_allocs, count_frees, total_bytes_requested);

    struct header *block_header = (struct header *)&__heap_start;
    int count_blocks = 0;
    int count_bytes = 0;

    while ((void *)block_header < heap_end) {
        struct footer *block_footer = footer_ptr(block_header);

        if (block_footer->status != FREE) {
            printf("%d bytes are lost, allocated by\n", block_header->payload_size);
            backtrace_print_frames(block_footer->frames, N_FRAMES);
                
            printf("\n");

            count_blocks += 1;
            count_bytes += block_header->payload_size;
        }
        block_header = next_header(block_header);
    }

    printf("Lost %d total bytes from %d blocks.\n", count_bytes, count_blocks);
}

void report_damaged_redzone(void *ptr) {
    struct header *block_header = (struct header *)((char *)ptr - HEADER_SIZE);
    struct footer *block_footer = footer_ptr(block_header);

    printf("\n=============================================\n");
    printf(  " **********  Mini-Valgrind Alert  ********** \n");
    printf(  "=============================================\n");
    printf("Attempt to free address %p that has damaged red zone(s): [%x] [%x]\n", ptr, block_header->redzone_prefix, block_footer->redzone_suffix);
    printf("Block of size %d bytes, allocated by\n", block_header->payload_size);
    backtrace_print_frames(block_footer->frames, N_FRAMES);
}
