#include "ring/autocover_buffer.h"
#include "common_macro.h"
#include "thread/posix_thread.h"
#include <string.h>

#define LOG_TAG "AUTOCOVER"
#include "log/logger.h"

typedef struct 
{
	auto_cover_buf_handle cover_buf_p;
	pthread_mutex_t mutex_coverbuf;
	volatile uint32_t data_in_counter;
	volatile bool exit_flag;
} cover_case_data_t;

static int autocover_buf_lock(void* user_data)
{
	cover_case_data_t* data_p = (cover_case_data_t*)user_data;
	return pthread_mutex_lock(&data_p->mutex_coverbuf);
}

static int autocover_buf_unlock(void* user_data)
{
	cover_case_data_t* data_p = (cover_case_data_t*)user_data;
	return pthread_mutex_unlock(&data_p->mutex_coverbuf);
}

static void* thread_consumer(void* param)
{
	LOGD("--> consumer thread in");
	cover_case_data_t* data_p = (cover_case_data_t*)param;
	int ret = 0;
	char buffer[16];
	while (!(data_p->exit_flag))
	{
		if (data_p->data_in_counter < sizeof(buffer)) // no enough data in queue
		{
			Sleep(1);// mock the situation that we are not always get data from queue, let queue automatically recycle it
			continue;
		}
		memset(buffer, 0, sizeof(buffer));
		autocover_buf_lock(param);
		const uint32_t read_pos = data_p->data_in_counter - sizeof(buffer);
		/*if (read_pos < 512)
		{
			LOGI("now read_pos=%u", read_pos);
		}*/
		if ((int)(sizeof(buffer)) == (ret = auto_cover_buf_read(data_p->cover_buf_p, read_pos, buffer, sizeof(buffer))))
		{
			if (0x00 != buffer[0] || 0x0F != buffer[sizeof(buffer) - 1])
			{
				LOGE("!!! read error !!! read_pos=%u", read_pos);
				ASSERT_ABORT(0);
			}
			else
			{
				//LOGI(" *** read succeed! --> read_pos=%u", read_pos);
			}
			//Sleep(1);
		}
		else
		{
			//LOGE("not able read. read_pos=%u, ret=%d", read_pos, ret);
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
	cover_case_data_t* data_p = (cover_case_data_t*)param;
	char buffer[16] = { 0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F };
	while (!(data_p->exit_flag))
	{
		autocover_buf_lock(param);
		if ((int)(sizeof(buffer)) == auto_cover_buf_write(data_p->cover_buf_p, buffer, sizeof(buffer)))
		{
			if (0 == data_p->data_in_counter)
			{
				LOGI("producer: new cycle begin");
			}
			data_p->data_in_counter += sizeof(buffer);
		}
		else
		{
			LOGE("!!!failed on write!!! it's unusual. shouldn't reach here!!! ");
			ASSERT_ABORT(0);
		}
		autocover_buf_unlock(param);
		//Sleep(1);
	}
	LOGD("<-- producer thread out. counter=%u", data_p->data_in_counter);
	return NULL;
}

int autocover_buffer_test()
{
	LOGD("--> now test autocover_buffer!");
	cover_case_data_t case_data = { 0 };
	case_data.data_in_counter = 0;
	case_data.exit_flag = false;
	pthread_mutex_init(&case_data.mutex_coverbuf, NULL);
	//auto_cover_buf_lock_t buf_lock = 
	//{
	//	.acquire = pthread_mutex_lock,
	//	.release = pthread_mutex_unlock,
	//	.arg = &data_p->mutex_coverbuf
	//};
	// we are not provide buf_lock, so we should protect read/write by ourselves on multi-thread!
	case_data.cover_buf_p = auto_cover_buf_create(32, NULL/*&buf_lock*/);
	ASSERT_ABORT(case_data.cover_buf_p);

	pthread_t pthread_consumer;
	pthread_t pthread_producer;
	pthread_create(&pthread_consumer, NULL, thread_consumer, &case_data);
	pthread_create(&pthread_producer, NULL, thread_producer, &case_data);

	LOGI("running....wait 80s...");
	sleep(80);
	case_data.exit_flag = true;
	pthread_join(pthread_producer, NULL);
	pthread_join(pthread_consumer, NULL);
	LOGI("producer and consumer are both exited...");

	auto_cover_buf_destroy(&case_data.cover_buf_p);
	pthread_mutex_destroy(&case_data.mutex_coverbuf);
	LOGD("<-- autocover_buffer test finished.");
	return 0;
}