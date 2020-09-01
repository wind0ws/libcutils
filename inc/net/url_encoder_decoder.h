#pragma once
#ifndef __LCU_URL_ENCODER_DECODER_H
#define __LCU_URL_ENCODER_DECODER_H

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

	/**
     * @brief url_encode : encode the base64 string "src_buf_plain".
     *
     * Note:
	 * 1) to ensure the result buffer(out_buf_encoded) has enough space to contain the encoded string,
	 *    we'd better to set out_buf_encoded_size to 3 * src_buf_plain_strlen + 1(for 'NULL terminator)
	 * 2) we don't check whether string has really been base64 encoded
	 *
     * @param out_buf_encoded:  the result url encoded buffer
     * @param out_buf_encoded_size: the result buffer size(should include the last NULL terminator)
     * @param src_buf_plain: the plain string to encode
     * @param src_buf_plain_strlen: the plain string length (exclude the last NULL terminator)
     *
     * @return >=0 represent the encoded result string length
     *          <0 encode failure
     */
	int lcu_url_encode(char* out_buf_encoded, const size_t out_buf_encoded_size,
		const char* src_buf_plain, const size_t src_buf_plain_strlen);
		
	/**
	 * @brief URLDecode : decode the url encoded string to base64 encoded string.
	 *
	 * Note:
	 * 1) to ensure the result buffer(out_buf_decoded) has enough space to contain the decoded string,
	 *    we'd better to set out_buf_decoded_size to src_plain_strlen + 1(for NULL terminator)
	 *
	 * @param out_buf_decoded:  the result buffer
	 * @param out_buf_decoded_size: the result buffer size(should include the last NULL terminator)
	 * @param src_buf_plain:  the url encoded string
	 * @param src_buf_plain_strlen: the string length(exclude the last NULL terminator)
	 * @param last_src_pos: the last decode src_buf_plain position. if you don't care, just pass NULL
	 *
	 * @return >=0 represent the decoded result string length
	 *          <0 encode failure
	 */
	int lcu_url_decode(char* out_buf_decoded, const size_t out_buf_decoded_size,
		const char* src_buf_plain, const size_t src_buf_plain_strlen, char** last_src_pos);

#ifdef __cplusplus
};
#endif

#endif //__LCU_URL_ENCODER_DECODER_H
