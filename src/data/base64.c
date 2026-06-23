/*
 * Base64 encoding/decoding (RFC1341)
 * Copyright (c) 2005-2011, Jouni Malinen <j@w1.fi>
 *
 * This software may be distributed under the terms of the BSD license.
 * See README for more details.
 */

/*
 * Base64 encoder/decoder. Originally Apache file ap_base64.c
 */

#include "data/base64.h"

static const char basis_64[] =
"ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

/* NOTE(reviewed 2026-06-08): the *_buf_size helpers compute in size_t but return
 * int, and lcu_base64_encode has no output-capacity parameter. For inputs whose
 * encoded/decoded size exceeds INT_MAX this truncates. This is a documented
 * hardening gap (D-3 in .ai/review-2026-06-08.md), NOT an active bug: there is no
 * contract promising SIZE_MAX-safe sizing, and realistic base64 payloads are far
 * below INT_MAX. If large/untrusted inputs become a use case, return size_t (or a
 * checked status) and reject sizes > INT_MAX. */
int lcu_base64_encode_buf_size(const size_t plain_buf_len)
{
	return (int)(((plain_buf_len + 2) / 3 * 4) + 1);
}

int lcu_base64_encode(char* out_buf_encoded, const char* src_buf_plain, const size_t src_buf_len)
{
	size_t i;
	char* p;
	/* P2-5: 接受合法的 1 字节/2 字节输入 (base64 标准支持: "A"->"QQ==").
	 * 修复前 src_buf_len<2 直接返回 -1 拒绝 1 字节. */
	if (src_buf_len < 1 || NULL == out_buf_encoded || NULL == src_buf_plain)
	{
		return -1;
	}

	p = out_buf_encoded;
	/* P2-5: 主循环只在 src_buf_len >= 3 时进入,避免 src_buf_len-2 在 size_t 下溢. */
	if (src_buf_len >= 3)
	{
		for (i = 0; i < src_buf_len - 2; i += 3)
		{
			*p++ = basis_64[(src_buf_plain[i] >> 2) & 0x3F];
			*p++ = basis_64[((src_buf_plain[i] & 0x3) << 4) |
				((int)(src_buf_plain[i + 1] & 0xF0) >> 4)];
			*p++ = basis_64[((src_buf_plain[i + 1] & 0xF) << 2) |
				((int)(src_buf_plain[i + 2] & 0xC0) >> 6)];
			*p++ = basis_64[src_buf_plain[i + 2] & 0x3F];
		}
	}
	else
	{
		i = 0; /* 直接进入尾部 padding 处理 */
	}
	if (i < src_buf_len)
	{
		*p++ = basis_64[(src_buf_plain[i] >> 2) & 0x3F];
		if (i == (src_buf_len - 1))
		{
			*p++ = basis_64[((src_buf_plain[i] & 0x3) << 4)];
			*p++ = '=';
		}
		else
		{
			*p++ = basis_64[((src_buf_plain[i] & 0x3) << 4) |
				((int)(src_buf_plain[i + 1] & 0xF0) >> 4)];
			*p++ = basis_64[((src_buf_plain[i + 1] & 0xF) << 2)];
		}
		*p++ = '=';
	}

	*p++ = '\0';
	return (int)(p - out_buf_encoded);
}

/* aaaack but it's fast and const should make it shared text page. */
static const unsigned char pr2six[256] =
        {
                /* ASCII table */
                64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64,
                64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64,
                64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 62, 64, 64, 64, 63,
                52, 53, 54, 55, 56, 57, 58, 59, 60, 61, 64, 64, 64, 64, 64, 64,
                64,  0,  1,  2,  3,  4,  5,  6,  7,  8,  9, 10, 11, 12, 13, 14,
                15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 64, 64, 64, 64, 64,
                64, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 36, 37, 38, 39, 40,
                41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51, 64, 64, 64, 64, 64,
                64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64,
                64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64,
                64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64,
                64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64,
                64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64,
                64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64,
                64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64,
                64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64
        };

int lcu_base64_decode_buf_size(const size_t encoded_buf_len)
{
    return (int)(((encoded_buf_len + 3) / 4) * 3 + 1);
}

int lcu_base64_decode(char *out_buf_plain, const char *src_buf_encoded)
{
    int nbytesdecoded;
    register const unsigned char *bufin;
    register unsigned char *bufout;
    register int nprbytes;

    bufin = (const unsigned char *) src_buf_encoded;
    while (pr2six[*(bufin++)] <= 63);
    nprbytes = (int)(bufin - (const unsigned char *) src_buf_encoded) - 1;
    nbytesdecoded = ((nprbytes + 3) / 4) * 3;

    bufout = (unsigned char *) out_buf_plain;
    bufin = (const unsigned char *) src_buf_encoded;

    while (nprbytes > 4) 
    {
        *(bufout++) = (unsigned char) (pr2six[*bufin] << 2 | pr2six[bufin[1]] >> 4);
        *(bufout++) = (unsigned char) (pr2six[bufin[1]] << 4 | pr2six[bufin[2]] >> 2);
        *(bufout++) = (unsigned char) (pr2six[bufin[2]] << 6 | pr2six[bufin[3]]);
        bufin += 4;
        nprbytes -= 4;
    }

    /* Note: (nprbytes == 1) would be an error, so just ingore that case */
    if (nprbytes > 1) 
    {
        *(bufout++) = (unsigned char) (pr2six[*bufin] << 2 | pr2six[bufin[1]] >> 4);
    }
    if (nprbytes > 2) 
    {
        *(bufout++) = (unsigned char) (pr2six[bufin[1]] << 4 | pr2six[bufin[2]] >> 2);
    }
    if (nprbytes > 3) 
    {
        *(bufout++) = (unsigned char) (pr2six[bufin[2]] << 6 | pr2six[bufin[3]]);
    }

    *(bufout++) = '\0';
    nbytesdecoded -= ((4 - nprbytes) & 3);
    return nbytesdecoded;
}
