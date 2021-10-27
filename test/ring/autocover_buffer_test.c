#include "ring/autocover_buffer.h"
#include "common_macro.h"
#include "log/xlog.h"
#include "thread/thread_wrapper.h"
#include <string.h>

typedef struct 
{
	auto_cover_buf_handle cover_buf_p;
	pthread_mutex_t mutex_coverbuf;
	volatile uint32_t data_in_counter;
	bool exit_flag;
} cover_case_data;

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

static void* thread_consumer(void* param)
{
	LOGD("--> consumer thread in");
	cover_case_data* data_p = (cover_case_data*)param;
	char buffer[12];
	while (!(data_p->exit_flag))
	{
		if (data_p->data_in_counter < sizeof(buffer))
		{
			Sleep(1);
			continue;
		}
		memset(buffer, 0, sizeof(buffer));
		autocover_buf_lock(param);
		uint32_t read_pos = data_p->data_in_counter - sizeof(buffer);
		/*if (read_pos < 512)
		{
			LOGI("now read_pos=%u", read_pos);
		}*/
		if (auto_cover_buf_read(data_p->cover_buf_p, read_pos, buffer, sizeof(buffer)) == sizeof(buffer))
		{
			if (buffer[0] != 0x01 || buffer[11] != 0x04)
			{
				LOGE("!!! read error !!! read_pos=%u", read_pos);
			}
			else
			{
				//LOGI(" *** read succeed! --> read_pos=%u", read_pos);
			}
			//Sleep(1);
		}
		else
		{
			//LOGE("not able read. read_pos=%u", read_pos);
			//Sleep(1);
		}
		autocover_buf_unlock(param);
	}
	LOGD("<-- consumer thread out. counter=%u", data_p->data_in_counter);
	return NULL;
}

static void* thread_producer(void* param)
{
	LOGD("--> producer thread in");
	cover_case_data* data_p = (cover_case_data*)param;
	char buffer[4] = { 0x01, 0x02, 0x03, 0x04 };
	while (!(data_p->exit_flag))
	{
		autocover_buf_lock(param);
		if (auto_cover_buf_write(data_p->cover_buf_p, buffer, sizeof(buffer)) == sizeof(buffer))
		{
			data_p->data_in_counter += sizeof(buffer);
		}
		else
		{
			LOGE("failed on write!!! shouldn't reach here!!!");
		}
		autocover_buf_unlock(param);
		//Sleep(1);
	}
	LOGD("<-- producer thread out. counter=%u", data_p->data_in_counter);
	return NULL;
}

int autocover_buffer_test()
{
	LOGD("now test autocover_buffer!");
	cover_case_data case_data = { 0 };
	case_data.data_in_counter = 0;
	case_data.exit_flag = false;
	pthread_mutex_init(&case_data.mutex_coverbuf, NULL);
	//auto_cover_buf_lock_t buf_lock = 
	//{
	//	.acquire = NULL,
	//	.release = NULL,
	//	.arg = &case_data
	//};
	case_data.cover_buf_p = auto_cover_buf_create(16, NULL/*&buf_lock*/);
	ASSERT(case_data.cover_buf_p);

	pthread_t pthread_consumer;
	pthread_t pthread_producer;
	pthread_create(&pthread_consumer, NULL, thread_consumer, &case_data);
	pthread_create(&pthread_producer, NULL, thread_producer, &case_data);

	sleep(10);
	case_data.exit_flag = true;
	pthread_join(pthread_producer, NULL);
	pthread_join(pthread_consumer, NULL);
	pthread_mutex_destroy(&case_data.mutex_coverbuf);

	auto_cover_buf_destroy(&case_data.cover_buf_p);
	LOGD("autocover_buffer test finished.");
	return 0;
}