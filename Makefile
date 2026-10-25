# Lume top-level convenience targets. The real build lives in the staged
# bootstrap (scripts/bootstrap.sh) and the seed Makefile (bootstrap/lume0).
.PHONY: bootstrap seed test asan clean

## Staged, self-verifying self-host: C seed -> lumec+lasm -> rebuild in Lume,
## asserting stage 2 == stage 3 bit-for-bit. The only step that uses C is the seed.
bootstrap:
	./scripts/bootstrap.sh

## Build just the frozen C seed (lume0 + its own assembler/ELF writer).
seed:
	$(MAKE) -C bootstrap/lume0

## Conformance suite (includes the self-host and zero-C-loop tests).
test:
	$(MAKE) -C bootstrap/lume0 test

## Same suite under AddressSanitizer/UBSan.
asan:
	$(MAKE) -C bootstrap/lume0 asan

clean:
	$(MAKE) -C bootstrap/lume0 clean
	rm -rf build
