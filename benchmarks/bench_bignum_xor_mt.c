/**
 * @file bench_bignum_xor_mt.c
 * @brief Multi-thread benchmark-framework entry point for bignum_xor.
 *
 * @details The entry point constructs the same project-owned adapter as the ST
 * binary and forwards argc/argv to benchmark_core_run_mt. The core validates
 * thread count and total-iterations divisibility, creates independent worker
 * state, synchronizes lifecycle, combines checksums, and publishes one
 * `benchmark=...` line followed by `Benchmark finished.`.
 *
 * @warning Each worker receives an independent bignum_t state. The adapter must
 * not be changed to share mutable operand storage between workers.
 * @par Thread safety Thread safety is provided by benchmark-core's per-worker state
 * and synchronization boundary; the entry point itself owns no shared state.
 */
#include "bignum_xor_benchmark_adapter.h"

/**
 * @brief Runs the multi-thread benchmark lifecycle.
 * @param[in] argc Number of command-line arguments.
 * @param[in] argv Borrowed argument vector parsed by benchmark-core.
 * @return Process exit code 0 only for BENCHMARK_CORE_STATUS_SUCCESS; 1 for
 * adapter initialization or any core argument, thread, callback, clock, or protocol failure.
 * @pre `--threads` is positive and `--total-iterations` is divisible by it when supplied.
 * @post Success publishes one aggregate benchmark protocol result after all workers join.
 * @par Complexity O(T * (B + I/T)) time and O(T * B) core-managed state, where T is
 * worker count, B is bignum capacity, and I is total measured iterations.
 */
int main(int argc, char **argv)
{
    benchmark_adapter_t adapter;
    bignum_xor_benchmark_status_t adapter_status =
        bignum_xor_benchmark_adapter_init(&adapter);
    if (adapter_status != BIGNUM_XOR_BENCHMARK_STATUS_SUCCESS) return 1;
    return benchmark_core_run_mt(argc, argv, &adapter) == BENCHMARK_CORE_STATUS_SUCCESS
        ? 0 : 1;
}
