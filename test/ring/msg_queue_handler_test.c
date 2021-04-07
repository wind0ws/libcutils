#include "ring/msg_queue_handler.h"
#include "common_macro.h"
#include "thread/thread_wrapper.h"
#include "log/xlog.h"

#define LOG_TAG "HDL_TEST"

typedef struct 
{
	msg_queue_handler handler;
}my_handler_t;

static void handler_cb(queue_msg_t* msg_p, void* user_data)
{
	TLOGD(LOG_TAG, "receive msg(what=%d, obj_len=%d): %s", msg_p->what, msg_p->obj_len, msg_p->obj);
}

static int run_msg_queue_handler_testcase()
{
	my_handler_t my_hdl = { 0 };
	my_hdl.handler = msg_queue_handler_create(4 * 1024, handler_cb, &my_hdl);
	ASSERT_ABORT(my_hdl.handler);
	queue_msg_t* msg_p = (queue_msg_t *)calloc(1, sizeof(queue_msg_t) + 2048);
	ASSERT_ABORT(msg_p);
	for (size_t i = 0;i < 256;++i)
	{
		msg_p->what = i;
		msg_p->obj_len = snprintf(msg_p->obj, 2048, "hello, I'm queue msg %zd", i) + 1;
		int status_send = 1;
		int retry_counter = 0;
		do 
		{
			status_send = msg_queue_handler_send(my_hdl.handler, msg_p);
			if (status_send == 4)
			{
				TLOGW(LOG_TAG, "full. %d sleeping", i);
				usleep(2000);
			}
		} while (status_send != 0 && retry_counter++ < 2);
		if (status_send)
		{
			TLOGE(LOG_TAG, "error(%d) on send msg. %d", status_send, i);
		}
	}
	free(msg_p);
	sleep(2);
	msg_queue_handler_destroy(&my_hdl.handler);
	return 0;
}

int msg_queue_handler_test()
{
	return run_msg_queue_handler_testcase();
}
