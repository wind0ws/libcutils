#include "mem/mem_debug.h"
#include "ring/fixed_msg_queue.h"
#include "ring/ringbuffer.h"
#include <malloc.h>

struct _fixed_msg_queue_s 
{
    ring_buffer_handle ring_buf_p;
    uint32_t one_msg_byte_size;
};

fixed_msg_queue fixed_msg_queue_create(__in uint32_t one_msg_byte_size,
                                       __in uint32_t max_msg_capacity) 
{
    if (one_msg_byte_size < 1 || max_msg_capacity < 1)
    {
        return NULL;
    }
    const size_t expect_mem_size = sizeof(struct _fixed_msg_queue_s) + one_msg_byte_size * max_msg_capacity;
    char *raw_mem = (char *)malloc(expect_mem_size);
    if (!raw_mem)
    {
        return NULL;
    }
    fixed_msg_queue msg_queue_p = (fixed_msg_queue)raw_mem;
    msg_queue_p->one_msg_byte_size = one_msg_byte_size;
    msg_queue_p->ring_buf_p = RingBuffer_create_with_mem(raw_mem + sizeof(struct _fixed_msg_queue_s),
        (uint32_t)(expect_mem_size - sizeof(struct _fixed_msg_queue_s)));
    if (!msg_queue_p->ring_buf_p)
    {
        free(msg_queue_p);
        return NULL;
    }
    return msg_queue_p;
}

bool fixed_msg_queue_push(__in fixed_msg_queue msg_queue, __in const void *msg_p)
{
    if (fixed_msg_queue_is_full(msg_queue)) 
    {
        return false;
    }
    return RingBuffer_write(msg_queue->ring_buf_p, msg_p,
                            msg_queue->one_msg_byte_size) ==
           msg_queue->one_msg_byte_size;
}

bool fixed_msg_queue_pop(__in fixed_msg_queue msg_queue, __in void *msg_p)
{
    if (fixed_msg_queue_is_empty(msg_queue) || !msg_p) 
    {
        return false;
    }
    return RingBuffer_read(msg_queue->ring_buf_p, msg_p,
                           msg_queue->one_msg_byte_size) ==
           msg_queue->one_msg_byte_size;
}

extern inline void fixed_msg_queue_clear(__in fixed_msg_queue msg_queue) 
{
    RingBuffer_clear(msg_queue->ring_buf_p);
}

extern inline uint32_t fixed_msg_queue_available_pop_amount(__in fixed_msg_queue msg_queue) 
{
    return RingBuffer_available_read(msg_queue->ring_buf_p) /
           msg_queue->one_msg_byte_size;
}

extern inline uint32_t fixed_msg_queue_available_push_amount(__in fixed_msg_queue msg_queue) 
{
    return RingBuffer_available_write(msg_queue->ring_buf_p) /
           msg_queue->one_msg_byte_size;
}

extern inline bool fixed_msg_queue_is_empty(__in fixed_msg_queue msg_queue) 
{
    return 0 == fixed_msg_queue_available_pop_amount(msg_queue);
}

extern inline bool fixed_msg_queue_is_full(__in fixed_msg_queue msg_queue) 
{
    return 0 == fixed_msg_queue_available_push_amount(msg_queue);
}

void fixed_msg_queue_destroy(__inout fixed_msg_queue *msg_queue_p) 
{
    if (NULL == msg_queue_p || NULL == *msg_queue_p)
    {
        return;
    }
    fixed_msg_queue msg_queue = *msg_queue_p;
    RingBuffer_destroy(&(msg_queue->ring_buf_p));
    free(msg_queue);
    *msg_queue_p = NULL;
}
