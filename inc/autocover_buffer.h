#pragma once
#ifndef __AUTOCOVER_BUFFER_H__
#define __AUTOCOVER_BUFFER_H__

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

	typedef int(* auto_cover_buf_lock_fn)(void *user_data);
	typedef int(* auto_cover_buf_unlock_fn)(void* user_data);

	typedef struct __auto_cover_buf* auto_cover_buf;

	auto_cover_buf auto_cover_buf_create(uint32_t capacity_size, auto_cover_buf_lock_fn lockfn, auto_cover_buf_lock_fn unlockfn, void* lock_unlock_fn_user_data);

	int auto_cover_buf_available_read(const auto_cover_buf cover_buf_p, uint32_t read_pos);

	int auto_cover_buf_read(const auto_cover_buf cover_buf_p, uint32_t read_pos, char* target, uint32_t req_read_len);

	int auto_cover_buf_write(const auto_cover_buf cover_buf_p, char* source, uint32_t write_len);

	void auto_cover_buf_destroy(auto_cover_buf* cover_buf_pp);

#ifdef __cplusplus
}
#endif

#endif