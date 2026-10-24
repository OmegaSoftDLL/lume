/* Lume Binary Encoding (LBE): a dense binary serialization of the AST.
 * The AI-first bet made concrete -- a program as structure the machine emits and loads directly,
 * with no text to re-parse (so a syntactically-invalid program cannot exist). Text is only a
 * human-review projection; LBE is the native substrate. The compiler lowers LBE to the actual
 * machine bits. See docs/08-ai-first.md and docs/09-lbe.md. */
#include "ast.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* unsigned LEB128 varint */
static void put_uv(FILE *f, unsigned long long v) {
    do { unsigned char b = v & 0x7F; v >>= 7; if (v) b |= 0x80; fputc(b, f); } while (v);
}
static unsigned long long get_uv(const unsigned char *buf, size_t n, size_t *pos) {
    unsigned long long v = 0; int shift = 0;
    while (*pos < n) { unsigned char b = buf[(*pos)++]; v |= (unsigned long long)(b & 0x7F) << shift; if (!(b & 0x80)) break; shift += 7; }
    return v;
}

#define LBE_MAGIC "LBE1"

static void write_node(FILE *f, const Node *n) {
    put_uv(f, (unsigned long long)n->kind);
    put_uv(f, (unsigned long long)n->flags);
    put_uv(f, (unsigned long long)n->line);
    put_uv(f, (unsigned long long)n->col);
    if (n->text) { put_uv(f, (unsigned long long)n->text_len + 1); fwrite(n->text, 1, (size_t)n->text_len, f); }
    else put_uv(f, 0);
    put_uv(f, (unsigned long long)n->nkid);
    for (int i = 0; i < n->nkid; i++) write_node(f, n->kids[i]);
}

void lbe_write(const Node *root, FILE *f) {
    fwrite(LBE_MAGIC, 1, 4, f);
    write_node(f, root);
}

static Node *read_node(const unsigned char *buf, size_t n, size_t *pos, Arena *a) {
    int kind = (int)get_uv(buf, n, pos);
    int flags = (int)get_uv(buf, n, pos);
    int line = (int)get_uv(buf, n, pos);
    int col = (int)get_uv(buf, n, pos);
    unsigned long long tl = get_uv(buf, n, pos);
    Node *node = node_new(a, (NodeKind)kind, line, col);
    node->flags = flags;
    if (tl > 0) {
        int len = (int)(tl - 1);
        char *s = arena_alloc(a, (size_t)len + 1);
        if (len > 0 && *pos + (size_t)len <= n) memcpy(s, buf + *pos, (size_t)len);
        s[len] = 0; *pos += (size_t)len;
        node_set_text(node, s, len);
    }
    int nk = (int)get_uv(buf, n, pos);
    for (int i = 0; i < nk; i++) node_add(a, node, read_node(buf, n, pos, a));
    return node;
}

Node *lbe_read(const unsigned char *buf, size_t n, Arena *a) {
    if (n < 4 || memcmp(buf, LBE_MAGIC, 4) != 0) return NULL;
    size_t pos = 4;
    return read_node(buf, n, &pos, a);
}
