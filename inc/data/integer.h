#pragma once
#ifndef LCU_INTEGER_H
#define LCU_INTEGER_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

	/**
	 * Determine whether the number is n's power of 2.
	 * 
	 * @param i : a number
	 * @return true for positive, otherwise negative
	 */
	bool integer_is_power_of_two(uint32_t i);

	/**
	 * round up the number to power of two.
	 * note: if i == 0, then return 0
	 * 
	 * @param i : a number
	 * @return round up number. e.g., 0 -> 0, 1 -> 2, 2 -> 2, 127 -> 128
	 */
	uint32_t integer_roundup_pow_of_two(uint32_t i);

	/**
	 * round down the number to power of two.
	 * note: if i < 2, then return 0
	 * 
	 * @param i : a number
	 * @return round down number. e.g., 0 -> 0, 1 -> 0, 2 -> 2, 127 -> 64
	 */
	uint32_t integer_rounddown_pow_of_two(uint32_t i);

#ifdef __cplusplus
}
#endif

#endif // !LCU_INTEGER_H
