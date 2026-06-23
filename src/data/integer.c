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
	i |= (i >> 1);
	i |= (i >> 2);
	i |= (i >> 4);
	i |= (i >> 8);
	i |= (i >> 16);

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

size_t integer_add_offset(size_t base, ptrdiff_t offset)
{
    if (0 == offset)
    {
        return base;
    }

    if (offset > 0)
    {
        // 处理正偏移
        if ((size_t)offset > (SIZE_MAX - base))
        {
            // 正溢出处理（例如返回最大值或报错）
            return SIZE_MAX;
        }
        return base + (size_t)offset;
    }
    else
    {
        // 处理负偏移, 取绝对值（转换为无符号）
        size_t abs_offset = (size_t)(-offset);
        if (abs_offset > base)
        {
            // 下溢处理（例如返回0或最小值）
            return 0;
        }
        return base - abs_offset;
    }
}
