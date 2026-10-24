#include "lume0.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void diag_add(DiagList *d, Severity sev, const char *code, int line, int col, int len,
              const char *msg, const char *help) {
    if (d->len == d->cap) {
        d->cap = d->cap ? d->cap * 2 : 16;
        d->items = realloc(d->items, d->cap * sizeof(Diag));
        if (!d->items) { fputs("lume0: out of memory\n", stderr); exit(70); }
    }
    Diag *x = &d->items[d->len++];
    x->sev = sev; x->code = code; x->line = line; x->col = col; x->len = len;
    snprintf(x->msg, sizeof x->msg, "%s", msg);
    snprintf(x->help, sizeof x->help, "%s", help ? help : "");
    if (sev == SEV_ERROR) d->errors++;
}

static const char *sev_name(Severity s) {
    return s == SEV_ERROR ? "error" : s == SEV_WARNING ? "warning" : "note";
}

static void json_str(const char *s) {
    putchar('"');
    for (; *s; s++) {
        switch (*s) {
        case '"': fputs("\\\"", stdout); break;
        case '\\': fputs("\\\\", stdout); break;
        case '\n': fputs("\\n", stdout); break;
        case '\t': fputs("\\t", stdout); break;
        default: putchar(*s);
        }
    }
    putchar('"');
}

void diag_print(const DiagList *d, const char *file, bool json) {
    for (size_t i = 0; i < d->len; i++) {
        const Diag *x = &d->items[i];
        if (json) {
            printf("{\"severity\":\"%s\",\"code\":\"%s\",\"file\":", sev_name(x->sev), x->code);
            json_str(file);
            printf(",\"line\":%d,\"col\":%d,\"len\":%d,\"message\":", x->line, x->col, x->len);
            json_str(x->msg);
            printf(",\"help\":");
            json_str(x->help);
            printf(",\"doc\":\"docs/03-specification.md#17-diagnostics\"}\n");
        } else {
            fprintf(stderr, "%s[%s]: %s\n  --> %s:%d:%d\n", sev_name(x->sev), x->code, x->msg,
                    file, x->line, x->col);
            if (x->help[0]) fprintf(stderr, "  help: %s\n", x->help);
        }
    }
}

void diag_free(DiagList *d) { free(d->items); memset(d, 0, sizeof *d); }

char *read_file(const char *path, size_t *out_len) {
    FILE *f = fopen(path, "rb");
    if (!f) return NULL;
    if (fseek(f, 0, SEEK_END) != 0) { fclose(f); return NULL; }
    long n = ftell(f);
    if (n < 0) { fclose(f); return NULL; }
    rewind(f);
    char *buf = malloc((size_t)n + 1);
    if (!buf) { fclose(f); return NULL; }
    size_t got = fread(buf, 1, (size_t)n, f);
    fclose(f);
    buf[got] = '\0';
    if (out_len) *out_len = got;
    return buf;
}
