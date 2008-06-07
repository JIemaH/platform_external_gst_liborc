
#include "config.h"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include <unistd.h>
#include <sys/types.h>
#include <sys/mman.h>

#include <orc/orcprogram.h>
#include <orc/x86.h>

#define SIZE 65536

/* mmx rules */

void
mmx_emit_loadi_s16 (OrcProgram *p, int reg, int value)
{
  if (value == 0) {
    printf("  pxor %%%s, %%%s\n", x86_get_regname_mmx(reg),
        x86_get_regname_mmx(reg));
    *p->codeptr++ = 0x0f;
    *p->codeptr++ = 0xef;
    x86_emit_modrm_reg (p, reg, reg);
  } else {
#if 1
    x86_emit_mov_imm_reg (p, 4, value, X86_ECX);

    printf("  movd %%ecx, %%%s\n", x86_get_regname_mmx(reg));
    *p->codeptr++ = 0x0f;
    *p->codeptr++ = 0x6e;
    x86_emit_modrm_reg (p, X86_ECX, reg);
#else
    printf("  pinsrw $0, %%ecx, %%%s\n", x86_get_regname_mmx(reg));
#endif

    printf("  pshufw $0, %%%s, %%%s\n", x86_get_regname_mmx(reg),
        x86_get_regname_mmx(reg));

    *p->codeptr++ = 0x0f;
    *p->codeptr++ = 0x70;
    x86_emit_modrm_reg (p, reg, reg);
    *p->codeptr++ = 0x00;
  }
}

static void
mmx_rule_loadi_s16 (OrcProgram *p, void *user, OrcInstruction *insn)
{
  mmx_emit_loadi_s16 (p, p->vars[insn->args[0]].alloc,
      p->vars[insn->args[2]].s16);
}

static void
mmx_rule_add_s16 (OrcProgram *p, void *user, OrcInstruction *insn)
{
  printf("  paddw %%%s, %%%s\n",
      x86_get_regname_mmx(p->vars[insn->args[2]].alloc),
      x86_get_regname_mmx(p->vars[insn->args[0]].alloc));

  *p->codeptr++ = 0x0f;
  *p->codeptr++ = 0xfd;
  x86_emit_modrm_reg (p, p->vars[insn->args[2]].alloc,
      p->vars[insn->args[0]].alloc);
}

static void
mmx_rule_sub_s16 (OrcProgram *p, void *user, OrcInstruction *insn)
{
  printf("  psubw %%%s, %%%s\n",
      x86_get_regname_mmx(p->vars[insn->args[2]].alloc),
      x86_get_regname_mmx(p->vars[insn->args[0]].alloc));

  *p->codeptr++ = 0x0f;
  *p->codeptr++ = 0xf9;
  x86_emit_modrm_reg (p, p->vars[insn->args[2]].alloc,
      p->vars[insn->args[0]].alloc);
}

static void
mmx_rule_mul_s16 (OrcProgram *p, void *user, OrcInstruction *insn)
{
  printf("  pmullw %%%s, %%%s\n",
      x86_get_regname_mmx(p->vars[insn->args[2]].alloc),
      x86_get_regname_mmx(p->vars[insn->args[0]].alloc));

  *p->codeptr++ = 0x0f;
  *p->codeptr++ = 0xd5;
  x86_emit_modrm_reg (p, p->vars[insn->args[2]].alloc,
      p->vars[insn->args[0]].alloc);
}

static void
mmx_rule_lshift_s16 (OrcProgram *p, void *user, OrcInstruction *insn)
{
  if (p->vars[insn->args[2]].vartype == ORC_VAR_TYPE_CONST) {
    printf("  psllw $%d, %%%s\n",
        p->vars[insn->args[2]].s16,
        x86_get_regname_mmx(p->vars[insn->args[0]].alloc));

    *p->codeptr++ = 0x0f;
    *p->codeptr++ = 0x71;
    x86_emit_modrm_reg (p, p->vars[insn->args[0]].alloc, 6);
    *p->codeptr++ = p->vars[insn->args[2]].s16;
  } else {
    /* FIXME this doesn't work quite right */
    printf("  psllw %%%s, %%%s\n",
        x86_get_regname_mmx(p->vars[insn->args[2]].alloc),
        x86_get_regname_mmx(p->vars[insn->args[0]].alloc));

    *p->codeptr++ = 0x0f;
    *p->codeptr++ = 0xf1;
    x86_emit_modrm_reg (p, p->vars[insn->args[0]].alloc,
        p->vars[insn->args[2]].alloc);
  }
}

static void
mmx_rule_rshift_s16 (OrcProgram *p, void *user, OrcInstruction *insn)
{
  if (p->vars[insn->args[2]].vartype == ORC_VAR_TYPE_CONST) {
    printf("  psraw $%d, %%%s\n",
        p->vars[insn->args[2]].s16,
        x86_get_regname_mmx(p->vars[insn->args[0]].alloc));

    *p->codeptr++ = 0x0f;
    *p->codeptr++ = 0x71;
    x86_emit_modrm_reg (p, p->vars[insn->args[0]].alloc, 4);
    *p->codeptr++ = p->vars[insn->args[2]].s16;
  } else {
    /* FIXME this doesn't work quite right */
    printf("  psraw %%%s, %%%s\n",
        x86_get_regname_mmx(p->vars[insn->args[2]].alloc),
        x86_get_regname_mmx(p->vars[insn->args[0]].alloc));

    *p->codeptr++ = 0x0f;
    *p->codeptr++ = 0xe1;
    x86_emit_modrm_reg (p, p->vars[insn->args[0]].alloc,
        p->vars[insn->args[2]].alloc);
  }
}

void
orc_program_mmx_register_rules (void)
{
  int i;

  orc_rule_register ("_loadi_s16", ORC_RULE_MMX_4, mmx_rule_loadi_s16, NULL,
      ORC_RULE_REG_IMM);

  for(i=ORC_RULE_MMX_1; i <= ORC_RULE_MMX_4; i++) {
    orc_rule_register ("add_s16", i, mmx_rule_add_s16, NULL,
        ORC_RULE_REG_REG);
    orc_rule_register ("sub_s16", i, mmx_rule_sub_s16, NULL,
        ORC_RULE_REG_REG);
    orc_rule_register ("mul_s16", i, mmx_rule_mul_s16, NULL,
        ORC_RULE_REG_REG);
    orc_rule_register ("lshift_s16", i, mmx_rule_lshift_s16, NULL,
        ORC_RULE_REG_REG);
    orc_rule_register ("rshift_s16", i, mmx_rule_rshift_s16, NULL,
        ORC_RULE_REG_REG);
  }
}

