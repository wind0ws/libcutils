#include "data/integer.h"

#define LOG_TAG "INTEGER_TEST"
#include "log/logger.h"

int integer_test()
{
	TLOGD(LOG_TAG, "is_power_of_two(128) = %d", integer_is_power_of_two(128));
	TLOGD(LOG_TAG, "is_power_of_two(255) = %d", integer_is_power_of_two(255));
	TLOGD(LOG_TAG, "roundup(0) = %d", integer_roundup_pow_of_two(0));
	TLOGD(LOG_TAG, "roundup(1) = %d", integer_roundup_pow_of_two(1));
	TLOGD(LOG_TAG, "roundup(2) = %d", integer_roundup_pow_of_two(2));
	TLOGD(LOG_TAG, "roundup(127) = %d", integer_roundup_pow_of_two(127));
	TLOGD(LOG_TAG, "rounddown(1) = %d", integer_rounddown_pow_of_two(1));
	TLOGD(LOG_TAG, "rounddown(2) = %d", integer_rounddown_pow_of_two(2));
	TLOGD(LOG_TAG, "rounddown(127) = %d", integer_rounddown_pow_of_two(127));
	return 0;
}
