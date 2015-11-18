/******************************** -*- C -*- ****************************
 *
 *	Support macros for the i386 math coprocessor
 *
 ***********************************************************************/


/***********************************************************************
 *
 * Copyright 2000,2001,2002,2004,2008,2010 Free Software Foundation, Inc.
 *
 * This file is part of GNU lightning.
 *
 * GNU lightning is free software; you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License as published
 * by the Free Software Foundation; either version 3, or (at your option)
 * any later version.
 * 
 * GNU lightning is distributed in the hope that it will be useful, but 
 * WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
 * or  FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public
 * License for more details.
 * 
 * You should have received a copy of the GNU Lesser General Public License
 * along with GNU lightning; see the file COPYING.LESSER; if not, write to the
 * Free Software Foundation, 51 Franklin Street, Fifth Floor, Boston,
 * MA 02110-1301, USA.
 *
 * Authors:
 *	Paolo Bonzini
 *	Paulo Cesar Pereira de Andrade
 ***********************************************************************/

#ifndef __lightning_fp_x87_h
#define __lightning_fp_x87_h

/* We really must map the x87 stack onto a flat register file.  In practice,
   we can provide something sensible and make it work on the x86 using the
   stack like a file of eight registers.

   We use six or seven registers so as to have some freedom
   for floor, ceil, round, (and log, tan, atn and exp).

   Not hard at all, basically play with FXCH.  FXCH is mostly free,
   so the generated code is not bad.  Of course we special case when one
   of the operands turns out to be ST0.  */

/* - moves:

	move FPR0 to FPR3
		FST  ST3

	move FPR3 to FPR0
		FXCH ST3
		FST  ST3

	move FPR3 to FPR1
                FLD  ST3
                FSTP ST2   Stack is rotated, so FPRn becomes STn+1 */

/* - loads:

	load into FPR0
		FSTP ST0
		FLD  [FUBAR]

	load into FPR3
		FSTP ST3     Save old st0 into destination register
		FLD  [FUBAR]
		FXCH ST3     Get back old st0

   (and similarly for immediates, using the stack) */

__jit_inline void
x87_absr_d(jit_state_t _jit, jit_fpr_t f0, jit_fpr_t f1)
{
    if (f0 == f1) {
	if (f0 == _ST0)
	    FABS_();
	else {
	    FXCHr(f0);
	    FABS_();
	    FXCHr(f0);
	}
    }
    else {
	FLDr(f1);
	FABS_();
	FSTPr((jit_fpr_t)(f0 + 1));
    }
}

__jit_inline void
x87_negr_d(jit_state_t _jit, jit_fpr_t f0, jit_fpr_t f1)
{
    if (f0 == f1) {
	if (f0 == _ST0)
	    FCHS_();
	else {
	    FXCHr(f0);
	    FCHS_();
	    FXCHr(f0);
	}
    }
    else {
	FLDr(f1);
	FCHS_();
	FSTPr((jit_fpr_t)(f0 + 1));
    }
}

__jit_inline void
x87_sqrtr_d(jit_state_t _jit, jit_fpr_t f0, jit_fpr_t f1)
{
    if (f0 == f1) {
	if (f0 == _ST0)
	    FSQRT_();
	else {
	    FXCHr(f0);
	    FSQRT_();
	    FXCHr(f0);
	}
    }
    else {
	FLDr(f1);
	FSQRT_();
	FSTPr((jit_fpr_t)(f0 + 1));
    }
}

__jit_inline void
x87_sin(jit_state_t _jit)
{
    jit_insn	*n_label;
    jit_insn	*f_label;
    jit_insn	*r_label;

    /* save %rax */
#if __WORDSIZE == 32
    jit_pushr_i(_RAX);
#else
    jit_movr_l(JIT_REXTMP, _RAX);
#endif
    /* classify argument */
    FXAM_();
    /* do nothing if zero or unordered */
    FNSTSWr(_RAX);
    TESTWir(FPSW_NAN | FPSW_ZERO, _RAX);
    JNZSm(_jit->x.pc);
    n_label = _jit->x.pc;
    /* calculate once */
    FSIN_();
    FNSTSWr(_RAX);
    /* if C2 is set, it means the value is out of range */
    TESTWir(FPSW_FINITE, _RAX);
    JZSm(_jit->x.pc);
    f_label = _jit->x.pc;
    /* load pi*2 */
    FLDPI_();
    FADDrr(_ST0, _ST0);
    /* swap top of stack */
    FXCHr(_ST1);
    /* calculate remainder */
    r_label = _jit->x.pc;
    FPREM1_();
    FNSTSWr(_RAX);
    /* if C2 is set, it means the value is partial */
    TESTWir(FPSW_FINITE, _RAX);
    JNZSm(r_label);
    /* value in range now */
    FSTPr(_ST1);
    FSIN_();
    jit_patch_rel_char_at(n_label, _jit->x.pc);
    jit_patch_rel_char_at(f_label, _jit->x.pc);
    /* restore %rax */
#if __WORDSIZE == 32
    jit_popr_i(_RAX);
#else
    jit_movr_l(_RAX, JIT_REXTMP);
#endif
}

__jit_inline void
x87_sinr_d(jit_state_t _jit, jit_fpr_t f0, jit_fpr_t f1)
{
    if (f0 == f1) {
	if (f0 == _ST0)
	    x87_sin(_jit);
	else {
	    FXCHr(f0);
	    x87_sin(_jit);
	    FXCHr(f0);
	}
    }
    else {
	FLDr(f1);
	x87_sin(_jit);
	FSTPr((jit_fpr_t)(f0 + 1));
    }
}

__jit_inline void
x87_cos(jit_state_t _jit)
{
    jit_insn	*n_label;
    jit_insn	*f_label;
    jit_insn	*r_label;

    /* save %rax */
#if __WORDSIZE == 32
    jit_pushr_i(_RAX);
#else
    jit_movr_l(JIT_REXTMP, _RAX);
#endif
    /* classify argument */
    FXAM_();
    /* do nothing if unordered */
    FNSTSWr(_RAX);
    TESTWir(FPSW_NAN, _RAX);
    JNZSm(_jit->x.pc);
    n_label = _jit->x.pc;
    /* calculate once */
    FCOS_();
    FNSTSWr(_RAX);
    /* if C2 is set, it means the value is out of range */
    TESTWir(FPSW_FINITE, _RAX);
    JZSm(_jit->x.pc);
    f_label = _jit->x.pc;
    /* load pi*2 */
    FLDPI_();
    FADDrr(_ST0, _ST0);
    /* swap top of stack */
    FXCHr(_ST1);
    /* calculate remainder */
    r_label = _jit->x.pc;
    FPREM1_();
    FNSTSWr(_RAX);
    /* if C2 is set, it means the value is partial */
    TESTWir(FPSW_FINITE, _RAX);
    JNZSm(r_label);
    /* value in range now */
    FSTPr(_ST1);
    FCOS_();
    jit_patch_rel_char_at(n_label, _jit->x.pc);
    jit_patch_rel_char_at(f_label, _jit->x.pc);
    /* restore %rax */
#if __WORDSIZE == 32
    jit_popr_i(_RAX);
#else
    jit_movr_l(_RAX, JIT_REXTMP);
#endif
}

__jit_inline void
x87_cosr_d(jit_state_t _jit, jit_fpr_t f0, jit_fpr_t f1)
{
    if (f0 == f1) {
	if (f0 == _ST0)
	    x87_cos(_jit);
	else {
	    FXCHr(f0);
	    x87_cos(_jit);
	    FXCHr(f0);
	}
    }
    else {
	FLDr(f1);
	x87_cos(_jit);
	FSTPr((jit_fpr_t)(f0 + 1));
    }
}

__jit_inline void
x87_tan(jit_state_t _jit)
{
    jit_insn	*n_label;
    jit_insn	*f_label;
    jit_insn	*r_label;

    /* save %rax */
#if __WORDSIZE == 32
    jit_pushr_i(_RAX);
#else
    jit_movr_l(JIT_REXTMP, _RAX);
#endif
    /* classify argument */
    FXAM_();
    /* do nothing if zero or unordered */
    FNSTSWr(_RAX);
    TESTWir(FPSW_NAN | FPSW_ZERO, _RAX);
    JNZSm(_jit->x.pc);
    n_label = _jit->x.pc;
    /* calculate once */
    FPTAN_();
    FNSTSWr(_RAX);
    /* if C2 is set, it means the value is out of range */
    TESTWir(FPSW_FINITE, _RAX);
    JZSm(_jit->x.pc);
    f_label = _jit->x.pc;
    /* load pi*2 */
    FLDPI_();
    FADDrr(_ST0, _ST0);
    /* swap top of stack */
    FXCHr(_ST1);
    /* calculate remainder */
    r_label = _jit->x.pc;
    FPREM1_();
    FNSTSWr(_RAX);
    /* if C2 is set, it means the value is partial */
    TESTWir(FPSW_FINITE, _RAX);
    JNZSm(r_label);
    /* value in range now */
    FSTPr(_ST1);
    FPTAN_();
    jit_patch_rel_char_at(f_label, _jit->x.pc);
    /* remove 1.0 from top of stack */
    FSTPr(_ST0);
    /* 1.0 not in stack if argument is NaN or zero */
    jit_patch_rel_char_at(n_label, _jit->x.pc);
    /* restore %rax */
#if __WORDSIZE == 32
    jit_popr_i(_RAX);
#else
    jit_movr_l(_RAX, JIT_REXTMP);
#endif
}

__jit_inline void
x87_tanr_d(jit_state_t _jit, jit_fpr_t f0, jit_fpr_t f1)
{
    if (f0 == f1) {
	if (f0 == _ST0)
	    x87_tan(_jit);
	else {
	    FXCHr(f0);
	    x87_tan(_jit);
	    FXCHr(f0);
	}
    }
    else {
	FLDr(f1);
	x87_tan(_jit);
	FSTPr((jit_fpr_t)(f0 + 1));
    }
}

__jit_inline void
x87_atan(jit_state_t _jit)
{
    FLD1_();
    FPATAN_();
}

__jit_inline void
x87_atanr_d(jit_state_t _jit, jit_fpr_t f0, jit_fpr_t f1)
{
    if (f0 == f1) {
	if (f0 == _ST0)
	    x87_atan(_jit);
	else {
	    FXCHr(f0);
	    x87_atan(_jit);
	    FXCHr(f0);
	}
    }
    else {
	FLDr(f1);
	x87_atan(_jit);
	FSTPr((jit_fpr_t)(f0 + 1));
    }
}

__jit_inline void
x87_log(jit_state_t _jit)
{
    FLDLN2_();
    FXCHr(_ST1);
    FYL2X_();
}

__jit_inline void
x87_logr_d(jit_state_t _jit, jit_fpr_t f0, jit_fpr_t f1)
{
    if (f0 == f1) {
	if (f0 == _ST0)
	    x87_log(_jit);
	else {
	    FXCHr(f0);
	    x87_log(_jit);
	    FXCHr(f0);
	}
    }
    else {
	FLDr(f1);
	x87_log(_jit);
	FSTPr((jit_fpr_t)(f0 + 1));
    }
}

__jit_inline void
x87_log2(jit_state_t _jit)
{
    FLD1_();
    FXCHr(_ST1);
    FYL2X_();
}

__jit_inline void
x87_log2r_d(jit_state_t _jit, jit_fpr_t f0, jit_fpr_t f1)
{
    if (f0 == f1) {
	if (f0 == _ST0)
	    x87_log2(_jit);
	else {
	    FXCHr(f0);
	    x87_log2(_jit);
	    FXCHr(f0);
	}
    }
    else {
	FLDr(f1);
	x87_log2(_jit);
	FSTPr((jit_fpr_t)(f0 + 1));
    }
}

__jit_inline void
x87_log10(jit_state_t _jit)
{
    FLDLG2_();
    FXCHr(_ST1);
    FYL2X_();
}

__jit_inline void
x87_log10r_d(jit_state_t _jit, jit_fpr_t f0, jit_fpr_t f1)
{
    if (f0 == f1) {
	if (f0 == _ST0)
	    x87_log10(_jit);
	else {
	    FXCHr(f0);
	    x87_log10(_jit);
	    FXCHr(f0);
	}
    }
    else {
	FLDr(f1);
	x87_log10(_jit);
	FSTPr((jit_fpr_t)(f0 + 1));
    }
}

__jit_inline void
x87_addr_d(jit_state_t _jit, jit_fpr_t f0, jit_fpr_t f1, jit_fpr_t f2)
{
    if (f0 == f1) {
	if (f2 == _ST0)
	    FADDrr(_ST0, f0);
	else if (f0 == _ST0)
	    FADDrr(f2, _ST0);
	else {
	    FXCHr(f0);
	    FADDrr(f2, _ST0);
	    FXCHr(f0);
	}
    }
    else if (f0 == f2) {
	if (f1 == _ST0)
	    FADDrr(_ST0, f0);
	else if (f0 == _ST0)
	    FADDrr(f1, _ST0);
	else {
	    FXCHr(f0);
	    FADDrr(f1, _ST0);
	    FXCHr(f0);
	}
    }
    else {
	FLDr(f1);
	FADDrr((jit_fpr_t)(f2 + 1), _ST0);
	FSTPr((jit_fpr_t)(f0 + 1));
    }
}

__jit_inline void
x87_subr_d(jit_state_t _jit, jit_fpr_t f0, jit_fpr_t f1, jit_fpr_t f2)
{
    if (f0 == f1) {
	if (f2 == _ST0)
	    FSUBRrr(_ST0, f0);
	else if (f0 == _ST0)
	    FSUBrr(f2, _ST0);
	else {
	    FXCHr(f0);
	    FSUBrr(f2, _ST0);
	    FXCHr(f0);
	}
    }
    else if (f0 == f2) {
	if (f1 == _ST0)
	    FSUBrr(_ST0, f0);
	else if (f0 == _ST0)
	    FSUBRrr(f1, _ST0);
	else {
	    FXCHr(f0);
	    FSUBRrr(f1, _ST0);
	    FXCHr(f0);
	}
    }
    else {
	FLDr(f1);
	FSUBrr((jit_fpr_t)(f2 + 1), _ST0);
	FSTPr((jit_fpr_t)(f0 + 1));
    }
}

__jit_inline void
x87_mulr_d(jit_state_t _jit, jit_fpr_t f0, jit_fpr_t f1, jit_fpr_t f2)
{
    if (f0 == f1) {
	if (f2 == _ST0)
	    FMULrr(_ST0, f0);
	else if (f0 == _ST0)
	    FMULrr(f2, _ST0);
	else {
	    FXCHr(f0);
	    FMULrr(f2, _ST0);
	    FXCHr(f0);
	}
    }
    else if (f0 == f2) {
	if (f1 == _ST0)
	    FMULrr(_ST0, f0);
	else if (f0 == _ST0)
	    FMULrr(f1, _ST0);
	else {
	    FXCHr(f0);
	    FMULrr(f1, _ST0);
	    FXCHr(f0);
	}
    }
    else {
	FLDr(f1);
	FMULrr((jit_fpr_t)(f2 + 1), _ST0);
	FSTPr((jit_fpr_t)(f0 + 1));
    }
}

__jit_inline void
x87_divr_d(jit_state_t _jit, jit_fpr_t f0, jit_fpr_t f1, jit_fpr_t f2)
{
    if (f0 == f1) {
	if (f2 == _ST0)
	    FDIVRrr(_ST0, f0);
	else if (f0 == _ST0)
	    FDIVrr(f2, _ST0);
	else {
	    FXCHr(f0);
	    FDIVrr(f2, _ST0);
	    FXCHr(f0);
	}
    }
    else if (f0 == f2) {
	if (f1 == _ST0)
	    FDIVrr(_ST0, f0);
	else if (f0 == _ST0)
	    FDIVRrr(f1, _ST0);
	else {
	    FXCHr(f0);
	    FDIVRrr(f1, _ST0);
	    FXCHr(f0);
	}
    }
    else {
	FLDr(f1);
	FDIVrr((jit_fpr_t)(f2 + 1), _ST0);
	FSTPr((jit_fpr_t)(f0 + 1));
    }
}

__jit_inline void
x87_ldr_f(jit_state_t _jit, jit_fpr_t f0, jit_gpr_t r0)
{
    FLDSm(0, r0, _NOREG, _SCL1);
    FSTPr((jit_fpr_t)(f0 + 1));
}

__jit_inline void
x87_ldi_f(jit_state_t _jit, jit_fpr_t f0, void *i0)
{
#if __WORDSIZE == 64
    if (!jit_can_sign_extend_int_p((long)i0)) {
	jit_movi_l(JIT_REXTMP, (long)i0);
	FLDSm(0, JIT_REXTMP, _NOREG, _SCL1);
    }
    else
#endif
	FLDSm((long)i0, _NOREG, _NOREG, _SCL1);
    FSTPr((jit_fpr_t)(f0 + 1));
}

__jit_inline void
x87_ldxr_f(jit_state_t _jit, jit_fpr_t f0, jit_gpr_t r0, jit_gpr_t r1)
{
    FLDSm(0, r0, r1, _SCL1);
    FSTPr((jit_fpr_t)(f0 + 1));
}

__jit_inline void
x87_ldxi_f(jit_state_t _jit, jit_fpr_t f0, jit_gpr_t r0, long i0)
{
#if __WORDSIZE == 64
    if (!jit_can_sign_extend_int_p(i0)) {
	jit_movi_l(JIT_REXTMP, i0);
	FLDSm(0, r0, JIT_REXTMP, _SCL1);
    }
    else
#endif
	FLDSm(i0, r0, _NOREG, _SCL1);
    FSTPr((jit_fpr_t)(f0 + 1));
}

__jit_inline void
x87_ldi_d(jit_state_t _jit, jit_fpr_t f0, void *i0)
{
#if __WORDSIZE == 64
    if (!jit_can_sign_extend_int_p((long)i0)) {
	jit_movi_l(JIT_REXTMP, (long)i0);
	FLDLm(0, JIT_REXTMP, _NOREG, _SCL1);
    }
    else
#endif
	FLDLm((long)i0, _NOREG, _NOREG, _SCL1);
    FSTPr((jit_fpr_t)(f0 + 1));
}

__jit_inline void
x87_ldr_d(jit_state_t _jit, jit_fpr_t f0, jit_gpr_t r0)
{
    FLDLm(0, r0, _NOREG, _SCL1);
    FSTPr((jit_fpr_t)(f0 + 1));
}

__jit_inline void
x87_ldxr_d(jit_state_t _jit, jit_fpr_t f0, jit_gpr_t r0, jit_gpr_t r1)
{
    FLDLm(0, r0, r1, _SCL1);
    FSTPr((jit_fpr_t)(f0 + 1));
}

__jit_inline void
x87_ldxi_d(jit_state_t _jit, jit_fpr_t f0, jit_gpr_t r0, long i0)
{
#if __WORDSIZE == 64
    if (!jit_can_sign_extend_int_p(i0)) {
	jit_movi_l(JIT_REXTMP, i0);
	FLDLm(0, r0, JIT_REXTMP, _SCL1);
    }
    else
#endif
	FLDLm(i0, r0, _NOREG, _SCL1);
    FSTPr((jit_fpr_t)(f0 + 1));
}

__jit_inline void
x87_str_f(jit_state_t _jit, jit_gpr_t r0, jit_fpr_t f0)
{
    if (f0 == _ST0)
	FSTSm(0, r0, _NOREG, _SCL1);
    else {
	FXCHr(f0);
	FSTSm(0, r0, _NOREG, _SCL1);
	FXCHr(f0);
    }
}

__jit_inline void
x87_sti_f(jit_state_t _jit, void *i0, jit_fpr_t f0)
{
#if __WORDSIZE == 64
    if (!jit_can_sign_extend_int_p((long)i0)) {
	jit_movi_l(JIT_REXTMP, (long)i0);
	x87_str_f(_jit, JIT_REXTMP, f0);
	return;
    }
#endif
    if (f0 == _ST0)
	FSTSm((long)i0, _NOREG, _NOREG, _SCL1);
    else {
	FXCHr(f0);
	FSTSm((long)i0, _NOREG, _NOREG, _SCL1);
	FXCHr(f0);
    }
}

__jit_inline void
x87_stxr_f(jit_state_t _jit, jit_gpr_t r0, jit_gpr_t r1, jit_fpr_t f0)
{
    if (f0 == _ST0)
	FSTSm(0, r0, r1, _SCL1);
    else {
	FXCHr(f0);
	FSTSm(0, r0, r1, _SCL1);
	FXCHr(f0);
    }
}

__jit_inline void
x87_stxi_f(jit_state_t _jit, long i0, jit_gpr_t r0, jit_fpr_t f0)
{
#if __WORDSIZE == 64
    if (!jit_can_sign_extend_int_p(i0)) {
	jit_movi_l(JIT_REXTMP, (long)i0);
	x87_stxr_f(_jit, JIT_REXTMP, r0, f0);
	return;
    }
#endif
    if (f0 == _ST0)
	FSTSm(i0, r0, _NOREG, _SCL1);
    else {
	FXCHr(f0);
	FSTSm(i0, r0, _NOREG, _SCL1);
	FXCHr(f0);
    }
}

__jit_inline void
x87_str_d(jit_state_t _jit, jit_gpr_t r0, jit_fpr_t f0)
{
    if (f0 == _ST0)
	FSTLm(0, r0, _NOREG, _SCL1);
    else {
	FXCHr(f0);
	FSTLm(0, r0, _NOREG, _SCL1);
	FXCHr(f0);
    }
}

__jit_inline void
x87_sti_d(jit_state_t _jit, void *i0, jit_fpr_t f0)
{
#if __WORDSIZE == 64
    if (!jit_can_sign_extend_int_p((long)i0)) {
	jit_movi_l(JIT_REXTMP, (long)i0);
	x87_str_d(_jit, JIT_REXTMP, f0);
	return;
    }
#endif
    if (f0 == _ST0)
	FSTLm((long)i0, _NOREG, _NOREG, _SCL1);
    else {
	FXCHr(f0);
	FSTLm((long)i0, _NOREG, _NOREG, _SCL1);
	FXCHr(f0);
    }
}

__jit_inline void
x87_stxr_d(jit_state_t _jit, jit_gpr_t r0, jit_gpr_t r1, jit_fpr_t f0)
{
    if (f0 == _ST0)
	FSTLm(0, r0, r1, _SCL1);
    else {
	FXCHr(f0);
	FSTLm(0, r0, r1, _SCL1);
	FXCHr(f0);
    }
}

__jit_inline void
x87_stxi_d(jit_state_t _jit, long i0, jit_gpr_t r0, jit_fpr_t f0)
{
#if __WORDSIZE == 64
    if (!jit_can_sign_extend_int_p(i0)) {
	jit_movi_l(JIT_REXTMP, (long)i0);
	x87_stxr_d(_jit, JIT_REXTMP, r0, f0);
	return;
    }
#endif
    if (f0 == _ST0)
	FSTLm(i0, r0, _NOREG, _SCL1);
    else {
	FXCHr(f0);
	FSTLm(i0, r0, _NOREG, _SCL1);
	FXCHr(f0);
    }
}

__jit_inline void
x87_movi_f(jit_state_t _jit, jit_fpr_t f0, float i0)
{
    union {
	int	i;
	float	f;
    } data;
    int		c;

    c = 1;
    data.f = i0;
    if (data.f == 0.0) {
	if (data.i & 0x80000000)
	    c = 0;
	else
	    FLDZ_();
    }
    else if (data.f == 1.0)
	FLD1_();
    /* these should be optional for reproducibly tests
     * that rely on load of truncated values */
    else if (data.f == 3.3219280948873623478703195458468f)
	FLDL2T_();
    else if (data.f == 1.4426950408889634073599246886656f)
	FLDL2E_();
    else if (data.f == 3.1415926535897932384626421096161f)
	FLDPI_();
    else if (data.f == 0.3010299956639811952137387498515f)
	FLDLG2_();
    else if (data.f == 0.6931471805599453094172323683399f)
	FLDLN2_();
    else
	c = 0;

    if (c)
	FSTPr((jit_fpr_t)(f0 + 1));
    else {
	jit_pushi_i(data.i);
	x87_ldr_f(_jit, f0, JIT_SP);
	jit_addi_l(JIT_SP, JIT_SP, sizeof(long));
    }
}

__jit_inline void
x87_movi_d(jit_state_t _jit, jit_fpr_t f0, double i0)
{
    union {
	int	i[2];
	long	l;
	double	d;
    } data;
    int		c;

    c = 1;
    data.d = i0;
    if (data.d == 0.0) {
	if (data.i[1] & 0x80000000)
	    c = 0;
	else
	    FLDZ_();
    }
    else if (data.d == 1.0)
	FLD1_();
    /* these should be optional for reproducibly tests
     * that rely on load of truncated values */
    else if (data.d == 3.3219280948873623478703195458468)
	FLDL2T_();
    else if (data.d == 1.4426950408889634073599246886656)
	FLDL2E_();
    else if (data.d == 3.1415926535897932384626421096161)
	FLDPI_();
    else if (data.d == 0.3010299956639811952137387498515)
	FLDLG2_();
    else if (data.d == 0.6931471805599453094172323683399)
	FLDLN2_();
    else
	c = 0;

    if (c)
	FSTPr((jit_fpr_t)(f0 + 1));
    else {
#if __WORDSIZE == 64
	PUSHQi(data.l);
#else
	PUSHLi(data.i[1]);
	PUSHLi(data.i[0]);
#endif
	x87_ldr_d(_jit, f0, JIT_SP);
	jit_addi_l(JIT_SP, JIT_SP, 8);
    }
}

__jit_inline void
x87_movr_d(jit_state_t _jit, jit_fpr_t f0, jit_fpr_t f1)
{
    if (f0 != f1) {
	if (f1 == _ST0)
	    FSTr(f0);
	else if (f0 == _ST0) {
	    FXCHr(f1);
	    FSTr(f1);
	}
	else {
	    FLDr(f1);
	    FSTPr((jit_fpr_t)(f0 + 1));
	}
    }
}

__jit_inline void
x87_extr_i_d(jit_state_t _jit, jit_fpr_t f0, jit_gpr_t r0)
{
    jit_pushr_i(r0);
    FILDLm(0, JIT_SP, _NOREG, _SCL1);
    FSTPr((jit_fpr_t)(f0 + 1));
    jit_popr_i(r0);
}

__jit_inline void
x87_rintr_d_i(jit_state_t _jit, jit_gpr_t r0, jit_fpr_t f0)
{
    jit_pushr_i(_RAX);
    /* store integer using current rounding mode */
    if (f0 == _ST0)
	FISTLm(0, JIT_SP, _NOREG, _SCL1);
    else {
	FXCHr(f0);
	FISTLm(0, JIT_SP, _NOREG, _SCL1);
	FXCHr(f0);
    }
    jit_popr_i(r0);
}

/* This looks complex/slow, but actually is quite faster than
 * adjusting the rounding mode as done in _safe_roundr_d_i
 */
__jit_inline void
x87_386_roundr_d_i(jit_state_t _jit, jit_gpr_t r0, jit_fpr_t f0)
{
    jit_insn	*label;

    /* make room on stack */
    jit_pushr_i(_RAX);
    if (r0 != _RAX)
	MOVLrr(_RAX, r0);

    FLDr(f0);

    /* status test(st(0), 0.0) in %ax to know if positive */
    FTST_();
    FNSTSWr(_RAX);

    /*	Assuming default, round to nearest:
     *	f(n) = n - rint(n)
     *	f(0.3) =  0.3	f(-0.3) = -0.3
     *	f(0.5) =  0.5	f(-0.5) = -0.5	(wrong round down to even)
     *	f(0.7) = -0.3	f(-0.7) =  0.3
     *	f(1.3) =  0.3	f(-1.3) = -0.3
     *	f(1.5) = -0.5	f(-1.5) =  0.5	(correct round up to even)
     *	f(1.7) = -0.3	f(-1.7) =  0.3
     *
     *	Logic used is:
     *	-0.5 * sgn(n) + (n - rint(n))
     *
     *	If result of above is not zero, round to nearest (even on ties)
     *	will round away from zero as expected, otherwise:
     *	rint(n - -0.5 * sgn(n))
     *
     *	Example:
     *	round_to_nearest(0.5) = 0, what is wrong, following above:
     *		0.5 - rint(0.5) = 0.5
     *		-0.5 * 1 + 0.5 = 0
     *		rint(0.5 - -0.5 * 1) = 1
     *	with negative value:
     *	round_to_nearest(-2.5) = -2, what is wrong, following above:
     *		-2.5 - rint(-2.5) = -0.5
     *		-0.5 * -1 + -0.5 = 0.0
     *		rint(-2.5 - -0.5 * -1) = -3
     */

    /* st(0) = rint(st(0)) */
    FRNDINT_();

    /* st(0) = st(f0+1)-st(0) */
    FSUBRrr((jit_fpr_t)(f0 + 1), _ST0);

    /* st(0) = -0.5, st(1) = fract */
    FLD1_();
    FCHS_();
    F2XM1_();

    /* if (st(f0+2) is positive, do not change sign of -0.5 */
    ANDWir(FPSW_COND, _RAX);
    TESTWrr(_RAX, _RAX);
    JZSm(_jit->x.pc + 2);
    label = _jit->x.pc;
    FCHS_();
    jit_patch_rel_char_at(label, _jit->x.pc);

    /* st(0) = *0.5 + fract, st(1) = *0.5 */
    FXCHr(_ST1);
    FADDrr(_ST1, _ST0);

    /* status of test(st(0), 0.0) in %ax to know if zero
     * (tie round to nearest, that was even, and was round towards zero) */
    FTST_();
    FNSTSWr(_RAX);

    /* replace top of x87 stack with jit register argument */
    FFREEr(_ST0);
    FINCSTP_();
    FLDr((jit_fpr_t)(f0 + 1));

    /* if operation did not result in zero, can round to near */
    ANDWir(FPSW_COND, _RAX);
    CMPWir(FPSW_EQ, _RAX);
    JNESm(_jit->x.pc + 2);
    label = _jit->x.pc;

    /* adjust for round, st(0) = st(0) - *0.5 */
    FSUBrr(_ST1, _ST0);

    jit_patch_rel_char_at(label, _jit->x.pc);

    /* overwrite *0.5 with (possibly adjusted) value */
    FSTPr(_ST1);

    /* store value and pop x87 stack */
    FISTPLm(0, JIT_SP, _NOREG, _SCL1);

    if (r0 != _RAX)
	jit_xchgr_i(_RAX, r0);
    jit_popr_i(r0);
}

__jit_inline void
x87_safe_roundr_d_i(jit_state_t _jit, jit_gpr_t r0, jit_fpr_t f0)
{
    jit_insn	*label;

    /* make room on stack and save %rax */
    jit_subi_l(JIT_SP, JIT_SP, sizeof(long) << 1);
    if (r0 != _RAX)
	MOVLrr(_RAX, r0);

    /* load copy of value */
    FLDr(f0);

    /* store control word */
    FSTCWm(0, JIT_SP, _NOREG, _SCL1);
    /* load control word */
    jit_ldr_s(_RAX, JIT_SP);
    /* make copy */
    jit_stxi_s(sizeof(long), JIT_SP, _RAX);

    /* clear top bits and select chop (truncate mode) */
    MOVZBLrr(_RAX, _RAX);
#if __WORDSIZE == 32
    ORWir(FPCW_CHOP, _RAX);
#else
    ORLir(FPCW_CHOP, _RAX);
#endif

    /* load new control word */
    jit_str_s(JIT_SP, _RAX);
    FLDCWm(0, JIT_SP, _NOREG, _SCL1);

    /* compare with 0 */
    FTST_();
    FNSTSWr(_RAX);

    /* load -0.5 */
    FLD1_();
    FCHS_();
    F2XM1_();

    /* if negative keep sign of -0.5 */
    ANDWir(FPSW_COND, _RAX);
    CMPWir(FPSW_LT, _RAX);
    JESm(_jit->x.pc + 2);
    label = _jit->x.pc;
    FCHS_();
    jit_patch_rel_char_at(label, _jit->x.pc);

    /* add/sub 0.5 */
    FADDPr(_ST1);

    /* round adjusted value using truncation */
    FISTPLm(0, JIT_SP, _NOREG, _SCL1);

    /* load result and restore state */
    FLDCWm(sizeof(long), JIT_SP, _NOREG, _SCL1);
    if (r0 != _RAX)
	jit_xchgr_i(_RAX, r0);
    jit_ldr_i(r0, JIT_SP);
    jit_addi_l(JIT_SP, JIT_SP, sizeof(long) << 1);
}

__jit_inline void
x87_roundr_d_i(jit_state_t _jit, jit_gpr_t r0, jit_fpr_t f0)
{
    if (jit_round_to_nearest_p())
	x87_386_roundr_d_i(_jit, r0, f0);
    else
	x87_safe_roundr_d_i(_jit, r0, f0);
}

__jit_inline void
x87_i386_truncr_d_i(jit_state_t _jit, jit_gpr_t r0, jit_fpr_t f0)
{
    /* make room, store control word and copy */
    jit_subi_l(JIT_SP, JIT_SP, sizeof(long) << 1);
    FSTCWm(0, JIT_SP, _NOREG, _SCL1);
    jit_ldr_s(r0, JIT_SP);
    jit_stxi_s(sizeof(long), JIT_SP, r0);

    /* clear top bits and select chop (round towards zero) */
    if (jit_reg8_p(r0))
	/* always the path in 64-bit mode */
	MOVZBLrr(r0, r0);
    else
	/* 32-bit mode only */
	ANDWir(0xff, r0);
#if __WORDSIZE == 32
    ORWir(FPCW_CHOP, r0);
#else
    ORLir(FPCW_CHOP, r0);
#endif

    /* load new control word and convert integer */
    jit_str_s(JIT_SP, r0);
    FLDCWm(0, JIT_SP, _NOREG, _SCL1);
    if (f0 == _ST0)
	FISTLm(0, JIT_SP, _NOREG, _SCL1);
    else {
	FXCHr(f0);
	FISTLm(0, JIT_SP, _NOREG, _SCL1);
	FXCHr(f0);
    }

    /* load result and restore state */
    FLDCWm(sizeof(long), JIT_SP, _NOREG, _SCL1);
    jit_ldr_i(r0, JIT_SP);
    jit_addi_l(JIT_SP, JIT_SP, sizeof(long) << 1);
}

__jit_inline void
x87_i686_truncr_d_i(jit_state_t _jit, jit_gpr_t r0, jit_fpr_t f0)
{
    jit_pushr_i(_RAX);
    FLDr(f0);
    FISTTPLm(0, JIT_SP, _NOREG, _SCL1);
    jit_popr_i(r0);
}

__jit_inline void
x87_truncr_d_i(jit_state_t _jit, jit_gpr_t r0, jit_fpr_t f0)
{
    if (jit_i686_p())
	x87_i686_truncr_d_i(_jit, r0, f0);
    else
	/* also "safe" version */
	x87_i386_truncr_d_i(_jit, r0, f0);
}

__jit_inline void
x87_safe_floorr_d_i(jit_state_t _jit, jit_gpr_t r0, jit_fpr_t f0)
{
    /* make room, store control word and copy */
    jit_subi_l(JIT_SP, JIT_SP, sizeof(long) << 1);
    FSTCWm(0, JIT_SP, _NOREG, _SCL1);
    jit_ldr_s(r0, JIT_SP);
    jit_stxi_s(sizeof(long), JIT_SP, r0);

    /* clear top bits and select down (round towards minus infinity) */
    if (jit_reg8_p(r0))
	MOVZBLrr(r0, r0);
    else
	ANDWir(0xff, r0);
#if __WORDSIZE == 32
    ORWir(FPCW_DOWN, r0);
#else
    ORLir(FPCW_DOWN, r0);
#endif

    /* load new control word and convert integer */
    jit_str_s(JIT_SP, r0);
    FLDCWm(0, JIT_SP, _NOREG, _SCL1);

    if (f0 == _ST0)
	FISTLm(0, JIT_SP, _NOREG, _SCL1);
    else {
	FXCHr(f0);
	FISTLm(0, JIT_SP, _NOREG, _SCL1);
	FXCHr(f0);
    }

    /* load integer and restore state */
    FLDCWm(sizeof(long), JIT_SP, _NOREG, _SCL1);
    jit_ldr_i(r0, JIT_SP);
    jit_addi_l(JIT_SP, JIT_SP, sizeof(long) << 1);
}

__jit_inline void
x87_i386_floorr_d_i(jit_state_t _jit, jit_gpr_t r0, jit_fpr_t f0)
{
    jit_insn	*label;

    jit_pushr_i(_RAX);
    if (r0 != _RAX)
	MOVLrr(_RAX, r0);
    FLDr(f0);
    FRNDINT_();
    FUCOMr((jit_fpr_t)(f0 + 1));
    FNSTSWr(_RAX);
    ANDWir(FPSW_COND, _RAX);
    TESTWrr(_RAX, _RAX);
    JNESm(_jit->x.pc + 4);
    label = _jit->x.pc;
    FLD1_();
    FSUBRPr(_ST1);
    jit_patch_rel_char_at(label, _jit->x.pc);
    FISTPLm(0, JIT_SP, _NOREG, _SCL1);
    if (r0 != _RAX)
	jit_xchgr_i(_RAX, r0);
    jit_popr_i(r0);
}

__jit_inline void
x87_i686_floorr_d_i(jit_state_t _jit, jit_gpr_t r0, jit_fpr_t f0)
{
    jit_insn	*label;

    /* make room */
    jit_pushr_i(_RAX);
    /* push value */
    FLDr(f0);
    /* round to nearest */
    FRNDINT_();
    /* compare and set flags */
    FXCHr((jit_fpr_t)(f0 + 1));
    FCOMIr((jit_fpr_t)(f0 + 1));
    FXCHr((jit_fpr_t)(f0 + 1));
    /* store integer */
    FISTPLm(0, JIT_SP, _NOREG, _SCL1);
    jit_popr_i(r0);
    JPESm(_jit->x.pc + 3);
    label = _jit->x.pc;
    /* subtract 1 if carry */
    SBBLir(0, r0);
    jit_patch_rel_char_at(label, _jit->x.pc);
}

__jit_inline void
x87_floorr_d_i(jit_state_t _jit, jit_gpr_t r0, jit_fpr_t f0)
{
    if (jit_round_to_nearest_p()) {
	if (jit_i686_p())
	    x87_i686_floorr_d_i(_jit, r0, f0);
	else
	    x87_i386_floorr_d_i(_jit, r0, f0);
    }
    else
	x87_safe_floorr_d_i(_jit, r0, f0);
}

__jit_inline void
x87_safe_ceilr_d_i(jit_state_t _jit, jit_gpr_t r0, jit_fpr_t f0)
{
    /* make room, store control word and copy */
    jit_subi_l(JIT_SP, JIT_SP, sizeof(long) << 1);
    FSTCWm(0, JIT_SP, _NOREG, _SCL1);
    jit_ldr_s(r0, JIT_SP);
    jit_stxi_s(sizeof(long), JIT_SP, r0);

    /* clear top bits and select up (round towards positive infinity) */
    if (jit_reg8_p(r0))
	MOVZBLrr(r0, r0);
    else
	ANDWir(0xff, r0);
#if __WORDSIZE == 32
    ORWir(FPCW_UP, r0);
#else
    ORLir(FPCW_UP, r0);
#endif

    /* load new control word and convert integer */
    jit_str_s(JIT_SP, r0);
    FLDCWm(0, JIT_SP, _NOREG, _SCL1);
    if (f0 == _ST0)
	FISTLm(0, JIT_SP, _NOREG, _SCL1);
    else {
	FXCHr(f0);
	FISTLm(0, JIT_SP, _NOREG, _SCL1);
	FXCHr(f0);
    }

    /* load integer and restore state */
    FLDCWm(sizeof(long), JIT_SP, _NOREG, _SCL1);
    jit_ldr_i(r0, JIT_SP);
    jit_addi_l(JIT_SP, JIT_SP, sizeof(long) << 1);
}

__jit_inline void
x87_i386_ceilr_d_i(jit_state_t _jit, jit_gpr_t r0, jit_fpr_t f0)
{
    jit_insn	*label;

    jit_pushr_i(_RAX);
    if (r0 != _RAX)
	MOVLrr(_RAX, r0);
    FLDr(f0);
    FRNDINT_();
    FUCOMr((jit_fpr_t)(f0 + 1));
    FNSTSWr(_RAX);
    ANDWir(FPSW_COND, _RAX);
    CMPWir(FPSW_LT, _RAX);
    JNESm(_jit->x.pc + 4);
    label = _jit->x.pc;
    FLD1_();
    FADDPr(_ST1);
    jit_patch_rel_char_at(label, _jit->x.pc);
    FISTPLm(0, JIT_SP, _NOREG, _SCL1);
    if (r0 != _RAX)
	jit_xchgr_i(_RAX, r0);
    jit_popr_i(r0);
}

__jit_inline void
x87_i686_ceilr_d_i(jit_state_t _jit, jit_gpr_t r0, jit_fpr_t f0)
{
    jit_insn	*label;

    /* make room */
    jit_pushr_i(_RAX);
    /* push value */
    FLDr(f0);
    /* round to nearest */
    FRNDINT_();
    /* compare and set flags */
    FCOMIr((jit_fpr_t)(f0 + 1));
    /* store integer */
    FISTPLm(0, JIT_SP, _NOREG, _SCL1);
    jit_popr_i(r0);
    JPESm(_jit->x.pc + 4);
    label = _jit->x.pc;
    /* add 1 if carry */
    ADCLir(0, r0);
    jit_patch_rel_char_at(label, _jit->x.pc);
}

__jit_inline void
x87_ceilr_d_i(jit_state_t _jit, jit_gpr_t r0, jit_fpr_t f0)
{
    if (jit_round_to_nearest_p()) {
	if (jit_i686_p())
	    x87_i686_ceilr_d_i(_jit, r0, f0);
	else
	    x87_i386_ceilr_d_i(_jit, r0, f0);
    }
    else
	x87_safe_ceilr_d_i(_jit, r0, f0);
}

/* After FNSTSW we have 1 if <, 40 if =, 0 if >, 45 if unordered.  Here
   is how to map the values of the status word's high byte to the
   conditions.

         <     =     >     unord    valid values    condition
  gt     no    no    yes   no       0               STSW & 45 == 0
  lt     yes   no    no    no       1               STSW & 45 == 1
  eq     no    yes   no    no       40              STSW & 45 == 40
  unord  no    no    no    yes      45              bit 2 == 1

  ge     no    yes   no    no       0, 40           bit 0 == 0
  unlt   yes   no    no    yes      1, 45           bit 0 == 1
  ltgt   yes   no    yes   no       0, 1            bit 6 == 0
  uneq   no    yes   no    yes      40, 45          bit 6 == 1
  le     yes   yes   no    no       1, 40           odd parity for STSW & 41
  ungt   no    no    yes   yes      0, 45           even parity for STSW & 41

  unle   yes   yes   no    yes      1, 40, 45       STSW & 45 != 0
  unge   no    yes   yes   yes      0, 40, 45       STSW & 45 != 1
  ne     yes   no    yes   yes      0, 1, 45        STSW & 45 != 40
  ord    yes   yes   yes   no       0, 1, 40        bit 2 == 0

  lt, le, ungt, unge are actually computed as gt, ge, unlt, unle with
  the operands swapped; it is more efficient this way.  */

__jit_inline void
x87_i386_fp_cmp(jit_state_t _jit, jit_gpr_t r0, jit_fpr_t f0, jit_fpr_t f1,
		int shift, int _and, x86_cc_t cc)
{
    if (f0 == _ST0)
	FUCOMr(f1);
    else {
	FLDr(f0);
	FUCOMPr((jit_fpr_t)(f1 + 1));
    }
    if (r0 != _RAX)
	MOVLrr(_RAX, r0);
    FNSTSWr(_RAX);
    SHRLir(shift, _RAX);
    if (_and)
	ANDLir(_and, _RAX);
    else
	MOVLir(0, _RAX);
    if (shift == 8) {
	if (cc < 0) {
	    CMPBir(0x40, _RAX);
	    cc = (x86_cc_t)-cc;
	}
	SETCCir(cc, _RAX);
    }
    else if (cc == 0)
	ADCBir(0, _RAX);
    else
	SBBBir(-1, _RAX);
    if (r0 != _RAX)
	jit_xchgr_i(r0, _RAX);
}

__jit_inline void
x87_i686_fp_cmp(jit_state_t _jit, jit_gpr_t r0, jit_fpr_t f0, jit_fpr_t f1,
		x86_cc_t cc)
{
    int		 rc;
    jit_gpr_t	 reg;

    if ((rc = jit_reg8_p(r0)))
	reg = r0;
    else {
	MOVLrr(_RAX, r0);
	reg = _RAX;
    }

    XORLrr(reg, reg);
    if (f0 == _ST0)
	FUCOMIr(f1);
    else {
	FLDr(f0);
	FUCOMIPr((jit_fpr_t)(f1 + 1));
    }
    SETCCir(cc, reg);
    if (!rc)
	jit_xchgr_i(_RAX, r0);
}

__jit_inline void
x87_ltr_d(jit_state_t _jit, jit_gpr_t r0, jit_fpr_t f0, jit_fpr_t f1)
{
    if (jit_i686_p())
	x87_i686_fp_cmp(_jit, r0, f1, f0,	X86_CC_A);
    else
	x87_i386_fp_cmp(_jit, r0, f1, f0,	8, 0x45, X86_CC_Z);
}

__jit_inline void
x87_ler_d(jit_state_t _jit, jit_gpr_t r0, jit_fpr_t f0, jit_fpr_t f1)
{
    if (jit_i686_p())
	x87_i686_fp_cmp(_jit, r0, f1, f0,	X86_CC_AE);
    else
	x87_i386_fp_cmp(_jit, r0, f1, f0,	9, 0, (x86_cc_t)-1);
}

__jit_inline void
x87_eqr_d(jit_state_t _jit, jit_gpr_t r0, jit_fpr_t f0, jit_fpr_t f1)
{
    jit_fpr_t			fr0, fr1;
    if (f1 == _ST0)	fr0 = f1, fr1 = f0;
    else		fr0 = f0, fr1 = f1;
    if (jit_i686_p()) {
	int		 rc;
	jit_gpr_t	 reg;
	jit_insn	*label;
	if ((rc = jit_reg8_p(r0)))
	    reg = r0;
	else {
	    MOVLrr(_RAX, r0);
	    reg = _RAX;
	}
	XORLrr(reg, reg);
	if (f0 == _ST0)
	    FUCOMIr(f1);
	else {
	    FLDr(f0);
	    FUCOMIPr((jit_fpr_t)(f1 + 1));
	}
	JPESm(_jit->x.pc + 3);
	label = _jit->x.pc;
	SETCCir(X86_CC_E, reg);
	jit_patch_rel_char_at(label, _jit->x.pc);
	if (!rc)
	    jit_xchgr_i(_RAX, r0);
    }
    else
	x87_i386_fp_cmp(_jit, r0, fr0, fr1,	8, 0x45, (x86_cc_t)-X86_CC_E);
}

__jit_inline void
x87_ger_d(jit_state_t _jit, jit_gpr_t r0, jit_fpr_t f0, jit_fpr_t f1)
{
    if (jit_i686_p())
	x87_i686_fp_cmp(_jit, r0, f0, f1,	X86_CC_AE);
    else
	x87_i386_fp_cmp(_jit, r0, f0, f1,	9, 0, (x86_cc_t)-1);
}

__jit_inline void
x87_gtr_d(jit_state_t _jit, jit_gpr_t r0, jit_fpr_t f0, jit_fpr_t f1)
{
    if (jit_i686_p())
	x87_i686_fp_cmp(_jit, r0, f0, f1,	X86_CC_A);
    else
	x87_i386_fp_cmp(_jit, r0, f0, f1,	8, 0x45, X86_CC_Z);
}

__jit_inline void
x87_ner_d(jit_state_t _jit, jit_gpr_t r0, jit_fpr_t f0, jit_fpr_t f1)
{
    jit_fpr_t		fr0, fr1;
    if (f1 == _ST0)	fr0 = f1, fr1 = f0;
    else		fr0 = f0, fr1 = f1;
    if (jit_i686_p()) {
	int		 rc;
	jit_gpr_t	 reg;
	jit_insn	*label;
	if ((rc = jit_reg8_p(r0)))
	    reg = r0;
	else {
	    MOVLrr(_RAX, r0);
	    reg = _RAX;
	}
	MOVLir(1, reg);
	if (f0 == _ST0)
	    FUCOMIr(f1);
	else {
	    FLDr(f0);
	    FUCOMIPr((jit_fpr_t)(f1 + 1));
	}
	JPESm(_jit->x.pc + 3);
	label = _jit->x.pc;
	SETCCir(X86_CC_NE, reg);
	jit_patch_rel_char_at(label, _jit->x.pc);
	if (!rc)
	    jit_xchgr_i(_RAX, r0);
    }
    else
	x87_i386_fp_cmp(_jit, r0, fr0, fr1,	8, 0x45, (x86_cc_t)-X86_CC_NE);
}

__jit_inline void
x87_unltr_d(jit_state_t _jit, jit_gpr_t r0, jit_fpr_t f0, jit_fpr_t f1)
{
    if (jit_i686_p())
	x87_i686_fp_cmp(_jit, r0, f0, f1,	X86_CC_NAE);
    else
	x87_i386_fp_cmp(_jit, r0, f0, f1,	9, 0, (x86_cc_t)0);
}

__jit_inline void
x87_unler_d(jit_state_t _jit, jit_gpr_t r0, jit_fpr_t f0, jit_fpr_t f1)
{
    if (f0 == f1)
	MOVLir(1, r0);
    else if (jit_i686_p())
	x87_i686_fp_cmp(_jit, r0, f0, f1,	X86_CC_NA);
    else
	x87_i386_fp_cmp(_jit, r0, f0, f1,	8, 0x45, X86_CC_NZ);
}

__jit_inline void
x87_ltgtr_d(jit_state_t _jit, jit_gpr_t r0, jit_fpr_t f0, jit_fpr_t f1)
{
    jit_fpr_t		fr0, fr1;
    if (f1 == _ST0)	fr0 = f1, fr1 = f0;
    else		fr0 = f0, fr1 = f1;
    if (f0 == f1)
	XORLrr(r0, r0);
    else if (jit_i686_p())
	x87_i686_fp_cmp(_jit, r0, fr0, fr1,	X86_CC_NE);
    else
	x87_i386_fp_cmp(_jit, r0, fr0, fr1,	15, 0, (x86_cc_t)-1);
}

__jit_inline void
x87_uneqr_d(jit_state_t _jit, jit_gpr_t r0, jit_fpr_t f0, jit_fpr_t f1)
{
    jit_fpr_t		fr0, fr1;
    if (f1 == _ST0)	fr0 = f1, fr1 = f0;
    else		fr0 = f0, fr1 = f1;
    if (f0 == f1)
	MOVLir(1, r0);
    else if (jit_i686_p())
	x87_i686_fp_cmp(_jit, r0, fr0, fr1,	X86_CC_E);
    else
	x87_i386_fp_cmp(_jit, r0, fr0, fr1,	15, 0, (x86_cc_t)0);
}

__jit_inline void
x87_unger_d(jit_state_t _jit, jit_gpr_t r0, jit_fpr_t f0, jit_fpr_t f1)
{
    if (f0 == f1)
	MOVLir(1, r0);
    else if (jit_i686_p())
	x87_i686_fp_cmp(_jit, r0, f1, f0,	X86_CC_NA);
    else
	x87_i386_fp_cmp(_jit, r0, f1, f0,	8, 0x45, X86_CC_NZ);
}

__jit_inline void
x87_ungtr_d(jit_state_t _jit, jit_gpr_t r0, jit_fpr_t f0, jit_fpr_t f1)
{
    if (jit_i686_p())
	x87_i686_fp_cmp(_jit, r0, f1, f0,	X86_CC_NAE);
    else
	x87_i386_fp_cmp(_jit, r0, f1, f0,	9, 0, (x86_cc_t)0);
}

__jit_inline void
x87_ordr_d(jit_state_t _jit, jit_gpr_t r0, jit_fpr_t f0, jit_fpr_t f1)
{
    jit_fpr_t		fr0, fr1;
    if (f1 == _ST0)	fr0 = f1, fr1 = f0;
    else		fr0 = f0, fr1 = f1;
    if (jit_i686_p())
	x87_i686_fp_cmp(_jit, r0, fr0, fr1,	X86_CC_NP);
    else
	x87_i386_fp_cmp(_jit, r0, fr0, fr1,	11, 0, (x86_cc_t)-1);
}

__jit_inline void
x87_unordr_d(jit_state_t _jit, jit_gpr_t r0, jit_fpr_t f0, jit_fpr_t f1)
{
    jit_fpr_t		fr0, fr1;
    if (f1 == _ST0)	fr0 = f1, fr1 = f0;
    else		fr0 = f0, fr1 = f1;
    if (jit_i686_p())
	x87_i686_fp_cmp(_jit, r0, fr0, fr1,	X86_CC_P);
    else
	x87_i386_fp_cmp(_jit, r0, fr0, fr1,	11, 0, (x86_cc_t)0);
}

__jit_inline void
x87_i386_fp_bcmp(jit_state_t _jit,
		 jit_insn *label, jit_fpr_t f0, jit_fpr_t f1,
		 int shift, int _and, int cmp, x86_cc_t cc)
{
    if (f0 == _ST0)
	FUCOMr(f1);
    else {
	FLDr(f0);
	FUCOMPr((jit_fpr_t)(f1 + 1));
    }
    jit_pushr_i(_RAX);
    FNSTSWr(_RAX);
    SHRLir(shift, _RAX);
    if (_and)
	ANDLir(_and, _RAX);
    if (cmp)
	CMPLir(cmp, _RAX);
    jit_popr_i(_RAX);
    JCCim(cc, label);
}

__jit_inline void
x87_i686_fp_bcmp(jit_state_t _jit, jit_insn *label, jit_fpr_t f0, jit_fpr_t f1,
		 x86_cc_t cc)
{
    if (f0 == _ST0)
	FUCOMIr(f1);
    else {
	FLDr(f0);
	FUCOMIPr((jit_fpr_t)(f1 + 1));
    }
    JCCim(cc, label);
}

__jit_inline jit_insn *
x87_bltr_d(jit_state_t _jit, jit_insn *label, jit_fpr_t f0, jit_fpr_t f1)
{
    if (jit_i686_p())
	x87_i686_fp_bcmp(_jit, label, f1, f0,	X86_CC_A);
    else
	x87_i386_fp_bcmp(_jit, label, f1, f0,	8, 0x45, 0, X86_CC_Z);
    return (_jit->x.pc);
}

__jit_inline jit_insn *
x87_bler_d(jit_state_t _jit, jit_insn *label, jit_fpr_t f0, jit_fpr_t f1)
{
    if (jit_i686_p())
	x87_i686_fp_bcmp(_jit, label, f1, f0,	X86_CC_AE);
    else
	x87_i386_fp_bcmp(_jit, label, f1, f0,	9, 0, 0, X86_CC_NC);
    return (_jit->x.pc);
}

__jit_inline jit_insn *
x87_beqr_d(jit_state_t _jit, jit_insn *label, jit_fpr_t f0, jit_fpr_t f1)
{
    jit_fpr_t		fr0, fr1;
    if (f1 == _ST0)	fr0 = f1, fr1 = f0;
    else		fr0 = f0, fr1 = f1;
    if (jit_i686_p()) {
	jit_insn	*jp_label;
	if (fr0 == _ST0)
	    FUCOMIr(fr1);
	else {
	    FLDr(fr0);
	    FUCOMIPr((jit_fpr_t)(fr1 + 1));
	}
	/* jump past user jump if unordered */
	JPESm(_jit->x.pc + 6);
	jp_label = _jit->x.pc;
	JCCim(X86_CC_E, label);
	jit_patch_rel_char_at(jp_label, _jit->x.pc);
    }
    else
	x87_i386_fp_bcmp(_jit, label, fr0, fr1,	8, 0x45, 0x40, X86_CC_Z);
    return (_jit->x.pc);
}

__jit_inline jit_insn *
x87_bger_d(jit_state_t _jit, jit_insn *label, jit_fpr_t f0, jit_fpr_t f1)
{
    if (jit_i686_p())
	x87_i686_fp_bcmp(_jit, label, f0, f1,	X86_CC_AE);
    else
	x87_i386_fp_bcmp(_jit, label, f0, f1,	9, 0, 0, X86_CC_NC);
    return (_jit->x.pc);
}

__jit_inline jit_insn *
x87_bgtr_d(jit_state_t _jit, jit_insn *label, jit_fpr_t f0, jit_fpr_t f1)
{
    if (jit_i686_p())
	x87_i686_fp_bcmp(_jit, label, f0, f1,	X86_CC_A);
    else
	x87_i386_fp_bcmp(_jit, label, f0, f1,	8, 0x45, 0, X86_CC_Z);
    return (_jit->x.pc);
}

__jit_inline jit_insn *
x87_bner_d(jit_state_t _jit, jit_insn *label, jit_fpr_t f0, jit_fpr_t f1)
{
    jit_fpr_t		fr0, fr1;
    if (f1 == _ST0)	fr0 = f1, fr1 = f0;
    else		fr0 = f0, fr1 = f1;
    if (jit_i686_p()) {
	jit_insn	*jp_label;
	jit_insn	*jz_label;
	if (fr0 == _ST0)
	    FUCOMIr(fr1);
	else {
	    FLDr(fr0);
	    FUCOMIPr((jit_fpr_t)(fr1 + 1));
	}
	/* jump to user jump if unordered */
	JPESm(_jit->x.pc + 2);
	jp_label = _jit->x.pc;
	/* jump past user jump if equal */
	JZSm(_jit->x.pc + 5);
	jz_label = _jit->x.pc;
	jit_patch_rel_char_at(jp_label, _jit->x.pc);
	JMPm(label);
	jit_patch_rel_char_at(jz_label, _jit->x.pc);
    }
    else
	x87_i386_fp_bcmp(_jit, label, fr0, fr1,	8, 0x45, 0x40, X86_CC_NZ);
    return (_jit->x.pc);
}

__jit_inline jit_insn *
x87_bunltr_d(jit_state_t _jit, jit_insn *label, jit_fpr_t f0, jit_fpr_t f1)
{
    if (jit_i686_p())
	x87_i686_fp_bcmp(_jit, label, f0, f1,	X86_CC_NAE);
    else
	x87_i386_fp_bcmp(_jit, label, f0, f1,	9, 0, 0, X86_CC_C);
    return (_jit->x.pc);
}

__jit_inline jit_insn *
x87_bunler_d(jit_state_t _jit, jit_insn *label, jit_fpr_t f0, jit_fpr_t f1)
{
    if (f0 == f1)
	JMPm(label);
    else if (jit_i686_p())
	x87_i686_fp_bcmp(_jit, label, f0, f1,	X86_CC_NA);
    else
	x87_i386_fp_bcmp(_jit, label, f0, f1,	8, 0x45, 0, X86_CC_NZ);
    return (_jit->x.pc);
}

__jit_inline jit_insn *
x87_bltgtr_d(jit_state_t _jit, jit_insn *label, jit_fpr_t f0, jit_fpr_t f1)
{
    jit_fpr_t		fr0, fr1;
    if (f1 == _ST0)	fr0 = f1, fr1 = f0;
    else		fr0 = f0, fr1 = f1;
    if (jit_i686_p())
	x87_i686_fp_bcmp(_jit, label, fr0, fr1,	X86_CC_NE);
    else
	x87_i386_fp_bcmp(_jit, label, fr0, fr1,	15, 0, 0, X86_CC_NC);
    return (_jit->x.pc);
}

__jit_inline jit_insn *
x87_buneqr_d(jit_state_t _jit, jit_insn *label, jit_fpr_t f0, jit_fpr_t f1)
{
    jit_fpr_t		fr0, fr1;
    if (f1 == _ST0)	fr0 = f1, fr1 = f0;
    else		fr0 = f0, fr1 = f1;
    if (f0 == f1)
	JMPm(label);
    else if (jit_i686_p())
	x87_i686_fp_bcmp(_jit, label, fr0, fr1,	X86_CC_E);
    else
	x87_i386_fp_bcmp(_jit, label, fr0, fr1,	15, 0, 0, X86_CC_C);
    return (_jit->x.pc);
}

__jit_inline jit_insn *
x87_bunger_d(jit_state_t _jit, jit_insn *label, jit_fpr_t f0, jit_fpr_t f1)
{
    if (f0 == f1)
	JMPm(label);
    else if (jit_i686_p())
	x87_i686_fp_bcmp(_jit, label, f1, f0,	X86_CC_NA);
    else
	x87_i386_fp_bcmp(_jit, label, f1, f0,	8, 0x45, 0, X86_CC_NZ);
    return (_jit->x.pc);
}

__jit_inline jit_insn *
x87_bungtr_d(jit_state_t _jit, jit_insn *label, jit_fpr_t f0, jit_fpr_t f1)
{
    if (jit_i686_p())
	x87_i686_fp_bcmp(_jit, label, f1, f0,	X86_CC_NAE);
    else
	x87_i386_fp_bcmp(_jit, label, f1, f0,	9, 0, 0, X86_CC_C);
    return (_jit->x.pc);
}

__jit_inline jit_insn *
x87_bordr_d(jit_state_t _jit, jit_insn *label, jit_fpr_t f0, jit_fpr_t f1)
{
    jit_fpr_t		fr0, fr1;
    if (f1 == _ST0)	fr0 = f1, fr1 = f0;
    else		fr0 = f0, fr1 = f1;
    if (jit_i686_p())
	x87_i686_fp_bcmp(_jit, label, fr0, fr1,	X86_CC_NP);
    else
	x87_i386_fp_bcmp(_jit, label, fr0, fr1,	11, 0, 0, X86_CC_NC);
    return (_jit->x.pc);
}

__jit_inline jit_insn *
x87_bunordr_d(jit_state_t _jit, jit_insn *label, jit_fpr_t f0, jit_fpr_t f1)
{
    jit_fpr_t		fr0, fr1;
    if (f1 == _ST0)	fr0 = f1, fr1 = f0;
    else		fr0 = f0, fr1 = f1;
    if (jit_i686_p())
	x87_i686_fp_bcmp(_jit, label, fr0, fr1,	X86_CC_P);
    else
	x87_i386_fp_bcmp(_jit, label, fr0, fr1,	11, 0, 0, X86_CC_C);
    return (_jit->x.pc);
}

#if 0
#define jit_exp()	(FLDL2E_(), 			/* fldl2e */ \
			 FMULPr(_ST1), 			/* fmulp */ \
			 FLDr(_ST0),			/* fld st */ \
			 FRNDINT(),		 	/* frndint */ \
			 FSUBRrr(_ST0, _ST1),		/* fsubr */ \
			 FXCHr(_ST1), 			/* fxch st(1) */ \
			 F2XM1_(), 			/* f2xm1 */ \
			 FLD1_(), 			/* fld1 */ \
			 FADDPr(_ST1), 			/* faddp */ \
			 FSCALE_(), 			/* fscale */ \
			 FSTPr(_ST1))			/* fstp st(1) */
#endif

#endif /* __lightning_fp_x87_h */
