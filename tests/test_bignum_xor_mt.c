/**
 * @file test_bignum_xor_mt.c
 * @brief Multithreaded independence and reentrancy tests for bignum_xor.
 *
 * @details Eight pthread workers execute 10,000 iterations each against private
 * stack operands and a private result. Every iteration checks the typed success
 * status and both word-level XOR values. The test is an independence oracle: no
 * object is shared between workers, so a failure indicates an unsafe global
 * state, incorrect reentrancy, or an arithmetic error rather than a deliberate
 * data race. Thread creation/join failures and worker mismatches are asserted;
 * any failure exits non-zero.
 *
 * @par Synchronization boundary pthread_create and pthread_join synchronize the
 * worker lifecycle. The library call itself receives disjoint objects and must
 * not require a global lock or mutable global state.
 * @par Ownership Each worker owns its stack operands and its `worker_data_t`
 * slot; the main thread owns the array and reads a slot only after join.
 */
/* ------------------------------------------------------------------ */
#include <assert.h>
#include <pthread.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>

#include "bignum_xor.h"

/**
 * @brief Stores one worker identifier and its failure flag.
 * @par Invariant `failed == 0` until the worker observes a status or word
 * mismatch; the main thread reads it only after pthread_join returns.
 */
typedef struct {
    size_t id; /**< Stable worker identifier used to vary the first operand. */
    int failed; /**< Non-zero when this worker observes a contract mismatch. */
} worker_data_t;

/**
 * @brief Executes the fixed independent-object XOR workload for one worker.
 * @param[in,out] opaque Pointer to the caller-owned worker_data_t slot.
 * @return NULL after 10,000 successful checks or at the first mismatch.
 * @details The first operand varies by worker id and iteration; the second is
 * fixed. The exact oracle is `a.words[i] ^ b.words[i]` for both active words,
 * and the required API status is `BIGNUM_XOR_SUCCESS`.
 */
static void *worker(void *opaque)
{
    worker_data_t *data = opaque;
    for (size_t iteration = 0; iteration < 10000U; ++iteration) {
        bignum_t a = { { UINT64_MAX - data->id, iteration + 1U }, 2U };
        bignum_t b = { { UINT64_C(0x5555555555555555), UINT64_MAX }, 2U };
        bignum_t result;
        if (bignum_xor(&result, &a, &b) != BIGNUM_XOR_SUCCESS ||
            result.words[0] != (a.words[0] ^ b.words[0]) ||
            result.words[1] != (a.words[1] ^ b.words[1])) {
            data->failed = 1;
            return NULL;
        }
    }
    return NULL;
}

/**
 * @brief Creates, joins, and validates the fixed eight-worker test group.
 * @return EXIT_SUCCESS after all workers complete without a mismatch;
 * assertion failure terminates with a non-zero status.
 * @details The main thread does not inspect worker flags before join, making
 * the join the explicit happens-before boundary for result publication.
 */
int main(void)
{
    enum { THREAD_COUNT = 8 };
    pthread_t threads[THREAD_COUNT];
    worker_data_t data[THREAD_COUNT];
    for (size_t i = 0; i < THREAD_COUNT; ++i) {
        data[i] = (worker_data_t){ i, 0 };
        assert(pthread_create(&threads[i], NULL, worker, &data[i]) == 0);
    }
    for (size_t i = 0; i < THREAD_COUNT; ++i) {
        assert(pthread_join(threads[i], NULL) == 0);
        assert(data[i].failed == 0);
    }
    puts("--- Multithreaded bignum_xor test passed ---");
    return 0;
}
