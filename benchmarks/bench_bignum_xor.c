/**
 * @file bench_bignum_xor.c
 * @brief Single-thread benchmark-framework entry point for bignum_xor.
 *
 * @details The entry point constructs the project-owned adapter, forwards the
 * complete argc/argv vector unchanged to benchmark_core_run_st, and maps the
 * named core status to the ISO C process exit contract. The core owns dataset
 * allocation, warm-up, timing, checksum publication, and the required
 * `benchmark=...` then `Benchmark finished.` protocol.
 *
 * @warning This executable is an x86-64/System V C11 boundary binary. It must
 * be linked with benchmark-core, bignum-core, and the bignum_xor implementation.
 * @par Thread safety benchmark-core performs the ST lifecycle; the adapter has no
 * shared mutable state.
 */
#include "bignum_xor_benchmark_adapter.h"

/**
 * @brief Runs the single-thread benchmark lifecycle.
 * @param[in] argc Number of command-line arguments supplied by the process.
 * @param[in] argv Borrowed argument vector; benchmark-core parses and does not retain it.
 * @return Process exit code 0 only for BENCHMARK_CORE_STATUS_SUCCESS; 1 for
 * adapter initialization or any core argument, callback, clock, or protocol failure.
 * @pre argc/argv are the standard ISO C process arguments.
 * @post A successful process publishes one complete benchmark protocol result.
 * @par Complexity Delegated to benchmark-core; per measured operation is O(B).
 */
int main(int argc, char **argv)
{
    benchmark_adapter_t adapter;
    bignum_xor_benchmark_status_t adapter_status =
        bignum_xor_benchmark_adapter_init(&adapter);
    if (adapter_status != BIGNUM_XOR_BENCHMARK_STATUS_SUCCESS) return 1;
    return benchmark_core_run_st(argc, argv, &adapter) == BENCHMARK_CORE_STATUS_SUCCESS
        ? 0 : 1;
}
