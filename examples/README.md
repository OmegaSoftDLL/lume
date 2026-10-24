# Examples

| File | Stage | Shows |
|---|---|---|
| `01_hello.lume` | 0 | script mode, interpolation |
| `02_basics.lume` | 0 | bindings, loops, `if` expressions |
| `03_records_enums.lume` | 0 | records, enums with payloads, `match`, UFCS |
| `04_errors_optionals.lume` | 0 | error sets, `fail`, `?`, `catch`, `T?`, `??`, `?.` |
| `05_collections_closures.lume` | 0 | lists/maps/sets, comprehensions, closures, `|>` |
| `06_traits_generics.lume` | 0/1 | traits, defaults, generics with bounds, `dyn` |
| `07_contracts_defer.lume` | 0 | `requires/ensures/invariant`, range types, `decimal`, `defer`, `assert` |
| `08_shell_script.lume` | 1 | `std.sh` as a Bash replacement |
| `09_stdlib.lume` | 0 | standard library: strings, lists, maps, math, higher-order functions |
| `future/10_data_query.lume` | 2 | `table`, `index`, `query`, `persist` |
| `future/11_concurrency.lume` | 3 | `scope`, `spawn`, channels, `select`, `par for` |
| `future/12_model_training.lume` | 4 | `model`, `tensor` shapes, `grad` |
| `future/13_llm_agent.lume` | 4 | `extern llm fn`, typed tools, table memory, supervisor |
| `future/14_kernel_freestanding.lume` | 5 | `@mmio`, `@bits`, `interrupt`/`realtime`, capabilities |
| `future/15_multi_agent.lume` | 3/4 | many agents running at once: supervisor, workers, channels, typed `llm fn` |

Stage-0 examples are also conformance tests: `tests/conformance/stage0/` holds each with its
expected output.
