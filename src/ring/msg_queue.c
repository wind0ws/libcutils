#include "mem/mem_debug.h"
#include "ring/msg_queue.h"
#include "ring/ring_buffer.h"
#include "common_macro.h"

struct _msg_queue
{
	ring_buffer_handle ring_handle;
};

typedef struct
{
	uint32_t msg_size;
} msg_header_t;

msg_queue msg_queue_create(__in uint32_t buf_size)
{
	if (buf_size < (sizeof(msg_header_t) + 32U))
	{
		return NULL;
	}
	buf_size += 24U; // <-- 24 for ringbuffer internal struct size.
	const size_t expect_mem_size = sizeof(struct _msg_queue) + buf_size;
	char* raw_mem = (char*)malloc(expect_mem_size);
	if (!raw_mem)
	{
		return NULL;
	}
	msg_queue msg_queue_p = (msg_queue)raw_mem;
	msg_queue_p->ring_handle = ring_buffer_create_with_mem(raw_mem + sizeof(struct _msg_queue), buf_size);
	if (!msg_queue_p->ring_handle)
	{
		free(raw_mem);
		return NULL;
	}
	return msg_queue_p;
}

msg_q_code_e msg_queue_push(__in msg_queue msg_queue_p, __in const void* msg_p, __in const uint32_t msg_size)
{
	if (!msg_queue_p || !msg_p || 0 == msg_size)
	{
		return MSG_Q_CODE_NULL_HANDLE;
	}
	if (ring_buffer_available_write(msg_queue_p->ring_handle) < (sizeof(msg_header_t) + msg_size))
	{
		return MSG_Q_CODE_FULL;
	}

	msg_header_t header = { .msg_size = msg_size };
	uint32_t write_bytes = ring_buffer_write(msg_queue_p->ring_handle, &header, sizeof(msg_header_t));
	ASSERT_ABORT(write_bytes == sizeof(msg_header_t));
	write_bytes = ring_buffer_write(msg_queue_p->ring_handle, msg_p, msg_size);
	ASSERT_ABORT(write_bytes == msg_size);
	return MSG_Q_CODE_SUCCESS;
}

uint32_t msg_queue_next_msg_size(__in msg_queue msg_queue_p)
{
	if (!msg_queue_p ||
		ring_buffer_available_read(msg_queue_p->ring_handle) < sizeof(msg_header_t) + 1U)
	{
		return 0;
	}

	msg_header_t header = { 0 };
	uint32_t read_bytes = ring_buffer_peek(msg_queue_p->ring_handle, &header, sizeof(msg_header_t));
	ASSERT_ABORT(read_bytes == sizeof(msg_header_t));
	return header.msg_size;
}

__success(return == MSG_Q_CODE_SUCCESS)
msg_q_code_e msg_queue_pop(__in msg_queue msg_queue_p, __out void* msg_p, __inout uint32_t * msg_size_p)
{
	if (!msg_queue_p || !msg_p || !msg_size_p)
	{
		return MSG_Q_CODE_NULL_HANDLE;
	}
	const uint32_t available_read_bytes = ring_buffer_available_read(msg_queue_p->ring_handle);
	if (available_read_bytes < sizeof(msg_header_t) + 1)
	{
		return MSG_Q_CODE_EMPTY;
	}
	msg_header_t header = { 0 };
	uint32_t read_bytes = ring_buffer_peek(msg_queue_p->ring_handle, &header, sizeof(msg_header_t));
	ASSERT_ABORT(read_bytes == sizeof(msg_header_t));
	if (available_read_bytes < sizeof(header) + header.msg_size)
	{
		return MSG_Q_CODE_AGAIN; // msg buffer is not full copied to queue, maybe just write header.
	}
	if (header.msg_size > *msg_size_p)
	{
		*msg_size_p = header.msg_size; // tell caller the msg real size
		return MSG_Q_CODE_BUF_NOT_ENOUGH;
	}
#if 1
	read_bytes = ring_buffer_read_with_offset(msg_queue_p->ring_handle, (uint32_t)sizeof(msg_header_t), msg_p, header.msg_size);
#else
	read_bytes = ring_buffer_discard(msg_queue_p->ring_handle, sizeof(msg_header_t));
	ASSERT_ABORT(read_bytes == sizeof(msg_header_t));
	read_bytes = ring_buffer_read(msg_queue_p->ring_handle, msg_p, header.msg_size);
#endif
	ASSERT_ABORT(read_bytes == header.msg_size);
	return MSG_Q_CODE_SUCCESS;
}

void msg_queue_clear(__in msg_queue msg_queue_p)
{
	ASSERT_ABORT(msg_queue_p);
	ring_buffer_clear(msg_queue_p->ring_handle);
}

uint32_t msg_queue_available_pop_bytes(__in msg_queue msg_queue_p)
{
	ASSERT_ABORT(msg_queue_p);
	return ring_buffer_available_read(msg_queue_p->ring_handle);
}

uint32_t msg_queue_available_push_bytes(__in msg_queue msg_queue_p)
{
	ASSERT_ABORT(msg_queue_p);
	return ring_buffer_available_write(msg_queue_p->ring_handle);
}

void msg_queue_destroy(__inout msg_queue* msg_queue_pp)
{
	if (!msg_queue_pp || !((*msg_queue_pp)->ring_handle))
	{
		return;
	}
	ring_buffer_destroy(&((*msg_queue_pp)->ring_handle));
	free(*msg_queue_pp);
	*msg_queue_pp = NULL;
}
