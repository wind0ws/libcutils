#include <malloc.h>
#include "ring/fixed_msg_queue.h"
#include "ring/ringbuffer.h"

struct __fixed_msg_queue 
{
    ring_buf_handle ring_buf_p;
    uint32_t one_msg_byte_size;
};

fixed_msg_queue fixed_msg_queue_create(__in uint32_t one_msg_byte_size,
                                       __in uint32_t max_msg_capacity) 
{
    if (one_msg_byte_size < 1 || max_msg_capacity < 1)
    {
        return NULL;
    }
    size_t expect_mem_size = sizeof(struct __fixed_msg_queue) + one_msg_byte_size * max_msg_capacity;
    char *raw_mem = (char *)malloc(expect_mem_size);
    if (!raw_mem)
    {
        return NULL;
    }
    fixed_msg_queue msg_queue_p = (fixed_msg_queue)raw_mem;
    msg_queue_p->one_msg_byte_size = one_msg_byte_size;
    msg_queue_p->ring_buf_p = RingBuffer_create_with_mem(raw_mem + sizeof(struct __fixed_msg_queue),
        expect_mem_size - sizeof(struct __fixed_msg_queue));
    if (!msg_queue_p->ring_buf_p)
    {
        free(msg_queue_p);
        return NULL;
    }
    return msg_queue_p;
}

bool fixed_msg_queue_push(__in fixed_msg_queue fixed_msg_queue_p, __in const void *msg_p) 
{
    if (fixed_msg_queue_is_full(fixed_msg_queue_p)) 
    {
        return false;
    }
    return RingBuffer_write(fixed_msg_queue_p->ring_buf_p, msg_p,
                            fixed_msg_queue_p->one_msg_byte_size) ==
           fixed_msg_queue_p->one_msg_byte_size;
}

bool fixed_msg_queue_pop(__in fixed_msg_queue fixed_msg_queue_p, __in void *msg_p) 
{
    if (fixed_msg_queue_is_empty(fixed_msg_queue_p) || !msg_p) 
    {
        return false;
    }
    return RingBuffer_read(fixed_msg_queue_p->ring_buf_p, msg_p,
                           fixed_msg_queue_p->one_msg_byte_size) ==
           fixed_msg_queue_p->one_msg_byte_size;
}

extern inline void fixed_msg_queue_clear(__in fixed_msg_queue fixed_msg_queue_p) 
{
    RingBuffer_clear(fixed_msg_queue_p->ring_buf_p);
}

extern inline uint32_t fixed_msg_queue_available_pop_msg_amount(__in fixed_msg_queue fixed_msg_queue_p) 
{
    return RingBuffer_available_read(fixed_msg_queue_p->ring_buf_p) /
           fixed_msg_queue_p->one_msg_byte_size;
}

extern inline uint32_t fixed_msg_queue_available_push_msg_amount(__in fixed_msg_queue fixed_msg_queue_p) 
{
    return RingBuffer_available_write(fixed_msg_queue_p->ring_buf_p) /
           fixed_msg_queue_p->one_msg_byte_size;
}

extern inline bool fixed_msg_queue_is_empty(__in fixed_msg_queue fixed_msg_queue_p) 
{
    return fixed_msg_queue_available_pop_msg_amount(fixed_msg_queue_p) == 0;
}

extern inline bool fixed_msg_queue_is_full(__in fixed_msg_queue fixed_msg_queue_p) 
{
    return fixed_msg_queue_available_push_msg_amount(fixed_msg_queue_p) == 0;
}

void fixed_msg_queue_destroy(__inout fixed_msg_queue *fixed_msg_queue_pp) 
{
    if (NULL == fixed_msg_queue_pp || NULL == *fixed_msg_queue_pp)
    {
        return;
    }
    fixed_msg_queue fixed_msg_queue_p = *fixed_msg_queue_pp;
    RingBuffer_destroy(&fixed_msg_queue_p->ring_buf_p);
    free(fixed_msg_queue_p);
    *fixed_msg_queue_pp = NULL;
}
