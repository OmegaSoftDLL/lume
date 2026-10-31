.section .text
.globl _start
_start:
    call A
    call Ab
    movq $60, %rax
    movq $0, %rdi
    syscall
A:
    movq $120, %rsi
    ret
Ab:
    movq $240, %rsi
    ret
