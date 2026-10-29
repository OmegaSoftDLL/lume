# 35 — Public economic benchmark protocol 0.1

**Status: shipped as a bounded Lume-only harness, not as a cross-model or cross-language result.**
This increment for #337 executes real compilation, QF_LIA proof, and native target execution. It does
not call an AI provider, use credentials, or infer token and monetary costs. Repository candidates are
harness fixtures, not recorded model successes and not public benchmark scores.

## What protocol 0.1 measures

`scripts/economic_benchmark.py` reads a `lume-economic-benchmark-suite/0.1` manifest and executes each
candidate attempt in order. It stops a task after its first `PASS`. An attempt is `PASS` only when all
of these checks succeed on the current checkout:

1. `lume build --target x86_64-linux-elf` exits zero and creates a native artifact;
2. `lume prove --json` exits zero, returns valid JSON with at least one obligation, and every
   obligation has status `proven` (`runtime_checked`, `refuted`, missing, and malformed results fail);
3. the native artifact exits zero before the timeout and its stdout exactly matches the task's
   expected UTF-8 bytes.

A failed or timed-out stage makes the attempt `FAIL`; later stages are `NOT_RUN`. There is no path from
a candidate-declared result, a recorded transcript, or compilation alone to `PASS`.

The `lume-economic-benchmark/0.1` report records, for every actual attempt:

- candidate SHA-256, attempt number, outcome, failed stage, and observed defects;
- per-stage and per-attempt wall time in milliseconds, measured with a monotonic clock;
- exit codes, timeout flags, proof-status counts, artifact/stdout hashes, and raw log paths;
- attempts to first pass, defects, verification time, suite/repository identities, and environment.

Generation latency, provider identity, zero-shot success, tokens, and all monetary costs are
`UNKNOWN` in this repository-fixture path. Verification wall time excludes generation and human repair
time. In particular, `safe_deposit_repair` measures two real verification attempts and one observed
proof defect; it does **not** claim a total maintenance cost.

## Reproduce the harness self-test

Requirements are a native Linux x86-64 host, the checked-out Lume repository, POSIX shell, and Python
3.8 or newer. Python uses only its standard library. No third-party package, network service, AI API,
credential, C compiler, external assembler, or external linker is used by the benchmark path.

```sh
make -C bootstrap/lume0
sh scripts/check-economic-benchmark.sh
```

The check first prepares the self-hosted tools through the existing `./lume` wrapper. This one-time
setup has its own timeout and is reported separately, so cold toolchain construction is neither
repeated nor counted as candidate compilation time. It then runs six attempts and verifies the report
independently. The expected fixture outcomes are:

- `clamp_nonnegative`: first-attempt `PASS`;
- `safe_deposit_repair`: proof failure followed by a second-attempt `PASS`;
- `negative_compile_failure`: compilation `FAIL`;
- `negative_proof_failure`: compilation succeeds, missing proof obligations cause `FAIL`;
- `negative_execution_failure`: compilation and proof succeed, native division by zero causes `FAIL`.

To retain a report and its raw artifacts, run:

```sh
sh scripts/run-economic-benchmark.sh \
  --output build/economic-benchmark/results.json \
  --artifacts build/economic-benchmark/artifacts \
  --self-test
```

The default suite is `tests/economic_benchmark/suite.json`. `--setup-timeout` bounds each one-time
toolchain preparation command; `--compile-timeout`, `--proof-timeout`, and `--execution-timeout`
bound each candidate process. Their values are recorded in the report. Generated
reports, logs, and binaries are outputs, not versioned evidence. A public result should include the
JSON report and logs so another operator can inspect failures and rerun the same suite.

## Adding candidates

Each task declares an exact expected stdout and an ordered list of `.src` candidates relative to the
manifest. Candidate paths must remain inside the suite directory. The `self_test` expectation is used
only by `--self-test` to detect harness regressions; it never determines an attempt outcome.

For a future externally generated run, preserve the complete candidate for every attempt and generate
attempt one without diagnostics, compiler feedback, repairs, or task-specific exemplars. Only that
protocol can make the first attempt eligible for a zero-shot measurement. The current fixture manifest
does not satisfy that provenance requirement, so its aggregate zero-shot field remains `UNKNOWN`.
Token counts require provider usage receipts or a named, versioned tokenizer applicable to the exact
request and response. Monetary cost requires traceable pricing and metering; source-size counts must
not be relabelled as provider tokens or dollars.

## Claim boundaries and fair comparison requirements

Protocol 0.1 supports a Lume-only measurement path on native Linux x86-64. It does not establish that
Lume is faster, cheaper, more reliable, or more maintainable than another language, and it does not
rank models. Existing `tests/llm_gen_eval/` transcripts remain regression fixtures for #230 and are
not economic benchmark evidence.

A public cross-model or cross-language comparison still needs:

1. frozen natural-language intents and equivalent executable acceptance criteria for every language;
2. independent candidate provenance, model/provider/version, decoding settings, prompt/context, and
   attempt timestamps, with no feedback in the zero-shot arm;
3. actual input/output token receipts and reproducible monetary pricing, or explicit `UNKNOWN` fields;
4. the same task visibility, attempt budget, timeouts, hardware class, target, and acceptance gates for
   every arm, including any proof obligations rather than giving Lume a stricter hidden bar;
5. defects classified from observable compile/proof/test/runtime failures and negative results retained;
6. maintenance changes applied after the initial acceptance, measuring subsequent attempts, generation
   and verification time, human intervention, tokens, and cost separately;
7. enough independent runs to report variability, plus published raw reports and an independent rerun.

Until those conditions are met, use this harness to reproduce real Lume outcomes and defects only, not
to make an economic-inflection or superiority claim.
