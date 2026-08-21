/**
 * @file    bignum_xor.c
 * @brief   Portable reference implementation of bignum_xor.
 * @version 0.1.0
 * @details Validates complete-object aliasing before reading either input,
 * computes the inclusive XOR into a stack temporary, clears unused words, and
 * publishes the result atomically after normalization. The temporary keeps
 * exact in-place aliases safe while partial overlaps are rejected explicitly.
 */
/* ------------------------------------------------------------------ */
#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include "bignum_xor.h"

/**
 * @brief Reports whether two distinct complete bignum objects overlap.
 * @param[in] x First complete object; caller-owned and non-NULL.
 * @param[in] y Second complete object; caller-owned and non-NULL.
 * @return Non-zero when the byte ranges overlap; exact identity is permitted.
 * @details uintptr_t range arithmetic mirrors the fixed complete-object alias
 * contract used by the ASM implementation. Callers validate NULL separately.
 */
static int ranges_overlap(const bignum_t *x, const bignum_t *y)
{
    if (x == y) {
        return 0;
    }
    uintptr_t xb = (uintptr_t)(const void *)x;
    uintptr_t yb = (uintptr_t)(const void *)y;
    uintptr_t xe = xb + sizeof(*x);
    uintptr_t ye = yb + sizeof(*y);
    return xb < ye && yb < xe;
}

/* The public header owns the complete API contract documentation. */
bignum_xor_status_t bignum_xor(
    bignum_t *result,
    const bignum_t *a,
    const bignum_t *b)
{
    if (result == NULL || a == NULL || b == NULL) {
        return BIGNUM_XOR_ERROR_NULL_PTR;
    }
    if (a->len > BIGNUM_CAPACITY || b->len > BIGNUM_CAPACITY) {
        return BIGNUM_XOR_ERROR_CAPACITY_EXCEEDED;
    }
    if (ranges_overlap(result, a) || ranges_overlap(result, b) ||
        ranges_overlap(a, b)) {
        return BIGNUM_XOR_ERROR_BUFFER_OVERLAP;
    }

    size_t out_len = a->len > b->len ? a->len : b->len;
    bignum_t tmp;
    memset(&tmp, 0, sizeof(tmp));
    for (size_t i = 0U; i < out_len; ++i) {
        uint64_t aw = i < a->len ? a->words[i] : UINT64_C(0);
        uint64_t bw = i < b->len ? b->words[i] : UINT64_C(0);
        tmp.words[i] = aw ^ bw;
    }
    tmp.len = out_len;
    while (tmp.len > 0U && tmp.words[tmp.len - 1U] == UINT64_C(0)) {
        --tmp.len;
    }
    *result = tmp;
    return BIGNUM_XOR_SUCCESS;
}

/* SPDX-License-Identifier: MIT */
