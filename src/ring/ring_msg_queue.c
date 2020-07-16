#include <malloc.h>
#include "ring/ring_msg_queue.h"

struct __ring_msg_queue {
    ring_buf_handle ring_buf_p;
    uint32_t one_msg_byte_size;
};

ring_msg_queue RingMsgQueue_create(__in uint32_t one_msg_byte_size,
                                   __in uint32_t max_msg_capacity) {
    RING_LOGD("create ring msg queue. one_queue_msg_byte_size=%d, maxQueueMsgCapacity=%d",
         one_msg_byte_size, max_msg_capacity);
    ring_msg_queue msg_queue_p = calloc(1, sizeof(struct __ring_msg_queue));
    if (!msg_queue_p)
    {
        return NULL;
    }
    msg_queue_p->one_msg_byte_size = one_msg_byte_size;
    msg_queue_p->ring_buf_p = RingBuffer_create(one_msg_byte_size * max_msg_capacity);
    return msg_queue_p;
}

bool RingMsgQueue_push(__in ring_msg_queue ring_msg_queue_p, __in const void *msg_p) {
    if (RingMsgQueue_is_full(ring_msg_queue_p)) {
        return false;
    }
    return RingBuffer_write(ring_msg_queue_p->ring_buf_p, msg_p,
                            ring_msg_queue_p->one_msg_byte_size) ==
           ring_msg_queue_p->one_msg_byte_size;
}

bool RingMsgQueue_pop(__in ring_msg_queue ring_msg_queue_p, __in void *msg_p) {
    if (RingMsgQueue_is_empty(ring_msg_queue_p) || !msg_p) {
        return false;
    }
    return RingBuffer_read(ring_msg_queue_p->ring_buf_p, msg_p,
                           ring_msg_queue_p->one_msg_byte_size) ==
           ring_msg_queue_p->one_msg_byte_size;
}

extern inline void RingMsgQueue_clear(__in ring_msg_queue ring_msg_queue_p) {
    RingBuffer_clear(ring_msg_queue_p->ring_buf_p);
}

extern inline uint32_t RingMsgQueue_available_pop_msg_amount(__in ring_msg_queue ring_msg_queue_p) {
    return RingBuffer_available_read(ring_msg_queue_p->ring_buf_p) /
           ring_msg_queue_p->one_msg_byte_size;
}

extern inline uint32_t RingMsgQueue_available_push_msg_amount(__in ring_msg_queue ring_msg_queue_p) {
    return RingBuffer_available_write(ring_msg_queue_p->ring_buf_p) /
           ring_msg_queue_p->one_msg_byte_size;
}

extern inline bool RingMsgQueue_is_empty(__in ring_msg_queue ring_msg_queue_p) {
    return RingMsgQueue_available_pop_msg_amount(ring_msg_queue_p) == 0;
}

extern inline bool RingMsgQueue_is_full(__in ring_msg_queue ring_msg_queue_p) {
    return RingMsgQueue_available_push_msg_amount(ring_msg_queue_p) == 0;
}

void RingMsgQueue_destroy(__in ring_msg_queue *ring_msg_queue_pp) {
    if (NULL == ring_msg_queue_pp || NULL == *ring_msg_queue_pp)
    {
        return;
    }
    ring_msg_queue ring_msg_queue_p = *ring_msg_queue_pp;
    RingBuffer_destroy(&ring_msg_queue_p->ring_buf_p);
    free(ring_msg_queue_p);
    *ring_msg_queue_pp = NULL;
}
