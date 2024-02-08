/*
 * Files: heap_workflow.c
 * ----------------------
 * Exercise a heap allocator on a simulated workflow of random requests.
 * Confirm contents of payload data, report if problem detected.
 * Track stats (throughput, utilization) and report summary.
 *
 * Written by jzelenski for CS107, circa 2015
 * Adapted for use in CS107e, Winter 2021
 */

#include <stdint.h>
#include <stdarg.h>
#include "assert.h"
#include "malloc.h"
#include "mango.h"
#include "printf.h"
#include "rand.h"
#include "strings.h"
#include "timer.h"
#include "uart.h"

#define ALIGNMENT 8

typedef enum { MALLOC = 0, FREE = 1 } op_t;

// track address, size for each allocated block
typedef struct {
    void *ptr;
    size_t size;
} block_t;

typedef struct {
    // track count of payload bytes (cur in-use, peak in-use, aggregate)
    size_t cur, peak, aggregate;
    int mticks, mcount; // malloc: elapsed ticks, op count
    int fticks, fcount; // free: elapsed ticks, op count
    void *seg_start, *seg_end; // sbrk start, end
    block_t *blocks;
    int nused, maxblocks;
} sim_t;

static void run_workflow(int num_inuse_blocks, int max_size, int num_ops);
static op_t choose_op(op_t last, int remaining, sim_t *sim);
static void checked_malloc(size_t requested_size, sim_t *sim);
static void checked_free(int index, sim_t *sim);
static void verify_block_address(block_t *block, sim_t *sim);
static void verify_payloads(sim_t *sim);
static void report_problem(const char* format, ...);

void main(void) {
    uart_init();

    const int max_blocks = 100;
    const int max_block_size = 256;
    const int num_ops = 5000;

    run_workflow(max_blocks, max_block_size, num_ops);
    printf("Completed %s\n", __FILE__);
}

/* Function: run_workflow
 * -----------------------
 * Run heap allocator on simulated workflow.
 * max_blocks is max number of simultaneous in-use blocks
 * max_size is max size of allocation request
 * num_ops is count of operations to run in sequence
 */
static void run_workflow(int max_blocks, int max_size, int num_ops) {
    block_t blocks[max_blocks];
    memset(blocks, 0, sizeof(blocks));

    sim_t sim = { 0 };
    sim.seg_start = sbrk(0);
    sim.blocks = blocks;
    sim.maxblocks = max_blocks;
    sim.nused = 0;
    op_t which = MALLOC;

    for (int i = 1; i <= num_ops; i++) {
        which = choose_op(which, num_ops - i, &sim);
        if (which == MALLOC) {
            int chosen_size = (rand() % max_size) + 1;
            checked_malloc(chosen_size, &sim);
        } else {
            int chosen_index = rand() % sim.nused;
            checked_free(chosen_index, &sim);
        }
        verify_payloads(&sim);
        if (i % 1000 == 0) printf ("TRACE:\t%d operations completed\n", i);
    }

    printf("\nAll requests serviced, no problems detected.\n");
    printf("Throughput:\n");
    printf("\t%d Kops/sec malloc \t(%d mallocs, %d total ticks)\n",
        sim.mcount*1000/sim.mticks, sim.mcount, sim.mticks);
    printf("\t%d Kops/sec free\t(%d frees, %d total ticks)\n",
        sim.fcount*1000/sim.fticks, sim.fcount, sim.fticks);

    size_t sbrk_size = (char *)sim.seg_end - (char *)sim.seg_start;
    printf("Utilization:\n");
    printf("\tmalloc'ed %ld total bytes, peak in-use %ld bytes, sbrk extended %ld bytes\n",
        sim.aggregate, sim.peak, sbrk_size);
    printf("\t%ld%% total/sbrk\t(over 100%% indicates recycling)\n", sim.aggregate*100/sbrk_size);
    printf("\t%ld%%  peak/sbrk\t(packing density, up to theoretical max of 100%%) \n", sim.peak*100/sbrk_size);
}

/* Function: choose_op
 * -------------------
 * Choose whether next op is malloc or free in semi-random manner.
 */
op_t choose_op(op_t last, int remaining, sim_t *sim) {
    if (sim->nused == 0) return MALLOC;     // no in use blocks, must malloc
    if (sim->nused == sim->maxblocks) return FREE; // all blocks in use, must free
    if (sim->nused >= remaining) return FREE; // free all in-use at end of simulation
    int next = rand() % 3 ? last : !last;   // lean toward repeat of lastmost op
    return next;
}

/* Function: checked_malloc
 * -------------------------
 * Service malloc request and time operation.
 * Confirm block returned is valid.
 * Fill payload data using low-order byte of size.
 * Update simulation stats.
 */
static void checked_malloc(size_t requested_size, sim_t *sim) {
    unsigned long start = timer_get_ticks();
    void *p = malloc(requested_size);
    unsigned long elapsed = timer_get_ticks() - start;
    sim->mticks += elapsed;
    sim->mcount++;

    block_t block = (block_t){ .ptr = p, .size = requested_size };
    // confirm validity of block returned by malloc
    verify_block_address(&block, sim);
    // memset payload data, later read to verify payload intact
    memset(block.ptr, block.size & 0xFF, block.size);
    sim->blocks[sim->nused++] = block;
    sim->cur += block.size;
    sim->aggregate += block.size;
    if (sim->cur > sim->peak) sim->peak = sim->cur;
}

/* Function: checked_free
 * ----------------------
 * Service free request and time operation.
 * Update simulation stats.
 */
static void checked_free(int index, sim_t *sim) {
    size_t old_size = sim->blocks[index].size;
    void *p = sim->blocks[index].ptr;

    unsigned long start = timer_get_ticks();
    free(p);
    unsigned long elapsed = timer_get_ticks() - start;
    sim->fticks += elapsed;
    sim->fcount++;

    sim->blocks[index] = sim->blocks[--sim->nused]; // replace with last, shrink array
    sim->cur -= old_size;
}

/* Function: verify_block_address
 * ------------------------------
 * Check block returned by malloc to confirm:
 *     block address is correctly aligned
 *     block address + size is within heap segment
 *     block address + size doesn't overlap any existing allocated block
 */
static void verify_block_address(block_t *block, sim_t *sim) {
    if (block->ptr == NULL && block->size == 0) return;

    // address must be ALIGNMENT-byte aligned
    if (((uintptr_t)block->ptr) % ALIGNMENT != 0) {
        report_problem("New block (%p) not aligned to %d bytes", block->ptr, ALIGNMENT);
    }
    // block must lie within the extent of the heap
    void *block_end = (char *)block->ptr + block->size;
    sim->seg_end = sbrk(0);
    if (block->ptr < sim->seg_start || block_end > sim->seg_end) {
        report_problem("New block (%p:%p) not within heap segment (%p:%p)",
                        block->ptr, block_end, sim->seg_start, sim->seg_end);
    }
    // block must not overlap any other blocks
    for (int i = 0; i < sim->nused; i++) {
        void *other_start = sim->blocks[i].ptr;
        void *other_end = (char *)other_start + sim->blocks[i].size;
        if ((block->ptr >= other_start && block->ptr < other_end) || (block_end > other_start && block_end < other_end) ||
            (block->ptr < other_start && block_end >= other_end)) {
            report_problem("New block (%p:%p) overlaps existing block (%p:%p)",
                            block->ptr, block_end, other_start, other_end);
        }
    }
}

/* Function: verify_payloads
 * -------------------------
 * Confirm payload contents are intact for all in-use blocks.
 */
static void verify_payloads(sim_t *sim){
    for (int i = 0; i < sim->nused; i++) {
        size_t size = sim->blocks[i].size;
        unsigned char byte = size & 0xFF, *cur = sim->blocks[i].ptr;
        for (size_t j = 0; j < size; j++, cur++) {
            if (*cur != byte) {
                report_problem("invalid payload data at offset %d of block at address %p", \
                    j, sim->blocks[i].ptr);
            }
        }
    }
}

/* Function: report_problem
 * ------------------------
 * Report when problem is detected using formatted error string.
 */
static void report_problem(const char* format, ...) {
    char buf[1024];
    va_list args;
    va_start(args, format);
    vsnprintf(buf, sizeof(buf), format, args);
    va_end(args);
    printf("\nPROBLEM DETECTED: %s\n", buf);
    mango_abort();
}
