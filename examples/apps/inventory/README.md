# Stage 2 Capstone Application — Inventory, Invoicing & Reports (`examples/apps/inventory/`)

This directory contains the official **Stage 2 Exit Criterion Application** (`docs/04-roadmap.md`
line 106): a complete, self-contained business application (< 500 lines of Lume-Core across 4
modules) built, verified, and tested exclusively with `./lume`.

## Architecture & Stage 2 Tracks Exercised

| File | Role | Stage 2 Pillars Exercised |
|------|------|---------------------------|
| `lume.toml` | Package manifest (`name = "inventory"`, `edition = "2026"`, `allow = ["io"]`) | **Track T (`T3`)** manifest validation & P26 capability audit |
| `lume.lock` | Deterministic SHA-256 content-addressed lockfile | **Track T (`T3`)** `lume pkg lock` / `lume pkg verify` (P32) |
| `catalog.src` | Product catalog CSV schema, `StockStatus` ADT, comptime discount schedule, hygienic restock macro, and stock queries | **Track C (`C1`–`C4b`)**, **Track M (`M1`/`M2`)**, **Track D (`D1`–`D3`)**, **Track F (`F1`–`F4`)** |
| `invoicing.src` | Customer master & invoice lines CSV ledgers, penny-exact `decimal(10, 2)` tax/discount settlement, and equijoin + grouped queries | **Track D (`D1`–`D3`)**, **Track C (`C1`/`C4a`)**, **Track M (`M2`)**, **Track T (`T3` diamond `use`)** |
| `reports.src` | Deterministic 5-section executive report (valuation, category stock, low-stock replenishment, customer settlements, regional revenue) | **Track D**, **Track F**, **Track T** |
| `main.src` | Application entrypoint (`lume run` / `lume build`) and 10 domain unit tests (`fn test_inventory_*()` for `lume test`) | **Track T (`T1`/`T3`)** |

## Commands

```sh
# 1. Validate lume.toml and verify all SHA-256 module digests against lume.lock
./lume pkg verify examples/apps/inventory

# 2. Compile and run the full inventory, invoicing & executive report natively (no C)
./lume run examples/apps/inventory/main.src

# 3. Discover and execute all unit tests (15 std.decimal + 10 inventory domain tests = 25 passed)
./lume test examples/apps/inventory/main.src

# 4. Build a standalone native ELF executable
./lume build examples/apps/inventory/main.src -o build/inventory
```
