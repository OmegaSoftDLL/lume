# Contributing to Lume

Thank you for helping build Lume. Before anything, read **[`AGENTS.md`](AGENTS.md)** and
**[`docs/PROJECT_STATE.md`](docs/PROJECT_STATE.md)** — they hold the full rules, architecture
and hard-won pitfalls. This file is the summary.

## Ground rules

- **English only** in the repository: code, comments, filenames, folders, commit messages,
  PRs and docs. (Project discussion with the maintainer may be in Portuguese; the repo is not.)
- **Golden rule: no bugs in Lume's own toolchain.** Every fix ships with a regression test.
- **Do not use Rust** to implement Lume.
- **C is the frozen bootstrap seed only.** It is not on the steady-state path; implement new
  language capabilities in Lume and do not expand the seed.
- If you borrow an idea from another language, make Lume's version **genuinely superior**,
  not a shallow clone.
- The triple self-build (Phase 1 exit) is complete. Preserve the `stage 2 == stage 3`
  fixpoint and the Zero-C gate; do not claim milestones beyond what the tests demonstrate.

## Workflow

1. Branch from `main`. One focused change per branch.
2. Build and test locally — both must report `0 failed`:
   ```
   make -C bootstrap/lume0 test
   make -C bootstrap/lume0 asan
   ```
   The build is strict: `-Werror -Wconversion -Wpedantic -Wshadow`.
3. Add a conformance/regression test for any new behavior
   (`tests/conformance/stage1/` and/or `bootstrap/lume0/tests/`).
4. Open a pull request. CI must be green; `main` is protected and cannot be pushed to directly.
5. The maintainer reviews and merges.

## Where things live

- `bootstrap/lume0/` — the C seed (lexer, parser, interpreter, LBE, `asmgen`, `resolve`, `elf`).
- `compiler/stage1/` — Lume-written compilers: the ML-core (`data_*`) and the block language.
- `examples/`, `docs/`, `grammar/`, `tests/`.

See `docs/PROJECT_STATE.md` §9 for pitfalls (reserved keywords, `{{`/`}}` escaping, the flat
`if/elif` AST, conformance-test naming, strict-flag gotchas). Please don't rediscover them.
