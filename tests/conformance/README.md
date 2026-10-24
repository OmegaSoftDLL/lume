# Conformance tests

Normative tests for the Lume specification, tagged by stage (`docs/04-roadmap.md`).

- `stage0/` — programs in the core subset with their expected output (`*.lume` + `*.expected`).
  Populated as soon as `lume0 run` exists; the Stage-0 examples in `examples/` are the first
  entries.

An implementation conforms to spec v0.1 at stage *N* if it passes every test tagged ≤ *N*.
