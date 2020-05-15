#pragma once
#ifndef __RAND_H
#define __RAND_H

#include <stdlib.h>
#include <time.h>

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

	static inline int rand_range(int a, int b)
	{
		//b should bigger than a, or we just return 0.
		if (a >= b)
		{
			return 0;
		}
		srand((unsigned)time(NULL));
		return rand() % (b - a) + a;
	}

#ifdef __cplusplus
};
#endif // __cplusplus

#endif // !__LC_RAND_H
