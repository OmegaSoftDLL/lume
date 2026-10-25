## What & why

<!-- One or two sentences: what this changes and why. -->

## Checklist (see AGENTS.md / docs/PROJECT_STATE.md)

- [ ] One focused change; English only (code, docs, comments, filenames).
- [ ] `make -C bootstrap/lume0 test` passes (0 failed).
- [ ] `make -C bootstrap/lume0 asan` passes (0 failed).
- [ ] New behavior is covered by a conformance/regression test (golden rule: no bugs).
- [ ] No new bug in Lume's toolchain; backends agree where they overlap.
- [ ] Does not use Rust; treats C as the disposable seed (moves toward native, not toward C).
- [ ] If a borrowed idea: ours is genuinely better than the source, not a shallow clone.
- [ ] `docs/PROJECT_STATE.md` / `docs/10-stage1-status.md` updated if scope changed.

## Notes for reviewers

<!-- Risks, follow-ups, anything to look at closely. -->
