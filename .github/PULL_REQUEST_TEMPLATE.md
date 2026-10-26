## What & why
<!-- One or two sentences: what this changes and why. -->

## Checklist (Lume engineering rules)
- [ ] One focused change, one green PR.
- [ ] A regression test ships with the change (GOLDEN RULE: zero bugs in the toolchain).
- [ ] The three gates are green: `make bootstrap` (stage 2 == stage 3), `make zero-c`, and the oracle byte-for-byte test.
- [ ] The self-hosted compiler was edited in `compiler/stage1/lume/NN_*.src`, **not** the generated `lumec.src`.
- [ ] No Rust was used to implement Lume; C stays only as the frozen seed.
- [ ] Diagnostics are AI-first (stable code + JSON + fix-it) where applicable.
- [ ] Docs updated (`docs/PROJECT_STATE.md` and the relevant plan) if behavior changed.
- [ ] No faked completion — nothing claimed done that is not proven in CI.

🤖 Generated with [Claude Code](https://claude.com/claude-code)
