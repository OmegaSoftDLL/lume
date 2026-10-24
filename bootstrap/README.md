# bootstrap/ — Stage 0

`lume0/` is the **disposable Stage 0 interpreter** for Lume, written in portable C11 with no
dependencies. It exists to run the Stage 1 compiler (written in Lume) once; after the triple
self-build succeeds it is frozen and kept only as the reproducibility root
(see `docs/04-roadmap.md`, Stage 0 and "trusting trust").

Lume is not implemented in C. Lume is implemented in Lume. This is the seed.

```
cd bootstrap/lume0
make            # builds ./lume0
make test       # golden token dumps, all examples lex, negative cases, JSON output
make asan       # same tests under AddressSanitizer + UBSan
./lume0 tokens ../../examples/03_records_enums.lume
./lume0 parse  ../../examples/03_records_enums.lume          # print the AST
./lume0 check  ../../examples/03_records_enums.lume --json
```

Status: **lexer + parser + interpreter working** (spec §1–§9). `lume0 run` executes the Stage-0
examples (01–07): bindings, loops, `if`/`match`, records, enums, error unions, optionals, closures,
comprehensions, UFCS method calls, traits with dynamic dispatch, generics, contracts
(`requires`/`ensures`/`invariant` with `old()`), `defer`, exact `decimal`, string interpolation
with format specs, and pipelines. 31 tests pass; ASan/UBSan clean.
Next: widen coverage (more of `std`, `std.sh` for example 08), then start Stage 1 (`lumec` in Lume).

```
./lume0 run    ../../examples/03_records_enums.lume   # run a program
./lume0 parse  ../../examples/03_records_enums.lume    # print the AST
```

Size budget: <= 8,000 lines of C in total (currently ~3,350).
