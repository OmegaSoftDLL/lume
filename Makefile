# Lume top-level convenience targets. The real build lives in the staged
# bootstrap (scripts/bootstrap.src) and the seed Makefile (bootstrap/lume0).
.PHONY: amalgamate bootstrap seed zero-c test asan clean

## Generate compiler/stage1/lumec.src by concatenating compiler/stage1/lume/*.src
## (SQLite-style amalgamation). lumec.src is a build artifact, not a source file;
## edit the modules in compiler/stage1/lume/, never the generated lumec.src.
amalgamate: seed
	mkdir -p build
	printf '@root %s\n' "$(CURDIR)" > build/amalgamate.in
	if [ "$$(uname -s)" = "Linux" ] && [ "$$(uname -m)" = "x86_64" ]; then \
		./bootstrap/lume0/lume0 run compiler/stage1/data_asm_cc.lume scripts/amalgamate.src build/amalgamate.s; \
		./bootstrap/lume0/lume0 asm build/amalgamate.s build/amalgamate; \
		chmod +x build/amalgamate; \
		build/amalgamate < build/amalgamate.in >/dev/null; \
	else \
		./bootstrap/lume0/lume0 run compiler/stage1/compiler_in_core.lume scripts/amalgamate.src build/amalgamate.in >/dev/null; \
	fi

## Staged, self-verifying self-host: C seed -> lumec+lasm -> rebuild in Lume,
## asserting stage 2 == stage 3 bit-for-bit. The only step that uses C is the seed.
bootstrap: seed amalgamate
	ulimit -s unlimited 2>/dev/null || ulimit -s 524288 2>/dev/null || ulimit -s 65520 2>/dev/null || ulimit -s 32768 2>/dev/null || true; \
	./lume build scripts/bootstrap.src -o build/bootstrap_runner && \
	printf '@root %s\n' "$(CURDIR)" | build/bootstrap_runner

## Build just the frozen C seed (lume0 + its own assembler/ELF writer).
seed:
	$(MAKE) -C bootstrap/lume0

## Prove the zero-C steady state: after the seed mints the first Lume tools,
## shadow cc/gcc/clang/as/ld with failing stubs and rebuild stages 1-3 (Lume
## building Lume, assembled by lasm), asserting the stage 2 == stage 3 fixpoint.
zero-c: amalgamate
	./scripts/check-zero-c.sh

## Conformance suite (includes the self-host and zero-C-loop tests).
test: amalgamate
	$(MAKE) -C bootstrap/lume0 test

## Same suite under AddressSanitizer/UBSan.
asan: amalgamate
	$(MAKE) -C bootstrap/lume0 asan

clean:
	$(MAKE) -C bootstrap/lume0 clean
	rm -rf build
