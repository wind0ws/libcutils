/******************************************************************************
 *
 *  Copyright (C) 2014 Google, Inc.
 *
 *  Licensed under the Apache License, Version 2.0 (the "License");
 *  you may not use this file except in compliance with the License.
 *  You may obtain a copy of the License at:
 *
 *  http://www.apache.org/licenses/LICENSE-2.0
 *
 *  Unless required by applicable law or agreed to in writing, software
 *  distributed under the License is distributed on an "AS IS" BASIS,
 *  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *  See the License for the specific language governing permissions and
 *  limitations under the License.
 *
 * reference https://chromium.googlesource.com/aosp/platform/system/bt/+/refs/heads/master/osi/src/hash_functions.c
 ******************************************************************************/
#include "data/hash_functions.h"

int hash_function_naive(const void* key)
{
	return (int)key;
}

// 2^32 = 2654435761 

int hash_function_integer(const void* key)
{
	return ((int)key) * 2654435761;
}

int hash_function_pointer(const void* key)
{
	return ((int)key) * 2654435761;
}

/**
 * hash string.
 * 
 * Magic Constant 5381:
 * 1. odd number
 * 2. prime number
 * 3. deficient number
 * 4. 001/010/100/000/101 b
 */
int hash_function_string(const void* key)
{
	int hash = 5381;
	const char* str = (const char*)key;
	char c;
	while ((c = *str) != '\0')
	{
		hash = ((hash << 5) + hash) + c;
		++str;
	}
	return hash;
}
