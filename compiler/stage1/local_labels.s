# local_labels.s -- GAS numeric local labels REUSED: every `1:` is a distinct label and each
# `1f` / `1b` resolves to the NEAREST one (forward / backward). Hand-written test for lasm.
# A single-name-per-number assembler (the old lasm and the elf.c oracle) maps every `1` to one
# address (the last definition), so the first loop's `jne 1b` jumps past its own body and the
# `jmp 1f` lands on the wrong `1:`; the program then hangs or prints the wrong thing.
# Correct run: loop A adds 1 three times (rbx=3), `jmp 1f` skips the dead store to the next `1:`,
# loop B adds 10 four times (rbx=43), then `jne 2f` is not taken and "ok" is printed.
    .globl _start
    .section .text
_start:
    xorq %rbx, %rbx
    movq $3, %rcx
1:
    addq $1, %rbx
    subq $1, %rcx
    jne 1b
    jmp 1f
    movq $99, %rbx
1:
    movq $4, %rcx
1:
    addq $10, %rbx
    subq $1, %rcx
    jne 1b
    cmpq $43, %rbx
    jne 2f
    leaq okm(%rip), %rsi
    movq $3, %rdx
    jmp 1f
2:
    leaq nom(%rip), %rsi
    movq $3, %rdx
1:
    movq $1, %rax
    movq $1, %rdi
    syscall
    movq $60, %rax
    xorq %rdi, %rdi
    syscall
    .section .rodata
okm:
    .asciz "ok\n"
nom:
    .asciz "no\n"
