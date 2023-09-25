#include "ring/msg_queue_handler.h"
#include "common_macro.h"
#include "thread/posix_thread.h"

#define LOG_TAG "HDL_TEST"
#include "log/logger.h"

typedef struct
{
	msg_queue_handler handler;
} my_handler_t;

static int handler_cb(queue_msg_t* msg_p, void* user_data)
{
	LOGD("receive msg(what=%d, obj_len=%d): %s", msg_p->what, msg_p->obj_len, msg_p->obj);
	return 0;
}

#define MSG_OBJ_MAX_SIZE  (2048)
static int run_msg_queue_handler_testcase()
{
	my_handler_t my_hdl = { 0 };
	my_hdl.handler = msg_queue_handler_create(4 * 1024, handler_cb, &my_hdl);
	ASSERT_ABORT(my_hdl.handler);

	queue_msg_t* msg_p = (queue_msg_t*)calloc(1, sizeof(queue_msg_t) + MSG_OBJ_MAX_SIZE);
	ASSERT_ABORT(msg_p);
	for (int i = 0; i < 512; ++i)
	{
		msg_p->what = i;
		msg_p->obj_len = snprintf(msg_p->obj, MSG_OBJ_MAX_SIZE, "hello, I'm queue msg %d", i) + 1;
		MSG_Q_CODE status_send;
		int retry_counter = 0;
		do
		{
			status_send = msg_queue_handler_push(my_hdl.handler, msg_p);
			if (MSG_Q_CODE_FULL == status_send)
			{
				LOGW("queue full. sleeping at %d", i);
				usleep(2000);
			}
		} while (status_send != 0 && retry_counter++ < 2);
		if (status_send)
		{
			LOGE("error(%d) on send msg. %d", status_send, i);
		}
	}
	free(msg_p);
	sleep(2); // give some time for handler to handle message. this is not necessary
	msg_queue_handler_destroy(&my_hdl.handler);
	return 0;
}

int msg_queue_handler_test()
{
	return run_msg_queue_handler_testcase();
}
