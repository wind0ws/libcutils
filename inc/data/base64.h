#pragma once
#ifndef __LCU_BASE64_H
#define __LCU_BASE64_H

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

	/**
	 * get base64 encode buf size for alloc encode buf, return size is include NULL terminator.
	 *
	 * @param plain_buf_len: how many characters(bytes) do you want to encode.
	 * @return encode buf size
	 */
	int lcu_base64_encode_buf_size(const size_t plain_buf_len);

	/**
	 * encode base64
	 *
	 * Note: we add NULL terminator at end of out_buf_encoded, so you can just treat out_buf as string.
	 *       the result encode string is not include CRLF(new line).
	 *
	 * @param out_buf_encoded: output encode buffer
	 * @param src_buf_plain: plain buffer that you want to encode
	 * @param src_buf_len: how many characters(bytes) do you want to encode,
	 *                     this should small or equal src_buf_plain size.
	 * @return encode buf size.
	 */
	int lcu_base64_encode(char* out_buf_encoded, const char* src_buf_plain, const size_t src_buf_len);

	/**
	 * get base64 decode buf size for alloc decode buf, return size is include NULL terminator.
	 *
	 * @param encoded_buf_len: how many characters(bytes) do you want to decode.
	 * @return decode buf size
	 */
	int lcu_base64_decode_buf_size(const size_t encoded_buf_len);

	/**
	 * decode base64
	 *
	 * @param out_buf_plain: output decode buf
	 * @param src_buf_encoded: the base64 encoded buf
	 * @return decoded byte size.
	 */
	int lcu_base64_decode(char* out_buf_plain, const char* src_buf_encoded);

#ifdef  __cplusplus
};
#endif

#endif // __LCU_BASE64_H
