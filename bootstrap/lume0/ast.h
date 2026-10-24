/* AST for lume0 (spec §2–§9). Uniform node: kind + optional text slice + child list.
 * Nodes live in an arena freed all at once. The JSON printer mirrors this structure. */
#ifndef LUME0_AST_H
#define LUME0_AST_H

#include "lume0.h"

#define NODE_KINDS(X) \
    X(N_MODULE) X(N_USE) X(N_MOD) X(N_CONST) X(N_ATTR) X(N_DOC) \
    X(N_FN) X(N_PARAM) X(N_GENERICS) X(N_GENERIC) X(N_EFFECTS) X(N_FROM) \
    X(N_REQUIRES) X(N_ENSURES) X(N_INVARIANT) \
    X(N_TYPEDECL) X(N_RECORD) X(N_FIELD) X(N_ENUM) X(N_VARIANT) X(N_ERRORDEF) \
    X(N_DISTINCT) X(N_TRAIT) X(N_IMPL) X(N_PROP) \
    /* types */ \
    X(N_TYNAME) X(N_TYTUPLE) X(N_TYARRAY) X(N_TYSLICE) X(N_TYFN) X(N_TYOPT) X(N_TYERR) \
    X(N_TYDYN) X(N_TYIMPL) X(N_TYOWN) X(N_TYMUT) X(N_TYREF) X(N_TYRANGE) X(N_TYWHERE) \
    X(N_TYSELF) X(N_TYNEVER) X(N_TYHOLE) \
    /* statements */ \
    X(N_LET) X(N_VAR) X(N_ASSIGN) X(N_EXPRSTMT) X(N_IF) X(N_WHILE) X(N_FOR) X(N_LOOP) \
    X(N_BREAK) X(N_CONTINUE) X(N_RETURN) X(N_DEFER) X(N_ERRDEFER) X(N_FAIL) X(N_PANIC) \
    X(N_ASSERT) X(N_USING) X(N_UNSAFE) X(N_COMPTIME) X(N_SCOPE) X(N_WITH) X(N_BLOCK) \
    /* expressions */ \
    X(N_INT) X(N_FLOAT) X(N_STR) X(N_RAWSTR) X(N_BYTESTR) X(N_REGEX) X(N_CHAR) X(N_BOOL) \
    X(N_NONE) X(N_TODO) X(N_HOLE) X(N_IDENT) X(N_PATH) X(N_SELF) \
    X(N_CALL) X(N_INDEX) X(N_FIELD_GET) X(N_TRYOP) X(N_OPTCHAIN) X(N_AWAIT) \
    X(N_UNARY) X(N_BINARY) X(N_RANGE) X(N_CAST) X(N_CATCH) \
    X(N_IFEXPR) X(N_MATCH) X(N_MATCHARM) X(N_CLOSURE) X(N_CLOSPARAM) \
    X(N_LIST) X(N_MAP) X(N_SET) X(N_TUPLE) X(N_MAPENTRY) X(N_COMPREHENSION) X(N_COMPFOR) \
    X(N_ARG) X(N_FIELDACC) X(N_LISTREPEAT) X(N_DOBLOCK) \
    /* patterns */ \
    X(N_PWILD) X(N_PLIT) X(N_PRANGE) X(N_PBIND) X(N_PVARIANT) X(N_PTUPLE) X(N_PLIST) \
    X(N_PSOME) X(N_PNONE) X(N_POK) X(N_PERR) X(N_PIS) X(N_PREST) X(N_PFIELD) X(N_POR) X(N_PAT) \
    X(N_ERROR_NODE)

typedef enum {
#define X(k) k,
    NODE_KINDS(X)
#undef X
    N__COUNT
} NodeKind;

const char *node_kind_name(NodeKind k);

typedef struct Node Node;
struct Node {
    NodeKind     kind;
    const char  *text;      /* slice into source (identifier/literal/op); may be NULL */
    int          text_len;
    int          line, col;
    Node       **kids;
    int          nkid, cap;
    int          flags;     /* small per-kind bitfield: pub, mut, own, desc, inclusive, … */
};

enum { F_PUB = 1<<0, F_MUT = 1<<1, F_OWN = 1<<2, F_DESC = 1<<3, F_INCLUSIVE = 1<<4,
       F_CRATE = 1<<5, F_EXTENSION = 1<<6, F_PAR = 1<<7, F_REF = 1<<8, F_ASYNC = 1<<9 };

/* Arena */
typedef struct Arena Arena;
Arena *arena_new(void);
void   arena_free(Arena *a);

void *arena_alloc(Arena *a, size_t n);
Node *node_new(Arena *a, NodeKind kind, int line, int col);
void  node_add(Arena *a, Node *parent, Node *child);
void  node_set_text(Node *n, const char *text, int len);

void  ast_print_json(const Node *root);

#endif

/* Parser (spec §2–§9): tokens -> AST. Returns the module root (never NULL; errors in diags). */
Node *lume_parse(const TokenList *toks, Arena *arena, DiagList *diags);

/* Tree-walking interpreter (Stage 0). Runs the core subset; evaluates dynamically with runtime
 * contract checks. Returns process exit code; diagnostics for load errors go to `diags`. */
int lume_run(const Node *module, const char *file, DiagList *diags);
