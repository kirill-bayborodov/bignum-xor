# bignum-xor

[![CI](https://github.com/kirill-bayborodov/bignum-xor/actions/workflows/ci.yml/badge.svg)](https://github.com/kirill-bayborodov/bignum-xor/actions/workflows/ci.yml)

`bignum-xor` is a standalone C/ASM module that computes `result = a ^ b` for normalized 2048-bit `bignum_t` objects. The production path is x86-64 YASM conforming to the System V AMD64 ABI; the C11 implementation is retained as an independent reference path.

The operation performs no dynamic allocation inside the library function. It validates all pointers, operand lengths, and complete-object overlap before mutating `result`. Exact aliases are supported, partial overlap is rejected, and the destination tail is cleared after a successful operation.

## Distribution

The module is part of the `bignum-lib` family. The official Makefile is retained without modification. `bignum-core` is supplied as a Git submodule; the benchmark framework is a CI-provided vendor distribution placed in `libs/benchmark-framework/dist`.

| Dependency | Path | Purpose |
|---|---|---|
| `bignum-core` | `libs/bignum-core` | Defines `bignum_t` and `BIGNUM_CAPACITY` |
| `benchmark-framework` distribution | `libs/benchmark-framework/dist` | Provides the benchmark library, headers, matrix tools, profiles, and documentation |

Clone the source dependency and prepare the vendor artifact:

```bash
git clone --recurse-submodules https://github.com/kirill-bayborodov/bignum-xor.git
cd bignum-xor
git submodule update --init --recursive
# CI downloads the latest compatible benchmark-framework `dist` artifact here.
# The artifact must provide the public framework header/library, tools, and profiles;
# the current latest artifact may flatten the header/library at dist root.
```

## Features

- **Typed API:** explicit success, NULL-pointer, capacity, and partial-overlap statuses.
- **Dual implementation:** optimized x86-64 YASM and portable C11 reference path.
- **Normalized output:** leading zero words are removed and zero is represented by `len == 0`.
- **Exact aliases:** `result == a`, `result == b`, and `a == b` are valid.
- **Overlap safety:** any partial overlap among complete `bignum_t` objects is rejected before writes.
- **Full-tail clearing:** all words outside the logical result are set to zero.
- **No library allocation:** the C path uses a fixed stack temporary; ASM uses registers and the destination object.
- **Thread safety:** concurrent calls are safe for separate, non-overlapping objects.
- **Reproducible verification:** deterministic, fuzz, canary, overlap, MT, runner, and adapter tests are included.
- **Template benchmarks:** ST and MT runners report machine-readable timing and checksums.

## Dependencies and tools

| Tool | Purpose |
|---|---|
| `make` | Build, test, lint, benchmark, install, and distribution targets |
| `gcc` | C11 compilation and linking |
| `yasm` | x86-64 assembly compilation |
| `cppcheck` | Static analysis |
| `valgrind` | Leak and Helgrind diagnostics |
| `perf` | Optional performance-counter workflow |
| `pthread` | Multithreaded tests and benchmarks |

## API

The public API is declared in `include/bignum_xor.h`:

```c
typedef enum {
    BIGNUM_XOR_SUCCESS                 =  0,
    BIGNUM_XOR_ERROR_NULL_PTR          = -1,
    BIGNUM_XOR_ERROR_CAPACITY_EXCEEDED = -2,
    BIGNUM_XOR_ERROR_BUFFER_OVERLAP    = -3
} bignum_xor_status_t;

bignum_xor_status_t bignum_xor(
    bignum_t *result,
    const bignum_t *a,
    const bignum_t *b);
```

### Contract

| Condition | Return value | Result behavior |
|---|---|---|
| Valid pointers, valid lengths, no partial overlap | `BIGNUM_XOR_SUCCESS` | `result` becomes normalized `a ^ b`; its complete tail is cleared |
| Any pointer is `NULL` | `BIGNUM_XOR_ERROR_NULL_PTR` | No object is accessed |
| `a->len` or `b->len` exceeds `BIGNUM_CAPACITY` | `BIGNUM_XOR_ERROR_CAPACITY_EXCEEDED` | `result` remains unchanged |
| Any partial overlap among the three objects | `BIGNUM_XOR_ERROR_BUFFER_OVERLAP` | `result` remains unchanged |
| Exact pointer alias | `BIGNUM_XOR_SUCCESS` | Exact aliases are supported |

The representation is little-endian by 64-bit words. The output length is at most `max(a->len, b->len)` and is reduced while its highest words are zero. If either operand has `len == 0`, the result is normalized zero.

A partial overlap means that two distinct complete `bignum_t` memory ranges intersect. Exact pointer equality is explicitly allowed. The overlap rule is checked before any destination write.

Example:

```c
#include "bignum_xor.h"

bignum_t a = { { UINT64_C(0xFFFF) }, 1U };
bignum_t b = { { UINT64_C(0x0F0F) }, 1U };
bignum_t result;

bignum_xor_status_t status = bignum_xor(&result, &a, &b);
/* result.words[0] == 0xF0F0 and result.len == 1 */
```

## Build and test

Build the release object and dependencies:

```bash
make build CONFIG=release
```

The production object is generated at `build/bignum_xor.o`.

Run the complete suite:

```bash
make test CONFIG=release
```

Expected summary:

```text
=== Summary: 0 / 5 failed ===
```

Run the independent C reference path:

```bash
make clean
make test CONFIG=release USE_ASM=no
```

Run static analysis and diagnostics:

```bash
make lint
make test_sanitize CONFIG=debug SAN=address
make test_sanitize CONFIG=debug SAN=undefined
make test_helgrind CONFIG=release
```

| Test file | Scope |
|---|---|
| `tests/test_bignum_xor.c` | Basic values, zero, normalization, aliases, invalid arguments, and full capacity |
| `tests/test_bignum_xor_extra.c` | Independent XOR model, 10,000-case fuzz, canaries, source preservation, and partial-overlap diagnostics |
| `tests/test_bignum_xor_mt.c` | Concurrent independent-object calls |
| `tests/test_bignum_xor_runner.c` | Integration smoke test |
| `tests/benchmark_adapter/test_bignum_xor_benchmark_adapter.c` | Deterministic adapter validation: vocabulary statuses, fixed-seed state reproducibility, callback success/checksum, NULL/invalid-profile errors, and malformed-length operation error |

## Benchmarks

The benchmark entrypoints are `benchmarks/bench_bignum_xor.c` and `benchmarks/bench_bignum_xor_mt.c`. Both construct the project-owned adapter and delegate lifecycle, warm-up, timing, dataset copies, checksum publication, and protocol validation to the CI-provided benchmark-framework distribution.

### Single-thread and multithread CLI

The core accepts the same workload vocabulary in both modes:

```text
--input-kind zero|nonzero|mixed
--operation-kind xor-zero|xor-mask|xor-random|xor-mixed
--measure-mode end-to-end|kernel-only
--size-profile one|quarter|half|variable|near-capacity
--capacity-profile normal|near-capacity
--warmup N --data-count N --seed N
```

ST additionally requires `--iterations N`; MT requires `--threads N` and `--total-iterations N`, where total iterations must be divisible by the thread count. A complete reproducible ST example is:

```bash
./bin/bench_bignum_xor \
  --input-kind nonzero --operation-kind xor-random \
  --measure-mode kernel-only --size-profile quarter \
  --capacity-profile normal --iterations 100000 \
  --warmup 1000 --data-count 4096 --seed 123456789
```

The corresponding MT example is:

```bash
./bin/bench_bignum_xor_mt \
  --input-kind mixed --operation-kind xor-mixed \
  --measure-mode end-to-end --size-profile variable \
  --capacity-profile normal --threads 2 \
  --total-iterations 100000 --warmup 1000 \
  --data-count 4096 --seed 123456789
```

`xor-zero`, `xor-mask`, `xor-random`, and `xor-mixed` select deterministic second-operand patterns. Successful output includes workload metadata, iterations, successful calls, checksum, elapsed seconds, and `ns_per_call`, followed by exactly one `Benchmark finished.` marker. Performance comparisons must keep seed, data count, profile, compiler configuration, CPU affinity, thread count, and measurement mode constant.

### Benchmark-framework matrix

The official Makefile derives adapter and profile filenames from `LIB_NAME`, which is computed from the repository directory. For this repository, the generated paths are `benchmarks/adapter/bignum_xor_benchmark_adapter.[ch]` and `benchmarks/profiles/bignum_xor_standard.json`/`bignum_xor_full.json`. The standard eight-profile matrix can be executed as follows:

```bash
make build CONFIG=release
make -s bin/bench_bignum_xor bin/bench_bignum_xor_mt CONFIG=release
mkdir -p benchmarks/reports
libs/benchmark-framework/build/tools/bench_matrix \
  --manifest benchmarks/profiles/bignum_xor_standard.json \
  --output benchmarks/reports/bignum_xor_standard_matrix.json \
  --st-binary bin/bench_bignum_xor --mt-binary bin/bench_bignum_xor_mt \
  --repetitions 1 --iterations 1001 --mt-total-iterations 2000 \
  --threads 2 --warmup 10 --data-count 32 \
  --seed 11400714819323198485 --timeout-seconds 30
```

The expected result is 16 successful samples and zero failures. Full schema, profile vocabulary, modification rules, and failure handling are documented in the adjacent files `benchmarks/profiles/bignum_xor_standard.json.md` and `benchmarks/profiles/bignum_xor_full.json.md`. These companion documents are selected from the same `LIB_NAME`-derived basename.

## Perf workflow

When `/usr/local/bin/perf` and the required permissions are available:

```bash
make bench_full CONFIG=release REPORT_NAME=baseline PERF_RUNS=5 KEEP_PERF=1
make bench_stat_st CONFIG=release REPORT_NAME=baseline_st PERF_RUNS=5
make bench_stat_mt CONFIG=release REPORT_NAME=baseline_mt MT_THREADS=2 MT_CPU_LIST=0-1 PERF_RUNS=5
```

If the sandbox does not expose Linux performance counters, run the standalone benchmark binaries directly and collect perf reports on a permitted host.

Parameterized matrix reports use the official profile targets:

```bash
make bench_matrix CONFIG=release REPORT_NAME=baseline PERF_RUNS=5
```

Reports are written under `benchmarks/reports/`.

## Installation and distribution

```bash
make install CONFIG=release
make dist CONFIG=release
make clean
```

The official Makefile must not be edited without explicit authorization.

## Linking

```bash
make build CONFIG=release
gcc your_app.c build/bignum_xor.o \
  -I./include -I./libs/bignum-core/include \
  -o your_app -no-pie
```

The application must use the System V AMD64 ABI and link the dependency objects required by the selected distribution.

## Contributing

Contributions must preserve the typed status API, validation-before-mutation guarantee, exact alias behavior, partial-overlap rejection, normalized representation, complete tail clearing, no-allocation property, and thread-safety contract. New behavior requires deterministic and independent-reference tests. The official Makefile must remain unmodified.

## License

This project is licensed under the MIT License. See [LICENSE](LICENSE) for details.
