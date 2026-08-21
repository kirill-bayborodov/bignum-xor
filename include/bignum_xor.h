/**
 * @file    bignum_xor.h
 * @brief   Побитовый XOR двух normalized bignum_t.
 * @version 0.1.0
 * @details
 *   Revision 0.1.0: typed API and C/ASM implementation for bitwise XOR.
 *   The operation computes the word-wise inclusive XOR of two fixed-capacity
 *   bignum objects without dynamic allocation. Successful output is normalized
 *   and all unused storage words are cleared.
 */
/* ------------------------------------------------------------------ */
#pragma once
#ifndef BIGNUM_XOR_H
#define BIGNUM_XOR_H

#include <stddef.h>
#include <stdint.h>
#include "bignum.h"

#ifndef BIGNUM_CAPACITY
#  error "bignum.h must define BIGNUM_CAPACITY"
#endif

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Reports the result of a bignum_xor operation.
 * @details A successful return guarantees that result contains the normalized
 * inclusive XOR. Error returns occur before result mutation.
 */
typedef enum {
    BIGNUM_XOR_SUCCESS                 = 0,  /**< Operation completed; result is normalized and valid. */
    BIGNUM_XOR_ERROR_NULL_PTR          = -1, /**< A required pointer is NULL; result is unchanged. */
    BIGNUM_XOR_ERROR_CAPACITY_EXCEEDED = -2, /**< An input len exceeds BIGNUM_CAPACITY; result is unchanged. */
    BIGNUM_XOR_ERROR_BUFFER_OVERLAP    = -3  /**< Partial object overlap was detected; result is unchanged. */
} bignum_xor_status_t;

/**
 * @brief Computes `result = a ^ b` for normalized 2048-bit bignum objects.
 * @details The implementation computes through a stack temporary in the C
 * reference path and through register/word operations in the ASM path. The
 * output length starts at the larger input length because XOR preserves words
 * present in either operand, then leading zero words are removed. The complete
 * destination storage is cleared before the normalized result is published.
 * Exact aliases `result == a`, `result == b`, and `a == b` are supported.
 * Partial overlap between any two distinct complete objects is rejected.
 *
 * @param[out] result Caller-allocated destination object; unchanged on error.
 * @param[in]  a Caller-owned first operand; its complete object is read only.
 * @param[in]  b Caller-owned second operand; its complete object is read only.
 * @return A named bignum_xor_status_t value describing success or the rejected
 *         input condition.
 * @pre Every non-NULL pointer refers to a complete bignum_t object, and each
 *      input len is at most BIGNUM_CAPACITY.
 * @post On success, result equals the normalized bitwise XOR and has zeroed tail
 *       words. On error, result storage is not modified.
 * @warning Partial overlap is not a supported aliasing mode; use exact aliases
 *          or independent complete bignum_t objects.
 * Thread-safety: safe for independent objects; no global mutable state exists.
 * Complexity: O(BIGNUM_CAPACITY) worst-case time and O(1) auxiliary memory in
 *             the ASM path; the C reference uses one stack bignum_t temporary.
 */
bignum_xor_status_t bignum_xor(
    bignum_t *result,
    const bignum_t *a,
    const bignum_t *b);

#ifdef __cplusplus
}
#endif

#endif /* BIGNUM_XOR_H */

/* SPDX-License-Identifier: MIT */
