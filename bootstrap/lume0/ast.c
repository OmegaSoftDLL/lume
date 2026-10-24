#include "ast.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

const char *node_kind_name(NodeKind k) {
    static const char *names[] = {
#define X(k) #k,
        NODE_KINDS(X)
#undef X
    };
    return (k >= 0 && k < N__COUNT) ? names[k] + 2 /* strip "N_" */ : "?";
}

/* ---------- arena: bump blocks ---------- */

#define ARENA_BLOCK (64 * 1024)
typedef struct Block { struct Block *next; size_t used; char data[ARENA_BLOCK]; } Block;
struct Arena { Block *head; };

Arena *arena_new(void) {
    Arena *a = calloc(1, sizeof *a);
    if (!a) { fputs("lume0: out of memory\n", stderr); exit(70); }
    return a;
}
void *arena_alloc(Arena *a, size_t n) {
    n = (n + 15) & ~(size_t)15;
    if (!a->head || a->head->used + n > ARENA_BLOCK) {
        size_t cap = n > ARENA_BLOCK ? n : ARENA_BLOCK;
        Block *b = malloc(sizeof(Block) - ARENA_BLOCK + cap);
        if (!b) { fputs("lume0: out of memory\n", stderr); exit(70); }
        b->next = a->head; b->used = 0; a->head = b;
    }
    void *p = a->head->data + a->head->used;
    a->head->used += n;
    return p;
}
void arena_free(Arena *a) {
    if (!a) return;
    for (Block *b = a->head; b;) { Block *n = b->next; free(b); b = n; }
    free(a);
}

Node *node_new(Arena *a, NodeKind kind, int line, int col) {
    Node *n = arena_alloc(a, sizeof *n);
    memset(n, 0, sizeof *n);
    n->kind = kind; n->line = line; n->col = col;
    return n;
}
void node_set_text(Node *n, const char *text, int len) { n->text = text; n->text_len = len; }

void node_add(Arena *a, Node *parent, Node *child) {
    if (!child) return;
    if (parent->nkid == parent->cap) {
        int nc = parent->cap ? parent->cap * 2 : 4;
        Node **k = arena_alloc(a, (size_t)nc * sizeof(Node *));
        if (parent->kids) memcpy(k, parent->kids, (size_t)parent->nkid * sizeof(Node *));
        parent->kids = k; parent->cap = nc;
    }
    parent->kids[parent->nkid++] = child;
}

/* ---------- JSON printer ---------- */

static void json_text(const char *s, int len) {
    putchar('"');
    for (int i = 0; i < len; i++) {
        unsigned char c = (unsigned char)s[i];
        switch (c) {
        case '"':  fputs("\\\"", stdout); break;
        case '\\': fputs("\\\\", stdout); break;
        case '\n': fputs("\\n", stdout); break;
        case '\t': fputs("\\t", stdout); break;
        case '\r': break;
        default:
            if (c < 0x20) printf("\\u%04x", c);
            else putchar((int)c);
        }
    }
    putchar('"');
}

static void flags_json(int f) {
    const struct { int bit; const char *name; } F[] = {
        {F_PUB,"pub"},{F_MUT,"mut"},{F_OWN,"own"},{F_DESC,"desc"},{F_INCLUSIVE,"inclusive"},
        {F_CRATE,"crate"},{F_EXTENSION,"extension"},{F_PAR,"par"},{F_REF,"ref"},{F_ASYNC,"async"},
    };
    bool first = true;
    fputs(",\"flags\":[", stdout);
    for (size_t i = 0; i < sizeof F / sizeof F[0]; i++)
        if (f & F[i].bit) { if (!first) putchar(','); json_text(F[i].name,(int)strlen(F[i].name)); first=false; }
    putchar(']');
}

static void print_node(const Node *n, int indent) {
    for (int i = 0; i < indent; i++) putchar(' ');
    printf("{\"kind\":\"%s\"", node_kind_name(n->kind));
    if (n->text) { fputs(",\"text\":", stdout); json_text(n->text, n->text_len); }
    if (n->flags) flags_json(n->flags);
    printf(",\"line\":%d,\"col\":%d", n->line, n->col);
    if (n->nkid) {
        fputs(",\"kids\":[\n", stdout);
        for (int i = 0; i < n->nkid; i++) {
            print_node(n->kids[i], indent + 2);
            fputs(i + 1 < n->nkid ? ",\n" : "\n", stdout);
        }
        for (int i = 0; i < indent; i++) putchar(' ');
        putchar(']');
    }
    putchar('}');
}

void ast_print_json(const Node *root) { print_node(root, 0); putchar('\n'); }

int lume_node_kind_count(void) { return N__COUNT; }
const char *lume_node_kind_at(int i) { return node_kind_name((NodeKind)i); }
