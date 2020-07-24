#include "ring/autocover_buffer.h"
#include "common_macro.h"
#include "log/xlog.h"
#include "thread/thread_wrapper.h"
#include <string.h>

typedef struct 
{
	auto_cover_buf_handle cover_buf_p;
	pthread_mutex_t mutex_coverbuf;
	uint32_t data_in_counter;
	bool exit_flag;
}cover_case_data;

static void* thread_consumer(void* param)
{
	LOGD("consumer thread in")
	cover_case_data* data_p = (cover_case_data*)param;
	char buffer[12];
	while (!(data_p->exit_flag))
	{
		if (data_p->data_in_counter < 12)
		{
			Sleep(80);
			continue;
		}
		memset(buffer, 0, 12);
		uint32_t read_pos = data_p->data_in_counter - 12;
		if (auto_cover_buf_read(data_p->cover_buf_p, read_pos, buffer, 12) ==  12)
		{
			if (buffer[0] != 0x01 || buffer[11] != 0x04)
			{
				LOGE("!!! read error !!! read_pos=%u", read_pos);
			}
			else
			{
				LOGI("~~~ read succeed! ~~~ read_pos=%u", read_pos);
			}
			Sleep(150);
		}
		else
		{
			LOGD("not able read. read_pos=%u", read_pos);
			Sleep(80);
		}
	}
	LOGD("consumer thread out")
	return NULL;
}

static void* thread_producer(void* param)
{
	LOGD("producer thread in")
	cover_case_data* data_p = (cover_case_data*)param;
	char buffer[4] = { 0x01, 0x02, 0x03, 0x04};
	while (!(data_p->exit_flag))
	{
		if (auto_cover_buf_write(data_p->cover_buf_p, buffer, 4) == 4)
		{
			data_p->data_in_counter += 4;
		}
		else
		{
			LOGE("failed on write!!! shouldn't reach here!!!");
		}
		Sleep(100);
	}
	LOGD("producer thread out")
	return NULL;
}

static int autocover_buf_lock(void* user_data)
{
	cover_case_data* data_p = (cover_case_data*)user_data;
	return pthread_mutex_lock(&data_p->mutex_coverbuf);
}

static int autocover_buf_unlock(void* user_data)
{
	cover_case_data* data_p = (cover_case_data*)user_data;
	return pthread_mutex_unlock(&data_p->mutex_coverbuf);
}

int autocover_buffer_test()
{
	LOGD("now test autocover_buffer!");
	cover_case_data case_data = { 0 };
	case_data.data_in_counter = 0;
	case_data.exit_flag = false;
	pthread_mutex_init(&case_data.mutex_coverbuf, NULL);
	auto_cover_buf_lock_t buf_lock = {
	.acquire = autocover_buf_lock,
	.release = autocover_buf_unlock,
	.arg = &case_data
	};
	case_data.cover_buf_p = auto_cover_buf_create(16, &buf_lock);
	ASSERT(case_data.cover_buf_p);

	pthread_t pthread_consumer;
	pthread_t pthread_producer;
	pthread_create(&pthread_consumer, NULL, thread_consumer, &case_data);
	pthread_create(&pthread_producer, NULL, thread_producer, &case_data);

	sleep(5);
	case_data.exit_flag = true;
	pthread_join(pthread_producer, NULL);
	pthread_join(pthread_consumer, NULL);
	pthread_mutex_destroy(&case_data.mutex_coverbuf);

	auto_cover_buf_destroy(&case_data.cover_buf_p);
	LOGD("autocover_buffer test finished.");
	return 0;
}