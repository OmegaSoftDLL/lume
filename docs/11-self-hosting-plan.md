# 11 — Self-hosting plan: reaching 100% of Phase 1 (and deleting the C)

Phase 1 exits when the Lume-written compiler compiles the *entire* language it is written in,
then does a **triple self-build** (seed builds compiler, compiler builds compiler, compiler
builds compiler again — the last two bit-identical), after which the C seed `lume0` and the
external `as`/`ld` are deleted. This document is the concrete route from where we actually are
to there. It is written after reading the code, not the roadmap; where the two disagree, the
code wins.

## 0. What the code actually says (premise check)

The briefing framed the blocker as "the native backend compiles Layer B (core), but `lumec`
and `data_*` are written in Layer A." Reading the sources confirms the blocker **and sharpens
it in two ways that change the plan**:

1. **Every tool in `compiler/stage1/` is written in surface Layer A — including the native
   backend itself.** `data_asmgen.lume` (the x86-64 code generator, 343 lines), `data_parser.lume`
   (202), `layout.lume` (115) and `lexer.lume` all use `var`, `while`, `for … in`, `mut`
   parameters, `.push()`/`.len()`/`.chars()`/`.map()`/`.join()`, string interpolation `"{c}"`,
   maps (`map[str,int]`), `??`, and `char` literals. None of that is in the functional ML-core
   the native backend accepts (`DInt DStr DVar DCons DCall DList DIndex DRec DField DBin DIf
   DLetIn DMatch` + builtins `count/at/slice/toint/str`). So the native backend cannot compile
   its own source, nor the parser that feeds it.

2. **`lumec` (the surface→C driver: `lumec.lume` + `token/lexer/ast/parser/check/codegen`) is a
   dead end for self-hosting.** It emits **C**, not native code, and it only accepts a tiny
   expression language (`let`/`fn`/`if`-`then`-`else`/calls/binops — not even `match` or records),
   i.e. it compiles **far less** than the Layer A it is written in. It cannot be the fixpoint
   vehicle. The real self-hosting vehicle is the **ML-core → native** path
   (`data_parser` + `data_asmgen` + `lume0 asm`), which already produces a native binary with
   **no C and no as/ld** — proven for `calc.src`, a complete little calculator *compiler* written
   in the ML-core (`bootstrap/lume0/tests/run.sh` lines 120-137).

So the honest target is: **make the ML-core → native compiler compile its own source.** The
self-hosted `lumec` is the ML-core compiler re-expressed in the ML-core — not the surface→C
`lumec.lume`, which we retire from the critical path.

## 1. The pivot decision

Two ways to close the gap between "the language the compiler is written in" and "the language
the compiler accepts":

### (a) Grow the native backend up to full Layer A
Teach `data_asmgen` + the runtime to compile `var` + reassignment, `while`/`for`/`break`,
`mut` reference parameters, growable lists with `push`/`pop`, maps/hash tables, **closures**
(captured environments → heap closures + indirect calls), string interpolation lowering,
method dispatch, optionals/`??`, ranges and the `for` iterator protocol, and a `char` type —
then compile the existing Layer A tools unchanged.

- **Pro:** the tools stay as written; no rewrite of ~900 lines.
- **Con:** this is building a complete imperative-plus-closures runtime, with a real heap
  (closures and growable maps effectively force GC or arena discipline) — easily 5–10× the
  work of (b), and every line must be bug-free (the golden rule). Worse, it is **circular**:
  `data_asmgen` is itself Layer A, so until it is rewritten in whatever subset it compiles, it
  can only ever run under the C interpreter. (a) does not remove C by itself; it just enlarges
  the mountain to climb before you can.

### (b) Rewrite the compiler in the functional subset the backend already compiles  ✅ recommended
Freeze an implementation language — call it **Lume-Core**: the functional ML-core that
`data_asmgen` compiles **today** (ADTs + `match`, records, lists, strings, ints, `let … in`,
`if`/`then`/`else`, calls, recursion, builtins), plus a *tiny* set of additions listed in §2.
Rewrite the lexer, parser and code generator in Lume-Core (recursion and immutable
record-threading instead of `var`/`while`/`mut`/`push`/maps/closures), exactly the way
`calc.src` already is. Then the native backend compiles the whole toolchain, including itself.

- **Pro:** the backend needs **almost no new features** — `calc.src` proves a scanner + Pratt
  parser + codegen already run natively in this subset. The work is a disciplined, testable
  rewrite, each piece diffable against the Layer A original. It removes C as a *consequence*.
- **Con:** ~900 lines of imperative code must be re-expressed functionally; and functional
  style at real scale exposes two runtime limits (stack depth without TCO, bump-heap without
  GC — see §5) that the tiny demos never hit.

**Recommendation: (b).** The functional core is already a proven, running compiler substrate;
(a) is a second language runtime we do not need and that does not, on its own, delete any C.
The one-line alternative worth naming: a **hybrid** that adds only `var` + `while` (no closures,
no maps, no methods) to the backend and keeps the tools imperative. It loses to (b) because the
functional subset works *today* with zero backend changes — recursion replaces loops cleanly —
so paying for mutable-locals-and-loops codegen buys nothing on the critical path.

## 2. Exact feature gap for path (b), in dependency order

Reading `data_asmgen.lume`, `data_parser.lume`, `data_eval.lume`, `layout.lume` and the `lumec`
modules, the gap splits into **genuine backend/runtime additions** (small) and a **functional
rewrite** of the tools (the bulk, no backend change). Everything the tools need that the backend
*already* has: `fn`, `let … in`, `if/then/else`, `match` on `PInt/PWild/PBind/PCons`, records
(`DRec`/`DField`), list literals + `DIndex`, string `+`/compare, arithmetic, `and`/`or`,
`count/at/slice/toint/str`, and recursion (incl. mutual — labels resolve across the whole text
section). Those require **zero** new work; the tools just have to be written using only them.

Genuine additions, smallest-first:

- **G1 — source input.** The native runtime only *writes* stdout (`printval`). A self-compiling
  binary must *read* its own source. Add a runtime builtin `read()` (whole stdin → boxed string
  via the `read` syscall into a heap buffer) — and, optionally, argv. Driver shape:
  `print agen(parse(read()))`, run as `./lumec < in.src > out.s`. **Blocks everything else.**
- **G2 — string-escape lexing.** `layout.lume`'s scanner stops at the first `"` (`while … src[j]
  != '"'`) and `data_parser`'s `DStr` only strips the quotes — neither handles `\"`, `\n`, `\\`,
  `\t`. The rewritten code generator is a giant asm-text emitter full of these; the Lume-Core
  lexer must scan past `\"` and the string *value* must carry real newline/quote/backslash bytes
  (null-terminated strings already hold newlines — asm text has no embedded NUL, so no runtime
  change is needed here, only correct lex + unescape). **Blocks the codegen rewrite (G6 below).**
- **G3 — top-level `let`.** `agen()` ignores top-level `DLet` (`let _ = 0`); only `fn` defs and
  `print` are emitted. Constrain Lume-Core programs to **fn-definitions + a single final `print`**
  (the `calc.src` shape), or add top-level-`let` lowering. Cheapest: adopt the constraint and
  add a test that pins it. Also pin two existing limits: **≤ 6 function parameters** (`areg`
  covers `rdi…r9` only) and string-dispatch via `if` (no string-literal patterns, no record
  patterns, no guards in `match`).

The rewrite itself (no backend change, pure source work) in dependency order:

- **R4 — layout lexer** → Lume-Core: compute leading-space counts and thread an indent *stack
  as an immutable list* through recursion (replacing `var stack`/`push`/`pop`); emit the token
  list as a `Cons`/`Nil` ADT. Depends on G2.
- **R5 — parser** → Lume-Core: the `data_parser` Pratt logic re-expressed by threading a
  `{toks, pos}` record through returns (the `{node, rest}` pattern already in `calc.src`),
  replacing every `var`/`mut p`/`.push`. Depends on R4.
- **R6 — code generator** → Lume-Core: `data_asmgen` re-expressed functionally — label counters
  and the `AC` record threaded as return values instead of `mut AC`; all `var out = "" … out = out
  + …` loops become string-building recursion; `.chars()` becomes index recursion with `at`/`count`;
  the `locals` lookup (`idx_of`) becomes recursion over a `Cons` list. Depends on G2, R5. Largest
  single item — split into (6a) int/`let`/`if`/call/recursion core, (6b) ADT/record/list/`match`,
  (6c) the string-runtime glue — each diffed byte-for-byte against the current Layer A output.

Deliberately **out of scope for the fixpoint**: the surface→C `lumec` modules
(`token/lexer/ast/parser/check/codegen.lume`), maps, closures, `for`, `mut`. They are not needed
to compile Lume-Core and are not ported.

## 3. PR sequence (each small, green, with a regression test)

Numbered from this doc. Keep the interpreter/native parity check alive at every step, and name
native `.expected` files with a distinct suffix so the generic `.lume` test loop ignores them
(PROJECT_STATE §8).

1. **This document.** No code.
2. **G1 `read()` builtin** in the `data_asmgen` runtime tail + a native test: a `.src` that reads
   stdin and prints a transform of it, through `data_asm_cc` → `lume0 asm`, no C.
3. **G2 escape handling**: teach the ML-core string path to lex past `\"` and unescape
   `\n \t \\ \"`; regression `.src` whose output contains quotes and newlines, native == C oracle.
4. **G3 program-shape pins**: test fixing fn-only + single-`print`, ≤6 params, `if`-dispatch; no
   backend change, just guard rails so the rewrite cannot drift into unsupported territory.
5. **R4 lexer in Lume-Core** (`lclex.src`): dump its token stream for `calc.src`; assert it equals
   the Layer A `layout.lume` token dump.
6. **R5 parser in Lume-Core** (`lcparse.src`): dump the AST (`show`-style) for a sample `.src`;
   assert it matches the `data_parser` AST.
7. **R6a codegen core** (`lcgen.src`, integer/`let`/`if`/call/recursion): emit asm for `nfib.src`;
   assert **byte-identical** to current `data_asmgen` output.
8. **R6b codegen ADT/record/list/match**: extend `lcgen.src`; byte-identical asm for
   `ev.src`/`reclist.src`/`list.src`.
9. **R6c codegen string runtime + full driver** (`lumec.src` = `read()` → lex → parse → gen →
   `print`): byte-identical asm for `calc.src` vs the Layer A pipeline.
10. **Bootstrap the native compiler**: `lume0 run data_asm_cc.lume lumec.src lumec.s` →
    `lume0 asm lumec.s lumec`. Test: `./lumec < calc.src` assembled by `lume0 asm` runs and
    matches `calc_asm.expected`.
11. **FIXPOINT A (compiler self-hosts, C assembler still allowed)**: `./lumec < lumec.src > s2.s`;
    assemble; `./lumec2 < lumec.src > s3.s`; assert `s2.s == s3.s`. This is the triple build at
    the **asm** boundary — the compiler now compiles its own source.
12. **Scale hardening** (only if 11 overflows): add self/mutual **tail-call optimization** to the
    backend, and/or enlarge the process stack and the bump-heap region; stress test on a large
    synthetic program. (See §5 — this may need to move earlier if R6 already overflows.)
13. **G4 byte buffers + sized `write(fd,buf,len)`** in the runtime (ELF contains NUL bytes, so a
    C-string cannot represent it); test emitting raw bytes including NUL.
14. **Assembler in Lume-Core** (`lcasm.src`, port of `elf.c`): instruction encoder + ELF64 layout;
    assemble `elf_hello.s` and `lumec.s` to **byte-identical** ELF vs `lume0 asm`.
15. **FIXPOINT B (no C anywhere)**: pipeline = native `lumec` + native `lcasm`, triple build,
    **bit-identical binary**. Then port `resolve.c` → `lccheck.src` (16), and finally **delete
    `lume0`** (C seed, `elf.c`, `asmgen.c`, `interp.c`, `resolve.c`), rewire Makefile/CI to build
    the toolchain from a committed frozen seed binary, and update PROJECT_STATE + `docs/10`. **= Phase 1 exit.**

## 4. The C→Lume rewrites (`elf.c`, `resolve.c`) — what and when

- **`elf.c` (390 lines: assembler + ELF64 writer).** On the critical path: it is the last C
  between asm text and a runnable binary, and "no C" cannot be claimed while it stands. It is the
  **hardest** port, because ELF output is binary with embedded NUL bytes — impossible with the
  current null-terminated string value. So it is gated on **G4 (byte buffers + sized write)** and
  comes **after** the compiler already self-hosts through `lume0 asm` (PR 13–14). Do not attempt it
  before the compiler fixpoint: debugging a new binary writer and a new self-hosted compiler at the
  same time violates "one change, one green PR."
- **`resolve.c` (373 lines: the semantic `check`).** **Not** required for the triple self-build —
  a compiler self-hosts without a checker. But `check` is the AI-first selling point (R0200/T0001/
  D0001/D0002/A000x/…) and must not be lost when C is deleted. Port it **just before** the delete
  (PR 16), after compiler and assembler self-host, keeping all 9 `examples/0*.lume` green through
  the port. If schedule pressure forces a choice, the delete can ship with `check` temporarily
  absent and restored immediately after — but prefer porting first.

## 5. Honest estimate and the biggest risks

**Historical estimate (stale — Phase 1 is complete; the triple self-build and the Zero-C staged bootstrap are reached): "remaining work: roughly half of Phase 1 (~45–55%)."** What existed is real and load-bearing: a
native backend for the functional core, our own ELF writer, and a *proof* (`calc.src`) that a
compiler compiles natively with no C/as/ld. What remains is the harder half: the full functional
rewrite surviving real-program scale, a **binary** ELF writer in Lume (the single least-proven
piece), and a genuinely bit-identical triple build. Rule of thumb: the last ~20% (byte-buffer ELF
in Lume + bit-identical fixpoint) carries most of the risk, not most of the lines.

Biggest risks, roughly in order of how likely they are to bite:

1. **Stack depth without TCO.** The functional style builds the token list and AST with non-tail
   recursion; depth ≈ token count (thousands) for a ~1000-line compile. The runtime uses the raw
   process stack with no tail-call optimization. This is the most probable wall. *Mitigation:* PR 12
   (self/mutual TCO, or a larger stack); may have to move ahead of PR 9 if R6 overflows first.
2. **Bump-heap exhaustion / no GC.** `alloc` is a 64 MB bump allocator with no reclaim; every int,
   string and cons cell is boxed. A self-compile allocates heavily and never frees. *Mitigation:*
   enlarge the region, or add a simple mark/sweep or arena-reset between phases.
3. **Byte-accurate ELF in Lume (`elf.c` port).** Binary layout, section offsets and relocations
   must match `lume0 asm` to the byte for a bit-identical fixpoint. *Mitigation:* G4 first, then
   diff ELF bytes against the C writer on a growing corpus before switching the pipeline.
4. **Escape/encoding fidelity (G2).** The codegen emits asm text dense with `\n`/`\"`/`\\`; one
   wrong unescape and the self-compile diverges silently. *Mitigation:* byte-identical asm diffs at
   PRs 7–9, before any fixpoint.
5. **Silent divergence at the fixpoint.** The golden rule means a 1-byte difference between build 2
   and build 3 is a bug, not a rounding error. *Mitigation:* the diffs are the tests at every PR;
   never advance a stage whose output is not identical to its oracle.

### What would change this recommendation

- If a **GC or growable-map/closure runtime already had to be built** for other Phase-2 reasons,
  path (a)'s marginal cost would drop and the calculus could flip toward growing the backend.
- If the **functional rewrite overflows the stack even with TCO and a large stack** (risk 1
  proving fatal), the backend must gain real iteration (`var`/`while`) anyway — collapsing toward
  the hybrid — and the plan would add that as an early, mandatory brick instead of an if-needed one.
- If a **fixpoint cannot be made bit-identical** because the toolchain is nondeterministic
  (e.g. map iteration order leaking into output), that source of nondeterminism must be removed
  before any self-build is attempted; the functional, map-free Lume-Core is chosen partly to avoid
  exactly this.
