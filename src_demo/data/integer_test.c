#include "data/integer.h"

#define LOG_TAG "INTEGER_TEST"
#include "log/logger.h"

int integer_test()
{
	LOGD("is_power_of_two(128) = %d", integer_is_power_of_two(128));
	LOGD("is_power_of_two(255) = %d", integer_is_power_of_two(255));
	LOGD("roundup(0) = %d", integer_roundup_pow_of_two(0));
	LOGD("roundup(1) = %d", integer_roundup_pow_of_two(1));
	LOGD("roundup(2) = %d", integer_roundup_pow_of_two(2));
	LOGD("roundup(127) = %d", integer_roundup_pow_of_two(127));
	LOGD("rounddown(1) = %d", integer_rounddown_pow_of_two(1));
	LOGD("rounddown(2) = %d", integer_rounddown_pow_of_two(2));
	LOGD("rounddown(127) = %d", integer_rounddown_pow_of_two(127));
	return 0;
}
