#pragma once
#ifndef BIT_OPS_H
#define BIT_OPS_H

#include <stdint.h>
#include <inttypes.h>

// 读取 num 的 bit 位（返回 0 或 1）
#ifndef GET_BIT
#define GET_BIT(num, bit) \
    (((uintmax_t)(num) >> (uintmax_t)(bit)) & 1U)
#endif // !GET_BIT

// 将 num 的 bit 位设置为 val (0/1) — 就地修改
#ifndef SET_BIT
#define SET_BIT(num, bit, val)                            \
    do                                                    \
    {                                                     \
        if ((val) & 1U)                                   \
            (num) |= ((uintmax_t)1 << (uintmax_t)(bit));  \
        else                                              \
            (num) &= ~((uintmax_t)1 << (uintmax_t)(bit)); \
    } while (0)
#endif // !SET_BIT

// 反转 num 的 bit 位 — 就地修改
#ifndef TOGGLE_BIT
#define TOGGLE_BIT(num, bit)                         \
    do                                               \
    {                                                \
        (num) ^= ((uintmax_t)1 << (uintmax_t)(bit)); \
    } while (0)
#endif // !TOGGLE_BIT

// 一次性批量设置多个位
// mask: 要修改的位掩码
// val:  新值，只取 mask 范围内的位
#ifndef SET_BITS
#define SET_BITS(num, mask, val)                                                                      \
    do                                                                                                \
    {                                                                                                 \
        (num) = ((uintmax_t)(num) & ~((uintmax_t)(mask))) | (((uintmax_t)(val)) & (uintmax_t)(mask)); \
    } while (0)
#endif // !SET_BITS

// 一次性批量翻转多个位
// mask: 要翻转的位掩码
#ifndef TOGGLE_BITS
#define TOGGLE_BITS(num, mask)        \
    do                                \
    {                                 \
        (num) ^= ((uintmax_t)(mask)); \
    } while (0)
#endif // !TOGGLE_BITS

#endif // !BIT_OPS_H
