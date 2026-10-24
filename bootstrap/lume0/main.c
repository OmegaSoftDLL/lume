#include "lume0.h"
#include "ast.h"
#include "value.h"

int lume_keyword_count(void);
const char *lume_keyword_at(int i);
int lume_operator_count(void);
const char *lume_operator_at(int i);
int lume_token_kind_count(void);
const char *token_kind_name(TokenKind k);
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int usage(void) {
    fputs("lume0 " LUME0_VERSION " — Stage 0 bootstrap interpreter for Lume\n"
          "usage:\n"
          "  lume0 tokens <file.lume> [--json]   lex and print tokens\n"
          "  lume0 parse  <file.lume> [--json]   parse and print the AST\n"
          "  lume0 describe [--json]              emit a machine-readable self-description of the language\n"
          "  lume0 check  <file.lume> [--json]   lex + parse; report diagnostics only\n"
          "  lume0 run    <file.lume>            run (in progress)\n"
          "  lume0 --version\n", stderr);
    return 64;
}

int main(int argc, char **argv) {
    if (argc >= 2 && strcmp(argv[1], "--version") == 0) { puts("lume0 " LUME0_VERSION); return 0; }
    if (argc >= 2 && strcmp(argv[1], "describe") == 0) {
        /* AI-first: a model that has never seen Lume loads this to ground itself before writing code */
        printf("{\n  \"language\": \"Lume\",\n  \"stage\": \"0 (bootstrap interpreter)\",\n  \"version\": \"" LUME0_VERSION "\",\n");
        printf("  \"edition\": \"2026\",\n");
        printf("  \"audience\": \"authored by AI, reviewed by humans\",\n");
        printf("  \"keywords\": [");
        for (int i = 0; i < lume_keyword_count(); i++) printf("%s\"%s\"", i ? ", " : "", lume_keyword_at(i));
        printf("],\n  \"operators\": [");
        for (int i = 0; i < lume_operator_count(); i++) { const char *o = lume_operator_at(i); printf("%s\"", i ? ", " : ""); for (const char *c = o; *c; c++) { if (*c == '\"' || *c == '\\') putchar('\\'); putchar(*c); } putchar('\"'); }
        printf("],\n  \"token_kinds\": [");
        for (int i = 0; i < lume_token_kind_count(); i++) printf("%s\"%s\"", i ? ", " : "", token_kind_name((TokenKind)i));
        printf("],\n  \"ast_node_kinds\": [");
        for (int i = 0; i < lume_node_kind_count(); i++) printf("%s\"%s\"", i ? ", " : "", lume_node_kind_at(i));
        printf("],\n  \"diagnostics_protocol\": \"lume0 parse|check|run --json emits one JSON diagnostic per line: {severity,code,file,line,col,len,message,help,doc}\",\n");
        printf("  \"workflow\": \"write .lume -> `lume0 check --json` -> read diagnostics -> fix -> repeat; `lume0 parse --json` returns the AST; `lume0 run` executes\",\n");
        printf("  \"spec\": \"docs/03-specification.md\",\n  \"self_description\": \"docs/08-ai-first.md\"\n}\n");
        return 0;
    }
    if (argc < 3) return usage();
    const char *cmd = argv[1], *path = argv[2];
    bool json = argc >= 4 && strcmp(argv[3], "--json") == 0;

    size_t len = 0;
    char *src = read_file(path, &len);
    if (!src) { fprintf(stderr, "lume0: cannot read %s\n", path); return 66; }

    DiagList diags = {0};
    TokenList toks = {0};
    bool ok = lume_lex(src, len, &toks, &diags);

    int rc = 0;
    if (strcmp(cmd, "tokens") == 0) {
        tokens_print(&toks, json);
        diag_print(&diags, path, json);
        rc = ok ? 0 : 1;
    } else if (strcmp(cmd, "parse") == 0) {
        Arena *arena = arena_new();
        Node *root = lume_parse(&toks, arena, &diags);
        if (diags.errors == 0) ast_print_json(root);
        diag_print(&diags, path, json);
        rc = diags.errors == 0 ? 0 : 1;
        arena_free(arena);
    } else if (strcmp(cmd, "check") == 0) {
        Arena *arena = arena_new();
        (void)lume_parse(&toks, arena, &diags);
        diag_print(&diags, path, json);
        if (diags.errors == 0 && !json) fprintf(stderr, "%s: OK (%zu tokens, parsed; type checker not yet implemented)\n", path, toks.len);
        rc = diags.errors == 0 ? 0 : 1;
        arena_free(arena);
    } else if (strcmp(cmd, "run") == 0) {
        Arena *arena = arena_new();
        Node *root = lume_parse(&toks, arena, &diags);
        if (diags.errors) { diag_print(&diags, path, json); rc = 1; }
        else { rc = lume_run(root, path, &diags); diag_print(&diags, path, json); if (diags.errors && rc == 0) rc = 1; }
        arena_free(arena);
    } else {
        rc = usage();
    }
    tokens_free(&toks);
    diag_free(&diags);
    free(src);
    return rc;
}
