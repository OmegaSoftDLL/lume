# Contributing to Lume

Thank you for helping build Lume. Contributions of code, tests, documentation,
bug reports, and design proposals are welcome.

## Before you start

- Follow the [Code of Conduct](CODE_OF_CONDUCT.md).
- Read [`AGENTS.md`](AGENTS.md) and [`docs/PROJECT_STATE.md`](docs/PROJECT_STATE.md)
  for the architecture, delivery rules, and known pitfalls.
- Check open issues and pull requests to avoid duplicating existing work.
- Report suspected vulnerabilities privately through the
  [Security Policy](SECURITY.md), never through a public issue.

## Ground rules

- **English only** in the repository: code, comments, filenames, folders, commit messages,
  PRs and docs. Project discussion with the maintainer may be in Portuguese.
- **Golden rule: no bugs in Lume's own toolchain.** Every fix ships with a regression test.
- **Do not use Rust** to implement Lume.
- **C is the frozen bootstrap seed only.** It is not on the steady-state path; implement new
  language capabilities in Lume and do not expand the seed.
- If you borrow an idea from another language, make Lume's version **genuinely superior**,
  not a shallow clone.
- The triple self-build (Phase 1 exit) is complete. Preserve the `stage 2 == stage 3`
  fixpoint and the Zero-C gate; do not claim milestones beyond what the tests demonstrate.
- Describe capabilities accurately as shipped, a model, or planned. A model, mock, or partial
  implementation must not be presented as a completed feature.

## Development workflow

1. Create a focused branch from an up-to-date `main`.
2. Make the smallest coherent change and update the relevant documentation in the same branch.
3. Add conformance or regression coverage for every behavioral fix:
   `tests/conformance/stage1/` and/or `bootstrap/lume0/tests/`.
4. Build and run the required local checks; both suites must report `0 failed`:

   ```sh
   make -C bootstrap/lume0 test
   make -C bootstrap/lume0 asan
   ```

   The seed build is strict: `-Werror -Wconversion -Wpedantic -Wshadow`.
   Changes affecting the self-hosted toolchain must also preserve `make bootstrap`
   (`stage 2 == stage 3`) and `make zero-c`.
5. Review the complete diff, remove generated artifacts, and verify that unrelated files are unchanged.
6. Open a pull request and complete the PR template. All required CI checks must pass before merge.

`main` is protected. Do not push directly to it or merge a branch with failing required checks.

## Where things live

- `bootstrap/lume0/` — the frozen C seed and its conformance suite.
- `compiler/stage1/lume/NN_*.src` — the self-hosted compiler modules; edit these, never the
  generated `compiler/stage1/lumec.src`.
- `compiler/stage1/` — the self-hosted CLI, standard library, target backends, and tooling.
- `examples/` — executable examples and application capstones.
- `tests/conformance/` — normative, stage-tagged conformance cases.
- `docs/`, `grammar/`, and `formal/` — documentation, grammar, and mechanised formalism.

See `docs/PROJECT_STATE.md` §9 for reserved keywords, interpolation escaping, AST shapes,
conformance-test naming, and strict-compiler pitfalls.

## Bug reports and proposals

Use the bug-report template for reproducible defects. Include the affected release or commit,
operating system and architecture, exact command, expected and actual behavior, and a minimal
`.lume` or `.src` reproduction.

Use the feature-proposal template for language or tooling ideas. Explain the problem, prior art,
how Lume's design improves on it, and how the proposal can be tested without weakening existing
safety or verification guarantees.

## License

Lume is available under either the MIT or Apache-2.0 license. Unless explicitly stated otherwise,
contributions are dual-licensed under the same terms, as described in [`COPYRIGHT`](COPYRIGHT).
