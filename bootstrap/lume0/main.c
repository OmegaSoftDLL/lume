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
          "  lume0 emit <file.lume> <out.lbe>     compile source to the Lume Binary Encoding (AST, no text)\n"
          "  lume0 run    <file.lbe>              load and run an LBE program directly (no re-parse)\n"
          "  lume0 emit-asm <file.lume> <out.s>   compile to x86-64 assembly directly (no C in the pipeline)\n"
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
        printf("  \"check_catches\": [\"L* lexical\", \"P* parse\", \"R0200 undefined name\", \"T0001 literal type mismatch vs annotation\", \"D0001 duplicate parameter\", \"D0002 duplicate top-level function\", \"A0001 too many arguments\", \"A0002 too few arguments\", \"C0001 break/continue outside a loop\", \"C0002 return outside a function\", \"M0001 assignment to an immutable binding\", \"U0001 unreachable match arm\", \"U0002 unreachable code after return/break/continue\"],\n");
        printf("  \"workflow\": \"write .lume -> `lume0 check --json` -> read diagnostics -> fix -> repeat; check does lex+parse+name-resolution+basic-type+duplicate checks; `lume0 parse --json` returns the AST; `lume0 run` executes; `lume0 emit-asm`/`lume0 asm` lower to a native ELF with no C and no as/ld\",\n");
        printf("  \"spec\": \"docs/03-specification.md\",\n  \"self_description\": \"docs/08-ai-first.md\"\n}\n");
        return 0;
    }
    if (argc >= 4 && strcmp(argv[1], "emit-asm") == 0) {
        size_t len = 0; char *src = read_file(argv[2], &len);
        if (!src) { fprintf(stderr, "lume0: cannot read %s\n", argv[2]); return 66; }
        DiagList d = {0}; TokenList t = {0};
        lume_lex(src, len, &t, &d);
        Arena *ar = arena_new();
        Node *root = lume_parse(&t, ar, &d);
        int rc0 = 0;
        if (d.errors) { diag_print(&d, argv[2], false); rc0 = 1; }
        else { FILE *o = fopen(argv[3], "wb"); if (!o) { fprintf(stderr, "lume0: cannot write %s\n", argv[3]); rc0 = 73; } else { if (asm_emit(root, o) != 0) { fprintf(stderr, "lume0: program uses features the x86-64 backend does not support yet (functions/if/strings)\n"); rc0 = 1; } fclose(o); } }
        arena_free(ar); tokens_free(&t); diag_free(&d); free(src);
        return rc0;
    }
    if (argc >= 4 && strcmp(argv[1], "asm") == 0) {
        size_t len = 0; char *src = read_file(argv[2], &len);
        if (!src) { fprintf(stderr, "lume0: cannot read %s\n", argv[2]); return 66; }
        int rc0 = elf_assemble(src, argv[3]);
        free(src);
        return rc0;
    }
    if (argc >= 4 && strcmp(argv[1], "emit") == 0) {
        size_t len = 0; char *src = read_file(argv[2], &len);
        if (!src) { fprintf(stderr, "lume0: cannot read %s\n", argv[2]); return 66; }
        DiagList d = {0}; TokenList t = {0};
        bool ok0 = lume_lex(src, len, &t, &d);
        Arena *ar = arena_new();
        Node *root = lume_parse(&t, ar, &d);
        int rc0 = 0;
        if (d.errors) { diag_print(&d, argv[2], false); rc0 = 1; }
        else { FILE *out = fopen(argv[3], "wb"); if (!out) { fprintf(stderr, "lume0: cannot write %s\n", argv[3]); rc0 = 73; } else { lbe_write(root, out); fclose(out); } }
        (void)ok0; arena_free(ar); tokens_free(&t); diag_free(&d); free(src);
        return rc0;
    }
    if (argc < 3) return usage();
    const char *cmd = argv[1], *path = argv[2];
    bool json = argc >= 4 && strcmp(argv[3], "--json") == 0;

    size_t len = 0;
    char *src = read_file(path, &len);
    if (!src) { fprintf(stderr, "lume0: cannot read %s\n", path); return 66; }

    DiagList diags = {0};
    TokenList toks = {0};
    size_t _pl = strlen(path);
    bool is_lbe = _pl > 4 && strcmp(path + _pl - 4, ".lbe") == 0;
    bool ok = true;
    if (!is_lbe) ok = lume_lex(src, len, &toks, &diags);

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
        Node *root = lume_parse(&toks, arena, &diags);
        if (diags.errors == 0) lume_resolve(root, &diags);
        diag_print(&diags, path, json);
        if (diags.errors == 0 && !json) fprintf(stderr, "%s: OK (%zu tokens, parsed, names resolved)\n", path, toks.len);
        rc = diags.errors == 0 ? 0 : 1;
        arena_free(arena);
    } else if (strcmp(cmd, "run") == 0) {
        lume_set_args(argc - 3, argv + 3);   /* args after `run <file>` are the program's */
        Arena *arena = arena_new();
        size_t pl = strlen(path);
        Node *root;
        if (pl > 4 && strcmp(path + pl - 4, ".lbe") == 0) {
            root = lbe_read((const unsigned char *)src, len, arena);   /* structure in, no re-parse */
            if (!root) { fprintf(stderr, "lume0: %s is not a valid LBE file\n", path); arena_free(arena); tokens_free(&toks); diag_free(&diags); free(src); return 65; }
        } else {
            root = lume_parse(&toks, arena, &diags);
        }
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
