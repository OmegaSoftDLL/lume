/* lume0 — Stage 0 bootstrap interpreter for the Lume language.
 * Disposable: exists to run the Stage 1 compiler (written in Lume) once.
 * Portable C11, no dependencies. See docs/04-roadmap.md §Stage 0.
 */
#ifndef LUME0_H
#define LUME0_H

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

#define LUME0_VERSION "0.0.1"

/* ---------- Diagnostics (spec §17) ---------- */

typedef enum { SEV_ERROR, SEV_WARNING, SEV_NOTE } Severity;

typedef struct {
    Severity    sev;
    const char *code;     /* "L0001" */
    char        msg[256];
    char        help[256];
    int         line, col, len;
} Diag;

typedef struct {
    Diag  *items;
    size_t len, cap;
    int    errors;
} DiagList;

void diag_add(DiagList *d, Severity sev, const char *code, int line, int col, int len,
              const char *msg, const char *help);
void diag_print(const DiagList *d, const char *file, bool json);
void diag_free(DiagList *d);

/* ---------- Tokens ---------- */

#define TOKEN_KINDS(X) \
    /* layout */ \
    X(TK_NEWLINE) X(TK_INDENT) X(TK_DEDENT) X(TK_EOF) \
    /* literals & names */ \
    X(TK_IDENT) X(TK_INT) X(TK_FLOAT) X(TK_STRING) X(TK_RAW_STRING) X(TK_BYTE_STRING) \
    X(TK_REGEX) X(TK_CHAR) X(TK_LABEL) X(TK_DOC_COMMENT) \
    /* keywords */ \
    X(TK_KW_AND) X(TK_KW_AS) X(TK_KW_ASSERT) X(TK_KW_ASYNC) X(TK_KW_AWAIT) X(TK_KW_BREAK) \
    X(TK_KW_CATCH) X(TK_KW_COMPTIME) X(TK_KW_CONST) X(TK_KW_CONTINUE) X(TK_KW_DEFER) \
    X(TK_KW_DISTINCT) X(TK_KW_DO) X(TK_KW_DYN) X(TK_KW_ELIF) X(TK_KW_ELSE) X(TK_KW_ENSURES) \
    X(TK_KW_ENUM) X(TK_KW_ERRDEFER) X(TK_KW_ERROR) X(TK_KW_EXTERN) X(TK_KW_FAIL) X(TK_KW_FALSE) \
    X(TK_KW_FN) X(TK_KW_FOR) X(TK_KW_FROM) X(TK_KW_IF) X(TK_KW_IMPL) X(TK_KW_IMPORT) X(TK_KW_IN) \
    X(TK_KW_INVARIANT) X(TK_KW_IS) X(TK_KW_LET) X(TK_KW_LOOP) X(TK_KW_MACRO) X(TK_KW_MATCH) \
    X(TK_KW_MOD) X(TK_KW_MUT) X(TK_KW_NONE) X(TK_KW_NOT) X(TK_KW_OR) X(TK_KW_OWN) X(TK_KW_PANIC) \
    X(TK_KW_PUB) X(TK_KW_RECORD) X(TK_KW_REQUIRES) X(TK_KW_RETURN) X(TK_KW_SCOPE) X(TK_KW_SELECT) \
    X(TK_KW_SELF) X(TK_KW_SELF_TYPE) X(TK_KW_SPAWN) X(TK_KW_THEN) X(TK_KW_TODO) X(TK_KW_TRAIT) \
    X(TK_KW_TRUE) X(TK_KW_TYPE) X(TK_KW_UNSAFE) X(TK_KW_USE) X(TK_KW_USING) X(TK_KW_VAR) \
    X(TK_KW_WHERE) X(TK_KW_WHILE) X(TK_KW_WITH) X(TK_KW_YIELD) \
    /* punctuation / operators */ \
    X(TK_LPAREN) X(TK_RPAREN) X(TK_LBRACKET) X(TK_RBRACKET) X(TK_LBRACE) X(TK_RBRACE) \
    X(TK_COMMA) X(TK_DOT) X(TK_COLON) X(TK_SEMI) X(TK_AT) X(TK_HASH) X(TK_DOLLAR) X(TK_TILDE) \
    X(TK_PLUS) X(TK_MINUS) X(TK_STAR) X(TK_SLASH) X(TK_PERCENT) X(TK_POW) \
    X(TK_PLUS_WRAP) X(TK_MINUS_WRAP) X(TK_PLUS_SAT) X(TK_MINUS_SAT) X(TK_PLUS_CHK) X(TK_MINUS_CHK) \
    X(TK_EQ) X(TK_EQEQ) X(TK_NE) X(TK_LT) X(TK_LE) X(TK_GT) X(TK_GE) \
    X(TK_PLUS_EQ) X(TK_MINUS_EQ) X(TK_STAR_EQ) X(TK_SLASH_EQ) X(TK_PERCENT_EQ) X(TK_POW_EQ) \
    X(TK_AMP_EQ) X(TK_PIPE_EQ) X(TK_CARET_EQ) X(TK_SHL_EQ) X(TK_SHR_EQ) \
    X(TK_AMP) X(TK_AMPAMP) X(TK_PIPE) X(TK_PIPEPIPE) X(TK_CARET) X(TK_SHL) X(TK_SHR) \
    X(TK_ARROW) X(TK_FAT_ARROW) X(TK_DOTDOT) X(TK_DOTDOT_EQ) X(TK_ELLIPSIS) \
    X(TK_QUESTION) X(TK_QQ) X(TK_BANG) X(TK_PIPE_GT) \
    X(TK_ERROR)

typedef enum {
#define X(k) k,
    TOKEN_KINDS(X)
#undef X
    TK__COUNT
} TokenKind;

const char *token_kind_name(TokenKind k);

typedef struct {
    TokenKind   kind;
    const char *start;   /* points into source */
    int         len;
    int         line, col;
} Token;

typedef struct {
    Token *items;
    size_t len, cap;
} TokenList;

/* ---------- Lexer ---------- */

typedef struct {
    const char *src;
    size_t      len;
    size_t      pos;
    int         line, col;
    int         indent_stack[128];
    int         indent_depth;      /* number of entries in indent_stack (stack[0] == 0) */
    int         bracket_depth;     /* ( [ { nesting: newlines insignificant inside */
    bool        at_line_start;
    bool        joining;           /* previous line ended with a continuation operator */
    TokenList   out;
    DiagList   *diags;
} Lexer;

/* Lexes the whole source into `out`. Returns false if any error diagnostic was produced. */
bool lume_lex(const char *src, size_t len, TokenList *out, DiagList *diags);
void tokens_free(TokenList *t);
void tokens_print(const TokenList *t, bool json);

/* ---------- Utilities ---------- */

char *read_file(const char *path, size_t *out_len);
int   elf_assemble(const char *src, const char *outpath);  /* x86-64 asm -> static ELF, no as/ld */

#endif
