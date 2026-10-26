# Data Tracks D1–D3 Handoff

This note captures the completed Phase 2 data work for future sessions. D1, D2, and D3 are
merged. Work after D3 has not been defined; follow Ricardo's direction on what comes next.

## D1 — exact decimals

- PR #120, merged to main.
- Source: compiler/stage1/decimal.src; Lume compiler integration is in the self-hosted
  compiler modules under compiler/stage1/lume/ (never edit generated lumec.src).
- Decimal values use a signed integer coefficient and scale. Parsing, formatting, comparisons,
  arithmetic, casts, exact division, and contextual coercion preserve exact values.
- Precision/scale are bounded at 18 digits. Overflow, division by zero, repeating division,
  and inexact rescaling fail rather than silently rounding.
- Regression tests compare the ML-core interpreter and native output; Stage 0 C is also used as
  a correctness oracle in CI.

## D2 — typed tables and CSV

- PR #127, merged to main.
- Source: compiler/stage1/table.src; nominal table[T] type hooks are in
  compiler/stage1/lume/06_typecheck.src.
- read_csv(text, schema) requires an explicit schema and exact header match. It handles quoted
  delimiters, doubled quotes, CRLF, and multiline quoted cells; validates row widths and parses
  str, int, and decimal cells without guessing types.
- Runtime tables expose typed cell tags and checked accessors: table_row_count,
  table_column_count, and table_cell.
- lumec accepts table[T], substitutes T through generic return types, and rejects known row
  type mismatches. Automatic reflection from a nominal record into a runtime schema is not
  implemented.

## D3 — typed relational queries

- PR #131, merge commit bd2a4a7 on 2026-10-26. Implementation commits: 6b4e2b6 and the
  CI correction f44b12b.
- Runtime operations live in compiler/stage1/query.src:
  - query_where(table, column, operator, tagged_value) filters rows.
  - query_order(table, column, "asc" | "desc") uses stable ordering.
  - query_join(left, right, left_prefix, right_prefix, left_key, right_key) performs an
    equijoin; output column names are <prefix>_<field>. Prefixes must be non-empty and distinct.
  - query_group(table, key, sum) emits rows with the key, count, and sum; sums support
    integer and decimal cells.
- Source-level calls add compile-time row declarations, for example:
  query_where(Customer, customers, "balance", ">", 100);
  query_join(Order, Customer, Joined, orders, customers, "orders", "customers", "customer_id", "id");
  query_group(Customer, CityTotals, customers, "city", "balance").
- Compile-time query row arguments are erased by compiler/stage1/lume/03_comptime.src. The
  query checker in compiler/stage1/lume/06_typecheck.src validates nominal row declarations,
  field names, comparison types, join key compatibility, join output fields/types, and group
  output fields/types. Query result expressions are known as table[Row] or table[Out] by the
  gradual checker.
- Tests: compiler/stage1/query_test.src, tests/conformance/stage1/query_native.expected,
  and added coverage in bootstrap/lume0/tests/run.sh. Coverage runs interpreter and native
  assembly, validates query results, accepts valid typed query calls, and rejects unknown fields,
  incompatible comparison types, and incompatible join-key types.
- The first PR run failed only the new runtime regression: query.src depended on app, which was
  not part of the concatenated D1/D2 runtime modules, and the expected output omitted the
  grouped count. f44b12b added a query-local append helper and corrected the expected output.
  The following run passed all four required checks: Documentation & examples, Self-host bootstrap
  (stage2 == stage3), Stage 0 Ubuntu, and Stage 0 macOS. Do not confuse this passing bootstrap
  gate with the project milestone “triple self-build”; the latter is still not complete.

## Continuation rules

- D1–D3 are complete. Future data-track scope is open and must follow Ricardo's direction.
- Keep compiler/stage1/lumec.src generated. Make compiler changes only in the relevant
  compiler/stage1/lume/NN_*.src module.
- Keep future work on separate track files/modules where possible to avoid collisions.
- Space pushes if multiple pushes become necessary, to avoid GitHub throttling.
- Continue the project PR discipline: one branch, tests/regression, all four required checks
  green, then merge. The Windows environment used for D3 had no WSL distro/toolchain, so full
  compiler/native validation was performed by GitHub CI.
