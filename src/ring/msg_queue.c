#include "ring/msg_queue.h"
#include "common_macro.h"

struct __msg_queue
{
	ring_buf_handle ring_buf_p;
};

typedef struct 
{
	int msg_size;
} msg_header;

msg_queue msg_queue_create(__in uint32_t buf_size)
{
	if (buf_size < 2)
	{
		return NULL;
	}
	msg_queue msg_queue_p = calloc(1, sizeof(struct __msg_queue));
	if (!msg_queue_p)
	{
		return NULL;
	}
	msg_queue_p->ring_buf_p = RingBuffer_create(buf_size);
	if (!msg_queue_p->ring_buf_p)
	{
		free(msg_queue_p);
		return NULL;
	}
	return msg_queue_p;
}

bool msg_queue_push(__in msg_queue msg_queue_p, __in const void* msg_p, __in const uint32_t msg_size)
{
	if (!msg_queue_p || !msg_p || msg_size == 0 || 
		RingBuffer_available_write(msg_queue_p->ring_buf_p) < sizeof(msg_header) + msg_size)
	{
		return false;
	}

	msg_header header = { .msg_size = msg_size };
	uint32_t write_bytes = RingBuffer_write(msg_queue_p->ring_buf_p, &header, sizeof(msg_header));
	ASSERT_ABORT(write_bytes ==  sizeof(msg_header));
	write_bytes = RingBuffer_write(msg_queue_p->ring_buf_p, msg_p, msg_size);
	ASSERT_ABORT(write_bytes == msg_size);
	return true;
}

uint32_t msg_queue_next_msg_size(__in msg_queue msg_queue_p)
{
	if (!msg_queue_p ||
		RingBuffer_available_write(msg_queue_p->ring_buf_p) < sizeof(msg_header) + 1)
	{
		return 0;
	}

	msg_header header = { .msg_size = 0 };
	uint32_t read_bytes = RingBuffer_peek(msg_queue_p->ring_buf_p, &header, sizeof(msg_header));
	ASSERT_ABORT(read_bytes == sizeof(msg_header));
	return header.msg_size;
}

bool msg_queue_pop(__in msg_queue msg_queue_p, __inout void* msg_p, __inout uint32_t* msg_size_p)
{
	uint32_t real_msg_size;
	if (!msg_queue_p || (real_msg_size = msg_queue_next_msg_size(msg_queue_p)) > *msg_size_p)
	{
		return false;
	}
	uint32_t read_bytes = RingBuffer_discard(msg_queue_p->ring_buf_p, sizeof(msg_header));
	ASSERT_ABORT(read_bytes == sizeof(msg_header));
	read_bytes = RingBuffer_read(msg_queue_p->ring_buf_p, msg_p, real_msg_size);
	ASSERT_ABORT(read_bytes == real_msg_size);
	return true;
}

void msg_queue_clear(__in msg_queue msg_queue_p)
{
	ASSERT_ABORT(msg_queue_p);
	RingBuffer_clear(msg_queue_p->ring_buf_p);
}

uint32_t msg_queue_available_pop_bytes(__in msg_queue msg_queue_p)
{
	ASSERT_ABORT(msg_queue_p);
	return RingBuffer_available_read(msg_queue_p->ring_buf_p);
}

uint32_t msg_queue_available_push_bytes(__in msg_queue msg_queue_p)
{
	ASSERT_ABORT(msg_queue_p);
	return RingBuffer_available_write(msg_queue_p->ring_buf_p);
}

bool msg_queue_is_empty(__in msg_queue msg_queue_p)
{
	ASSERT_ABORT(msg_queue_p);
	return RingBuffer_is_empty(msg_queue_p->ring_buf_p);
}

bool msg_queue_is_full(__in msg_queue msg_queue_p)
{
	ASSERT_ABORT(msg_queue_p);
	return RingBuffer_is_full(msg_queue_p->ring_buf_p);
}

void msg_queue_destroy(__in msg_queue* msg_queue_pp)
{
	if (!msg_queue_pp || !(*msg_queue_pp)->ring_buf_p)
	{
		return;
	}
	RingBuffer_destroy(&((*msg_queue_pp)->ring_buf_p));
	*msg_queue_pp = NULL;
}
