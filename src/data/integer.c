#include "data/integer.h"

bool integer_is_power_of_two(uint32_t i)
{
	return (i > 1 && !(i & (i - 1)));
}

uint32_t integer_roundup_pow_of_two(uint32_t i)
{
	if (i < 1)
	{
		return 0;
	}
	if (integer_is_power_of_two(i))
	{
		return i;
	}

	// "Smear" the high-order bit all the way to the right.
	i |= i >> 1;
	i |= i >> 2;
	i |= i >> 4;
	i |= i >> 8;
	i |= i >> 16;

	return (i + 1U);
}

uint32_t integer_rounddown_pow_of_two(uint32_t i)
{
	if (i < 2)
	{
		return 0;
	}
	uint32_t roundup_integer = integer_roundup_pow_of_two(i);
	if (roundup_integer == i)
	{
		return roundup_integer;
	}
	return (roundup_integer >> 1);
}
