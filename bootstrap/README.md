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
./lume0 check  ../../examples/03_records_enums.lume --json
```

Status: **lexer complete** (spec §1: layout tokens, all literal forms, diagnostics `L0001–L0060`).
Next: `parser.c` (spec §2–§9), then `check.c`, then `interp.c`.

Size budget: ≤ 8,000 lines of C in total.
