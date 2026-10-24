/* Runtime values for the Stage-0 interpreter. Boxed: everything is a Val* from a grow-only arena. */
#ifndef LUME0_VALUE_H
#define LUME0_VALUE_H
#include "ast.h"
#include <stdbool.h>

typedef enum {
    V_UNIT, V_BOOL, V_INT, V_FLOAT, V_DEC, V_CHAR, V_STR,
    V_LIST, V_SET, V_TUPLE, V_MAP, V_RECORD, V_VARIANT,
    V_OPT, V_RESULT, V_RANGE, V_CLOSURE, V_BUILTIN, V_FIELDACC, V_TYPE
} VKind;

typedef struct Val Val;
typedef struct Env Env;
typedef struct Interp Interp;

typedef struct { char *key; int klen; Val *val; } Entry;

struct Val {
    VKind kind;
    /* scalars */
    long long i;        /* V_INT; V_DEC units; V_CHAR codepoint */
    double    f;        /* V_FLOAT */
    int       scale;    /* V_DEC: number of fractional digits */
    bool      b;        /* V_BOOL; V_OPT is_some; V_RESULT is_ok */
    /* string */
    char     *s; int slen;
    /* sequences: list/set/tuple items, variant payload */
    Val     **items; int n, cap;
    char    **names;   /* optional field/payload names, parallel to items */
    /* map / record fields */
    Entry    *ent; int nent, cent;
    /* names */
    const char *tname;  /* type name (record/variant/type) */
    const char *tag;    /* variant tag */
    /* opt/result inner */
    Val *inner;
    /* range */
    long long lo, hi; bool inc;
    /* closure */
    const Node *cl_params_owner; /* the N_FN or N_CLOSURE node (for params/body) */
    Env  *cl_env;
    const char *cl_name;
    bool  is_closure_fn;         /* true if from N_FN (params are N_PARAM kids) */
    /* builtin */
    Val *(*builtin)(Interp *, Val **args, int nargs, const Node *site);
    const char *bname;
    /* fieldacc: the field name */
    const char *facc; int facc_len;
};

/* control-flow signal threaded through evaluation */
typedef enum { FL_NORMAL, FL_RETURN, FL_BREAK, FL_CONTINUE, FL_FAIL, FL_PANIC } Flow;

struct Interp {
    Arena      *a;          /* AST arena (borrowed) */
    Arena      *rt;         /* runtime arena (values, envs) */
    DiagList   *d;
    const char *file;
    Env        *globals;
    /* control-flow carrier */
    Flow        flow;
    Val        *signal;     /* return/break value, or fail/panic payload */
    const char *brk_label;
    Env        *old_env;    /* snapshot for old() in ensures */
    int         exit_code;
    bool        had_runtime_error;
};

/* arena-backed constructors */
Val *v_unit(Interp *);
Val *v_bool(Interp *, bool);
Val *v_int(Interp *, long long);
Val *v_float(Interp *, double);
Val *v_dec(Interp *, long long units, int scale);
Val *v_char(Interp *, unsigned cp);
Val *v_str(Interp *, const char *s, int len);
Val *v_str_owned(Interp *, char *s, int len);
Val *v_list(Interp *);
Val *v_opt_some(Interp *, Val *);
Val *v_opt_none(Interp *);
Val *v_ok(Interp *, Val *);
Val *v_err(Interp *, Val *);
void v_push(Interp *, Val *list, Val *item);

/* environment */
Env *env_new(Interp *, Env *parent);
Val *env_get(Env *, const char *name, int len);
Val **env_slot(Env *, const char *name, int len);     /* for assignment; NULL if absent */
void env_define(Interp *, Env *, const char *name, int len, Val *);

/* display */
char *val_to_cstr(Interp *, Val *, int *out_len);       /* Show */
bool  val_equal(Val *, Val *);
bool  val_truthy(Val *);

#endif
