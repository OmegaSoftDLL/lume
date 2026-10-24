#include "lume0.h"
#include "ast.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int usage(void) {
    fputs("lume0 " LUME0_VERSION " — Stage 0 bootstrap interpreter for Lume\n"
          "usage:\n"
          "  lume0 tokens <file.lume> [--json]   lex and print tokens\n"
          "  lume0 parse  <file.lume> [--json]   parse and print the AST\n"
          "  lume0 check  <file.lume> [--json]   lex + parse; report diagnostics only\n"
          "  lume0 run    <file.lume>            run (in progress)\n"
          "  lume0 --version\n", stderr);
    return 64;
}

int main(int argc, char **argv) {
    if (argc >= 2 && strcmp(argv[1], "--version") == 0) { puts("lume0 " LUME0_VERSION); return 0; }
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
        diag_print(&diags, path, json);
        if (ok) { fputs("lume0: `run` is not implemented yet (Stage 0: lexer+parser done; checker+interpreter next)\n", stderr); rc = 2; }
        else rc = 1;
    } else {
        rc = usage();
    }
    tokens_free(&toks);
    diag_free(&diags);
    free(src);
    return rc;
}
