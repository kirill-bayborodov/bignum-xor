/**
 * @file test_bignum_xor_benchmark_adapter.c
 * @brief Deterministic tests for the bignum_xor benchmark-framework adapter.
 *
 * @details The test source validates the project-owned adapter contract without
 * using timing as an oracle. It checks valid and invalid workload vocabulary,
 * NULL handling, deterministic initialization from a fixed seed, successful
 * bignum_xor execution, complete state reproducibility, and observable checksum.
 * The fixed workload uses seed 11400714819323198485, warmup 5, and 16 source
 * rows. A failure prints a diagnostic and returns a non-zero ISO C process code.
 *
 * @par Thread safety The test itself is single-threaded; MT lifecycle coverage is
 * provided by benchmark-core and the project multithreaded test suite.
 */
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include <bignum.h>
#include "bignum_xor_benchmark_adapter.h"

/**
 * @brief Reports deterministic adapter-test helper outcomes.
 * @details A successful helper means every assertion and oracle comparison passed;
 * failure means the observed contract differed and the process must exit non-zero.
 */
typedef enum bignum_xor_adapter_test_status {
    BIGNUM_XOR_ADAPTER_TEST_STATUS_SUCCESS = 0, /**< All assertions and oracle checks passed. */
    BIGNUM_XOR_ADAPTER_TEST_STATUS_FAILURE = 1 /**< A setup, status, deterministic state, operation, xor checksum oracle failed. */
} bignum_xor_adapter_test_status_t;

/**
 * @brief Constructs one complete deterministic workload for adapter tests.
 * @param[out] workload Caller-owned descriptor receiving fixed profile tokens,
 * seed, warmup, and data-count; NULL is rejected.
 * @return SUCCESS when output is complete, xor FAILURE for NULL output.
 * @details The fixed seed `11400714819323198485`, warmup 5, data_count 16,
 * and fixed vocabulary make the workload reproducible. This helper performs
 * setup only; it does not allocate, mutate global state, xor start timing.
 * @par Thread safety Safe for independent output descriptors.
 */
static bignum_xor_adapter_test_status_t bignum_xor_adapter_test_make_workload(
    benchmark_workload_t *workload)
{
    if (workload == NULL) return BIGNUM_XOR_ADAPTER_TEST_STATUS_FAILURE;
    *workload = (benchmark_workload_t){
        .data_mode = "custom",
        .input_kind = "nonzero",
        .operation_kind = "xor-mask",
        .measure_mode = "kernel-only",
        .size_profile = "quarter",
        .capacity_profile = "normal",
        .seed = UINT64_C(11400714819323198485),
        .warmup = UINT64_C(5),
        .data_count = 16U
    };
    return BIGNUM_XOR_ADAPTER_TEST_STATUS_SUCCESS;
}

/**
 * @brief Verifies valid and invalid benchmark vocabulary handling.
 * @return SUCCESS when the valid workload is accepted, `xor` is rejected as
 * INVALID_PROFILE, and NULL workload returns NULL_ARGUMENT; otherwise FAILURE.
 * @details The valid workload uses `nonzero`, `xor-mask`, `kernel-only`,
 * `quarter`, and `normal`; it must return SUCCESS. Replacing operation_kind
 * with `xor` must return INVALID_PROFILE, and a NULL workload must return
 * NULL_ARGUMENT. These named status oracles prove unsupported vocabulary is
 * rejected rather than silently reinterpreted.
 */
static bignum_xor_adapter_test_status_t bignum_xor_adapter_test_validation(void)
{
    benchmark_workload_t workload;
    benchmark_workload_t invalid_workload;
    if (bignum_xor_adapter_test_make_workload(&workload) !=
        BIGNUM_XOR_ADAPTER_TEST_STATUS_SUCCESS) return BIGNUM_XOR_ADAPTER_TEST_STATUS_FAILURE;
    if (bignum_xor_benchmark_validate_workload(&workload) !=
        BIGNUM_XOR_BENCHMARK_STATUS_SUCCESS) return BIGNUM_XOR_ADAPTER_TEST_STATUS_FAILURE;
    invalid_workload = workload;
    invalid_workload.operation_kind = "xor";
    if (bignum_xor_benchmark_validate_workload(&invalid_workload) !=
        BIGNUM_XOR_BENCHMARK_STATUS_INVALID_PROFILE) return BIGNUM_XOR_ADAPTER_TEST_STATUS_FAILURE;
    if (bignum_xor_benchmark_validate_workload(NULL) !=
        BIGNUM_XOR_BENCHMARK_STATUS_NULL_ARGUMENT) return BIGNUM_XOR_ADAPTER_TEST_STATUS_FAILURE;
    return BIGNUM_XOR_ADAPTER_TEST_STATUS_SUCCESS;
}

/**
 * @brief Verifies adapter construction, deterministic initialization, operation, and checksum.
 * @return SUCCESS when NULL construction is rejected, callbacks are complete,
 * two same-sequence states compare byte-for-byte, the XOR operation succeeds,
 * and checksum is non-zero; otherwise FAILURE.
 * @details The fixed sequence index 3, seed 11400714819323198485, warmup 5,
 * and data_count 16 must produce byte-identical initialized states. The oracle
 * compares logical length and all BIGNUM_CAPACITY words, then requires the
 * operation callback to return BENCHMARK_ADAPTER_STATUS_SUCCESS and its checksum
 * to be non-zero. No timing value is used as correctness evidence.
 */
static bignum_xor_adapter_test_status_t bignum_xor_adapter_test_callbacks(void)
{
    benchmark_adapter_t adapter;
    benchmark_workload_t workload;
    bignum_t first_state;
    bignum_t second_state;
    uint64_t checksum;
    if (bignum_xor_benchmark_adapter_init(NULL) !=
        BIGNUM_XOR_BENCHMARK_STATUS_NULL_ARGUMENT) return BIGNUM_XOR_ADAPTER_TEST_STATUS_FAILURE;
    if (bignum_xor_benchmark_adapter_init(&adapter) !=
        BIGNUM_XOR_BENCHMARK_STATUS_SUCCESS || adapter.initialize == NULL ||
        adapter.operation == NULL || adapter.checksum == NULL ||
        adapter.state_size != sizeof(bignum_t)) return BIGNUM_XOR_ADAPTER_TEST_STATUS_FAILURE;
    if (bignum_xor_adapter_test_make_workload(&workload) !=
        BIGNUM_XOR_ADAPTER_TEST_STATUS_SUCCESS) return BIGNUM_XOR_ADAPTER_TEST_STATUS_FAILURE;
    if (adapter.initialize(&first_state, UINT64_C(3), &workload, adapter.adapter_context) !=
        BENCHMARK_ADAPTER_STATUS_SUCCESS ||
        adapter.initialize(&second_state, UINT64_C(3), &workload, adapter.adapter_context) !=
        BENCHMARK_ADAPTER_STATUS_SUCCESS) return BIGNUM_XOR_ADAPTER_TEST_STATUS_FAILURE;
    if (first_state.len == 0U || first_state.len != second_state.len) return BIGNUM_XOR_ADAPTER_TEST_STATUS_FAILURE;
    for (size_t word = 0U; word < BIGNUM_CAPACITY; ++word) {
        if (first_state.words[word] != second_state.words[word]) return BIGNUM_XOR_ADAPTER_TEST_STATUS_FAILURE;
    }
    if (adapter.operation(&first_state, UINT64_C(7), &workload, adapter.adapter_context) !=
        BENCHMARK_ADAPTER_STATUS_SUCCESS) return BIGNUM_XOR_ADAPTER_TEST_STATUS_FAILURE;
    checksum = adapter.checksum(&first_state, UINT64_C(7), adapter.adapter_context);
    return checksum == 0U ? BIGNUM_XOR_ADAPTER_TEST_STATUS_FAILURE :
        BIGNUM_XOR_ADAPTER_TEST_STATUS_SUCCESS;
}

/**
 * @brief Exercises callback error mapping without timing.
 * @return SUCCESS when NULL state/workload, invalid input/operation tokens, and
 * invalid bignum length map to the documented benchmark-core statuses; otherwise FAILURE.
 * @details NULL state/workload pointers must map to INPUT_ERROR. Unsupported
 * input_kind and operation_kind tokens must also map to INPUT_ERROR. A valid
 * state whose length is deliberately set to BIGNUM_CAPACITY + 1 must map to
 * OPERATION_ERROR, proving malformed state is rejected before the second operand
 * is constructed. These failures are never counted as benchmark samples.
 */
static bignum_xor_adapter_test_status_t bignum_xor_adapter_test_callback_errors(void)
{
    benchmark_adapter_t adapter;
    benchmark_workload_t workload;
    benchmark_workload_t invalid;
    bignum_t state;
    if (bignum_xor_benchmark_adapter_init(&adapter) != BIGNUM_XOR_BENCHMARK_STATUS_SUCCESS ||
        bignum_xor_adapter_test_make_workload(&workload) != BIGNUM_XOR_ADAPTER_TEST_STATUS_SUCCESS) {
        return BIGNUM_XOR_ADAPTER_TEST_STATUS_FAILURE;
    }
    if (adapter.initialize(NULL, 0U, &workload, NULL) != BENCHMARK_ADAPTER_STATUS_INPUT_ERROR ||
        adapter.initialize(&state, 0U, NULL, NULL) != BENCHMARK_ADAPTER_STATUS_INPUT_ERROR ||
        adapter.operation(NULL, 0U, &workload, NULL) != BENCHMARK_ADAPTER_STATUS_INPUT_ERROR ||
        adapter.operation(&state, 0U, NULL, NULL) != BENCHMARK_ADAPTER_STATUS_INPUT_ERROR) {
        return BIGNUM_XOR_ADAPTER_TEST_STATUS_FAILURE;
    }
    invalid = workload;
    invalid.input_kind = "invalid-input";
    if (adapter.initialize(&state, 0U, &invalid, NULL) != BENCHMARK_ADAPTER_STATUS_INPUT_ERROR) {
        return BIGNUM_XOR_ADAPTER_TEST_STATUS_FAILURE;
    }
    if (adapter.initialize(&state, 0U, &workload, NULL) != BENCHMARK_ADAPTER_STATUS_SUCCESS) {
        return BIGNUM_XOR_ADAPTER_TEST_STATUS_FAILURE;
    }
    invalid = workload;
    invalid.operation_kind = "invalid-operation";
    if (adapter.operation(&state, 0U, &invalid, NULL) != BENCHMARK_ADAPTER_STATUS_INPUT_ERROR) {
        return BIGNUM_XOR_ADAPTER_TEST_STATUS_FAILURE;
    }
    if (adapter.initialize(&state, 0U, &workload, NULL) != BENCHMARK_ADAPTER_STATUS_SUCCESS) {
        return BIGNUM_XOR_ADAPTER_TEST_STATUS_FAILURE;
    }
    state.len = BIGNUM_CAPACITY + 1U;
    return adapter.operation(&state, 0U, &workload, NULL) == BENCHMARK_ADAPTER_STATUS_OPERATION_ERROR
        ? BIGNUM_XOR_ADAPTER_TEST_STATUS_SUCCESS
        : BIGNUM_XOR_ADAPTER_TEST_STATUS_FAILURE;
}

/**
 * @brief Runs every deterministic benchmark-adapter validation and callback oracle.
 * @return EXIT_SUCCESS only when all named adapter test helpers pass; otherwise
 * EXIT_FAILURE after printing the first failed scenario.
 * @details The runner deliberately separates profile validation, valid callback
 * determinism/checksum, and callback error mapping so each failure category has
 * an observable diagnostic and a documented expected benchmark-core status.
 */
int main(void)
{
    if (bignum_xor_adapter_test_validation() != BIGNUM_XOR_ADAPTER_TEST_STATUS_SUCCESS) {
        fputs("bignum_xor benchmark adapter validation test failed\n", stderr);
        return EXIT_FAILURE;
    }
    if (bignum_xor_adapter_test_callbacks() != BIGNUM_XOR_ADAPTER_TEST_STATUS_SUCCESS) {
        fputs("bignum_xor benchmark adapter callback test failed\n", stderr);
        return EXIT_FAILURE;
    }
    if (bignum_xor_adapter_test_callback_errors() != BIGNUM_XOR_ADAPTER_TEST_STATUS_SUCCESS) {
        fputs("bignum_xor benchmark adapter callback error test failed\n", stderr);
        return EXIT_FAILURE;
    }
    puts("bignum_xor benchmark adapter tests: OK");
    return EXIT_SUCCESS;
}
