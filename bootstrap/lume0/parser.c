/* Recursive-descent parser for Lume (spec §2–§9), with a Pratt expression parser.
 * Scope: the Stage-0/1 surface subset. Reserved later-stage syntax parses where unambiguous
 * and is left for the checker to flag. Errors are P-coded diagnostics with recovery to NEWLINE/DEDENT. */
#include "ast.h"
#include <stdio.h>
#include <string.h>
#include <stdbool.h>

typedef struct {
    const Token *t;
    size_t       n, i;
    Arena       *a;
    DiagList    *d;
    int          errors_before;
} P;

/* ---------- token access ---------- */
static const Token *cur(P *p)  { return &p->t[p->i]; }
static TokenKind kind(P *p)    { return p->t[p->i].kind; }
static TokenKind peek(P *p, size_t o) { return p->i + o < p->n ? p->t[p->i + o].kind : TK_EOF; }
static bool at(P *p, TokenKind k) { return kind(p) == k; }
static const Token *advance(P *p) { const Token *t = &p->t[p->i]; if (p->i + 1 < p->n) p->i++; return t; }
static bool accept(P *p, TokenKind k) { if (at(p, k)) { advance(p); return true; } return false; }

static void perr(P *p, const char *code, const char *msg, const char *help) {
    const Token *t = cur(p);
    diag_add(p->d, SEV_ERROR, code, t->line, t->col, t->len ? t->len : 1, msg, help);
}

/* recover: skip tokens until a NEWLINE/DEDENT/EOF so the next statement can start cleanly */
static void sync(P *p) {
    int depth = 0;
    while (!at(p, TK_EOF)) {
        TokenKind k = kind(p);
        if (depth == 0 && (k == TK_NEWLINE || k == TK_DEDENT)) return;
        if (k == TK_INDENT) depth++;
        else if (k == TK_DEDENT && depth > 0) depth--;
        advance(p);
    }
}

static bool expect(P *p, TokenKind k, const char *code, const char *msg, const char *help) {
    if (at(p, k)) { advance(p); return true; }
    perr(p, code, msg, help);
    return false;
}

static void skip_newlines(P *p) { while (at(p, TK_NEWLINE)) advance(p); }

static Node *mk(P *p, NodeKind nk) { const Token *t = cur(p); return node_new(p->a, nk, t->line, t->col); }
static Node *mk_at(P *p, NodeKind nk, const Token *t) { return node_new(p->a, nk, t->line, t->col); }

/* ---------- forward decls ---------- */
static Node *parse_type(P *p);
static Node *parse_expr(P *p, int minbp);
static Node *parse_stmt(P *p);
static Node *parse_block(P *p);
static Node *parse_pattern(P *p);
static Node *parse_if(P *p);
static Node *parse_match(P *p);
static Node *parse_fn(P *p, Node *doc, Node *attrs, int flags);

/* ---------- helpers ---------- */
static Node *ident_node(P *p, NodeKind nk) {
    Node *n = mk(p, nk);
    if (at(p, TK_IDENT) || at(p, TK_KW_SELF) || at(p, TK_KW_SELF_TYPE)) {
        const Token *t = advance(p);
        node_set_text(n, t->start, t->len);
    } else {
        perr(p, "P0001", "expected an identifier", NULL);
    }
    return n;
}

static Node *parse_doc(P *p) {
    if (!at(p, TK_DOC_COMMENT)) return NULL;
    Node *d = mk(p, N_DOC);
    while (at(p, TK_DOC_COMMENT)) {
        const Token *t = advance(p);
        Node *line = node_new(p->a, N_DOC, t->line, t->col);
        node_set_text(line, t->start, t->len);
        node_add(p->a, d, line);
        skip_newlines(p);
    }
    return d;
}

static Node *parse_attrs(P *p) {
    if (!at(p, TK_AT)) return NULL;
    Node *list = mk(p, N_ATTR);
    while (at(p, TK_AT)) {
        advance(p);
        Node *a = mk(p, N_ATTR);
        if (at(p, TK_IDENT)) { const Token *t = advance(p); node_set_text(a, t->start, t->len); }
        if (accept(p, TK_LPAREN)) {
            while (!at(p, TK_RPAREN) && !at(p, TK_EOF)) {
                node_add(p->a, a, parse_expr(p, 0));
                if (!accept(p, TK_COMMA)) break;
            }
            expect(p, TK_RPAREN, "P0002", "expected ) to close attribute arguments", NULL);
        }
        node_add(p->a, list, a);
        skip_newlines(p);
    }
    return list;
}

/* ---------- types ---------- */
static Node *parse_effects(P *p) {
    Node *e = mk(p, N_EFFECTS);
    expect(p, TK_LBRACE, "P0010", "expected { to open an effect row", "effects look like / {io, alloc}");
    while (!at(p, TK_RBRACE) && !at(p, TK_EOF)) {
        if (at(p, TK_IDENT)) { const Token *t = advance(p); Node *x = node_new(p->a, N_IDENT, t->line, t->col); node_set_text(x, t->start, t->len); node_add(p->a, e, x); }
        else { perr(p, "P0011", "expected an effect name", NULL); break; }
        if (!accept(p, TK_COMMA)) break;
    }
    expect(p, TK_RBRACE, "P0012", "expected } to close the effect row", NULL);
    return e;
}

static Node *parse_type_atom(P *p) {
    TokenKind k = kind(p);
    if (k == TK_KW_OWN || k == TK_KW_MUT || k == TK_AMP) {
        advance(p);
        Node *n = mk(p, k == TK_KW_OWN ? N_TYOWN : k == TK_KW_MUT ? N_TYMUT : N_TYREF);
        node_add(p->a, n, parse_type_atom(p));
        return n;
    }
    if (k == TK_KW_DYN || k == TK_KW_IMPL) {
        advance(p);
        Node *n = mk(p, k == TK_KW_DYN ? N_TYDYN : N_TYIMPL);
        node_add(p->a, n, parse_type_atom(p));
        return n;
    }
    if (k == TK_KW_SELF_TYPE) { advance(p); return mk_at(p, N_TYSELF, &p->t[p->i-1]); }
    if (k == TK_IDENT && cur(p)->len == 5 && memcmp(cur(p)->start, "never", 5) == 0) { /* not a kw */ }
    if (k == TK_KW_FN) {
        Node *n = mk(p, N_TYFN); advance(p);
        expect(p, TK_LPAREN, "P0013", "expected ( in function type", NULL);
        while (!at(p, TK_RPAREN) && !at(p, TK_EOF)) { node_add(p->a, n, parse_type(p)); if (!accept(p, TK_COMMA)) break; }
        expect(p, TK_RPAREN, "P0014", "expected ) in function type", NULL);
        if (accept(p, TK_ARROW)) node_add(p->a, n, parse_type(p));
        if (at(p, TK_SLASH)) { advance(p); node_add(p->a, n, parse_effects(p)); }
        return n;
    }
    if (k == TK_LPAREN) {
        Node *n = mk(p, N_TYTUPLE); advance(p);
        while (!at(p, TK_RPAREN) && !at(p, TK_EOF)) { node_add(p->a, n, parse_type(p)); if (!accept(p, TK_COMMA)) break; }
        expect(p, TK_RPAREN, "P0015", "expected ) to close a tuple type", NULL);
        return n;
    }
    if (k == TK_LBRACKET) {
        Node *n = mk(p, N_TYARRAY); advance(p);
        node_add(p->a, n, parse_type(p));
        if (accept(p, TK_SEMI)) node_add(p->a, n, parse_expr(p, 0));
        expect(p, TK_RBRACKET, "P0016", "expected ] to close an array/list type", NULL);
        return n;
    }
    if (k == TK_IDENT || k == TK_KW_SELF) {
        Node *n = mk(p, N_TYNAME);
        const Token *t = advance(p);
        node_set_text(n, t->start, t->len);
        while (at(p, TK_DOT) && peek(p, 1) == TK_IDENT) { advance(p); advance(p); } /* path: keep head text */
        if (accept(p, TK_LBRACKET)) {
            while (!at(p, TK_RBRACKET) && !at(p, TK_EOF)) {
                node_add(p->a, n, parse_type(p));
                if (!accept(p, TK_COMMA)) break;
            }
            expect(p, TK_RBRACKET, "P0017", "expected ] to close type arguments", NULL);
        } else if (accept(p, TK_LPAREN)) {
            /* parenthesised args: decimal(12, 2), money(BRL), tensor(f32, n) */
            while (!at(p, TK_RPAREN) && !at(p, TK_EOF)) {
                node_add(p->a, n, parse_expr(p, 0));
                if (!accept(p, TK_COMMA)) break;
            }
            expect(p, TK_RPAREN, "P0017", "expected ) to close type arguments", NULL);
        }
        return n;
    }
    if (k == TK_KW_SELF_TYPE) { advance(p); return mk(p, N_TYSELF); }
    perr(p, "P0018", "expected a type", NULL);
    Node *bad = mk(p, N_ERROR_NODE); advance(p); return bad;
}

static Node *parse_type(P *p) {
    Node *t = parse_type_atom(p);
    for (;;) {
        if (at(p, TK_QUESTION)) { advance(p); Node *o = node_new(p->a, N_TYOPT, t->line, t->col); node_add(p->a, o, t); t = o; }
        else if (at(p, TK_LBRACKET) && peek(p, 1) == TK_RBRACKET) { advance(p); advance(p); Node *s = node_new(p->a, N_TYSLICE, t->line, t->col); node_add(p->a, s, t); t = s; }
        else if (at(p, TK_BANG)) { advance(p); Node *e = node_new(p->a, N_TYERR, t->line, t->col); node_add(p->a, e, t); if (!at(p,TK_NEWLINE)&&!at(p,TK_SLASH)&&!at(p,TK_EQ)&&!at(p,TK_LBRACE)&&!at(p,TK_KW_FROM)&&!at(p,TK_COMMA)&&!at(p,TK_RPAREN)) node_add(p->a, e, parse_type(p)); t = e; }
        else if (at(p, TK_KW_IN)) { advance(p); Node *r = node_new(p->a, N_TYRANGE, t->line, t->col); node_add(p->a, r, t); node_add(p->a, r, parse_expr(p, 0)); t = r; }
        else if (at(p, TK_KW_WHERE)) { advance(p); Node *w = node_new(p->a, N_TYWHERE, t->line, t->col); node_add(p->a, w, t); node_add(p->a, w, parse_expr(p, 0)); t = w; }
        else break;
    }
    return t;
}

/* ---------- generics ---------- */
static Node *parse_generics(P *p) {
    Node *g = mk(p, N_GENERICS);
    expect(p, TK_LBRACKET, "P0020", "expected [ to open generic parameters", NULL);
    while (!at(p, TK_RBRACKET) && !at(p, TK_EOF)) {
        Node *one = mk(p, N_GENERIC);
        Node *name = ident_node(p, N_IDENT);
        node_add(p->a, one, name);
        if (accept(p, TK_COLON)) {
            node_add(p->a, one, parse_type(p));
            while (accept(p, TK_PLUS)) node_add(p->a, one, parse_type(p));
        }
        node_add(p->a, g, one);
        if (!accept(p, TK_COMMA)) break;
    }
    expect(p, TK_RBRACKET, "P0021", "expected ] to close generic parameters", NULL);
    return g;
}

/* ---------- blocks ---------- */
static Node *parse_block(P *p) {
    Node *b = mk(p, N_BLOCK);
    if (accept(p, TK_LBRACE)) {                      /* explicit-brace form */
        skip_newlines(p);
        while (!at(p, TK_RBRACE) && !at(p, TK_EOF)) {
            node_add(p->a, b, parse_stmt(p));
            while (at(p, TK_SEMI) || at(p, TK_NEWLINE)) advance(p);
        }
        expect(p, TK_RBRACE, "P0030", "expected } to close block", NULL);
        return b;
    }
    if (!expect(p, TK_NEWLINE, "P0031", "expected a new line then an indented block", NULL)) return b;
    skip_newlines(p);
    if (!expect(p, TK_INDENT, "P0032", "expected an indented block", "indent the body under its header")) return b;
    while (!at(p, TK_DEDENT) && !at(p, TK_EOF)) {
        skip_newlines(p);
        if (at(p, TK_DEDENT) || at(p, TK_EOF)) break;
        node_add(p->a, b, parse_stmt(p));
    }
    expect(p, TK_DEDENT, "P0033", "expected the block to end (dedent)", NULL);
    return b;
}

/* ---------- parameters ---------- */
static void parse_params(P *p, Node *fn) {
    expect(p, TK_LPAREN, "P0040", "expected ( to open parameters", NULL);
    while (!at(p, TK_RPAREN) && !at(p, TK_EOF)) {
        Node *par = mk(p, N_PARAM);
        if (at(p, TK_KW_OWN)) { advance(p); par->flags |= F_OWN; }
        else if (at(p, TK_KW_MUT)) { advance(p); par->flags |= F_MUT; }
        if (at(p, TK_KW_SELF)) { const Token *t = advance(p); node_set_text(par, t->start, t->len); }
        else {
            Node *name = ident_node(p, N_IDENT);
            node_add(p->a, par, name);
            expect(p, TK_COLON, "P0041", "expected : and a type for the parameter", NULL);
            if (accept(p, TK_ELLIPSIS)) par->flags |= F_REF; /* variadic marker (reuse flag) */
            node_add(p->a, par, parse_type(p));
            if (accept(p, TK_EQ)) node_add(p->a, par, parse_expr(p, 0));
        }
        node_add(p->a, fn, par);
        if (!accept(p, TK_COMMA)) break;
    }
    expect(p, TK_RPAREN, "P0042", "expected ) to close parameters", NULL);
}

/* ---------- functions ---------- */
static Node *parse_fn(P *p, Node *doc, Node *attrs, int flags) {
    Node *fn = mk(p, N_FN);
    fn->flags = flags;
    advance(p); /* fn */
    if (doc) node_add(p->a, fn, doc);
    if (attrs) node_add(p->a, fn, attrs);
    Node *name = ident_node(p, N_IDENT);
    node_add(p->a, fn, name);
    if (at(p, TK_LBRACKET)) node_add(p->a, fn, parse_generics(p));
    parse_params(p, fn);
    if (accept(p, TK_ARROW)) {
        node_add(p->a, fn, parse_type(p));     /* parse_type handles trailing ! E */
    } else if (at(p, TK_BANG)) {               /* `fn f(..) ! E`: unit value, error set E */
        advance(p);
        Node *e = mk(p, N_TYERR);
        if (!at(p, TK_NEWLINE) && !at(p, TK_SLASH) && !at(p, TK_EQ) && !at(p, TK_KW_FROM))
            node_add(p->a, e, parse_type(p));
        node_add(p->a, fn, e);
    }
    if (at(p, TK_SLASH)) { advance(p); node_add(p->a, fn, parse_effects(p)); }
    if (at(p, TK_KW_FROM)) { advance(p); Node *f = mk(p, N_FROM); node_add(p->a, f, ident_node(p, N_IDENT)); node_add(p->a, fn, f); }

    if (accept(p, TK_EQ)) {                      /* expression body */
        node_add(p->a, fn, parse_expr(p, 0));
        accept(p, TK_NEWLINE);
        return fn;
    }
    if (at(p, TK_NEWLINE) && (peek(p, 1) == TK_INDENT || peek(p, 1) == TK_NEWLINE))
        node_add(p->a, fn, parse_block(p));      /* block body (contracts parsed as statements) */
    else
        accept(p, TK_NEWLINE);                   /* signature only (trait method) */
    return fn;
}

/* ---------- type declarations ---------- */
static void parse_one_variant(P *p, Node *parent) {
    Node *v = mk(p, N_VARIANT);
    node_add(p->a, v, ident_node(p, N_IDENT));
    if (accept(p, TK_LPAREN)) {
        while (!at(p, TK_RPAREN) && !at(p, TK_EOF)) {
            Node *f = mk(p, N_FIELD);
            node_add(p->a, f, ident_node(p, N_IDENT));
            expect(p, TK_COLON, "P0051", "expected : and a type in variant payload", NULL);
            node_add(p->a, f, parse_type(p));
            node_add(p->a, v, f);
            if (!accept(p, TK_COMMA)) break;
        }
        expect(p, TK_RPAREN, "P0052", "expected ) to close variant payload", NULL);
    }
    node_add(p->a, parent, v);
}

static void parse_variant_list(P *p, Node *parent) {
    if (at(p, TK_NEWLINE)) {                       /* indented: one variant per line */
        advance(p); skip_newlines(p);
        if (!expect(p, TK_INDENT, "P0050", "expected indented variants", NULL)) return;
        while (!at(p, TK_DEDENT) && !at(p, TK_EOF)) {
            skip_newlines(p);
            if (at(p, TK_DEDENT)) break;
            parse_one_variant(p, parent);
            accept(p, TK_PIPE);
            skip_newlines(p);
        }
        expect(p, TK_DEDENT, "P0053", "expected variants to end (dedent)", NULL);
        return;
    }
    do { parse_one_variant(p, parent); } while (accept(p, TK_PIPE));   /* inline: A | B(..) | C */
    accept(p, TK_NEWLINE);
}

static Node *parse_record_body(P *p, Node *rec) {
    if (accept(p, TK_LBRACE)) {                   /* inline record { a: T, b: U } */
        while (!at(p, TK_RBRACE) && !at(p, TK_EOF)) {
            Node *f = mk(p, N_FIELD);
            node_add(p->a, f, ident_node(p, N_IDENT));
            expect(p, TK_COLON, "P0060", "expected : and a type for the field", NULL);
            node_add(p->a, f, parse_type(p));
            node_add(p->a, rec, f);
            if (!accept(p, TK_COMMA)) break;
        }
        expect(p, TK_RBRACE, "P0061", "expected } to close record", NULL);
        accept(p, TK_NEWLINE);
        return rec;
    }
    expect(p, TK_NEWLINE, "P0062", "expected fields on the following indented lines", NULL);
    skip_newlines(p);
    if (!expect(p, TK_INDENT, "P0063", "expected indented record fields", NULL)) return rec;
    while (!at(p, TK_DEDENT) && !at(p, TK_EOF)) {
        skip_newlines(p);
        if (at(p, TK_DEDENT)) break;
        if (at(p, TK_KW_INVARIANT)) {
            advance(p); Node *inv = mk(p, N_INVARIANT); node_add(p->a, inv, parse_expr(p, 0));
            node_add(p->a, rec, inv); accept(p, TK_NEWLINE); continue;
        }
        Node *f = mk(p, N_FIELD);
        int fl = 0;
        if (at(p, TK_KW_PUB)) { advance(p); fl |= F_PUB; }
        f->flags = fl;
        node_add(p->a, f, ident_node(p, N_IDENT));
        expect(p, TK_COLON, "P0064", "expected : and a type for the field", NULL);
        node_add(p->a, f, parse_type(p));
        if (accept(p, TK_EQ)) node_add(p->a, f, parse_expr(p, 0));
        node_add(p->a, rec, f);
        accept(p, TK_NEWLINE);
    }
    expect(p, TK_DEDENT, "P0065", "expected record to end (dedent)", NULL);
    return rec;
}

static Node *parse_typedecl(P *p, Node *doc, Node *attrs, int flags) {
    Node *td = mk(p, N_TYPEDECL);
    td->flags = flags;
    advance(p); /* type */
    if (doc) node_add(p->a, td, doc);
    if (attrs) node_add(p->a, td, attrs);
    node_add(p->a, td, ident_node(p, N_IDENT));
    if (at(p, TK_LBRACKET)) node_add(p->a, td, parse_generics(p));
    expect(p, TK_EQ, "P0070", "expected = in a type declaration", NULL);
    if (at(p, TK_KW_RECORD)) { advance(p); Node *r = mk(p, N_RECORD); parse_record_body(p, r); node_add(p->a, td, r); }
    else if (at(p, TK_KW_ENUM)) { advance(p); Node *e = mk(p, N_ENUM); parse_variant_list(p, e); node_add(p->a, td, e); }
    else if (at(p, TK_KW_ERROR)) { advance(p); Node *e = mk(p, N_ERRORDEF); parse_variant_list(p, e); node_add(p->a, td, e); }
    else if (at(p, TK_KW_DISTINCT)) { advance(p); Node *d = mk(p, N_DISTINCT); node_add(p->a, d, parse_type(p)); node_add(p->a, td, d); accept(p, TK_NEWLINE); }
    else { node_add(p->a, td, parse_type(p)); accept(p, TK_NEWLINE); }
    return td;
}

/* ---------- trait / impl ---------- */
static void parse_assoc_items(P *p, Node *parent) {
    expect(p, TK_NEWLINE, "P0080", "expected an indented body", NULL);
    skip_newlines(p);
    if (!expect(p, TK_INDENT, "P0081", "expected an indented body", NULL)) return;
    while (!at(p, TK_DEDENT) && !at(p, TK_EOF)) {
        skip_newlines(p);
        if (at(p, TK_DEDENT)) break;
        Node *doc = parse_doc(p);
        Node *attrs = parse_attrs(p);
        int fl = 0; if (at(p, TK_KW_PUB)) { advance(p); fl |= F_PUB; }
        if (at(p, TK_KW_FN)) node_add(p->a, parent, parse_fn(p, doc, attrs, fl));
        else if (at(p, TK_KW_TYPE)) {
            advance(p); Node *td = mk(p, N_TYPEDECL); node_add(p->a, td, ident_node(p, N_IDENT));
            if (accept(p, TK_EQ)) node_add(p->a, td, parse_type(p));
            node_add(p->a, parent, td); accept(p, TK_NEWLINE);
        } else if (at(p, TK_KW_CONST)) {
            advance(p); Node *c = mk(p, N_CONST); node_add(p->a, c, ident_node(p, N_IDENT));
            if (accept(p, TK_COLON)) node_add(p->a, c, parse_type(p));
            if (accept(p, TK_EQ)) node_add(p->a, c, parse_expr(p, 0));
            node_add(p->a, parent, c); accept(p, TK_NEWLINE);
        } else { perr(p, "P0082", "expected a method, associated type or const", NULL); sync(p); }
    }
    expect(p, TK_DEDENT, "P0083", "expected the body to end (dedent)", NULL);
}

static Node *parse_trait(P *p, Node *doc, Node *attrs, int flags) {
    Node *tr = mk(p, N_TRAIT); tr->flags = flags; advance(p);
    if (doc) node_add(p->a, tr, doc);
    if (attrs) node_add(p->a, tr, attrs);
    node_add(p->a, tr, ident_node(p, N_IDENT));
    if (at(p, TK_LBRACKET)) node_add(p->a, tr, parse_generics(p));
    if (accept(p, TK_COLON)) { node_add(p->a, tr, parse_type(p)); while (accept(p, TK_PLUS)) node_add(p->a, tr, parse_type(p)); }
    parse_assoc_items(p, tr);
    return tr;
}

static Node *parse_impl(P *p, Node *doc, Node *attrs, int flags) {
    Node *im = mk(p, N_IMPL); im->flags = flags; advance(p);
    if (doc) node_add(p->a, im, doc);
    if (attrs) node_add(p->a, im, attrs);
    if (at(p, TK_LBRACKET)) node_add(p->a, im, parse_generics(p));
    node_add(p->a, im, parse_type(p));
    if (accept(p, TK_KW_FOR)) node_add(p->a, im, parse_type(p));
    if (at(p, TK_KW_WHERE)) { advance(p); do { node_add(p->a, im, ident_node(p, N_IDENT)); expect(p, TK_COLON, "P0090", "expected : in where clause", NULL); node_add(p->a, im, parse_type(p)); } while (accept(p, TK_COMMA)); }
    parse_assoc_items(p, im);
    return im;
}

static Node *parse_use(P *p) {
    Node *u = mk(p, N_USE); advance(p);
    node_add(p->a, u, ident_node(p, N_IDENT));
    while (at(p, TK_DOT)) {
        advance(p);
        if (accept(p, TK_STAR)) { Node *s = mk(p, N_IDENT); node_set_text(s, "*", 1); node_add(p->a, u, s); break; }
        if (at(p, TK_LPAREN)) {
            advance(p);
            while (!at(p, TK_RPAREN) && !at(p, TK_EOF)) { node_add(p->a, u, ident_node(p, N_IDENT)); if (!accept(p, TK_COMMA)) break; }
            expect(p, TK_RPAREN, "P0100", "expected ) to close selective import", NULL);
            break;
        }
        node_add(p->a, u, ident_node(p, N_IDENT));
    }
    if (accept(p, TK_KW_AS)) node_add(p->a, u, ident_node(p, N_IDENT));
    accept(p, TK_NEWLINE);
    return u;
}

static Node *parse_const(P *p, int flags) {
    Node *c = mk(p, N_CONST); c->flags = flags; advance(p);
    node_add(p->a, c, ident_node(p, N_IDENT));
    if (accept(p, TK_COLON)) node_add(p->a, c, parse_type(p));
    expect(p, TK_EQ, "P0110", "expected = in const declaration", NULL);
    node_add(p->a, c, parse_expr(p, 0));
    accept(p, TK_NEWLINE);
    return c;
}

/* ---------- item dispatch ---------- */
static Node *parse_item_or_stmt(P *p) {
    Node *doc = parse_doc(p);
    Node *attrs = parse_attrs(p);
    int flags = 0;
    if (at(p, TK_KW_PUB)) { advance(p); flags |= F_PUB; if (accept(p, TK_LPAREN)) { accept(p, TK_IDENT); accept(p, TK_RPAREN); flags |= F_CRATE; } }
    switch (kind(p)) {
    case TK_KW_FN:    return parse_fn(p, doc, attrs, flags);
    case TK_KW_TYPE:  return parse_typedecl(p, doc, attrs, flags);
    case TK_KW_TRAIT: return parse_trait(p, doc, attrs, flags);
    case TK_KW_IMPL:  return parse_impl(p, doc, attrs, flags);
    case TK_KW_CONST: return parse_const(p, flags);
    case TK_KW_USE:   return parse_use(p);
    default:
        if (flags || doc || attrs) {
            /* `pub`/doc/attrs on something we didn't recognise as an item */
            Node *s = parse_stmt(p);
            return s;
        }
        return parse_stmt(p);
    }
}

/* ======================================================================
 *  Statements
 * ====================================================================== */

static bool is_assign_op(TokenKind k) {
    switch (k) {
    case TK_EQ: case TK_PLUS_EQ: case TK_MINUS_EQ: case TK_STAR_EQ: case TK_SLASH_EQ:
    case TK_PERCENT_EQ: case TK_POW_EQ: case TK_AMP_EQ: case TK_PIPE_EQ: case TK_CARET_EQ:
    case TK_SHL_EQ: case TK_SHR_EQ: return true;
    default: return false;
    }
}

static Node *parse_label_opt(P *p) {
    if (at(p, TK_LABEL) && peek(p, 1) == TK_COLON) {
        Node *l = mk(p, N_IDENT);
        const Token *t = advance(p); node_set_text(l, t->start, t->len);
        advance(p); /* : */
        return l;
    }
    return NULL;
}

static Node *parse_stmt(P *p) {
    TokenKind k = kind(p);

    /* items are also valid as statements (nested fn/type/const, use, pub …) */
    if (k == TK_KW_FN || k == TK_KW_TYPE || k == TK_KW_TRAIT || k == TK_KW_IMPL ||
        k == TK_KW_CONST || k == TK_KW_USE || k == TK_DOC_COMMENT || k == TK_AT || k == TK_KW_PUB)
        return parse_item_or_stmt(p);

    switch (k) {
    case TK_KW_LET: case TK_KW_VAR: {
        Node *n = mk(p, k == TK_KW_LET ? N_LET : N_VAR); advance(p);
        node_add(p->a, n, parse_pattern(p));
        if (accept(p, TK_COLON)) node_add(p->a, n, parse_type(p));
        if (accept(p, TK_EQ)) node_add(p->a, n, parse_expr(p, 0));
        accept(p, TK_NEWLINE);
        return n;
    }
    case TK_KW_REQUIRES: case TK_KW_ENSURES: {
        Node *n = mk(p, k == TK_KW_REQUIRES ? N_REQUIRES : N_ENSURES); advance(p);
        node_add(p->a, n, parse_expr(p, 0));
        if (accept(p, TK_KW_OR)) { expect(p, TK_KW_FAIL, "P0120", "expected `fail` after `or` in a contract", NULL); node_add(p->a, n, parse_expr(p, 0)); }
        accept(p, TK_NEWLINE);
        return n;
    }
    case TK_KW_INVARIANT: { Node *n = mk(p, N_INVARIANT); advance(p); node_add(p->a, n, parse_expr(p, 0)); accept(p, TK_NEWLINE); return n; }
    case TK_KW_IF:    return parse_if(p);
    case TK_KW_MATCH: return parse_match(p);
    case TK_KW_WHILE: {
        Node *l = parse_label_opt(p);
        Node *n = mk(p, N_WHILE); advance(p);
        if (l) node_add(p->a, n, l);
        node_add(p->a, n, parse_expr(p, 0));
        node_add(p->a, n, parse_block(p));
        if (accept(p, TK_KW_ELSE)) node_add(p->a, n, parse_block(p));
        return n;
    }
    case TK_KW_FOR: {
        Node *l = parse_label_opt(p);
        Node *n = mk(p, N_FOR); advance(p);
        if (l) node_add(p->a, n, l);
        if (accept(p, TK_KW_OWN)) n->flags |= F_OWN;
        node_add(p->a, n, parse_pattern(p));
        expect(p, TK_KW_IN, "P0121", "expected `in` in a for loop", NULL);
        if (at(p, TK_IDENT) && cur(p)->len == 3 && memcmp(cur(p)->start, "par", 3) == 0) { advance(p); n->flags |= F_PAR; } /* `par` is contextual */
        node_add(p->a, n, parse_expr(p, 0));
        node_add(p->a, n, parse_block(p));
        if (accept(p, TK_KW_ELSE)) node_add(p->a, n, parse_block(p));
        return n;
    }
    case TK_KW_LOOP: { Node *l = parse_label_opt(p); Node *n = mk(p, N_LOOP); advance(p); if (l) node_add(p->a, n, l); node_add(p->a, n, parse_block(p)); return n; }
    case TK_KW_BREAK: {
        Node *n = mk(p, N_BREAK); advance(p);
        if (at(p, TK_LABEL)) { Node *lb = mk(p, N_IDENT); const Token *t = advance(p); node_set_text(lb, t->start, t->len); node_add(p->a, n, lb); }
        if (!at(p, TK_NEWLINE) && !at(p, TK_DEDENT) && !at(p, TK_EOF)) node_add(p->a, n, parse_expr(p, 0));
        accept(p, TK_NEWLINE); return n;
    }
    case TK_KW_CONTINUE: { Node *n = mk(p, N_CONTINUE); advance(p); if (at(p, TK_LABEL)) { Node *lb = mk(p, N_IDENT); const Token *t = advance(p); node_set_text(lb, t->start, t->len); node_add(p->a, n, lb); } accept(p, TK_NEWLINE); return n; }
    case TK_KW_RETURN: { Node *n = mk(p, N_RETURN); advance(p); if (!at(p, TK_NEWLINE) && !at(p, TK_DEDENT) && !at(p, TK_EOF)) node_add(p->a, n, parse_expr(p, 0)); accept(p, TK_NEWLINE); return n; }
    case TK_KW_DEFER: { Node *n = mk(p, N_DEFER); advance(p); node_add(p->a, n, parse_expr(p, 0)); accept(p, TK_NEWLINE); return n; }
    case TK_KW_ERRDEFER: { Node *n = mk(p, N_ERRDEFER); advance(p); node_add(p->a, n, parse_expr(p, 0)); accept(p, TK_NEWLINE); return n; }
    case TK_KW_FAIL: { Node *n = mk(p, N_FAIL); advance(p); node_add(p->a, n, parse_expr(p, 0)); accept(p, TK_NEWLINE); return n; }
    case TK_KW_PANIC: { Node *n = mk(p, N_PANIC); advance(p); if (!at(p, TK_NEWLINE) && !at(p, TK_DEDENT) && !at(p, TK_EOF)) node_add(p->a, n, parse_expr(p, 0)); accept(p, TK_NEWLINE); return n; }
    case TK_KW_ASSERT: { Node *n = mk(p, N_ASSERT); advance(p); node_add(p->a, n, parse_expr(p, 0)); if (accept(p, TK_COMMA)) node_add(p->a, n, parse_expr(p, 0)); accept(p, TK_NEWLINE); return n; }
    case TK_KW_USING: { Node *n = mk(p, N_USING); advance(p); node_add(p->a, n, ident_node(p, N_IDENT)); expect(p, TK_EQ, "P0122", "expected = in `using`", NULL); node_add(p->a, n, parse_expr(p, 0)); node_add(p->a, n, parse_block(p)); return n; }
    case TK_KW_UNSAFE:   { Node *n = mk(p, N_UNSAFE); advance(p); node_add(p->a, n, parse_block(p)); return n; }
    case TK_KW_COMPTIME: { Node *n = mk(p, N_COMPTIME); advance(p); node_add(p->a, n, parse_block(p)); return n; }
    case TK_KW_SCOPE: { Node *n = mk(p, N_SCOPE); advance(p); if (at(p, TK_IDENT)) node_add(p->a, n, ident_node(p, N_IDENT)); node_add(p->a, n, parse_block(p)); return n; }
    case TK_KW_WITH: { Node *n = mk(p, N_WITH); advance(p); node_add(p->a, n, ident_node(p, N_IDENT)); expect(p, TK_EQ, "P0123", "expected = in `with`", NULL); node_add(p->a, n, parse_expr(p, 0)); node_add(p->a, n, parse_block(p)); return n; }
    default: break;
    }

    /* expression statement or assignment */
    Node *e = parse_expr(p, 0);
    if (is_assign_op(kind(p))) {
        Node *a = mk(p, N_ASSIGN);
        const Token *op = advance(p);
        node_set_text(a, op->start, op->len);
        node_add(p->a, a, e);
        node_add(p->a, a, parse_expr(p, 0));
        accept(p, TK_NEWLINE);
        return a;
    }
    Node *s = node_new(p->a, N_EXPRSTMT, e->line, e->col);
    node_add(p->a, s, e);
    accept(p, TK_NEWLINE);
    return s;
}

/* ======================================================================
 *  if / match
 * ====================================================================== */

static Node *parse_if(P *p) {
    const Token *kw = advance(p); /* if */
    Node *cond = parse_expr(p, 0);
    if (accept(p, TK_KW_THEN)) {                     /* expression form */
        Node *n = mk_at(p, N_IFEXPR, kw);
        node_add(p->a, n, cond);
        node_add(p->a, n, parse_expr(p, 0));
        expect(p, TK_KW_ELSE, "P0130", "`if … then …` must have an `else`", "every then-branch needs an else so the if has a value");
        node_add(p->a, n, parse_expr(p, 0));
        return n;
    }
    Node *n = mk_at(p, N_IF, kw);
    node_add(p->a, n, cond);
    node_add(p->a, n, parse_block(p));
    while (at(p, TK_KW_ELIF)) {
        advance(p);
        node_add(p->a, n, parse_expr(p, 0));
        node_add(p->a, n, parse_block(p));
    }
    if (accept(p, TK_KW_ELSE)) node_add(p->a, n, parse_block(p));
    return n;
}

static Node *parse_match(P *p) {
    Node *n = mk(p, N_MATCH); advance(p);
    node_add(p->a, n, parse_expr(p, 0));
    expect(p, TK_NEWLINE, "P0140", "expected match arms on indented lines", NULL);
    skip_newlines(p);
    if (!expect(p, TK_INDENT, "P0141", "expected indented match arms", NULL)) return n;
    while (!at(p, TK_DEDENT) && !at(p, TK_EOF)) {
        skip_newlines(p);
        if (at(p, TK_DEDENT)) break;
        Node *arm = mk(p, N_MATCHARM);
        Node *pat = parse_pattern(p);
        if (at(p, TK_PIPE)) { Node *alt = node_new(p->a, N_POR, pat->line, pat->col); node_add(p->a, alt, pat); while (accept(p, TK_PIPE)) node_add(p->a, alt, parse_pattern(p)); pat = alt; }
        node_add(p->a, arm, pat);
        if (accept(p, TK_KW_IF)) { Node *g = node_new(p->a, N_MATCHARM, cur(p)->line, cur(p)->col); node_add(p->a, g, parse_expr(p, 0)); node_add(p->a, arm, g); }
        expect(p, TK_FAT_ARROW, "P0142", "expected => after a match pattern", NULL);
        if (at(p, TK_NEWLINE)) node_add(p->a, arm, parse_block(p));
        else { node_add(p->a, arm, parse_expr(p, 0)); accept(p, TK_NEWLINE); }
        node_add(p->a, n, arm);
        skip_newlines(p);
    }
    expect(p, TK_DEDENT, "P0143", "expected match to end (dedent)", NULL);
    return n;
}

/* ======================================================================
 *  Patterns
 * ====================================================================== */

static bool starts_lower(const Token *t) { return t->len > 0 && (t->start[0] >= 'a' && t->start[0] <= 'z'); }

static Node *parse_pattern(P *p) {
    TokenKind k = kind(p);
    Node *pat = NULL;
    if (k == TK_IDENT && cur(p)->len == 1 && cur(p)->start[0] == '_') { pat = mk(p, N_PWILD); advance(p); }
    else if (k == TK_INT || k == TK_FLOAT || k == TK_STRING || k == TK_CHAR || k == TK_KW_TRUE || k == TK_KW_FALSE) {
        Node *lit = mk(p, N_PLIT); const Token *t = advance(p); node_set_text(lit, t->start, t->len);
        if (at(p, TK_DOTDOT) || at(p, TK_DOTDOT_EQ)) {
            Node *r = node_new(p->a, N_PRANGE, lit->line, lit->col);
            if (at(p, TK_DOTDOT_EQ)) r->flags |= F_INCLUSIVE;
            advance(p);
            node_add(p->a, r, lit);
            Node *hi = mk(p, N_PLIT); const Token *t2 = advance(p); node_set_text(hi, t2->start, t2->len);
            node_add(p->a, r, hi); pat = r;
        } else pat = lit;
    }
    else if (k == TK_KW_NONE) { pat = mk(p, N_PNONE); advance(p); }
    else if (k == TK_KW_IS) { advance(p); pat = mk(p, N_PIS); node_add(p->a, pat, parse_type(p)); }
    else if (k == TK_DOTDOT) { pat = mk(p, N_PREST); advance(p); if (at(p, TK_IDENT)) node_add(p->a, pat, ident_node(p, N_IDENT)); }
    else if (k == TK_LPAREN) {
        pat = mk(p, N_PTUPLE); advance(p);
        while (!at(p, TK_RPAREN) && !at(p, TK_EOF)) { node_add(p->a, pat, parse_pattern(p)); if (!accept(p, TK_COMMA)) break; }
        expect(p, TK_RPAREN, "P0150", "expected ) to close a tuple pattern", NULL);
    }
    else if (k == TK_LBRACKET) {
        pat = mk(p, N_PLIST); advance(p);
        while (!at(p, TK_RBRACKET) && !at(p, TK_EOF)) { node_add(p->a, pat, parse_pattern(p)); if (!accept(p, TK_COMMA)) break; }
        expect(p, TK_RBRACKET, "P0151", "expected ] to close a list pattern", NULL);
    }
    else if (k == TK_KW_OWN || k == TK_KW_MUT) {
        int fl = (k == TK_KW_OWN) ? F_OWN : F_MUT; advance(p);
        pat = mk(p, N_PBIND); pat->flags = fl; node_add(p->a, pat, ident_node(p, N_IDENT));
    }
    else if (k == TK_IDENT) {
        const Token *name = advance(p);
        /* path? Name.Variant */
        while (at(p, TK_DOT) && peek(p, 1) == TK_IDENT) { advance(p); name = advance(p); }
        if (at(p, TK_LPAREN)) {                      /* variant / record pattern with subpatterns */
            Node *v = node_new(p->a, N_PVARIANT, name->line, name->col);
            node_set_text(v, name->start, name->len);
            advance(p);
            while (!at(p, TK_RPAREN) && !at(p, TK_EOF)) {
                if (at(p, TK_IDENT) && peek(p, 1) == TK_COLON) {   /* field: pattern */
                    Node *f = mk(p, N_PFIELD); node_add(p->a, f, ident_node(p, N_IDENT)); advance(p); node_add(p->a, f, parse_pattern(p)); node_add(p->a, v, f);
                } else node_add(p->a, v, parse_pattern(p));
                if (!accept(p, TK_COMMA)) break;
            }
            expect(p, TK_RPAREN, "P0152", "expected ) to close a variant pattern", NULL);
            pat = v;
        } else if (starts_lower(name)) {             /* lowercase bare ident = binding */
            pat = node_new(p->a, N_PBIND, name->line, name->col); node_set_text(pat, name->start, name->len);
        } else {                                     /* PascalCase bare = unit variant / const */
            pat = node_new(p->a, N_PVARIANT, name->line, name->col); node_set_text(pat, name->start, name->len);
        }
    }
    else { perr(p, "P0153", "expected a pattern", NULL); pat = mk(p, N_ERROR_NODE); advance(p); }

    if (accept(p, TK_AT)) {                          /* binder @ pattern */
        Node *b = node_new(p->a, N_PAT, pat->line, pat->col);
        node_add(p->a, b, pat);
        node_add(p->a, b, parse_pattern(p));
        pat = b;
    }
    return pat;
}

/* ======================================================================
 *  Expressions (Pratt)
 * ====================================================================== */

/* left binding power of an infix/postfix operator; 0 = not an infix operator */
static int infix_lbp(P *p) {
    switch (kind(p)) {
    case TK_KW_CATCH:   return 1;
    case TK_PIPE_GT:    return 2;
    case TK_KW_OR:      return (peek(p, 1) == TK_KW_FAIL) ? 0 : 3;  /* `or fail` is contract syntax */
    case TK_PIPEPIPE:   return 3;
    case TK_KW_AND: case TK_AMPAMP:  return 4;
    case TK_QQ:         return 5;
    case TK_EQEQ: case TK_NE: case TK_LT: case TK_LE: case TK_GT: case TK_GE:
    case TK_KW_IN: case TK_KW_IS: case TK_KW_AS: return 6;
    case TK_KW_NOT:     return (peek(p, 1) == TK_KW_IN) ? 6 : 0;   /* `not in` */
    case TK_DOTDOT: case TK_DOTDOT_EQ: return 7;
    case TK_PIPE:       return 8;
    case TK_CARET:      return 9;
    case TK_AMP:        return 10;
    case TK_SHL: case TK_SHR: return 11;
    case TK_PLUS: case TK_MINUS: case TK_PLUS_WRAP: case TK_MINUS_WRAP:
    case TK_PLUS_SAT: case TK_MINUS_SAT: case TK_PLUS_CHK: case TK_MINUS_CHK: return 12;
    case TK_STAR: case TK_SLASH: case TK_PERCENT: case TK_AT: return 13;
    case TK_POW:        return 14;   /* right-assoc */
    default:            return 0;
    }
}

static Node *parse_args(P *p) {
    Node *list = mk(p, N_ARG);     /* container; children are N_ARG */
    while (!at(p, TK_RPAREN) && !at(p, TK_EOF)) {
        Node *arg = mk(p, N_ARG);
        if (at(p, TK_IDENT) && peek(p, 1) == TK_COLON) { node_add(p->a, arg, ident_node(p, N_IDENT)); advance(p); }
        if (accept(p, TK_KW_OWN)) arg->flags |= F_OWN;
        node_add(p->a, arg, parse_expr(p, 0));
        node_add(p->a, list, arg);
        if (!accept(p, TK_COMMA)) break;
    }
    return list;
}

static Node *parse_closure(P *p, int flags) {
    Node *n = mk(p, N_CLOSURE); n->flags = flags;
    if (at(p, TK_PIPEPIPE)) advance(p);    /* no params */
    else {
        expect(p, TK_PIPE, "P0160", "expected | to open a closure", NULL);
        while (!at(p, TK_PIPE) && !at(p, TK_EOF)) {
            Node *cp = mk(p, N_CLOSPARAM);
            if (accept(p, TK_KW_OWN)) cp->flags |= F_OWN;
            else if (accept(p, TK_KW_MUT)) cp->flags |= F_MUT;
            node_add(p->a, cp, parse_pattern(p));
            if (accept(p, TK_COLON)) node_add(p->a, cp, parse_type(p));
            node_add(p->a, n, cp);
            if (!accept(p, TK_COMMA)) break;
        }
        expect(p, TK_PIPE, "P0161", "expected | to close the closure parameters", NULL);
    }
    if (accept(p, TK_ARROW)) node_add(p->a, n, parse_type(p));
    if (at(p, TK_NEWLINE)) node_add(p->a, n, parse_block(p));
    else node_add(p->a, n, parse_expr(p, 0));
    return n;
}

static Node *lit_node(P *p, NodeKind nk) { Node *n = mk(p, nk); const Token *t = advance(p); node_set_text(n, t->start, t->len); return n; }

static Node *parse_primary(P *p) {
    TokenKind k = kind(p);
    switch (k) {
    case TK_INT:         return lit_node(p, N_INT);
    case TK_FLOAT:       return lit_node(p, N_FLOAT);
    case TK_STRING:      return lit_node(p, N_STR);
    case TK_RAW_STRING:  return lit_node(p, N_RAWSTR);
    case TK_BYTE_STRING: return lit_node(p, N_BYTESTR);
    case TK_REGEX:       return lit_node(p, N_REGEX);
    case TK_CHAR:        return lit_node(p, N_CHAR);
    case TK_KW_TRUE: case TK_KW_FALSE: { Node *n = mk(p, N_BOOL); const Token *t = advance(p); node_set_text(n, t->start, t->len); return n; }
    case TK_KW_NONE:     { Node *n = mk(p, N_NONE); advance(p); return n; }
    case TK_KW_SELF:     { Node *n = mk(p, N_SELF); advance(p); return n; }
    case TK_QUESTION:    { Node *n = mk(p, N_HOLE); advance(p); return n; }
    case TK_KW_TODO:     { Node *n = mk(p, N_TODO); advance(p); if (at(p, TK_STRING)) node_add(p->a, n, lit_node(p, N_STR)); return n; }
    case TK_IDENT:       { Node *n = mk(p, N_IDENT); const Token *t = advance(p); node_set_text(n, t->start, t->len); return n; }
    case TK_KW_IF:       return parse_if(p);
    case TK_KW_MATCH:    return parse_match(p);
    case TK_KW_LOOP:     { Node *n = mk(p, N_LOOP); advance(p); node_add(p->a, n, parse_block(p)); return n; }
    case TK_KW_DO:       { Node *n = mk(p, N_DOBLOCK); advance(p); node_add(p->a, n, parse_block(p)); return n; }
    case TK_KW_UNSAFE:   { Node *n = mk(p, N_UNSAFE); advance(p); node_add(p->a, n, parse_block(p)); return n; }
    case TK_KW_COMPTIME: { Node *n = mk(p, N_COMPTIME); advance(p); if (at(p, TK_NEWLINE)) node_add(p->a, n, parse_block(p)); else node_add(p->a, n, parse_expr(p, 0)); return n; }
    case TK_PIPE: case TK_PIPEPIPE: return parse_closure(p, 0);
    case TK_DOT:         {                     /* field accessor shorthand: .salary */
        if (peek(p, 1) == TK_IDENT) { Node *n = mk(p, N_FIELDACC); advance(p); node_add(p->a, n, ident_node(p, N_IDENT)); return n; }
        break;
    }
    case TK_LPAREN: {                           /* unit / paren / tuple */
        const Token *lp = advance(p);
        if (at(p, TK_RPAREN)) { advance(p); return mk_at(p, N_TUPLE, lp); }
        Node *first = parse_expr(p, 0);
        if (at(p, TK_COMMA)) {
            Node *tup = mk_at(p, N_TUPLE, lp); node_add(p->a, tup, first);
            while (accept(p, TK_COMMA)) { if (at(p, TK_RPAREN)) break; node_add(p->a, tup, parse_expr(p, 0)); }
            expect(p, TK_RPAREN, "P0170", "expected ) to close a tuple", NULL);
            return tup;
        }
        expect(p, TK_RPAREN, "P0171", "expected ) to close a parenthesised expression", NULL);
        return first;
    }
    case TK_LBRACKET: {                          /* list / comprehension / repeat */
        const Token *lb = advance(p);
        if (at(p, TK_RBRACKET)) { advance(p); return mk_at(p, N_LIST, lb); }
        Node *first = parse_expr(p, 0);
        if (accept(p, TK_SEMI)) {                /* [value; count] */
            Node *r = mk_at(p, N_LISTREPEAT, lb); node_add(p->a, r, first); node_add(p->a, r, parse_expr(p, 0));
            expect(p, TK_RBRACKET, "P0172", "expected ] to close the list", NULL); return r;
        }
        if (at(p, TK_KW_FOR)) {                  /* comprehension */
            Node *c = mk_at(p, N_COMPREHENSION, lb); node_add(p->a, c, first);
            while (at(p, TK_KW_FOR)) {
                advance(p); Node *cf = mk(p, N_COMPFOR);
                node_add(p->a, cf, parse_pattern(p));
                expect(p, TK_KW_IN, "P0173", "expected `in` in a comprehension", NULL);
                node_add(p->a, cf, parse_expr(p, 0));
                node_add(p->a, c, cf);
            }
            if (accept(p, TK_KW_IF)) node_add(p->a, c, parse_expr(p, 0));
            expect(p, TK_RBRACKET, "P0174", "expected ] to close the comprehension", NULL); return c;
        }
        Node *list = mk_at(p, N_LIST, lb); node_add(p->a, list, first);
        while (accept(p, TK_COMMA)) { if (at(p, TK_RBRACKET)) break; node_add(p->a, list, parse_expr(p, 0)); }
        expect(p, TK_RBRACKET, "P0175", "expected ] to close the list", NULL);
        return list;
    }
    case TK_LBRACE: {                            /* map / set / map-comprehension */
        const Token *lb = advance(p);
        if (at(p, TK_RBRACE)) { advance(p); return mk_at(p, N_MAP, lb); }
        Node *first = parse_expr(p, 0);
        if (accept(p, TK_COLON)) {               /* map */
            Node *val = parse_expr(p, 0);
            if (at(p, TK_KW_FOR)) {
                Node *c = mk_at(p, N_COMPREHENSION, lb);
                Node *e = mk(p, N_MAPENTRY); node_add(p->a, e, first); node_add(p->a, e, val); node_add(p->a, c, e);
                advance(p); Node *cf = mk(p, N_COMPFOR); node_add(p->a, cf, parse_pattern(p));
                expect(p, TK_KW_IN, "P0176", "expected `in` in a map comprehension", NULL);
                node_add(p->a, cf, parse_expr(p, 0)); node_add(p->a, c, cf);
                if (accept(p, TK_KW_IF)) node_add(p->a, c, parse_expr(p, 0));
                expect(p, TK_RBRACE, "P0177", "expected } to close the map comprehension", NULL); return c;
            }
            Node *m = mk_at(p, N_MAP, lb);
            Node *e = mk(p, N_MAPENTRY); node_add(p->a, e, first); node_add(p->a, e, val); node_add(p->a, m, e);
            while (accept(p, TK_COMMA)) {
                if (at(p, TK_RBRACE)) break;
                Node *k2 = parse_expr(p, 0); expect(p, TK_COLON, "P0178", "expected : in a map entry", NULL); Node *v2 = parse_expr(p, 0);
                Node *e2 = mk(p, N_MAPENTRY); node_add(p->a, e2, k2); node_add(p->a, e2, v2); node_add(p->a, m, e2);
            }
            expect(p, TK_RBRACE, "P0179", "expected } to close the map", NULL); return m;
        }
        Node *set = mk_at(p, N_SET, lb); node_add(p->a, set, first);   /* set */
        while (accept(p, TK_COMMA)) { if (at(p, TK_RBRACE)) break; node_add(p->a, set, parse_expr(p, 0)); }
        expect(p, TK_RBRACE, "P017A", "expected } to close the set", NULL);
        return set;
    }
    case TK_DOTDOT: case TK_DOTDOT_EQ: {         /* prefix range: ..b  ..=b */
        Node *n = mk(p, N_RANGE); if (k == TK_DOTDOT_EQ) n->flags |= F_INCLUSIVE; advance(p);
        node_add(p->a, n, parse_expr(p, 8));
        return n;
    }
    default: break;
    }
    perr(p, "P0180", "expected an expression", NULL);
    Node *bad = mk(p, N_ERROR_NODE); if (!at(p, TK_NEWLINE) && !at(p, TK_EOF)) advance(p);
    return bad;
}

static Node *parse_postfix(P *p, Node *left) {
    for (;;) {
        TokenKind k = kind(p);
        if (k == TK_LPAREN) {
            Node *call = node_new(p->a, N_CALL, left->line, left->col);
            node_add(p->a, call, left); advance(p);
            Node *args = parse_args(p);
            expect(p, TK_RPAREN, "P0190", "expected ) to close the call", NULL);
            for (int i = 0; i < args->nkid; i++) node_add(p->a, call, args->kids[i]);
            left = call;
        } else if (k == TK_LBRACKET) {
            Node *idx = node_new(p->a, N_INDEX, left->line, left->col);
            node_add(p->a, idx, left); advance(p);
            if (at(p, TK_DOTDOT) || at(p, TK_DOTDOT_EQ)) {   /* [..b] */
                Node *r = mk(p, N_RANGE); if (at(p, TK_DOTDOT_EQ)) r->flags |= F_INCLUSIVE; advance(p);
                if (!at(p, TK_RBRACKET)) node_add(p->a, r, parse_expr(p, 0));
                node_add(p->a, idx, r);
            } else {
                Node *e = parse_expr(p, 0);
                if (at(p, TK_DOTDOT) || at(p, TK_DOTDOT_EQ)) { Node *r = mk(p, N_RANGE); if (at(p, TK_DOTDOT_EQ)) r->flags |= F_INCLUSIVE; advance(p); node_add(p->a, r, e); if (!at(p, TK_RBRACKET)) node_add(p->a, r, parse_expr(p, 0)); node_add(p->a, idx, r); }
                else node_add(p->a, idx, e);
            }
            expect(p, TK_RBRACKET, "P0191", "expected ] to close the index", NULL);
            left = idx;
        } else if (k == TK_DOT) {
            advance(p);
            if (at(p, TK_KW_AWAIT)) { advance(p); Node *a = node_new(p->a, N_AWAIT, left->line, left->col); node_add(p->a, a, left); left = a; }
            else {
                Node *f = node_new(p->a, N_FIELD_GET, left->line, left->col);
                node_add(p->a, f, left);
                if (at(p, TK_IDENT) || at(p, TK_KW_SELF)) { const Token *t = advance(p); node_set_text(f, t->start, t->len); }
                else if (at(p, TK_INT)) { const Token *t = advance(p); node_set_text(f, t->start, t->len); }
                else perr(p, "P0192", "expected a field name or index after .", NULL);
                left = f;
            }
        } else if (k == TK_QUESTION) {
            if (peek(p, 1) == TK_DOT) break;     /* `?.` handled below */
            advance(p); Node *t = node_new(p->a, N_TRYOP, left->line, left->col); node_add(p->a, t, left); left = t;
        } else if (k == TK_QUESTION && peek(p, 1) == TK_DOT) {
            break;
        } else if ((left->kind == N_FIELD_GET || left->kind == N_CALL) &&
                   (k == TK_PIPEPIPE || (k == TK_PIPE && (peek(p, 1) == TK_IDENT || peek(p, 1) == TK_KW_OWN || peek(p, 1) == TK_KW_MUT)))) {
            /* trailing closure: recv.method |args| body  ==  method(recv, |args| body) */
            Node *clo = parse_closure(p, 0);
            Node *arg = node_new(p->a, N_ARG, clo->line, clo->col);
            node_add(p->a, arg, clo);
            if (left->kind == N_CALL) { node_add(p->a, left, arg); }
            else { Node *call = node_new(p->a, N_CALL, left->line, left->col); node_add(p->a, call, left); node_add(p->a, call, arg); left = call; }
        } else {
            break;
        }
    }
    return left;
}

/* `?.` is two tokens (QUESTION DOT); handle as a postfix in its own check */
static Node *parse_optchain(P *p, Node *left) {
    while (at(p, TK_QUESTION) && peek(p, 1) == TK_DOT) {
        advance(p); advance(p);
        Node *o = node_new(p->a, N_OPTCHAIN, left->line, left->col);
        node_add(p->a, o, left);
        node_add(p->a, o, ident_node(p, N_IDENT));
        left = parse_postfix(p, o);
    }
    return left;
}

static Node *parse_prefix(P *p) {
    TokenKind k = kind(p);
    if (k == TK_KW_OWN && (peek(p, 1) == TK_PIPE || peek(p, 1) == TK_PIPEPIPE)) { advance(p); return parse_closure(p, F_OWN); }
    if (k == TK_MINUS || k == TK_KW_NOT || k == TK_BANG || k == TK_TILDE || k == TK_KW_OWN || k == TK_KW_MUT || k == TK_AMP) {
        Node *n = mk(p, N_UNARY); const Token *t = advance(p); node_set_text(n, t->start, t->len);
        node_add(p->a, n, parse_prefix(p));
        return n;
    }
    Node *prim = parse_primary(p);
    prim = parse_postfix(p, prim);
    prim = parse_optchain(p, prim);
    return prim;
}

static Node *parse_expr(P *p, int minbp) {
    Node *left = parse_prefix(p);
    for (;;) {
        int lbp = infix_lbp(p);
        if (lbp == 0 || lbp < minbp) break;
        TokenKind k = kind(p);

        if (k == TK_KW_AS) {                         /* cast */
            advance(p);
            Node *c = node_new(p->a, N_CAST, left->line, left->col);
            if (accept(p, TK_BANG)) c->flags |= F_MUT;       /* as!  (truncating) */
            else if (accept(p, TK_QUESTION)) c->flags |= F_OWN; /* as? (checked optional) */
            node_add(p->a, c, left);
            node_add(p->a, c, parse_type(p));
            left = c; continue;
        }
        if (k == TK_KW_CATCH) {
            advance(p);
            Node *c = node_new(p->a, N_CATCH, left->line, left->col);
            node_add(p->a, c, left);
            node_add(p->a, c, parse_expr(p, lbp + 1));
            left = c; continue;
        }
        if (k == TK_KW_IS) {                         /* `x is Type` */
            advance(p);
            Node *b = node_new(p->a, N_BINARY, left->line, left->col); node_set_text(b, "is", 2);
            node_add(p->a, b, left); node_add(p->a, b, parse_type(p));
            left = b; continue;
        }
        if (k == TK_KW_NOT && peek(p, 1) == TK_KW_IN) {
            advance(p); advance(p);
            Node *b = node_new(p->a, N_BINARY, left->line, left->col); node_set_text(b, "not in", 6);
            node_add(p->a, b, left); node_add(p->a, b, parse_expr(p, lbp + 1));
            left = b; continue;
        }
        if (k == TK_DOTDOT || k == TK_DOTDOT_EQ) {   /* range, optional rhs */
            Node *r = node_new(p->a, N_RANGE, left->line, left->col);
            if (k == TK_DOTDOT_EQ) r->flags |= F_INCLUSIVE;
            advance(p);
            node_add(p->a, r, left);
            TokenKind nk = kind(p);
            if (nk != TK_NEWLINE && nk != TK_RPAREN && nk != TK_RBRACKET && nk != TK_RBRACE &&
                nk != TK_COMMA && nk != TK_EOF && nk != TK_DEDENT && nk != TK_KW_THEN && nk != TK_LBRACE)
                node_add(p->a, r, parse_expr(p, lbp + 1));
            left = r; continue;
        }

        /* ordinary binary */
        const Token *op = advance(p);
        Node *b = node_new(p->a, N_BINARY, left->line, left->col);
        node_set_text(b, op->start, op->len);
        node_add(p->a, b, left);
        int rbp = (k == TK_POW || k == TK_QQ) ? lbp : lbp + 1;   /* ** and ?? right-assoc */
        node_add(p->a, b, parse_expr(p, rbp));
        left = b;
    }
    return left;
}

/* ======================================================================
 *  Entry point
 * ====================================================================== */
Node *lume_parse(const TokenList *toks, Arena *arena, DiagList *diags) {
    P p = { .t = toks->items, .n = toks->len, .i = 0, .a = arena, .d = diags, .errors_before = diags->errors };
    Node *mod = node_new(arena, N_MODULE, 1, 1);

    skip_newlines(&p);
    if (at(&p, TK_KW_MOD) && peek(&p, 1) == TK_IDENT &&
        (peek(&p, 2) == TK_NEWLINE || peek(&p, 2) == TK_DOT)) {
        advance(&p);
        Node *m = node_new(arena, N_MOD, mod->line, mod->col);
        node_add(arena, m, ident_node(&p, N_IDENT));
        while (at(&p, TK_DOT)) { advance(&p); node_add(arena, m, ident_node(&p, N_IDENT)); }
        accept(&p, TK_NEWLINE);
        node_add(arena, mod, m);
    }

    while (!at(&p, TK_EOF)) {
        skip_newlines(&p);
        if (at(&p, TK_EOF)) break;
        size_t before = p.i;
        Node *item = parse_item_or_stmt(&p);
        node_add(arena, mod, item);
        if (p.i == before) { perr(&p, "P0999", "parser made no progress; skipping a token", NULL); advance(&p); }
        while (at(&p, TK_NEWLINE) || at(&p, TK_DEDENT)) advance(&p);   /* tolerate stray layout at top level */
    }
    return mod;
}
