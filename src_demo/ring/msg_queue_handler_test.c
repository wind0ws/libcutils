/*
 * 扩展测试覆盖：
 * 1. 普通消息处理 (未触发大消息逻辑)
 * 2. 大消息处理 (超过 threshold 触发外部内存分配与间接消息封装)
 * 3. 非优雅退出：销毁后剩余消息（尤其大消息）不再回调但外部内存应被释放
 * 4. 优雅退出：销毁时仍回调剩余消息
 * 5. 回调返回非 0 后进入 draining 模式（继续释放后续大消息内存，不再回调普通消息）
 */
#include "mem/mem_debug.h"
#include "ring/msg_queue_handler.h"
#include "common_macro.h"
#include "thread/portable_thread.h"

#define LOG_TAG "HANDLER_TEST"
#include "log/logger.h"

#define MSG_OBJ_MAX_SIZE (4096)

typedef struct
{
	msg_queue_handler handler;
	int received_total;		/* 收到的总消息数 */
	int received_big;		/* 收到的大消息（根据 obj_len >= threshold 判定） */
	int expected_total;		/* 预期发送的总消息数 */
	int expected_big;		/* 预期发送的大消息数 */
	int callback_errors;	/* 回调主动返回非 0 次数 */
	int trigger_error_what; /* 当 what == 该值时制造一次错误返回 */
	int drain_mode_entered; /* 回调错误后进入退出阶段标志(推断) */
	size_t threshold;		/* 大消息阈值 */
} test_ctx_t;

static void pri_on_msg_q_handler_status_changed(msg_q_handler_status_e status, void *user_data)
{
	LOGI("detected handler status: %d", status);
}

static int pri_handle_queue_msg(queue_msg_t *msg_p, void *user_data)
{
	test_ctx_t *ctx = (test_ctx_t *)user_data;
	++ctx->received_total;
	if (msg_p->obj_len >= (int)ctx->threshold)
	{
		++ctx->received_big;
	}
	if (ctx->trigger_error_what == msg_p->what && 0 == ctx->drain_mode_entered)
	{
		++ctx->callback_errors;
		ctx->drain_mode_entered = 1; /* 标记进入 draining 模式 */
		LOGD("deliberately inject error on msg what=%d to trigger draining mode", msg_p->what);
		return -1; /* 制造错误 */
	}
	LOGD("received msg(what=%d, obj_len=%d): %s", msg_p->what, msg_p->obj_len, msg_p->obj);
	return 0;
}

/* 发送一批混合消息：一部分普通，一部分大消息 */
static void pri_send_mixed_messages(test_ctx_t *ctx, int total_msgs)
{
	queue_msg_t *msg_buf = (queue_msg_t *)calloc(1, sizeof(queue_msg_t) + MSG_OBJ_MAX_SIZE);
	ASSERT_ABORT(msg_buf);

	ctx->expected_total = total_msgs;
	ctx->expected_big = 0;
	for (int i = 0; i < total_msgs; ++i)
	{
		msg_buf->what = i;
		bool make_big = (i % 7 == 0);									/* 每 7 条做一条大消息 */
		int payload_size = make_big ? (int)(ctx->threshold + 128) : 64; /* 大消息略高于阈值 */
		if (make_big)
		{
			++ctx->expected_big;
		}
		msg_buf->obj_len = payload_size;
		memset(msg_buf->obj, 'A' + (i % 26), payload_size - 1);
		msg_buf->obj[payload_size - 1] = '\0';
		msg_q_code_e status_send;
		do
		{
			if (MSG_Q_CODE_SUCCESS == (status_send = msg_queue_handler_push(ctx->handler, msg_buf)))
			{
				break;
			}
			if (MSG_Q_CODE_FULL != status_send)
			{
				LOGE_TRACE("push failed(status=%d, what=%d)", status_send, i);
				break;
			}
			usleep(1000);
		} while (MSG_Q_CODE_FULL == status_send);
	}
	free(msg_buf);
}

static int run_case_normal_and_big(bool graceful)
{
	test_ctx_t ctx = {0};
	ctx.trigger_error_what = 2000; /* 不触发错误 */
	ctx.threshold = 512;		   /* 设置阈值触发大消息路径 */
	msg_queue_handler_init_param_t init_param =
		{
			.callback =
				{
					.user_data = &ctx,
					.fn_handle_msg = pri_handle_queue_msg,
					.fn_on_status_changed = pri_on_msg_q_handler_status_changed,
				},
			.cfg =
				{
					.threshold_mem_size_for_alloc_obj = (uint32_t)ctx.threshold,
				},
		};
	ctx.handler = msg_queue_handler_create(8U * 1024U, &init_param);
	ASSERT_ABORT(ctx.handler);

	pri_send_mixed_messages(&ctx, 200); /* 发送 200 条 */
	/* 非优雅退出立刻销毁，优雅退出给一点处理时间 */
	if (graceful)
	{
		/* 给线程一些时间处理 （也可以不睡，优雅销毁会继续回调剩余消息）*/
		usleep(50 * 1000);
		msg_queue_handler_destroy(&ctx.handler, MSG_Q_HANDLER_DESTROY_FLAGS_GRACEFULLY);
	}
	else
	{
		msg_queue_handler_destroy(&ctx.handler, MSG_Q_HANDLER_DESTROY_FLAGS_NORMALLY);
	}

	LOGI("case(normal+big, graceful=%d): received_total=%d expected_total=%d received_big=%d expected_big=%d",
		 graceful, ctx.received_total, ctx.expected_total, ctx.received_big, ctx.expected_big);
	if (graceful)
	{
		ASSERT_ABORT(ctx.received_total == ctx.expected_total); /* 优雅退出应全部处理 */
		ASSERT_ABORT(ctx.received_big == ctx.expected_big);
	}
	else
	{
		ASSERT_ABORT(ctx.received_total <= ctx.expected_total); /* 非优雅可能少于全部 */
	}
	return 0;
}

static int run_case_draining_on_error()
{
	test_ctx_t ctx = {0};
	ctx.threshold = 256;
	ctx.trigger_error_what = 25; /* 第一次遇到 what=25 时让回调返回错误，转入 draining */
	msg_queue_handler_init_param_t init_param =
		{
			.callback =
				{
					.user_data = &ctx,
					.fn_handle_msg = pri_handle_queue_msg,
					.fn_on_status_changed = pri_on_msg_q_handler_status_changed,
				},
			.cfg =
				{
					.threshold_mem_size_for_alloc_obj = (uint32_t)ctx.threshold,
				},
		};
	ctx.handler = msg_queue_handler_create(8U * 1024U, &init_param);
	ASSERT_ABORT(ctx.handler);

	pri_send_mixed_messages(&ctx, 120);
	/* 销毁前稍等，使得错误触发并进入 drain 阶段 */
	usleep(200 * 1000);
	msg_queue_handler_destroy(&ctx.handler, MSG_Q_HANDLER_DESTROY_FLAGS_NORMALLY); /* 非优雅：drain 阶段不再回调普通消息 */

	LOGI("case(draining_on_error): received_total=%d callback_errors=%d drain_mode_entered=%d",
		 ctx.received_total, ctx.callback_errors, ctx.drain_mode_entered);
	ASSERT_ABORT(ctx.callback_errors == 1); /* 只应触发一次错误 */
	ASSERT_ABORT(ctx.drain_mode_entered == 1);
	/* 由于进入 draining，后续普通消息不再回调，因此收到的总数应 < 发送数 */
	ASSERT_ABORT(ctx.received_total < ctx.expected_total);
	return 0;
}

int msg_queue_handler_test(void)
{
	LOGI("---- run msg_queue_handler tests ----");
	int ret = 0;
	ret |= run_case_normal_and_big(false);
	ret |= run_case_normal_and_big(true);
	ret |= run_case_draining_on_error();
	LOGI("---- all tests done (ret=%d) ----", ret);
	return ret;
}

#include "lcu_test_registry.h"
LCU_TEST_REGISTER(msg_queue_handler_test, "test msg queue handler");
