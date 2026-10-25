/* Tree-walking interpreter for the Stage-0 Lume subset. Dynamically evaluated; contracts checked
 * at runtime (ownership/types are Stage 1's job). Goal: run examples 01–07. */
#include "value.h"
#include "ast.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

/* runtime arena = an AST Arena instance */
void *lume_rt_alloc(Interp *ip, size_t n) { return arena_alloc(ip->rt, n); }

/* ---------- registries built from the module ---------- */
typedef struct { const char *name; int len; const Node *decl; } TypeReg;
typedef struct { const char *name; int len; const char *owner; const Node *variant; } VarReg;
typedef struct { const char *name; int len; const char *self_type; const char *trait; Val *fn; } MethReg;
typedef struct { const char *trait; const char *method; Val *fn; } DefReg;
typedef struct { const char *type; const char *trait; } ImplReg;

typedef struct {
    TypeReg *types; int ntypes, ctypes;
    VarReg  *vars;  int nvars, cvars;
    MethReg *meths; int nmeths, cmeths;
    DefReg  *defs;  int ndefs, cdefs;
    ImplReg *impls; int nimpls, cimpls;
} Registry;

static Registry REG;

/* ---------- helpers ---------- */
static char *dup_text(Interp *ip, const Node *n) {
    char *s = lume_rt_alloc(ip, (size_t)n->text_len + 1);
    memcpy(s, n->text, (size_t)n->text_len); s[n->text_len] = 0; return s;
}
static char *ntz(Interp *ip, const char *t, int len) { char *s = lume_rt_alloc(ip, (size_t)len+1); memcpy(s, t, (size_t)len); s[len]=0; return s; }
static bool text_is(const Node *n, const char *s) {
    return n->text && (int)strlen(s) == n->text_len && memcmp(n->text, s, (size_t)n->text_len) == 0;
}
static void rerror(Interp *ip, const Node *n, const char *msg) {
    diag_add(ip->d, SEV_ERROR, "R0001", n ? n->line : 0, n ? n->col : 0, 1, msg,
             "this construct is not supported by the Stage-0 interpreter");
    ip->had_runtime_error = true;
    ip->flow = FL_PANIC;
    ip->signal = v_str(ip, msg, (int)strlen(msg));
}

/* forward */
static Val *eval(Interp *ip, const Node *n, Env *env);
static Val *eval_block(Interp *ip, const Node *b, Env *env);
static Val *call_value(Interp *ip, Val *fn, Val **args, int nargs, const Node *site);
static bool pattern_match(Interp *ip, const Node *pat, Val *v, Env *env);

/* ---------- numeric literals ---------- */
static long long parse_int(const char *s, int len) {
    long long v = 0; int base = 10, i = 0;
    if (len > 2 && s[0] == '0' && (s[1] == 'x' || s[1] == 'b' || s[1] == 'o')) {
        base = s[1] == 'x' ? 16 : s[1] == 'b' ? 2 : 8; i = 2;
    }
    for (; i < len; i++) {
        char c = s[i]; if (c == '_') continue;
        int d = (c >= '0' && c <= '9') ? c - '0' : (c >= 'a' && c <= 'f') ? c - 'a' + 10 : (c >= 'A' && c <= 'F') ? c - 'A' + 10 : 0;
        v = v * base + d;
    }
    return v;
}
static double parse_float(const char *s, int len) {
    char buf[64]; int j = 0;
    for (int i = 0; i < len && j < 63; i++) if (s[i] != '_') buf[j++] = s[i];
    buf[j] = 0; return strtod(buf, NULL);
}

/* ---------- string unescape + interpolation ---------- */
static Val *eval_source_expr(Interp *ip, const char *src, int len, Env *env);

static long long pow10i(int n) { long long p = 1; while (n-- > 0) p *= 10; return p; }

static Val *coerce_decimal(Interp *ip, Val *v, int scale) {
    if (v->kind == V_DEC) {
        long long u = v->i; int s = v->scale;
        while (s < scale) { u *= 10; s++; } while (s > scale) { u /= 10; s--; }
        return v_dec(ip, u, scale);
    }
    if (v->kind == V_INT)   return v_dec(ip, v->i * pow10i(scale), scale);
    if (v->kind == V_FLOAT) return v_dec(ip, llround(v->f * (double)pow10i(scale)), scale);
    return v;
}

/* append a formatted value honouring a Python-ish spec: [<>^]?width(.prec)? */
static void apply_format(Interp *ip, Val *v, const char *spec, int spec_len, char **out, int *outlen);

static Val *eval_string(Interp *ip, const Node *n, Env *env, bool interp, bool raw) {
    const char *t = n->text; int tl = n->text_len;
    int qs = 1, qe = 1;                           /* strip quotes (handle triple and prefixes) */
    int start = 0;
    if (!interp && !raw) start = 0;               /* plain */
    /* skip prefix letters r/b/re */
    while (start < tl && (t[start] == 'r' || t[start] == 'b' || t[start] == 'e')) start++;
    if (start + 2 < tl && t[start] == '"' && t[start+1] == '"' && t[start+2] == '"') { qs = 3; qe = 3; }
    int i = start + qs, end = tl - qe;
    char *buf = lume_rt_alloc(ip, (size_t)(tl * 4 + 1)); int bl = 0;
    Val *result_pieces = NULL; /* if interpolation, we build incrementally into buf */
    for (; i < end; i++) {
        char c = t[i];
        if (!raw && c == '\\' && i + 1 < end) {
            char e = t[++i];
            switch (e) {
            case 'n': buf[bl++] = '\n'; break; case 't': buf[bl++] = '\t'; break;
            case 'r': buf[bl++] = '\r'; break; case '0': buf[bl++] = '\0'; break;
            case '\\': buf[bl++] = '\\'; break; case '"': buf[bl++] = '"'; break;
            case '\'': buf[bl++] = '\''; break; case '{': buf[bl++] = '{'; break; case '}': buf[bl++] = '}'; break;
            case 'u': {
                if (i + 1 < end && t[i+1] == '{') { i += 2; unsigned cp = 0; while (i < end && t[i] != '}') { char h = t[i++]; int d = (h>='0'&&h<='9')?h-'0':(h>='a'&&h<='f')?h-'a'+10:(h>='A'&&h<='F')?h-'A'+10:0; cp = cp*16 + (unsigned)d; }
                    Val *ch = v_char(ip, cp); int cl; char *cs = val_to_cstr(ip, ch, &cl); memcpy(buf+bl, cs, (size_t)cl); bl += cl; }
                break;
            }
            default: buf[bl++] = e;
            }
            continue;
        }
        if (interp && c == '{') {
            if (i + 1 < end && t[i+1] == '{') { buf[bl++] = '{'; i++; continue; }
            int depth = 1, j = i + 1, exprstart = j;
            while (j < end && depth > 0) { if (t[j] == '{') depth++; else if (t[j] == '}') depth--; if (depth == 0) break; j++; }
            int exprend = j;
            /* split optional :spec at top level */
            int colon = -1, d2 = 0;
            for (int k = exprstart; k < exprend; k++) { char cc = t[k]; if (cc=='('||cc=='['||cc=='{') d2++; else if (cc==')'||cc==']'||cc=='}') d2--; else if (cc==':'&&d2==0) { colon = k; break; } }
            int elen = (colon >= 0 ? colon : exprend) - exprstart;
            Val *ev = eval_source_expr(ip, t + exprstart, elen, env);
            if (ip->flow != FL_NORMAL) return ev;
            char *piece; int plen;
            if (colon >= 0) apply_format(ip, ev, t + colon + 1, exprend - (colon + 1), &piece, &plen);
            else piece = val_to_cstr(ip, ev, &plen);
            memcpy(buf + bl, piece, (size_t)plen); bl += plen;
            i = exprend;
            continue;
        }
        if (interp && c == '}' && i + 1 < end && t[i+1] == '}') { buf[bl++] = '}'; i++; continue; }
        buf[bl++] = c;
    }
    buf[bl] = 0;
    (void)result_pieces;
    return v_str_owned(ip, buf, bl);
}

static void apply_format(Interp *ip, Val *v, const char *spec, int spec_len, char **out, int *outlen) {
    char align = 0; int width = 0, prec = -1; int i = 0;
    if (spec_len >= 1 && (spec[0] == '<' || spec[0] == '>' || spec[0] == '^')) { align = spec[0]; i = 1; }
    else if (spec_len >= 2 && (spec[1] == '<' || spec[1] == '>' || spec[1] == '^')) { align = spec[1]; i = 2; }
    while (i < spec_len && spec[i] >= '0' && spec[i] <= '9') { width = width * 10 + (spec[i]-'0'); i++; }
    if (i < spec_len && spec[i] == '.') { i++; prec = 0; while (i < spec_len && spec[i] >= '0' && spec[i] <= '9') { prec = prec*10 + (spec[i]-'0'); i++; } }

    char num[64]; char *body; int blen;
    if (prec >= 0 && (v->kind == V_FLOAT || v->kind == V_INT)) {
        double d = v->kind == V_FLOAT ? v->f : (double)v->i;
        blen = snprintf(num, sizeof num, "%.*f", prec, d); body = num;
    } else if (prec >= 0 && v->kind == V_DEC) {
        double d = (double)v->i / (double)pow10i(v->scale);
        blen = snprintf(num, sizeof num, "%.*f", prec, d); body = num;
    } else {
        body = val_to_cstr(ip, v, &blen);
    }
    if (width <= blen) { char *b = lume_rt_alloc(ip, (size_t)blen + 1); memcpy(b, body, (size_t)blen); b[blen] = 0; *out = b; *outlen = blen; return; }
    int pad = width - blen;
    char *buf = lume_rt_alloc(ip, (size_t)width + 1);
    int left = 0;
    if (align == '>' ) left = pad; else if (align == '^') left = pad/2; else left = 0; /* default left for strings */
    /* numbers default right-align */
    if (align == 0 && (v->kind==V_INT||v->kind==V_FLOAT||v->kind==V_DEC)) left = pad;
    int bi = 0;
    for (int k = 0; k < left; k++) buf[bi++] = ' ';
    memcpy(buf + bi, body, (size_t)blen); bi += blen;
    while (bi < width) buf[bi++] = ' ';
    buf[bi] = 0; *out = buf; *outlen = bi;
}

/* ======================================================================
 *  Numeric ops and ordering
 * ====================================================================== */
static bool is_num(Val *v) { return v->kind == V_INT || v->kind == V_FLOAT || v->kind == V_DEC; }
static double as_double(Val *v) { return v->kind == V_INT ? (double)v->i : v->kind == V_DEC ? (double)v->i / (double)pow10i(v->scale) : v->f; }

/* returns -2 if incomparable, else -1/0/1 */
static int cmp_vals(Val *a, Val *b) {
    if (a->kind == V_STR && b->kind == V_STR) {
        int m = a->slen < b->slen ? a->slen : b->slen;
        int c = memcmp(a->s, b->s, (size_t)m);
        if (c) return c < 0 ? -1 : 1;
        return a->slen == b->slen ? 0 : (a->slen < b->slen ? -1 : 1);
    }
    if (a->kind == V_CHAR && b->kind == V_CHAR) return a->i < b->i ? -1 : a->i > b->i ? 1 : 0;
    if (a->kind == V_DEC && b->kind == V_DEC) {
        long long ax = a->i, bx = b->i; int sa = a->scale, sb = b->scale;
        while (sa < sb) { ax *= 10; sa++; } while (sb < sa) { bx *= 10; sb++; }
        return ax < bx ? -1 : ax > bx ? 1 : 0;
    }
    if (is_num(a) && is_num(b)) { double x = as_double(a), y = as_double(b); return x < y ? -1 : x > y ? 1 : 0; }
    if (a->kind == V_BOOL && b->kind == V_BOOL) return (int)a->b - (int)b->b;
    return -2;
}

static Val *dec_binop(Interp *ip, char op, Val *a, Val *b) {
    /* promote int to decimal scale 0 */
    long long au = a->kind == V_DEC ? a->i : a->i, bu = b->kind == V_DEC ? b->i : b->i;
    int sa = a->kind == V_DEC ? a->scale : 0, sb = b->kind == V_DEC ? b->scale : 0;
    if (a->kind == V_INT) { au = a->i; sa = 0; } if (b->kind == V_INT) { bu = b->i; sb = 0; }
    if (op == '+' || op == '-') {
        int s = sa > sb ? sa : sb; long long x = au, y = bu; int ta = sa, tb = sb;
        while (ta < s) { x *= 10; ta++; } while (tb < s) { y *= 10; tb++; }
        return v_dec(ip, op == '+' ? x + y : x - y, s);
    }
    if (op == '*') return v_dec(ip, au * bu, sa + sb);
    if (op == '/') { int s = sa - sb; if (s < 0) s = 0; return v_dec(ip, bu ? au / bu : 0, s); }
    return v_int(ip, 0);
}

static Val *arith(Interp *ip, const char *op, int oplen, Val *a, Val *b, const Node *site) {
    char o = op[0];
    /* comparisons */
    if (oplen == 2 && op[0] == '=' && op[1] == '=') return v_bool(ip, val_equal(a, b));
    if (oplen == 2 && op[0] == '!' && op[1] == '=') return v_bool(ip, !val_equal(a, b));
    if ((o == '<' || o == '>')) {
        int c = cmp_vals(a, b);
        if (c == -2) { rerror(ip, site, "values are not comparable"); return v_unit(ip); }
        if (oplen == 1) return v_bool(ip, o == '<' ? c < 0 : c > 0);
        return v_bool(ip, o == '<' ? c <= 0 : c >= 0);   /* <=, >= */
    }
    if (a->kind == V_STR && b->kind == V_STR && o == '+') {
        int nl = a->slen + b->slen; char *buf = lume_rt_alloc(ip, (size_t)nl + 1);
        memcpy(buf, a->s, (size_t)a->slen); memcpy(buf + a->slen, b->s, (size_t)b->slen); buf[nl] = 0;
        return v_str_owned(ip, buf, nl);
    }
    if (a->kind == V_DEC || b->kind == V_DEC) {
        if ((a->kind == V_DEC || a->kind == V_INT) && (b->kind == V_DEC || b->kind == V_INT))
            return dec_binop(ip, o, a, b);
    }
    if (a->kind == V_FLOAT || b->kind == V_FLOAT) {
        double x = as_double(a), y = as_double(b);
        switch (o) {
        case '+': return v_float(ip, x + y); case '-': return v_float(ip, x - y);
        case '*': return v_float(ip, x * y); case '/': return v_float(ip, x / y);
        case '%': return v_float(ip, fmod(x, y));
        }
        if (oplen == 2 && op[0] == '*' && op[1] == '*') return v_float(ip, pow(x, y));
    }
    if (a->kind == V_INT && b->kind == V_INT) {
        long long x = a->i, y = b->i;
        if (oplen == 2 && op[0] == '*' && op[1] == '*') { long long r = 1; for (long long k = 0; k < y; k++) r *= x; return v_int(ip, r); }
        switch (o) {
        case '+': return v_int(ip, x + y); case '-': return v_int(ip, x - y);
        case '*': return v_int(ip, x * y);
        case '/': if (y == 0) { rerror(ip, site, "integer division by zero"); return v_unit(ip); } return v_int(ip, x / y);
        case '%': if (y == 0) { rerror(ip, site, "integer modulo by zero"); return v_unit(ip); } return v_int(ip, x % y);
        }
    }
    rerror(ip, site, "unsupported operands for this operator");
    return v_unit(ip);
}

/* ======================================================================
 *  Iteration
 * ====================================================================== */
static Val *to_iter_list(Interp *ip, Val *v) {
    if (v->kind == V_LIST || v->kind == V_SET || v->kind == V_TUPLE) return v;
    Val *out = v_list(ip);
    if (v->kind == V_RANGE) {
        for (long long i = v->lo; v->inc ? i <= v->hi : i < v->hi; i++) v_push(ip, out, v_int(ip, i));
    } else if (v->kind == V_STR) {
        for (int i = 0; i < v->slen;) {
            unsigned char c = (unsigned char)v->s[i]; int len = c < 0x80 ? 1 : c < 0xE0 ? 2 : c < 0xF0 ? 3 : 4;
            unsigned cp = 0;
            if (len == 1) cp = c; else { cp = c & (0x7Fu >> len); for (int k = 1; k < len; k++) cp = (cp << 6) | ((unsigned char)v->s[i+k] & 0x3F); }
            v_push(ip, out, v_char(ip, cp)); i += len;
        }
    } else if (v->kind == V_MAP) {
        for (int i = 0; i < v->nent; i++) {
            Val *t = v_list(ip); t->kind = V_TUPLE;
            v_push(ip, t, v_str(ip, v->ent[i].key, v->ent[i].klen));
            v_push(ip, t, v->ent[i].val);
            v_push(ip, out, t);
        }
    } else { rerror(ip, NULL, "value is not iterable"); }
    return out;
}

/* ======================================================================
 *  Registry build + lookups
 * ====================================================================== */
static void reg_push_type(const char *name, int len, const Node *decl) {
    if (REG.ntypes == REG.ctypes) { REG.ctypes = REG.ctypes ? REG.ctypes*2 : 16; REG.types = realloc(REG.types, (size_t)REG.ctypes*sizeof(TypeReg)); }
    REG.types[REG.ntypes++] = (TypeReg){ name, len, decl };
}
static const Node *find_type(const char *name, int len) {
    for (int i = 0; i < REG.ntypes; i++) if (REG.types[i].len == len && memcmp(REG.types[i].name, name, (size_t)len) == 0) return REG.types[i].decl;
    return NULL;
}
static void reg_push_var(const char *name, int len, const char *owner, const Node *v) {
    if (REG.nvars == REG.cvars) { REG.cvars = REG.cvars ? REG.cvars*2 : 16; REG.vars = realloc(REG.vars, (size_t)REG.cvars*sizeof(VarReg)); }
    REG.vars[REG.nvars++] = (VarReg){ name, len, owner, v };
}
static const VarReg *find_var(const char *name, int len) {
    for (int i = 0; i < REG.nvars; i++) if (REG.vars[i].len == len && memcmp(REG.vars[i].name, name, (size_t)len) == 0) return &REG.vars[i];
    return NULL;
}
static void reg_push_meth(const char *name, int len, const char *self_type, const char *trait, Val *fn) {
    if (REG.nmeths == REG.cmeths) { REG.cmeths = REG.cmeths ? REG.cmeths*2 : 16; REG.meths = realloc(REG.meths, (size_t)REG.cmeths*sizeof(MethReg)); }
    REG.meths[REG.nmeths++] = (MethReg){ name, len, self_type, trait, fn };
}
static void reg_push_def(const char *trait, const char *method, Val *fn) {
    if (REG.ndefs == REG.cdefs) { REG.cdefs = REG.cdefs ? REG.cdefs*2 : 16; REG.defs = realloc(REG.defs, (size_t)REG.cdefs*sizeof(DefReg)); }
    REG.defs[REG.ndefs++] = (DefReg){ trait, method, fn };
}
static void reg_push_impl(const char *type, const char *trait) {
    if (REG.nimpls == REG.cimpls) { REG.cimpls = REG.cimpls ? REG.cimpls*2 : 16; REG.impls = realloc(REG.impls, (size_t)REG.cimpls*sizeof(ImplReg)); }
    REG.impls[REG.nimpls++] = (ImplReg){ type, trait };
}
static bool type_impls_trait(const char *type, const char *trait) {
    if (!type || !trait) return false;
    for (int i = 0; i < REG.nimpls; i++) if (!strcmp(REG.impls[i].type, type) && !strcmp(REG.impls[i].trait, trait)) return true;
    return false;
}

/* find a method `name` for receiver type tname: concrete impl, then trait default */
static Val *find_method(const char *name, int len, const char *tname) {
    for (int i = 0; i < REG.nmeths; i++) {
        MethReg *m = &REG.meths[i];
        if (m->len == len && memcmp(m->name, name, (size_t)len) == 0 && m->self_type && tname && !strcmp(m->self_type, tname))
            return m->fn;
    }
    for (int i = 0; i < REG.ndefs; i++) {
        DefReg *d = &REG.defs[i];
        if ((int)strlen(d->method) == len && memcmp(d->method, name, (size_t)len) == 0 && type_impls_trait(tname, d->trait))
            return d->fn;
    }
    return NULL;
}

/* ======================================================================
 *  Function parts
 * ====================================================================== */
typedef struct { const Node *params[32]; int nparams; const Node *body; bool err_union; } FnParts;

static bool is_type_kind(NodeKind k) { return k >= N_TYNAME && k <= N_TYHOLE; }

static FnParts fn_parts(const Node *fn) {
    FnParts fp = {0}; bool seen_name = false;
    for (int i = 0; i < fn->nkid; i++) {
        const Node *k = fn->kids[i];
        switch (k->kind) {
        case N_DOC: case N_ATTR: case N_GENERICS: case N_EFFECTS: case N_FROM: break;
        case N_PARAM: if (fp.nparams < 32) fp.params[fp.nparams++] = k; break;
        case N_IDENT: if (!seen_name) seen_name = true; else fp.body = k; break;
        case N_BLOCK: fp.body = k; break;
        default:
            if (is_type_kind(k->kind)) { if (k->kind == N_TYERR) fp.err_union = true; }
            else fp.body = k;    /* expression body */
        }
    }
    return fp;
}

/* decimal scale declared by a param/field type, or -1 */
static int decimal_scale_of(const Node *type) {
    if (type && type->kind == N_TYNAME && text_is(type, "decimal") && type->nkid >= 2) {
        const Node *s = type->kids[1];
        if (s->kind == N_INT) return (int)parse_int(s->text, s->text_len);
    }
    return -1;
}

/* ======================================================================
 *  Field access
 * ====================================================================== */
static Val *field_get(Interp *ip, Val *recv, const char *name, int len, const Node *site) {
    if (recv->kind == V_RECORD || recv->kind == V_MAP || recv->kind == V_VARIANT) {
        for (int i = 0; i < recv->nent; i++)
            if (recv->ent[i].klen == len && memcmp(recv->ent[i].key, name, (size_t)len) == 0) return recv->ent[i].val;
    }
    if (recv->kind == V_VARIANT && recv->names)
        for (int i = 0; i < recv->n; i++)
            if (recv->names[i] && (int)strlen(recv->names[i]) == len && memcmp(recv->names[i], name, (size_t)len) == 0) return recv->items[i];
    if (recv->kind == V_TUPLE && len > 0 && name[0] >= '0' && name[0] <= '9') {
        int idx = (int)parse_int(name, len); if (idx >= 0 && idx < recv->n) return recv->items[idx];
    }
    rerror(ip, site, "no such field");
    return v_unit(ip);
}
static Entry *field_slot(Val *recv, const char *name, int len) {
    for (int i = 0; i < recv->nent; i++)
        if (recv->ent[i].klen == len && memcmp(recv->ent[i].key, name, (size_t)len) == 0) return &recv->ent[i];
    return NULL;
}

/* ======================================================================
 *  Builtins
 * ====================================================================== */
#define BI(name) static Val *name(Interp *ip, Val **a, int n, const Node *site)
BI(bi_len) { (void)site; if (!n) return v_int(ip, 0); Val *v = a[0];
    if (v->kind == V_LIST || v->kind == V_SET || v->kind == V_TUPLE) return v_int(ip, v->n);
    if (v->kind == V_MAP) return v_int(ip, v->nent);
    if (v->kind == V_STR) { Val *l = to_iter_list(ip, v); return v_int(ip, l->n); }
    if (v->kind == V_RANGE) return v_int(ip, (v->inc ? v->hi - v->lo + 1 : v->hi - v->lo));
    return v_int(ip, 0);
}
BI(bi_map) { Val *src = to_iter_list(ip, a[0]); Val *f = a[1]; Val *out = v_list(ip); (void)n; (void)site;
    for (int i = 0; i < src->n; i++) { Val *r = call_value(ip, f, &src->items[i], 1, site); if (ip->flow != FL_NORMAL) return r; v_push(ip, out, r); } return out; }
BI(bi_filter) { Val *src = to_iter_list(ip, a[0]); Val *f = a[1]; Val *out = v_list(ip); (void)n; (void)site;
    for (int i = 0; i < src->n; i++) { Val *r = call_value(ip, f, &src->items[i], 1, site); if (ip->flow != FL_NORMAL) return r; if (val_truthy(r)) v_push(ip, out, src->items[i]); } return out; }
BI(bi_each) { Val *src = to_iter_list(ip, a[0]); Val *f = a[1]; (void)n;
    for (int i = 0; i < src->n; i++) { call_value(ip, f, &src->items[i], 1, site); if (ip->flow != FL_NORMAL) return v_unit(ip); } return v_unit(ip); }
BI(bi_sum) { Val *src = to_iter_list(ip, a[0]); (void)n; (void)site; if (src->n == 0) return v_int(ip, 0);
    Val *acc = src->items[0]; for (int i = 1; i < src->n; i++) acc = arith(ip, "+", 1, acc, src->items[i], site); return acc; }
BI(bi_sorted) { Val *src = to_iter_list(ip, a[0]); (void)n; (void)site; Val *out = v_list(ip);
    for (int i = 0; i < src->n; i++) v_push(ip, out, src->items[i]);
    for (int i = 1; i < out->n; i++) { Val *key = out->items[i]; int j = i - 1;
        while (j >= 0 && cmp_vals(out->items[j], key) > 0) { out->items[j+1] = out->items[j]; j--; } out->items[j+1] = key; }
    return out; }
BI(bi_first) { Val *src = to_iter_list(ip, a[0]); (void)n; (void)site; return src->n ? v_opt_some(ip, src->items[0]) : v_opt_none(ip); }
BI(bi_last) { Val *src = to_iter_list(ip, a[0]); (void)n; (void)site; return src->n ? v_opt_some(ip, src->items[src->n-1]) : v_opt_none(ip); }
BI(bi_enumerate) { Val *src = to_iter_list(ip, a[0]); (void)n; (void)site; Val *out = v_list(ip);
    for (int i = 0; i < src->n; i++) { Val *t = v_list(ip); t->kind = V_TUPLE; v_push(ip, t, v_int(ip, i)); v_push(ip, t, src->items[i]); v_push(ip, out, t); } return out; }
BI(bi_push) { (void)n; (void)site; v_push(ip, a[0], a[1]); return v_unit(ip); }
BI(bi_pop) { (void)n; (void)site; Val *l = a[0]; if (l->n == 0) return v_opt_none(ip); Val *last = l->items[l->n - 1]; l->n -= 1; return v_opt_some(ip, last); }
BI(bi_keys) { (void)n; (void)site; Val *out = v_list(ip); if (a[0]->kind == V_MAP) for (int i = 0; i < a[0]->nent; i++) v_push(ip, out, v_str(ip, a[0]->ent[i].key, a[0]->ent[i].klen)); return out; }
BI(bi_values) { (void)n; (void)site; Val *out = v_list(ip); if (a[0]->kind == V_MAP) for (int i = 0; i < a[0]->nent; i++) v_push(ip, out, a[0]->ent[i].val); return out; }
BI(bi_str) { (void)n; (void)site; int l; char *s = val_to_cstr(ip, a[0], &l); return v_str_owned(ip, s, l); }
BI(bi_upper) { (void)n; (void)site; Val *v = a[0]; char *b = lume_rt_alloc(ip, (size_t)v->slen + 1); for (int i = 0; i < v->slen; i++) { char c = v->s[i]; b[i] = (c >= 'a' && c <= 'z') ? (char)(c - 32) : c; } b[v->slen] = 0; return v_str_owned(ip, b, v->slen); }
BI(bi_lower) { (void)n; (void)site; Val *v = a[0]; char *b = lume_rt_alloc(ip, (size_t)v->slen + 1); for (int i = 0; i < v->slen; i++) { char c = v->s[i]; b[i] = (c >= 'A' && c <= 'Z') ? (char)(c + 32) : c; } b[v->slen] = 0; return v_str_owned(ip, b, v->slen); }
BI(bi_trim) { (void)n; (void)site; Val *v = a[0]; int s = 0, e = v->slen; while (s < e && (v->s[s]==' '||v->s[s]=='\t'||v->s[s]=='\n'||v->s[s]=='\r')) s++; while (e > s && (v->s[e-1]==' '||v->s[e-1]=='\t'||v->s[e-1]=='\n'||v->s[e-1]=='\r')) e--; return v_str(ip, v->s + s, e - s); }
BI(bi_max) { (void)site; (void)ip; Val *m = a[0]; for (int i = 1; i < n; i++) if (cmp_vals(a[i], m) > 0) m = a[i]; return m; }
BI(bi_min) { (void)site; (void)ip; Val *m = a[0]; for (int i = 1; i < n; i++) if (cmp_vals(a[i], m) < 0) m = a[i]; return m; }
BI(bi_abs) { (void)n; (void)site; Val *v = a[0]; if (v->kind == V_INT) return v_int(ip, v->i < 0 ? -v->i : v->i); if (v->kind == V_FLOAT) return v_float(ip, fabs(v->f)); if (v->kind == V_DEC) return v_dec(ip, v->i < 0 ? -v->i : v->i, v->scale); return v; }
BI(bi_count) { (void)n; Val *src = to_iter_list(ip, a[0]); (void)site; return v_int(ip, src->n); }
BI(bi_println); BI(bi_print);
/* --- extended std: strings --- */
BI(bi_contains) { (void)n; (void)site; Val *c = a[0];
    if (c->kind == V_STR) { Val *sub = a[1]; if (sub->slen == 0) return v_bool(ip, true); for (int i = 0; i + sub->slen <= c->slen; i++) if (memcmp(c->s + i, sub->s, (size_t)sub->slen) == 0) return v_bool(ip, true); return v_bool(ip, false); }
    if (c->kind == V_MAP) { int kl; char *ks = val_to_cstr(ip, a[1], &kl); for (int i = 0; i < c->nent; i++) if (c->ent[i].klen == kl && memcmp(c->ent[i].key, ks, (size_t)kl) == 0) return v_bool(ip, true); return v_bool(ip, false); }
    Val *lst = to_iter_list(ip, c); for (int i = 0; i < lst->n; i++) if (val_equal(lst->items[i], a[1])) return v_bool(ip, true); return v_bool(ip, false); }
BI(bi_starts_with) { (void)n; (void)site; Val *s = a[0], *p = a[1]; return v_bool(ip, p->slen <= s->slen && memcmp(s->s, p->s, (size_t)p->slen) == 0); }
BI(bi_ends_with) { (void)n; (void)site; Val *s = a[0], *p = a[1]; return v_bool(ip, p->slen <= s->slen && memcmp(s->s + (s->slen - p->slen), p->s, (size_t)p->slen) == 0); }
BI(bi_index_of) { (void)n; (void)site; Val *c = a[0];
    if (c->kind == V_STR) { Val *sub = a[1]; for (int i = 0; i + sub->slen <= c->slen; i++) if (memcmp(c->s + i, sub->s, (size_t)sub->slen) == 0) return v_int(ip, i); return v_int(ip, -1); }
    Val *lst = to_iter_list(ip, c); for (int i = 0; i < lst->n; i++) if (val_equal(lst->items[i], a[1])) return v_int(ip, i); return v_int(ip, -1); }
BI(bi_split) { (void)n; (void)site; Val *s = a[0], *sep = a[1]; Val *out = v_list(ip);
    if (sep->slen == 0) { for (int i = 0; i < s->slen; i++) v_push(ip, out, v_str(ip, s->s + i, 1)); return out; }
    int start = 0; for (int i = 0; i + sep->slen <= s->slen; ) { if (memcmp(s->s + i, sep->s, (size_t)sep->slen) == 0) { v_push(ip, out, v_str(ip, s->s + start, i - start)); i += sep->slen; start = i; } else i++; }
    v_push(ip, out, v_str(ip, s->s + start, s->slen - start)); return out; }
BI(bi_replace) { (void)n; (void)site; Val *s = a[0], *from = a[1], *to = a[2]; if (from->slen == 0) return s;
    char *buf = lume_rt_alloc(ip, (size_t)s->slen * (to->slen > from->slen ? (size_t)(to->slen) : 1) + (size_t)s->slen + 1); int w = 0;
    for (int i = 0; i < s->slen; ) { if (i + from->slen <= s->slen && memcmp(s->s + i, from->s, (size_t)from->slen) == 0) { memcpy(buf + w, to->s, (size_t)to->slen); w += to->slen; i += from->slen; } else buf[w++] = s->s[i++]; }
    buf[w] = 0; return v_str_owned(ip, buf, w); }
BI(bi_repeat) { (void)n; (void)site; Val *s = a[0]; long long cnt = a[1]->i; if (cnt < 0) cnt = 0; char *buf = lume_rt_alloc(ip, (size_t)(s->slen) * (size_t)cnt + 1); int w = 0; for (long long k = 0; k < cnt; k++) { memcpy(buf + w, s->s, (size_t)s->slen); w += s->slen; } buf[w] = 0; return v_str_owned(ip, buf, w); }
BI(bi_chars) { (void)n; (void)site; return to_iter_list(ip, a[0]); }
BI(bi_join) { (void)n; (void)site; Val *lst = to_iter_list(ip, a[0]); Val *sep = n >= 2 ? a[1] : NULL;
    int cap = 64, w = 0; char *buf = lume_rt_alloc(ip, (size_t)cap);
    for (int i = 0; i < lst->n; i++) { if (i && sep) { while (w + sep->slen + 1 > cap) { cap *= 2; char *nb = lume_rt_alloc(ip, (size_t)cap); memcpy(nb, buf, (size_t)w); buf = nb; } memcpy(buf + w, sep->s, (size_t)sep->slen); w += sep->slen; }
        int el; char *es = val_to_cstr(ip, lst->items[i], &el); while (w + el + 1 > cap) { cap *= 2; char *nb = lume_rt_alloc(ip, (size_t)cap); memcpy(nb, buf, (size_t)w); buf = nb; } memcpy(buf + w, es, (size_t)el); w += el; }
    buf[w] = 0; return v_str_owned(ip, buf, w); }
BI(bi_parse_int) { (void)n; (void)site; Val *s = a[0]; if (s->kind != V_STR || s->slen == 0) return v_opt_none(ip); long long v = 0; int i = 0; bool neg = false; if (s->s[0] == '-') { neg = true; i = 1; } if (i >= s->slen) return v_opt_none(ip); for (; i < s->slen; i++) { char c = s->s[i]; if (c < '0' || c > '9') return v_opt_none(ip); v = v * 10 + (c - '0'); } return v_opt_some(ip, v_int(ip, neg ? -v : v)); }
/* --- extended std: lists --- */
BI(bi_reverse) { (void)n; (void)site; Val *src = to_iter_list(ip, a[0]);
    if (a[0]->kind == V_STR) { char *buf = lume_rt_alloc(ip, (size_t)src->n * 4 + 1); int w = 0; for (int i = src->n - 1; i >= 0; i--) { int cl; char *cs = val_to_cstr(ip, src->items[i], &cl); memcpy(buf + w, cs, (size_t)cl); w += cl; } buf[w] = 0; return v_str_owned(ip, buf, w); }
    Val *out = v_list(ip); for (int i = src->n - 1; i >= 0; i--) v_push(ip, out, src->items[i]); return out; }
BI(bi_take) { (void)n; (void)site; Val *src = to_iter_list(ip, a[0]); long long k = a[1]->i; Val *out = v_list(ip); for (int i = 0; i < src->n && i < k; i++) v_push(ip, out, src->items[i]); return out; }
BI(bi_drop) { (void)n; (void)site; Val *src = to_iter_list(ip, a[0]); long long k = a[1]->i; Val *out = v_list(ip); for (int i = (int)k; i < src->n; i++) if (i >= 0) v_push(ip, out, src->items[i]); return out; }
BI(bi_reduce) { (void)n; Val *src = to_iter_list(ip, a[0]); Val *acc = a[1]; Val *f = a[2]; for (int i = 0; i < src->n; i++) { Val *args2[2] = { acc, src->items[i] }; acc = call_value(ip, f, args2, 2, site); if (ip->flow != FL_NORMAL) return acc; } return acc; }
BI(bi_any) { (void)n; Val *src = to_iter_list(ip, a[0]); Val *f = a[1]; for (int i = 0; i < src->n; i++) { Val *r = call_value(ip, f, &src->items[i], 1, site); if (ip->flow != FL_NORMAL) return r; if (val_truthy(r)) return v_bool(ip, true); } return v_bool(ip, false); }
BI(bi_all) { (void)n; Val *src = to_iter_list(ip, a[0]); Val *f = a[1]; for (int i = 0; i < src->n; i++) { Val *r = call_value(ip, f, &src->items[i], 1, site); if (ip->flow != FL_NORMAL) return r; if (!val_truthy(r)) return v_bool(ip, false); } return v_bool(ip, true); }
BI(bi_zip) { (void)n; (void)site; Val *x = to_iter_list(ip, a[0]); Val *y = to_iter_list(ip, a[1]); Val *out = v_list(ip); int m = x->n < y->n ? x->n : y->n; for (int i = 0; i < m; i++) { Val *t = v_list(ip); t->kind = V_TUPLE; v_push(ip, t, x->items[i]); v_push(ip, t, y->items[i]); v_push(ip, out, t); } return out; }
BI(bi_concat) { (void)site; Val *out = v_list(ip); for (int k = 0; k < n; k++) { Val *s = to_iter_list(ip, a[k]); for (int i = 0; i < s->n; i++) v_push(ip, out, s->items[i]); } return out; }
BI(bi_range) { (void)site; Val *v = v_unit(ip); v->kind = V_RANGE; v->lo = n >= 2 ? a[0]->i : 0; v->hi = n >= 2 ? a[1]->i : a[0]->i; v->inc = false; return to_iter_list(ip, v); }
/* --- extended std: maps --- */
BI(bi_get) { (void)n; (void)site; Val *m = a[0]; int kl; char *ks = val_to_cstr(ip, a[1], &kl); for (int i = 0; i < m->nent; i++) if (m->ent[i].klen == kl && memcmp(m->ent[i].key, ks, (size_t)kl) == 0) return v_opt_some(ip, m->ent[i].val); return v_opt_none(ip); }
BI(bi_has) { (void)n; (void)site; Val *m = a[0]; int kl; char *ks = val_to_cstr(ip, a[1], &kl); for (int i = 0; i < m->nent; i++) if (m->ent[i].klen == kl && memcmp(m->ent[i].key, ks, (size_t)kl) == 0) return v_bool(ip, true); return v_bool(ip, false); }
BI(bi_entries) { (void)n; (void)site; Val *m = a[0]; Val *out = v_list(ip); for (int i = 0; i < m->nent; i++) { Val *t = v_list(ip); t->kind = V_TUPLE; v_push(ip, t, v_str(ip, m->ent[i].key, m->ent[i].klen)); v_push(ip, t, m->ent[i].val); v_push(ip, out, t); } return out; }
/* --- extended std: io --- */
BI(bi_eprintln) { (void)site; for (int i = 0; i < n; i++) { if (i) fputc(' ', stderr); int l; char *s = val_to_cstr(ip, a[i], &l); fwrite(s, 1, (size_t)l, stderr); } fputc('\n', stderr); return v_unit(ip); }
/* --- math --- */
BI(bi_sqrt) { (void)n; (void)site; return v_float(ip, sqrt(as_double(a[0]))); }
BI(bi_floor) { (void)n; (void)site; return v_int(ip, (long long)floor(as_double(a[0]))); }
BI(bi_ceil) { (void)n; (void)site; return v_int(ip, (long long)ceil(as_double(a[0]))); }
BI(bi_round) { (void)n; (void)site; return v_int(ip, llround(as_double(a[0]))); }
BI(bi_pow) { (void)n; (void)site; return v_float(ip, pow(as_double(a[0]), as_double(a[1]))); }


static Val *mk_builtin(Interp *ip, const char *name, Val *(*fn)(Interp *, Val **, int, const Node *)) {
    Val *v = v_unit(ip); v->kind = V_BUILTIN; v->builtin = fn; v->bname = name; return v;
}
static Val *lookup_builtin(Interp *ip, const char *name, int len) {
    struct { const char *n; Val *(*f)(Interp *, Val **, int, const Node *); } T[] = {
        {"len",bi_len},{"map",bi_map},{"filter",bi_filter},{"each",bi_each},{"sum",bi_sum},
        {"sorted",bi_sorted},{"first",bi_first},{"last",bi_last},{"enumerate",bi_enumerate},
        {"push",bi_push},{"pop",bi_pop},{"keys",bi_keys},{"values",bi_values},{"str",bi_str},{"upper",bi_upper},
        {"lower",bi_lower},{"trim",bi_trim},{"max",bi_max},{"min",bi_min},{"abs",bi_abs},{"count",bi_count},
        {"contains",bi_contains},{"starts_with",bi_starts_with},{"ends_with",bi_ends_with},
        {"index_of",bi_index_of},{"split",bi_split},{"replace",bi_replace},{"repeat",bi_repeat},
        {"chars",bi_chars},{"join",bi_join},{"parse_int",bi_parse_int},{"reverse",bi_reverse},
        {"take",bi_take},{"drop",bi_drop},{"reduce",bi_reduce},{"fold",bi_reduce},{"any",bi_any},
        {"all",bi_all},{"zip",bi_zip},{"concat",bi_concat},{"range",bi_range},
        {"get",bi_get},{"has",bi_has},{"entries",bi_entries},
        {"sqrt",bi_sqrt},{"floor",bi_floor},{"ceil",bi_ceil},{"round",bi_round},{"pow",bi_pow},
    };
    for (size_t i = 0; i < sizeof T / sizeof T[0]; i++)
        if ((int)strlen(T[i].n) == len && memcmp(T[i].n, name, (size_t)len) == 0) return mk_builtin(ip, T[i].n, T[i].f);
    return NULL;
}

/* ======================================================================
 *  println/print, deep copy, resolve, forward decls
 * ====================================================================== */
/* program args + file IO, for a file-driven lumec (set by main before lume_run) */
static int g_argc = 0;
static char **g_argv = NULL;
void lume_set_args(int argc, char **argv) { g_argc = argc; g_argv = argv; }

static Val *bi_args(Interp *ip, Val **a, int n, const Node *site) { (void)a; (void)n; (void)site; Val *out = v_list(ip); for (int i = 0; i < g_argc; i++) v_push(ip, out, v_str(ip, g_argv[i], (int)strlen(g_argv[i]))); return out; }
static Val *bi_read_file(Interp *ip, Val **a, int n, const Node *site) { (void)n; (void)site; size_t len = 0; char *s = read_file(a[0]->s, &len); if (!s) return v_str(ip, "", 0); Val *v = v_str(ip, s, (int)len); free(s); return v; }
static Val *bi_write_file(Interp *ip, Val **a, int n, const Node *site) { (void)n; (void)site; FILE *f = fopen(a[0]->s, "wb"); if (!f) return v_bool(ip, false); if (a[1]->slen) fwrite(a[1]->s, 1, (size_t)a[1]->slen, f); fclose(f); return v_bool(ip, true); }

BI(bi_println) { (void)site; for (int i = 0; i < n; i++) { if (i) fputc(' ', stdout); int l; char *s = val_to_cstr(ip, a[i], &l); fwrite(s, 1, (size_t)l, stdout); } fputc('\n', stdout); return v_unit(ip); }
BI(bi_print)   { (void)site; for (int i = 0; i < n; i++) { if (i) fputc(' ', stdout); int l; char *s = val_to_cstr(ip, a[i], &l); fwrite(s, 1, (size_t)l, stdout); } return v_unit(ip); }

static Val *deep_copy(Interp *ip, Val *v) {
    switch (v->kind) {
    case V_RECORD: case V_MAP: { Val *c = v_unit(ip); *c = *v; c->ent = lume_rt_alloc(ip, (size_t)(v->nent ? v->nent : 1) * sizeof(Entry));
        for (int i = 0; i < v->nent; i++) { c->ent[i] = v->ent[i]; c->ent[i].val = deep_copy(ip, v->ent[i].val); } c->cent = v->nent; return c; }
    case V_LIST: case V_SET: case V_TUPLE: case V_VARIANT: { Val *c = v_unit(ip); *c = *v; c->items = lume_rt_alloc(ip, (size_t)(v->n ? v->n : 1) * sizeof(Val *));
        for (int i = 0; i < v->n; i++) c->items[i] = deep_copy(ip, v->items[i]);
        c->cap = v->n; return c; }
    default: { Val *c = v_unit(ip); *c = *v; return c; }
    }
}

static Val *run_user_fn(Interp *ip, Val *fv, Val **args, int nargs, const Node *site);
static Val *construct_record(Interp *ip, const char *tname, const Node *decl, Val **vals, char **names, int *nlens, int nargs, const Node *site);
static Val *construct_variant(Interp *ip, const VarReg *vr, const char *name, int len, Val **vals, int nargs, const Node *site);
static Val *eval_call(Interp *ip, const Node *call, Env *env, Val *prepend);

static Val *resolve_ident(Interp *ip, const Node *id, Env *env) {
    Val *v = env_get(env, id->text, id->text_len);
    if (v) return v;
    Val *b = lookup_builtin(ip, id->text, id->text_len);
    if (b) return b;
    const VarReg *vr = find_var(id->text, id->text_len);   /* nullary variant */
    if (vr) return construct_variant(ip, vr, id->text, id->text_len, NULL, 0, id);
    rerror(ip, id, "undefined name");
    return v_unit(ip);
}

/* ======================================================================
 *  call_value / run_user_fn
 * ====================================================================== */
static Val *call_value(Interp *ip, Val *fn, Val **args, int nargs, const Node *site) {
    if (!fn) { rerror(ip, site, "calling a null value"); return v_unit(ip); }
    if (fn->kind == V_BUILTIN) return fn->builtin(ip, args, nargs, site);
    if (fn->kind == V_FIELDACC) { if (nargs < 1) { rerror(ip, site, "field accessor needs an argument"); return v_unit(ip); } return field_get(ip, args[0], fn->facc, fn->facc_len, site); }
    if (fn->kind == V_CLOSURE) {
        if (fn->is_closure_fn) return run_user_fn(ip, fn, args, nargs, site);
        /* lambda: N_CLOSURE kids = params (N_CLOSPARAM) ... then body (last) */
        const Node *c = fn->cl_params_owner;
        Env *frame = env_new(ip, fn->cl_env);
        int pi = 0;
        for (int i = 0; i < c->nkid - 1; i++) {
            const Node *cp = c->kids[i];
            if (cp->kind != N_CLOSPARAM) continue;
            const Node *patn = cp->nkid ? cp->kids[0] : NULL;
            Val *av = pi < nargs ? args[pi] : v_unit(ip); pi++;
            if (patn) pattern_match(ip, patn, av, frame);
        }
        const Node *body = c->kids[c->nkid - 1];
        Val *r = (body->kind == N_BLOCK) ? eval_block(ip, body, frame) : eval(ip, body, frame);
        return r;
    }
    rerror(ip, site, "value is not callable");
    return v_unit(ip);
}

static Val *run_user_fn(Interp *ip, Val *fv, Val **args, int nargs, const Node *site) {
    (void)site;
    FnParts fp = fn_parts(fv->cl_params_owner);
    Env *frame = env_new(ip, fv->cl_env);
    Env *snap  = env_new(ip, fv->cl_env);
    int ai = 0;
    for (int i = 0; i < fp.nparams; i++) {
        const Node *p = fp.params[i];
        const char *pname; int plen; const Node *ptype = NULL;
        if (p->text) { pname = p->text; plen = p->text_len; }           /* self */
        else { const Node *nn = p->kids[0]; pname = nn->text; plen = nn->text_len; if (p->nkid >= 2 && is_type_kind(p->kids[1]->kind)) ptype = p->kids[1]; }
        Val *v = ai < nargs ? args[ai] : v_unit(ip); ai++;
        int sc = decimal_scale_of(ptype); if (sc >= 0) v = coerce_decimal(ip, v, sc);
        env_define(ip, frame, pname, plen, v);
        env_define(ip, snap, pname, plen, deep_copy(ip, v));
    }
    Val *result = v_unit(ip);
    if (fp.body && fp.body->kind == N_BLOCK) {
        const Node *b = fp.body;
        const Node *ensures[32]; int nens = 0;
        const Node *defers[32]; int ndf = 0; const Node *errdefers[32]; int nedf = 0;
        for (int i = 0; i < b->nkid; i++) {
            const Node *st = b->kids[i];
            if (st->kind == N_ENSURES) { if (nens < 32) ensures[nens++] = st; continue; }
            if (st->kind == N_DEFER) { if (ndf < 32) defers[ndf++] = st->kids[0]; continue; }
            if (st->kind == N_ERRDEFER) { if (nedf < 32) errdefers[nedf++] = st->kids[0]; continue; }
            result = eval(ip, st, frame);
            if (ip->flow != FL_NORMAL) break;
        }
        Flow exitflow = ip->flow;
        if (exitflow == FL_FAIL) { Flow sf = ip->flow; Val *sg = ip->signal; ip->flow = FL_NORMAL;
            for (int i = nedf - 1; i >= 0; i--) { eval(ip, errdefers[i], frame); ip->flow = FL_NORMAL; }
            ip->flow = sf; ip->signal = sg; }
        { Flow sf = ip->flow; Val *sg = ip->signal; ip->flow = FL_NORMAL;      /* defers run on every exit */
          for (int i = ndf - 1; i >= 0; i--) { eval(ip, defers[i], frame); ip->flow = FL_NORMAL; }
          ip->flow = sf; ip->signal = sg; }
        if (ip->flow == FL_RETURN) { result = ip->signal; ip->flow = FL_NORMAL; }
        else if (ip->flow == FL_FAIL) { Val *e = ip->signal; ip->flow = FL_NORMAL; return fp.err_union ? v_err(ip, e) : e; }
        else if (ip->flow == FL_PANIC) { return v_unit(ip); }
        for (int i = 0; i < nens; i++) {                 /* postconditions on success */
            Env *cenv = env_new(ip, frame);
            env_define(ip, cenv, "result", 6, result);
            ip->old_env = snap;
            Val *ok = eval(ip, ensures[i]->kids[0], cenv);
            ip->old_env = NULL;
            if (ip->flow == FL_NORMAL && !val_truthy(ok)) {
                rerror(ip, ensures[i], "postcondition (ensures) violated");
                return v_unit(ip);
            }
        }
    } else if (fp.body) {
        result = eval(ip, fp.body, frame);
        if (ip->flow == FL_FAIL) { Val *e = ip->signal; ip->flow = FL_NORMAL; return fp.err_union ? v_err(ip, e) : e; }
        if (ip->flow == FL_RETURN) { result = ip->signal; ip->flow = FL_NORMAL; }
    }
    if (ip->flow == FL_PANIC) return v_unit(ip);
    return fp.err_union ? v_ok(ip, result) : result;
}

/* ======================================================================
 *  constructors
 * ====================================================================== */
static const Node *record_node_of(const Node *typedecl) {
    for (int i = 0; i < typedecl->nkid; i++) if (typedecl->kids[i]->kind == N_RECORD) return typedecl->kids[i];
    return NULL;
}
static Val *construct_record(Interp *ip, const char *tname, const Node *decl, Val **vals, char **names, int *nlens, int nargs, const Node *site) {
    const Node *rec = record_node_of(decl);
    if (!rec) { rerror(ip, site, "type is not a record"); return v_unit(ip); }
    Val *v = v_unit(ip); v->kind = V_RECORD; v->tname = tname;
    int nf = 0; for (int i = 0; i < rec->nkid; i++) if (rec->kids[i]->kind == N_FIELD) nf++;
    v->ent = lume_rt_alloc(ip, (size_t)(nf ? nf : 1) * sizeof(Entry)); v->cent = nf;
    int fieldpos = 0;
    for (int i = 0; i < rec->nkid; i++) {
        const Node *f = rec->kids[i]; if (f->kind != N_FIELD) continue;
        const Node *fname = f->kids[0];
        const Node *ftype = (f->nkid >= 2 && is_type_kind(f->kids[1]->kind)) ? f->kids[1] : NULL;
        const Node *fdef  = NULL;
        for (int j = 1; j < f->nkid; j++) if (!is_type_kind(f->kids[j]->kind)) fdef = f->kids[j];
        Val *val = NULL;
        for (int k = 0; k < nargs; k++)                          /* named arg */
            if (names && names[k] && nlens[k] == fname->text_len && memcmp(names[k], fname->text, (size_t)nlens[k]) == 0) { val = vals[k]; break; }
        if (!val && (!names || !names[fieldpos]) && fieldpos < nargs) val = vals[fieldpos];  /* positional */
        if (!val && fdef) val = eval(ip, fdef, ip->globals);
        if (!val) val = v_unit(ip);
        int sc = decimal_scale_of(ftype); if (sc >= 0) val = coerce_decimal(ip, val, sc);
        Entry *e = &v->ent[v->nent++];
        e->key = dup_text(ip, fname); e->klen = fname->text_len; e->val = val;
        fieldpos++;
    }
    return v;
}
static Val *construct_variant(Interp *ip, const VarReg *vr, const char *name, int len, Val **vals, int nargs, const Node *site) {
    (void)site;
    Val *v = v_unit(ip); v->kind = V_VARIANT;
    v->tag = vr ? vr->name : ntz(ip, name, len); v->tname = vr ? vr->owner : NULL;
    if (nargs) {
        v->items = lume_rt_alloc(ip, (size_t)nargs * sizeof(Val *));
        v->names = lume_rt_alloc(ip, (size_t)nargs * sizeof(char *));
        const Node *variant = vr ? vr->variant : NULL;
        int fi = 0;
        for (int i = 0; i < nargs; i++) {
            Val *val = vals[i]; const char *fn_name = NULL;
            if (variant) { for (int j = fi; j < variant->nkid; j++) if (variant->kids[j]->kind == N_FIELD) { const Node *f = variant->kids[j]; fn_name = dup_text(ip, f->kids[0]); const Node *ft = f->nkid>=2?f->kids[1]:NULL; int sc = decimal_scale_of(ft); if (sc>=0) val = coerce_decimal(ip, val, sc); fi = j+1; break; } }
            v->items[v->n] = val; v->names[v->n] = (char*)fn_name; v->n++;
        }
    }
    return v;
}

/* ======================================================================
 *  patterns
 * ====================================================================== */
static Val *lit_value(Interp *ip, const Node *n) {
    const char *t = n->text; int l = n->text_len;
    if (l >= 1 && (t[0] == '"')) return eval_string(ip, n, ip->globals, false, false);
    if (l == 4 && memcmp(t, "true", 4) == 0) return v_bool(ip, true);
    if (l == 5 && memcmp(t, "false", 5) == 0) return v_bool(ip, false);
    bool isfloat = false; for (int i = 0; i < l; i++) if (t[i] == '.' || t[i] == 'e' || t[i] == 'E') isfloat = true;
    if (isfloat) return v_float(ip, parse_float(t, l));
    return v_int(ip, parse_int(t, l));
}
static bool pattern_match(Interp *ip, const Node *pat, Val *v, Env *env) {
    switch (pat->kind) {
    case N_PWILD: return true;
    case N_PBIND: env_define(ip, env, pat->text, pat->text_len, v); return true;
    case N_PLIT: { Val *lv = lit_value(ip, pat); return val_equal(lv, v); }
    case N_PRANGE: { if (!is_num(v) && v->kind != V_CHAR) return false; Val *lo = lit_value(ip, pat->kids[0]); Val *hi = lit_value(ip, pat->kids[1]);
        int a = cmp_vals(v, lo), b = cmp_vals(v, hi); return a >= 0 && (pat->flags & F_INCLUSIVE ? b <= 0 : b < 0); }
    case N_PNONE: return v->kind == V_OPT && !v->b;
    case N_POR: for (int i = 0; i < pat->nkid; i++) if (pattern_match(ip, pat->kids[i], v, env)) return true; return false;
    case N_PAT: return pattern_match(ip, pat->kids[0], v, env) && pattern_match(ip, pat->kids[1], v, env);
    case N_PIS: return true;   /* Stage 0: no runtime type check */
    case N_PTUPLE: { if (v->kind != V_TUPLE || v->n != pat->nkid) return false; for (int i = 0; i < pat->nkid; i++) if (!pattern_match(ip, pat->kids[i], v->items[i], env)) return false; return true; }
    case N_PLIST: { if (v->kind != V_LIST && v->kind != V_SET) return false;
        /* support a trailing ..rest */
        int fixed = 0; bool rest = false; const Node *restpat = NULL;
        for (int i = 0; i < pat->nkid; i++) { if (pat->kids[i]->kind == N_PREST) { rest = true; restpat = pat->kids[i]; } else fixed++; }
        if (!rest && v->n != pat->nkid) return false;
        if (rest && v->n < fixed) return false;
        int vi = 0; for (int i = 0; i < pat->nkid; i++) { const Node *sp = pat->kids[i];
            if (sp->kind == N_PREST) { int take = v->n - fixed; Val *r = v_list(ip); for (int k = 0; k < take; k++) v_push(ip, r, v->items[vi++]); if (restpat->nkid) env_define(ip, env, restpat->kids[0]->text, restpat->kids[0]->text_len, r); }
            else { if (!pattern_match(ip, sp, v->items[vi++], env)) return false; } }
        return true; }
    case N_PVARIANT: {
        const char *name = pat->text; int len = pat->text_len;
        if (len == 4 && memcmp(name, "some", 4) == 0) return v->kind == V_OPT && v->b && (pat->nkid == 0 || pattern_match(ip, pat->kids[0], v->inner, env));
        if (len == 2 && memcmp(name, "ok", 2) == 0) return v->kind == V_RESULT && v->b && (pat->nkid == 0 || pattern_match(ip, pat->kids[0], v->inner, env));
        if (len == 3 && memcmp(name, "err", 3) == 0) return v->kind == V_RESULT && !v->b && (pat->nkid == 0 || pattern_match(ip, pat->kids[0], v->inner, env));
        if (v->kind != V_VARIANT) return false;
        if (!v->tag || (int)strlen(v->tag) != len || memcmp(v->tag, name, (size_t)len) != 0) return false;
        int pos = 0;
        for (int i = 0; i < pat->nkid; i++) {
            const Node *sp = pat->kids[i];
            if (sp->kind == N_PFIELD) { const char *fn = sp->kids[0]->text; int fl = sp->kids[0]->text_len;
                Val *fv = NULL; for (int k = 0; k < v->n; k++) if (v->names && v->names[k] && (int)strlen(v->names[k]) == fl && memcmp(v->names[k], fn, (size_t)fl) == 0) fv = v->items[k];
                if (!fv || !pattern_match(ip, sp->kids[1], fv, env)) return false; }
            else { if (pos >= v->n || !pattern_match(ip, sp, v->items[pos++], env)) return false; }
        }
        return true; }
    default: return false;
    }
}

/* ======================================================================
 *  eval_call
 * ====================================================================== */
static Val *eval_call(Interp *ip, const Node *call, Env *env, Val *prepend) {
    const Node *callee = call->kids[0];
    /* old() in contracts */
    if (!prepend && callee->kind == N_IDENT && text_is(callee, "old") && call->nkid >= 2)
        return eval(ip, call->kids[1]->kids[call->kids[1]->nkid - 1], ip->old_env ? ip->old_env : env);

    Val *vals[64]; char *names[64]; int nlens[64]; int na = 0;
    if (prepend) { vals[na] = prepend; names[na] = NULL; nlens[na] = 0; na++; }
    for (int i = 1; i < call->nkid && na < 64; i++) {
        const Node *arg = call->kids[i]; if (arg->kind != N_ARG) continue;
        const Node *expr; const char *nm = NULL; int nl = 0;
        if (arg->nkid == 2 && arg->kids[0]->kind == N_IDENT) { nm = arg->kids[0]->text; nl = arg->kids[0]->text_len; expr = arg->kids[1]; }
        else expr = arg->kids[arg->nkid - 1];
        Val *av = eval(ip, expr, env); if (ip->flow != FL_NORMAL) return av;
        vals[na] = av; names[na] = (char *)nm; nlens[na] = nl; na++;
    }

    if (callee->kind == N_FIELD_GET) {
        const Node *recvn = callee->kids[0]; const char *m = callee->text; int ml = callee->text_len;
        bool recv_bound = recvn->kind == N_IDENT && env_get(env, recvn->text, recvn->text_len) != NULL;
        if (!recv_bound && recvn->kind == N_IDENT && text_is(recvn, "io")) {
            if (ml == 7 && memcmp(m, "println", 7) == 0) return bi_println(ip, vals, na, call);
            if (ml == 5 && memcmp(m, "print", 5) == 0) return bi_print(ip, vals, na, call);
            if (ml == 8 && memcmp(m, "eprintln", 8) == 0) return bi_eprintln(ip, vals, na, call);
            if (ml == 9 && memcmp(m, "read_file", 9) == 0) return bi_read_file(ip, vals, na, call);
            if (ml == 10 && memcmp(m, "write_file", 10) == 0) return bi_write_file(ip, vals, na, call);
            rerror(ip, call, "unknown io function"); return v_unit(ip);
        }
        if (!recv_bound && recvn->kind == N_IDENT && text_is(recvn, "env")) {
            if (ml == 4 && memcmp(m, "args", 4) == 0) return bi_args(ip, vals, na, call);
            rerror(ip, call, "unknown env function"); return v_unit(ip);
        }
        if (!recv_bound && recvn->kind == N_IDENT && text_is(recvn, "math")) {
            Val *mf = lookup_builtin(ip, m, ml);
            if (mf) return mf->builtin(ip, vals, na, call);
            rerror(ip, call, "unknown math function"); return v_unit(ip);
        }
        if (recvn->kind == N_IDENT && find_type(recvn->text, recvn->text_len)) {
            const VarReg *vr = find_var(m, ml); return construct_variant(ip, vr, m, ml, vals, na, call);
        }
        Val *rv = eval(ip, recvn, env); if (ip->flow != FL_NORMAL) return rv;
        Val *args2[64]; int n2 = 0; args2[n2++] = rv; for (int i = 0; i < na; i++) args2[n2++] = vals[i];
        Val *meth = find_method(m, ml, rv->tname);
        if (meth) return call_value(ip, meth, args2, n2, call);
        Val *f = env_get(ip->globals, m, ml); if (!f) f = lookup_builtin(ip, m, ml);
        if (f) return call_value(ip, f, args2, n2, call);
        rerror(ip, call, "no such method or function"); return v_unit(ip);
    }
    if (callee->kind == N_OPTCHAIN) {
        const Node *recvn = callee->kids[0]; const Node *fieldn = callee->kids[1];
        Val *rv = eval(ip, recvn, env); if (ip->flow != FL_NORMAL) return rv;
        if (rv->kind == V_OPT && !rv->b) return v_opt_none(ip);
        Val *base = (rv->kind == V_OPT) ? rv->inner : rv;
        Val *args2[64]; int n2 = 0; args2[n2++] = base; for (int i = 0; i < na; i++) args2[n2++] = vals[i];
        Val *meth = find_method(fieldn->text, fieldn->text_len, base->tname);
        Val *f = meth ? meth : env_get(ip->globals, fieldn->text, fieldn->text_len);
        if (!f) f = lookup_builtin(ip, fieldn->text, fieldn->text_len);
        if (!f) { rerror(ip, call, "no such method in optional chain"); return v_unit(ip); }
        Val *r = call_value(ip, f, args2, n2, call);
        return v_opt_some(ip, r);
    }
    if (callee->kind == N_IDENT) {
        const Node *decl = find_type(callee->text, callee->text_len);
        if (decl) return construct_record(ip, dup_text(ip, callee), decl, vals, names, nlens, na, call);
        const VarReg *vr = find_var(callee->text, callee->text_len);
        if (vr) return construct_variant(ip, vr, callee->text, callee->text_len, vals, na, call);
        Val *f = env_get(env, callee->text, callee->text_len);
        if (!f) f = lookup_builtin(ip, callee->text, callee->text_len);
        if (!f) { rerror(ip, callee, "call to undefined function"); return v_unit(ip); }
        return call_value(ip, f, vals, na, call);
    }
    Val *f = eval(ip, callee, env); if (ip->flow != FL_NORMAL) return f;
    return call_value(ip, f, vals, na, call);
}

/* ======================================================================
 *  eval_block (with defer)
 * ====================================================================== */
static Val *eval_block(Interp *ip, const Node *b, Env *parent) {
    Env *env = env_new(ip, parent);
    const Node *defers[64]; int nd = 0;
    Val *last = v_unit(ip);
    for (int i = 0; i < b->nkid; i++) {
        const Node *st = b->kids[i];
        if (st->kind == N_DEFER) { if (nd < 64) defers[nd++] = st->kids[0]; continue; }
        last = eval(ip, st, env);
        if (ip->flow != FL_NORMAL) break;
    }
    if (nd) { Flow sf = ip->flow; Val *ss = ip->signal; ip->flow = FL_NORMAL;
        for (int i = nd - 1; i >= 0; i--) { eval(ip, defers[i], env); ip->flow = FL_NORMAL; }
        ip->flow = sf; ip->signal = ss; }
    return last;
}

/* ======================================================================
 *  eval
 * ====================================================================== */
static Val *eval(Interp *ip, const Node *n, Env *env) {
    if (ip->flow != FL_NORMAL) return v_unit(ip);
    switch (n->kind) {
    case N_INT:   return v_int(ip, parse_int(n->text, n->text_len));
    case N_FLOAT: return v_float(ip, parse_float(n->text, n->text_len));
    case N_STR:   return eval_string(ip, n, env, true, false);
    case N_RAWSTR: return eval_string(ip, n, env, false, true);
    case N_BYTESTR: return eval_string(ip, n, env, false, false);
    case N_REGEX: return eval_string(ip, n, env, false, true);
    case N_BOOL:  return v_bool(ip, n->text_len == 4);
    case N_NONE:  return v_opt_none(ip);
    case N_CHAR:  { /* 'a' or '\n' or '\u{..}' */ const char *t = n->text + 1; int l = n->text_len - 2;
        if (l >= 1 && t[0] == '\\') { char e = t[1]; unsigned cp = e=='n'?'\n':e=='t'?'\t':e=='r'?'\r':e=='0'?0:e=='\\'?'\\':e=='\''?'\'':e=='"'?'"':(unsigned)e;
            if (e == 'u') { cp = 0; for (int i = 3; i < l && t[i] != '}'; i++) { char h = t[i]; int d=(h>='0'&&h<='9')?h-'0':(h>='a'&&h<='f')?h-'a'+10:(h>='A'&&h<='F')?h-'A'+10:0; cp=cp*16+(unsigned)d; } }
            return v_char(ip, cp); }
        unsigned cp = 0; unsigned char c = (unsigned char)t[0]; int len = c<0x80?1:c<0xE0?2:c<0xF0?3:4;
        if (len == 1) cp = c; else { cp = c & (0x7Fu >> len); for (int k = 1; k < len && k < l; k++) cp = (cp<<6) | ((unsigned char)t[k] & 0x3F); }
        return v_char(ip, cp); }
    case N_TODO:  { ip->flow = FL_PANIC; ip->signal = v_str(ip, "reached a `todo` hole", 21); diag_add(ip->d, SEV_ERROR, "R0100", n->line, n->col, 1, "reached a `todo` hole at runtime", NULL); ip->had_runtime_error = true; return v_unit(ip); }
    case N_HOLE:  { ip->flow = FL_PANIC; ip->signal = v_str(ip, "reached a typed hole", 20); return v_unit(ip); }
    case N_SELF:  { Val *v = env_get(env, "self", 4); return v ? v : v_unit(ip); }
    case N_IDENT: return resolve_ident(ip, n, env);
    case N_FIELDACC: { Val *v = v_unit(ip); v->kind = V_FIELDACC; v->facc = n->kids[0]->text; v->facc_len = n->kids[0]->text_len; return v; }
    case N_TUPLE: { Val *v = v_list(ip); v->kind = V_TUPLE; for (int i = 0; i < n->nkid; i++) { v_push(ip, v, eval(ip, n->kids[i], env)); if (ip->flow != FL_NORMAL) return v; } return v; }
    case N_LIST:  { Val *v = v_list(ip); for (int i = 0; i < n->nkid; i++) { v_push(ip, v, eval(ip, n->kids[i], env)); if (ip->flow != FL_NORMAL) return v; } return v; }
    case N_SET:   { Val *v = v_list(ip); v->kind = V_SET; for (int i = 0; i < n->nkid; i++) { Val *e = eval(ip, n->kids[i], env); if (ip->flow != FL_NORMAL) return v; bool dup = false; for (int k = 0; k < v->n; k++) if (val_equal(v->items[k], e)) dup = true; if (!dup) v_push(ip, v, e); } return v; }
    case N_LISTREPEAT: { Val *item = eval(ip, n->kids[0], env); Val *cnt = eval(ip, n->kids[1], env); Val *v = v_list(ip); for (long long i = 0; i < cnt->i; i++) v_push(ip, v, item); return v; }
    case N_MAP: { Val *v = v_unit(ip); v->kind = V_MAP; for (int i = 0; i < n->nkid; i++) { const Node *e = n->kids[i]; Val *k = eval(ip, e->kids[0], env); Val *val = eval(ip, e->kids[1], env); if (ip->flow != FL_NORMAL) return v; int kl; char *ks = val_to_cstr(ip, k, &kl); if (v->nent == v->cent) { int nc = v->cent ? v->cent*2 : 8; Entry *ne = lume_rt_alloc(ip, (size_t)nc*sizeof(Entry)); if (v->ent) memcpy(ne, v->ent, (size_t)v->nent*sizeof(Entry)); v->ent = ne; v->cent = nc; } v->ent[v->nent].key = ks; v->ent[v->nent].klen = kl; v->ent[v->nent].val = val; v->nent++; } return v; }
    case N_COMPREHENSION: {
        Val *out = v_list(ip); const Node *body = n->kids[0];
        /* single for + optional if (Stage 0) */
        const Node *cf = NULL, *cond = NULL;
        for (int i = 1; i < n->nkid; i++) { if (n->kids[i]->kind == N_COMPFOR) cf = n->kids[i]; else cond = n->kids[i]; }
        Val *src = to_iter_list(ip, eval(ip, cf->kids[1], env));
        bool is_map = body->kind == N_MAPENTRY; if (is_map) { out = v_unit(ip); out->kind = V_MAP; }
        for (int i = 0; i < src->n; i++) {
            Env *e = env_new(ip, env);
            if (!pattern_match(ip, cf->kids[0], src->items[i], e)) continue;
            if (cond) { Val *c = eval(ip, cond, e); if (ip->flow != FL_NORMAL) return out; if (!val_truthy(c)) continue; }
            if (is_map) { Val *k = eval(ip, body->kids[0], e); Val *val = eval(ip, body->kids[1], e); int kl; char *ks = val_to_cstr(ip, k, &kl); if (out->nent == out->cent) { int nc = out->cent?out->cent*2:8; Entry *ne = lume_rt_alloc(ip,(size_t)nc*sizeof(Entry)); if (out->ent) memcpy(ne,out->ent,(size_t)out->nent*sizeof(Entry)); out->ent=ne; out->cent=nc; } out->ent[out->nent].key=ks; out->ent[out->nent].klen=kl; out->ent[out->nent].val=val; out->nent++; }
            else { Val *val = eval(ip, body, e); if (ip->flow != FL_NORMAL) return out; v_push(ip, out, val); }
        }
        return out; }
    case N_RANGE: { Val *v = v_unit(ip); v->kind = V_RANGE; v->inc = (n->flags & F_INCLUSIVE) != 0;
        if (n->nkid == 2) { v->lo = eval(ip, n->kids[0], env)->i; v->hi = eval(ip, n->kids[1], env)->i; }
        else if (n->nkid == 1) { v->lo = 0; v->hi = eval(ip, n->kids[0], env)->i; }
        return v; }
    case N_UNARY: { Val *x = eval(ip, n->kids[0], env); if (ip->flow != FL_NORMAL) return x;
        if (text_is(n, "-")) { if (x->kind == V_INT) return v_int(ip, -x->i); if (x->kind == V_FLOAT) return v_float(ip, -x->f); if (x->kind == V_DEC) return v_dec(ip, -x->i, x->scale); }
        if (text_is(n, "not") || text_is(n, "!")) return v_bool(ip, !val_truthy(x));
        if (text_is(n, "own") || text_is(n, "mut") || text_is(n, "&")) return x;   /* Stage 0: no-ops */
        return x; }
    case N_BINARY: {
        if (text_is(n, "|>")) {                                  /* pipeline */
            Val *left = eval(ip, n->kids[0], env); if (ip->flow != FL_NORMAL) return left;
            const Node *right = n->kids[1];
            if (right->kind == N_CALL) return eval_call(ip, right, env, left);
            Val *f = eval(ip, right, env); if (ip->flow != FL_NORMAL) return f;
            return call_value(ip, f, &left, 1, n);
        }
        if (text_is(n, "and") || text_is(n, "&&")) { Val *l = eval(ip, n->kids[0], env); if (ip->flow != FL_NORMAL || !val_truthy(l)) return ip->flow != FL_NORMAL ? l : v_bool(ip, false); return v_bool(ip, val_truthy(eval(ip, n->kids[1], env))); }
        if (text_is(n, "or") || text_is(n, "||")) { Val *l = eval(ip, n->kids[0], env); if (ip->flow != FL_NORMAL) return l; if (val_truthy(l)) return v_bool(ip, true); return v_bool(ip, val_truthy(eval(ip, n->kids[1], env))); }
        if (text_is(n, "is")) { return v_bool(ip, true); }       /* Stage 0: no runtime type test */
        if (text_is(n, "??")) { Val *l = eval(ip, n->kids[0], env); if (ip->flow != FL_NORMAL) return l; if (l->kind == V_OPT) return l->b ? l->inner : eval(ip, n->kids[1], env); return l; }
        Val *l = eval(ip, n->kids[0], env); if (ip->flow != FL_NORMAL) return l;
        Val *r = eval(ip, n->kids[1], env); if (ip->flow != FL_NORMAL) return r;
        if (text_is(n, "in")) { Val *src = to_iter_list(ip, r); for (int i = 0; i < src->n; i++) if (val_equal(src->items[i], l)) return v_bool(ip, true); return v_bool(ip, false); }
        return arith(ip, n->text, n->text_len, l, r, n); }
    case N_CALL:  return eval_call(ip, n, env, NULL);
    case N_INDEX: {
        Val *coll = eval(ip, n->kids[0], env); if (ip->flow != FL_NORMAL) return coll;
        const Node *idxn = n->kids[1];
        if (idxn->kind == N_RANGE) {
            Val *lst = to_iter_list(ip, coll); long long lo = 0, hi = lst->n; bool inc = (idxn->flags & F_INCLUSIVE) != 0;
            int kids = idxn->nkid;
            if (kids == 2) { lo = eval(ip, idxn->kids[0], env)->i; hi = eval(ip, idxn->kids[1], env)->i; }
            else if (kids == 1) { Val *b0 = eval(ip, idxn->kids[0], env); hi = b0->i; }  /* ..b */
            if (lo < 0) lo += lst->n;
            if (hi < 0) hi += lst->n;
            if (inc) hi += 1;
            if (lo < 0) lo = 0;
            if (hi > lst->n) hi = lst->n;
            if (coll->kind == V_STR) {                        /* slicing a string yields a string */
                int bl = 0; for (long long i = lo; i < hi; i++) bl += lst->items[i]->kind == V_CHAR ? 4 : 1;
                char *buf = lume_rt_alloc(ip, (size_t)bl + 1); int w = 0;
                for (long long i = lo; i < hi; i++) { int cl; char *cs = val_to_cstr(ip, lst->items[i], &cl); memcpy(buf + w, cs, (size_t)cl); w += cl; }
                buf[w] = 0; return v_str_owned(ip, buf, w);
            }
            Val *out = v_list(ip); for (long long i = lo; i < hi; i++) v_push(ip, out, lst->items[i]); return out;
        }
        Val *idx = eval(ip, idxn, env); if (ip->flow != FL_NORMAL) return idx;
        if (coll->kind == V_MAP) { int kl; char *ks = val_to_cstr(ip, idx, &kl); Entry *e = NULL; for (int i = 0; i < coll->nent; i++) if (coll->ent[i].klen == kl && memcmp(coll->ent[i].key, ks, (size_t)kl) == 0) e = &coll->ent[i]; return e ? e->val : v_unit(ip); }
        Val *lst = to_iter_list(ip, coll); long long i = idx->i; if (i < 0) i += lst->n; if (i < 0 || i >= lst->n) { rerror(ip, n, "index out of bounds"); return v_unit(ip); } return lst->items[i]; }
    case N_FIELD_GET: {
        const Node *recvn = n->kids[0];
        if (recvn->kind == N_IDENT && find_type(recvn->text, recvn->text_len)) { const VarReg *vr = find_var(n->text, n->text_len); return construct_variant(ip, vr, n->text, n->text_len, NULL, 0, n); }
        Val *rv = eval(ip, recvn, env); if (ip->flow != FL_NORMAL) return rv;
        return field_get(ip, rv, n->text, n->text_len, n); }
    case N_OPTCHAIN: {
        Val *rv = eval(ip, n->kids[0], env); if (ip->flow != FL_NORMAL) return rv;
        if (rv->kind == V_OPT && !rv->b) return v_opt_none(ip);
        Val *base = rv->kind == V_OPT ? rv->inner : rv;
        return v_opt_some(ip, field_get(ip, base, n->kids[1]->text, n->kids[1]->text_len, n)); }
    case N_TRYOP: {
        Val *x = eval(ip, n->kids[0], env); if (ip->flow != FL_NORMAL) return x;
        if (x->kind == V_RESULT) { if (x->b) return x->inner; ip->flow = FL_FAIL; ip->signal = x->inner; return v_unit(ip); }
        if (x->kind == V_OPT) { if (x->b) return x->inner; ip->flow = FL_RETURN; ip->signal = v_opt_none(ip); return v_unit(ip); }
        return x; }
    case N_CATCH: {
        Val *l = eval(ip, n->kids[0], env); if (ip->flow != FL_NORMAL) return l;
        if (l->kind == V_RESULT) { if (l->b) return l->inner; Val *h = eval(ip, n->kids[1], env); if (ip->flow != FL_NORMAL) return h; if (h->kind == V_CLOSURE || h->kind == V_BUILTIN) { Val *ei = l->inner; return call_value(ip, h, &ei, 1, n); } return h; }
        return l; }
    case N_CAST: {
        Val *x = eval(ip, n->kids[0], env); const Node *ty = n->kids[1];
        if (ty->kind == N_TYNAME) {
            if (text_is(ty, "f64") || text_is(ty, "f32") || text_is(ty, "float")) return v_float(ip, as_double(x));
            if (text_is(ty, "int") || text_is(ty, "i64") || text_is(ty, "i32") || text_is(ty, "u8") || text_is(ty, "usize") || text_is(ty, "u64")) return v_int(ip, x->kind == V_FLOAT ? (long long)x->f : x->kind == V_DEC ? x->i / pow10i(x->scale) : x->i);
            if (text_is(ty, "str")) { int l; char *s = val_to_cstr(ip, x, &l); return v_str_owned(ip, s, l); }
        }
        return x; }
    case N_IFEXPR: { Val *c = eval(ip, n->kids[0], env); if (ip->flow != FL_NORMAL) return c; return eval(ip, val_truthy(c) ? n->kids[1] : n->kids[2], env); }
    case N_IF: {
        Val *c = eval(ip, n->kids[0], env); if (ip->flow != FL_NORMAL) return c;
        if (val_truthy(c)) return eval_block(ip, n->kids[1], env);
        int i = 2;
        while (i + 1 < n->nkid && n->kids[i]->kind != N_BLOCK) { Val *ec = eval(ip, n->kids[i], env); if (ip->flow != FL_NORMAL) return ec; if (val_truthy(ec)) return eval_block(ip, n->kids[i+1], env); i += 2; }
        if (i < n->nkid && n->kids[i]->kind == N_BLOCK) return eval_block(ip, n->kids[i], env);
        return v_unit(ip); }
    case N_WHILE: {
        const Node *cond = n->kids[n->nkid >= 2 && n->kids[0]->kind == N_IDENT ? 1 : 0];
        const Node *body = NULL; for (int i = 0; i < n->nkid; i++) if (n->kids[i]->kind == N_BLOCK) { body = n->kids[i]; break; }
        while (1) { Val *c = eval(ip, cond, env); if (ip->flow != FL_NORMAL) return c; if (!val_truthy(c)) break;
            eval_block(ip, body, env);
            if (ip->flow == FL_BREAK) { ip->flow = FL_NORMAL; break; }
            if (ip->flow == FL_CONTINUE) { ip->flow = FL_NORMAL; continue; }
            if (ip->flow != FL_NORMAL) return v_unit(ip); }
        return v_unit(ip); }
    case N_FOR: {
        int ix = 0; if (n->kids[ix]->kind == N_IDENT) ix++;    /* label */
        const Node *pat = n->kids[ix++]; const Node *iter = n->kids[ix++]; const Node *body = n->kids[ix];
        Val *src = to_iter_list(ip, eval(ip, iter, env)); if (ip->flow != FL_NORMAL) return v_unit(ip);
        for (int i = 0; i < src->n; i++) {
            Env *e = env_new(ip, env);
            pattern_match(ip, pat, src->items[i], e);
            eval_block(ip, body, e);
            if (ip->flow == FL_BREAK) { ip->flow = FL_NORMAL; break; }
            if (ip->flow == FL_CONTINUE) { ip->flow = FL_NORMAL; continue; }
            if (ip->flow != FL_NORMAL) return v_unit(ip);
        }
        return v_unit(ip); }
    case N_LOOP: {
        const Node *body = NULL; for (int i = 0; i < n->nkid; i++) if (n->kids[i]->kind == N_BLOCK) body = n->kids[i];
        while (1) { eval_block(ip, body, env);
            if (ip->flow == FL_BREAK) { Val *s = ip->signal; ip->flow = FL_NORMAL; return s ? s : v_unit(ip); }
            if (ip->flow == FL_CONTINUE) { ip->flow = FL_NORMAL; continue; }
            if (ip->flow != FL_NORMAL) return v_unit(ip); }
    }
    case N_MATCH: {
        Val *scrut = eval(ip, n->kids[0], env); if (ip->flow != FL_NORMAL) return scrut;
        for (int i = 1; i < n->nkid; i++) {
            const Node *arm = n->kids[i]; const Node *pat = arm->kids[0]; const Node *body = arm->kids[arm->nkid - 1];
            const Node *guard = (arm->nkid == 3) ? arm->kids[1]->kids[0] : NULL;
            Env *e = env_new(ip, env);
            if (!pattern_match(ip, pat, scrut, e)) continue;
            if (guard) { Val *g = eval(ip, guard, e); if (ip->flow != FL_NORMAL) return g; if (!val_truthy(g)) continue; }
            return body->kind == N_BLOCK ? eval_block(ip, body, e) : eval(ip, body, e);
        }
        rerror(ip, n, "no match arm matched"); return v_unit(ip); }
    case N_DOBLOCK: return eval_block(ip, n->kids[0], env);
    case N_BLOCK: return eval_block(ip, n, env);
    case N_UNSAFE: case N_COMPTIME: case N_SCOPE: return eval_block(ip, n->kids[n->nkid - 1], env);
    case N_WITH: case N_USING: { Env *e = env_new(ip, env); Val *rhs = eval(ip, n->kids[1], env); if (ip->flow != FL_NORMAL) return rhs; env_define(ip, e, n->kids[0]->text, n->kids[0]->text_len, rhs); return eval_block(ip, n->kids[2], e); }
    /* statements */
    case N_LET: case N_VAR: {
        const Node *pat = n->kids[0]; const Node *tyn = NULL; const Node *rhs = NULL;
        for (int i = 1; i < n->nkid; i++) { if (is_type_kind(n->kids[i]->kind)) tyn = n->kids[i]; else rhs = n->kids[i]; }
        Val *v = rhs ? eval(ip, rhs, env) : v_unit(ip); if (ip->flow != FL_NORMAL) return v;
        int sc = decimal_scale_of(tyn); if (sc >= 0) v = coerce_decimal(ip, v, sc);
        pattern_match(ip, pat, v, env); return v_unit(ip); }
    case N_ASSIGN: {
        const Node *lhs = n->kids[0]; const Node *rhs = n->kids[1];
        Val *rv = eval(ip, rhs, env); if (ip->flow != FL_NORMAL) return rv;
        bool compound = !(n->text_len == 1 && n->text[0] == '=');
        if (compound) { char op[2] = { n->text[0], 0 }; Val *cur = eval(ip, lhs, env); if (ip->flow != FL_NORMAL) return cur; rv = arith(ip, op, 1, cur, rv, n); }
        if (lhs->kind == N_IDENT) { Val **slot = env_slot(env, lhs->text, lhs->text_len); if (slot) *slot = rv; else env_define(ip, env, lhs->text, lhs->text_len, rv); }
        else if (lhs->kind == N_FIELD_GET) { Val *recv = eval(ip, lhs->kids[0], env); Entry *e = field_slot(recv, lhs->text, lhs->text_len); if (e) e->val = rv; else rerror(ip, lhs, "no such field to assign"); }
        else if (lhs->kind == N_INDEX) { Val *coll = eval(ip, lhs->kids[0], env); Val *idx = eval(ip, lhs->kids[1], env);
            if (coll->kind == V_MAP) { int kl; char *ks = val_to_cstr(ip, idx, &kl); Entry *slot = NULL; for (int i = 0; i < coll->nent; i++) if (coll->ent[i].klen == kl && memcmp(coll->ent[i].key, ks, (size_t)kl) == 0) slot = &coll->ent[i];
                if (slot) slot->val = rv; else { if (coll->nent == coll->cent) { int nc = coll->cent?coll->cent*2:8; Entry *ne = lume_rt_alloc(ip,(size_t)nc*sizeof(Entry)); if (coll->ent) memcpy(ne,coll->ent,(size_t)coll->nent*sizeof(Entry)); coll->ent=ne; coll->cent=nc; } coll->ent[coll->nent].key=ks; coll->ent[coll->nent].klen=kl; coll->ent[coll->nent].val=rv; coll->nent++; } }
            else { long long i = idx->i; if (i < 0) i += coll->n; if (i >= 0 && i < coll->n) coll->items[i] = rv; else rerror(ip, lhs, "index out of bounds"); } }
        return v_unit(ip); }
    case N_EXPRSTMT: return eval(ip, n->kids[0], env);
    case N_RETURN: { ip->signal = n->nkid ? eval(ip, n->kids[0], env) : v_unit(ip); if (ip->flow == FL_NORMAL) ip->flow = FL_RETURN; return v_unit(ip); }
    case N_BREAK: { ip->signal = n->nkid && n->kids[n->nkid-1]->kind != N_IDENT ? eval(ip, n->kids[n->nkid-1], env) : v_unit(ip); ip->flow = FL_BREAK; return v_unit(ip); }
    case N_CONTINUE: ip->flow = FL_CONTINUE; return v_unit(ip);
    case N_DEFER: return v_unit(ip);   /* registered by eval_block */
    case N_FAIL: { ip->signal = eval(ip, n->kids[0], env); if (ip->flow == FL_NORMAL) ip->flow = FL_FAIL; return v_unit(ip); }
    case N_PANIC: { Val *m = n->nkid ? eval(ip, n->kids[0], env) : v_str(ip, "panic", 5); ip->signal = m; ip->flow = FL_PANIC; ip->had_runtime_error = true; return v_unit(ip); }
    case N_ASSERT: { Val *c = eval(ip, n->kids[0], env); if (ip->flow != FL_NORMAL) return c; if (!val_truthy(c)) { int l = 0; char *msg = n->nkid >= 2 ? val_to_cstr(ip, eval(ip, n->kids[1], env), &l) : (char*)"assertion failed"; diag_add(ip->d, SEV_ERROR, "R0200", n->line, n->col, 1, msg, NULL); ip->had_runtime_error = true; ip->flow = FL_PANIC; ip->signal = v_str(ip, msg, (int)strlen(msg)); } return v_unit(ip); }
    case N_REQUIRES: { Val *c = eval(ip, n->kids[0], env); if (ip->flow != FL_NORMAL) return c; if (!val_truthy(c)) { if (n->nkid >= 2) { ip->signal = eval(ip, n->kids[1], env); ip->flow = FL_FAIL; } else { diag_add(ip->d, SEV_ERROR, "R0201", n->line, n->col, 1, "precondition (requires) violated", NULL); ip->had_runtime_error = true; ip->flow = FL_PANIC; ip->signal = v_str(ip, "requires violated", 17); } } return v_unit(ip); }
    case N_ENSURES: return v_unit(ip);
    case N_CLOSURE: { Val *v = v_unit(ip); v->kind = V_CLOSURE; v->cl_params_owner = n; v->cl_env = env; v->is_closure_fn = false; return v; }
    default: rerror(ip, n, "unsupported expression in Stage 0"); return v_unit(ip);
    }
}

/* ======================================================================
 *  eval_source_expr (for string interpolation)
 * ====================================================================== */
static Val *eval_source_expr(Interp *ip, const char *src, int len, Env *env) {
    char *buf = lume_rt_alloc(ip, (size_t)len + 2); memcpy(buf, src, (size_t)len); buf[len] = '\n'; buf[len+1] = 0;
    DiagList d = {0}; TokenList toks = {0};
    lume_lex(buf, (size_t)len + 1, &toks, &d);
    Node *mod = lume_parse(&toks, ip->a, &d);
    Val *result = v_unit(ip);
    if (d.errors == 0) {
        for (int i = 0; i < mod->nkid; i++) { const Node *k = mod->kids[i];
            if (k->kind == N_EXPRSTMT) { result = eval(ip, k->kids[0], env); break; }
            if (k->kind != N_USE && k->kind != N_MOD) { result = eval(ip, k, env); break; } }
    } else { rerror(ip, NULL, "could not parse interpolation expression"); }
    tokens_free(&toks); diag_free(&d);
    return result;
}

/* ======================================================================
 *  registry build + lume_run
 * ====================================================================== */
static Val *mk_fn_closure(Interp *ip, const Node *fn, const char *name) { Val *v = v_unit(ip); v->kind = V_CLOSURE; v->cl_params_owner = fn; v->cl_env = ip->globals; v->is_closure_fn = true; v->cl_name = name; return v; }
static const char *fn_name(const Node *fn) { for (int i = 0; i < fn->nkid; i++) if (fn->kids[i]->kind == N_IDENT) return fn->kids[i]->text; return "?"; }
static int fn_name_len(const Node *fn) { for (int i = 0; i < fn->nkid; i++) if (fn->kids[i]->kind == N_IDENT) return fn->kids[i]->text_len; return 1; }
static bool fn_has_body(const Node *fn) { FnParts fp = fn_parts(fn); return fp.body != NULL; }

static void collect(Interp *ip, const Node *mod) {
    for (int i = 0; i < mod->nkid; i++) {
        const Node *it = mod->kids[i];
        if (it->kind == N_TYPEDECL) {
            const Node *nm = NULL; for (int j = 0; j < it->nkid; j++) if (it->kids[j]->kind == N_IDENT) { nm = it->kids[j]; break; }
            if (!nm) continue;
            reg_push_type(ntz(ip, nm->text, nm->text_len), nm->text_len, it);
            for (int j = 0; j < it->nkid; j++) if (it->kids[j]->kind == N_ENUM || it->kids[j]->kind == N_ERRORDEF) {
                const Node *en = it->kids[j];
                for (int v = 0; v < en->nkid; v++) if (en->kids[v]->kind == N_VARIANT) { const Node *var = en->kids[v]; reg_push_var(ntz(ip, var->kids[0]->text, var->kids[0]->text_len), var->kids[0]->text_len, ntz(ip, nm->text, nm->text_len), var); }
            }
        } else if (it->kind == N_FN) {
            env_define(ip, ip->globals, fn_name(it), fn_name_len(it), mk_fn_closure(ip, it, fn_name(it)));
        } else if (it->kind == N_CONST) {
            /* defer: evaluated lazily is complex; eval now in globals */
            const Node *nm = it->kids[0]; const Node *val = it->kids[it->nkid-1];
            if (val && !is_type_kind(val->kind)) env_define(ip, ip->globals, nm->text, nm->text_len, eval(ip, val, ip->globals));
        } else if (it->kind == N_TRAIT) {
            int tl = 0; const char *tn = NULL; for (int j = 0; j < it->nkid; j++) if (it->kids[j]->kind == N_IDENT) { tn = it->kids[j]->text; tl = it->kids[j]->text_len; break; }
            char *tnz = tn ? ntz(ip, tn, tl) : NULL;
            for (int j = 0; j < it->nkid; j++) if (it->kids[j]->kind == N_FN && fn_has_body(it->kids[j])) { const Node *m = it->kids[j]; reg_push_def(tnz, ntz(ip, fn_name(m), fn_name_len(m)), mk_fn_closure(ip, m, fn_name(m))); }
        } else if (it->kind == N_IMPL) {
            const Node *types[4]; int nt = 0; for (int j = 0; j < it->nkid && nt < 4; j++) if (it->kids[j]->kind == N_TYNAME) types[nt++] = it->kids[j];
            const char *selftype = NULL, *trait = NULL;
            if (nt >= 2) { trait = ntz(ip, types[0]->text, types[0]->text_len); selftype = ntz(ip, types[1]->text, types[1]->text_len); reg_push_impl(selftype, trait); }
            else if (nt == 1) { selftype = ntz(ip, types[0]->text, types[0]->text_len); }
            for (int j = 0; j < it->nkid; j++) if (it->kids[j]->kind == N_FN) { const Node *m = it->kids[j]; reg_push_meth(ntz(ip, fn_name(m), fn_name_len(m)), fn_name_len(m), selftype, trait, mk_fn_closure(ip, m, fn_name(m))); }
        }
    }
}

/* ---------- local module loading (use <path>) ---------- */
typedef struct { char **paths; int n, cap; } Visited;
static bool visited_add(Visited *v, const char *path) {
    for (int i = 0; i < v->n; i++) if (strcmp(v->paths[i], path) == 0) return false;
    if (v->n == v->cap) { v->cap = v->cap ? v->cap*2 : 8; v->paths = realloc(v->paths, (size_t)v->cap*sizeof(char*)); }
    char *dup = malloc(strlen(path) + 1); strcpy(dup, path); v->paths[v->n++] = dup; return true;
}
static void dirname_of(const char *file, char *out, size_t cap) {
    const char *slash = strrchr(file, '/');
    if (!slash) { snprintf(out, cap, "."); return; }
    size_t n = (size_t)(slash - file); if (n >= cap) n = cap - 1;
    memcpy(out, file, n); out[n] = 0;
}
/* probe the longest existing <dir>/<seg1>/.../<segk>.lume for a use's ident segments */
static char *resolve_use(const Node *use, const char *dir) {
    const Node *segn[16]; int m = 0;
    for (int i = 0; i < use->nkid && m < 16; i++)
        if (use->kids[i]->kind == N_IDENT && !(use->kids[i]->text_len == 1 && use->kids[i]->text[0] == '*'))
            segn[m++] = use->kids[i];
    static char path[1024];
    for (int k = m; k >= 1; k--) {
        int w = snprintf(path, sizeof path, "%s", dir);
        for (int i = 0; i < k; i++) w += snprintf(path + w, sizeof path - (size_t)w, "/%.*s", segn[i]->text_len, segn[i]->text);
        snprintf(path + w, sizeof path - (size_t)w, ".lume");
        FILE *f = fopen(path, "rb"); if (f) { fclose(f); return path; }
    }
    return NULL;
}
static void load_module(Interp *ip, const Node *mod, const char *dir, DiagList *diags, Visited *vis,
                        const Node ***list, int *nlist, int *clist) {
    for (int i = 0; i < mod->nkid; i++) {
        const Node *it = mod->kids[i];
        if (it->kind != N_USE) continue;
        char *path = resolve_use(it, dir);
        if (!path) continue;                      /* std or external: skip */
        if (!visited_add(vis, path)) continue;
        size_t len = 0; char *src = read_file(path, &len);
        if (!src) continue;
        char *srcbuf = arena_alloc(ip->a, len + 1); memcpy(srcbuf, src, len); srcbuf[len] = 0; free(src);
        TokenList toks = {0};
        lume_lex(srcbuf, len, &toks, diags);
        const Node *dep = lume_parse(&toks, ip->a, diags);
        tokens_free(&toks);
        char depdir[1024]; dirname_of(path, depdir, sizeof depdir);
        load_module(ip, dep, depdir, diags, vis, list, nlist, clist);   /* deps first */
        if (*nlist == *clist) { *clist = *clist ? *clist*2 : 8; *list = realloc(*list, (size_t)*clist*sizeof(Node*)); }
        (*list)[(*nlist)++] = dep;
    }
}

int lume_run(const Node *module, const char *file, DiagList *diags) {
    memset(&REG, 0, sizeof REG);
    Interp ip; memset(&ip, 0, sizeof ip);
    ip.rt = arena_new(); ip.a = arena_new(); ip.d = diags; ip.file = file; ip.flow = FL_NORMAL;
    ip.globals = env_new(&ip, NULL);
    /* load local module dependencies first, then collect them, then the entry module */
    char dir[1024]; dirname_of(file, dir, sizeof dir);
    Visited vis = {0}; const Node **deps = NULL; int ndeps = 0, cdeps = 0;
    load_module(&ip, module, dir, diags, &vis, &deps, &ndeps, &cdeps);
    for (int i = 0; i < ndeps; i++) collect(&ip, deps[i]);
    collect(&ip, module);
    Val *mainfn = env_get(ip.globals, "main", 4);
    if (mainfn && mainfn->kind == V_CLOSURE && mainfn->is_closure_fn) {
        Val *r = run_user_fn(&ip, mainfn, NULL, 0, module);
        if (ip.flow == FL_PANIC) { int l; char *m = val_to_cstr(&ip, ip.signal, &l); fprintf(stderr, "panic: %s\n", m); ip.exit_code = 1; }
        else if (r && r->kind == V_RESULT && !r->b) { int l; char *m = val_to_cstr(&ip, r->inner, &l); fprintf(stderr, "error: %s\n", m); ip.exit_code = 1; }
    } else {
        for (int i = 0; i < module->nkid; i++) { const Node *k = module->kids[i];
            if (k->kind == N_FN || k->kind == N_TYPEDECL || k->kind == N_TRAIT || k->kind == N_IMPL || k->kind == N_CONST || k->kind == N_USE || k->kind == N_MOD) continue;
            eval(&ip, k, ip.globals);
            if (ip.flow == FL_PANIC) { int l; char *m = val_to_cstr(&ip, ip.signal, &l); fprintf(stderr, "panic: %s\n", m); ip.exit_code = 1; break; }
            ip.flow = FL_NORMAL;
        }
    }
    int rc = ip.had_runtime_error && ip.exit_code == 0 ? 1 : ip.exit_code;
    free(REG.types); free(REG.vars); free(REG.meths); free(REG.defs); free(REG.impls);
    for (int i = 0; i < vis.n; i++) free(vis.paths[i]);
    free(vis.paths); free(deps);
    arena_free(ip.rt); arena_free(ip.a);
    return rc;
}
