#include "value.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* runtime arena: reuse the AST arena API shape but a separate instance */
extern void *lume_rt_alloc(Interp *ip, size_t n);   /* defined in interp.c */

static Val *val_new(Interp *ip, VKind k) {
    Val *v = lume_rt_alloc(ip, sizeof *v);
    memset(v, 0, sizeof *v);
    v->kind = k;
    return v;
}

Val *v_unit(Interp *ip)             { return val_new(ip, V_UNIT); }
Val *v_bool(Interp *ip, bool b)     { Val *v = val_new(ip, V_BOOL); v->b = b; return v; }
Val *v_int(Interp *ip, long long i) { Val *v = val_new(ip, V_INT); v->i = i; return v; }
Val *v_float(Interp *ip, double f)  { Val *v = val_new(ip, V_FLOAT); v->f = f; return v; }
Val *v_dec(Interp *ip, long long u, int s) { Val *v = val_new(ip, V_DEC); v->i = u; v->scale = s; return v; }
Val *v_char(Interp *ip, unsigned cp){ Val *v = val_new(ip, V_CHAR); v->i = (long long)cp; return v; }

Val *v_str_owned(Interp *ip, char *s, int len) { Val *v = val_new(ip, V_STR); v->s = s; v->slen = len; return v; }
Val *v_str(Interp *ip, const char *s, int len) {
    char *buf = lume_rt_alloc(ip, (size_t)len + 1);
    if (len) memcpy(buf, s, (size_t)len);
    buf[len] = 0;
    return v_str_owned(ip, buf, len);
}
Val *v_list(Interp *ip) { return val_new(ip, V_LIST); }
Val *v_opt_some(Interp *ip, Val *x) { Val *v = val_new(ip, V_OPT); v->b = true; v->inner = x; return v; }
Val *v_opt_none(Interp *ip)         { Val *v = val_new(ip, V_OPT); v->b = false; return v; }
Val *v_ok(Interp *ip, Val *x)       { Val *v = val_new(ip, V_RESULT); v->b = true; v->inner = x; return v; }
Val *v_err(Interp *ip, Val *x)      { Val *v = val_new(ip, V_RESULT); v->b = false; v->inner = x; return v; }

void v_push(Interp *ip, Val *list, Val *item) {
    if (list->n == list->cap) {
        int nc = list->cap ? list->cap * 2 : 8;
        Val **ni = lume_rt_alloc(ip, (size_t)nc * sizeof(Val *));
        if (list->items) memcpy(ni, list->items, (size_t)list->n * sizeof(Val *));
        list->items = ni; list->cap = nc;
    }
    list->items[list->n++] = item;
}

/* ---------- environment ---------- */
struct Env { Env *parent; Entry *ent; int n, cap; };

Env *env_new(Interp *ip, Env *parent) {
    Env *e = lume_rt_alloc(ip, sizeof *e);
    memset(e, 0, sizeof *e);
    e->parent = parent;
    return e;
}
static bool name_eq(const Entry *en, const char *name, int len) {
    return en->klen == len && memcmp(en->key, name, (size_t)len) == 0;
}
Val **env_slot(Env *e, const char *name, int len) {
    for (; e; e = e->parent)
        for (int i = 0; i < e->n; i++)
            if (name_eq(&e->ent[i], name, len)) return &e->ent[i].val;
    return NULL;
}
Val *env_get(Env *e, const char *name, int len) {
    Val **s = env_slot(e, name, len);
    return s ? *s : NULL;
}
void env_define(Interp *ip, Env *e, const char *name, int len, Val *v) {
    for (int i = 0; i < e->n; i++)                 /* shadow in same scope => overwrite */
        if (name_eq(&e->ent[i], name, len)) { e->ent[i].val = v; return; }
    if (e->n == e->cap) {
        int nc = e->cap ? e->cap * 2 : 8;
        Entry *ne = lume_rt_alloc(ip, (size_t)nc * sizeof(Entry));
        if (e->ent) memcpy(ne, e->ent, (size_t)e->n * sizeof(Entry));
        e->ent = ne; e->cap = nc;
    }
    char *key = lume_rt_alloc(ip, (size_t)len + 1);
    if (len > 0 && name) memcpy(key, name, (size_t)len);
    key[len] = 0;
    e->ent[e->n].key = key; e->ent[e->n].klen = len; e->ent[e->n].val = v;
    e->n++;
}

/* ---------- equality / truthiness ---------- */
bool val_truthy(Val *v) {
    switch (v->kind) {
    case V_BOOL: return v->b;
    case V_OPT:  return v->b;
    case V_UNIT: return false;
    default: return true;
    }
}
static double num_as_double(Val *v) {
    if (v->kind == V_INT) return (double)v->i;
    if (v->kind == V_FLOAT) return v->f;
    long long p = 1; for (int i = 0; i < v->scale; i++) p *= 10;
    return (double)v->i / (double)p;
}
bool val_equal(Val *a, Val *b) {
    bool an = a->kind==V_INT||a->kind==V_FLOAT||a->kind==V_DEC;
    bool bn = b->kind==V_INT||b->kind==V_FLOAT||b->kind==V_DEC;
    if (a->kind != b->kind) {
        if (an && bn) return num_as_double(a) == num_as_double(b);
        return false;
    }
    switch (a->kind) {
    case V_UNIT: return true;
    case V_BOOL: return a->b == b->b;
    case V_INT: case V_CHAR: return a->i == b->i;
    case V_FLOAT: return a->f == b->f;
    case V_DEC: {
        long long ax = a->i, bx = b->i; int sa = a->scale, sb = b->scale;
        while (sa < sb) { ax *= 10; sa++; } while (sb < sa) { bx *= 10; sb++; }
        return ax == bx;
    }
    case V_STR: return a->slen == b->slen && memcmp(a->s, b->s, (size_t)a->slen) == 0;
    case V_OPT: return a->b == b->b && (!a->b || val_equal(a->inner, b->inner));
    case V_RESULT: return a->b == b->b && val_equal(a->inner, b->inner);
    case V_LIST: case V_TUPLE: case V_SET:
        if (a->n != b->n) return false;
        for (int i = 0; i < a->n; i++) if (!val_equal(a->items[i], b->items[i])) return false;
        return true;
    case V_VARIANT:
        if (a->tag != b->tag && strcmp(a->tag ? a->tag : "", b->tag ? b->tag : "")) return false;
        if (a->n != b->n) return false;
        for (int i = 0; i < a->n; i++) if (!val_equal(a->items[i], b->items[i])) return false;
        return true;
    default: return a == b;
    }
}

/* ---------- Show ---------- */
typedef struct { char *buf; int len, cap; Interp *ip; } SB;
static void sb_put(SB *sb, const char *s, int n) {
    if (sb->len + n + 1 > sb->cap) {
        int nc = sb->cap ? sb->cap * 2 : 64;
        while (nc < sb->len + n + 1) nc *= 2;
        char *nb = lume_rt_alloc(sb->ip, (size_t)nc);
        if (sb->buf) memcpy(nb, sb->buf, (size_t)sb->len);
        sb->buf = nb; sb->cap = nc;
    }
    memcpy(sb->buf + sb->len, s, (size_t)n); sb->len += n; sb->buf[sb->len] = 0;
}
static void sb_puts(SB *sb, const char *s) { sb_put(sb, s, (int)strlen(s)); }
static void sb_fmt(SB *sb, const char *fmt, long long x) { char t[32]; int n = snprintf(t, sizeof t, fmt, x); sb_put(sb, t, n); }

static void dec_to_sb(SB *sb, long long units, int scale) {
    char tmp[64];
    bool neg = units < 0; unsigned long long u = neg ? (unsigned long long)(-units) : (unsigned long long)units;
    int n = snprintf(tmp, sizeof tmp, "%llu", u);
    if (neg) sb_puts(sb, "-");
    if (scale == 0) { sb_put(sb, tmp, n); return; }
    if (n <= scale) { sb_puts(sb, "0."); for (int i = 0; i < scale - n; i++) sb_puts(sb, "0"); sb_put(sb, tmp, n); }
    else { sb_put(sb, tmp, n - scale); sb_puts(sb, "."); sb_put(sb, tmp + (n - scale), scale); }
}

static void show(SB *sb, Val *v) {
    char t[64];
    switch (v->kind) {
    case V_UNIT: sb_puts(sb, "()"); break;
    case V_BOOL: sb_puts(sb, v->b ? "true" : "false"); break;
    case V_INT: sb_fmt(sb, "%lld", v->i); break;
    case V_FLOAT: { int n = snprintf(t, sizeof t, "%g", v->f); sb_put(sb, t, n); break; }
    case V_DEC: dec_to_sb(sb, v->i, v->scale); break;
    case V_CHAR: {
        unsigned cp = (unsigned)v->i;
        if (cp < 0x80) { char c = (char)cp; sb_put(sb, &c, 1); }
        else if (cp < 0x800) { char b[2] = { (char)(0xC0|(cp>>6)), (char)(0x80|(cp&0x3F)) }; sb_put(sb, b, 2); }
        else if (cp < 0x10000) { char b[3] = { (char)(0xE0|(cp>>12)), (char)(0x80|((cp>>6)&0x3F)), (char)(0x80|(cp&0x3F)) }; sb_put(sb, b, 3); }
        else { char b[4] = { (char)(0xF0|(cp>>18)), (char)(0x80|((cp>>12)&0x3F)), (char)(0x80|((cp>>6)&0x3F)), (char)(0x80|(cp&0x3F)) }; sb_put(sb, b, 4); }
        break;
    }
    case V_STR: sb_put(sb, v->s, v->slen); break;
    case V_OPT: if (v->b) { sb_puts(sb, "some("); show(sb, v->inner); sb_puts(sb, ")"); } else sb_puts(sb, "none"); break;
    case V_RESULT: sb_puts(sb, v->b ? "ok(" : "err("); show(sb, v->inner); sb_puts(sb, ")"); break;
    case V_LIST: case V_SET: {
        sb_puts(sb, v->kind == V_SET ? "{" : "[");
        for (int i = 0; i < v->n; i++) { if (i) sb_puts(sb, ", "); show(sb, v->items[i]); }
        sb_puts(sb, v->kind == V_SET ? "}" : "]");
        break;
    }
    case V_TUPLE: {
        sb_puts(sb, "(");
        for (int i = 0; i < v->n; i++) { if (i) sb_puts(sb, ", "); show(sb, v->items[i]); }
        sb_puts(sb, ")");
        break;
    }
    case V_MAP: {
        sb_puts(sb, "{");
        for (int i = 0; i < v->nent; i++) { if (i) sb_puts(sb, ", "); sb_put(sb, v->ent[i].key, v->ent[i].klen); sb_puts(sb, ": "); show(sb, v->ent[i].val); }
        sb_puts(sb, "}");
        break;
    }
    case V_RECORD: {
        sb_puts(sb, v->tname ? v->tname : "record"); sb_puts(sb, "(");
        for (int i = 0; i < v->nent; i++) { if (i) sb_puts(sb, ", "); sb_put(sb, v->ent[i].key, v->ent[i].klen); sb_puts(sb, ": "); show(sb, v->ent[i].val); }
        sb_puts(sb, ")");
        break;
    }
    case V_VARIANT: {
        sb_puts(sb, v->tag ? v->tag : "?");
        if (v->n) { sb_puts(sb, "("); for (int i = 0; i < v->n; i++) { if (i) sb_puts(sb, ", "); show(sb, v->items[i]); } sb_puts(sb, ")"); }
        break;
    }
    case V_RANGE: { sb_fmt(sb, "%lld", v->lo); sb_puts(sb, v->inc ? "..=" : ".."); sb_fmt(sb, "%lld", v->hi); break; }
    case V_CLOSURE: sb_puts(sb, "<closure>"); break;
    case V_BUILTIN: sb_puts(sb, "<builtin "); sb_puts(sb, v->bname ? v->bname : "?"); sb_puts(sb, ">"); break;
    case V_FIELDACC: sb_puts(sb, "<.accessor>"); break;
    case V_TYPE: sb_puts(sb, v->tname ? v->tname : "<type>"); break;
    }
}

char *val_to_cstr(Interp *ip, Val *v, int *out_len) {
    SB sb = { .ip = ip };
    show(&sb, v);
    if (!sb.buf) { sb.buf = lume_rt_alloc(ip, 1); sb.buf[0] = 0; sb.len = 0; }
    if (out_len) *out_len = sb.len;
    return sb.buf;
}
