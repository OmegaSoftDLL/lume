/* asmgen.c — a DIRECT native backend for the bootstrap: Lume AST -> x86-64 assembly (Linux),
 * no C in the pipeline. Proves the C codegen is a removable stepping stone: Lume lowers to the
 * metal. Scope v1: let/var bindings and integer arithmetic/comparison expressions; each top-level
 * statement prints its value as a decimal line; exit 0. Uses raw Linux syscalls, no libc.
 * Unsupported nodes (functions, if, strings) are a reported error for now. */
#include "ast.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct { char **names; int n, cap; FILE *out; int ok; } Asm;

static int var_slot(Asm *a, const char *name, int len) {
    for (int i = 0; i < a->n; i++) if ((int)strlen(a->names[i]) == len && memcmp(a->names[i], name, (size_t)len) == 0) return i;
    if (a->n == a->cap) { a->cap = a->cap ? a->cap * 2 : 8; a->names = realloc(a->names, (size_t)a->cap * sizeof(char *)); }
    char *s = malloc((size_t)len + 1); memcpy(s, name, (size_t)len); s[len] = 0;
    a->names[a->n] = s; return a->n++;
}

static long long parse_int_lit(const char *s, int len) {
    long long v = 0; int base = 10, i = 0;
    if (len > 2 && s[0] == '0' && (s[1] == 'x' || s[1] == 'b' || s[1] == 'o')) { base = s[1]=='x'?16:s[1]=='b'?2:8; i = 2; }
    for (; i < len; i++) { char c = s[i]; if (c == '_') continue; int d = (c>='0'&&c<='9')?c-'0':(c>='a'&&c<='f')?c-'a'+10:(c>='A'&&c<='F')?c-'A'+10:0; v = v*base + d; }
    return v;
}

static void gen_expr(Asm *a, const Node *n);

static void bin_op(Asm *a, const char *op, int oplen) {
    /* left in on stack first, right second: pop right->rcx, left->rax */
    fprintf(a->out, "    popq %%rcx\n    popq %%rax\n");
    if (oplen == 1) switch (op[0]) {
        case '+': fprintf(a->out, "    addq %%rcx, %%rax\n"); break;
        case '-': fprintf(a->out, "    subq %%rcx, %%rax\n"); break;
        case '*': fprintf(a->out, "    imulq %%rcx, %%rax\n"); break;
        case '/': fprintf(a->out, "    cqto\n    idivq %%rcx\n"); break;
        case '%': fprintf(a->out, "    cqto\n    idivq %%rcx\n    movq %%rdx, %%rax\n"); break;
        case '<': fprintf(a->out, "    cmpq %%rcx, %%rax\n    setl %%al\n    movzbq %%al, %%rax\n"); break;
        case '>': fprintf(a->out, "    cmpq %%rcx, %%rax\n    setg %%al\n    movzbq %%al, %%rax\n"); break;
        default: a->ok = 0;
    } else if (oplen == 2) {
        if (!memcmp(op, "==", 2)) fprintf(a->out, "    cmpq %%rcx, %%rax\n    sete %%al\n    movzbq %%al, %%rax\n");
        else if (!memcmp(op, "!=", 2)) fprintf(a->out, "    cmpq %%rcx, %%rax\n    setne %%al\n    movzbq %%al, %%rax\n");
        else if (!memcmp(op, "<=", 2)) fprintf(a->out, "    cmpq %%rcx, %%rax\n    setle %%al\n    movzbq %%al, %%rax\n");
        else if (!memcmp(op, ">=", 2)) fprintf(a->out, "    cmpq %%rcx, %%rax\n    setge %%al\n    movzbq %%al, %%rax\n");
        else if (!memcmp(op, "**", 2)) a->ok = 0;
        else a->ok = 0;
    } else a->ok = 0;
    fprintf(a->out, "    pushq %%rax\n");
}

static void gen_expr(Asm *a, const Node *n) {
    switch (n->kind) {
    case N_INT: fprintf(a->out, "    movq $%lld, %%rax\n    pushq %%rax\n", parse_int_lit(n->text, n->text_len)); break;
    case N_IDENT: { int s = var_slot(a, n->text, n->text_len); fprintf(a->out, "    movq v%d(%%rip), %%rax\n    pushq %%rax\n", s); break; }
    case N_UNARY:
        if (n->text_len == 1 && n->text[0] == '-') { gen_expr(a, n->kids[0]); fprintf(a->out, "    popq %%rax\n    negq %%rax\n    pushq %%rax\n"); }
        else a->ok = 0;
        break;
    case N_BINARY: gen_expr(a, n->kids[0]); gen_expr(a, n->kids[1]); bin_op(a, n->text, n->text_len); break;
    default: a->ok = 0; break;
    }
}

static void gen_stmt(Asm *a, const Node *n) {
    const Node *expr = NULL; int store = -1;
    if (n->kind == N_LET || n->kind == N_VAR) {
        const Node *pat = n->kids[0];
        const char *nm = pat->text; int nl = pat->text_len;
        for (int i = 1; i < n->nkid; i++) if (!(n->kids[i]->kind >= N_TYNAME && n->kids[i]->kind <= N_TYHOLE)) expr = n->kids[i];
        if (nm) store = var_slot(a, nm, nl);
    } else if (n->kind == N_EXPRSTMT) {
        expr = n->kids[0];
    } else return;
    if (!expr) return;
    gen_expr(a, expr);
    fprintf(a->out, "    popq %%rax\n");
    if (store >= 0) fprintf(a->out, "    movq %%rax, v%d(%%rip)\n", store);
    fprintf(a->out, "    movq %%rax, %%rdi\n    call print_int\n");
}

int asm_emit(const Node *module, FILE *out) {
    Asm a = {0}; a.out = out; a.ok = 1;
    /* body first into a temp buffer? We need var declarations up front. Emit code into memory via
       a temp file is overkill; instead do two passes: first count vars by walking, then emit. */
    /* Pass 1: discover variables (so .bss slots exist) by generating into /dev/null-like sink. */
    FILE *sink = tmpfile();
    Asm probe = {0}; probe.out = sink; probe.ok = 1;
    for (int i = 0; i < module->nkid; i++) gen_stmt(&probe, module->kids[i]);
    int nvars = probe.n;
    for (int i = 0; i < probe.n; i++) free(probe.names[i]);
    free(probe.names);
    if (sink) fclose(sink);

    fprintf(out, "    .section .bss\n    .lcomm pbuf, 32\n");
    for (int i = 0; i < nvars; i++) fprintf(out, "    .lcomm v%d, 8\n", i);
    fprintf(out, "    .section .text\n    .globl _start\n");
    /* print_int: rdi = value -> decimal + newline via write(1,...) */
    fprintf(out,
        "print_int:\n"
        "    movq %%rdi, %%rax\n"
        "    xorq %%r9, %%r9\n"
        "    testq %%rax, %%rax\n"
        "    jge 1f\n"
        "    negq %%rax\n"
        "    movq $1, %%r9\n"
        "1:\n"
        "    leaq pbuf+31(%%rip), %%rsi\n"
        "    movb $10, (%%rsi)\n"
        "    movq $1, %%rcx\n"
        "    movq $10, %%rbx\n"
        "2:\n"
        "    decq %%rsi\n"
        "    xorq %%rdx, %%rdx\n"
        "    divq %%rbx\n"
        "    addb $48, %%dl\n"
        "    movb %%dl, (%%rsi)\n"
        "    incq %%rcx\n"
        "    testq %%rax, %%rax\n"
        "    jnz 2b\n"
        "    testq %%r9, %%r9\n"
        "    je 3f\n"
        "    decq %%rsi\n"
        "    movb $45, (%%rsi)\n"
        "    incq %%rcx\n"
        "3:\n"
        "    movq $1, %%rax\n"
        "    movq $1, %%rdi\n"
        "    movq %%rcx, %%rdx\n"
        "    syscall\n"
        "    ret\n");
    fprintf(out, "_start:\n");
    for (int i = 0; i < module->nkid; i++) gen_stmt(&a, module->kids[i]);
    fprintf(out, "    movq $60, %%rax\n    xorq %%rdi, %%rdi\n    syscall\n");
    for (int i = 0; i < a.n; i++) free(a.names[i]);
    free(a.names);
    return a.ok ? 0 : 1;
}
