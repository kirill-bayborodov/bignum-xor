/**
 * @file test_bignum_xor_runner.c
 * @brief Distribution integration smoke test for bignum_xor.
 *
 * @details This executable links the public header and production object in the
 * same shape as the distributed runner. It checks a normal two-input XOR, an
 * exact destination alias, and the NULL-result negative path. The expected
 * statuses are named public enum values and the expected output is checked by
 * length and word value. Any failed assertion exits non-zero; the `PASSED`
 * marker is printed only after all three scenarios succeed.
 *
 * @par Ownership All bignum_t objects are stack-owned and remain valid for the
 * duration of each call. No cleanup beyond automatic storage is required.
 */
/* ------------------------------------------------------------------ */
#include <assert.h>
#include <stdint.h>
#include <stdio.h>

#include "bignum_xor.h"

/**
 * @brief Executes the distribution-facing success, alias, and NULL checks.
 * @return EXIT_SUCCESS after all three contract cases pass; assertion failure
 * terminates with a non-zero status.
 * @details The fixed operands are `UINT64_MAX` and `0x0F0F`, whose XOR oracle is
 * one word of `0xFFFFFFFFFFFFF0F0`. The alias case must preserve that value, while the
 * NULL-result case must return `BIGNUM_XOR_ERROR_NULL_PTR`.
 */
int main(void)
{
    bignum_t a = { { UINT64_MAX }, 1U };
    bignum_t b = { { UINT64_C(0x0F0F) }, 1U };
    bignum_t result;
    printf("Running test: test_bignum_xor_runner... ");
    assert(bignum_xor(&result, &a, &b) == BIGNUM_XOR_SUCCESS);
    assert(result.len == 1U && result.words[0] == UINT64_C(0xFFFFFFFFFFFFF0F0));
    assert(bignum_xor(&a, &a, &b) == BIGNUM_XOR_SUCCESS);
    assert(a.words[0] == UINT64_C(0xFFFFFFFFFFFFF0F0));
    assert(bignum_xor(NULL, &a, &b) == BIGNUM_XOR_ERROR_NULL_PTR);
    puts("PASSED");
    return 0;
}
