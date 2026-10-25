/* resolve.c — static name resolution for Lume. The first real semantic pass:
 * `lume0 check` no longer only lexes and parses, it reports references to names
 * that are not defined anywhere in the module. This is the start of the AI-first
 * promise -- the compiler catching the model's mistakes -- made real: a typo or a
 * reference to a name that does not exist is now an error at check time, not a
 * surprise at run time.
 *
 * Design: conservative, zero false positives by construction. Every binding name
 * in the module (parameters, let/var/const, loop and match bindings, closure
 * params, function/type/variant names, imported modules) is collected into one
 * flat set, together with the language builtins. A lowercase identifier used as a
 * value that is in neither set is undefined. Constructors and types (uppercase)
 * and member/method names (which live in FIELD_GET.text, not as identifier nodes)
 * are never flagged. Proper lexical scoping (use-before-def, out-of-scope use) is
 * a later pass; this one only catches names that exist nowhere. */
#include "lume0.h"
#include "ast.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct { char **v; int n, cap; } StrSet;
static int sset_has(const StrSet *s, const char *p, int len) {
    for (int i = 0; i < s->n; i++) if ((int)strlen(s->v[i]) == len && memcmp(s->v[i], p, (size_t)len) == 0) return 1;
    return 0;
}
static void sset_add(StrSet *s, const char *p, int len) {
    if (!p || len <= 0) return;
    if (sset_has(s, p, len)) return;
    if (s->n == s->cap) { s->cap = s->cap ? s->cap * 2 : 64; s->v = realloc(s->v, (size_t)s->cap * sizeof(char *)); }
    char *d = malloc((size_t)len + 1); memcpy(d, p, (size_t)len); d[len] = 0; s->v[s->n++] = d;
}
static void sset_free(StrSet *s) { for (int i = 0; i < s->n; i++) free(s->v[i]); free(s->v); }

static const char *BUILTINS[] = {
    "abs","all","any","args","ceil","chars","concat","contains","count","decimal",
    "drop","each","ends_with","entries","enumerate","env","eprintln","err","false",
    "filter","first","float","floor","fold","get","has","index_of","int","io","join",
    "keys","last","len","lower","main","map","math","max","min","ok","old","panic",
    "parse_int","pop","pow","print","println","push","range","rb","read_file","reduce",
    "repeat","replace","result","reverse","round","self","some","sorted","split","sqrt",
    "starts_with","str","sum","take","trim","true","upper","usize","values","wb",
    "write_file","zip", NULL
};
static int builtin_has(const char *p, int len) {
    for (int i = 0; BUILTINS[i]; i++) if ((int)strlen(BUILTINS[i]) == len && memcmp(BUILTINS[i], p, (size_t)len) == 0) return 1;
    return 0;
}

/* a node kind that introduces names via its direct IDENT children */
static int is_decl_kind(NodeKind k) {
    return k == N_PARAM || k == N_FN || k == N_CONST || k == N_TYPEDECL || k == N_RECORD ||
           k == N_ENUM || k == N_VARIANT || k == N_TRAIT || k == N_IMPL || k == N_GENERIC ||
           k == N_GENERICS || k == N_USE || k == N_CLOSPARAM || k == N_ERRORDEF || k == N_FIELD;
}

static void collect(const Node *n, StrSet *def) {
    if (!n) return;
    if (n->kind == N_PBIND) sset_add(def, n->text, n->text_len);
    /* `use mod (a, b)` import groups currently parse as a trailing tuple of
     * identifiers; treat identifiers named inside a tuple as defined so imported
     * symbols are not misreported (a tuple is not a place that introduces typos). */
    if (n->kind == N_TUPLE) {
        for (int i = 0; i < n->nkid; i++) if (n->kids[i]->kind == N_IDENT) sset_add(def, n->kids[i]->text, n->kids[i]->text_len);
    }
    if (is_decl_kind(n->kind)) {
        if (n->text) sset_add(def, n->text, n->text_len);
        for (int i = 0; i < n->nkid; i++) if (n->kids[i]->kind == N_IDENT) sset_add(def, n->kids[i]->text, n->kids[i]->text_len);
    }
    for (int i = 0; i < n->nkid; i++) collect(n->kids[i], def);
}

static void resolve_walk(const Node *n, const StrSet *def, DiagList *d) {
    if (!n) return;
    if (n->kind == N_USE) return;   /* module path segments are not value refs */
    if (n->kind == N_OPTCHAIN) {    /* x?.member : only the object is a value ref */
        if (n->nkid > 0) resolve_walk(n->kids[0], def, d);
        return;
    }
    if (n->kind == N_IDENT) {
        if (n->text && n->text_len > 0) {
            char c0 = n->text[0];
            int is_upper = (c0 >= 'A' && c0 <= 'Z');   /* constructor / type */
            if (!is_upper && !sset_has(def, n->text, n->text_len) && !builtin_has(n->text, n->text_len)) {
                char nm[64]; int L = n->text_len > 63 ? 63 : n->text_len;
                memcpy(nm, n->text, (size_t)L); nm[L] = 0;
                char msg[200]; snprintf(msg, sizeof msg, "undefined name `%s`", nm);
                diag_add(d, SEV_ERROR, "R0200", n->line, n->col, n->text_len, msg,
                         "declare it with let/var/fn, import it with use, or fix the spelling");
            }
        }
        return;   /* identifiers have no children to recurse */
    }
    for (int i = 0; i < n->nkid; i++) resolve_walk(n->kids[i], def, d);
}

/* ---- basic literal type checking against an explicit annotation ----
 * Only the clearest, zero-false-positive case: a let/var/const with an explicit
 * scalar type annotation AND a literal initializer of an obviously different
 * kind (e.g. `let x: int = "abc"`). Unknown/compound types and non-literal
 * initializers are left to the later full type checker. */
static int type_is(const char *t, int tl, const char *name) {
    return tl == (int)strlen(name) && memcmp(t, name, (size_t)tl) == 0;
}
static const char *lit_kind_name(NodeKind k) {
    return k == N_INT ? "integer" : k == N_STR ? "string" : k == N_BOOL ? "boolean" : k == N_FLOAT ? "float" : NULL;
}
/* 1 ok, 0 mismatch, -1 unknown (skip) */
static int lit_matches_type(const char *t, int tl, NodeKind lk) {
    int isint = type_is(t,tl,"int")||type_is(t,tl,"i8")||type_is(t,tl,"i16")||type_is(t,tl,"i32")||type_is(t,tl,"i64")||
                type_is(t,tl,"u8")||type_is(t,tl,"u16")||type_is(t,tl,"u32")||type_is(t,tl,"u64")||type_is(t,tl,"usize")||type_is(t,tl,"isize")||type_is(t,tl,"byte");
    int isstr = type_is(t,tl,"str")||type_is(t,tl,"string");
    int isbool= type_is(t,tl,"bool");
    int isflt = type_is(t,tl,"f32")||type_is(t,tl,"f64")||type_is(t,tl,"float")||type_is(t,tl,"decimal");
    if (isint) return lk == N_INT ? 1 : 0;
    if (isstr) return lk == N_STR ? 1 : 0;
    if (isbool) return lk == N_BOOL ? 1 : 0;
    if (isflt) return (lk == N_FLOAT || lk == N_INT) ? 1 : 0;
    return -1;
}
static int is_type_node(NodeKind k) { return k >= N_TYNAME && k <= N_TYHOLE; }

static void typecheck_walk(const Node *n, DiagList *d) {
    if (!n) return;
    if (n->kind == N_LET || n->kind == N_VAR || n->kind == N_CONST) {
        const Node *ty = NULL, *val = NULL;
        for (int i = 1; i < n->nkid; i++) {
            if (is_type_node(n->kids[i]->kind)) ty = n->kids[i];
            else val = n->kids[i];
        }
        if (ty && ty->kind == N_TYNAME && ty->text && val) {
            const char *lkname = lit_kind_name(val->kind);
            if (lkname) {
                int ok = lit_matches_type(ty->text, ty->text_len, val->kind);
                if (ok == 0) {
                    char tn[48]; int tl = ty->text_len > 47 ? 47 : ty->text_len;
                    memcpy(tn, ty->text, (size_t)tl); tn[tl] = 0;
                    char msg[200]; snprintf(msg, sizeof msg, "type mismatch: annotated `%s` but initialized with a %s literal", tn, lkname);
                    diag_add(d, SEV_ERROR, "T0001", val->line, val->col, val->text_len, msg,
                             "make the value match the type, or change the annotation");
                }
            }
        }
    }
    for (int i = 0; i < n->nkid; i++) typecheck_walk(n->kids[i], d);
}

/* ---- duplicate detection: repeated parameter names, repeated top-level fns ---- */
static const Node *name_node(const Node *n) {
    for (int i = 0; i < n->nkid; i++) if (n->kids[i]->kind == N_IDENT) return n->kids[i];
    return NULL;
}
static void dup_params(const Node *fn, DiagList *d) {
    StrSet seen = {0};
    for (int i = 0; i < fn->nkid; i++) {
        if (fn->kids[i]->kind != N_PARAM) continue;
        const Node *nm = name_node(fn->kids[i]);
        const char *p = nm ? nm->text : fn->kids[i]->text;
        int pl = nm ? nm->text_len : fn->kids[i]->text_len;
        if (!p || pl <= 0) continue;
        if (sset_has(&seen, p, pl)) {
            char b[48]; int L = pl > 47 ? 47 : pl; memcpy(b, p, (size_t)L); b[L] = 0;
            char msg[160]; snprintf(msg, sizeof msg, "duplicate parameter `%s`", b);
            int line = nm ? nm->line : fn->kids[i]->line, col = nm ? nm->col : fn->kids[i]->col;
            diag_add(d, SEV_ERROR, "D0001", line, col, pl, msg, "each parameter name must be unique");
        } else sset_add(&seen, p, pl);
    }
    sset_free(&seen);
}
static void dup_walk(const Node *n, DiagList *d) {
    if (!n) return;
    if (n->kind == N_FN) dup_params(n, d);
    for (int i = 0; i < n->nkid; i++) dup_walk(n->kids[i], d);
}
static void dup_toplevel_fns(const Node *module, DiagList *d) {
    StrSet seen = {0};
    for (int i = 0; i < module->nkid; i++) {
        if (module->kids[i]->kind != N_FN) continue;
        const Node *nm = name_node(module->kids[i]);
        if (!nm || !nm->text) continue;
        if (sset_has(&seen, nm->text, nm->text_len)) {
            char b[48]; int L = nm->text_len > 47 ? 47 : nm->text_len; memcpy(b, nm->text, (size_t)L); b[L] = 0;
            char msg[160]; snprintf(msg, sizeof msg, "duplicate function `%s`", b);
            diag_add(d, SEV_ERROR, "D0002", nm->line, nm->col, nm->text_len, msg, "rename one of the definitions");
        } else sset_add(&seen, nm->text, nm->text_len);
    }
    sset_free(&seen);
}

/* ---- call arity checking for direct calls to top-level functions ----
 * Conservative: only N_CALL whose callee is a bare lowercase identifier naming a
 * top-level fn. Methods (FIELD_GET callee), builtins, constructors and piped calls
 * are not touched. A function with a parameter we cannot read plainly (rest, pattern,
 * unknown) is skipped entirely, so there are no false positives. */
typedef struct { const char *name; int len; int minreq, total, skip; } FnSig;
typedef struct { FnSig *v; int n, cap; } FnSigs;
static const FnSig *sig_find(const FnSigs *s, const char *p, int len) {
    for (int i = 0; i < s->n; i++) if (s->v[i].len == len && memcmp(s->v[i].name, p, (size_t)len) == 0) return &s->v[i];
    return NULL;
}
static void sigs_build(const Node *module, FnSigs *out) {
    for (int i = 0; i < module->nkid; i++) {
        const Node *fn = module->kids[i];
        if (fn->kind != N_FN) continue;
        const Node *nm = name_node(fn);
        if (!nm || !nm->text) continue;
        int minreq = 0, total = 0, skip = 0;
        for (int j = 0; j < fn->nkid; j++) {
            if (fn->kids[j]->kind != N_PARAM) continue;
            total++;
            const Node *pn = NULL; int hasdef = 0;
            for (int k = 0; k < fn->kids[j]->nkid; k++) {
                NodeKind ck = fn->kids[j]->kids[k]->kind;
                if (ck == N_IDENT && !pn) pn = fn->kids[j]->kids[k];
                else if (is_type_node(ck)) { /* type annotation */ }
                else hasdef = 1;   /* a default value makes the parameter optional */
            }
            if (!pn && !fn->kids[j]->text) skip = 1;   /* not a plain named parameter */
            if (!hasdef) minreq++;
        }
        if (out->n == out->cap) { out->cap = out->cap ? out->cap * 2 : 32; out->v = realloc(out->v, (size_t)out->cap * sizeof(FnSig)); }
        out->v[out->n].name = nm->text; out->v[out->n].len = nm->text_len;
        out->v[out->n].minreq = minreq; out->v[out->n].total = total; out->v[out->n].skip = skip;
        out->n++;
    }
}
static void arity_walk(const Node *n, const FnSigs *sigs, DiagList *d) {
    if (!n) return;
    if (n->kind == N_CALL && n->nkid >= 1 && n->kids[0]->kind == N_IDENT) {
        const Node *callee = n->kids[0];
        char c0 = callee->text && callee->text_len > 0 ? callee->text[0] : 0;
        if (!(c0 >= 'A' && c0 <= 'Z')) {
            const FnSig *sig = sig_find(sigs, callee->text, callee->text_len);
            if (sig && !sig->skip) {
                int argc = 0;
                for (int i = 1; i < n->nkid; i++) if (n->kids[i]->kind == N_ARG) argc++;
                char nm[48]; int L = callee->text_len > 47 ? 47 : callee->text_len;
                memcpy(nm, callee->text, (size_t)L); nm[L] = 0;
                if (argc > sig->total) {
                    char msg[200]; snprintf(msg, sizeof msg, "too many arguments to `%s`: it takes %d, got %d", nm, sig->total, argc);
                    diag_add(d, SEV_ERROR, "A0001", callee->line, callee->col, callee->text_len, msg, "pass the right number of arguments");
                } else if (argc < sig->minreq) {
                    char msg[200]; snprintf(msg, sizeof msg, "too few arguments to `%s`: it needs %d, got %d", nm, sig->minreq, argc);
                    diag_add(d, SEV_ERROR, "A0002", callee->line, callee->col, callee->text_len, msg, "pass the required arguments");
                }
            }
        }
    }
    for (int i = 0; i < n->nkid; i++) arity_walk(n->kids[i], sigs, d);
}

/* ---- control-flow context: break/continue only in a loop, return only in a fn ----
 * A function or closure boundary resets the loop context (break cannot cross it). */
static void ctx_walk(const Node *n, int fn_depth, int loop_depth, DiagList *d) {
    if (!n) return;
    switch (n->kind) {
    case N_BREAK:
    case N_CONTINUE:
        if (loop_depth == 0)
            diag_add(d, SEV_ERROR, "C0001", n->line, n->col, n->text_len ? n->text_len : 1,
                     n->kind == N_BREAK ? "`break` outside a loop" : "`continue` outside a loop",
                     "break/continue may only appear inside while/for/loop");
        return;
    case N_RETURN:
        if (fn_depth == 0)
            diag_add(d, SEV_ERROR, "C0002", n->line, n->col, n->text_len ? n->text_len : 1,
                     "`return` outside a function", "return may only appear inside a function body");
        for (int i = 0; i < n->nkid; i++) ctx_walk(n->kids[i], fn_depth, loop_depth, d);
        return;
    case N_FN:
    case N_CLOSURE:
        for (int i = 0; i < n->nkid; i++) ctx_walk(n->kids[i], fn_depth + 1, 0, d);
        return;
    case N_WHILE:
    case N_FOR:
    case N_LOOP:
        for (int i = 0; i < n->nkid; i++) ctx_walk(n->kids[i], fn_depth, loop_depth + 1, d);
        return;
    default:
        for (int i = 0; i < n->nkid; i++) ctx_walk(n->kids[i], fn_depth, loop_depth, d);
        return;
    }
}

/* run name resolution + basic literal type checks + duplicate detection + arity */
void lume_resolve(const Node *module, DiagList *diags) {
    if (!module) return;
    StrSet def = {0};
    collect(module, &def);
    resolve_walk(module, &def, diags);
    sset_free(&def);
    typecheck_walk(module, diags);
    dup_walk(module, diags);
    dup_toplevel_fns(module, diags);
    FnSigs sigs = {0};
    sigs_build(module, &sigs);
    arity_walk(module, &sigs, diags);
    free(sigs.v);
    ctx_walk(module, 0, 0, diags);
}
