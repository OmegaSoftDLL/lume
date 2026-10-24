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

Status: **lexer + parser complete** (spec §1–§9). The lexer produces layout tokens and all literal
forms with diagnostics `L0001–L0060`; the recursive-descent + Pratt parser builds an AST for modules,
`use`, functions with contracts and effects, `record`/`enum`/`error` types, traits/impls, generics,
all statements and the full expression grammar, with diagnostics `P0001–P0999` and recovery. All 8
Stage-0/1 examples parse; the `future/` examples lex (their syntax lands in later stages).
Next: `check.c` (name resolution + type inference), then `interp.c` (tree-walking evaluator), then
`lume0 run`.

```
./lume0 parse ../../examples/03_records_enums.lume --json   # print the AST as JSON
```

Size budget: <= 8,000 lines of C in total (currently ~2,000).
