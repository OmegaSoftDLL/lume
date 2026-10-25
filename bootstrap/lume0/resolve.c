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

/* run name resolution over a parsed module, adding diagnostics */
void lume_resolve(const Node *module, DiagList *diags) {
    if (!module) return;
    StrSet def = {0};
    collect(module, &def);
    resolve_walk(module, &def, diags);
    sset_free(&def);
}
