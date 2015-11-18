/******************************** -*- C -*- ****************************
 *
 *	Platform-independent layer (PowerPC version)
 *
 ***********************************************************************/


/***********************************************************************
 *
 * Copyright 2000, 2001, 2002, 2006 Free Software Foundation, Inc.
 * Written by Paolo Bonzini.
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
 ***********************************************************************/




#ifndef __lightning_core_h
#define __lightning_core_h

#define jit_can_sign_extend_short_p(im)	((im) >= -32678 && (im) <= 32767)
#define jit_can_zero_extend_short_p(im)	((im) >= 0 && (im) <= 65535)

/* Patch a `stwu' instruction (with immediate operand) so that it decreases
   r1 by AMOUNT.  AMOUNT should already be rounded so that %sp remains quadword
   aligned.  */
#define jit_patch_stwu(amount)                               \
  (*(_jitl.stwu) &= ~_MASK (16),                               \
   *(_jitl.stwu) |= _s16 ((amount)))

#define jit_allocai(n)							  \
   (_jitl.frame_size += (n),						  \
    ((n) <= _jitl.slack							  \
     ? 0 : jit_patch_stwu (-((_jitl.frame_size + 15) & ~15))),		  \
    _jitl.slack = ((_jitl.frame_size + 15) & ~15) - _jitl.frame_size,	  \
    _jitl.frame_size - (n))

#define JIT_SP			1
#define JIT_FP			1
#define JIT_RET			3
#define JIT_R_NUM		3
#define JIT_V_NUM		7
#define JIT_R(i)		(9+(i))
#define JIT_V(i)		(31-(i))
#define JIT_AUX			JIT_V(JIT_V_NUM)  /* for 32-bit operands & shift counts */

#define jit_movr_i(r0, r1)		MRrr(r0, r1)
#define jit_movi_i(r0, i0)		ppc_movi_i(_jit, r0, i0)
__jit_inline void
ppc_movi_i(jit_state_t _jit, int r0, int i0)
{
    if (jit_can_sign_extend_short_p(i0))
	LIri(r0, i0);
    else {
	LISri(r0, _jit_US(i0 >> 16));
	if (_jit_US(i0))
	    ORIrri(r0, r0, _jit_US(i0));
    }
}

#define jit_movi_p(r0, i0)		ppc_movi_p(_jit, r0, i0)
__jit_inline jit_insn *
ppc_movi_p(jit_state_t _jit, int r0, void *i0)
{
    int		im = (int)i0;
    LISri(r0, _jit_US(im >> 16));
    ORIrri(r0, r0, _jit_US(im));
    return (_jit->x.pc);
}

#define jit_notr_i(r0, r1)		NOTrr(r0, r1)
#define jit_negr_i(r0, r1)		NEGrr(r0, r1)
#define jit_extr_c_i(r0, r1)		EXTSBrr(r0, r1)
#define jit_extr_s_i(d, rs)		EXTSHrr((d), (rs))

/* If possible, use the `small' instruction (rd, rs, imm)
 * else load imm into r26 and use the `big' instruction (rd, rs, r26)
 */
#define jit_chk_ims(imm, small, big)		(_siP(16,(imm)) ? (small) : (MOVEIri(JIT_AUX, imm),  (big)) )
#define jit_chk_imu(imm, small, big)		(_uiP(16,(imm)) ? (small) : (MOVEIri(JIT_AUX, imm),  (big)) )
#define jit_chk_imu15(imm, small, big)		(_uiP(15,(imm)) ? (small) : (MOVEIri(JIT_AUX, imm),  (big)) )

#define jit_big_ims(imm, big)	               (MOVEIri(JIT_AUX, imm),  (big))
#define jit_big_imu(imm, big)	               (MOVEIri(JIT_AUX, imm),  (big))

/* Helper macros for branches */
#define jit_s_brai(rs, is, jmp)			(jit_chk_ims (is, CMPWIri(rs, is), CMPWrr(rs, JIT_AUX)),   jmp, _jit->x.pc)
#define jit_s_brar(s1, s2, jmp)			(		  CMPWrr(s1, s2), 		           jmp, _jit->x.pc)
#define jit_u_brai(rs, is, jmp)			(jit_chk_imu (is, CMPLWIri(rs, is), CMPLWrr(rs, JIT_AUX)), jmp, _jit->x.pc)
#define jit_u_brar(s1, s2, jmp)			(		  CMPLWrr(s1, s2), 		           jmp, _jit->x.pc)

/* modulus with big immediate                    with small immediate
 * movei r24, imm                                movei r24, imm
 * mtlr  r31
 * divw  r31, rs, r24		(or divwu)       divw r24, rs, r24
 * mullw r31, r31, r24                           mulli r24, r24, imm
 * sub   d, rs, r31                              sub   d, rs, r24
 * mflr  r31
 *
 *
 * jit_mod_big expects immediate in JIT_AUX.  */

#define _jit_mod_big(div, d, rs)		(MTLRr(31), div(31, (rs), JIT_AUX), \
						MULLWrrr(31, 31, JIT_AUX), SUBrrr((d), (rs), 31), \
						MFLRr(31))

#define _jit_mod_small(div, d, rs, imm)		(MOVEIri(JIT_AUX, (imm)), div(JIT_AUX, (rs), JIT_AUX), \
						 MULLIrri(JIT_AUX, JIT_AUX, (imm)), SUBrrr((d), (rs), JIT_AUX))

/* Patch a movei instruction made of a LIS at lis_pc and an ORI at ori_pc. */
#define jit_patch_movei(lis_pc, ori_pc, dest)			\
	(*(lis_pc) &= ~_MASK(16), *(lis_pc) |= _HI(dest),		\
	 *(ori_pc) &= ~_MASK(16), *(ori_pc) |= _LO(dest))		\

/* Patch a branch instruction */
#define jit_patch_branch(jump_pc,pv)				\
	(*(jump_pc) &= ~_MASK(16) | 3,				\
	 *(jump_pc) |= (_jit_UL(pv) - _jit_UL(jump_pc)) & _MASK(16))

#define jit_patch_ucbranch(jump_pc,pv)                          \
         (*(jump_pc) &= ~_MASK(26) | 3,                         \
         (*(jump_pc) |= (_jit_UL((pv)) - _jit_UL(jump_pc)) & _MASK(26)))

#define _jit_b_encoding		(18 << 26)
#define _jit_blr_encoding	((19 << 26) | (20 << 21) | (00 << 16) | (00 << 11) | (16 << 1))
#define _jit_is_ucbranch(a)     (((*(a) & (63<<26)) == _jit_b_encoding))

#define jit_patch_at(i0, i1)		ppc_patch_at(_jit, i0, i1)
__jit_inline void
ppc_patch_at(jit_state_t _jit, jit_insn *jump, jit_insn *label)
{
    _ui		*i0 = (_ui *)jump, *i1 = (_ui *)label;
    if ((*(i0 - 1) & ~1) == _jit_blr_encoding)
	jit_patch_movei(i0 - 4, i0 - 3, i1);
    else if (_jit_is_ucbranch(i0 - 1))
	jit_patch_ucbranch(i0 - 1, i1);
    else
	jit_patch_branch(i0 - 1, i1);
}

#define jit_patch_calli(i0, i1)		ppc_patch_movi(_jit, i0, i1)
#define jit_patch_movi(i0, i1)		ppc_patch_movi(_jit, i0, i1)
__jit_inline void
ppc_patch_movi(jit_state_t _jit, jit_insn *addr, void *value)
{
    _ui	*i0 = (_ui *)addr, *i1 = (_ui *)value;
    jit_patch_movei(i0 - 2, i0 - 1, i1);
}

#define	jit_arg_c()			(_jitl.nextarg_geti--)
#define	jit_arg_i()			(_jitl.nextarg_geti--)
#define	jit_arg_l()			(_jitl.nextarg_geti--)
#define	jit_arg_p()			(_jitl.nextarg_geti--)
#define	jit_arg_s()			(_jitl.nextarg_geti--)
#define	jit_arg_uc()			(_jitl.nextarg_geti--)
#define	jit_arg_ui()			(_jitl.nextarg_geti--)
#define	jit_arg_ul()			(_jitl.nextarg_geti--)
#define	jit_arg_us()			(_jitl.nextarg_geti--)

/* Check Mach-O-Runtime documentation: Must skip GPR(s) whenever "corresponding" FPR is used */
#define jit_arg_f()                    (_jitl.nextarg_geti-- ,_jitl.nextarg_getd++)
#define jit_arg_d()                    (_jitl.nextarg_geti-=2,_jitl.nextarg_getd++)

#define jit_addr_i(r0, r1, r2)		ADDrrr(r0, r1, r2)
#define jit_addi_i(r0, r1, i0)		ppc_addi_i(_jit, r0, r1, i0)
__jit_inline void
ppc_addi_i(jit_state_t _jit, int r0, int r1, int i0)
{
    if (jit_can_sign_extend_short_p(i0))
	ADDIrri(r0, r1, i0);
    else if (!(i0 & 0x0000ffff))
	ADDISrri(r0, r1, _jit_US(i0 >> 16));
    else {
	jit_movi_i(JIT_AUX, i0);
	ADDrrr(r0, r1, JIT_AUX);
    }
}

#define jit_addcr_ui(r0, r1, r2)	ADDCrrr(r0, r1, r2)
#define jit_addci_ui(r0, r1, i0)	ppc_addci_ui(_jit, r0, r1, i0)
__jit_inline void
ppc_addci_ui(jit_state_t _jit, int r0, int r1, int i0)
{
    if (jit_can_sign_extend_short_p(i0))
	ADDICrri(r0, r1, i0);
    else {
	jit_movi_i(JIT_AUX, i0);
	ADDCrrr(r0, r1, JIT_AUX);
    }
}

#define jit_addxr_ui(r0, r1, r2)	ADDErrr(r0, r1, r2)
#define jit_addxi_ui(r0, r1, i0)	ppc_addxi_ui(_jit, r0, r1, i0)
__jit_inline void
ppc_addxi_ui(jit_state_t _jit, int r0, int r1, int i0)
{
    jit_movi_i(JIT_AUX, i0);
    ADDErrr(r0, r1, JIT_AUX);
}

#define jit_subr_i(r0, r1, r2)		SUBrrr(r0, r1, r2)
#define jit_subi_i(r0, r1, i0)		ppc_subi_i(_jit, r0, r1, i0)
__jit_inline void
ppc_subi_i(jit_state_t _jit, int r0, int r1, int i0)
{
    int		ni0 = -i0;
    if (jit_can_sign_extend_short_p(ni0))
	ADDIrri(r0, r1, ni0);
    else if (!(ni0 & 0x0000ffff))
	ADDISrri(r0, r1, _jit_US(ni0 >> 16));
    else {
	jit_movi_i(JIT_AUX, i0);
	SUBrrr(r0, r1, JIT_AUX);
    }
}

#define jit_subcr_ui(r0, r1, r2)	SUBCrrr(r0, r1, r2)
#define jit_subci_ui(r0, r1, i0)	ppc_subci_ui(_jit, r0, r1, i0)
__jit_inline void
ppc_subci_ui(jit_state_t _jit, int r0, int r1, int i0)
{
    jit_movi_i(JIT_AUX, i0);
    SUBCrrr(r0, r1, JIT_AUX);
}

#define jit_subxr_ui(r0, r1, r2)	SUBErrr(r0, r1, r2)
#define jit_subxi_ui(r0, r1, i0)	ppc_subxi_ui(_jit, r0, r1, i0)
__jit_inline void
ppc_subxi_ui(jit_state_t _jit, int r0, int r1, int i0)
{
    jit_movi_i(JIT_AUX, i0);
    SUBErrr(r0, r1, JIT_AUX);
}

#define jit_hmulr_i(r0, r1, r2)		MULHWrrr(r0, r1, r2)
#define jit_hmuli_i(r0, r1, i0)		ppc_hmuli_i(_jit, r0, r1, i0)
__jit_inline void
ppc_hmuli_i(jit_state_t _jit, int r0, int r1, int i0)
{
    jit_movi_i(JIT_AUX, i0);
    MULHWrrr(r0, r1, JIT_AUX);
}

#define jit_hmulr_ui(r0, r1, r2)	MULHWUrrr(r0, r1, r2)
#define jit_hmuli_ui(r0, r1, i0)	ppc_hmuli_ui(_jit, r0, r1, i0)
__jit_inline void
ppc_hmuli_ui(jit_state_t _jit, int r0, int r1, int i0)
{
    jit_movi_i(JIT_AUX, i0);
    MULHWUrrr(r0, r1, JIT_AUX);
}

#define jit_mulr_i(r0, r1, r2)		MULLWrrr(r0, r1, r2)
#define jit_mulr_ui(r0, r1, r2)		jit_mulr_i(r0, r1, r2)
#define jit_muli_i(r0, r1, i0)		ppc_muli_i(_jit, r0, r1, i0)
#define jit_muli_ui(r0, r1, i0)		jit_muli_i(r0, r1, i0)
__jit_inline void
ppc_muli_i(jit_state_t _jit, int r0, int r1, int i0)
{
    if (jit_can_sign_extend_short_p(i0))
	MULLIrri(r0, r1, i0);
    else {
	jit_movi_i(JIT_AUX, i0);
	MULLWrrr(r0, r1, JIT_AUX);
    }
}

#define jit_divr_i(r0, r1, r2)		DIVWrrr(r0, r1, r2)
#define jit_divi_i(r0, r1, i0)		ppc_divi_i(_jit, r0, r1, i0)
__jit_inline void
ppc_divi_i(jit_state_t _jit, int r0, int r1, int i0)
{
    jit_movi_i(JIT_AUX, i0);
    DIVWrrr(r0, r1, JIT_AUX);
}

#define jit_divr_ui(r0, r1, r2)		DIVWUrrr(r0, r1, r2)
#define jit_divi_ui(r0, r1, i0)		ppc_divi_ui(_jit, r0, r1, i0)
__jit_inline void
ppc_divi_ui(jit_state_t _jit, int r0, int r1, int i0)
{
    jit_movi_i(JIT_AUX, i0);
    DIVWUrrr(r0, r1, JIT_AUX);
}

#define jit_modi_i(d, rs, is)		jit_chk_ims  ((is), _jit_mod_small(jit_divr_i , (d), (rs), (is)), _jit_mod_big(jit_divr_i , (d), (rs)))
#define jit_modi_ui(d, rs, is)		jit_chk_imu15((is), _jit_mod_small(jit_divr_ui, (d), (rs), (is)), _jit_mod_big(jit_divr_ui, (d), (rs)))
#define jit_modr_i(d, s1, s2)		(DIVWrrr(JIT_AUX, (s1), (s2)), MULLWrrr(JIT_AUX, JIT_AUX, (s2)), SUBrrr((d), (s1), JIT_AUX))
#define jit_modr_ui(d, s1, s2)		(DIVWUrrr(JIT_AUX, (s1), (s2)), MULLWrrr(JIT_AUX, JIT_AUX, (s2)), SUBrrr((d), (s1), JIT_AUX))

#define jit_andr_i(r0, r1, r2)		ANDrrr(r0, r1, r2)
#define jit_andi_i(r0, r1, i0)		ppc_andi_i(_jit, r0, r1, i0)
__jit_inline void
ppc_andi_i(jit_state_t _jit, int r0, int r1, int i0)
{
    if (jit_can_zero_extend_short_p(i0))
	ANDI_rri(r0, r1, i0);
    else if (!(i0 & 0x0000ffff))
	ANDIS_rri(r0, r1, _jit_US(i0 >> 16));
    else {
	jit_movi_i(JIT_AUX, i0);
	ANDrrr(r0, r1, JIT_AUX);
    }
}

#define jit_orr_i(r0, r1, r2)		ORrrr(r0, r1, r2)
#define jit_ori_i(r0, r1, i0)		ppc_ori_i(_jit, r0, r1, i0)
__jit_inline void
ppc_ori_i(jit_state_t _jit, int r0, int r1, int i0)
{
    if (jit_can_zero_extend_short_p(i0))
	ORIrri(r0, r1, i0);
    else if (!(i0 & 0x0000ffff))
	ORISrri(r0, r1, _jit_US(i0 >> 16));
    else {
	jit_movi_i(JIT_AUX, i0);
	ORrrr(r0, r1, JIT_AUX);
    }
}

#define jit_xorr_i(r0, r1, r2)		XORrrr(r0, r1, r2)
#define jit_xori_i(r0, r1, i0)		ppc_xori_i(_jit, r0, r1, i0)
__jit_inline void
ppc_xori_i(jit_state_t _jit, int r0, int r1, int i0)
{
    if (jit_can_zero_extend_short_p(i0))
	XORIrri(r0, r1, i0);
    else if (!(i0 & 0x0000ffff))
	XORISrri(r0, r1, _jit_US(i0 >> 16));
    else {
	jit_movi_i(JIT_AUX, i0);
	XORrrr(r0, r1, JIT_AUX);
    }
}

#define jit_lshr_i(r0, r1, r2)		SLWrrr(r0, r1, r2)
#define jit_lshi_i(r0, r1, i0)		SLWIrri(r0, r1, i0)
#define jit_rshr_i(r0, r1, r2)		SRAWrrr(r0, r1, r2)
#define jit_rshi_i(r0, r1, i0)		SRAWIrri(r0, r1, i0)
#define jit_rshr_ui(r0, r1, r2)		SRWrrr(r0, r1, r2)
#define jit_rshi_ui(r0, r1, i0)		ppc_rshi_ui(_jit, r0, r1, i0)
__jit_inline void
ppc_rshi_ui(jit_state_t _jit, int r0, int r1, int i0)
{
    if (i0)
	SRWIrri(r0, r1, i0);
    else
	jit_movr_i(r0, r1);
}

#define jit_ltr_i(r0, r1, r2)		ppc_ltr_i(_jit, r0, r1, r2)
__jit_inline void
ppc_ltr_i(jit_state_t _jit, int r0, int r1, int r2)
{
    CMPWrr(r1, r2);
    MFCRr(r0);
    EXTRWIrrii(r0, r0, 1, _lt);
}

#define jit_lti_i(r0, r1, i0)		ppc_lti_i(_jit, r0, r1, i0)
__jit_inline void
ppc_lti_i(jit_state_t _jit, int r0, int r1, int i0)
{
    if (jit_can_sign_extend_short_p(i0))
	CMPWIri(r1, i0);
    else {
	jit_movi_i(JIT_AUX, i0);
	CMPWrr(r1, JIT_AUX);
    }
    MFCRr(r0);
    EXTRWIrrii(r0, r0, 1, _lt);
}

#define jit_ltr_ui(r0, r1, r2)		ppc_ltr_ui(_jit, r0, r1, r2)
__jit_inline void
ppc_ltr_ui(jit_state_t _jit, int r0, int r1, int r2)
{
    CMPLWrr(r1, r2);
    MFCRr(r0);
    EXTRWIrrii(r0, r0, 1, _lt);
}

#define jit_lti_ui(r0, r1, i0)		ppc_lti_ui(_jit, r0, r1, i0)
__jit_inline void
ppc_lti_ui(jit_state_t _jit, int r0, int r1, int i0)
{
    if (jit_can_zero_extend_short_p(i0))
	CMPLWIri(r1, i0);
    else {
	jit_movi_i(JIT_AUX, i0);
	CMPLWrr(r1, JIT_AUX);
    }
    MFCRr(r0);
    EXTRWIrrii(r0, r0, 1, _lt);
}

#define jit_ler_i(r0, r1, r2)		ppc_ler_i(_jit, r0, r1, r2)
__jit_inline void
ppc_ler_i(jit_state_t _jit, int r0, int r1, int r2)
{
    CMPWrr(r1, r2);
    CRNOTii(_gt, _gt);
    MFCRr(r0);
    EXTRWIrrii(r0, r0, 1, _gt);
}

#define jit_lei_i(r0, r1, i0)		ppc_lei_i(_jit, r0, r1, i0)
__jit_inline void
ppc_lei_i(jit_state_t _jit, int r0, int r1, int i0)
{
    if (jit_can_sign_extend_short_p(i0))
	CMPWIri(r1, i0);
    else {
	jit_movi_i(JIT_AUX, i0);
	CMPWrr(r1, JIT_AUX);
    }
    CRNOTii(_gt, _gt);
    MFCRr(r0);
    EXTRWIrrii(r0, r0, 1, _gt);
}

#define jit_ler_ui(r0, r1, r2)		ppc_ler_ui(_jit, r0, r1, r2)
__jit_inline void
ppc_ler_ui(jit_state_t _jit, int r0, int r1, int r2)
{
    CMPLWrr(r1, r2);
    CRNOTii(_gt, _gt);
    MFCRr(r0);
    EXTRWIrrii(r0, r0, 1, _gt);
}

#define jit_lei_ui(r0, r1, i0)		ppc_lei_ui(_jit, r0, r1, i0)
__jit_inline void
ppc_lei_ui(jit_state_t _jit, int r0, int r1, int i0)
{
    if (jit_can_zero_extend_short_p(i0))
	CMPLWIri(r1, i0);
    else {
	jit_movi_i(JIT_AUX, i0);
	CMPLWrr(r1, JIT_AUX);
    }
    CRNOTii(_gt, _gt);
    MFCRr(r0);
    EXTRWIrrii(r0, r0, 1, _gt);
}

#define jit_eqr_i(r0, r1, r2)		ppc_eqr_i(_jit, r0, r1, r2)
__jit_inline void
ppc_eqr_i(jit_state_t _jit, int r0, int r1, int r2)
{
    CMPWrr(r1, r2);
    MFCRr(r0);
    EXTRWIrrii(r0, r0, 1, _eq);
}

#define jit_eqi_i(r0, r1, i0)		ppc_eqi_i(_jit, r0, r1, i0)
__jit_inline void
ppc_eqi_i(jit_state_t _jit, int r0, int r1, int i0)
{
    if (jit_can_sign_extend_short_p(i0))
	CMPWIri(r1, i0);
    else if (jit_can_zero_extend_short_p(i0))
	CMPLWIri(r1, i0);
    else {
	jit_movi_i(JIT_AUX, i0);
	CMPWrr(r1, JIT_AUX);
    }
    MFCRr(r0);
    EXTRWIrrii(r0, r0, 1, _eq);
}

#define jit_ger_i(r0, r1, r2)		ppc_ger_i(_jit, r0, r1, r2)
__jit_inline void
ppc_ger_i(jit_state_t _jit, int r0, int r1, int r2)
{
    CMPWrr(r1, r2);
    CRNOTii(_lt, _lt);
    MFCRr(r0);
    EXTRWIrrii(r0, r0, 1, _lt);
}

#define jit_gei_i(r0, r1, i0)		ppc_gei_i(_jit, r0, r1, i0)
__jit_inline void
ppc_gei_i(jit_state_t _jit, int r0, int r1, int i0)
{
    if (jit_can_sign_extend_short_p(i0))
	CMPWIri(r1, i0);
    else {
	jit_movi_i(JIT_AUX, i0);
	CMPWrr(r1, JIT_AUX);
    }
    CRNOTii(_lt, _lt);
    MFCRr(r0);
    EXTRWIrrii(r0, r0, 1, _lt);
}

#define jit_ger_ui(r0, r1, r2)		ppc_ger_ui(_jit, r0, r1, r2)
__jit_inline void
ppc_ger_ui(jit_state_t _jit, int r0, int r1, int r2)
{
    CMPLWrr(r1, r2);
    CRNOTii(_lt, _lt);
    MFCRr(r0);
    EXTRWIrrii(r0, r0, 1, _lt);
}

#define jit_gei_ui(r0, r1, i0)		ppc_gei_ui(_jit, r0, r1, i0)
__jit_inline void
ppc_gei_ui(jit_state_t _jit, int r0, int r1, int i0)
{
    if (jit_can_zero_extend_short_p(i0))
	CMPLWIri(r1, i0);
    else {
	jit_movi_i(JIT_AUX, i0);
	CMPLWrr(r1, JIT_AUX);
    }
    CRNOTii(_lt, _lt);
    MFCRr(r0);
    EXTRWIrrii(r0, r0, 1, _lt);
}

#define jit_gtr_i(r0, r1, r2)		ppc_gtr_i(_jit, r0, r1, r2)
__jit_inline void
ppc_gtr_i(jit_state_t _jit, int r0, int r1, int r2)
{
    CMPWrr(r1, r2);
    MFCRr(r0);
    EXTRWIrrii(r0, r0, 1, _gt);
}

#define jit_gti_i(r0, r1, i0)		ppc_gti_i(_jit, r0, r1, i0)
__jit_inline void
ppc_gti_i(jit_state_t _jit, int r0, int r1, int i0)
{
    if (jit_can_sign_extend_short_p(i0))
	CMPWIri(r1, i0);
    else {
	jit_movi_i(JIT_AUX, i0);
	CMPWrr(r1, JIT_AUX);
    }
    MFCRr(r0);
    EXTRWIrrii(r0, r0, 1, _gt);
}

#define jit_gtr_ui(r0, r1, r2)		ppc_gtr_ui(_jit, r0, r1, r2)
__jit_inline void
ppc_gtr_ui(jit_state_t _jit, int r0, int r1, int r2)
{
    CMPLWrr(r1, r2);
    MFCRr(r0);
    EXTRWIrrii(r0, r0, 1, _gt);
}

#define jit_gti_ui(r0, r1, i0)		ppc_gti_ui(_jit, r0, r1, i0)
__jit_inline void
ppc_gti_ui(jit_state_t _jit, int r0, int r1, int i0)
{
    if (jit_can_zero_extend_short_p(i0))
	CMPLWIri(r1, i0);
    else {
	jit_movi_i(JIT_AUX, i0);
	CMPLWrr(r1, JIT_AUX);
    }
    MFCRr(r0);
    EXTRWIrrii(r0, r0, 1, _gt);
}

#define jit_ner_i(r0, r1, r2)		ppc_ner_i(_jit, r0, r1, r2)
__jit_inline void
ppc_ner_i(jit_state_t _jit, int r0, int r1, int r2)
{
    CMPWrr(r1, r2);
    CRNOTii(_eq, _eq);
    MFCRr(r0);
    EXTRWIrrii(r0, r0, 1, _eq);
}

#define jit_nei_i(r0, r1, i0)		ppc_nei_i(_jit, r0, r1, i0)
__jit_inline void
ppc_nei_i(jit_state_t _jit, int r0, int r1, int i0)
{
    if (jit_can_sign_extend_short_p(i0))
	CMPWIri(r1, i0);
    else if (jit_can_zero_extend_short_p(i0))
	CMPLWIri(r1, i0);
    else {
	jit_movi_i(JIT_AUX, i0);
	CMPWrr(r1, JIT_AUX);
    }
    CRNOTii(_eq, _eq);
    MFCRr(r0);
    EXTRWIrrii(r0, r0, 1, _eq);
}

#define jit_bmsi_i(label, rs, is)	(jit_chk_imu((is), ANDI_rri(JIT_AUX, (rs), (is)), AND_rrr(JIT_AUX, (rs), JIT_AUX)), BNEi((label)), _jit->x.pc)
#define jit_bmci_i(label, rs, is)	(jit_chk_imu((is), ANDI_rri(JIT_AUX, (rs), (is)), AND_rrr(JIT_AUX, (rs), JIT_AUX)), BEQi((label)), _jit->x.pc)
#define jit_bmsr_i(label, s1, s2)	(		   AND_rrr(JIT_AUX, (s1), (s2)),				    BNEi((label)), _jit->x.pc)
#define jit_bmcr_i(label, s1, s2)	(		   AND_rrr(JIT_AUX, (s1), (s2)),				    BEQi((label)), _jit->x.pc)
#define jit_beqi_i(label, rs, is)	jit_s_brai((rs), (is), BEQi((label)) )
#define jit_beqr_i(label, s1, s2)	jit_s_brar((s1), (s2), BEQi((label)) )
#define jit_bgei_i(label, rs, is)	jit_s_brai((rs), (is), BGEi((label)) )
#define jit_bgei_ui(label, rs, is)	jit_u_brai((rs), (is), BGEi((label)) )
#define jit_bger_i(label, s1, s2)	jit_s_brar((s1), (s2), BGEi((label)) )
#define jit_bger_ui(label, s1, s2)	jit_u_brar((s1), (s2), BGEi((label)) )
#define jit_bgti_i(label, rs, is)	jit_s_brai((rs), (is), BGTi((label)) )
#define jit_bgti_ui(label, rs, is)	jit_u_brai((rs), (is), BGTi((label)) )
#define jit_bgtr_i(label, s1, s2)	jit_s_brar((s1), (s2), BGTi((label)) )
#define jit_bgtr_ui(label, s1, s2)	jit_u_brar((s1), (s2), BGTi((label)) )
#define jit_blei_i(label, rs, is)	jit_s_brai((rs), (is), BLEi((label)) )
#define jit_blei_ui(label, rs, is)	jit_u_brai((rs), (is), BLEi((label)) )
#define jit_bler_i(label, s1, s2)	jit_s_brar((s1), (s2), BLEi((label)) )
#define jit_bler_ui(label, s1, s2)	jit_u_brar((s1), (s2), BLEi((label)) )
#define jit_blti_i(label, rs, is)	jit_s_brai((rs), (is), BLTi((label)) )
#define jit_blti_ui(label, rs, is)	jit_u_brai((rs), (is), BLTi((label)) )
#define jit_bltr_i(label, s1, s2)	jit_s_brar((s1), (s2), BLTi((label)) )
#define jit_bltr_ui(label, s1, s2)	jit_u_brar((s1), (s2), BLTi((label)) )
#define jit_bnei_i(label, rs, is)	jit_s_brai((rs), (is), BNEi((label)) )
#define jit_bner_i(label, s1, s2)	jit_s_brar((s1), (s2), BNEi((label)) )

#define jit_boaddr_i(i0, r0, r1)	ppc_boaddr_i(_jit, i0, r0, r1)
__jit_inline jit_insn *
ppc_boaddr_i(jit_state_t _jit, jit_insn *i0, int r0, int r1)
{
    ADDOrrr(r0, r0, r1);
    MCRXRi(0);
    BGTi(i0);				/* GT = bit 1 of XER = OV */
    return (_jit->x.pc);
}

#define jit_boaddi_i(i0, r0, i1)	ppc_boaddi_i(_jit, i0, r0, i1)
__jit_inline jit_insn *
ppc_boaddi_i(jit_state_t _jit, jit_insn *i0, int r0, int i1)
{
    jit_movi_i(JIT_AUX, i1);
    return (jit_boaddr_i(i0, r0, JIT_AUX));
}

#define jit_bosubr_i(i0, r0, r1)	ppc_bosubr_i(_jit, i0, r0, r1)
__jit_inline jit_insn *
ppc_bosubr_i(jit_state_t _jit, jit_insn *i0, int r0, int r1)
{
    SUBOrrr(r0, r0, r1);
    MCRXRi(0);
    BGTi(i0);
    return (_jit->x.pc);
}

#define jit_bosubi_i(i0, r0, i1)	ppc_bosubi_i(_jit, i0, r0, i1)
__jit_inline jit_insn *
ppc_bosubi_i(jit_state_t _jit, jit_insn *i0, int r0, int i1)
{
    jit_movi_i(JIT_AUX, i1);
    return (jit_bosubr_i(i0, r0, JIT_AUX));
}

#define jit_boaddr_ui(i0, r0, r1)	ppc_boaddr_ui(_jit, i0, r0, r1)
__jit_inline jit_insn *
ppc_boaddr_ui(jit_state_t _jit, jit_insn *i0, int r0, int r1)
{
    ADDCrrr(r0, r0, r1);
    MCRXRi(0);
    BEQi(i0);				/* EQ = bit 2 of XER = CA */
    return (_jit->x.pc);
}

#define jit_boaddi_ui(i0, r0, i1)	ppc_boaddi_ui(_jit, i0, r0, i1)
__jit_inline jit_insn *
ppc_boaddi_ui(jit_state_t _jit, jit_insn *i0, int r0, int i1)
{
    if (jit_can_sign_extend_short_p(i1)) {
	ADDICrri(r0, r0, i1);
	MCRXRi(0);
	BEQi(i0);
	return (_jit->x.pc);
    }
    jit_movi_i(JIT_AUX, i1);
    return (jit_boaddr_ui(i0, r0, JIT_AUX));
}

#define jit_bosubr_ui(i0, r0, r1)	ppc_bosubr_ui(_jit, i0, r0, r1)
__jit_inline jit_insn *
ppc_bosubr_ui(jit_state_t _jit, jit_insn *i0, int r0, int r1)
{
    SUBCrrr(r0, r0, r1);
    MCRXRi(0);
    BNEi(i0);				/* PPC uses "carry" not "borrow" */
    return (_jit->x.pc);
}

#define jit_bosubi_ui(i0, r0, i1)	ppc_bosubi_ui(_jit, i0, r0, i1)
__jit_inline jit_insn *
ppc_bosubi_ui(jit_state_t _jit, jit_insn *i0, int r0, int i1)
{
    jit_movi_i(JIT_AUX, i1);
    return (jit_bosubr_ui(i0, r0, JIT_AUX));
}

#define jit_calli(label)	        ((void)jit_movi_p(JIT_AUX, (label)), MTCTRr(JIT_AUX), BCTRL(), _jitl.nextarg_puti = _jitl.nextarg_putf = _jitl.nextarg_putd = 0, _jit->x.pc)
#define jit_callr(reg)			(MTCTRr(reg), BCTRL())
#define jit_jmpi(label)			(B_EXT((label)), _jit->x.pc)
#define jit_jmpr(reg)			(MTLRr(reg), BLR())

#define jit_ldr_uc(r0, r1)		LBZrx(r0, 0, r1)
#define jit_ldi_uc(r0, i0)		ppc_ldi_uc(_jit, r0, i0)
__jit_inline void
ppc_ldi_uc(jit_state_t _jit, int r0, void *i0)
{
    int		lo, hi;
    if (jit_can_sign_extend_short_p((long)i0))
	LBZrm(r0, i0, 0);
    else {
	hi = ((_ui)i0 >> 16) + ((_us)((_ui)i0) >> 15);
	lo = (_us)((_ui)i0 - (hi << 16));
	LISri(JIT_AUX, hi);
	LBZrm(r0, lo, JIT_AUX);
    }
}

#define jit_ldxr_uc(r0, r1, r2)		ppc_ldxr_uc(_jit, r0, r1, r2)
__jit_inline void
ppc_ldxr_uc(jit_state_t _jit, int r0, int r1, int r2)
{
    if (r1 == 0) {
	if (r2 != 0)
	    LBZrx(r0, r2, r1);
	else {
	    jit_movr_i(JIT_AUX, r1);
	    LBZrx(r0, JIT_AUX, r2);
	}
    }
    else
	LBZrx(r0, r1, r2);
}

#define jit_ldxi_uc(r0, r1, i0)		ppc_ldxi_uc(_jit, r0, r1, i0)
__jit_inline void
ppc_ldxi_uc(jit_state_t _jit, int r0, int r1, int i0)
{
    if (i0 == 0)
	jit_ldr_uc(r0, r1);
    else if (jit_can_sign_extend_short_p(i0)) {
	if (r1 == 0) {
	    jit_movr_i(JIT_AUX, r1);
	    LBZrm(r0, i0, JIT_AUX);
	}
	else
	    LBZrm(r0, i0, r1);
    }
    else {
	jit_movi_i(JIT_AUX, i0);
	jit_ldxr_uc(r0, r1, JIT_AUX);
    }
}

#define jit_ldr_c(r0, r1)		ppc_ldr_c(_jit, r0, r1)
__jit_inline void
ppc_ldr_c(jit_state_t _jit, int r0, int r1)
{
    jit_ldr_uc(r0, r1);
    jit_extr_c_i(r0, r0);
}

#define jit_ldi_c(r0, i0)		ppc_ldi_c(_jit, r0, i0)
__jit_inline void
ppc_ldi_c(jit_state_t _jit, int r0, void *i0)
{
    jit_ldi_uc(r0, i0);
    jit_extr_c_i(r0, r0);
}

#define jit_ldxr_c(r0, r1, r2)		ppc_ldxr_c(_jit, r0, r1, r2)
__jit_inline void
ppc_ldxr_c(jit_state_t _jit, int r0, int r1, int r2)
{
    jit_ldxr_uc(r0, r1, r2);
    jit_extr_c_i(r0, r0);
}

#define jit_ldxi_c(r0, r1, i0)		ppc_ldxi_c(_jit, r0, r1, i0)
__jit_inline void
ppc_ldxi_c(jit_state_t _jit, int r0, int r1, int i0)
{
    jit_ldxi_uc(r0, r1, i0);
    jit_extr_c_i(r0, r0);
}

#define jit_ldr_s(r0, r1)		LHArx(r0, 0, r1)
#define jit_ldi_s(r0, i0)		ppc_ldi_s(_jit, r0, i0)
__jit_inline void
ppc_ldi_s(jit_state_t _jit, int r0, void *i0)
{
    int		lo, hi;
    if (jit_can_sign_extend_short_p((long)i0))
	LHArm(r0, i0, 0);
    else {
	hi = ((_ui)i0 >> 16) + ((_us)((_ui)i0) >> 15);
	lo = (_us)((_ui)i0 - (hi << 16));
	LISri(JIT_AUX, hi);
	LHArm(r0, lo, JIT_AUX);
    }
}

#define jit_ldxr_s(r0, r1, r2)		ppc_ldxr_s(_jit, r0, r1, r2)
__jit_inline void
ppc_ldxr_s(jit_state_t _jit, int r0, int r1, int r2)
{
    if (r1 == 0) {
	if (r2 != 0)
	    LHArx(r0, r2, r1);
	else {
	    jit_movr_i(JIT_AUX, r1);
	    LHArx(r0, JIT_AUX, r2);
	}
    }
    else
	LHArx(r0, r1, r2);
}

#define jit_ldxi_s(r0, r1, i0)		ppc_ldxi_s(_jit, r0, r1, i0)
__jit_inline void
ppc_ldxi_s(jit_state_t _jit, int r0, int r1, int i0)
{
    if (i0 == 0)
	jit_ldr_s(r0, r1);
    else if (jit_can_sign_extend_short_p(i0)) {
	if (r1 == 0) {
	    jit_movr_i(JIT_AUX, r1);
	    LHArm(r0, i0, JIT_AUX);
	}
	else
	    LHArm(r0, i0, r1);
    }
    else {
	jit_movi_i(JIT_AUX, i0);
	jit_ldxr_s(r0, r1, JIT_AUX);
    }
}

#define jit_ldr_us(r0, r1)		LHZrx(r0, 0, r1)
#define jit_ldi_us(r0, i0)		ppc_ldi_us(_jit, r0, i0)
__jit_inline void
ppc_ldi_us(jit_state_t _jit, int r0, void *i0)
{
    int		lo, hi;
    if (jit_can_sign_extend_short_p((long)i0))
	LHZrm(r0, i0, 0);
    else {
	hi = ((_ui)i0 >> 16) + ((_us)((_ui)i0) >> 15);
	lo = (_us)((_ui)i0 - (hi << 16));
	LISri(JIT_AUX, hi);
	LHZrm(r0, lo, JIT_AUX);
    }
}

#define jit_ldxr_us(r0, r1, r2)		ppc_ldxr_us(_jit, r0, r1, r2)
__jit_inline void
ppc_ldxr_us(jit_state_t _jit, int r0, int r1, int r2)
{
    if (r1 == 0) {
	if (r2 != 0)
	    LHZrx(r0, r2, r1);
	else {
	    jit_movr_i(JIT_AUX, r1);
	    LHZrx(r0, JIT_AUX, r2);
	}
    }
    else
	LHZrx(r0, r1, r2);
}

#define jit_ldxi_us(r0, r1, i0)		ppc_ldxi_us(_jit, r0, r1, i0)
__jit_inline void
ppc_ldxi_us(jit_state_t _jit, int r0, int r1, int i0)
{
    if (i0 == 0)
	jit_ldr_us(r0, r1);
    else if (jit_can_sign_extend_short_p(i0)) {
	if (r1 == 0) {
	    jit_movr_i(JIT_AUX, r1);
	    LHZrm(r0, i0, JIT_AUX);
	}
	else
	    LHZrm(r0, i0, r1);
    }
    else {
	jit_movi_i(JIT_AUX, i0);
	jit_ldxr_us(r0, r1, JIT_AUX);
    }
}

#define jit_ldr_i(r0, r1)		LWZrx(r0, 0, r1)
#define jit_ldi_i(r0, i0)		ppc_ldi_i(_jit, r0, i0)
__jit_inline void
ppc_ldi_i(jit_state_t _jit, int r0, void *i0)
{
    int		lo, hi;
    if (jit_can_sign_extend_short_p((long)i0))
	LWZrm(r0, i0, 0);
    else {
	hi = ((_ui)i0 >> 16) + ((_us)((_ui)i0) >> 15);
	lo = (_us)((_ui)i0 - (hi << 16));
	LISri(JIT_AUX, hi);
	LWZrm(r0, lo, JIT_AUX);
    }
}

#define jit_ldxr_i(r0, r1, r2)		ppc_ldxr_i(_jit, r0, r1, r2)
__jit_inline void
ppc_ldxr_i(jit_state_t _jit, int r0, int r1, int r2)
{
    if (r1 == 0) {
	if (r2 != r1)
	    LWZrx(r0, r2, r1);
	else {
	    jit_movr_i(JIT_AUX, r1);
	    LWZrx(r0, JIT_AUX, r2);
	}
    }
    else
	LWZrx(r0, r1, r2);
}

#define jit_ldxi_i(r0, r1, i0)		ppc_ldxi_i(_jit, r0, r1, i0)
__jit_inline void
ppc_ldxi_i(jit_state_t _jit, int r0, int r1, int i0)
{
    if (i0 == 0)
	jit_ldr_i(r0, r1);
    else if (jit_can_sign_extend_short_p(i0)) {
	if (r1 == 0) {
	    jit_movr_i(JIT_AUX, r1);
	    LWZrm(r0, i0, JIT_AUX);
	}
	else
	    LWZrm(r0, i0, r1);
    }
    else {
	jit_movi_i(JIT_AUX, i0);
	jit_ldxr_i(r0, r1, JIT_AUX);
    }
}

#define jit_str_c(r0, r1)		STBrx(r1, 0, r0)
#define jit_sti_c(i0, r0)		ppc_sti_c(_jit, i0, r0)
__jit_inline void
ppc_sti_c(jit_state_t _jit, void *i0, int r0)
{
    int		lo, hi;
    if (jit_can_sign_extend_short_p((long)i0))
	STBrm(r0, i0, 0);
    else {
	hi = ((_ui)i0 >> 16) + ((_us)((_ui)i0) >> 15);
	lo = (_us)((_ui)i0 - (hi << 16));
	LISri(JIT_AUX, hi);
	STBrm(r0, lo, JIT_AUX);
    }
}

#define jit_stxr_c(r0, r1, r2)		ppc_stxr_c(_jit, r0, r1, r2)
__jit_inline void
ppc_stxr_c(jit_state_t _jit, int r0, int r1, int r2)
{
    if (r0 == 0) {
	if (r1 != 0)
	    STBrx(r2, r1, r0);
	else {
	    jit_movr_i(JIT_AUX, r0);
	    STBrx(r2, 0, JIT_AUX);
	}
    }
    else
	STBrx(r2, r0, r1);
}

#define jit_stxi_c(i0, r0, r1)		ppc_stxi_c(_jit, i0, r0, r1)
__jit_inline void
ppc_stxi_c(jit_state_t _jit, int i0, int r0, int r1)
{
    if (i0 == 0)
	jit_str_c(r0, r1);
    else if (jit_can_sign_extend_short_p(i0)) {
	if (r0 == 0) {
	    jit_movr_i(JIT_AUX, r0);
	    STBrm(r1, i0, JIT_AUX);
	}
	else
	    STBrm(r1, i0, r0);
    }
    else {
	jit_movi_i(JIT_AUX, i0);
	jit_stxr_c(JIT_AUX, r0, r1);
    }
}

#define jit_str_s(r0, r1)		STHrx(r1, 0, r0)
#define jit_sti_s(i0, r0)		ppc_sti_s(_jit, i0, r0)
__jit_inline void
ppc_sti_s(jit_state_t _jit, void *i0, int r0)
{
    int		lo, hi;
    if (jit_can_sign_extend_short_p((long)i0))
	STHrm(r0, i0, 0);
    else {
	hi = ((_ui)i0 >> 16) + ((_us)((_ui)i0) >> 15);
	lo = (_us)((_ui)i0 - (hi << 16));
	LISri(JIT_AUX, hi);
	STHrm(r0, lo, JIT_AUX);
    }
}

#define jit_stxr_s(r0, r1, r2)		ppc_stxr_s(_jit, r0, r1, r2)
__jit_inline void
ppc_stxr_s(jit_state_t _jit, int r0, int r1, int r2)
{
    if (r0 == 0) {
	if (r1 != 0)
	    STHrx(r2, r1, r0);
	else {
	    jit_movr_i(JIT_AUX, r0);
	    STHrx(r2, 0, JIT_AUX);
	}
    }
    else
	STHrx(r2, r0, r1);
}

#define jit_stxi_s(i0, r0, r1)		ppc_stxi_s(_jit, i0, r0, r1)
__jit_inline void
ppc_stxi_s(jit_state_t _jit, int i0, int r0, int r1)
{
    if (i0 == 0)
	jit_str_s(r0, r1);
    else if (jit_can_sign_extend_short_p(i0)) {
	if (r0 == 0) {
	    jit_movr_i(JIT_AUX, r0);
	    STHrm(r1, i0, JIT_AUX);
	}
	else
	    STHrm(r1, i0, r0);
    }
    else {
	jit_movi_i(JIT_AUX, i0);
	jit_stxr_s(JIT_AUX, r0, r1);
    }
}

#define jit_str_i(r0, r1)		STWrx(r1, 0, r0)
#define jit_sti_i(i0, r0)		ppc_sti_i(_jit, i0, r0)
__jit_inline void
ppc_sti_i(jit_state_t _jit, void *i0, int r0)
{
    int		lo, hi;
    if (jit_can_sign_extend_short_p((long)i0))
	STWrm(r0, i0, 0);
    else {
	hi = ((_ui)i0 >> 16) + ((_us)((_ui)i0) >> 15);
	lo = (_us)((_ui)i0 - (hi << 16));
	LISri(JIT_AUX, hi);
	STWrm(r0, lo, JIT_AUX);
    }
}

#define jit_stxr_i(r0, r1, r2)		ppc_stxr_i(_jit, r0, r1, r2)
__jit_inline void
ppc_stxr_i(jit_state_t _jit, int r0, int r1, int r2)
{
    if (r0 == 0) {
	if (r1 != 0)
	    STWrx(r2, r1, r0);
	else {
	    jit_movr_i(JIT_AUX, r0);
	    STWrx(r2, 0, JIT_AUX);
	}
    }
    else
	STWrx(r2, r0, r1);
}

#define jit_stxi_i(i0, r0, r1)		ppc_stxi_i(_jit, i0, r0, r1)
__jit_inline void
ppc_stxi_i(jit_state_t _jit, int i0, int r0, int r1)
{
    if (i0 == 0)
	jit_str_i(r0, r1);
    else if (jit_can_sign_extend_short_p(i0)) {
	if (r0 == 0) {
	    jit_movr_i(JIT_AUX, r0);
	    STWrm(r1, i0, JIT_AUX);
	}
	else
	    STWrm(r1, i0, r0);
    }
    else {
	jit_movi_i(JIT_AUX, i0);
	jit_stxr_i(JIT_AUX, r0, r1);
    }
}

#define jit_nop()			NOP()

#ifdef JIT_NEED_PUSH_POP
#define jit_popr_i(rs)			(LWZrm((rs),  0, 1), ADDIrri(1, 1, 4))
#define jit_pushr_i(rs)			(STWrm((rs), -4, 1), ADDIrri (1, 1, -4))
#endif

#define jit_prepare_i(numi)		(_jitl.nextarg_puti = numi)
#define jit_prepare_f(numf)		(_jitl.nextarg_putf = numf)
#define jit_prepare_d(numd)		(_jitl.nextarg_putd = numd)

#define jit_prolog(n)			ppc_prolog(_jit, (n))
/* Emit a prolog for a function.
  
   The +32 in frame_size computation is to accound for the parameter area of
   a function frame. 

   On PPC the frame must have space to host the arguments of any callee.
   However, as it currently stands, the argument to jit_trampoline (n) is
   the number of arguments of the caller we generate. Therefore, the
   callee can overwrite a part of the stack (saved register area) when it
   flushes its own parameter on the stack. The addition of a constant 
   offset = 32 is enough to hold eight 4 bytes arguments.  This is less
   than perfect but is a reasonable work around for now. 
   Better solution must be investigated.  */
__jit_inline void
ppc_prolog(jit_state_t _jit, int n)
{
  int frame_size;
  int i;
  int first_saved_reg = JIT_AUX - n;
  int num_saved_regs = 32 - first_saved_reg;

  _jitl.nextarg_geti = JIT_AUX - 1;
  _jitl.nextarg_getd = 1;
  _jitl.nbArgs = n;

  MFLRr(0);

#ifdef __APPLE__
  STWrm(0, 8, 1);			/* stw   r0, 8(r1)	   */
#else
  STWrm(0, 4, 1);			/* stw   r0, 4(r1)	   */
#endif

  /* 0..55 -> frame data
     56..frame_size -> saved registers

     The STMW instruction is patched by jit_allocai, thus leaving
     the space for the allocai above the 56 bytes.  jit_allocai is
     also able to reuse the slack space needed to keep the stack
     quadword-aligned.  */

  _jitl.frame_size = 24 + 32 + num_saved_regs * 4;	/* r27..r31 + args */

  /* The stack must be quad-word aligned.  */
  frame_size = (_jitl.frame_size + 15) & ~15;
  _jitl.slack = frame_size - _jitl.frame_size;
  _jitl.stwu = _jit->x.ui_pc;
  STWUrm(1, -frame_size, 1);		/* stwu  r1, -x(r1)	   */

  STMWrm(first_saved_reg, 24 + 32, 1);		/* stmw  rI, ofs(r1)	   */
  for (i = 0; i < n; i++)
    MRrr(JIT_AUX-1-i, 3+i);		/* save parameters below r24	   */
}

#define jit_pusharg_i(rs)		(--_jitl.nextarg_puti, MRrr((3 + _jitl.nextarg_putd * 2 + _jitl.nextarg_putf + _jitl.nextarg_puti), (rs)))

#define jit_ret()			ppc_ret(_jit)
__jit_inline void
ppc_ret(jit_state_t _jit)
{
  int n = _jitl.nbArgs;
  int first_saved_reg = JIT_AUX - n;
  int frame_size = (_jitl.frame_size + 15) & ~15;

#ifdef __APPLE__
  LWZrm(0, frame_size + 8, 1);		/* lwz   r0, x+8(r1)  (ret.addr.)  */
#else
  LWZrm(0, frame_size + 4, 1);		/* lwz   r0, x+4(r1)  (ret.addr.)  */
#endif
  MTLRr(0);				/* mtspr LR, r0			   */

  LMWrm(first_saved_reg, 24 + 32, 1);	/* lmw   rI, ofs(r1)		   */
  ADDIrri(1, 1, frame_size);		/* addi  r1, r1, x		   */
  BLR();				/* blr				   */
}

#define jit_retval_i(rd)		MRrr((rd), 3)
#define jit_rsbi_i(d, rs, is)		jit_chk_ims((is), SUBFICrri((d), (rs), (is)), SUBFCrrr((d), (rs), JIT_AUX))

#endif /* __lightning_core_h */
