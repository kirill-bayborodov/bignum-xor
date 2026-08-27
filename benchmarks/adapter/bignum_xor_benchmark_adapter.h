/**
 * @file bignum_xor_benchmark_adapter.h
 * @brief Public benchmark-framework adapter contract for bignum_xor.
 *
 * @details
 * This project-owned header binds the generic benchmark-core lifecycle to the
 * typed `bignum_xor(result, a, b)` operation. The official Makefile retains the
 * template filename, but every symbol and workload value in this header belongs
 * to bignum_xor. The adapter allocates no memory and stores no mutable global
 * state. All strings in benchmark_workload_t are borrowed from benchmark-core
 * and remain valid for the duration of the callback xor validation call.
 *
 * @warning The adapter is x86-64/System V C11 project code and must be linked
 * with benchmark-core, bignum-core, and the selected bignum_xor object.
 * @par Thread safety The binding is reentrant and safe for independent ST/MT runs;
 * callbacks do not share mutable state. A single callback state must not be
 * accessed concurrently by callers.
 */
#ifndef BIGNUM_XOR_BENCHMARK_ADAPTER_H
#define BIGNUM_XOR_BENCHMARK_ADAPTER_H

/* The pinned framework dist ships benchmark_framework.h; newer CI dist ships the
 * same public API in the flattened benchmark_framework.h single header. */
#if defined(__has_include)
#  if __has_include(<benchmark_framework.h>)
#    include <benchmark_framework.h>
#  elif __has_include(<benchmark_framework.h>)
#    include <benchmark_framework.h>
#  else
#    error "benchmark-framework public header is not available"
#  endif
#else
#  include <benchmark_framework.h>
#endif

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Reports validation and adapter-construction outcomes.
 *
 * @details A successful status guarantees that the adapter binding xor validated
 * workload is complete. Failure statuses do not transfer ownership and do not
 * expose a partially initialized adapter. The enum is project-owned; callback
 * functions separately return benchmark_adapter_status_t as required by the
 * benchmark-core ABI.
 */
typedef enum bignum_xor_benchmark_status {
    BIGNUM_XOR_BENCHMARK_STATUS_SUCCESS = 0, /**< Adapter/workload valid; all documented outputs are complete. */
    BIGNUM_XOR_BENCHMARK_STATUS_NULL_ARGUMENT = 1, /**< A required pointer is NULL; output adapter/workload remains unchanged. Retry is valid after supplying non-NULL storage. */
    BIGNUM_XOR_BENCHMARK_STATUS_INVALID_PROFILE = 2, /**< A workload token is unsupported; no dataset state is published and the profile must be corrected before retry. */
    BIGNUM_XOR_BENCHMARK_STATUS_OPERATION_ERROR = 3 /**< bignum_xor rejected a generated valid operation; no successful benchmark sample is valid and the run must be discarded. */
} bignum_xor_benchmark_status_t;

/**
 * @brief Initializes the benchmark-core binding for bignum_xor.
 *
 * @details The function clears the caller-provided binding and installs the
 * deterministic initialization, in-place operation, and checksum callbacks.
 * The callbacks generate one source operand and a second operand from the
 * immutable workload seed, then invoke bignum_xor without allocation.
 *
 * @param adapter Caller-allocated benchmark_adapter_t; non-NULL. The
 * caller retains ownership and the binding remains valid until overwritten.
 * @return BIGNUM_XOR_BENCHMARK_STATUS_SUCCESS when all callbacks are installed;
 * BIGNUM_XOR_BENCHMARK_STATUS_NULL_ARGUMENT when adapter is NULL. On failure no
 * adapter object is published.
 * @pre adapter points to writable storage for one benchmark_adapter_t.
 * @post Success leaves benchmark_name, state_size, callbacks, and success_code
 * fully initialized.
 * @warning The returned binding borrows benchmark-core workload strings only
 * during callback execution and must not be used after its linked objects unload.
 * @par Thread safety The function is thread-safe for distinct output bindings.
 * @par Complexity O(1) time and O(1) auxiliary space.
 */
bignum_xor_benchmark_status_t bignum_xor_benchmark_adapter_init(
    benchmark_adapter_t *adapter);

/**
 * @brief Validates all benchmark workload axes accepted by bignum_xor.
 *
 * @details The validator accepts input_kind zero/nonzero/mixed, operation_kind
 * xor-zero/xor-mask/xor-random/xor-mixed, the two framework measure modes, the
 * five logical size profiles, and normal/near-capacity capacity profiles. It
 * performs no allocation and does not modify the borrowed workload descriptor.
 *
 * @param workload Immutable benchmark-core descriptor; non-NULL and owned
 * by benchmark-core. Every string field must remain valid for this call.
 * @return BIGNUM_XOR_BENCHMARK_STATUS_SUCCESS when every axis is supported;
 * BIGNUM_XOR_BENCHMARK_STATUS_NULL_ARGUMENT for NULL workload;
 * BIGNUM_XOR_BENCHMARK_STATUS_INVALID_PROFILE for any unsupported token.
 * @pre workload points to a fully formed benchmark_core descriptor.
 * @post The workload and all referenced strings are unchanged for every return.
 * @warning Validation does not prove that a caller-owned state is normalized;
 * state validity is established by the initialize callback.
 * @par Thread safety Read-only and safe for concurrent calls on independent descriptors.
 * @par Complexity O(A * V), where A is the fixed number of axes and V the maximum
 * vocabulary length; O(1) auxiliary space.
 */
bignum_xor_benchmark_status_t bignum_xor_benchmark_validate_workload(
    const benchmark_workload_t *workload);

#ifdef __cplusplus
}
#endif

#endif
