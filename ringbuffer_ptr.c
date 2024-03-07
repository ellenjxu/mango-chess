/*
 * Implementation of ringbuffer_ptr module.
 * Adapted from ringbuffer by Javier Garcia Nieto <jgnieto@stanford.edu>
 *
 * The ring buffer data structure allows lock-free concurrent
 * access by one reader and one writer.
 *
 * Author: Philip Levis <pal@cs.stanford.edu>
 *         Julie Zelenski <zelenski@cs.stanford.edu>
 */

#include "ringbuffer_ptr.h"
#include "assert.h"
#include "malloc.h"
#include <stdint.h>

#define LENGTH 512

struct ringbuffer_ptr {
    uintptr_t entries[LENGTH];
    int head, tail;
};


/*
 * Added by Javier Garcia Nieto <jgnieto@stanford.edu>
 */
void rb_ptr_clear_free(rb_ptr_t *rb) {
    while (!rb_ptr_empty(rb)) {
        uintptr_t elem;
        rb_ptr_dequeue(rb, &elem);
        free((void *)elem);
    }
}

// Below is the original code from ringbuffer.c, replacing int with uintptr_t
rb_ptr_t *rb_ptr_new(void) {
    rb_ptr_t *rb = malloc(sizeof(struct ringbuffer_ptr));
    assert(rb != NULL);
    rb->head = rb->tail = 0;
    return rb;
}

bool rb_ptr_empty(rb_ptr_t *rb) {
    assert(rb != NULL);
    return rb->head == rb->tail;
}

bool rb_ptr_full(rb_ptr_t *rb) {
    assert(rb != NULL);
    return (rb->tail + 1) % LENGTH == rb->head;
}

/*
 * Note: enqueue is called by writer. enqueue advances rb->tail,
 * no changes to rb->head.  This design allows safe concurrent access.
 */
bool rb_ptr_enqueue(rb_ptr_t *rb, uintptr_t elem) {
    assert(rb != NULL);
    if (rb_ptr_full(rb)) {
        return false;
    }

    rb->entries[rb->tail] = elem;
    rb->tail = (rb->tail + 1) % LENGTH;
    return true;
}

/*
 * Note: dequeue is called by reader. dequeue advances rb->head,
 * no changes to rb->tail. This design allows safe concurrent access.
 */
bool rb_ptr_dequeue(rb_ptr_t *rb, uintptr_t *p_elem) {
    assert(rb != NULL && p_elem != NULL);
    if (rb_ptr_empty(rb)) {
        return false;
    }

    *p_elem = rb->entries[rb->head];
    rb->head = (rb->head + 1) % LENGTH;
    return true;
}
