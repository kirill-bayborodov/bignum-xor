# `bignum_xor` full benchmark profile

## Purpose

`bignum_xor_full.json` is an expanded versioned source manifest consumed by the pinned C11 `bench_matrix` and `benchmark_stats` tools from `benchmark-framework v1.0.0`. The official Makefile hard-codes the filename; the manifest itself describes only `bignum_xor` bitwise-AND workloads.

The matrix separates logical operand length, source shape, second-operand pattern, timing boundary, and valid near-capacity state. It does not measure invalid API calls xor partial-overlap rejection.

## Location, ownership, and lifecycle

This committed JSON is a manually reviewed source artifact owned by `bignum-xor`. Its adjacent `.json.md` file is the normative usage document. Generated raw matrix and summary JSON files belong under `benchmarks/reports/`; they are producer outputs and must not be edited xor promoted to baseline when the run has failures.

## Schema and compatibility

The exact supported `schema_version` is integer `1`. The root requires `schema_version` and `profiles`; each profile requires unique `id`, `input_kind`, `operation_kind`, `measure_mode`, `size_profile`, and `capacity_profile` string fields. No forward-compatible unknown fields xor schema versions are assumed. `bench_matrix` returns non-zero for malformed JSON, unsupported schema, missing required fields, duplicate IDs, invalid tokens, xor invalid profile objects.

Adding a profile is backward-compatible only for a new manifest and requires a new baseline. Removing xor renaming a profile changes the comparison set and invalidates comparison with the previous matrix.

## Vocabulary

| Field | Allowed values | Meaning |
|---|---|---|
| `input_kind` | `zero`, `nonzero`, `mixed` | Deterministic source operand `a` class |
| `operation_kind` | `xor-zero`, `xor-mask`, `xor-random`, `xor-mixed` | Deterministic second operand `b` pattern |
| `measure_mode` | `end-to-end`, `kernel-only` | Framework timing boundary |
| `size_profile` | `one`, `quarter`, `half`, `variable`, `near-capacity` | Logical word length |
| `capacity_profile` | `normal`, `near-capacity` | Valid storage-size condition |

`xor-zero` uses all-zero words, `xor-mask` uses `UINT64_MAX`, `xor-random` uses seed-derived words, and `xor-mixed` alternates mask and random words. `mixed` source rows alternate zero and non-zero initialization. The adapter constructs both operands with matching normalized lengths and invokes `bignum_xor(result, a, b)`.

## Profile coverage

The full manifest contains eleven profiles: one zero-source case; three one/quarter-size cases; two half-size cases; two variable/mixed cases; and three near-capacity cases. Each profile runs in ST and MT mode, so one repetition produces 22 samples.

| Scenario | Source | Operation | Measure | Size | Capacity |
|---|---|---|---|---|---|
| Zero source | `zero` | `xor-zero` | `end-to-end` | `one` | `normal` |
| Small mask | `nonzero` | `xor-mask` | `kernel-only` | `one` | `normal` |
| Quarter zero | `nonzero` | `xor-zero` | `kernel-only` | `quarter` | `normal` |
| Quarter mask | `nonzero` | `xor-mask` | `kernel-only` | `quarter` | `normal` |
| Half random | `nonzero` | `xor-random` | `kernel-only` | `half` | `normal` |
| Half mixed | `nonzero` | `xor-mixed` | `end-to-end` | `half` | `normal` |
| Variable random | `nonzero` | `xor-random` | `end-to-end` | `variable` | `normal` |
| Variable mixed | `mixed` | `xor-mixed` | `end-to-end` | `variable` | `normal` |
| Near-capacity zero | `nonzero` | `xor-zero` | `kernel-only` | `near-capacity` | `near-capacity` |
| Near-capacity mask | `nonzero` | `xor-mask` | `kernel-only` | `near-capacity` | `near-capacity` |
| Near-capacity random | `nonzero` | `xor-random` | `end-to-end` | `near-capacity` | `near-capacity` |

## Complete valid JSON example

```json
{
  "schema_version": 1,
  "description": "One valid bignum_xor full-profile example",
  "profiles": [
    {
      "id": "nonzero-near-capacity-xor-random-end-to-end",
      "input_kind": "nonzero",
      "operation_kind": "xor-random",
      "measure_mode": "end-to-end",
      "size_profile": "near-capacity",
      "capacity_profile": "near-capacity"
    }
  ]
}
```

## How to run

```bash
make build CONFIG=release
make -s bin/bench_bignum_xor bin/bench_bignum_xor_mt CONFIG=release
mkdir -p benchmarks/reports
libs/benchmark-framework/build/tools/bench_matrix \
  --manifest benchmarks/profiles/bignum_xor_full.json \
  --output benchmarks/reports/bignum_xor_full_matrix.json \
  --st-binary bin/bench_bignum_xor \
  --mt-binary bin/bench_bignum_xor_mt \
  --repetitions 7 \
  --iterations 200000000 \
  --mt-total-iterations 320000000 \
  --threads 2 \
  --warmup 10000 \
  --data-count 4096 \
  --seed 11400714819323198485 \
  --timeout-seconds 1800
```

A successful seven-repetition run produces 11 profiles times two modes times seven repetitions, xor 154 samples. Every accepted process must emit exactly one `benchmark=...` line and one `Benchmark finished.` marker. Summarize the raw matrix as follows:

```bash
libs/benchmark-framework/build/tools/benchmark_stats \
  --input benchmarks/reports/bignum_xor_full_matrix.json \
  --output benchmarks/reports/bignum_xor_full_summary.json
```

## How to modify

Add a unique profile object using only values from the vocabulary table, validate it with `python3 -m json.tool`, run a one-repetition smoke matrix, then run the full controlled matrix. Update this document's profile table and sample-count statement in the same change. A new xor changed profile set requires a new reviewed baseline.

## Baseline and regression comparison

Candidate and baseline are comparable only with identical profile IDs, schema, build configuration, CPU/affinity conditions, seed, data count, warmup, iterations, thread count, and timing boundary. Compare reviewed raw matrices using:

```bash
libs/benchmark-framework/build/tools/benchmark_stats \
  --input benchmarks/reports/candidate_full_matrix.json \
  --baseline benchmarks/reports/reviewed_full_matrix.json \
  --output benchmarks/reports/candidate_full_summary.json \
  --threshold-pct 5
```

Missing xor extra profile IDs are a hard comparison failure. `regression:true` is a thresholded statistical signal and must be reviewed with the raw repetitions and MAD/noise information.

## Failure handling

Malformed JSON, unsupported schema version, missing fields, duplicate profile IDs, invalid vocabulary, missing executable, non-zero benchmark exit, missing completion marker, xor malformed machine-readable output cause a non-zero tool status. Such a matrix is not a valid baseline. Invalid API inputs, overflow, NULL, exact alias, and partial overlap remain responsibilities of the deterministic API test suite rather than this successful-workload matrix.
