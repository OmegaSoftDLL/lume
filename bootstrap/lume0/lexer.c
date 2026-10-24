/* Lexer for Lume (spec §1). Produces NEWLINE/INDENT/DEDENT layout tokens. */
#include "lume0.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

int lume_keyword_count(void);
const char *lume_keyword_at(int i);
int lume_operator_count(void);
const char *lume_operator_at(int i);
int lume_token_kind_count(void);

const char *token_kind_name(TokenKind k) {
    static const char *names[] = {
#define X(k) #k,
        TOKEN_KINDS(X)
#undef X
    };
    return (k >= 0 && k < TK__COUNT) ? names[k] + 3 /* strip "TK_" */ : "?";
}

/* ---------- keywords ---------- */

static const struct { const char *word; TokenKind kind; } KEYWORDS[] = {
    {"and", TK_KW_AND}, {"as", TK_KW_AS}, {"assert", TK_KW_ASSERT}, {"async", TK_KW_ASYNC},
    {"await", TK_KW_AWAIT}, {"break", TK_KW_BREAK}, {"catch", TK_KW_CATCH},
    {"comptime", TK_KW_COMPTIME}, {"const", TK_KW_CONST}, {"continue", TK_KW_CONTINUE},
    {"defer", TK_KW_DEFER}, {"distinct", TK_KW_DISTINCT}, {"do", TK_KW_DO}, {"dyn", TK_KW_DYN},
    {"elif", TK_KW_ELIF}, {"else", TK_KW_ELSE}, {"ensures", TK_KW_ENSURES}, {"enum", TK_KW_ENUM},
    {"errdefer", TK_KW_ERRDEFER}, {"error", TK_KW_ERROR}, {"extern", TK_KW_EXTERN},
    {"fail", TK_KW_FAIL}, {"false", TK_KW_FALSE}, {"fn", TK_KW_FN}, {"for", TK_KW_FOR},
    {"from", TK_KW_FROM}, {"if", TK_KW_IF}, {"impl", TK_KW_IMPL}, {"import", TK_KW_IMPORT},
    {"in", TK_KW_IN}, {"invariant", TK_KW_INVARIANT}, {"is", TK_KW_IS}, {"let", TK_KW_LET},
    {"loop", TK_KW_LOOP}, {"macro", TK_KW_MACRO}, {"match", TK_KW_MATCH}, {"mod", TK_KW_MOD},
    {"mut", TK_KW_MUT}, {"none", TK_KW_NONE}, {"not", TK_KW_NOT}, {"or", TK_KW_OR},
    {"own", TK_KW_OWN}, {"panic", TK_KW_PANIC}, {"pub", TK_KW_PUB}, {"record", TK_KW_RECORD},
    {"requires", TK_KW_REQUIRES}, {"return", TK_KW_RETURN}, {"scope", TK_KW_SCOPE},
    {"select", TK_KW_SELECT}, {"self", TK_KW_SELF}, {"Self", TK_KW_SELF_TYPE},
    {"spawn", TK_KW_SPAWN}, {"then", TK_KW_THEN}, {"todo", TK_KW_TODO}, {"trait", TK_KW_TRAIT},
    {"true", TK_KW_TRUE}, {"type", TK_KW_TYPE}, {"unsafe", TK_KW_UNSAFE}, {"use", TK_KW_USE},
    {"using", TK_KW_USING}, {"var", TK_KW_VAR}, {"where", TK_KW_WHERE}, {"while", TK_KW_WHILE},
    {"with", TK_KW_WITH}, {"yield", TK_KW_YIELD},
};

static TokenKind keyword_kind(const char *s, int len) {
    for (size_t i = 0; i < sizeof KEYWORDS / sizeof KEYWORDS[0]; i++)
        if ((int)strlen(KEYWORDS[i].word) == len && memcmp(KEYWORDS[i].word, s, (size_t)len) == 0)
            return KEYWORDS[i].kind;
    return TK_IDENT;
}

/* ---------- operators, longest match first ---------- */

static const struct { const char *text; TokenKind kind; } OPERATORS[] = {
    {"**=", TK_POW_EQ}, {"..=", TK_DOTDOT_EQ}, {"<<=", TK_SHL_EQ}, {">>=", TK_SHR_EQ},
    {"...", TK_ELLIPSIS},
    {"+%", TK_PLUS_WRAP}, {"-%", TK_MINUS_WRAP}, {"+|", TK_PLUS_SAT}, {"-|", TK_MINUS_SAT},
    {"+?", TK_PLUS_CHK}, {"-?", TK_MINUS_CHK},
    {"??", TK_QQ}, {"|>", TK_PIPE_GT}, {"->", TK_ARROW}, {"=>", TK_FAT_ARROW},
    {"==", TK_EQEQ}, {"!=", TK_NE}, {"<=", TK_LE}, {">=", TK_GE},
    {"+=", TK_PLUS_EQ}, {"-=", TK_MINUS_EQ}, {"*=", TK_STAR_EQ}, {"/=", TK_SLASH_EQ},
    {"%=", TK_PERCENT_EQ}, {"&=", TK_AMP_EQ}, {"|=", TK_PIPE_EQ}, {"^=", TK_CARET_EQ},
    {"<<", TK_SHL}, {">>", TK_SHR}, {"..", TK_DOTDOT}, {"&&", TK_AMPAMP}, {"||", TK_PIPEPIPE},
    {"**", TK_POW},
    {"(", TK_LPAREN}, {")", TK_RPAREN}, {"[", TK_LBRACKET}, {"]", TK_RBRACKET},
    {"{", TK_LBRACE}, {"}", TK_RBRACE}, {",", TK_COMMA}, {".", TK_DOT}, {":", TK_COLON},
    {";", TK_SEMI}, {"@", TK_AT}, {"$", TK_DOLLAR}, {"~", TK_TILDE},
    {"+", TK_PLUS}, {"-", TK_MINUS}, {"*", TK_STAR}, {"/", TK_SLASH}, {"%", TK_PERCENT},
    {"=", TK_EQ}, {"<", TK_LT}, {">", TK_GT}, {"&", TK_AMP}, {"|", TK_PIPE}, {"^", TK_CARET},
    {"?", TK_QUESTION}, {"!", TK_BANG},
};

/* Tokens after which a line break does NOT end the logical line (spec §1.5). */
static bool is_continuation(TokenKind k) {
    switch (k) {
    case TK_PLUS: case TK_MINUS: case TK_STAR: case TK_SLASH: case TK_PERCENT: case TK_POW:
    case TK_EQEQ: case TK_NE: case TK_LT: case TK_LE: case TK_GT: case TK_GE:
    case TK_AMP: case TK_AMPAMP: case TK_PIPE: case TK_PIPEPIPE: case TK_CARET:
    case TK_SHL: case TK_SHR: case TK_DOT: case TK_PIPE_GT: case TK_COMMA: case TK_QQ:
    case TK_EQ: case TK_PLUS_EQ: case TK_MINUS_EQ: case TK_STAR_EQ: case TK_SLASH_EQ:
    case TK_KW_AND: case TK_KW_OR: case TK_ARROW:
        return true;
    default:
        return false;
    }
}

/* ---------- helpers ---------- */

static void push_token(Lexer *lx, TokenKind kind, const char *start, int len, int line, int col) {
    TokenList *t = &lx->out;
    if (t->len == t->cap) {
        t->cap = t->cap ? t->cap * 2 : 256;
        t->items = realloc(t->items, t->cap * sizeof(Token));
        if (!t->items) { fputs("lume0: out of memory\n", stderr); exit(70); }
    }
    Token *tok = &t->items[t->len++];
    tok->kind = kind; tok->start = start; tok->len = len; tok->line = line; tok->col = col;
}

static inline char peek(const Lexer *lx, size_t off) {
    return lx->pos + off < lx->len ? lx->src[lx->pos + off] : '\0';
}

static inline void advance(Lexer *lx) {
    if (lx->pos >= lx->len) return;
    if (lx->src[lx->pos] == '\n') { lx->line++; lx->col = 1; }
    else if (((unsigned char)lx->src[lx->pos] & 0xC0) != 0x80) lx->col++; /* count code points */
    lx->pos++;
}

static void error_at(Lexer *lx, const char *code, int line, int col, int len, const char *msg,
                     const char *help) {
    diag_add(lx->diags, SEV_ERROR, code, line, col, len, msg, help);
}

static bool is_ident_start(unsigned char c) { return isalpha(c) || c == '_' || c >= 0x80; }
static bool is_ident_char(unsigned char c)  { return isalnum(c) || c == '_' || c >= 0x80; }

static TokenKind last_kind(const Lexer *lx) {
    return lx->out.len ? lx->out.items[lx->out.len - 1].kind : TK_NEWLINE;
}

/* ---------- layout ---------- */

static void handle_line_start(Lexer *lx) {
    /* Measure indentation; skip blank and comment-only lines entirely. */
    for (;;) {
        size_t p = lx->pos;
        int spaces = 0;
        while (p < lx->len && (lx->src[p] == ' ' || lx->src[p] == '\t')) {
            if (lx->src[p] == '\t') {
                error_at(lx, "L0001", lx->line, (int)(p - lx->pos) + 1, 1,
                         "tab character in indentation",
                         "Lume uses spaces for indentation; run `lume fmt` or configure your editor");
            }
            spaces++; p++;
        }
        char c = p < lx->len ? lx->src[p] : '\0';
        if (c == '\n' || c == '\r') {                 /* blank line */
            while (lx->pos < lx->len && lx->src[lx->pos] != '\n') advance(lx);
            if (lx->pos < lx->len) advance(lx);
            if (lx->pos >= lx->len) return;
            continue;
        }
        if (c == '#' && !(p + 1 < lx->len && lx->src[p + 1] == '#')) {   /* comment-only line */
            if (p + 1 < lx->len && lx->src[p + 1] == '[') break;          /* block comment: lex normally */
            while (lx->pos < lx->len && lx->src[lx->pos] != '\n') advance(lx);
            if (lx->pos < lx->len) advance(lx);
            if (lx->pos >= lx->len) return;
            continue;
        }
        if (c == '\0') return;
        /* consume the indentation */
        while (lx->pos < p) advance(lx);
        /* a line that STARTS with `|>` or `.method` continues the previous logical line (spec §1.5) */
        bool leading_continuation =
            (c == '|' && p + 1 < lx->len && lx->src[p + 1] == '>') ||
            (c == '.' && p + 1 < lx->len && is_ident_start((unsigned char)lx->src[p + 1]));
        if (leading_continuation && lx->out.len && lx->out.items[lx->out.len - 1].kind == TK_NEWLINE) {
            lx->out.len--;                       /* retract the NEWLINE: same logical line */
            lx->joining = true;
        }
        if (lx->bracket_depth > 0 || lx->joining) { lx->joining = false; return; }
        int cur = lx->indent_stack[lx->indent_depth - 1];
        if (spaces > cur) {
            if (lx->indent_depth >= 128) {
                error_at(lx, "L0003", lx->line, 1, spaces, "indentation nested too deeply", NULL);
                return;
            }
            lx->indent_stack[lx->indent_depth++] = spaces;
            push_token(lx, TK_INDENT, lx->src + lx->pos, 0, lx->line, lx->col);
        } else if (spaces < cur) {
            while (lx->indent_depth > 1 && lx->indent_stack[lx->indent_depth - 1] > spaces) {
                lx->indent_depth--;
                push_token(lx, TK_DEDENT, lx->src + lx->pos, 0, lx->line, lx->col);
            }
            if (lx->indent_stack[lx->indent_depth - 1] != spaces) {
                error_at(lx, "L0002", lx->line, 1, spaces,
                         "inconsistent dedent: this line does not match any enclosing indentation level",
                         "align the line with an outer block, or indent it to continue the inner one");
            }
        }
        return;
    }
}

static void end_logical_line(Lexer *lx) {
    if (lx->bracket_depth > 0) return;
    TokenKind k = last_kind(lx);
    if (k == TK_NEWLINE || k == TK_INDENT || k == TK_DEDENT) return;   /* nothing on this line */
    if (is_continuation(k)) { lx->joining = true; return; }
    push_token(lx, TK_NEWLINE, lx->src + lx->pos, 0, lx->line, lx->col);
}

/* ---------- literals ---------- */

static void lex_number(Lexer *lx) {
    const char *start = lx->src + lx->pos;
    int line = lx->line, col = lx->col;
    TokenKind kind = TK_INT;
    if (peek(lx, 0) == '0' && (peek(lx, 1) == 'x' || peek(lx, 1) == 'b' || peek(lx, 1) == 'o')) {
        char base = peek(lx, 1);
        advance(lx); advance(lx);
        int digits = 0;
        for (;;) {
            char c = peek(lx, 0);
            bool ok = (base == 'x') ? isxdigit((unsigned char)c)
                    : (base == 'b') ? (c == '0' || c == '1')
                    : (c >= '0' && c <= '7');
            if (ok) { digits++; advance(lx); }
            else if (c == '_') advance(lx);
            else break;
        }
        if (digits == 0)
            error_at(lx, "L0011", line, col, (int)(lx->src + lx->pos - start),
                     "numeric literal has a base prefix but no digits", NULL);
    } else {
        while (isdigit((unsigned char)peek(lx, 0)) || peek(lx, 0) == '_') advance(lx);
        /* fraction: '.' followed by a digit (so `1..3` and `x.0` stay separate tokens) */
        if (peek(lx, 0) == '.' && isdigit((unsigned char)peek(lx, 1))) {
            kind = TK_FLOAT;
            advance(lx);
            while (isdigit((unsigned char)peek(lx, 0)) || peek(lx, 0) == '_') advance(lx);
        }
        if ((peek(lx, 0) == 'e' || peek(lx, 0) == 'E') &&
            (isdigit((unsigned char)peek(lx, 1)) ||
             ((peek(lx, 1) == '+' || peek(lx, 1) == '-') && isdigit((unsigned char)peek(lx, 2))))) {
            kind = TK_FLOAT;
            advance(lx);
            if (peek(lx, 0) == '+' || peek(lx, 0) == '-') advance(lx);
            while (isdigit((unsigned char)peek(lx, 0))) advance(lx);
        }
    }
    if (is_ident_start((unsigned char)peek(lx, 0))) {
        /* unit suffixes (1.5h, 250ms, 2026-10-24d) are Stage 2: lex the suffix as a separate
           identifier for now, but reject letters glued to hex/float like `0xFFz`. */
    }
    push_token(lx, kind, start, (int)(lx->src + lx->pos - start), line, col);
}

/* Scan a string body starting after the opening quote(s). `quotes` is 1 or 3.
   interpolate: honour `{ ... }` with nested braces/quotes. escapes: honour backslash. */
static void lex_string_body(Lexer *lx, int quotes, bool interpolate, bool escapes,
                            const char *start, int line, int col, TokenKind kind) {
    int depth = 0;
    for (;;) {
        char c = peek(lx, 0);
        if (c == '\0') {
            error_at(lx, "L0020", line, col, (int)(lx->src + lx->pos - start),
                     "unterminated string literal", "add the closing quote");
            break;
        }
        if (depth == 0) {
            if (quotes == 1 && c == '"') { advance(lx); break; }
            if (quotes == 3 && c == '"' && peek(lx, 1) == '"' && peek(lx, 2) == '"') {
                advance(lx); advance(lx); advance(lx); break;
            }
            if (quotes == 1 && c == '\n') {
                error_at(lx, "L0020", line, col, (int)(lx->src + lx->pos - start),
                         "unterminated string literal (newline inside a single-line string)",
                         "use \"\"\"triple quotes\"\"\" for multi-line strings");
                break;
            }
        }
        if (escapes && c == '\\') {
            advance(lx);
            char e = peek(lx, 0);
            if (strchr("ntr0\\'\"{}", e)) advance(lx);
            else if (e == 'u' && peek(lx, 1) == '{') {
                advance(lx); advance(lx);
                int n = 0;
                while (isxdigit((unsigned char)peek(lx, 0))) { advance(lx); n++; }
                if (peek(lx, 0) != '}' || n == 0 || n > 6)
                    error_at(lx, "L0021", lx->line, lx->col, 1, "malformed \\u{...} escape",
                             "write 1–6 hex digits: \\u{1F600}");
                else advance(lx);
            } else {
                error_at(lx, "L0021", lx->line, lx->col - 1, 2, "unknown escape sequence",
                         "valid escapes: \\n \\t \\r \\0 \\\\ \\' \\\" \\{ \\} \\u{...}");
                if (e) advance(lx);
            }
            continue;
        }
        if (interpolate) {
            if (c == '{') depth++;
            else if (c == '}') { if (depth > 0) depth--; }
            else if (depth > 0 && c == '"') {
                /* nested string inside an interpolation: skip it */
                advance(lx);
                while (peek(lx, 0) && peek(lx, 0) != '"' && peek(lx, 0) != '\n') {
                    if (peek(lx, 0) == '\\') advance(lx);
                    advance(lx);
                }
                if (peek(lx, 0) == '"') advance(lx);
                continue;
            }
        }
        advance(lx);
    }
    push_token(lx, kind, start, (int)(lx->src + lx->pos - start), line, col);
}

static void lex_string(Lexer *lx, TokenKind kind, int prefix_len) {
    const char *start = lx->src + lx->pos;
    int line = lx->line, col = lx->col;
    for (int i = 0; i < prefix_len; i++) advance(lx);
    bool triple = peek(lx, 0) == '"' && peek(lx, 1) == '"' && peek(lx, 2) == '"';
    if (triple) { advance(lx); advance(lx); advance(lx); }
    else advance(lx);
    bool interpolate = (kind == TK_STRING);
    bool escapes = (kind == TK_STRING || kind == TK_BYTE_STRING);
    lex_string_body(lx, triple ? 3 : 1, interpolate, escapes, start, line, col, kind);
}

static void lex_char_or_label(Lexer *lx) {
    const char *start = lx->src + lx->pos;
    int line = lx->line, col = lx->col;
    advance(lx); /* ' */
    /* label: 'ident not followed by a closing quote */
    if (is_ident_start((unsigned char)peek(lx, 0))) {
        size_t p = lx->pos;
        while (p < lx->len && is_ident_char((unsigned char)lx->src[p])) p++;
        bool closes = p < lx->len && lx->src[p] == '\'';
        size_t ident_len = p - lx->pos;
        /* a single ASCII char followed by ' is a char literal: 'a' */
        if (!closes || ident_len > 1) {
            if (closes && ident_len > 1) { /* multi-byte UTF-8 single code point? */
                int cps = 0;
                for (size_t i = lx->pos; i < p; i++) if (((unsigned char)lx->src[i] & 0xC0) != 0x80) cps++;
                if (cps == 1) goto char_lit;
            }
            while (lx->pos < p) advance(lx);
            push_token(lx, TK_LABEL, start, (int)(lx->src + lx->pos - start), line, col);
            return;
        }
    }
char_lit:
    if (peek(lx, 0) == '\\') {
        advance(lx);
        if (peek(lx, 0) == 'u' && peek(lx, 1) == '{') {
            while (peek(lx, 0) && peek(lx, 0) != '}') advance(lx);
            if (peek(lx, 0) == '}') advance(lx);
        } else if (peek(lx, 0)) advance(lx);
    } else if (peek(lx, 0) && peek(lx, 0) != '\'' && peek(lx, 0) != '\n') {
        advance(lx);
        while (((unsigned char)peek(lx, 0) & 0xC0) == 0x80) advance(lx);
    } else {
        error_at(lx, "L0022", line, col, 1, "empty character literal", "write a character: 'a'");
    }
    if (peek(lx, 0) == '\'') advance(lx);
    else error_at(lx, "L0022", line, col, (int)(lx->src + lx->pos - start),
                  "unterminated character literal", "a char literal holds exactly one character: 'a'");
    push_token(lx, TK_CHAR, start, (int)(lx->src + lx->pos - start), line, col);
}

static void lex_comment(Lexer *lx) {
    const char *start = lx->src + lx->pos;
    int line = lx->line, col = lx->col;
    if (peek(lx, 1) == '[') {                    /* #[ ... ]# nestable block comment */
        int depth = 0;
        for (;;) {
            if (peek(lx, 0) == '\0') {
                error_at(lx, "L0030", line, col, 2, "unterminated block comment", "close it with ]#");
                return;
            }
            if (peek(lx, 0) == '#' && peek(lx, 1) == '[') { depth++; advance(lx); advance(lx); continue; }
            if (peek(lx, 0) == ']' && peek(lx, 1) == '#') {
                advance(lx); advance(lx);
                if (--depth == 0) return;
                continue;
            }
            advance(lx);
        }
    }
    bool doc = peek(lx, 1) == '#';
    while (peek(lx, 0) && peek(lx, 0) != '\n') advance(lx);
    if (doc) push_token(lx, TK_DOC_COMMENT, start, (int)(lx->src + lx->pos - start), line, col);
}

/* ---------- main loop ---------- */

bool lume_lex(const char *src, size_t len, TokenList *out, DiagList *diags) {
    Lexer lx = {0};
    lx.src = src; lx.len = len; lx.line = 1; lx.col = 1;
    lx.indent_stack[0] = 0; lx.indent_depth = 1;
    lx.at_line_start = true;
    lx.diags = diags;
    /* skip UTF-8 BOM */
    if (len >= 3 && (unsigned char)src[0] == 0xEF && (unsigned char)src[1] == 0xBB && (unsigned char)src[2] == 0xBF)
        lx.pos = 3;

    while (lx.pos < lx.len) {
        if (lx.at_line_start) {
            lx.at_line_start = false;
            handle_line_start(&lx);
            if (lx.pos >= lx.len) break;
        }
        char c = peek(&lx, 0);
        if (c == '\r') { advance(&lx); continue; }
        if (c == '\n') { end_logical_line(&lx); advance(&lx); lx.at_line_start = true; continue; }
        if (c == ' ') { advance(&lx); continue; }
        if (c == '\t') {
            error_at(&lx, "L0001", lx.line, lx.col, 1, "tab character in source",
                     "Lume uses spaces; tabs are only allowed inside string literals");
            advance(&lx); continue;
        }
        if (c == '#') { lex_comment(&lx); continue; }
        if (isdigit((unsigned char)c)) { lex_number(&lx); continue; }
        if (c == '"') { lex_string(&lx, TK_STRING, 0); continue; }
        if (c == '\'') { lex_char_or_label(&lx); continue; }
        if (c == '`') {                                       /* raw identifier `name` */
            const char *start = lx.src + lx.pos; int line = lx.line, col = lx.col;
            advance(&lx);
            while (peek(&lx, 0) && peek(&lx, 0) != '`' && peek(&lx, 0) != '\n') advance(&lx);
            if (peek(&lx, 0) == '`') advance(&lx);
            else error_at(&lx, "L0040", line, col, 1, "unterminated raw identifier", "close it with `");
            push_token(&lx, TK_IDENT, start, (int)(lx.src + lx.pos - start), line, col);
            continue;
        }
        if (is_ident_start((unsigned char)c)) {
            /* string prefixes: r"  b"  re" */
            if (c == 'r' && peek(&lx, 1) == '"') { lex_string(&lx, TK_RAW_STRING, 1); continue; }
            if (c == 'b' && peek(&lx, 1) == '"') { lex_string(&lx, TK_BYTE_STRING, 1); continue; }
            if (c == 'r' && peek(&lx, 1) == 'e' && peek(&lx, 2) == '"') { lex_string(&lx, TK_REGEX, 2); continue; }
            const char *start = lx.src + lx.pos; int line = lx.line, col = lx.col;
            while (is_ident_char((unsigned char)peek(&lx, 0))) advance(&lx);
            int n = (int)(lx.src + lx.pos - start);
            push_token(&lx, keyword_kind(start, n), start, n, line, col);
            continue;
        }
        /* operators */
        bool matched = false;
        for (size_t i = 0; i < sizeof OPERATORS / sizeof OPERATORS[0]; i++) {
            size_t n = strlen(OPERATORS[i].text);
            if (lx.pos + n <= lx.len && memcmp(lx.src + lx.pos, OPERATORS[i].text, n) == 0) {
                TokenKind k = OPERATORS[i].kind;
                /* `-?`/`+?` only when followed by whitespace: `x -?y` is rare; `a?-b`... keep simple:
                   treat `+?`/`-?` as checked ops only if the next char is a space. */
                if ((k == TK_PLUS_CHK || k == TK_MINUS_CHK) && lx.pos + n < lx.len && lx.src[lx.pos + n] != ' ')
                    continue;
                if (k == TK_LPAREN || k == TK_LBRACKET || k == TK_LBRACE) lx.bracket_depth++;
                if ((k == TK_RPAREN || k == TK_RBRACKET || k == TK_RBRACE) && lx.bracket_depth > 0) lx.bracket_depth--;
                push_token(&lx, k, lx.src + lx.pos, (int)n, lx.line, lx.col);
                for (size_t j = 0; j < n; j++) advance(&lx);
                matched = true;
                break;
            }
        }
        if (matched) continue;
        error_at(&lx, "L0050", lx.line, lx.col, 1, "unexpected character", NULL);
        advance(&lx);
    }
    end_logical_line(&lx);
    while (lx.indent_depth > 1) {
        lx.indent_depth--;
        push_token(&lx, TK_DEDENT, lx.src + lx.pos, 0, lx.line, lx.col);
    }
    if (lx.bracket_depth > 0)
        error_at(&lx, "L0060", lx.line, lx.col, 1, "unclosed bracket at end of file", NULL);
    push_token(&lx, TK_EOF, lx.src + lx.pos, 0, lx.line, lx.col);
    *out = lx.out;
    return diags->errors == 0;
}

void tokens_free(TokenList *t) { free(t->items); memset(t, 0, sizeof *t); }

void tokens_print(const TokenList *t, bool json) {
    if (json) putchar('[');
    for (size_t i = 0; i < t->len; i++) {
        const Token *k = &t->items[i];
        if (json) {
            printf("%s{\"kind\":\"%s\",\"line\":%d,\"col\":%d,\"text\":\"", i ? "," : "",
                   token_kind_name(k->kind), k->line, k->col);
            for (int j = 0; j < k->len; j++) {
                char c = k->start[j];
                if (c == '"' || c == '\\') { putchar('\\'); putchar(c); }
                else if (c == '\n') fputs("\\n", stdout);
                else putchar(c);
            }
            fputs("\"}", stdout);
        } else {
            printf("%4d:%-3d %-12s", k->line, k->col, token_kind_name(k->kind));
            if (k->len) { putchar(' '); fwrite(k->start, 1, (size_t)k->len, stdout); }
            putchar('\n');
        }
    }
    if (json) puts("]");
}

int lume_keyword_count(void) { return (int)(sizeof KEYWORDS / sizeof KEYWORDS[0]); }
const char *lume_keyword_at(int i) { return KEYWORDS[i].word; }
int lume_operator_count(void) { return (int)(sizeof OPERATORS / sizeof OPERATORS[0]); }
const char *lume_operator_at(int i) { return OPERATORS[i].text; }
int lume_token_kind_count(void) { return TK__COUNT; }
