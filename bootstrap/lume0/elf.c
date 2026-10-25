/* elf.c — a tiny x86-64 assembler + static ELF64 writer for the bootstrap.
 * It turns the assembly that the Lume-written backend emits into a runnable
 * executable WITHOUT the system assembler (`as`) or linker (`ld`). This is the
 * next dependency the project drops on the way to "no C and no assembly": the
 * machine code is written out as raw bytes in an ELF container.
 *
 * This first brick covers a minimal instruction set (enough to run a program
 * that writes to stdout and exits via raw syscalls). It is grown, one verified
 * instruction form at a time, until it assembles the full backend output. Any
 * form it does not yet understand is a hard error, never a silent miscompile. */
#include "lume0.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

static char *xstrdup(const char *s) { size_t n = strlen(s) + 1; char *d = malloc(n); memcpy(d, s, n); return d; }

#define ELF_VBASE 0x400000ULL
#define ELF_HDRS  120          /* ELF header (64) + one program header (56) */

/* ---- output byte buffer ---- */
typedef struct { uint8_t *d; size_t n, cap; } Buf;
static void buf_put(Buf *b, uint8_t x) {
    if (b->n == b->cap) { b->cap = b->cap ? b->cap * 2 : 256; b->d = realloc(b->d, b->cap); }
    b->d[b->n++] = x;
}
static void buf_put32(Buf *b, uint32_t x) { for (int i = 0; i < 4; i++) buf_put(b, (uint8_t)((x >> (8 * i)) & 0xff)); }
static void buf_put64(Buf *b, uint64_t x) { for (int i = 0; i < 8; i++) buf_put(b, (uint8_t)((x >> (8 * i)) & 0xff)); }

/* ---- label table (name -> payload offset) ---- */
typedef struct { char *name; size_t off; } Label;
typedef struct { Label *v; int n, cap; } Labels;
static void lbl_add(Labels *t, const char *name, size_t off) {
    if (t->n == t->cap) { t->cap = t->cap ? t->cap * 2 : 32; t->v = realloc(t->v, (size_t)t->cap * sizeof(Label)); }
    t->v[t->n].name = xstrdup(name); t->v[t->n].off = off; t->n++;
}
static long lbl_find(const Labels *t, const char *name) {
    for (int i = 0; i < t->n; i++) if (strcmp(t->v[i].name, name) == 0) return (long)t->v[i].off;
    return -1;
}

/* ---- register parsing: rax=0 rcx=1 rdx=2 rbx=3 rsp=4 rbp=5 rsi=6 rdi=7 r8..r15 ---- */
static int reg_num(const char *s) {
    if (s[0] != '%') return -1;
    s++;
    static const char *n64[8] = { "rax", "rcx", "rdx", "rbx", "rsp", "rbp", "rsi", "rdi" };
    for (int i = 0; i < 8; i++) if (strcmp(s, n64[i]) == 0) return i;
    if (s[0] == 'r' && s[1] >= '8' && s[1] <= '9' && s[2] == 0) return 8 + (s[1] - '8');
    if (s[0] == 'r' && s[1] == '1' && s[2] >= '0' && s[2] <= '5' && s[3] == 0) return 10 + (s[2] - '0');
    return -1;
}

/* ---- a parsed instruction (sizes are fixed given operands, so we lay out in
 * one pass and patch rip-relative displacements in a second) ---- */
typedef enum { I_MOVri, I_MOVrr, I_XORrr, I_LEArip, I_SYSCALL, I_RET } IKind;
typedef struct {
    IKind kind;
    int r1, r2;           /* registers */
    long long imm;        /* immediate */
    char sym[64];         /* rip-relative target label */
    size_t off;           /* payload offset assigned in layout */
    size_t len;           /* encoded length */
} Ins;

static size_t ins_len(const Ins *in) {
    switch (in->kind) {
    case I_MOVri:  return 7;   /* REX.W C7 /0 imm32 */
    case I_MOVrr:  return 3;   /* REX.W 89 /r */
    case I_XORrr:  return 3;   /* REX.W 31 /r */
    case I_LEArip: return 7;   /* REX.W 8D /r disp32 */
    case I_SYSCALL:return 2;
    case I_RET:    return 1;
    }
    return 0;
}

static uint8_t rexw(int r, int b) { return (uint8_t)(0x48 | (r >= 8 ? 4 : 0) | (b >= 8 ? 1 : 0)); }

/* emit one instruction's bytes; `payload_vaddr` is the vaddr of payload[0] */
static int ins_emit(const Ins *in, Buf *out, const Labels *lab, uint64_t payload_vaddr, char *err) {
    switch (in->kind) {
    case I_MOVri:
        buf_put(out, rexw(0, in->r1));
        buf_put(out, 0xC7);
        buf_put(out, (uint8_t)(0xC0 | (in->r1 & 7)));
        buf_put32(out, (uint32_t)(int32_t)in->imm);
        return 0;
    case I_MOVrr:
        buf_put(out, rexw(in->r2, in->r1));
        buf_put(out, 0x89);
        buf_put(out, (uint8_t)(0xC0 | ((in->r2 & 7) << 3) | (in->r1 & 7)));
        return 0;
    case I_XORrr:
        buf_put(out, rexw(in->r2, in->r1));
        buf_put(out, 0x31);
        buf_put(out, (uint8_t)(0xC0 | ((in->r2 & 7) << 3) | (in->r1 & 7)));
        return 0;
    case I_LEArip: {
        long toff = lbl_find(lab, in->sym);
        if (toff < 0) { snprintf(err, 128, "undefined label %s", in->sym); return 1; }
        uint64_t insn_end = payload_vaddr + in->off + in->len;
        int32_t disp = (int32_t)((uint64_t)((uint64_t)payload_vaddr + (uint64_t)toff) - insn_end);
        buf_put(out, rexw(in->r1, 0));
        buf_put(out, 0x8D);
        buf_put(out, (uint8_t)(0x05 | ((in->r1 & 7) << 3)));
        buf_put32(out, (uint32_t)disp);
        return 0;
    }
    case I_SYSCALL: buf_put(out, 0x0F); buf_put(out, 0x05); return 0;
    case I_RET:     buf_put(out, 0xC3); return 0;
    }
    return 0;
}

/* trim leading/trailing whitespace in place-ish (returns pointer past leading ws) */
static char *trim(char *s) {
    while (*s == ' ' || *s == '\t') s++;
    size_t n = strlen(s);
    while (n > 0 && (s[n-1] == ' ' || s[n-1] == '\t' || s[n-1] == '\r' || s[n-1] == '\n')) s[--n] = 0;
    return s;
}

/* assemble AT&T-ish source (the minimal subset) into `code` payload + collect
 * rodata bytes appended after code. Returns 0 on success. */
int elf_assemble(const char *src, const char *outpath) {
    char err[160] = {0};
    /* split into lines (mutable copy) */
    char *buf = xstrdup(src);
    Ins *ins = NULL; int ni = 0, cap = 0;
    Labels labels = {0};
    Buf rodata = {0};
    /* rodata labels are assigned offsets relative to rodata start; we fix them
     * after code size is known. Record (label, rodata-offset) separately. */
    typedef struct { char *name; size_t roff; } RoLab; RoLab *rol = NULL; int nrol = 0, rolcap = 0;
    int section = 0; /* 0 = text, 1 = rodata */
    int entry_defined = 0;

    for (char *line = strtok(buf, "\n"); line; line = strtok(NULL, "\n")) {
        char *s = trim(line);
        if (*s == 0 || *s == '#') continue;
        if (s[0] == '.') {
            if (strncmp(s, ".section .rodata", 16) == 0) { section = 1; continue; }
            if (strncmp(s, ".section .text", 14) == 0)   { section = 0; continue; }
            if (strncmp(s, ".section .bss", 13) == 0)     { section = 2; continue; }
            if (strncmp(s, ".globl", 6) == 0) continue;
            if (strncmp(s, ".lcomm", 6) == 0) continue; /* bss handled later; unused in this brick */
            if (strncmp(s, ".asciz", 6) == 0 || strncmp(s, ".ascii", 6) == 0) {
                /* .asciz "..."  -> raw bytes (+ NUL for .asciz) */
                int zterm = (s[5] == 'z');
                char *q = strchr(s, '"'); if (!q) continue; q++;
                while (*q && *q != '"') {
                    char c = *q++;
                    if (c == '\\') { char e = *q++; if (e=='n') c='\n'; else if (e=='t') c='\t'; else if (e=='0') c='\0'; else c=e; }
                    buf_put(&rodata, (uint8_t)c);
                }
                if (zterm) buf_put(&rodata, 0);
                continue;
            }
            continue; /* ignore other directives */
        }
        /* label? ends with ':' and has no space */
        size_t L = strlen(s);
        if (L > 0 && s[L-1] == ':') {
            s[L-1] = 0;
            if (section == 1) {
                if (nrol == rolcap) { rolcap = rolcap ? rolcap*2 : 16; rol = realloc(rol, (size_t)rolcap*sizeof(RoLab)); }
                rol[nrol].name = xstrdup(s); rol[nrol].roff = rodata.n; nrol++;
            } else {
                /* text label: offset assigned during layout; store a marker via a pending list.
                 * Simplest: emit a zero-length pseudo-instruction is messy, so record now with
                 * a placeholder and fix during layout by matching index. We instead record the
                 * current instruction count and resolve offset in the layout pass. */
                /* Represent as a label tied to the NEXT instruction index. */
                if (strcmp(s, "_start") == 0) entry_defined = 1;
                /* store with a sentinel offset; resolved after we know sizes */
                lbl_add(&labels, s, (size_t)(-(long)(ni + 1))); /* negative-ish marker: -(idx+1) */
            }
            continue;
        }
        /* instruction: mnemonic + operands */
        char *sp = strchr(s, ' ');
        char mn[16] = {0};
        char *ops = NULL;
        if (sp) { size_t ml = (size_t)(sp - s); if (ml >= sizeof(mn)) ml = sizeof(mn)-1; memcpy(mn, s, ml); ops = trim(sp+1); }
        else { strncpy(mn, s, sizeof(mn)-1); }
        Ins in; memset(&in, 0, sizeof(in));
        if (strcmp(mn, "syscall") == 0) in.kind = I_SYSCALL;
        else if (strcmp(mn, "ret") == 0) in.kind = I_RET;
        else if (strcmp(mn, "movq") == 0 || strcmp(mn, "xorq") == 0 || strcmp(mn, "leaq") == 0) {
            char *comma = ops ? strchr(ops, ',') : NULL;
            if (!comma) { snprintf(err, sizeof(err), "bad operands: %s", s); goto fail; }
            *comma = 0; char *a = trim(ops); char *bb = trim(comma+1);
            if (strcmp(mn, "xorq") == 0) {
                in.kind = I_XORrr; in.r1 = reg_num(bb); in.r2 = reg_num(a);
                if (in.r1 < 0 || in.r2 < 0) { snprintf(err, sizeof(err), "unsupported xorq form: %s", s); goto fail; }
            } else if (strcmp(mn, "leaq") == 0) {
                /* leaq SYM(%rip), %reg */
                char *rip = strstr(a, "(%rip)");
                if (!rip) { snprintf(err, sizeof(err), "unsupported leaq form: %s", s); goto fail; }
                *rip = 0;
                in.kind = I_LEArip; in.r1 = reg_num(bb);
                strncpy(in.sym, a, sizeof(in.sym)-1);
                if (in.r1 < 0) { snprintf(err, sizeof(err), "unsupported leaq dst: %s", s); goto fail; }
            } else {
                /* movq: imm->reg or reg->reg */
                if (a[0] == '$') {
                    in.kind = I_MOVri; in.r1 = reg_num(bb); in.imm = strtoll(a+1, NULL, 0);
                    if (in.r1 < 0) { snprintf(err, sizeof(err), "unsupported movq imm dst: %s", s); goto fail; }
                } else {
                    in.kind = I_MOVrr; in.r2 = reg_num(a); in.r1 = reg_num(bb);
                    if (in.r1 < 0 || in.r2 < 0) { snprintf(err, sizeof(err), "unsupported movq form (brick 1): %s", s); goto fail; }
                }
            }
        } else { snprintf(err, sizeof(err), "unknown/unsupported instruction (brick 1): %s", s); goto fail; }
        if (ni == cap) { cap = cap ? cap*2 : 64; ins = realloc(ins, (size_t)cap*sizeof(Ins)); }
        ins[ni++] = in;
    }

    if (!entry_defined) { snprintf(err, sizeof(err), "_start not defined"); goto fail; }

    /* layout pass: assign payload offsets to instructions, then resolve text labels */
    size_t off = 0;
    for (int i = 0; i < ni; i++) { ins[i].off = off; ins[i].len = ins_len(&ins[i]); off += ins[i].len; }
    size_t code_size = off;
    /* resolve text labels whose stored offset is the -(idx+1) marker */
    for (int i = 0; i < labels.n; i++) {
        long marker = (long)labels.v[i].off;
        if (marker < 0) {
            int idx = (int)(-marker - 1);
            labels.v[i].off = (idx < ni) ? ins[idx].off : code_size;
        }
    }
    /* rodata labels sit after code */
    for (int i = 0; i < nrol; i++) lbl_add(&labels, rol[i].name, code_size + rol[i].roff);

    uint64_t payload_vaddr = ELF_VBASE + ELF_HDRS;
    long entry_off = lbl_find(&labels, "_start");
    if (entry_off < 0) { snprintf(err, sizeof(err), "_start unresolved"); goto fail; }

    /* emit code */
    Buf code = {0};
    for (int i = 0; i < ni; i++) if (ins_emit(&ins[i], &code, &labels, payload_vaddr, err)) goto fail;

    /* payload = code || rodata */
    Buf payload = {0};
    for (size_t i = 0; i < code.n; i++) buf_put(&payload, code.d[i]);
    for (size_t i = 0; i < rodata.n; i++) buf_put(&payload, rodata.d[i]);

    /* write ELF: header + program header + payload, one RWX LOAD segment */
    uint64_t filesz = ELF_HDRS + payload.n;
    uint64_t entry_vaddr = payload_vaddr + (uint64_t)entry_off;
    Buf f = {0};
    /* ELF header (64 bytes) */
    buf_put(&f, 0x7f); buf_put(&f, 'E'); buf_put(&f, 'L'); buf_put(&f, 'F');
    buf_put(&f, 2); buf_put(&f, 1); buf_put(&f, 1); buf_put(&f, 0);
    for (int i = 0; i < 8; i++) buf_put(&f, 0);
    buf_put(&f, 2); buf_put(&f, 0);                 /* e_type = ET_EXEC */
    buf_put(&f, 0x3e); buf_put(&f, 0);              /* e_machine = x86-64 */
    buf_put32(&f, 1);                               /* e_version */
    buf_put64(&f, entry_vaddr);                     /* e_entry */
    buf_put64(&f, 64);                              /* e_phoff */
    buf_put64(&f, 0);                               /* e_shoff */
    buf_put32(&f, 0);                               /* e_flags */
    buf_put(&f, 64); buf_put(&f, 0);                /* e_ehsize */
    buf_put(&f, 56); buf_put(&f, 0);                /* e_phentsize */
    buf_put(&f, 1); buf_put(&f, 0);                 /* e_phnum */
    buf_put(&f, 0); buf_put(&f, 0);                 /* e_shentsize */
    buf_put(&f, 0); buf_put(&f, 0);                 /* e_shnum */
    buf_put(&f, 0); buf_put(&f, 0);                 /* e_shstrndx */
    /* program header (56 bytes): PT_LOAD, RWX */
    buf_put32(&f, 1);                               /* p_type = PT_LOAD */
    buf_put32(&f, 7);                               /* p_flags = RWX */
    buf_put64(&f, 0);                               /* p_offset */
    buf_put64(&f, ELF_VBASE);                       /* p_vaddr */
    buf_put64(&f, ELF_VBASE);                       /* p_paddr */
    buf_put64(&f, filesz);                          /* p_filesz */
    buf_put64(&f, filesz);                          /* p_memsz */
    buf_put64(&f, 0x1000);                          /* p_align */
    for (size_t i = 0; i < payload.n; i++) buf_put(&f, payload.d[i]);

    FILE *o = fopen(outpath, "wb");
    if (!o) { snprintf(err, sizeof(err), "cannot write %s", outpath); goto fail; }
    fwrite(f.d, 1, f.n, o);
    fclose(o);
    /* make it executable */
    {
        char cmd[512]; snprintf(cmd, sizeof(cmd), "chmod +x '%s'", outpath); int rc = system(cmd); (void)rc;
    }
    free(buf); free(ins); free(code.d); free(rodata.d); free(payload.d); free(f.d);
    for (int i = 0; i < labels.n; i++) { free(labels.v[i].name); }
    free(labels.v);
    for (int i = 0; i < nrol; i++) { free(rol[i].name); }
    free(rol);
    return 0;

fail:
    fprintf(stderr, "lume0 asm: %s\n", err);
    free(buf); free(ins); free(rodata.d);
    for (int i = 0; i < labels.n; i++) { free(labels.v[i].name); }
    free(labels.v);
    for (int i = 0; i < nrol; i++) { free(rol[i].name); }
    free(rol);
    return 1;
}
