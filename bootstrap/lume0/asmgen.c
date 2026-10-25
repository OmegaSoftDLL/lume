/* asmgen.c — direct x86-64 (Linux) backend for the bootstrap: Lume AST -> assembly, no C.
 * v2 scope: top-level let/var + integer arithmetic/comparison, user-defined functions with
 * parameters (recursion via a real rbp stack frame) and if-expressions. Each top-level statement
 * prints its value as a decimal line via raw syscalls; exit 0. No libc, no C in the pipeline. */
#include "ast.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static const char *ARGREG[6] = { "%rdi", "%rsi", "%rdx", "%rcx", "%r8", "%r9" };

typedef struct { char **names; int n, cap; } Names;
static int names_find(const Names *v, const char *s, int len) {
    for (int i = 0; i < v->n; i++) if ((int)strlen(v->names[i]) == len && memcmp(v->names[i], s, (size_t)len) == 0) return i;
    return -1;
}
static int names_add(Names *v, const char *s, int len) {
    int i = names_find(v, s, len); if (i >= 0) return i;
    if (v->n == v->cap) { v->cap = v->cap ? v->cap*2 : 8; v->names = realloc(v->names, (size_t)v->cap*sizeof(char*)); }
    char *d = malloc((size_t)len+1); memcpy(d, s, (size_t)len); d[len]=0; v->names[v->n]=d; return v->n++;
}
static void names_free(Names *v) { for (int i=0;i<v->n;i++) free(v->names[i]); free(v->names); }

typedef struct { FILE *out; Names globals; const Names *locals; int label; int ok; } Asm;

static long long pint(const char *s, int len) {
    long long v=0; int base=10,i=0;
    if (len>2 && s[0]=='0' && (s[1]=='x'||s[1]=='b'||s[1]=='o')) { base=s[1]=='x'?16:s[1]=='b'?2:8; i=2; }
    for (; i<len; i++){ char c=s[i]; if(c=='_')continue; int d=(c>='0'&&c<='9')?c-'0':(c>='a'&&c<='f')?c-'a'+10:(c>='A'&&c<='F')?c-'A'+10:0; v=v*base+d; }
    return v;
}

static void gen_expr(Asm *a, const Node *n);
static void gen_stmt(Asm *a, const Node *n);
static void gen_block_value(Asm *a, const Node *n);

/* collect local binding names (var/let) reachable in a function body, so the
 * frame reserves a slot for each. Params are added separately, before this. */
static void collect_locals(const Node *n, Names *v) {
    if (!n) return;
    if (n->kind == N_VAR || n->kind == N_LET || n->kind == N_FOR) {
        const Node *pat = n->nkid ? n->kids[0] : NULL;
        if (pat && pat->text) names_add(v, pat->text, (int)pat->text_len);
    }
    for (int i = 0; i < n->nkid; i++) collect_locals(n->kids[i], v);
}

static void gen_bin(Asm *a, const char *op, int oplen) {
    fprintf(a->out, "    popq %%rcx\n    popq %%rax\n");
    if (oplen==1) switch(op[0]){
        case '+': fprintf(a->out,"    addq %%rcx, %%rax\n"); break;
        case '-': fprintf(a->out,"    subq %%rcx, %%rax\n"); break;
        case '*': fprintf(a->out,"    imulq %%rcx, %%rax\n"); break;
        case '/': fprintf(a->out,"    cqto\n    idivq %%rcx\n"); break;
        case '%': fprintf(a->out,"    cqto\n    idivq %%rcx\n    movq %%rdx, %%rax\n"); break;
        case '<': fprintf(a->out,"    cmpq %%rcx, %%rax\n    setl %%al\n    movzbq %%al, %%rax\n"); break;
        case '>': fprintf(a->out,"    cmpq %%rcx, %%rax\n    setg %%al\n    movzbq %%al, %%rax\n"); break;
        default: a->ok=0;
    } else if (oplen==2) {
        if(!memcmp(op,"==",2)) fprintf(a->out,"    cmpq %%rcx, %%rax\n    sete %%al\n    movzbq %%al, %%rax\n");
        else if(!memcmp(op,"!=",2)) fprintf(a->out,"    cmpq %%rcx, %%rax\n    setne %%al\n    movzbq %%al, %%rax\n");
        else if(!memcmp(op,"<=",2)) fprintf(a->out,"    cmpq %%rcx, %%rax\n    setle %%al\n    movzbq %%al, %%rax\n");
        else if(!memcmp(op,">=",2)) fprintf(a->out,"    cmpq %%rcx, %%rax\n    setge %%al\n    movzbq %%al, %%rax\n");
        else if(!memcmp(op,"or",2)) fprintf(a->out,"    cmpq $0, %%rax\n    setne %%al\n    cmpq $0, %%rcx\n    setne %%cl\n    orb %%cl, %%al\n    movzbq %%al, %%rax\n");
        else a->ok=0;
    } else if (oplen==3 && !memcmp(op,"and",3)) {
        fprintf(a->out,"    cmpq $0, %%rax\n    setne %%al\n    cmpq $0, %%rcx\n    setne %%cl\n    andb %%cl, %%al\n    movzbq %%al, %%rax\n");
    } else a->ok=0;
    fprintf(a->out, "    pushq %%rax\n");
}

static void gen_expr(Asm *a, const Node *n) {
    switch (n->kind) {
    case N_INT: fprintf(a->out, "    movq $%lld, %%rax\n    pushq %%rax\n", pint(n->text, n->text_len)); break;
    case N_IDENT: {
        int li = a->locals ? names_find(a->locals, n->text, n->text_len) : -1;
        if (li >= 0) fprintf(a->out, "    movq %d(%%rbp), %%rax\n    pushq %%rax\n", -8*(li+1));
        else { int g = names_add(&a->globals, n->text, n->text_len); fprintf(a->out, "    movq v%d(%%rip), %%rax\n    pushq %%rax\n", g); }
        break;
    }
    case N_UNARY:
        if (n->text_len==1 && n->text[0]=='-') { gen_expr(a, n->kids[0]); fprintf(a->out, "    popq %%rax\n    negq %%rax\n    pushq %%rax\n"); }
        else if ((n->text_len==3 && !memcmp(n->text,"not",3)) || (n->text_len==1 && n->text[0]=='!')) { gen_expr(a, n->kids[0]); fprintf(a->out, "    popq %%rax\n    cmpq $0, %%rax\n    sete %%al\n    movzbq %%al, %%rax\n    pushq %%rax\n"); }
        else a->ok=0;
        break;
    case N_BINARY: gen_expr(a, n->kids[0]); gen_expr(a, n->kids[1]); gen_bin(a, n->text, n->text_len); break;
    case N_IFEXPR: {
        int L = a->label++; 
        gen_expr(a, n->kids[0]);                         /* cond -> stack */
        fprintf(a->out, "    popq %%rax\n    testq %%rax, %%rax\n    jz .Lelse%d\n", L);
        gen_expr(a, n->kids[1]);                         /* then -> stack */
        fprintf(a->out, "    jmp .Lend%d\n.Lelse%d:\n", L, L);
        gen_expr(a, n->kids[2]);                         /* else -> stack */
        fprintf(a->out, ".Lend%d:\n", L);
        break;
    }
    case N_CALL: {
        const Node *callee = n->kids[0];
        if (callee->kind != N_IDENT) { a->ok = 0; break; }
        int k = 0;
        for (int i = 1; i < n->nkid; i++) if (n->kids[i]->kind == N_ARG) { gen_expr(a, n->kids[i]->kids[n->kids[i]->nkid-1]); k++; }
        if (k > 6) { a->ok = 0; break; }
        for (int i = k - 1; i >= 0; i--) fprintf(a->out, "    popq %s\n", ARGREG[i]);
        fprintf(a->out, "    call %.*s\n    pushq %%rax\n", callee->text_len, callee->text);
        break;
    }
    case N_BLOCK: case N_DOBLOCK: gen_block_value(a, n); break;
    case N_IF: {
        /* an if/else (or if/elif/else) in value position: each branch yields a value */
        int L = a->label++;
        gen_expr(a, n->kids[0]);
        fprintf(a->out, "    popq %%rax\n    testq %%rax, %%rax\n    jz .Lie%d\n", L);
        const Node *th = n->kids[1];
        if (th->kind == N_BLOCK || th->kind == N_DOBLOCK) gen_block_value(a, th); else gen_expr(a, th);
        fprintf(a->out, "    jmp .Lix%d\n.Lie%d:\n", L, L);
        if (n->nkid >= 3) {
            const Node *el = n->kids[2];
            if (el->kind == N_BLOCK || el->kind == N_DOBLOCK) gen_block_value(a, el); else gen_expr(a, el);
        } else fprintf(a->out, "    pushq $0\n");
        fprintf(a->out, ".Lix%d:\n", L);
        break;
    }
    default: a->ok = 0; break;
    }
}

/* slot for an identifier being assigned/bound: a local frame slot if known,
 * otherwise a global .bss slot. Sets *islocal. */
static int slot_of(Asm *a, const Node *id, int *islocal) {
    int li = a->locals ? names_find(a->locals, id->text, (int)id->text_len) : -1;
    if (li >= 0) { *islocal = 1; return li; }
    *islocal = 0; return names_add(&a->globals, id->text, (int)id->text_len);
}
static void store_top(Asm *a, int slot, int islocal) {
    if (islocal) fprintf(a->out, "    popq %%rax\n    movq %%rax, %d(%%rbp)\n", -8*(slot+1));
    else fprintf(a->out, "    popq %%rax\n    movq %%rax, v%d(%%rip)\n", slot);
}
static void load_slot(Asm *a, int slot, int islocal) {
    if (islocal) fprintf(a->out, "    movq %d(%%rbp), %%rax\n    pushq %%rax\n", -8*(slot+1));
    else fprintf(a->out, "    movq v%d(%%rip), %%rax\n    pushq %%rax\n", slot);
}

/* a statement: leaves nothing on the value stack. */
static void gen_stmt(Asm *a, const Node *n) {
    switch (n->kind) {
    case N_VAR: case N_LET: {
        const Node *pat = n->kids[0]; const Node *expr = NULL;
        for (int i = 1; i < n->nkid; i++) if (!(n->kids[i]->kind >= N_TYNAME && n->kids[i]->kind <= N_TYHOLE)) expr = n->kids[i];
        if (!expr) { a->ok = 0; break; }
        gen_expr(a, expr);
        if (pat->text) { int il; int s = slot_of(a, pat, &il); store_top(a, s, il); }
        else fprintf(a->out, "    addq $8, %%rsp\n");
        break;
    }
    case N_ASSIGN: {
        const Node *tgt = n->kids[0];
        if (tgt->kind != N_IDENT) { a->ok = 0; break; }
        gen_expr(a, n->kids[1]);
        int il; int s = slot_of(a, tgt, &il); store_top(a, s, il);
        break;
    }
    case N_WHILE: {
        int L = a->label++;
        fprintf(a->out, ".Lw%d:\n", L);
        gen_expr(a, n->kids[0]);
        fprintf(a->out, "    popq %%rax\n    testq %%rax, %%rax\n    jz .Lwe%d\n", L);
        gen_stmt(a, n->kids[1]);
        fprintf(a->out, "    jmp .Lw%d\n.Lwe%d:\n", L, L);
        break;
    }
    case N_IF: {
        int L = a->label++;
        gen_expr(a, n->kids[0]);
        fprintf(a->out, "    popq %%rax\n    testq %%rax, %%rax\n    jz .Lse%d\n", L);
        gen_stmt(a, n->kids[1]);
        fprintf(a->out, "    jmp .Lsx%d\n.Lse%d:\n", L, L);
        if (n->nkid >= 3) gen_stmt(a, n->kids[2]);
        fprintf(a->out, ".Lsx%d:\n", L);
        break;
    }
    case N_FOR: {
        const Node *pat = n->kids[0], *rng = n->kids[1], *body = n->kids[2];
        if (pat->kind != N_PBIND || rng->kind != N_RANGE || rng->nkid < 2) { a->ok = 0; break; }
        int il; int s = slot_of(a, pat, &il);
        int inc = (rng->flags & F_INCLUSIVE) != 0;
        int L = a->label++;
        gen_expr(a, rng->kids[0]); store_top(a, s, il);     /* i = lo */
        fprintf(a->out, ".Lf%d:\n", L);
        load_slot(a, s, il);                                 /* i */
        gen_expr(a, rng->kids[1]);                           /* hi */
        gen_bin(a, inc ? "<=" : "<", inc ? 2 : 1);
        fprintf(a->out, "    popq %%rax\n    testq %%rax, %%rax\n    jz .Lfe%d\n", L);
        gen_stmt(a, body);
        load_slot(a, s, il);
        fprintf(a->out, "    popq %%rax\n    incq %%rax\n    pushq %%rax\n");
        store_top(a, s, il);                                 /* i = i + 1 */
        fprintf(a->out, "    jmp .Lf%d\n.Lfe%d:\n", L, L);
        break;
    }
    case N_BLOCK: case N_DOBLOCK:
        for (int i = 0; i < n->nkid; i++) gen_stmt(a, n->kids[i]);
        break;
    case N_RETURN:
        if (n->nkid >= 1) gen_expr(a, n->kids[0]); else fprintf(a->out, "    pushq $0\n");
        fprintf(a->out, "    popq %%rax\n    movq %%rbp, %%rsp\n    popq %%rbp\n    ret\n");
        break;
    case N_EXPRSTMT:
        gen_expr(a, n->kids[0]);
        if (a->ok) fprintf(a->out, "    addq $8, %%rsp\n");
        break;
    default:
        gen_expr(a, n);
        if (a->ok) fprintf(a->out, "    addq $8, %%rsp\n");
        break;
    }
}

/* a block in value position: statements for effect, last one yields the value. */
static void gen_block_value(Asm *a, const Node *n) {
    int m = n->nkid;
    if (m == 0) { fprintf(a->out, "    pushq $0\n"); return; }
    for (int i = 0; i < m - 1; i++) gen_stmt(a, n->kids[i]);
    const Node *last = n->kids[m-1];
    if (last->kind == N_EXPRSTMT) gen_expr(a, last->kids[0]);
    else if (last->kind == N_BLOCK || last->kind == N_DOBLOCK) gen_block_value(a, last);
    else if (last->kind == N_IF || last->kind == N_IFEXPR) gen_expr(a, last);
    else if (last->kind == N_RETURN) gen_stmt(a, last);
    else { gen_stmt(a, last); fprintf(a->out, "    pushq $0\n"); }
}

static const Node *fn_body_of(const Node *fn, Names *params) {
    const Node *body = NULL; bool seen_name = false;
    for (int i = 0; i < fn->nkid; i++) {
        const Node *k = fn->kids[i];
        if (k->kind == N_IDENT && !seen_name) { seen_name = true; continue; }
        if (k->kind == N_PARAM) { const Node *nm = k->text ? k : (k->nkid ? k->kids[0] : NULL); if (nm) names_add(params, nm->text ? nm->text : k->kids[0]->text, nm->text ? nm->text_len : k->kids[0]->text_len); continue; }
        if (k->kind == N_DOC || k->kind == N_ATTR || k->kind == N_GENERICS || k->kind == N_EFFECTS || k->kind == N_FROM) continue;
        if (k->kind >= N_TYNAME && k->kind <= N_TYHOLE) continue;
        body = k;
    }
    return body;
}

static void gen_function(Asm *a, const Node *fn) {
    Names locals = {0};
    const Node *body = fn_body_of(fn, &locals);   /* params first */
    int nparams = locals.n;
    collect_locals(body, &locals);                /* then var/let slots */
    const char *name = NULL; int namelen = 0;
    for (int i = 0; i < fn->nkid; i++) if (fn->kids[i]->kind == N_IDENT) { name = fn->kids[i]->text; namelen = fn->kids[i]->text_len; break; }
    int frame = ((locals.n * 8) + 15) & ~15;
    fprintf(a->out, "%.*s:\n    pushq %%rbp\n    movq %%rsp, %%rbp\n", namelen, name);
    if (frame) fprintf(a->out, "    subq $%d, %%rsp\n", frame);
    for (int i = 0; i < nparams; i++) fprintf(a->out, "    movq %s, %d(%%rbp)\n", ARGREG[i], -8*(i+1));
    const Names *saved = a->locals; a->locals = &locals;
    if (body && (body->kind == N_BLOCK || body->kind == N_DOBLOCK)) gen_block_value(a, body);
    else if (body) gen_expr(a, body);
    else fprintf(a->out, "    pushq $0\n");
    a->locals = saved;
    fprintf(a->out, "    popq %%rax\n    movq %%rbp, %%rsp\n    popq %%rbp\n    ret\n");
    names_free(&locals);
}

static void gen_toplevel_stmt(Asm *a, const Node *n) {
    const Node *expr = NULL; int store = -1;
    if (n->kind == N_LET || n->kind == N_VAR) {
        const Node *pat = n->kids[0];
        for (int i = 1; i < n->nkid; i++) if (!(n->kids[i]->kind >= N_TYNAME && n->kids[i]->kind <= N_TYHOLE)) expr = n->kids[i];
        if (pat->text) store = names_add(&a->globals, pat->text, pat->text_len);
    } else if (n->kind == N_EXPRSTMT) expr = n->kids[0];
    else return;
    if (!expr) return;
    gen_expr(a, expr);
    fprintf(a->out, "    popq %%rax\n");
    if (store >= 0) fprintf(a->out, "    movq %%rax, v%d(%%rip)\n", store);
    fprintf(a->out, "    movq %%rax, %%rdi\n    call print_int\n");
}

int asm_emit(const Node *module, FILE *out) {
    Asm a = {0}; a.out = out; a.ok = 1;
    /* discover globals (top-level let/var) so .bss slots exist */
    for (int i = 0; i < module->nkid; i++) { const Node *s = module->kids[i];
        if ((s->kind == N_LET || s->kind == N_VAR) && s->nkid && s->kids[0]->text) names_add(&a.globals, s->kids[0]->text, s->kids[0]->text_len); }
    int nglob = a.globals.n;
    fprintf(out, "    .section .bss\n    .lcomm pbuf, 32\n");
    for (int i = 0; i < nglob; i++) fprintf(out, "    .lcomm v%d, 8\n", i);
    fprintf(out, "    .section .text\n");
    /* user functions */
    for (int i = 0; i < module->nkid; i++) if (module->kids[i]->kind == N_FN) gen_function(&a, module->kids[i]);
    /* print_int */
    fprintf(out,
        "print_int:\n    movq %%rdi, %%rax\n    xorq %%r9, %%r9\n    testq %%rax, %%rax\n    jge 1f\n    negq %%rax\n    movq $1, %%r9\n1:\n"
        "    leaq pbuf+31(%%rip), %%rsi\n    movb $10, (%%rsi)\n    movq $1, %%rcx\n    movq $10, %%rbx\n"
        "2:\n    decq %%rsi\n    xorq %%rdx, %%rdx\n    divq %%rbx\n    addb $48, %%dl\n    movb %%dl, (%%rsi)\n    incq %%rcx\n    testq %%rax, %%rax\n    jnz 2b\n"
        "    testq %%r9, %%r9\n    je 3f\n    decq %%rsi\n    movb $45, (%%rsi)\n    incq %%rcx\n3:\n"
        "    movq $1, %%rax\n    movq $1, %%rdi\n    movq %%rcx, %%rdx\n    syscall\n    ret\n");
    fprintf(out, "    .globl _start\n_start:\n");
    for (int i = 0; i < module->nkid; i++) gen_toplevel_stmt(&a, module->kids[i]);
    fprintf(out, "    movq $60, %%rax\n    xorq %%rdi, %%rdi\n    syscall\n");
    names_free(&a.globals);
    return a.ok ? 0 : 1;
}
