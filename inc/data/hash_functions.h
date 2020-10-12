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
 * reference https://chromium.googlesource.com/aosp/platform/system/bt/+/refs/heads/master/osi/include/hash_functions.h
 ******************************************************************************/
#pragma once
#ifndef __HASHMAP_FUNCTIONS_H
#define __HASHMAP_FUNCTIONS_H

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

int hash_function_naive(const void *key);

int hash_function_integer(const void *key);

// Hashes a pointer based only on its address value
int hash_function_pointer(const void *key);

int hash_function_string(const void *key);

#ifdef __cplusplus
}
#endif // __cplusplus

#endif //__HASHMAP_FUNCTIONS_H