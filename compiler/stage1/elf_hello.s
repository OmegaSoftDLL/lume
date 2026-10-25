## elf_hello.s — assembled to a runnable ELF by `lume0 asm` with no as/ld/cc.
    .globl _start
    .section .text
_start:
    movq $1, %rax
    movq $1, %rdi
    leaq msg(%rip), %rsi
    movq $16, %rdx
    syscall
    movq $60, %rax
    xorq %rdi, %rdi
    syscall
    .section .rodata
msg:
    .ascii "lume native elf\n"
