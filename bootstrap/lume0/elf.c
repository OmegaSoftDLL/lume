/* elf.c — an x86-64 assembler + static ELF64 writer for the bootstrap.
 * It turns the assembly that the Lume-written backend emits into a runnable
 * executable WITHOUT the system assembler (`as`) or linker (`ld`), writing the
 * machine-code bytes and the ELF container directly. This is how the project
 * drops its last external build dependency on the way to "no C and no assembly".
 *
 * It implements the AT&T-syntax instruction forms the backend emits. Any form
 * it does not understand is a hard error, never a silent miscompile. */
#ifndef _POSIX_C_SOURCE
#define _POSIX_C_SOURCE 200809L  /* expose POSIX fileno()/fchmod() under -std=c11 */
#endif
#include "lume0.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <sys/stat.h>

#define ELF_VBASE 0x400000ULL
#define ELF_HDRS  120          /* ELF header (64) + one program header (56) */

static char *xstrdup(const char *s) { size_t n = strlen(s) + 1; char *d = malloc(n); memcpy(d, s, n); return d; }

/* ---- byte buffer ---- */
typedef struct { uint8_t *d; size_t n, cap; } Buf;
static void buf_put(Buf *b, uint8_t x) {
    if (b->n == b->cap) { b->cap = b->cap ? b->cap * 2 : 256; b->d = realloc(b->d, b->cap); }
    b->d[b->n++] = x;
}
static void buf_put32(Buf *b, uint32_t x) { for (int i = 0; i < 4; i++) buf_put(b, (uint8_t)((x >> (8 * i)) & 0xff)); }
static void buf_put64(Buf *b, uint64_t x) { for (int i = 0; i < 8; i++) buf_put(b, (uint8_t)((x >> (8 * i)) & 0xff)); }

/* ---- symbol table ---- */
typedef struct { char *name; uint64_t vaddr; int known; } Sym;
typedef struct { Sym *v; int n, cap; } Syms;
static Sym *sym_get(Syms *t, const char *name) {
    for (int i = 0; i < t->n; i++) if (strcmp(t->v[i].name, name) == 0) return &t->v[i];
    if (t->n == t->cap) { t->cap = t->cap ? t->cap * 2 : 64; t->v = realloc(t->v, (size_t)t->cap * sizeof(Sym)); }
    t->v[t->n].name = xstrdup(name); t->v[t->n].vaddr = 0; t->v[t->n].known = 0;
    return &t->v[t->n++];
}

/* ---- registers: num + size ---- */
static int reg_lookup(const char *s, int *size) {
    if (s[0] != '%') return -1;
    s++;
    static const char *q[16] = { "rax","rcx","rdx","rbx","rsp","rbp","rsi","rdi","r8","r9","r10","r11","r12","r13","r14","r15" };
    for (int i = 0; i < 16; i++) if (strcmp(s, q[i]) == 0) { *size = 8; return i; }
    static const char *d[8] = { "eax","ecx","edx","ebx","esp","ebp","esi","edi" };
    for (int i = 0; i < 8; i++) if (strcmp(s, d[i]) == 0) { *size = 4; return i; }
    static const char *b[8] = { "al","cl","dl","bl","spl","bpl","sil","dil" };
    for (int i = 0; i < 8; i++) if (strcmp(s, b[i]) == 0) { *size = 1; return i; }
    return -1;
}

/* ---- operand ---- */
typedef enum { O_NONE, O_REG, O_IMM, O_MEM } OpType;
typedef struct {
    OpType type;
    int reg, size;        /* O_REG, or unused */
    long long imm;        /* O_IMM, or O_MEM disp/addend */
    int base, index, scale;
    int rip;
    char sym[64];
} Operand;

static int parse_operand(const char *s, Operand *o, char *err) {
    memset(o, 0, sizeof(*o)); o->index = -1; o->base = -1; o->scale = 1;
    if (s[0] == '$') { o->type = O_IMM; o->imm = strtoll(s + 1, NULL, 0); return 0; }
    if (s[0] == '%') { int sz; int r = reg_lookup(s, &sz); if (r < 0) { snprintf(err,128,"bad reg %s",s); return 1; } o->type = O_REG; o->reg = r; o->size = sz; return 0; }
    o->type = O_MEM;
    const char *lp = strchr(s, '(');
    char head[80] = {0};
    if (lp) { size_t hl = (size_t)(lp - s); if (hl >= sizeof(head)) hl = sizeof(head)-1; memcpy(head, s, hl); }
    else { strncpy(head, s, sizeof(head)-1); }
    if (head[0]) {
        char *plus = strrchr(head, '+');
        char *minus = strrchr(head, '-');
        char *op2 = plus ? plus : ((minus && minus != head) ? minus : NULL);
        if (head[0] == '-' || (head[0] >= '0' && head[0] <= '9')) {
            o->imm = strtoll(head, NULL, 0);
        } else if (op2) {
            size_t sl = (size_t)(op2 - head); if (sl >= sizeof(o->sym)) sl = sizeof(o->sym)-1;
            memcpy(o->sym, head, sl); o->sym[sl] = 0;
            o->imm = strtoll(op2, NULL, 0);
        } else {
            size_t hl = strlen(head); if (hl >= sizeof(o->sym)) hl = sizeof(o->sym)-1;
            memcpy(o->sym, head, hl); o->sym[hl] = 0;
        }
    }
    if (lp) {
        const char *rp = strchr(lp, ')');
        if (!rp) { snprintf(err,128,"bad mem %s",s); return 1; }
        char inner[64] = {0}; size_t il = (size_t)(rp - lp - 1); if (il >= sizeof(inner)) il = sizeof(inner)-1;
        memcpy(inner, lp + 1, il);
        if (strcmp(inner, "%rip") == 0) { o->rip = 1; return 0; }
        char *c1 = strchr(inner, ',');
        if (c1) {
            *c1 = 0; char *ipart = c1 + 1; char *spart = strchr(ipart, ',');
            int sc = 1;
            if (spart) { *spart = 0; sc = (int)strtol(spart + 1, NULL, 10); }
            int sz;
            o->base = reg_lookup(inner, &sz);
            o->index = reg_lookup(ipart, &sz); o->scale = sc;
            if (o->base < 0 || o->index < 0) { snprintf(err,128,"bad mem regs %s",s); return 1; }
        } else {
            int sz; o->base = reg_lookup(inner, &sz);
            if (o->base < 0) { snprintf(err,128,"bad mem base %s",s); return 1; }
        }
    }
    return 0;
}

typedef struct {
    char mn[12];
    Operand a, b;
    int nop;
    size_t off, len;
    char target[64];
} Ins;

static int scale_bits(int s) { return s == 2 ? 1 : s == 4 ? 2 : s == 8 ? 3 : 0; }

static int emit_rm(Buf *out, int opsize, const uint8_t *opc, int nopc, int rf, const Operand *rm,
                   Syms *syms, uint64_t insn_end, int strict, char *err) {
    uint8_t rex = 0x40;
    if (opsize == 8) rex |= 0x08;
    if (rf >= 8) rex |= 0x04;
    int modrm_mod = 0, modrm_rm = 0;
    int use_sib = 0; uint8_t sib = 0;
    long long disp = 0; int disp_sz = 0;
    int rip = 0;
    if (rm->type == O_REG) {
        if (rm->reg >= 8) rex |= 0x01;
        modrm_mod = 3; modrm_rm = rm->reg & 7;
    } else {
        if (rm->rip) { rip = 1; modrm_mod = 0; modrm_rm = 5; disp_sz = 4; }
        else {
            int base = rm->base, index = rm->index;
            if (base >= 8) rex |= 0x01;
            if (index >= 8) rex |= 0x02;
            if (index >= 0) { use_sib = 1; modrm_rm = 4; sib = (uint8_t)((scale_bits(rm->scale) << 6) | ((index & 7) << 3) | (base & 7)); }
            else { modrm_rm = base & 7; if (modrm_rm == 4) { use_sib = 1; sib = (uint8_t)((0 << 6) | (4 << 3) | 4); } }
            if (rm->imm == 0 && (base & 7) != 5) { modrm_mod = 0; disp_sz = 0; }
            else if (rm->imm >= -128 && rm->imm <= 127) { modrm_mod = 1; disp_sz = 1; }
            else { modrm_mod = 2; disp_sz = 4; }
            disp = rm->imm;
        }
    }
    if (rex != 0x40) buf_put(out, rex);
    for (int i = 0; i < nopc; i++) buf_put(out, opc[i]);
    buf_put(out, (uint8_t)((modrm_mod << 6) | ((rf & 7) << 3) | modrm_rm));
    if (use_sib) buf_put(out, sib);
    if (rip) {
        uint64_t target = 0;
        if (rm->sym[0]) { Sym *sy = sym_get(syms, rm->sym); if (sy->known) target = sy->vaddr + (uint64_t)rm->imm; else if (strict) { snprintf(err,128,"undefined symbol %s",rm->sym); return 1; } }
        buf_put32(out, (uint32_t)(int32_t)((uint64_t)target - insn_end));
    } else if (disp_sz == 1) buf_put(out, (uint8_t)(int8_t)disp);
    else if (disp_sz == 4) buf_put32(out, (uint32_t)(int32_t)disp);
    return 0;
}

static int alu_ext(const char *m) {
    if (!strncmp(m,"add",3)) return 0;
    if (!strncmp(m,"or",2)) return 1;
    if (!strncmp(m,"and",3)) return 4;
    if (!strncmp(m,"sub",3)) return 5;
    if (!strncmp(m,"xor",3)) return 6;
    if (!strncmp(m,"cmp",3)) return 7;
    return -1;
}
static int alu_store_op(const char *m) {
    if (!strncmp(m,"add",3)) return 0x01;
    if (!strncmp(m,"or",2)) return 0x09;
    if (!strncmp(m,"and",3)) return 0x21;
    if (!strncmp(m,"sub",3)) return 0x29;
    if (!strncmp(m,"xor",3)) return 0x31;
    if (!strncmp(m,"cmp",3)) return 0x39;
    return -1;
}
static int jcc_op(const char *m) {
    if (!strcmp(m,"je")||!strcmp(m,"jz")) return 0x84;
    if (!strcmp(m,"jne")||!strcmp(m,"jnz")) return 0x85;
    if (!strcmp(m,"jl")) return 0x8C;
    if (!strcmp(m,"jg")) return 0x8F;
    if (!strcmp(m,"jle")) return 0x8E;
    if (!strcmp(m,"jge")) return 0x8D;
    if (!strcmp(m,"js")) return 0x88;
    if (!strcmp(m,"jns")) return 0x89;
    return -1;
}
static int setcc_op(const char *m) {
    if (!strcmp(m,"sete")) return 0x94;
    if (!strcmp(m,"setne")) return 0x95;
    if (!strcmp(m,"setl")) return 0x9C;
    if (!strcmp(m,"setg")) return 0x9F;
    if (!strcmp(m,"setle")) return 0x9E;
    if (!strcmp(m,"setge")) return 0x9D;
    if (!strcmp(m,"sets")) return 0x98;
    if (!strcmp(m,"setns")) return 0x99;
    return -1;
}

static int emit_ins(Ins *in, Buf *out, Syms *syms, uint64_t payload_vaddr, int strict, char *err) {
    const char *m = in->mn;
    uint64_t insn_end = payload_vaddr + in->off + in->len;
    size_t L = strlen(m);
    char suf = L ? m[L-1] : 'q';
    int opsize = (suf=='q')?8:(suf=='l')?4:(suf=='b')?1:8;

    if (!strcmp(m,"syscall")) { buf_put(out,0x0F); buf_put(out,0x05); return 0; }
    if (!strcmp(m,"ret")) { buf_put(out,0xC3); return 0; }
    if (!strcmp(m,"cqto")) { buf_put(out,0x48); buf_put(out,0x99); return 0; }
    if (!strcmp(m,"pushq")||!strcmp(m,"popq")) {
        int r=in->a.reg; uint8_t base=(uint8_t)(!strcmp(m,"pushq")?0x50:0x58);
        if (r>=8) { buf_put(out,0x41); }
        buf_put(out,(uint8_t)(base+(r&7)));
        return 0;
    }
    if (!strcmp(m,"jmp")||!strcmp(m,"call")) {
        Sym *sy=sym_get(syms,in->target); uint64_t t=0;
        if (sy->known) t=sy->vaddr; else if (strict){snprintf(err,128,"undefined %s",in->target);return 1;}
        buf_put(out,(uint8_t)(!strcmp(m,"jmp")?0xE9:0xE8)); buf_put32(out,(uint32_t)(int32_t)((uint64_t)t-insn_end)); return 0;
    }
    if (jcc_op(m)>=0) {
        Sym *sy=sym_get(syms,in->target); uint64_t t=0;
        if (sy->known) t=sy->vaddr; else if (strict){snprintf(err,128,"undefined %s",in->target);return 1;}
        buf_put(out,0x0F); buf_put(out,(uint8_t)jcc_op(m)); buf_put32(out,(uint32_t)(int32_t)((uint64_t)t-insn_end)); return 0;
    }
    if (setcc_op(m)>=0) { uint8_t opc[2]={0x0F,(uint8_t)setcc_op(m)}; return emit_rm(out,1,opc,2,0,&in->a,syms,insn_end,strict,err); }
    if (!strcmp(m,"movzbq")||!strcmp(m,"movzbl")) { uint8_t opc[2]={0x0F,0xB6}; int os=!strcmp(m,"movzbq")?8:4; return emit_rm(out,os,opc,2,in->b.reg,&in->a,syms,insn_end,strict,err); }
    if (!strcmp(m,"negq")||!strcmp(m,"notq")||!strcmp(m,"mulq")||!strcmp(m,"divq")||!strcmp(m,"idivq")||!strcmp(m,"incq")||!strcmp(m,"decq")) {
        int ext; uint8_t op;
        if (!strcmp(m,"incq")){ext=0;op=0xFF;} else if (!strcmp(m,"decq")){ext=1;op=0xFF;}
        else { op=0xF7; ext=!strcmp(m,"negq")?3:!strcmp(m,"notq")?2:!strcmp(m,"mulq")?4:!strcmp(m,"divq")?6:7; }
        uint8_t opc[1]={op}; return emit_rm(out,8,opc,1,ext,&in->a,syms,insn_end,strict,err);
    }
    if (!strcmp(m,"imulq")) {
        if (in->a.type==O_IMM){ uint8_t opc[1]={0x69}; if(emit_rm(out,8,opc,1,in->b.reg,&in->b,syms,insn_end,strict,err))return 1; buf_put32(out,(uint32_t)(int32_t)in->a.imm); return 0; }
        uint8_t opc[2]={0x0F,0xAF}; return emit_rm(out,8,opc,2,in->b.reg,&in->a,syms,insn_end,strict,err);
    }
    if (!strncmp(m,"mov",3) && (suf=='q'||suf=='l'||suf=='b')) {
        if (in->a.type==O_IMM){
            /* a 64-bit immediate to a register that does not fit in a sign-extended
             * imm32 needs movabs (REX.W B8+r imm64); C7 would truncate it. */
            if (opsize==8 && in->b.type==O_REG && (in->a.imm < -2147483648LL || in->a.imm > 2147483647LL)) {
                uint8_t rex=0x48; if(in->b.reg>=8) rex|=0x01; buf_put(out,rex);
                buf_put(out,(uint8_t)(0xB8+(in->b.reg&7)));
                buf_put64(out,(uint64_t)in->a.imm);
                return 0;
            }
            uint8_t opc[1]={(uint8_t)((opsize==1)?0xC6:0xC7)}; if(emit_rm(out,opsize,opc,1,0,&in->b,syms,insn_end,strict,err))return 1; if(opsize==1)buf_put(out,(uint8_t)(int8_t)in->a.imm);else buf_put32(out,(uint32_t)(int32_t)in->a.imm); return 0; }
        if (in->a.type==O_REG && in->b.type==O_REG){ uint8_t opc[1]={(uint8_t)((opsize==1)?0x88:0x89)}; return emit_rm(out,opsize,opc,1,in->a.reg,&in->b,syms,insn_end,strict,err); }
        if (in->a.type==O_REG && in->b.type==O_MEM){ uint8_t opc[1]={(uint8_t)((opsize==1)?0x88:0x89)}; return emit_rm(out,opsize,opc,1,in->a.reg,&in->b,syms,insn_end,strict,err); }
        if (in->a.type==O_MEM && in->b.type==O_REG){ uint8_t opc[1]={(uint8_t)((opsize==1)?0x8A:0x8B)}; return emit_rm(out,opsize,opc,1,in->b.reg,&in->a,syms,insn_end,strict,err); }
    }
    if (!strcmp(m,"leaq")){ uint8_t opc[1]={0x8D}; return emit_rm(out,8,opc,1,in->b.reg,&in->a,syms,insn_end,strict,err); }
    if (!strcmp(m,"testq")||!strcmp(m,"testb")){ uint8_t opc[1]={(uint8_t)(!strcmp(m,"testb")?0x84:0x85)}; int os=!strcmp(m,"testb")?1:8; return emit_rm(out,os,opc,1,in->a.reg,&in->b,syms,insn_end,strict,err); }
    {
        char core[8]={0}; size_t cl=L>0?L-1:0; if(cl>=sizeof(core))cl=sizeof(core)-1; memcpy(core,m,cl);
        int ext=alu_ext(core); int sop=alu_store_op(core);
        if (ext>=0 && (suf=='q'||suf=='l'||suf=='b')) {
            if (in->a.type==O_IMM){ uint8_t opc[1]={(uint8_t)((opsize==1)?0x80:0x81)}; if(emit_rm(out,opsize,opc,1,ext,&in->b,syms,insn_end,strict,err))return 1; if(opsize==1)buf_put(out,(uint8_t)(int8_t)in->a.imm);else buf_put32(out,(uint32_t)(int32_t)in->a.imm); return 0; }
            if (in->a.type==O_REG && (in->b.type==O_REG||in->b.type==O_MEM)){ uint8_t opc[1]={(uint8_t)((opsize==1)?(sop-1):sop)}; return emit_rm(out,opsize,opc,1,in->a.reg,&in->b,syms,insn_end,strict,err); }
            if (in->a.type==O_MEM && in->b.type==O_REG){ uint8_t opc[1]={(uint8_t)((opsize==1)?(sop-1+2):(sop+2))}; return emit_rm(out,opsize,opc,1,in->b.reg,&in->a,syms,insn_end,strict,err); }
        }
    }
    snprintf(err,128,"unsupported instruction: %s", in->mn);
    return 1;
}

static char *trim(char *s) {
    while (*s==' '||*s=='\t') s++;
    size_t n=strlen(s); while (n>0 && (s[n-1]==' '||s[n-1]=='\t'||s[n-1]=='\r'||s[n-1]=='\n')) s[--n]=0;
    return s;
}

int elf_assemble(const char *src, const char *outpath) {
    char err[160]={0};
    char *buf = xstrdup(src);
    Ins *ins=NULL; int ni=0, cap=0;
    Syms syms={0};
    Buf rodata={0};
    typedef struct { char *name; size_t roff; } RoLab; RoLab *rol=NULL; int nrol=0,rolcap=0;
    typedef struct { char *name; size_t off, size; } Bss; Bss *bss=NULL; int nbss=0,bsscap=0; size_t bss_total=0;
    typedef struct { char *name; int idx; } TLab; TLab *tl=NULL; int ntl=0,tlcap=0;
    int section=0; int entry=0;

    for (char *line=strtok(buf,"\n"); line; line=strtok(NULL,"\n")) {
        char *s=trim(line);
        if (*s==0 || *s=='#') continue;
        { size_t Lz=strlen(s);
          if (Lz>0 && s[Lz-1]==':') {
            s[Lz-1]=0;
            if (section==1){ if(nrol==rolcap){rolcap=rolcap?rolcap*2:16;rol=realloc(rol,(size_t)rolcap*sizeof(RoLab));} rol[nrol].name=xstrdup(s);rol[nrol].roff=rodata.n;nrol++; }
            else { if(!strcmp(s,"_start"))entry=1; if(ntl==tlcap){tlcap=tlcap?tlcap*2:64;tl=realloc(tl,(size_t)tlcap*sizeof(TLab));} tl[ntl].name=xstrdup(s);tl[ntl].idx=ni;ntl++; }
            continue;
          }
        }
        if (s[0]=='.') {
            if (!strncmp(s,".section .rodata",16)){section=1;continue;}
            if (!strncmp(s,".section .text",14)){section=0;continue;}
            if (!strncmp(s,".section .bss",13)){section=2;continue;}
            if (!strncmp(s,".globl",6)) continue;
            if (!strncmp(s,".lcomm",6)) {
                char *p=trim(s+6); char *c=strchr(p,','); size_t sz=8;
                if (c){*c=0; sz=(size_t)strtoull(trim(c+1),NULL,10);} char *nm=trim(p);
                if (nbss==bsscap){bsscap=bsscap?bsscap*2:16;bss=realloc(bss,(size_t)bsscap*sizeof(Bss));}
                bss[nbss].name=xstrdup(nm); bss[nbss].off=bss_total; bss[nbss].size=sz; nbss++; bss_total+=sz;
                continue;
            }
            if (!strncmp(s,".asciz",6)||!strncmp(s,".ascii",6)) {
                int z=(s[5]=='z'); char *q=strchr(s,'"');
                if(!q)continue;
                q++;
                while(*q&&*q!='"'){char c=*q++; if(c=='\\'){char e=*q++; if(e=='n')c='\n';else if(e=='t')c='\t';else if(e=='0')c='\0';else if(e=='r')c='\r';else c=e;} buf_put(&rodata,(uint8_t)c);}
                if(z){buf_put(&rodata,0);}
                continue;
            }
            continue;
        }
        char *sp=strchr(s,' ');
        Ins in; memset(&in,0,sizeof(in));
        if (sp){ size_t ml=(size_t)(sp-s); if(ml>=sizeof(in.mn))ml=sizeof(in.mn)-1; memcpy(in.mn,s,ml); char *ops=trim(sp+1);
            if (!strcmp(in.mn,"jmp")||!strcmp(in.mn,"call")||jcc_op(in.mn)>=0) {
                strncpy(in.target,ops,sizeof(in.target)-1); in.nop=1;
                /* GAS numeric local labels: `1f`/`1b` both refer to label `1` (unique here) */
                size_t tl2=strlen(in.target);
                if (tl2>=2 && (in.target[tl2-1]=='f'||in.target[tl2-1]=='b')) {
                    int alldig=1; for(size_t k=0;k<tl2-1;k++) if(in.target[k]<'0'||in.target[k]>'9'){alldig=0;break;}
                    if (alldig) in.target[tl2-1]=0;
                }
            }
            else {
                /* split on the comma at parenthesis depth 0 (mem operands contain commas) */
                char *comma=NULL; int depth=0;
                for (char *p=ops; *p; p++){ if(*p=='(')depth++; else if(*p==')')depth--; else if(*p==','&&depth==0){comma=p;break;} }
                if (comma){*comma=0; char *a=trim(ops); char *b=trim(comma+1); if(parse_operand(a,&in.a,err)||parse_operand(b,&in.b,err))goto fail; in.nop=2;}
                else { if(parse_operand(ops,&in.a,err))goto fail; in.nop=1; }
            }
        } else { strncpy(in.mn,s,sizeof(in.mn)-1); in.nop=0; }
        if(ni==cap){cap=cap?cap*2:128;ins=realloc(ins,(size_t)cap*sizeof(Ins));}
        ins[ni++]=in;
    }
    if(!entry){snprintf(err,sizeof(err),"_start not defined");goto fail;}

    {
        size_t off=0;
        for(int i=0;i<ni;i++){ Buf tmp={0}; ins[i].off=off; ins[i].len=0;
            if(emit_ins(&ins[i],&tmp,&syms,0,0,err)){free(tmp.d);goto fail;}
            ins[i].len=tmp.n; off+=tmp.n; free(tmp.d);
        }
        size_t code_size=off;
        for(int i=0;i<ntl;i++){ Sym *sy=sym_get(&syms,tl[i].name); sy->vaddr=ELF_VBASE+ELF_HDRS+((tl[i].idx<ni)?ins[tl[i].idx].off:code_size); sy->known=1; }
        for(int i=0;i<nrol;i++){ Sym *sy=sym_get(&syms,rol[i].name); sy->vaddr=ELF_VBASE+ELF_HDRS+code_size+rol[i].roff; sy->known=1; }
        size_t data_end=code_size+rodata.n;
        for(int i=0;i<nbss;i++){ Sym *sy=sym_get(&syms,bss[i].name); sy->vaddr=ELF_VBASE+ELF_HDRS+data_end+bss[i].off; sy->known=1; }

        Buf code={0};
        for(int i=0;i<ni;i++){ if(emit_ins(&ins[i],&code,&syms,ELF_VBASE+ELF_HDRS,1,err)){free(code.d);goto fail;} }
        if(code.n!=code_size){ snprintf(err,sizeof(err),"internal size mismatch %zu vs %zu",code.n,code_size); free(code.d); goto fail; }

        Buf payload={0};
        for(size_t i=0;i<code.n;i++)buf_put(&payload,code.d[i]);
        for(size_t i=0;i<rodata.n;i++)buf_put(&payload,rodata.d[i]);
        uint64_t filesz=ELF_HDRS+payload.n;
        uint64_t memsz=filesz+bss_total;
        long es=-1; { Sym *sy=sym_get(&syms,"_start"); if(sy->known)es=(long)sy->vaddr; }
        if(es<0){snprintf(err,sizeof(err),"_start unresolved");free(code.d);free(payload.d);goto fail;}

        Buf f={0};
        buf_put(&f,0x7f);buf_put(&f,'E');buf_put(&f,'L');buf_put(&f,'F');
        buf_put(&f,2);buf_put(&f,1);buf_put(&f,1);buf_put(&f,0);
        for(int i=0;i<8;i++)buf_put(&f,0);
        buf_put(&f,2);buf_put(&f,0); buf_put(&f,0x3e);buf_put(&f,0);
        buf_put32(&f,1); buf_put64(&f,(uint64_t)es); buf_put64(&f,64); buf_put64(&f,0);
        buf_put32(&f,0); buf_put(&f,64);buf_put(&f,0); buf_put(&f,56);buf_put(&f,0);
        buf_put(&f,1);buf_put(&f,0); buf_put(&f,0);buf_put(&f,0); buf_put(&f,0);buf_put(&f,0); buf_put(&f,0);buf_put(&f,0);
        buf_put32(&f,1); buf_put32(&f,7); buf_put64(&f,0); buf_put64(&f,ELF_VBASE); buf_put64(&f,ELF_VBASE);
        buf_put64(&f,filesz); buf_put64(&f,memsz); buf_put64(&f,0x1000);
        for(size_t i=0;i<payload.n;i++)buf_put(&f,payload.d[i]);

        FILE *o=fopen(outpath,"wb"); if(!o){snprintf(err,sizeof(err),"cannot write %s",outpath);free(code.d);free(payload.d);free(f.d);goto fail;}
        fwrite(f.d,1,f.n,o);
        fchmod(fileno(o), 0755); /* on the open fd, before close: no shell (CodeQL #2) and no path TOCTOU race */
        fclose(o);
        free(code.d); free(payload.d); free(f.d);
    }

    free(buf); free(ins); free(rodata.d);
    for(int i=0;i<syms.n;i++){ free(syms.v[i].name); } free(syms.v);
    for(int i=0;i<nrol;i++){ free(rol[i].name); } free(rol);
    for(int i=0;i<nbss;i++){ free(bss[i].name); } free(bss);
    for(int i=0;i<ntl;i++){ free(tl[i].name); } free(tl);
    return 0;
fail:
    fprintf(stderr,"lume0 asm: %s\n",err);
    free(buf); free(ins); free(rodata.d);
    for(int i=0;i<syms.n;i++){ free(syms.v[i].name); } free(syms.v);
    for(int i=0;i<nrol;i++){ free(rol[i].name); } free(rol);
    for(int i=0;i<nbss;i++){ free(bss[i].name); } free(bss);
    for(int i=0;i<ntl;i++){ free(tl[i].name); } free(tl);
    return 1;
}
