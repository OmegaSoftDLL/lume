## What & why

<!-- One or two sentences: what this changes and why. Link the tracking issue. -->

## Checklist (see AGENTS.md / docs/PROJECT_STATE.md)

- [ ] One focused change; English only (code, docs, comments, filenames).
- [ ] `make -C bootstrap/lume0 test` passes (0 failed).
- [ ] `make -C bootstrap/lume0 asan` passes (0 failed).
- [ ] New behavior is covered by a conformance/regression test (golden rule: no bugs).
- [ ] No new bug in Lume's toolchain; backends agree where they overlap.
- [ ] The required `make bootstrap` (stage 2 == stage 3), `make zero-c`, and oracle byte-equivalence gates are green.
- [ ] The self-hosted compiler was edited in `compiler/stage1/lume/NN_*.src`, not the generated `lumec.src`.
- [ ] Does not use Rust; treats C as the frozen disposable seed (moves toward native, not toward C).
- [ ] If a borrowed idea: ours is genuinely better than the source, not a shallow clone.
- [ ] Diagnostics are AI-first (stable code + JSON + fix-it) where applicable.
- [ ] `docs/PROJECT_STATE.md` / `docs/10-stage1-status.md` and the relevant plan are updated if scope changed.
- [ ] No faked completion: shipped, model, and planned status matches the implementation and CI evidence.

## Notes for reviewers

<!-- Risks, follow-ups, validation limitations, anything to look at closely. -->
