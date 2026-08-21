/**
 * @file test_bignum_xor.c
 * @brief Deterministic contract and boundary tests for bignum_xor.
 *
 * @details The test exercises the public typed API with fixed two-word and
 * full-capacity operands. Each successful case computes its expected result
 * independently from the implementation under test, checks normalized length,
 * and checks the complete zeroed tail where that invariant is relevant.
 * Negative cases use named status values and verify validation-before-mutation.
 * Any failed assertion aborts the process with a non-zero exit status.
 *
 * @par Ownership The test owns all stack-allocated bignum_t values. The API
 * does not take ownership of any pointer and must not allocate memory.
 * @par Thread safety This file is single-threaded; independent concurrent calls
 * are covered by test_bignum_xor_mt.c.
 */
/* ------------------------------------------------------------------ */
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "bignum_xor.h"

/**
 * @brief Compares complete normalized bignum objects.
 * @param[in] actual Result produced by bignum_xor; must be non-NULL.
 * @param[in] expected Independent oracle object; must be non-NULL.
 * @details The comparison covers logical length and all capacity words, so a
 * stale destination tail cannot be hidden by comparing only active words.
 * @pre Both objects are fully initialized by the caller.
 * @post The objects are unchanged.
 */
static void assert_value(const bignum_t *actual, const bignum_t *expected)
{
    assert(actual->len == expected->len);
    assert(memcmp(actual->words, expected->words,
                  sizeof(actual->words)) == 0);
}

/**
 * @brief Verifies ordinary two-word inclusive XOR.
 * @details Fixed inputs are `{0xF0F0,0xFFFF}` and `{0x0FF0,0x0001}`;
 * the exact oracle is `{0xFF00,0xFFFE}` with `len == 2`. The expected status
 * is `BIGNUM_XOR_SUCCESS`, and the complete object comparison checks the output
 * words and normalized representation.
 */
static void test_basic_xor(void)
{
    bignum_t a = { { UINT64_C(0xF0F0), UINT64_C(0xFFFF) }, 2U };
    bignum_t b = { { UINT64_C(0x0FF0), UINT64_C(0x0001) }, 2U };
    bignum_t result;
    bignum_t expected = { { UINT64_C(0xFF00), UINT64_C(0xFFFE) }, 2U };
    assert(bignum_xor(&result, &a, &b) == BIGNUM_XOR_SUCCESS);
    assert_value(&result, &expected);
    puts("test_basic_xor: PASSED");
}

/**
 * @brief Verifies zero extension and normalization of a one-sided operand.
 * @details The second operand has `len == 0`, so the XOR result must equal the
 * two active all-ones words of the first operand with `len == 2`. Every word
 * from index 2 through `BIGNUM_CAPACITY - 1` is explicitly required to be zero.
 */
static void test_zero_operand_and_normalization(void)
{
    bignum_t a = { { UINT64_MAX, UINT64_MAX }, 2U };
    bignum_t b = { { 0U }, 0U };
    bignum_t result;
    assert(bignum_xor(&result, &a, &b) == BIGNUM_XOR_SUCCESS);
    assert(result.len == 2U);
    assert(result.words[0] == UINT64_MAX && result.words[1] == UINT64_MAX);
    for (size_t i = 2U; i < BIGNUM_CAPACITY; ++i) assert(result.words[i] == 0U);
    puts("test_zero_operand_and_normalization: PASSED");
}

/**
 * @brief Verifies XOR output length, zero extension, and tail clearing.
 * @details The first input has length 3 and the second length 2. The expected
 * oracle is `{0x1224,0,0x100}` with length 3;
 * this proves XOR uses the larger logical input length rather than truncating to
 * the shorter operand.
 */
static void test_max_length_zero_extension(void)
{
    bignum_t a = { { UINT64_C(0x1234), 0U, UINT64_C(0x100) }, 3U };
    bignum_t b = { { UINT64_C(0x10), 0U }, 2U };
    bignum_t result;
    bignum_t expected = { { UINT64_C(0x1224), 0U, UINT64_C(0x100) }, 3U };
    assert(bignum_xor(&result, &a, &b) == BIGNUM_XOR_SUCCESS);
    assert_value(&result, &expected);
    puts("test_max_length_zero_extension: PASSED");
}

/**
 * @brief Verifies leading-zero normalization after an XOR result.
 * @details Both inputs declare length 3, but only word zero is non-zero in the
 * first input. The expected status is success, output length is 1, and every
 * remaining capacity word must be zero.
 */
static void test_high_zero_normalization(void)
{
    bignum_t a = { { UINT64_C(0x1234), 0U, 0U }, 3U };
    bignum_t b = { { 0U, 0U, 0U }, 3U };
    bignum_t result;
    assert(bignum_xor(&result, &a, &b) == BIGNUM_XOR_SUCCESS);
    assert(result.len == 1U && result.words[0] == UINT64_C(0x1234));
    for (size_t i = 1U; i < BIGNUM_CAPACITY; ++i) assert(result.words[i] == 0U);
    puts("test_high_zero_normalization: PASSED");
}

/**
 * @brief Verifies all supported exact-alias combinations.
 * @details The fixed operands produce `{0xF0F0,0xFF}` for `result == a` and
 * `result == b`; `result == a == b` is also tested as a self-XOR producing zero. Exact aliases
 * must return `BIGNUM_XOR_SUCCESS` and preserve the mathematically expected
 * words. This is distinct from partial overlap, which is rejected elsewhere.
 */
static void test_exact_aliases(void)
{
    bignum_t a = { { UINT64_C(0xFFFF), UINT64_C(0xAA) }, 2U };
    bignum_t b = { { UINT64_C(0x0F0F), UINT64_C(0x55) }, 2U };
    bignum_t expected = { { UINT64_C(0xF0F0), UINT64_C(0xFF) }, 2U };
    assert(bignum_xor(&a, &a, &b) == BIGNUM_XOR_SUCCESS);
    assert_value(&a, &expected);
    a = (bignum_t){ { UINT64_C(0xFFFF), UINT64_C(0xAA) }, 2U };
    assert(bignum_xor(&b, &a, &b) == BIGNUM_XOR_SUCCESS);
    assert_value(&b, &expected);
    assert(bignum_xor(&a, &a, &a) == BIGNUM_XOR_SUCCESS);
    assert(a.len == 0U);
    for (size_t i = 0U; i < BIGNUM_CAPACITY; ++i) assert(a.words[i] == 0U);
    puts("test_exact_aliases: PASSED");
}

/**
 * @brief Verifies invalid arguments preserve the destination object.
 * @details A destination filled with `0x5A` is used as a byte-level snapshot.
 * Three NULL-pointer calls must return `BIGNUM_XOR_ERROR_NULL_PTR`; an operand
 * with `len == BIGNUM_CAPACITY + 1` must return
 * `BIGNUM_XOR_ERROR_CAPACITY_EXCEEDED`. Both failures must leave the snapshot
 * unchanged. The final exact-alias call is a positive control and must return
 * success with the one-word oracle value 3. Partial-overlap rejection is
 * documented and tested in test_bignum_xor_extra.c.
 */
static void test_invalid_arguments_preserve_result(void)
{
    bignum_t result;
    bignum_t before;
    bignum_t a = { { 1U }, 1U };
    bignum_t b = { { 2U }, 1U };
    memset(&result, 0x5A, sizeof(result));
    before = result;
    assert(bignum_xor(NULL, &a, &b) == BIGNUM_XOR_ERROR_NULL_PTR);
    assert(bignum_xor(&result, NULL, &b) == BIGNUM_XOR_ERROR_NULL_PTR);
    assert(bignum_xor(&result, &a, NULL) == BIGNUM_XOR_ERROR_NULL_PTR);
    assert(memcmp(&result, &before, sizeof(result)) == 0);
    a.len = BIGNUM_CAPACITY + 1U;
    assert(bignum_xor(&result, &a, &b) == BIGNUM_XOR_ERROR_CAPACITY_EXCEEDED);
    assert(memcmp(&result, &before, sizeof(result)) == 0);
    result = (bignum_t){ { UINT64_C(1) }, 1U };
    b = (bignum_t){ { UINT64_C(2) }, 1U };
    assert(bignum_xor(&result, &result, &b) == BIGNUM_XOR_SUCCESS);
    assert(result.len == 1U && result.words[0] == UINT64_C(3));
    puts("test_invalid_arguments_preserve_result: PASSED");
}

/**
 * @brief Verifies XOR over every word at the fixed 2048-bit capacity.
 * @details Each word uses deterministic complementary bit patterns derived from
 * its index. The expected oracle is computed as `a.words[i] ^ b.words[i]` for
 * all `BIGNUM_CAPACITY` words, and the result must retain full capacity.
 */
static void test_full_capacity(void)
{
    bignum_t a;
    bignum_t b;
    bignum_t result;
    for (size_t i = 0U; i < BIGNUM_CAPACITY; ++i) {
        a.words[i] = UINT64_C(0xAAAAAAAAAAAAAAAA) ^ i;
        b.words[i] = UINT64_C(0x5555555555555555) ^ i;
    }
    a.len = BIGNUM_CAPACITY;
    b.len = BIGNUM_CAPACITY;
    assert(bignum_xor(&result, &a, &b) == BIGNUM_XOR_SUCCESS);
    assert(result.len == BIGNUM_CAPACITY);
    for (size_t i = 0U; i < BIGNUM_CAPACITY; ++i)
        assert(result.words[i] == (a.words[i] ^ b.words[i]));
    puts("test_full_capacity: PASSED");
}

/**
 * @brief Runs all deterministic bignum_xor contract cases.
 * @return EXIT_SUCCESS after every assertion passes; assertion failure aborts
 * with a non-zero process status.
 * @details The order groups positive arithmetic, normalization, aliasing,
 * negative validation, and full-capacity coverage before printing the suite
 * completion marker consumed by the Makefile test runner.
 */
int main(void)
{
    puts("--- Starting deterministic bignum_xor tests ---");
    test_basic_xor();
    test_zero_operand_and_normalization();
    test_max_length_zero_extension();
    test_high_zero_normalization();
    test_exact_aliases();
    test_invalid_arguments_preserve_result();
    test_full_capacity();
    puts("--- All deterministic bignum_xor tests passed ---");
    return 0;
}
