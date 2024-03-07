#ifndef RINGBUFFER_PTR_H
#define RINGBUFFER_PTR_H

/*
 * Adaptation of ringbuffer.h to store pointers instead of integers.
 * Pointers update by Javier Garcia Nieto <jgnieto@stanford.edu>
 * --------
 * This module defines a ring buffer data structure that provides
 * a fixed-length FIFO (first-in-first-out) queue of uintptr_t elements.
 *
 * The queue is designed to allow concurrent access by 1 reader (rb_ptr_dequeue)
 * and 1 writer (rb_ptr_enqueue). The writer is typically the interrupt handler,
 * which is enqueuing data to be dequeued by the main program, the reader.
 *
 * Author: Philip Levis <pal@cs.stanford.edu>
 * Author: Julie Zelenski <zelenski@cs.stanford.edu>
 */

#include <stdbool.h>
#include <stdint.h>

/*
 * Type: `rb_ptr_t`
 *
 * This struct holds the information for a ring buffer.
 */
typedef struct ringbuffer_ptr rb_ptr_t;

/*
 * `rb_ptr_clear_free`
 * 
 * Added by Javier Garcia Nieto <jgnieto@stanford.edu>
 *
 * Clear ring buffer and free all pointers stored within.
 *
 * @param rb    the ring buffer to clear and free
 */
void rb_ptr_clear_free(rb_ptr_t *rb);

/*
 * `rb_ptr_new`
 *
 * Initializes a new empty ring buffer and returns a pointer to it.
 *
 * @return      pointer to new ring buffer or NULL if failed to create
 *
 * To set up a ring buffer in your code:
 *
 *     rb_ptr_t *rb = rb_new();
 */
rb_ptr_t *rb_ptr_new(void);

/*
 * `rb_ptr_empty`
 *
 * Check if a ring buffer is currently empty.
 *
 * @param rb    the ring buffer to check
 * @return      true if rb is empty, false otherwise
 */
bool rb_ptr_empty(rb_ptr_t *rb);

/*
 *  `rb_ptr_full`
 *
 * Check if a ring buffer is currently full. When full, existing
 * elements must first be dequeued before further elements can
 * be enqueued.
 *
 * @param rb    the ring buffer to check
 * @return      true if rb is full, false otherwise
 */
bool rb_ptr_full(rb_ptr_t *rb);

/*
 * `rb_ptr_enqueue`
 *
 * Add an element to the back of a ring buffer. If the ring buffer
 * is full, no changes are made and false is returned.
 *
 * @param rb    the ring buffer to enqueue to
 * @param elem  the element to enqueue
 * @return      true if elem was successfully enqueued, false otherwise
 */
bool rb_ptr_enqueue(rb_ptr_t *rb, uintptr_t elem);

/*
 * `rb_ptr_dequeue`
 *
 * If the ring buffer is not empty, remove frontmost element,
 * store into *p_elem, and return true.  p_elem should be the address
 * of a valid memory location into which to store the dequeued value.
 * If the ring buffer is empty, no changes are made to either the ring
 * buffer or *p_elem and the return value is false.
 *
 * @param rb        the ring buffer to dequeue from
 * @param p_elem    address at which to store the dequeued element
 * @return          true if an element was written to *p_elem, false otherwise
 */
bool rb_ptr_dequeue(rb_ptr_t *rb, uintptr_t *p_elem);

#endif
