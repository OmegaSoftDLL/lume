# AGENTS.md — read this before working on Lume

Any agent (Claude, Auggie/agy, or other) working on this repo must read
**[`docs/PROJECT_STATE.md`](docs/PROJECT_STATE.md)** first. It holds the full state, the
architecture, the DO / DO-NOT rules, the PR workflow, and the hard-won pitfalls. This file
is the short version.

## Non-negotiable rules

- **Converse in Portuguese (pt-BR). Write all code, docs, comments, filenames, commits and
  PRs in English.**
- **Golden rule: zero bugs in Lume's own toolchain.** Every fix ships with a regression test.
- **Do not use Rust** to implement Lume.
- **Edit the self-hosted compiler in `compiler/stage1/lume/NN_*.src`, never the generated
  `compiler/stage1/lumec.src`.** `lumec.src` is a build artifact (git-ignored), amalgamated
  from the modules in numeric order by `scripts/amalgamate.sh` (`make amalgamate`); the
  concatenation is byte-identical to the historical monolith. See `docs/13-module-split-plan.md`.
- **Do not fake completion.** The triple self-build (Phase 1 exit) **is done** — `stage 2 == stage 3` holds in CI (the `bootstrap` gate) — so never regress it, and never claim a milestone that is not real. Phases 1–5 and the compiler-self-hosting inflection are complete; what remains is **verification/trust-layer hardening** (the proof-certificate kernel must verify independently, WCET/WASM models must be labelled or enforced, autodiff must diagnose outside its fragment, P33 protection is unbuilt) — tracked in the open issues. State features at their real level: shipped, a model/stub, or planned.
- **C is the disposable seed only.** The end state is no C and no external `as`/`ld` (Lume
  emits native ELF itself — see `lume0 asm`). Move work toward native, not toward C.
- For each feature borrowed from another language, make ours **superior**, not a shallow clone.
- Keep all 9 `examples/0*.lume` passing `lume0 check` cleanly (the semantic checker must stay
  zero-false-positive).

## Workflow

- One change → one branch → one green PR → merge. Keep CI green (`gh`).
- Before committing: `make -C bootstrap/lume0 test` and `make -C bootstrap/lume0 asan`, both
  `0 failed`. The build is strict: `-Werror -Wconversion -Wpedantic -Wshadow`.
- Commit messages end with `Co-Authored-By: ...` (see the session's attribution note); PR
  bodies end with the Claude Code generation line. Use `git commit -F <file>` to avoid shell
  quoting issues.

## Build & run

```
make -C bootstrap/lume0
bootstrap/lume0/lume0 check  <file.lume>     # lex + parse + name/type/duplicate checks
bootstrap/lume0/lume0 run    <file.lume>     # interpret
bootstrap/lume0/lume0 emit-asm <f.lume> f.s  # x86-64 (integer subset)
bootstrap/lume0/lume0 asm    f.s  out        # assemble+link to native ELF, NO as/ld/cc
bootstrap/lume0/lume0 describe --json        # self-description for a model
```

See `docs/PROJECT_STATE.md` §9 for the pitfalls (reserved keywords, `{{`/`}}` escaping, flat
`if/elif` AST, conformance test naming, strict-flag gotchas) — do not rediscover them.
