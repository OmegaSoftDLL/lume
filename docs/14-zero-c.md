# 14 — Zero-C: when (and in what sense) Lume builds Lume without C or assembly

A recurring, fair question: *"From what point does Lume generate Lume without needing
C or an external assembler — and why do the tools still contain any C at all?"* This
note answers it precisely, and describes the **named CI gate** (`make zero-c`,
job *"Zero-C steady state (no cc/as/ld)"*) that proves the answer on every push.

## The short answer

- **The steady state is already C-free, and has been since the self-host loop closed.**
  Once the seed has minted the first Lume-built tools, the entire ongoing build —
  Lume compiling Lume, and the result assembled and linked into a native ELF
  executable — uses **no C compiler** and **no external assembler/linker** (`as`/`ld`).
  The assembler + ELF64 writer is `compiler/stage1/lasm.src`, **written in Lume**.
- **The only remaining C is the Stage-0 seed** (`bootstrap/lume0/`, a small C
  interpreter with its own ELF writer). It runs once, to mint the first `lumec`/`lasm`,
  and is then off the path. It is **kept on purpose**, not a leftover (see below).

So "Lume generates Lume with no C and no assembly" is true from **Stage 1 onward** — the
moment the seed-built tools rebuild the toolchain from Lume source. The three later
stages, the fixpoint, and the `lume` CLI's native builds never touch C or `as`/`ld`.

## The staged bootstrap (where C lives, and where it does not)

`scripts/bootstrap.sh` runs the GCC/Rust model:

| Stage | Who runs | Uses C? | Uses `as`/`ld`? |
|-------|----------|:------:|:---------------:|
| 0 — seed mint | `lume0` (C) mints `lumec0`, `lasm0` | **yes** (the seed, by design) | no — `lume0` writes ELF itself |
| 1 — Lume rebuilds Lume | `lumec0` + `lasm0` (Lume-built) | **no** | **no** — `lasm` writes ELF |
| 2 — rebuild again | `lumec1` + `lasm1` | **no** | **no** |
| 3 — once more | `lumec2` + `lasm2` | **no** | **no** |
| fixpoint | assert **stage 2 == stage 3**, bit for bit | — | — |

Comparing stage 2 to stage 3 (not stage 1) is deliberate and is what GCC does: stage 1
is produced along a seed-influenced path, so the fixpoint is the point from which the
Lume-built tools reproduce themselves exactly. `as`/`ld` are replaced throughout the
steady state by `lasm` (Lume); the `cc`/`gcc` compiler is replaced by `lumec` (Lume).

## Why the C seed stays (and is *not* replaced by a committed binary)

`bootstrap/lume0` is the **reproducibility root**, kept frozen exactly as GCC needs a
system C compiler and Rust needs a prior `rustc`. Keeping it as **auditable C source**
(rather than removing C by committing a Lume-built binary seed into the repository) is a
deliberate security choice:

- **No trusting-trust surface.** Every byte of the toolchain can be rebuilt from
  inspectable source — the C seed plus the Lume sources — with nothing to take on faith.
  Committing a prebuilt `lumec`/`lasm` binary as the seed would reintroduce exactly the
  Ken-Thompson *"Reflections on Trusting Trust"* hole the from-source root avoids.
- **Reproducible from a tiny, reviewable base.** `lume0` is small C; anyone can read it,
  build it, and verify the whole language rebuilds from it. See `docs/04-roadmap.md`
  (the reproducibility-root rationale).

"Zero-C" therefore means **zero C in the steady state**, with a small, frozen, auditable
C seed as the from-source root — not "no C anywhere in the repository." That is the
strongest honest claim, and it is the one the gate below enforces.

## The gate: `make zero-c` — proven adversarially

A claim that the steady state is C-free is only worth the proof behind it, so the gate
does not merely *avoid* calling C — it makes calling C **impossible** and shows the build
still succeeds. `scripts/check-zero-c.sh`:

1. Builds the seed and mints `lumec0`/`lasm0` (the one place C is allowed).
2. **Shadows** `cc`, `gcc`, `clang`, `c++`, `g++`, `as`, and `ld` on `PATH` with stubs
   that fail loudly (exit 97) if invoked, and verifies the shims are actually first.
3. Runs stages 1-3 under that shadowed toolchain and asserts **stage 2 == stage 3**
   (both the `.s` and the native binaries), then compiles and runs a sample program
   whose output must be `55`.

If any C compiler or external `as`/`ld` had crept into the steady-state path, a stub
would fire and the build would fail. Green means: stages 1-3 built Lume-with-Lume and
assembled native ELF with `lasm`, and `cc`/`gcc`/`clang`/`as`/`ld` were never invoked.

Run it locally with `make zero-c`; in CI it is the job *"Zero-C steady state
(no cc/as/ld)"*, alongside the self-host fixpoint gate (`make bootstrap`).

## Frozen C seed policy & `lasm.src` self-validation (`#205`)

Now that the self-host loop is closed, **`bootstrap/lume0/*.c` and `bootstrap/lume0/*.h` are strictly frozen**:

1. **Cryptographic seed freeze (`bootstrap/lume0/SEED_FREEZE.sha256`):**
   `scripts/check-zero-c.sh` verifies the exact SHA-256 of every `.c` and `.h` file in
   `bootstrap/lume0/` before minting stage 0. New language features, directives (such as
   `.byte` for P33 `.rodata` protection), and target formats **must** be implemented in
   Lume (`compiler/stage1/lasm.src`, `compiler/stage1/lume/*.src`), never by growing the C
   seed. The only permitted edits to `bootstrap/lume0/` are critical security/UB fixes that
   update `SEED_FREEZE.sha256` in an explicitly reviewed seed-maintenance commit.
2. **Retiring `lume0 asm` as an assembler oracle:**
   In `bootstrap/lume0/tests/run.sh`, `lasm.src` is initially cross-checked against `lume0 asm`
   on the minimal bootstrap instruction subset (`cmp -s lasm.elf c.elf`). For all post-Stage-1
   features (`.byte`, P33 protection, new target triples in `docs/26-targets.md`), `lasm.src`
   is **self-validating**: its output is verified by executing the resulting binary, checking
   its ELF/section invariants and contracts, and proving `stage 2 == stage 3` bit-for-bit
   without consulting `lume0 asm`.

