/******************************** -*- C -*- ****************************
 *
 *	Platform-independent layer (arm version)
 *
 ***********************************************************************/

/***********************************************************************
 *
 * Copyright 2011 Free Software Foundation, Inc.
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
 * Free Software Foundation, 59 Temple Place - Suite 330, Boston,
 * MA 02111-1307, USA.
 *
 * Authors:
 *	Paulo Cesar Pereira de Andrade
 ***********************************************************************/

#ifndef __lightning_core_arm_h
#define __lightning_core_arm_h

#import "../lightning.h"

#import "asm.h"

#define JIT_R_NUM			4
static const jit_gpr_t
jit_r_order[JIT_R_NUM] = {
    _R0, _R1, _R2, _R3
};
#define JIT_R(i)			jit_r_order[i]

#define JIT_V_NUM			4
static const jit_gpr_t
jit_v_order[JIT_V_NUM] = {
    _R4, _R5, _R6, _R7
};
#define JIT_V(i)			jit_v_order[i]

#define JIT_FRAMESIZE			48

#define jit_no_set_flags()		jit_flags.no_set_flags
#define jit_armv5_p()			(jit_cpu.version >= 5)
#define jit_armv5e_p()			(jit_cpu.version >= 5 && jit_cpu.extend)
#define jit_armv7r_p()			0
#define jit_swf_p()			(jit_cpu.vfp == 0)
#define jit_hardfp_p()			jit_cpu.abi

extern int	__aeabi_idivmod(int, int);
extern unsigned	__aeabi_uidivmod(unsigned, unsigned);

#define jit_nop(n)			arm_nop(_jit, n)
__jit_inline void
arm_nop(jit_state_t _jit, int n)
{
    assert(n >= 0);
    if (jit_thumb_p()) {
	for (; n > 0; n -= 2)
	    T1_NOP();
    }
    else {
	for (; n > 0; n -= 4)
	    _NOP();
    }
}

#define jit_movr_i(r0, r1)		arm_movr_i(_jit, r0, r1)
__jit_inline void
arm_movr_i(jit_state_t _jit, jit_gpr_t r0, jit_gpr_t r1)
{
    if (r0 != r1) {
	if (jit_thumb_p())
	    T1_MOV(r0, r1);
	else
	    _MOV(r0, r1);
    }
}

#define jit_movi_i(r0, i0)		arm_movi_i(_jit, r0, i0)
__jit_inline void
arm_movi_i(jit_state_t _jit, jit_gpr_t r0, int i0)
{
    int		i;
    if (jit_thumb_p()) {
	if (!jit_no_set_flags() && r0 < 8 && !(i0 & 0xffffff80))
	    T1_MOVI(r0, i0);
	else if ((i = encode_thumb_immediate(i0)) != -1)
	    T2_MOVI(r0, i);
	else if ((i = encode_thumb_immediate(~i0)) != -1)
	    T2_MVNI(r0, i);
	else {
	    T2_MOVWI(r0, _jit_US(i0));
	    if (i0 & 0xffff0000)
		T2_MOVTI(r0, _jit_US((unsigned)i0 >> 16));
	}
    }
    else {
	if (jit_armv6_p() && !(i0 & 0xffff0000))
	    _MOVWI(r0, i0);
	else if ((i = encode_arm_immediate(i0)) != -1)
	    _MOVI(r0, i);
	else if ((i = encode_arm_immediate(~i0)) != -1)
	    _MVNI(r0, i);
	else if (jit_armv6_p()) {
	    _MOVWI(r0, _jit_US(i0));
	    if ((i0 & 0xffff0000))
		_MOVTI(r0, _jit_US((unsigned)i0 >> 16));
	}
	else {
	    int     p0, p1, p2, p3, q0, q1, q2, q3;
	    p0 = i0 & 0x000000ff;   p1 = i0 & 0x0000ff00;
	    p2 = i0 & 0x00ff0000;   p3 = i0 & 0xff000000;
	    i0 = ~i0;
	    q0 = i0 & 0x000000ff;   q1 = i0 & 0x0000ff00;
	    q2 = i0 & 0x00ff0000;   q3 = i0 & 0xff000000;
	    if (!!p0 + !!p1 + !!p2 + !!p3 <= !!q0 + !!q1 + !!q2 + !!q3) {
		/* prefer no inversion on tie */
		if (p3) {
		    _MOVI(r0, encode_arm_immediate(p3));
		    if (p2) _ORRI(r0, r0, encode_arm_immediate(p2));
		    if (p1) _ORRI(r0, r0, encode_arm_immediate(p1));
		    if (p0) _ORRI(r0, r0, p0);
		}
		else if (p2) {
		    _MOVI(r0, encode_arm_immediate(p2));
		    if (p1) _ORRI(r0, r0, encode_arm_immediate(p1));
		    if (p0) _ORRI(r0, r0, p0);
		}
		else {
		    _MOVI(r0, encode_arm_immediate(p1));
		    _ORRI(r0, r0, p0);
		}
	    }
	    else {
		if (q3) {
		    _MVNI(r0, encode_arm_immediate(q3));
		    if (q2) _EORI(r0, r0, encode_arm_immediate(q2));
		    if (q1) _EORI(r0, r0, encode_arm_immediate(q1));
		    if (q0) _EORI(r0, r0, q0);
		}
		else if (q2) {
		    _MVNI(r0, encode_arm_immediate(q2));
		    if (q1) _EORI(r0, r0, encode_arm_immediate(q1));
		    if (q0) _EORI(r0, r0, q0);
		}
		else {
		    _MVNI(r0, encode_arm_immediate(q1));
		    _EORI(r0, r0, q0);
		}
	    }
	}
    }
}

#define jit_movi_p(r0, i0)		arm_movi_p(_jit, r0, i0)
__jit_inline jit_insn *
arm_movi_p(jit_state_t _jit, jit_gpr_t r0, void *i0)
{
    jit_insn	*l;
    uintptr_t im;
    int		 q0, q1, q2, q3;
    im = (uintptr_t)i0; 		l = _jit->x.pc;
    if (jit_thumb_p()) {
	T2_MOVWI(r0, _jit_US((uintptr_t)i0));
	T2_MOVTI(r0, _jit_US((uintptr_t)i0 >> 16));
    }
    else {
	if (jit_armv6_p()) {
	    _MOVWI(r0, _jit_US((uintptr_t)i0));
	    _MOVTI(r0, _jit_US((uintptr_t)i0 >> 16));
	}
	else {
	    q0 = im & 0x000000ff;	q1 = im & 0x0000ff00;
	    q2 = im & 0x00ff0000;	q3 = im & 0xff000000;
	    _MOVI(r0, encode_arm_immediate(q3));
	    _ORRI(r0, r0, encode_arm_immediate(q2));
	    _ORRI(r0, r0, encode_arm_immediate(q1));
	    _ORRI(r0, r0, q0);
	}
    }
    return (l);
}

#define jit_patch_movi(i0, i1)		arm_patch_movi(_jit, i0, i1)
__jit_inline void
arm_patch_movi(jit_state_t _jit, jit_insn *i0, void *i1)
{
    union {
	short		*s;
	int		*i;
	void		*v;
    } u;
    jit_thumb_t		 thumb;
    uintptr_t	 im;
    int			 q0, q1, q2, q3;
    im = (uintptr_t)i1;		u.v = i0;
    if (jit_thumb_p()) {
	q0 = (im & 0xf000) << 4;
	q1 = (im & 0x0800) << 15;
	q2 = (im & 0x0700) << 4;
	q3 =  im & 0x00ff;
	code2thumb(thumb.s[0], thumb.s[1], u.s[0], u.s[1]);
	assert(   (thumb.i & 0xfbf00000) == THUMB2_MOVWI);
	thumb.i = (thumb.i & 0xfbf00f00) | q0 | q1 | q2 | q3;
	thumb2code(thumb.s[0], thumb.s[1], u.s[0], u.s[1]);
	im >>= 16;
	q0 = (im & 0xf000) << 4;
	q1 = (im & 0x0800) << 15;
	q2 = (im & 0x0700) << 4;
	q3 =  im & 0x00ff;
	code2thumb(thumb.s[0], thumb.s[1], u.s[2], u.s[3]);
	assert(   (thumb.i & 0xfbf00000) == THUMB2_MOVTI);
	thumb.i = (thumb.i & 0xfbf00f00) | q0 | q1 | q2 | q3;
	thumb2code(thumb.s[0], thumb.s[1], u.s[2], u.s[3]);
    }
    else {
	if (jit_armv6_p()) {
	    q0 =  im &      0xfff;
	    q1 = (im &     0xf000) <<  4;
	    q2 = (im &  0xfff0000) >> 16;
	    q3 = (im & 0xf0000000) >> 12;
	    assert(  (u.i[0] & 0x0ff00000) == (ARM_MOVWI));
	    assert(  (u.i[1] & 0x0ff00000) == (ARM_MOVTI));
	    u.i[0] = (u.i[0] & 0xfff0f000) | q1 | q0;
	    u.i[1] = (u.i[1] & 0xfff0f000) | q3 | q2;
	}
	else {
	    q0 = im & 0x000000ff;	q1 = im & 0x0000ff00;
	    q2 = im & 0x00ff0000;	q3 = im & 0xff000000;
	    assert(  (u.i[0] & 0x0ff00000) == (ARM_MOV|ARM_I));
	    u.i[0] = (u.i[0] & 0xfffff000) | encode_arm_immediate(q3);
	    assert(  (u.i[1] & 0x0ff00000) == (ARM_ORR|ARM_I));
	    u.i[1] = (u.i[1] & 0xfffff000) | encode_arm_immediate(q2);
	    assert(  (u.i[2] & 0x0ff00000) == (ARM_ORR|ARM_I));
	    u.i[2] = (u.i[2] & 0xfffff000) | encode_arm_immediate(q1);
	    assert(  (u.i[3] & 0x0ff00000) == (ARM_ORR|ARM_I));
	    u.i[3] = (u.i[3] & 0xfffff000) | encode_arm_immediate(q0);
	}
    }
}

#define jit_patch_calli(i0, i1)		arm_patch_at(_jit, i0, i1)
#define jit_patch_at(jump, label)	arm_patch_at(_jit, jump, label)
__jit_inline void
arm_patch_at(jit_state_t _jit, jit_insn *jump, jit_insn *label)
{
    long		 d;
    union {
	short		*s;
	int		*i;
	void		*v;
    } u;
    jit_thumb_t		 thumb;
    u.v = jump;
    if (jit_thumb_p() && jump >= _jitl.thumb) {
#if 0
	/* this actually matches other patterns, so cannot patch
	 * automatically short jumps */
	if ((u.s[0] & THUMB_CC_B) == THUMB_CC_B) {
	    assert(_s8P(d));
	    u.s[0] = (u.s[0] & 0xff00) | (d & 0xff);
	}
	else if ((u.s[0] & THUMB_B) == THUMB_B) {
	    assert(_s11P(d));
	    u.s[0] = (u.s[0] & 0xf800) | (d & 0x7ff);
	}
	else
#endif
	{
	    code2thumb(thumb.s[0], thumb.s[1], u.s[0], u.s[1]);
	    if ((thumb.i & THUMB2_B) == THUMB2_B) {
		d = (((long)label - (long)jump) >> 1) - 2;
		assert(_s24P(d));
		thumb.i = THUMB2_B | encode_thumb_jump(d);
		thumb2code(thumb.s[0], thumb.s[1], u.s[0], u.s[1]);
	    }
	    else if ((thumb.i & THUMB2_B) == THUMB2_CC_B) {
		d = (((long)label - (long)jump) >> 1) - 2;
		assert(_s20P(d));
		thumb.i = THUMB2_CC_B | (thumb.i & 0x3c00000) |
			  encode_thumb_cc_jump(d);
		thumb2code(thumb.s[0], thumb.s[1], u.s[0], u.s[1]);
	    }
	    else if ((thumb.i & 0xfbf08000) == THUMB2_MOVWI)
		jit_patch_movi(jump, label);
	    else
		assert(!"handled branch opcode");
	}
    }
    else {
	/* 0x0e000000 because 0x01000000 is (branch&) link modifier */
	if ((u.i[0] & 0x0e000000) == ARM_B) {
	    d = (((long)label - (long)jump) >> 2) - 2;
	    assert(_s24P(d));
	    u.i[0] = (u.i[0] & 0xff000000) | (d & 0x00ffffff);
	}
	else if (( jit_armv6_p() && (u.i[0] & 0x0ff00000) == ARM_MOVWI) ||
		 (!jit_armv6_p() && (u.i[0] & 0x0ff00000) == (ARM_MOV|ARM_I)))
	    jit_patch_movi(jump, label);
	else
	    assert(!"handled branch opcode");
    }
}

#define jit_notr_i(r0, r1)		arm_notr_i(_jit, r0, r1)
__jit_inline void
arm_notr_i(jit_state_t _jit, jit_gpr_t r0, jit_gpr_t r1)
{
    if (jit_thumb_p()) {
	if (!jit_no_set_flags() && (r0|r1) < 8)
	    T1_NOT(r0, r1);
	else
	    T2_NOT(r0, r1);
    }
    else
	_NOT(r0, r1);
}

#define jit_negr_i(r0, r1)		arm_negr_i(_jit, r0, r1)
__jit_inline void
arm_negr_i(jit_state_t _jit, jit_gpr_t r0, jit_gpr_t r1)
{
    if (jit_thumb_p()) {
	if (!jit_no_set_flags() && (r0|r1) < 8)
	    T1_RSBI(r0, r1);
	else
	    T2_RSBI(r0, r1, 0);
    }
    else
	_RSBI(r0, r1, 0);
}

#define jit_addr_i(r0, r1, r2)		arm_addr_i(_jit, r0, r1, r2)
__jit_inline void
arm_addr_i(jit_state_t _jit, jit_gpr_t r0, jit_gpr_t r1, jit_gpr_t r2)
{
    if (jit_thumb_p()) {
	if (!jit_no_set_flags() && (r0|r1|r2) < 8)
	    T1_ADD(r0, r1, r2);
	else if (r0 == r1 || r0 == r2)
	    T1_ADDX(r0, r0 == r1 ? r2 : r1);
	else
	    T2_ADD(r0, r1, r2);
    }
    else
	_ADD(r0, r1, r2);
}

#define jit_addi_i(r0, r1, i0)		arm_addi_i(_jit, r0, r1, i0)
__jit_inline void
arm_addi_i(jit_state_t _jit, jit_gpr_t r0, jit_gpr_t r1, int i0)
{
    int		i;
    jit_gpr_t	reg;
    if (jit_thumb_p()) {
	if (!jit_no_set_flags() && (r0|r1) < 8 && !(i0 & ~7))
	    T1_ADDI3(r0, r1, i0);
	else if (!jit_no_set_flags() && (r0|r1) < 8 && !(-i0 & ~7))
	    T1_SUBI3(r0, r1, -i0);
	else if (!jit_no_set_flags() && r0 < 8 && r0 == r1 && !(i0 & ~0xff))
	    T1_ADDI8(r0, i0);
	else if (!jit_no_set_flags() && r0 < 8 && r0 == r1 && !(-i0 & ~0xff))
	    T1_SUBI8(r0, -i0);
	else if ((i = encode_thumb_immediate(i0)) != -1)
	    T2_ADDI(r0, r1, i);
	else if ((i = encode_thumb_immediate(-i0)) != -1)
	    T2_SUBI(r0, r1, i);
	else if ((i = encode_thumb_word_immediate(i0)) != -1)
	    T2_ADDWI(r0, r1, i);
	else if ((i = encode_thumb_word_immediate(-i0)) != -1)
	    T2_SUBWI(r0, r1, i);
	else {
	    reg = r0 != r1 ? r0 : JIT_TMP;
	    jit_movi_i(reg, i0);
	    T2_ADD(r0, r1, reg);
	}
    }
    else {
	if ((i = encode_arm_immediate(i0)) != -1)
	    _ADDI(r0, r1, i);
	else if ((i = encode_arm_immediate(-i0)) != -1)
	    _SUBI(r0, r1, i);
	else {
	    reg = r0 != r1 ? r0 : JIT_TMP;
	    jit_movi_i(reg, i0);
	    _ADD(r0, r1, reg);
	}
    }
}

#define jit_addcr_ui(r0, r1, r2)	arm_addcr_ui(_jit, r0, r1, r2)
__jit_inline void
arm_addcr_ui(jit_state_t _jit, jit_gpr_t r0, jit_gpr_t r1, jit_gpr_t r2)
{
    if (jit_thumb_p()) {
	/* thumb auto set carry if not inside IT block */
	if ((r0|r1|r2) < 8)
	    T1_ADD(r0, r1, r2);
	else
	    T2_ADDS(r0, r1, r2);
    }
    else
	_ADDS(r0, r1, r2);
}

#define jit_addci_ui(r0, r1, i0)	arm_addci_ui(_jit, r0, r1, i0)
__jit_inline void
arm_addci_ui(jit_state_t _jit, jit_gpr_t r0, jit_gpr_t r1, int i0)
{
    int		i;
    jit_gpr_t	reg;
    if (jit_thumb_p()) {
	if ((r0|r1) < 8 && !(i0 & ~7))
	    T1_ADDI3(r0, r1, i0);
	else if ((r0|r1) < 8 && !(-i0 & ~7))
	    T1_SUBI3(r0, r1, -i0);
	else if (r0 < 8 && r0 == r1 && !(i0 & ~0xff))
	    T1_ADDI8(r0, i0);
	else if (r0 < 8 && r0 == r1 && !(-i0 & ~0xff))
	    T1_SUBI8(r0, -i0);
	else if ((i = encode_thumb_immediate(i0)) != -1)
	    T2_ADDSI(r0, r1, i);
	else if ((i = encode_thumb_immediate(-i0)) != -1)
	    T2_SUBSI(r0, r1, i);
	else {
	    reg = r0 != r1 ? r0 : JIT_TMP;
	    jit_movi_i(reg, i0);
	    T2_ADDS(r0, r1, reg);
	}
    }
    else {
	if ((i = encode_arm_immediate(i0)) != -1)
	    _ADDSI(r0, r1, i);
	else if ((i = encode_arm_immediate(-i0)) != -1)
	    _SUBSI(r0, r1, i);
	else {
	    reg = r0 != r1 ? r0 : JIT_TMP;
	    jit_movi_i(reg, i0);
	    _ADDS(r0, r1, reg);
	}
    }
}

#define jit_addxr_ui(r0, r1, r2)	arm_addxr_ui(_jit, r0, r1, r2)
__jit_inline void
arm_addxr_ui(jit_state_t _jit, jit_gpr_t r0, jit_gpr_t r1, jit_gpr_t r2)
{
    /* keep setting carry because don't know last ADC */
    if (jit_thumb_p()) {
	/* thumb auto set carry if not inside IT block */
	if ((r0|r1|r2) < 8 && (r0 == r1 || r0 == r2))
	    T1_ADC(r0, r0 == r1 ? r2 : r1);
	else
	    T2_ADCS(r0, r1, r2);
    }
    else
	_ADCS(r0, r1, r2);
}

#define jit_addxi_ui(r0, r1, i0)	arm_addxi_ui(_jit, r0, r1, i0)
__jit_inline void
arm_addxi_ui(jit_state_t _jit, jit_gpr_t r0, jit_gpr_t r1, int i0)
{
    int		i;
    jit_gpr_t	reg;
    if (jit_thumb_p()) {
	if ((i = encode_thumb_immediate(i0)) != -1)
	    T2_ADCSI(r0, r1, i);
	else if ((i = encode_thumb_immediate(-i0)) != -1)
	    T2_SBCSI(r0, r1, i);
	else {
	    int		no_set_flags = jit_no_set_flags();
	    reg = r0 != r1 ? r0 : JIT_TMP;
	    jit_no_set_flags() = 1;
	    jit_movi_i(reg, i0);
	    jit_no_set_flags() = no_set_flags;
	    T2_ADCS(r0, r1, reg);
	}
    }
    else {
	if ((i = encode_arm_immediate(i0)) != -1)
	    _ADCSI(r0, r1, i);
	else if ((i = encode_arm_immediate(-i0)) != -1)
	    _SBCSI(r0, r1, i);
	else {
	    reg = r0 != r1 ? r0 : JIT_TMP;
	    jit_movi_i(reg, i0);
	    _ADCS(r0, r1, reg);
	}
    }
}

#define jit_subr_i(r0, r1, r2)		arm_subr_i(_jit, r0, r1, r2)
__jit_inline void
arm_subr_i(jit_state_t _jit, jit_gpr_t r0, jit_gpr_t r1, jit_gpr_t r2)
{
    if (jit_thumb_p()) {
	if (!jit_no_set_flags() && (r0|r1|r2) < 8)
	    T1_SUB(r0, r1, r2);
	else
	    T2_SUB(r0, r1, r2);
    }
    else
	_SUB(r0, r1, r2);
}

#define jit_subi_i(r0, r1, i0)		arm_subi_i(_jit, r0, r1, i0)
__jit_inline void
arm_subi_i(jit_state_t _jit, jit_gpr_t r0, jit_gpr_t r1, int i0)
{
    int		i;
    jit_gpr_t	reg;
    if (jit_thumb_p()) {
	if (!jit_no_set_flags() && (r0|r1) < 8 && !(i0 & ~7))
	    T1_SUBI3(r0, r1, i0);
	else if (!jit_no_set_flags() && (r0|r1) < 8 && !(-i0 & ~7))
	    T1_ADDI3(r0, r1, -i0);
	else if (!jit_no_set_flags() && r0 < 8 && r0 == r1 && !(i0 & ~0xff))
	    T1_SUBI8(r0, i0);
	else if (!jit_no_set_flags() && r0 < 8 && r0 == r1 && !(-i0 & ~0xff))
	    T1_ADDI8(r0, -i0);
	else if ((i = encode_thumb_immediate(i0)) != -1)
	    T2_SUBI(r0, r1, i);
	else if ((i = encode_thumb_immediate(-i0)) != -1)
	    T2_ADDI(r0, r1, i);
	else if ((i = encode_thumb_word_immediate(i0)) != -1)
	    T2_SUBWI(r0, r1, i);
	else if ((i = encode_thumb_word_immediate(-i0)) != -1)
	    T2_ADDWI(r0, r1, i);
	else {
	    reg = r0 != r1 ? r0 : JIT_TMP;
	    jit_movi_i(reg, i0);
	    T2_SUB(r0, r1, reg);
	}
    }
    else {
	if ((i = encode_arm_immediate(i0)) != -1)
	    _SUBI(r0, r1, i);
	else if ((i = encode_arm_immediate(-i0)) != -1)
	    _ADDI(r0, r1, i);
	else {
	    reg = r0 != r1 ? r0 : JIT_TMP;
	    jit_movi_i(reg, i0);
	    _SUB(r0, r1, reg);
	}
    }
}

#define jit_subcr_ui(r0, r1, r2)	arm_subcr_ui(_jit, r0, r1, r2)
__jit_inline void
arm_subcr_ui(jit_state_t _jit, jit_gpr_t r0, jit_gpr_t r1, jit_gpr_t r2)
{
    if (jit_thumb_p()) {
	/* thumb auto set carry if not inside IT block */
	if ((r0|r1|r2) < 8)
	    T1_SUB(r0, r1, r2);
	else
	    T2_SUBS(r0, r1, r2);
    }
    else
	_SUBS(r0, r1, r2);
}

#define jit_subci_ui(r0, r1, i0)	arm_subci_ui(_jit, r0, r1, i0)
__jit_inline void
arm_subci_ui(jit_state_t _jit, jit_gpr_t r0, jit_gpr_t r1, int i0)
{
    int		i;
    jit_gpr_t	reg;
    if (jit_thumb_p()) {
	if ((r0|r1) < 8 && !(i0 & ~7))
	    T1_SUBI3(r0, r1, i0);
	else if ((r0|r1) < 8 && !(-i0 & ~7))
	    T1_ADDI3(r0, r1, -i0);
	else if (r0 < 8 && r0 == r1 && !(i0 & ~0xff))
	    T1_SUBI8(r0, i0);
	else if (r0 < 8 && r0 == r1 && !(-i0 & ~0xff))
	    T1_ADDI8(r0, -i0);
	else if ((i = encode_thumb_immediate(i0)) != -1)
	    T2_SUBSI(r0, r1, i);
	else if ((i = encode_thumb_immediate(-i0)) != -1)
	    T2_ADDSI(r0, r1, i);
	else {
	    reg = r0 != r1 ? r0 : JIT_TMP;
	    jit_movi_i(reg, i0);
	    T2_SUBS(r0, r1, reg);
	}
    }
    else {
	if ((i = encode_arm_immediate(i0)) != -1)
	    _SUBSI(r0, r1, i);
	else if ((i = encode_arm_immediate(-i0)) != -1)
	    _ADDSI(r0, r1, i);
	else {
	    reg = r0 != r1 ? r0 : JIT_TMP;
	    jit_movi_i(reg, i0);
	    _SUBS(r0, r1, reg);
	}
    }
}

#define jit_subxr_ui(r0, r1, r2)	arm_subxr_ui(_jit, r0, r1, r2)
__jit_inline void
arm_subxr_ui(jit_state_t _jit, jit_gpr_t r0, jit_gpr_t r1, jit_gpr_t r2)
{
    /* keep setting carry because don't know last ADC */
    if (jit_thumb_p()) {
	/* thumb auto set carry if not inside IT block */
	if ((r0|r1|r2) < 8 && r0 == r1)
	    T1_SBC(r0, r2);
	else
	    T2_SBCS(r0, r1, r2);
    }
    else
	_SBCS(r0, r1, r2);
}

#define jit_subxi_ui(r0, r1, i0)	arm_subxi_ui(_jit, r0, r1, i0)
__jit_inline void
arm_subxi_ui(jit_state_t _jit, jit_gpr_t r0, jit_gpr_t r1, int i0)
{
    int		i;
    jit_gpr_t	reg;
    if (jit_thumb_p()) {
	if ((i = encode_thumb_immediate(i0)) != -1)
	    T2_SBCSI(r0, r1, i);
	else if ((i = encode_thumb_immediate(-i0)) != -1)
	    T2_ADCSI(r0, r1, i);
	else {
	    int		no_set_flags = jit_no_set_flags();
	    reg = r0 != r1 ? r0 : JIT_TMP;
	    jit_no_set_flags() = 1;
	    jit_movi_i(reg, i0);
	    jit_no_set_flags() = no_set_flags;
	    T2_SBCS(r0, r1, reg);
	}
    }
    else {
	if ((i = encode_arm_immediate(i0)) != -1)
	    _SBCSI(r0, r1, i);
	else if ((i = encode_arm_immediate(-i0)) != -1)
	    _ADCSI(r0, r1, i);
	else {
	    reg = r0 != r1 ? r0 : JIT_TMP;
	    jit_movi_i(reg, i0);
	    _SBCS(r0, r1, reg);
	}
    }
}

#define jit_rsbr_i(r0, r1, r2)		arm_rsbr_i(_jit, r0, r1, r2)
__jit_inline void
arm_rsbr_i(jit_state_t _jit, jit_gpr_t r0, jit_gpr_t r1, jit_gpr_t r2)
{
    if (jit_thumb_p())
	T2_RSB(r0, r1, r2);
    else
	_RSB(r0, r1, r2);
}

#define jit_rsbi_i(r0, r1, i0)		arm_rsbi_i(_jit, r0, r1, i0)
__jit_inline void
arm_rsbi_i(jit_state_t _jit, jit_gpr_t r0, jit_gpr_t r1, int i0)
{
    int		i;
    jit_gpr_t	reg;
    if (jit_thumb_p()) {
	if (i0 == 0)
	    arm_negr_i(_jit, r0, r1);
	else if ((i = encode_thumb_immediate(i0)) != -1)
	    T2_RSBI(r0, r1, i);
	else {
	    reg = r0 != r1 ? r0 : JIT_TMP;
	    jit_movi_i(reg, i0);
	    T2_RSB(r0, r1, reg);
	}
    }
    else {
	if ((i = encode_arm_immediate(i0)) != -1)
	    _RSBI(r0, r1, i);
	else {
	    reg = r0 != r1 ? r0 : JIT_TMP;
	    jit_movi_i(reg, i0);
	    _RSB(r0, r1, reg);
	}
    }
}

#define jit_mulr_i(r0, r1, r2)		arm_mulr_i(_jit, r0, r1, r2)
#define jit_mulr_ui(r0, r1, r2)		arm_mulr_i(_jit, r0, r1, r2)
__jit_inline void
arm_mulr_i(jit_state_t _jit, jit_gpr_t r0, jit_gpr_t r1, jit_gpr_t r2)
{
    if (jit_thumb_p()) {
	if (!jit_no_set_flags() && r0 == r2 && (r0|r1) < 8)
	    T1_MUL(r0, r1);
	else if (!jit_no_set_flags() && r0 == r1 && (r0|r2) < 8)
	    T1_MUL(r0, r2);
	else
	    T2_MUL(r0, r1, r2);
    }
    else {
	if (r0 == r1 && !jit_armv6_p()) {
	    if (r0 != r2)
		_MUL(r0, r2, r1);
	    else {
		_MOV(JIT_TMP, r1);
		_MUL(r0, JIT_TMP, r2);
	    }
	}
	else
	    _MUL(r0, r1, r2);
    }
}

#define jit_muli_i(r0, r1, i0)		arm_muli_i(_jit, r0, r1, i0)
#define jit_muli_ui(r0, r1, i0)		arm_muli_i(_jit, r0, r1, i0)
__jit_inline void
arm_muli_i(jit_state_t _jit, jit_gpr_t r0, jit_gpr_t r1, int i0)
{
    jit_gpr_t	reg;
    reg = r0 != r1 ? r0 : JIT_TMP;
    jit_movi_i(reg, i0);
    arm_mulr_i(_jit, r0, r1, reg);
}

#define jit_hmulr_i(r0, r1, r2)		arm_hmulr_i(_jit, r0, r1, r2)
__jit_inline void
arm_hmulr_i(jit_state_t _jit, jit_gpr_t r0, jit_gpr_t r1, jit_gpr_t r2)
{
    if (jit_thumb_p())
	T2_SMULL(JIT_TMP, r0, r1, r2);
    else {
	if (r0 == r1 && !jit_armv6_p()) {
	    assert(r2 != JIT_TMP);
	    _SMULL(JIT_TMP, r0, r2, r1);
	}
	else
	    _SMULL(JIT_TMP, r0, r1, r2);
    }
}

#define jit_hmuli_i(r0, r1, i0)		arm_hmuli_i(_jit, r0, r1, i0)
__jit_inline void
arm_hmuli_i(jit_state_t _jit, jit_gpr_t r0, jit_gpr_t r1, int i0)
{
    jit_gpr_t	reg;
    if (jit_thumb_p()) {
	assert(r0 != JIT_TMP);
	jit_movi_i(JIT_TMP, i0);
	T2_SMULL(JIT_TMP, r0, r1, JIT_TMP);
    }
    else {
	if (r0 != r1 || jit_armv6_p()) {
	    jit_movi_i(JIT_TMP, i0);
	    _SMULL(JIT_TMP, r0, r1, JIT_TMP);
	}
	else {
	    if (r0 != _R0)	    reg = _R0;
	    else if (r0 != _R1)     reg = _R1;
	    else if (r0 != _R2)     reg = _R2;
	    else		    reg = _R3;
	    _PUSH(1<<reg);
	    jit_movi_i(reg, i0);
	    _SMULL(JIT_TMP, r0, r1, reg);
	    _POP(1<<reg);
	}
    }
}

#define jit_hmulr_ui(r0, r1, r2)	arm_hmulr_ui(_jit, r0, r1, r2)
__jit_inline void
arm_hmulr_ui(jit_state_t _jit, jit_gpr_t r0, jit_gpr_t r1, jit_gpr_t r2)
{
    if (jit_thumb_p())
	T2_UMULL(JIT_TMP, r0, r1, r2);
    else {
	if (r0 == r1 && !jit_armv6_p()) {
	    assert(r2 != JIT_TMP);
	    _UMULL(JIT_TMP, r0, r2, r1);
	}
	else
	    _UMULL(JIT_TMP, r0, r1, r2);
    }
}

#define jit_hmuli_ui(r0, r1, i0)	arm_hmuli_ui(_jit, r0, r1, i0)
__jit_inline void
arm_hmuli_ui(jit_state_t _jit, jit_gpr_t r0, jit_gpr_t r1, int i0)
{
    jit_gpr_t	reg;
    if (jit_thumb_p()) {
	assert(r0 != JIT_TMP);
	jit_movi_i(JIT_TMP, i0);
	T2_UMULL(JIT_TMP, r0, r1, JIT_TMP);
    }
    else {
	if (r0 != r1 || jit_armv6_p()) {
	    jit_movi_i(JIT_TMP, i0);
	    _UMULL(JIT_TMP, r0, r1, JIT_TMP);
	}
	else {
	    if (r0 != _R0)	    reg = _R0;
	    else if (r0 != _R1)     reg = _R1;
	    else if (r0 != _R2)     reg = _R2;
	    else		    reg = _R3;
	    _PUSH(1<<reg);
	    jit_movi_i(reg, i0);
	    _UMULL(JIT_TMP, r0, r1, reg);
	    _POP(1<<reg);
	}
    }
}

static void
arm_divmod(jit_state_t _jit, int div, int sign,
	   jit_gpr_t r0, jit_gpr_t r1, jit_gpr_t r2)
{
    int			 d;
    int			 l;
    void		*p;
    l = 0xf;
    if ((int)r0 < 4)
	/* bogus extra push to align at 8 bytes */
	l = (l & ~(1 << r0)) | 0x10;
    if (jit_thumb_p())
	T1_PUSH(l);
    else
	_PUSH(l);
    if (r1 == _R1 && r2 == _R0) {
	jit_movr_i(JIT_FTMP, _R0);
	jit_movr_i(_R0, _R1);
	jit_movr_i(_R1, JIT_FTMP);
    }
    else if (r2 == _R0) {
	jit_movr_i(_R1, r2);
	jit_movr_i(_R0, r1);
    }
    else {
	jit_movr_i(_R0, r1);
	jit_movr_i(_R1, r2);
    }
    if (sign)		p = (void*)__aeabi_idivmod;
    else		p = (void*)__aeabi_uidivmod;
    if (!jit_exchange_p()) {
	if (jit_thumb_p())
	    d = (((uintptr_t)p - (uintptr_t)_jit->x.pc) >> 1) - 2;
	else
	    d = (((uintptr_t)p - (uintptr_t)_jit->x.pc) >> 2) - 2;
	if (_s24P(d)) {
	    if (jit_thumb_p())
		T2_BLI(encode_thumb_jump(d));
	    else
		_BLI(d & 0x00ffffff);
	}
	else
	    goto fallback;
    }
    else {
    fallback:
	jit_movi_i(JIT_FTMP, (uintptr_t)p);
	if (jit_thumb_p())
	    T1_BLX(JIT_FTMP);
	else
	    _BLX(JIT_FTMP);
    }
    if (div)
	jit_movr_i(r0, _R0);
    else
	jit_movr_i(r0, _R1);
    if (jit_thumb_p())
	T1_POP(l);
    else
	_POP(l);
}

#define jit_divr_i(r0, r1, r2)		arm_divr_i(_jit, r0, r1, r2)
__jit_inline void
arm_divr_i(jit_state_t _jit, jit_gpr_t r0, jit_gpr_t r1, jit_gpr_t r2)
{
    if (jit_armv7r_p() && jit_thumb_p())
	T2_SDIV(r0, r1, r2);
    else
	arm_divmod(_jit, 1, 1, r0, r1, r2);
}

#define jit_divi_i(r0, r1, i0)		arm_divi_i(_jit, r0, r1, i0)
__jit_inline void
arm_divi_i(jit_state_t _jit, jit_gpr_t r0, jit_gpr_t r1, int i0)
{
    jit_movi_i(JIT_TMP, i0);
    arm_divr_i(_jit, r0, r1, JIT_TMP);
}

#define jit_divr_ui(r0, r1, r2)		arm_divr_ui(_jit, r0, r1, r2)
__jit_inline void
arm_divr_ui(jit_state_t _jit, jit_gpr_t r0, jit_gpr_t r1, jit_gpr_t r2)
{
    if (jit_armv7r_p() && jit_thumb_p())
	T2_UDIV(r0, r1, r2);
    else
	arm_divmod(_jit, 1, 0, r0, r1, r2);
}

#define jit_divi_ui(r0, r1, i0)		arm_divi_ui(_jit, r0, r1, i0)
__jit_inline void
arm_divi_ui(jit_state_t _jit, jit_gpr_t r0, jit_gpr_t r1, unsigned int i0)
{
    jit_movi_i(JIT_TMP, i0);
    arm_divr_ui(_jit, r0, r1, JIT_TMP);
}

#define jit_modr_i(r0, r1, r2)		arm_modr_i(_jit, r0, r1, r2)
__jit_inline void
arm_modr_i(jit_state_t _jit, jit_gpr_t r0, jit_gpr_t r1, jit_gpr_t r2)
{
    arm_divmod(_jit, 0, 1, r0, r1, r2);
}

#define jit_modi_i(r0, r1, i0)		arm_modi_i(_jit, r0, r1, i0)
__jit_inline void
arm_modi_i(jit_state_t _jit, jit_gpr_t r0, jit_gpr_t r1, int i0)
{
    jit_movi_i(JIT_TMP, i0);
    arm_modr_i(_jit, r0, r1, JIT_TMP);
}

#define jit_modr_ui(r0, r1, r2)		arm_modr_ui(_jit, r0, r1, r2)
__jit_inline void
arm_modr_ui(jit_state_t _jit, jit_gpr_t r0, jit_gpr_t r1, jit_gpr_t r2)
{
    arm_divmod(_jit, 0, 0, r0, r1, r2);
}

#define jit_modi_ui(r0, r1, i0)		arm_modi_ui(_jit, r0, r1, i0)
__jit_inline void
arm_modi_ui(jit_state_t _jit, jit_gpr_t r0, jit_gpr_t r1, int i0)
{
    jit_movi_i(JIT_TMP, i0);
    arm_modr_ui(_jit, r0, r1, JIT_TMP);
}

#define jit_andr_i(r0, r1, r2)		arm_andr_i(_jit, r0, r1, r2)
__jit_inline void
arm_andr_i(jit_state_t _jit, jit_gpr_t r0, jit_gpr_t r1, jit_gpr_t r2)
{
    if (jit_thumb_p()) {
	if (!jit_no_set_flags() && (r0|r1|r2) < 8 && (r0 == r1 || r0 == r2))
	    T1_AND(r0, r0 == r1 ? r2 : r1);
	else
	    T2_AND(r0, r1, r2);
    }
    else
	_AND(r0, r1, r2);
}

#define jit_andi_i(r0, r1, i0)		arm_andi_i(_jit, r0, r1, i0)
__jit_inline void
arm_andi_i(jit_state_t _jit, jit_gpr_t r0, jit_gpr_t r1, int i0)
{
    int		i;
    jit_gpr_t	reg;
    if (jit_thumb_p()) {
	if ((i = encode_thumb_immediate(i0)) != -1)
	    T2_ANDI(r0, r1, i);
	else if ((i = encode_thumb_immediate(~i0)) != -1)
	    T2_BICI(r0, r1, i);
	else {
	    reg = r0 != r1 ? r0 : JIT_TMP;
	    jit_movi_i(reg, i0);
	    T2_AND(r0, r1, reg);
	}
    }
    else {
	if ((i = encode_arm_immediate(i0)) != -1)
	    _ANDI(r0, r1, i);
	else if ((i = encode_arm_immediate(~i0)) != -1)
	    _BICI(r0, r1, i);
	else {
	    reg = r0 != r1 ? r0 : JIT_TMP;
	    jit_movi_i(reg, i0);
	    _AND(r0, r1, reg);
	}
    }
}

#define jit_orr_i(r0, r1, r2)		arm_orr_i(_jit, r0, r1, r2)
__jit_inline void
arm_orr_i(jit_state_t _jit, jit_gpr_t r0, jit_gpr_t r1, jit_gpr_t r2)
{
    if (jit_thumb_p()) {
	if (!jit_no_set_flags() && (r0|r1|r2) < 8 && (r0 == r1 || r0 == r2))
	    T1_ORR(r0, r0 == r1 ? r2 : r1);
	else
	    T2_ORR(r0, r1, r2);
    }
    else
	_ORR(r0, r1, r2);
}

#define jit_ori_i(r0, r1, i0)		arm_ori_i(_jit, r0, r1, i0)
__jit_inline void
arm_ori_i(jit_state_t _jit, jit_gpr_t r0, jit_gpr_t r1, int i0)
{
    int		i;
    jit_gpr_t	reg;
    if (jit_thumb_p()) {
	if ((i = encode_thumb_immediate(i0)) != -1)
	    T2_ORRI(r0, r1, i);
	else {
	    reg = r0 != r1 ? r0 : JIT_TMP;
	    jit_movi_i(reg, i0);
	    T2_ORR(r0, r1, reg);
	}
    }
    else {
	if ((i = encode_arm_immediate(i0)) != -1)
	    _ORRI(r0, r1, i);
	else {
	    reg = r0 != r1 ? r0 : JIT_TMP;
	    jit_movi_i(reg, i0);
	    _ORR(r0, r1, reg);
	}
    }
}

#define jit_xorr_i(r0, r1, r2)		arm_xorr_i(_jit, r0, r1, r2)
__jit_inline void
arm_xorr_i(jit_state_t _jit, jit_gpr_t r0, jit_gpr_t r1, jit_gpr_t r2)
{
    if (jit_thumb_p()) {
	if (!jit_no_set_flags() && (r0|r1|r2) < 8 && (r0 == r1 || r0 == r2))
	    T1_EOR(r0, r0 == r1 ? r2 : r1);
	else
	    T2_EOR(r0, r1, r2);
    }
    else
	_EOR(r0, r1, r2);
}

#define jit_xori_i(r0, r1, i0)		arm_xori_i(_jit, r0, r1, i0)
__jit_inline void
arm_xori_i(jit_state_t _jit, jit_gpr_t r0, jit_gpr_t r1, int i0)
{
    int		i;
    jit_gpr_t	reg;
    if (jit_thumb_p()) {
	if ((i = encode_thumb_immediate(i0)) != -1)
	    T2_EORI(r0, r1, i);
	else {
	    reg = r0 != r1 ? r0 : JIT_TMP;
	    jit_movi_i(reg, i0);
	    T2_EOR(r0, r1, reg);
	}
    }
    else {
	if ((i = encode_arm_immediate(i0)) != -1)
	    _EORI(r0, r1, i);
	else {
	    reg = r0 != r1 ? r0 : JIT_TMP;
	    jit_movi_i(reg, i0);
	    _EOR(r0, r1, reg);
	}
    }
}

#define jit_lshr_i(r0, r1, r2)		arm_lshr_i(_jit, r0, r1, r2)
__jit_inline void
arm_lshr_i(jit_state_t _jit, jit_gpr_t r0, jit_gpr_t r1, jit_gpr_t r2)
{
    if (jit_thumb_p()) {
	if (!jit_no_set_flags() && (r0|r1|r2) < 8 && r0 == r1)
	    T1_LSL(r0, r2);
	else
	    T2_LSL(r0, r1, r2);
    }
    else
	_LSL(r0, r1, r2);
}

#define jit_lshi_i(r0, r1, i0)		arm_lshi_i(_jit, r0, r1, i0)
__jit_inline void
arm_lshi_i(jit_state_t _jit, jit_gpr_t r0, jit_gpr_t r1, int i0)
{
    assert(i0 >= 0 && i0 <= 31);
    if (i0 == 0)
	jit_movr_i(r0, r1);
    else if (jit_thumb_p()) {
	if (!jit_no_set_flags() && (r0|r1) < 8)
	    T1_LSLI(r0, r1, i0);
	else
	    T2_LSLI(r0, r1, i0);
    }
    else
	_LSLI(r0, r1, i0);
}

#define jit_rshr_i(r0, r1, r2)		arm_rshr_i(_jit, r0, r1, r2)
__jit_inline void
arm_rshr_i(jit_state_t _jit, jit_gpr_t r0, jit_gpr_t r1, jit_gpr_t r2)
{
    if (jit_thumb_p()) {
	if (!jit_no_set_flags() && (r0|r1|r2) < 8 && r0 == r1)
	    T1_ASR(r0, r2);
	else
	    T2_ASR(r0, r1, r2);
    }
    else
	_ASR(r0, r1, r2);
}

#define jit_rshi_i(r0, r1, i0)		arm_rshi_i(_jit, r0, r1, i0)
__jit_inline void
arm_rshi_i(jit_state_t _jit, jit_gpr_t r0, jit_gpr_t r1, int i0)
{
    assert(i0 >= 0 && i0 <= 31);
    if (i0 == 0)
	jit_movr_i(r0, r1);
    else if (jit_thumb_p()) {
	if (!jit_no_set_flags() && (r0|r1) < 8)
	    T1_ASRI(r0, r1, i0);
	else
	    T2_ASRI(r0, r1, i0);
    }
    else
	_ASRI(r0, r1, i0);
}

#define jit_rshr_ui(r0, r1, r2)		arm_rshr_ui(_jit, r0, r1, r2)
__jit_inline void
arm_rshr_ui(jit_state_t _jit, jit_gpr_t r0, jit_gpr_t r1, jit_gpr_t r2)
{
    if (jit_thumb_p()) {
	if (!jit_no_set_flags() && (r0|r1|r2) < 8 && r0 == r1)
	    T1_LSR(r0, r2);
	else
	    T2_LSR(r0, r1, r2);
    }
    else
	_LSR(r0, r1, r2);
}

#define jit_rshi_ui(r0, r1, i0)		arm_rshi_ui(_jit, r0, r1, i0)
__jit_inline void
arm_rshi_ui(jit_state_t _jit, jit_gpr_t r0, jit_gpr_t r1, int i0)
{
    assert(i0 >= 0 && i0 <= 31);
    if (i0 == 0)
	jit_movr_i(r0, r1);
    else if (jit_thumb_p()) {
	if (!jit_no_set_flags() && (r0|r1) < 8)
	    T1_LSRI(r0, r1, i0);
	else
	    T2_LSRI(r0, r1, i0);
    }
    else
	_LSRI(r0, r1, i0);
}

__jit_inline void
arm_ccr(jit_state_t _jit, int ct, int cf,
	jit_gpr_t r0, jit_gpr_t r1, jit_gpr_t r2)
{
    if (jit_thumb_p()) {
	assert((ct ^ cf) >> 28 == 1);
	if ((r1|r2) < 8)
	    T1_CMP(r1, r2);
	else if ((r1&r2) & 8)
	    T1_CMPX(r1, r2);
	else
	    T2_CMP(r1, r2);
	_ITE(ct);
	if (r0 < 8) {
	    T1_MOVI(r0, 1);
	    T1_MOVI(r0, 0);
	}
	else {
	    T2_MOVI(r0, 1);
	    T2_MOVI(r0, 0);
	}
    }
    else {
	_CMP(r1, r2);
	_CC_MOVI(ct, r0, 1);
	_CC_MOVI(cf, r0, 0);
    }
}
__jit_inline void
arm_cci(jit_state_t _jit, int ct, int cf,
	jit_gpr_t r0, jit_gpr_t r1, int i0)
{
    int		i;
    jit_gpr_t	reg;
    if (jit_thumb_p()) {
	if (r1 < 7 && !(i0 & 0xffffff00))
	    T1_CMPI(r1, i0);
	else if ((i = encode_thumb_immediate(i0)) != -1)
	    T2_CMPI(r1, i);
	else if ((i = encode_thumb_immediate(-i0)) != -1)
	    T2_CMNI(r1, i);
	else {
	    reg = r0 != r1 ? r0 : JIT_TMP;
	    jit_movi_i(reg, i0);
	    arm_ccr(_jit, ct, cf, r0, r1, reg);
	    return;
	}
	_ITE(ct);
	if (r0 < 8) {
	    T1_MOVI(r0, 1);
	    T1_MOVI(r0, 0);
	}
	else {
	    T2_MOVI(r0, 1);
	    T2_MOVI(r0, 0);
	}
    }
    else {
	if ((i = encode_arm_immediate(i0)) != -1)
	    _CMPI(r1, i);
	else if ((i = encode_arm_immediate(-i0)) != -1)
	    _CMNI(r1, i);
	else {
	    reg = r0 != r1 ? r0 : JIT_TMP;
	    jit_movi_i(reg, i0);
	    _CMP(r1, reg);
	}
	_CC_MOVI(ct, r0, 1);
	_CC_MOVI(cf, r0, 0);
    }
}
#define jit_ltr_i(r0, r1, r2)	arm_ccr(_jit,ARM_CC_LT,ARM_CC_GE,r0,r1,r2)
#define jit_lti_i(r0, r1, i0)	arm_cci(_jit,ARM_CC_LT,ARM_CC_GE,r0,r1,i0)
#define jit_ltr_ui(r0, r1, r2)	arm_ccr(_jit,ARM_CC_LO,ARM_CC_HS,r0,r1,r2)
#define jit_lti_ui(r0, r1, i0)	arm_cci(_jit,ARM_CC_LO,ARM_CC_HS,r0,r1,i0)
#define jit_ler_i(r0, r1, r2)	arm_ccr(_jit,ARM_CC_LE,ARM_CC_GT,r0,r1,r2)
#define jit_lei_i(r0, r1, i0)	arm_cci(_jit,ARM_CC_LE,ARM_CC_GT,r0,r1,i0)
#define jit_ler_ui(r0, r1, r2)	arm_ccr(_jit,ARM_CC_LS,ARM_CC_HI,r0,r1,r2)
#define jit_lei_ui(r0, r1, i0)	arm_cci(_jit,ARM_CC_LS,ARM_CC_HI,r0,r1,i0)
#define jit_eqr_i(r0, r1, r2)	arm_ccr(_jit,ARM_CC_EQ,ARM_CC_NE,r0,r1,r2)
#define jit_eqi_i(r0, r1, i0)	arm_cci(_jit,ARM_CC_EQ,ARM_CC_NE,r0,r1,i0)
#define jit_ger_i(r0, r1, r2)	arm_ccr(_jit,ARM_CC_GE,ARM_CC_LT,r0,r1,r2)
#define jit_gei_i(r0, r1, i0)	arm_cci(_jit,ARM_CC_GE,ARM_CC_LT,r0,r1,i0)
#define jit_ger_ui(r0, r1, r2)	arm_ccr(_jit,ARM_CC_HS,ARM_CC_LO,r0,r1,r2)
#define jit_gei_ui(r0, r1, i0)	arm_cci(_jit,ARM_CC_HS,ARM_CC_LO,r0,r1,i0)
#define jit_gtr_i(r0, r1, r2)	arm_ccr(_jit,ARM_CC_GT,ARM_CC_LE,r0,r1,r2)
#define jit_gti_i(r0, r1, i0)	arm_cci(_jit,ARM_CC_GT,ARM_CC_LE,r0,r1,i0)
#define jit_gtr_ui(r0, r1, r2)	arm_ccr(_jit,ARM_CC_HI,ARM_CC_LS,r0,r1,r2)
#define jit_gti_ui(r0, r1, i0)	arm_cci(_jit,ARM_CC_HI,ARM_CC_LS,r0,r1,i0)

#define jit_ner_i(r0, r1, r2)		arm_ner_i(_jit, r0, r1, r2)
__jit_inline void
arm_ner_i(jit_state_t _jit, jit_gpr_t r0, jit_gpr_t r1, jit_gpr_t r2)
{
    if (jit_thumb_p())
	arm_ccr(_jit, ARM_CC_NE, ARM_CC_EQ, r0, r1, r2);
    else {
	_SUBS(r0, r1, r2);
	_CC_MOVI(ARM_CC_NE, r0, 1);
    }
}

#define jit_nei_i(r0, r1, i0)		arm_nei_i(_jit, r0, r1, i0)
__jit_inline void
arm_nei_i(jit_state_t _jit, jit_gpr_t r0, jit_gpr_t r1, int i0)
{
    int		i;
    jit_gpr_t	reg;
    if (jit_thumb_p())
	arm_cci(_jit, ARM_CC_NE, ARM_CC_EQ, r0, r1, i0);
    else {
	if ((i = encode_arm_immediate(i0)) != -1)
	    _SUBSI(r0, r1, i);
	else if ((i = encode_arm_immediate(-i0)) != -1)
	    _ADDSI(r0, r1, i);
	else {
	    reg = r0 != r1 ? r0 : JIT_TMP;
	    jit_movi_i(reg, i0);
	    _SUBS(r0, r1, reg);
	}
	_CC_MOVI(ARM_CC_NE, r0, 1);
    }
}

#define jit_jmpr(r0)			arm_jmpr(_jit, r0)
__jit_inline void
arm_jmpr(jit_state_t _jit, jit_gpr_t r0)
{
    if (jit_thumb_p())
	T1_MOV(_R15, r0);
    else
	_MOV(_R15, r0);
}

#define jit_jmpi(i0)			arm_jmpi(_jit, i0)
__jit_inline jit_insn *
arm_jmpi(jit_state_t _jit, void *i0)
{
    jit_insn	*l;
    long	 d;
    l = _jit->x.pc;
    if (jit_thumb_p() && _jitl.after_prolog) {
	d = (((long)i0 - (long)l) >> 1) - 2;
	if (_s20P(d))
	    T2_B(encode_thumb_jump(d));
	else {
	    jit_movi_p(JIT_TMP, i0);
	    jit_jmpr(JIT_TMP);
	}
    }
    else {
	d = (((long)i0 - (long)l) >> 2) - 2;
	assert(_s24P(d));
	_CC_B(ARM_CC_AL, d & 0x00ffffff);
    }
    return (l);
}

__jit_inline jit_insn *
arm_bccr(jit_state_t _jit, int cc, jit_insn *i0, jit_gpr_t r0, jit_gpr_t r1)
{
    jit_insn	*l;
    long	 d;
    if (jit_thumb_p()) {
	if ((r0|r1) < 8)
	    T1_CMP(r0, r1);
	else if ((r0&r1) & 8)
	    T1_CMPX(r0, r1);
	else
	    T2_CMP(r0, r1);
	/* use only thumb2 conditional as does not know if will be patched */
	l = _jit->x.pc;
	d = (((long)i0 - (long)l) >> 1) - 2;
	assert(_s20P(d));
	T2_CC_B(cc, encode_thumb_cc_jump(d));
    }
    else {
	_CMP(r0, r1);
	l = _jit->x.pc;
	d = (((long)i0 - (long)l) >> 2) - 2;
	assert(_s24P(d));
	_CC_B(cc, d & 0x00ffffff);
    }
    return (l);
}
__jit_inline jit_insn *
arm_bcci(jit_state_t _jit, int cc, jit_insn *i0, jit_gpr_t r0, int i1)
{
    jit_insn	*l;
    long	 d;
    int		 i;
    if (jit_thumb_p()) {
	if (r0 < 7 && !(i1 & 0xffffff00))
	    T1_CMPI(r0, i1);
	else if ((i = encode_thumb_immediate(i1)) != -1)
	    T2_CMPI(r0, i);
	else if ((i = encode_thumb_immediate(-i1)) != -1)
	    T2_CMNI(r0, i);
	else {
	    jit_movi_i(JIT_TMP, i1);
	    T2_CMP(r0, JIT_TMP);
	}
	/* use only thumb2 conditional as does not know if will be patched */
	l = _jit->x.pc;
	d = (((long)i0 - (long)l) >> 1) - 2;
	assert(_s20P(d));
	T2_CC_B(cc, encode_thumb_cc_jump(d));
    }
    else {
	if ((i = encode_arm_immediate(i1)) != -1)
	    _CMPI(r0, i);
	else if ((i = encode_arm_immediate(-i1)) != -1)
	    _CMNI(r0, i);
	else {
	    jit_movi_i(JIT_TMP, i1);
	    _CMP(r0, JIT_TMP);
	}
	l = _jit->x.pc;
	d = (((long)i0 - (long)l) >> 2) - 2;
	assert(_s24P(d));
	_CC_B(cc, d & 0x00ffffff);
    }
    return (l);
}
#define jit_bltr_i(i0, r0, r1)		arm_bccr(_jit, ARM_CC_LT, i0, r0, r1)
#define jit_blti_i(i0, r0, i1)		arm_bcci(_jit, ARM_CC_LT, i0, r0, i1)
#define jit_bltr_ui(i0, r0, r1) 	arm_bccr(_jit, ARM_CC_LO, i0, r0, r1)
#define jit_blti_ui(i0, r0, i1) 	arm_bcci(_jit, ARM_CC_LO, i0, r0, i1)
#define jit_bler_i(i0, r0, r1)		arm_bccr(_jit, ARM_CC_LE, i0, r0, r1)
#define jit_blei_i(i0, r0, i1)		arm_bcci(_jit, ARM_CC_LE, i0, r0, i1)
#define jit_bler_ui(i0, r0, r1) 	arm_bccr(_jit, ARM_CC_LS, i0, r0, r1)
#define jit_blei_ui(i0, r0, i1) 	arm_bcci(_jit, ARM_CC_LS, i0, r0, i1)
#define jit_beqr_i(i0, r0, r1)		arm_bccr(_jit, ARM_CC_EQ, i0, r0, r1)
#define jit_beqi_i(i0, r0, i1)		arm_bcci(_jit, ARM_CC_EQ, i0, r0, i1)
#define jit_bger_i(i0, r0, r1)		arm_bccr(_jit, ARM_CC_GE, i0, r0, r1)
#define jit_bgei_i(i0, r0, i1)		arm_bcci(_jit, ARM_CC_GE, i0, r0, i1)
#define jit_bger_ui(i0, r0, r1) 	arm_bccr(_jit, ARM_CC_HS, i0, r0, r1)
#define jit_bgei_ui(i0, r0, i1) 	arm_bcci(_jit, ARM_CC_HS, i0, r0, i1)
#define jit_bgtr_i(i0, r0, r1)		arm_bccr(_jit, ARM_CC_GT, i0, r0, r1)
#define jit_bgti_i(i0, r0, i1)		arm_bcci(_jit, ARM_CC_GT, i0, r0, i1)
#define jit_bgtr_ui(i0, r0, r1) 	arm_bccr(_jit, ARM_CC_HI, i0, r0, r1)
#define jit_bgti_ui(i0, r0, i1) 	arm_bcci(_jit, ARM_CC_HI, i0, r0, i1)
#define jit_bner_i(i0, r0, r1)		arm_bccr(_jit, ARM_CC_NE, i0, r0, r1)
#define jit_bnei_i(i0, r0, i1)		arm_bcci(_jit, ARM_CC_NE, i0, r0, i1)

__jit_inline jit_insn *
arm_baddr(jit_state_t _jit, int cc, jit_insn *i0, jit_gpr_t r0, jit_gpr_t r1)
{
    jit_insn	*l;
    long	 d;
    if (jit_thumb_p()) {
	if ((r0|r1) < 8)
	    T1_ADD(r0, r0, r1);
	else
	    T2_ADDS(r0, r0, r1);
	l = _jit->x.pc;
	d = (((long)i0 - (long)l) >> 1) - 2;
	assert(_s20P(d));
	T2_CC_B(cc, encode_thumb_cc_jump(d));
    }
    else {
	_ADDS(r0, r0, r1);
	l = _jit->x.pc;
	d = (((long)i0 - (long)l) >> 2) - 2;
	assert(_s24P(d));
	_CC_B(cc, d & 0x00ffffff);
    }
    return (l);
}
__jit_inline jit_insn *
arm_baddi(jit_state_t _jit, int cc, jit_insn *i0, jit_gpr_t r0, int i1)
{
    jit_insn	*l;
    long	 d;
    int		 i;
    if (jit_thumb_p()) {
	if (r0 < 8 && !(i1 & ~7))
	    T1_ADDI3(r0, r0, i1);
	else if (r0 < 8 && !(-i1 & ~7))
	    T1_SUBI3(r0, r0, -i1);
	else if (r0 < 8 && !(i1 & ~0xff))
	    T1_ADDI8(r0, i1);
	else if (r0 < 8 && !(-i1 & ~0xff))
	    T1_SUBI8(r0, -i1);
	else if ((i = encode_thumb_immediate(i1)) != -1)
	    T2_ADDSI(r0, r0, i);
	else if ((i = encode_thumb_immediate(-i1)) != -1)
	    T2_SUBSI(r0, r0, i);
	else {
	    jit_movi_i(JIT_TMP, i1);
	    T2_ADDS(r0, r0, JIT_TMP);
	}
	l = _jit->x.pc;
	d = (((long)i0 - (long)l) >> 1) - 2;
	assert(_s20P(d));
	T2_CC_B(cc, encode_thumb_cc_jump(d));
    }
    else {
	if ((i = encode_arm_immediate(i1)) != -1)
	    _ADDSI(r0, r0, i);
	else if ((i = encode_arm_immediate(-i1)) != -1)
	    _SUBSI(r0, r0, i);
	else {
	    jit_movi_i(JIT_TMP, i1);
	    _ADDS(r0, r0, JIT_TMP);
	}
	l = _jit->x.pc;
	d = (((long)i0 - (long)l) >> 2) - 2;
	assert(_s24P(d));
	_CC_B(cc, d & 0x00ffffff);
    }
    return (l);
}
#define jit_boaddr_i(i0, r0, r1)	arm_baddr(_jit, ARM_CC_VS, i0, r0, r1)
#define jit_boaddi_i(i0, r0, i1)	arm_baddi(_jit, ARM_CC_VS, i0, r0, i1)
#define jit_boaddr_ui(i0, r0, r1)	arm_baddr(_jit, ARM_CC_HS, i0, r0, r1)
#define jit_boaddi_ui(i0, r0, i1)	arm_baddi(_jit, ARM_CC_HS, i0, r0, i1)

__jit_inline jit_insn *
arm_bsubr(jit_state_t _jit, int cc, jit_insn *i0, jit_gpr_t r0, jit_gpr_t r1)
{
    jit_insn	*l;
    long	 d;
    if (jit_thumb_p()) {
	if ((r0|r1) < 8)
	    T1_SUB(r0, r0, r1);
	else
	    T2_SUBS(r0, r0, r1);
	l = _jit->x.pc;
	d = (((long)i0 - (long)l) >> 1) - 2;
	assert(_s20P(d));
	T2_CC_B(cc, encode_thumb_cc_jump(d));
    }
    else {
	_SUBS(r0, r0, r1);
	l = _jit->x.pc;
	d = (((long)i0 - (long)l) >> 2) - 2;
	assert(_s24P(d));
	_CC_B(cc, d & 0x00ffffff);
    }
    return (l);
}

__jit_inline jit_insn *
arm_bsubi(jit_state_t _jit, int cc, jit_insn *i0, jit_gpr_t r0, int i1)
{
    jit_insn	*l;
    long	 d;
    int		 i;
    if (jit_thumb_p()) {
	if (r0 < 8 && !(i1 & ~7))
	    T1_SUBI3(r0, r0, i1);
	else if (r0 < 8 && !(-i1 & ~7))
	    T1_ADDI3(r0, r0, -i1);
	else if (r0 < 8 && !(i1 & ~0xff))
	    T1_SUBI8(r0, i1);
	else if (r0 < 8 && !(-i1 & ~0xff))
	    T1_ADDI8(r0, -i1);
	else if ((i = encode_thumb_immediate(i1)) != -1)
	    T2_SUBSI(r0, r0, i);
	else if ((i = encode_thumb_immediate(-i1)) != -1)
	    T2_ADDSI(r0, r0, i);
	else {
	    jit_movi_i(JIT_TMP, i1);
	    T2_SUBS(r0, r0, JIT_TMP);
	}
	l = _jit->x.pc;
	d = (((long)i0 - (long)l) >> 1) - 2;
	assert(_s20P(d));
	T2_CC_B(cc, encode_thumb_cc_jump(d));
    }
    else {
	if ((i = encode_arm_immediate(i1)) != -1)
	    _SUBSI(r0, r0, i);
	else if ((i = encode_arm_immediate(-i1)) != -1)
	    _ADDSI(r0, r0, i);
	else {
	    jit_movi_i(JIT_TMP, i1);
	    _SUBS(r0, r0, JIT_TMP);
	}
	l = _jit->x.pc;
	d = (((long)i0 - (long)l) >> 2) - 2;
	assert(_s24P(d));
	_CC_B(cc, d & 0x00ffffff);
    }
    return (l);
}
#define jit_bosubr_i(i0, r0, r1)	arm_bsubr(_jit, ARM_CC_VS, i0, r0, r1)
#define jit_bosubi_i(i0, r0, i1)	arm_bsubi(_jit, ARM_CC_VS, i0, r0, i1)
#define jit_bosubr_ui(i0, r0, r1)	arm_bsubr(_jit, ARM_CC_LO, i0, r0, r1)
#define jit_bosubi_ui(i0, r0, i1)	arm_bsubi(_jit, ARM_CC_LO, i0, r0, i1)

__jit_inline jit_insn *
arm_bmxr(jit_state_t _jit, int cc, jit_insn *i0, jit_gpr_t r0, jit_gpr_t r1)
{
    jit_insn	*l;
    long	 d;
    if (jit_thumb_p()) {
	if ((r0|r1) < 8)
	    T1_TST(r0, r1);
	else
	    T2_TST(r0, r1);
	l = _jit->x.pc;
	d = (((long)i0 - (long)l) >> 1) - 2;
	assert(_s20P(d));
	T2_CC_B(cc, encode_thumb_cc_jump(d));
    }
    else {
	if (jit_armv5_p())
	    _TST(r0, r1);
	else
	    _ANDS(JIT_TMP, r0, r1);
	l = _jit->x.pc;
	d = (((long)i0 - (long)l) >> 2) - 2;
	assert(_s24P(d));
	_CC_B(cc, d & 0x00ffffff);
    }
    return (l);
}
__jit_inline jit_insn *
arm_bmxi(jit_state_t _jit, int cc, jit_insn *i0, jit_gpr_t r0, int i1)
{
    jit_insn	*l;
    long	 d;
    int		 i;
    if (jit_thumb_p()) {
	if ((i = encode_thumb_immediate(i1)) != -1)
	    T2_TSTI(r0, i);
	else {
	    jit_movi_i(JIT_TMP, i1);
	    T2_TST(r0, JIT_TMP);
	}
	l = _jit->x.pc;
	d = (((long)i0 - (long)l) >> 1) - 2;
	assert(_s20P(d));
	T2_CC_B(cc, encode_thumb_cc_jump(d));
    }
    else {
	if (jit_armv5_p()) {
	    if ((i = encode_arm_immediate(i1)) != -1)
		_TSTI(r0, i);
	    else {
		jit_movi_i(JIT_TMP, i1);
		_TST(r0, JIT_TMP);
	    }
	}
	else {
	    if ((i = encode_arm_immediate(i1)) != -1)
		_ANDSI(JIT_TMP, r0, i);
	    else if ((i = encode_arm_immediate(~i1)) != -1)
		_BICSI(JIT_TMP, r0, i);
	    else {
		jit_movi_i(JIT_TMP, i1);
		_ANDS(JIT_TMP, r0, JIT_TMP);
	    }
	}
	l = _jit->x.pc;
	d = (((long)i0 - (long)l) >> 2) - 2;
	assert(_s24P(d));
	_CC_B(cc, d & 0x00ffffff);
    }
    return (l);
}
#define jit_bmsr_i(i0, r0, r1)		arm_bmxr(_jit, ARM_CC_NE, i0, r0, r1)
#define jit_bmsi_i(i0, r0, i1)		arm_bmxi(_jit, ARM_CC_NE, i0, r0, i1)
#define jit_bmcr_i(i0, r0, r1)		arm_bmxr(_jit, ARM_CC_EQ, i0, r0, r1)
#define jit_bmci_i(i0, r0, i1)		arm_bmxi(_jit, ARM_CC_EQ, i0, r0, i1)

#define jit_ldr_c(r0, r1)		arm_ldr_c(_jit, r0, r1)
__jit_inline void
arm_ldr_c(jit_state_t _jit, jit_gpr_t r0, jit_gpr_t r1)
{
    if (jit_thumb_p())
	T2_LDRSBI(r0, r1, 0);
    else
	_LDRSBI(r0, r1, 0);
}

#define jit_ldi_c(r0, i0)		arm_ldi_c(_jit, r0, i0)
__jit_inline void
arm_ldi_c(jit_state_t _jit, jit_gpr_t r0, void *i0)
{
    jit_movi_i(JIT_TMP, (uintptr_t)i0);
    if (jit_thumb_p())
	T2_LDRSBI(r0, JIT_TMP, 0);
    else
	_LDRSBI(r0, JIT_TMP, 0);
}

#define jit_ldxr_c(r0, r1, r2)		arm_ldxr_c(_jit, r0, r1, r2)
__jit_inline void
arm_ldxr_c(jit_state_t _jit, jit_gpr_t r0, jit_gpr_t r1, jit_gpr_t r2)
{
    if (jit_thumb_p()) {
	if ((r0|r1|r2) < 8)
	    T1_LDRSB(r0, r1, r2);
	else
	    T2_LDRSB(r0, r1, r2);
    }
    else
	_LDRSB(r0, r1, r2);
}

#define jit_ldxi_c(r0, r1, i0)		arm_ldxi_c(_jit, r0, r1, i0)
__jit_inline void
arm_ldxi_c(jit_state_t _jit, jit_gpr_t r0, jit_gpr_t r1, int i0)
{
    jit_gpr_t		reg;
    if (jit_thumb_p()) {
	if (i0 >= 0 && i0 <= 255)
	    T2_LDRSBI(r0, r1, i0);
	else if (i0 < 0 && i0 >= -255)
	    T2_LDRSBIN(r0, r1, -i0);
	else if (i0 >= 0 && i0 <= 4095)
	    T2_LDRSBWI(r0, r1, i0);
	else {
	    reg = r0 != r1 ? r0 : JIT_TMP;
	    jit_movi_i(reg, i0);
	    if ((r0|r1|reg) < 8)
		T1_LDRSB(r0, r1, reg);
	    else
		T2_LDRSB(r0, r1, reg);
	}
    }
    else {
	if (i0 >= 0 && i0 <= 255)
	    _LDRSBI(r0, r1, i0);
	else if (i0 < 0 && i0 >= -255)
	    _LDRSBIN(r0, r1, -i0);
	else {
	    reg = r0 != r1 ? r0 : JIT_TMP;
	    jit_movi_i(reg, i0);
	    _LDRSB(r0, r1, reg);
	}
    }
}

#define jit_ldr_uc(r0, r1)		arm_ldr_uc(_jit, r0, r1)
__jit_inline void
arm_ldr_uc(jit_state_t _jit, jit_gpr_t r0, jit_gpr_t r1)
{
    if (jit_thumb_p())
	T2_LDRBI(r0, r1, 0);
    else
	_LDRBI(r0, r1, 0);
}

#define jit_ldi_uc(r0, i0)		arm_ldi_uc(_jit, r0, i0)
__jit_inline void
arm_ldi_uc(jit_state_t _jit, jit_gpr_t r0, void *i0)
{
    jit_movi_i(JIT_TMP, (uintptr_t)i0);
    if (jit_thumb_p())
	T2_LDRBI(r0, JIT_TMP, 0);
    else
	_LDRBI(r0, JIT_TMP, 0);
}

#define jit_ldxr_uc(r0, r1, r2)		arm_ldxr_uc(_jit, r0, r1, r2)
__jit_inline void
arm_ldxr_uc(jit_state_t _jit, jit_gpr_t r0, jit_gpr_t r1, jit_gpr_t r2)
{
    if (jit_thumb_p()) {
	if ((r0|r1|r2) < 8)
	    T1_LDRB(r0, r1, r2);
	else
	    T2_LDRB(r0, r1, r2);
    }
    else
	_LDRB(r0, r1, r2);
}

#define jit_ldxi_uc(r0, r1, i0)		arm_ldxi_uc(_jit, r0, r1, i0)
__jit_inline void
arm_ldxi_uc(jit_state_t _jit, jit_gpr_t r0, jit_gpr_t r1, int i0)
{
    jit_gpr_t		reg;
    if (jit_thumb_p()) {
	if ((r0|r1) < 8 && i0 >= 0 && i0 < 0x20)
	    T1_LDRBI(r0, r1, i0);
	else if (i0 >= 0 && i0 <= 255)
	    T2_LDRBI(r0, r1, i0);
	else if (i0 < 0 && i0 >= -255)
	    T2_LDRBIN(r0, r1, -i0);
	else if (i0 >= 0 && i0 <= 4095)
	    T2_LDRBWI(r0, r1, i0);
	else {
	    reg = r0 != r1 ? r0 : JIT_TMP;
	    jit_movi_i(reg, i0);
	    if ((r0|r1|reg) < 8)
		T1_LDRB(r0, r1, reg);
	    else
		T2_LDRB(r0, r1, reg);
	}
    }
    else {
	if (i0 >= 0 && i0 <= 4095)
	    _LDRBI(r0, r1, i0);
	else if (i0 < 0 && i0 >= -4095)
	    _LDRBIN(r0, r1, -i0);
	else {
	    reg = r0 != r1 ? r0 : JIT_TMP;
	    jit_movi_i(reg, i0);
	    _LDRB(r0, r1, reg);
	}
    }
}

#define jit_ldr_s(r0, r1)		arm_ldr_s(_jit, r0, r1)
__jit_inline void
arm_ldr_s(jit_state_t _jit, jit_gpr_t r0, jit_gpr_t r1)
{
    if (jit_thumb_p())
	T2_LDRSHI(r0, r1, 0);
    else
	_LDRSHI(r0, r1, 0);
}

#define jit_ldi_s(r0, i0)		arm_ldi_s(_jit, r0, i0)
__jit_inline void
arm_ldi_s(jit_state_t _jit, jit_gpr_t r0, void *i0)
{
    jit_movi_i(JIT_TMP, (uintptr_t)i0);
    if (jit_thumb_p())
	T2_LDRSHI(r0, JIT_TMP, 0);
    else
	_LDRSHI(r0, JIT_TMP, 0);
}

#define jit_ldxr_s(r0, r1, r2)		arm_ldxr_s(_jit, r0, r1, r2)
__jit_inline void
arm_ldxr_s(jit_state_t _jit, jit_gpr_t r0, jit_gpr_t r1, jit_gpr_t r2)
{
    if (jit_thumb_p()) {
	if ((r0|r1|r2) < 8)
	    T1_LDRSH(r0, r1, r2);
	else
	    T2_LDRSH(r0, r1, r2);
    }
    else
	_LDRSH(r0, r1, r2);
}

#define jit_ldxi_s(r0, r1, i0)		arm_ldxi_s(_jit, r0, r1, i0)
__jit_inline void
arm_ldxi_s(jit_state_t _jit, jit_gpr_t r0, jit_gpr_t r1, int i0)
{
    jit_gpr_t		reg;
    if (jit_thumb_p()) {
	if (i0 >= 0 && i0 <= 255)
	    T2_LDRSHI(r0, r1, i0);
	else if (i0 < 0 && i0 >= -255)
	    T2_LDRSHIN(r0, r1, -i0);
	else if (i0 >= 0 && i0 <= 4095)
	    T2_LDRSHWI(r0, r1, i0);
	else {
	    reg = r0 != r1 ? r0 : JIT_TMP;
	    jit_movi_i(reg, i0);
	    if ((r0|r1|reg) < 8)
		T1_LDRSH(r0, r1, reg);
	    else
		T2_LDRSH(r0, r1, reg);
	}
    }
    else {
	if (i0 >= 0 && i0 <= 255)
	    _LDRSHI(r0, r1, i0);
	else if (i0 < 0 && i0 >= -255)
	    _LDRSHIN(r0, r1, -i0);
	else {
	    reg = r0 != r1 ? r0 : JIT_TMP;
	    jit_movi_i(reg, i0);
	    _LDRSH(r0, r1, reg);
	}
    }

}
#define jit_ldr_us(r0, r1)		arm_ldr_us(_jit, r0, r1)
__jit_inline void
arm_ldr_us(jit_state_t _jit, jit_gpr_t r0, jit_gpr_t r1)
{
    if (jit_thumb_p())
	T2_LDRHI(r0, r1, 0);
    else
	_LDRHI(r0, r1, 0);
}

#define jit_ldi_us(r0, i0)		arm_ldi_us(_jit, r0, i0)
__jit_inline void
arm_ldi_us(jit_state_t _jit, jit_gpr_t r0, void *i0)
{
    jit_movi_i(JIT_TMP, (uintptr_t)i0);
    if (jit_thumb_p())
	T2_LDRHI(r0, JIT_TMP, 0);
    else
	_LDRHI(r0, JIT_TMP, 0);
}

#define jit_ldxr_us(r0, r1, r2)		arm_ldxr_us(_jit, r0, r1, r2)
__jit_inline void
arm_ldxr_us(jit_state_t _jit, jit_gpr_t r0, jit_gpr_t r1, jit_gpr_t r2)
{
    if (jit_thumb_p()) {
	if ((r0|r1|r2) < 8)
	    T1_LDRH(r0, r1, r2);
	else
	    T2_LDRH(r0, r1, r2);
    }
    else
	_LDRH(r0, r1, r2);
}

#define jit_ldxi_us(r0, r1, i0)		arm_ldxi_us(_jit, r0, r1, i0)
__jit_inline void
arm_ldxi_us(jit_state_t _jit, jit_gpr_t r0, jit_gpr_t r1, int i0)
{
    jit_gpr_t		reg;
    if (jit_thumb_p()) {
	if ((r0|r1) < 8 && i0 >= 0 && !(i0 & 1) && (i0 >> 1) < 0x20)
	    T1_LDRHI(r0, r1, i0 >> 1);
	else if (i0 >= 0 && i0 <= 255)
	    T2_LDRHI(r0, r1, i0);
	else if (i0 < 0 && i0 >= -255)
	    T2_LDRHIN(r0, r1, -i0);
	else if (i0 >= 0 && i0 <= 4095)
	    T2_LDRHWI(r0, r1, i0);
	else {
	    reg = r0 != r1 ? r0 : JIT_TMP;
	    jit_movi_i(reg, i0);
	    if ((r0|r1|reg) < 8)
		T1_LDRH(r0, r1, reg);
	    else
		T2_LDRH(r0, r1, reg);
	}
    }
    else {
	if (i0 >= 0 && i0 <= 255)
	    _LDRHI(r0, r1, i0);
	else if (i0 < 0 && i0 >= -255)
	    _LDRHIN(r0, r1, -i0);
	else {
	    reg = r0 != r1 ? r0 : JIT_TMP;
	    jit_movi_i(reg, i0);
	    _LDRH(r0, r1, reg);
	}
    }
}

#define jit_ldr_i(r0, r1)		arm_ldr_i(_jit, r0, r1)
__jit_inline void
arm_ldr_i(jit_state_t _jit, jit_gpr_t r0, jit_gpr_t r1)
{
    if (jit_thumb_p())
	T2_LDRI(r0, r1, 0);
    else
	_LDRI(r0, r1, 0);
}

#define jit_ldi_i(r0, i0)		arm_ldi_i(_jit, r0, i0)
__jit_inline void
arm_ldi_i(jit_state_t _jit, jit_gpr_t r0, void *i0)
{
    jit_movi_i(JIT_TMP, (uintptr_t)i0);
    if (jit_thumb_p())
	T2_LDRI(r0, JIT_TMP, 0);
    else
	_LDRI(r0, JIT_TMP, 0);
}

#define jit_ldxr_i(r0, r1, r2)		arm_ldxr_i(_jit, r0, r1, r2)
__jit_inline void
arm_ldxr_i(jit_state_t _jit, jit_gpr_t r0, jit_gpr_t r1, jit_gpr_t r2)
{
    if (jit_thumb_p()) {
	if ((r0|r1|r2) < 8)
	    T1_LDR(r0, r1, r2);
	else
	    T2_LDR(r0, r1, r2);
    }
    else
	_LDR(r0, r1, r2);
}

#define jit_ldxi_i(r0, r1, i0)		arm_ldxi_i(_jit, r0, r1, i0)
__jit_inline void
arm_ldxi_i(jit_state_t _jit, jit_gpr_t r0, jit_gpr_t r1, int i0)
{
    jit_gpr_t		reg;
    if (jit_thumb_p()) {
	if ((r0|r1) < 8 && i0 >= 0 && !(i0 & 3) && (i0 >> 2) < 0x20)
	    T1_LDRI(r0, r1, i0 >> 2);
	else if (r1 == _R13 && r0 < 8 &&
		 i0 >= 0 && !(i0 & 3) && (i0 >> 2) <= 255)
	    T1_LDRISP(r0, i0 >> 2);
	else if (i0 >= 0 && i0 <= 255)
	    T2_LDRI(r0, r1, i0);
	else if (i0 < 0 && i0 >= -255)
	    T2_LDRIN(r0, r1, -i0);
	else if (i0 >= 0 && i0 <= 4095)
	    T2_LDRWI(r0, r1, i0);
	else {
	    reg = r0 != r1 ? r0 : JIT_TMP;
	    jit_movi_i(reg, i0);
	    if ((r0|r1|reg) < 8)
		T1_LDR(r0, r1, reg);
	    else
		T2_LDR(r0, r1, reg);
	}
    }
    else {
	if (i0 >= 0 && i0 <= 4095)
	    _LDRI(r0, r1, i0);
	else if (i0 < 0 && i0 >= -4095)
	    _LDRIN(r0, r1, -i0);
	else {
	    reg = r0 != r1 ? r0 : JIT_TMP;
	    jit_movi_i(reg, i0);
	    _LDR(r0, r1, reg);
	}
    }
}

#define jit_str_c(r0, r1)		arm_str_c(_jit, r0, r1)
__jit_inline void
arm_str_c(jit_state_t _jit, jit_gpr_t r0, jit_gpr_t r1)
{
    if (jit_thumb_p())
	T2_STRBI(r1, r0, 0);
    else
	_STRBI(r1, r0, 0);
}

#define jit_sti_c(r0, i0)		arm_sti_c(_jit, r0, i0)
__jit_inline void
arm_sti_c(jit_state_t _jit, void *i0, jit_gpr_t r0)
{
    jit_movi_i(JIT_TMP, (uintptr_t)i0);
    if (jit_thumb_p())
	T2_STRBI(r0, JIT_TMP, 0);
    else
	_STRBI(r0, JIT_TMP, 0);
}

#define jit_stxr_c(r0, r1, r2)		arm_stxr_c(_jit, r0, r1, r2)
__jit_inline void
arm_stxr_c(jit_state_t _jit, jit_gpr_t r0, jit_gpr_t r1, jit_gpr_t r2)
{
    if (jit_thumb_p()) {
	if ((r0|r1|r2) < 8)
	    T1_STRB(r2, r1, r0);
	else
	    T2_STRB(r2, r1, r0);
    }
    else
	_STRB(r2, r1, r0);
}

#define jit_stxi_c(r0, r1, i0)		arm_stxi_c(_jit, r0, r1, i0)
__jit_inline void
arm_stxi_c(jit_state_t _jit, int i0, jit_gpr_t r0, jit_gpr_t r1)
{
    if (jit_thumb_p()) {
	if ((r0|r1) < 8 && i0 >= 0 && i0 < 0x20)
	    T1_STRBI(r1, r0, i0);
	else if (i0 >= 0 && i0 <= 255)
	    T2_STRBI(r1, r0, i0);
	else if (i0 < 0 && i0 >= -255)
	    T2_STRBIN(r1, r0, -i0);
	else if (i0 >= 0 && i0 <= 4095)
	    T2_STRBWI(r1, r0, i0);
	else {
	    jit_movi_i(JIT_TMP, (int)i0);
	    if ((r0|r1|JIT_TMP) < 8)
		T1_STRB(r1, r0, JIT_TMP);
	    else
		T2_STRB(r1, r0, JIT_TMP);
	}
    }
    else {
	if (i0 >= 0 && i0 <= 4095)
	    _STRBI(r1, r0, i0);
	else if (i0 < 0 && i0 >= -4095)
	    _STRBIN(r1, r0, -i0);
	else {
	    jit_movi_i(JIT_TMP, i0);
	    _STRB(r1, r0, JIT_TMP);
	}
    }
}

#define jit_str_s(r0, r1)		arm_str_s(_jit, r0, r1)
__jit_inline void
arm_str_s(jit_state_t _jit, jit_gpr_t r0, jit_gpr_t r1)
{
    if (jit_thumb_p())
	T2_STRHI(r1, r0, 0);
    else
	_STRHI(r1, r0, 0);
}

#define jit_sti_s(r0, i0)		arm_sti_s(_jit, r0, i0)
__jit_inline void
arm_sti_s(jit_state_t _jit, void *i0, jit_gpr_t r0)
{
    jit_movi_i(JIT_TMP, (uintptr_t)i0);
    if (jit_thumb_p())
	T2_STRHI(r0, JIT_TMP, 0);
    else
	_STRHI(r0, JIT_TMP, 0);
}

#define jit_stxr_s(r0, r1, r2)		arm_stxr_s(_jit, r0, r1, r2)
__jit_inline void
arm_stxr_s(jit_state_t _jit, jit_gpr_t r0, jit_gpr_t r1, jit_gpr_t r2)
{
    if (jit_thumb_p()) {
	if ((r0|r1|r2) < 8)
	    T1_STRH(r2, r1, r0);
	else
	    T2_STRH(r2, r1, r0);
    }
    else
	_STRH(r2, r1, r0);
}

#define jit_stxi_s(r0, r1, i0)		arm_stxi_s(_jit, r0, r1, i0)
__jit_inline void
arm_stxi_s(jit_state_t _jit, int i0, jit_gpr_t r0, jit_gpr_t r1)
{
    if (jit_thumb_p()) {
	if ((r0|r1) < 8 && i0 >= 0 && !(i0 & 1) && (i0 >> 1) < 0x20)
	    T1_STRHI(r1, r0, i0 >> 1);
	else if (i0 >= 0 && i0 <= 255)
	    T2_STRHI(r1, r0, i0);
	else if (i0 < 0 && i0 >= -255)
	    T2_STRHIN(r1, r0, -i0);
	else if (i0 >= 0 && i0 <= 4095)
	    T2_STRHWI(r1, r0, i0);
	else {
	    jit_movi_i(JIT_TMP, (int)i0);
	    if ((r0|r1|JIT_TMP) < 8)
		T1_STRH(r1, r0, JIT_TMP);
	    else
		T2_STRH(r1, r0, JIT_TMP);
	}
    }
    else {
	if (i0 >= 0 && i0 <= 255)
	    _STRHI(r1, r0, i0);
	else if (i0 < 0 && i0 >= -255)
	    _STRHIN(r1, r0, -i0);
	else {
	    jit_movi_i(JIT_TMP, i0);
	    _STRH(r1, r0, JIT_TMP);
	}
    }
}

#define jit_str_i(r0, r1)		arm_str_i(_jit, r0, r1)
__jit_inline void
arm_str_i(jit_state_t _jit, jit_gpr_t r0, jit_gpr_t r1)
{
    if (jit_thumb_p())
	T2_STRI(r1, r0, 0);
    else
	_STRI(r1, r0, 0);
}

#define jit_sti_i(r0, i0)		arm_sti_i(_jit, r0, i0)
__jit_inline void
arm_sti_i(jit_state_t _jit, void *i0, jit_gpr_t r0)
{
    jit_movi_i(JIT_TMP, (uintptr_t)i0);
    if (jit_thumb_p())
	T2_STRI(r0, JIT_TMP, 0);
    else
	_STRI(r0, JIT_TMP, 0);
}

#define jit_stxr_i(r0, r1, r2)		arm_stxr_i(_jit, r0, r1, r2)
__jit_inline void
arm_stxr_i(jit_state_t _jit, jit_gpr_t r0, jit_gpr_t r1, jit_gpr_t r2)
{
    if (jit_thumb_p()) {
	if ((r0|r1|r2) < 8)
	    T1_STR(r2, r1, r0);
	else
	    T2_STR(r2, r1, r0);
    }
    else
	_STR(r2, r1, r0);
}

#define jit_stxi_i(r0, r1, i0)		arm_stxi_i(_jit, r0, r1, i0)
__jit_inline void
arm_stxi_i(jit_state_t _jit, int i0, jit_gpr_t r0, jit_gpr_t r1)
{
    if (jit_thumb_p()) {
	if ((r0|r1) < 8 && i0 >= 0 && !(i0 & 3) && (i0 >> 2) < 0x20)
	    T1_STRI(r1, r0, i0 >> 2);
	else if (r0 == _R13 && r1 < 8 &&
		 i0 >= 0 && !(i0 & 3) && (i0 >> 2) <= 255)
	    T1_STRISP(r1, i0 >> 2);
	else if (i0 >= 0 && i0 <= 255)
	    T2_STRI(r1, r0, i0);
	else if (i0 < 0 && i0 >= -255)
	    T2_STRIN(r1, r0, -i0);
	else if (i0 >= 0 && i0 <= 4095)
	    T2_STRWI(r1, r0, i0);
	else {
	    jit_movi_i(JIT_TMP, (int)i0);
	    if ((r0|r1|JIT_TMP) < 8)
		T1_STR(r1, r0, JIT_TMP);
	    else
		T2_STR(r1, r0, JIT_TMP);
	}
    }
    else {
	if (i0 >= 0 && i0 <= 4095)
	    _STRI(r1, r0, i0);
	else if (i0 < 0 && i0 >= -4095)
	    _STRIN(r1, r0, -i0);
	else {
	    jit_movi_i(JIT_TMP, i0);
	    _STR(r1, r0, JIT_TMP);
	}
    }
}

#if __BYTE_ORDER == __LITTLE_ENDIAN
/* inline glibc htons (without register clobber) */
#define jit_ntoh_us(r0, r1)		arm_ntoh_us(_jit, r0, r1)
__jit_inline void
arm_ntoh_us(jit_state_t _jit, jit_gpr_t r0, jit_gpr_t r1)
{
    if (jit_thumb_p()) {
	if ((r0|r1) < 8)
	    T1_REV16(r0, r1);
	else
	    T2_REV16(r0, r1);
    }
    else {
	if (jit_armv6_p())
	    _REV16(r0, r1);
	else {
	    _LSLI(JIT_TMP, r1, 24);
	    _LSRI(r0, r1, 8);
	    _ORR_SI(r0, r0, JIT_TMP, ARM_LSR, 16);
	}
    }
}

/* inline glibc htonl (without register clobber) */
#define jit_ntoh_ui(r0, r1)		arm_ntoh_ui(_jit, r0, r1)
__jit_inline void
arm_ntoh_ui(jit_state_t _jit, jit_gpr_t r0, jit_gpr_t r1)
{
    if (jit_thumb_p()) {
	if ((r0|r1) < 8)
	    T1_REV(r0, r1);
	else
	    T2_REV(r0, r1);
    }
    else {
	if (jit_armv6_p())
	    _REV(r0, r1);
	else {
	    _EOR_SI(JIT_TMP, r1, r1, ARM_ROR, 16);
	    _LSRI(JIT_TMP, JIT_TMP, 8);
	    _BICI(JIT_TMP, JIT_TMP, encode_arm_immediate(0xff00));
	    _EOR_SI(r0, JIT_TMP, r1, ARM_ROR, 8);
	}
    }
}
#endif

#define jit_extr_c_i(r0, r1)		arm_extr_c_i(_jit, r0, r1)
__jit_inline void
arm_extr_c_i(jit_state_t _jit, jit_gpr_t r0, jit_gpr_t r1)
{
    if (jit_thumb_p()) {
	if ((r0|r1) < 8)
	    T1_SXTB(r0, r1);
	else
	    T2_SXTB(r0, r1);
    }
    else {
	if (jit_armv6_p())
	    _SXTB(r0, r1);
	else {
	    _LSLI(r0, r1, 24);
	    _ASRI(r0, r0, 24);
	}
    }
}

#define jit_extr_c_ui(r0, r1)		arm_extr_c_ui(_jit, r0, r1)
__jit_inline void
arm_extr_c_ui(jit_state_t _jit, jit_gpr_t r0, jit_gpr_t r1)
{
    if (jit_thumb_p()) {
	if ((r0|r1) < 8)
	    T1_UXTB(r0, r1);
	else
	    T2_UXTB(r0, r1);
    }
    else {
	if (jit_armv6_p())
	    _UXTB(r0, r1);
	else
	    _ANDI(r0, r1, 0xff);
    }
}

#define jit_extr_s_i(r0, r1)		arm_extr_s_i(_jit, r0, r1)
__jit_inline void
arm_extr_s_i(jit_state_t _jit, jit_gpr_t r0, jit_gpr_t r1)
{
    if (jit_thumb_p()) {
	if ((r0|r1) < 8)
	    T1_SXTH(r0, r1);
	else
	    T2_SXTH(r0, r1);
    }
    else {
	if (jit_armv6_p())
	    _SXTH(r0, r1);
	else {
	    _LSLI(r0, r1, 16);
	    _ASRI(r0, r0, 16);
	}
    }
}

#define jit_extr_s_ui(r0, r1)		arm_extr_s_ui(_jit, r0, r1)
__jit_inline void
arm_extr_s_ui(jit_state_t _jit, jit_gpr_t r0, jit_gpr_t r1)
{
    if (jit_thumb_p()) {
	if ((r0|r1) < 8)
	    T1_UXTH(r0, r1);
	else
	    T2_UXTH(r0, r1);
    }
    else {
	if (jit_armv6_p())
	    _UXTH(r0, r1);
	else {
	    /* _ANDI(r0, r1, 0xffff) needs more instructions */
	    _LSLI(r0, r1, 16);
	    _LSRI(r0, r0, 16);
	}
    }
}

#define jit_allocai(i0)			arm_allocai(_jit, i0)
__jit_inline int
arm_allocai(jit_state_t _jit, int i0)
{
    assert(i0 >= 0);
    _jitl.alloca_offset += i0;
    jit_patch_movi((jit_insn *)_jitl.stack, (void *)
		   ((_jitl.alloca_offset + _jitl.stack_length + 7) & -8));
    return (-_jitl.alloca_offset);
}

#define jit_prolog(n)			arm_prolog(_jit, n)
__jit_inline void
arm_prolog(jit_state_t _jit, int i0)
{
    if (jit_thumb_p()) {
	/*  switch to thumb mode (better approach would be to
	 * or 1 address being called, but no clear distinction
	 * of what is a pointer to a jit function, or if patching
	 * a pointer to a jit function) */
	_ADDI(_R12, _R15, 1);
	_BX(_R12);
	if (!_jitl.after_prolog) {
	    _jitl.after_prolog = 1;
	    _jitl.thumb = _jit->x.pc;
	}  
	if (jit_hardfp_p()) {
	    T2_PUSH((1<<_R4)|(1<<_R5)|(1<<_R6)|(1<<_R7)|(1<<_R8)|(1<<_R9)|
		    /* previous fp and return address */
		    (1<<JIT_FP)|(1<<JIT_LR));
	    _VPUSH_F64(_D8, 8);
	    T2_PUSH(/* arguments (should keep state and only save "i0" registers) */
		    (1<<_R0)|(1<<_R1)|(1<<_R2)|(1<<_R3));
	}
	else
	    T2_PUSH(/* arguments (should keep state and only save "i0" registers) */
		    (1<<_R0)|(1<<_R1)|(1<<_R2)|(1<<_R3)|
		    (1<<_R4)|(1<<_R5)|(1<<_R6)|(1<<_R7)|(1<<_R8)|(1<<_R9)|
		    /* previous fp and return address */
		    (1<<JIT_FP)|(1<<JIT_LR));
	T2_MOV(JIT_FP, JIT_SP);
    }
    else {
	if (jit_hardfp_p()) {
	    _PUSH((1<<_R4)|(1<<_R5)|(1<<_R6)|(1<<_R7)|(1<<_R8)|(1<<_R9)|
		  /* previous fp and return address */
		  (1<<JIT_FP)|(1<<JIT_LR));
	    _VPUSH_F64(_D8, 8);
	    _PUSH(/* arguments (should keep state and only save "i0" registers) */
		  (1<<_R0)|(1<<_R1)|(1<<_R2)|(1<<_R3));
	}
	else
	    _PUSH(/* arguments (should keep state and only save "i0" registers) */
		  (1<<_R0)|(1<<_R1)|(1<<_R2)|(1<<_R3)|
		  (1<<_R4)|(1<<_R5)|(1<<_R6)|(1<<_R7)|(1<<_R8)|(1<<_R9)|
		  /* previous fp and return address */
		  (1<<JIT_FP)|(1<<JIT_LR));
	_MOV(JIT_FP, JIT_SP);
    }

    _jitl.nextarg_get = _jitl.nextarg_getf = 0;
    _jitl.framesize = JIT_FRAMESIZE;
    if (jit_hardfp_p())
	_jitl.framesize += 64;

    /* patch alloca and stack adjustment */
    _jitl.stack = (int *)_jit->x.pc;

    if (jit_swf_p())
	/* 6 soft double precision float registers */
	_jitl.alloca_offset = 48;
    else
	_jitl.alloca_offset = 0;

    jit_movi_p(JIT_TMP, (void *)_jitl.alloca_offset);
    jit_subr_i(JIT_SP, JIT_SP, JIT_TMP);
    _jitl.stack_length = _jitl.stack_offset = 0;
}

#define jit_callr(r0)			arm_callr(_jit, r0)
static void
arm_callr(jit_state_t _jit, jit_gpr_t r0)
{
    if (jit_thumb_p())
	T1_BLX(r0);
    else
	_BLX(r0);
}

#define jit_calli(i0)			arm_calli(_jit, i0)
__jit_inline jit_insn *
arm_calli(jit_state_t _jit, void *i0)
{
    jit_insn	*l;
    l = _jit->x.pc;
    jit_movi_p(JIT_TMP, i0);
    if (jit_thumb_p())
	T1_BLX(JIT_TMP);
    else
	_BLX(JIT_TMP);
    return (l);
}

#define jit_prepare_i(i0)		arm_prepare_i(_jit, i0)
__jit_inline void
arm_prepare_i(jit_state_t _jit, int i0)
{
    assert(i0 >= 0 && !_jitl.stack_offset && !_jitl.nextarg_put);
    _jitl.stack_offset = i0 << 2;
}

#define jit_arg_c()			arm_arg_i(_jit)
#define jit_arg_uc()			arm_arg_i(_jit)
#define jit_arg_s()			arm_arg_i(_jit)
#define jit_arg_us()			arm_arg_i(_jit)
#define jit_arg_i()			arm_arg_i(_jit)
#define jit_arg_ui()			arm_arg_i(_jit)
#define jit_arg_l()			arm_arg_i(_jit)
#define jit_arg_ul()			arm_arg_i(_jit)
#define jit_arg_p()			arm_arg_i(_jit)
__jit_inline int
arm_arg_i(jit_state_t _jit)
{
    int		ofs = _jitl.nextarg_get++;
    if (ofs > 3) {
	ofs = _jitl.framesize;
	_jitl.framesize += sizeof(int);
    }
    return (ofs);
}

#define jit_getarg_c(r0, i0)		arm_getarg_c(_jit, r0, i0)
__jit_inline void
arm_getarg_c(jit_state_t _jit, jit_gpr_t r0, int i0)
{
    if (i0 < 4)
	i0 <<= 2;
#if __BYTE_ORDER == __BIG_ENDIAN
    i0 += sizeof(int) - sizeof(char);
#endif
    jit_ldxi_c(r0, JIT_FP, i0);
}

#define jit_getarg_uc(r0, i0)		arm_getarg_uc(_jit, r0, i0)
__jit_inline void
arm_getarg_uc(jit_state_t _jit, jit_gpr_t r0, int i0)
{
    if (i0 < 4)
	i0 <<= 2;
#if __BYTE_ORDER == __BIG_ENDIAN
    i0 += sizeof(int) - sizeof(char);
#endif
    jit_ldxi_uc(r0, JIT_FP, i0);
}

#define jit_getarg_s(r0, i0)		arm_getarg_s(_jit, r0, i0)
__jit_inline void
arm_getarg_s(jit_state_t _jit, jit_gpr_t r0, int i0)
{
    if (i0 < 4)
	i0 <<= 2;
#if __BYTE_ORDER == __BIG_ENDIAN
    i0 += sizeof(int) - sizeof(short);
#endif
    jit_ldxi_s(r0, JIT_FP, i0);
}

#define jit_getarg_us(r0, i0)		arm_getarg_us(_jit, r0, i0)
__jit_inline void
arm_getarg_us(jit_state_t _jit, jit_gpr_t r0, int i0)
{
    if (i0 < 4)
	i0 <<= 2;
#if __BYTE_ORDER == __BIG_ENDIAN
    i0 += sizeof(int) - sizeof(short);
#endif
    jit_ldxi_us(r0, JIT_FP, i0);
}

#define jit_getarg_i(r0, i0)		arm_getarg_i(_jit, r0, i0)
#define jit_getarg_ui(r0, i0)		arm_getarg_i(_jit, r0, i0)
#define jit_getarg_l(r0, i0)		arm_getarg_i(_jit, r0, i0)
#define jit_getarg_ul(r0, i0)		arm_getarg_i(_jit, r0, i0)
#define jit_getarg_p(r0, i0)		arm_getarg_i(_jit, r0, i0)
__jit_inline void
arm_getarg_i(jit_state_t _jit, jit_gpr_t r0, int i0)
{
    /* arguments are saved in prolog */
    if (i0 < 4)
	i0 <<= 2;
    jit_ldxi_i(r0, JIT_FP, i0);
}

#define jit_pusharg_i(r0)		arm_pusharg_i(_jit, r0)
__jit_inline void
arm_pusharg_i(jit_state_t _jit, jit_gpr_t r0)
{
    int		ofs = _jitl.nextarg_put++;
    assert(ofs < 256);
    _jitl.stack_offset -= sizeof(int);
    _jitl.arguments[ofs] = (int *)_jit->x.pc;
    _jitl.types[ofs >> 5] &= ~(1 << (ofs & 31));
    /* force 32 bit instruction due to possibly needing to trasform it */
    if (jit_thumb_p())
	T2_STRWI(r0, JIT_SP, 0);
    else
	_STRI(r0, JIT_SP, 0);
}

static void
arm_patch_arguments(jit_state_t _jit)
{
    int		 reg;
    int		 ioff;
    int		 foff;
    int		 size;
    int		 index;
    jit_thumb_t	 thumb;
    int		 offset;
    union {
	_ui	*i;
	_us	*s;
    } u;

    ioff = foff = 0;
    for (index = _jitl.nextarg_put - 1, offset = 0; index >= 0; index--) {
	if (_jitl.types[index >> 5] & (1 << (index & 31)))
	    size = sizeof(double);
	else
	    size = sizeof(int);
	u.i = (_ui*)_jitl.arguments[index];
	if (jit_thumb_p()) {
	    code2thumb(thumb.s[0], thumb.s[1], u.s[0], u.s[1]);
	    switch (thumb.i & 0xfff00f00) {
		case ARM_CC_AL|ARM_VSTR|ARM_P:
		    if (jit_hardfp_p()) {
			if (foff < 16) {
			    reg = (thumb.i >> 12) & 0xf;
			    thumb.i = (ARM_CC_AL|ARM_VMOV_F |
				       ((foff >> 1) << 12) | reg);
			    if (foff & 1)
				thumb.i |= ARM_V_D;
			    thumb2code(thumb.s[0], thumb.s[1], u.s[0], u.s[1]);
			    ++foff;
			    continue;
			}
		    }
		    else {
			if (ioff < 4) {
			    thumb.i = ((thumb.i & 0xfff0ff00) |
				       (JIT_FP << 16) | ioff);
			    thumb2code(thumb.s[0], thumb.s[1], u.s[0], u.s[1]);
			    ++ioff;
			    continue;
			}
		    }
		    thumb.i = (thumb.i & 0xffffff00) | (offset >> 2);
		    thumb2code(thumb.s[0], thumb.s[1], u.s[0], u.s[1]);
		    break;
		case ARM_CC_AL|ARM_VSTR|ARM_V_F64|ARM_P:
		    if (jit_hardfp_p()) {
			if (foff & 1)
			    ++foff;
			if (foff < 16) {
			    reg = (thumb.i >> 12) & 0xf;
			    thumb.i = (ARM_CC_AL|ARM_VMOV_F|ARM_V_F64 |
				       ((foff >> 1) << 12) | reg);
			    thumb2code(thumb.s[0], thumb.s[1], u.s[0], u.s[1]);
			    foff += 2;
			    continue;
			}
		    }
		    else {
			if (ioff & 1)
			    ++ioff;
			if (ioff < 4) {
			    thumb.i = ((thumb.i & 0xfff0ff00) |
				       (JIT_FP << 16) | ioff);
			    thumb2code(thumb.s[0], thumb.s[1], u.s[0], u.s[1]);
			    ioff += 2;
			    continue;
			}
		    }
		    if (offset & 7)
			offset += sizeof(int);
		    thumb.i = (thumb.i & 0xffffff00) | (offset >> 2);
		    thumb2code(thumb.s[0], thumb.s[1], u.s[0], u.s[1]);
		    break;
		case THUMB2_STRWI:
		thumb_stri:
		    if (size == 8 && (ioff & 1))
			++ioff;
		    if (ioff < 4) {
			thumb.i = ((thumb.i & 0xfff0f000) |
				   (JIT_FP << 16) | (ioff << 2));
			thumb2code(thumb.s[0], thumb.s[1], u.s[0], u.s[1]);
			++ioff;
			if (size == 8) {
			    code2thumb(thumb.s[0], thumb.s[1], u.s[2], u.s[3]);
			    thumb.i = ((thumb.i & 0xfff0f000) |
				       (JIT_FP << 16) | (ioff << 2));
			    thumb2code(thumb.s[0], thumb.s[1], u.s[2], u.s[3]);
			    ++ioff;
			}
			continue;
		    }
		    if (size == 8 && (offset & 7))
			offset += sizeof(int);
		    thumb.i = (thumb.i & 0xfffff000) | offset;
		    thumb2code(thumb.s[0], thumb.s[1], u.s[0], u.s[1]);
		    if (size == 8) {
			code2thumb(thumb.s[0], thumb.s[1], u.s[2], u.s[3]);
			thumb.i = (thumb.i & 0xfffff000) | (offset + 4);
			thumb2code(thumb.s[0], thumb.s[1], u.s[2], u.s[3]);
		    }
		    break;
		default:
		    /* offset too large */
		    if ((thumb.i & 0xfff00000) == THUMB2_STRWI)
			goto thumb_stri;
		    abort();
	    }
	}
	else {
	    switch (u.i[0] & 0xfff00f00) {
		case ARM_CC_AL|ARM_VSTR|ARM_P:
		    if (jit_hardfp_p()) {
			if (foff < 16) {
			    reg = (u.i[0] >> 12) & 0xf;
			    u.i[0] = (ARM_CC_AL|ARM_VMOV_F |
				      ((foff >> 1) << 12) | reg);
			    if (foff & 1)
				u.i[0] |= ARM_V_D;
			    ++foff;
			    continue;
			}
		    }
		    else {
			if (ioff < 4) {
			    u.i[0] = ((u.i[0] & 0xfff0ff00) |
				      (JIT_FP << 16) | ioff);
			    ++ioff;
			    continue;
			}
		    }
		    u.i[0] = (u.i[0] & 0xffffff00) | (offset >> 2);
		    break;
		case ARM_CC_AL|ARM_VSTR|ARM_V_F64|ARM_P:
		    if (jit_hardfp_p()) {
			if (foff & 1)
			    ++foff;
			if (foff < 16) {
			    reg = (u.i[0] >> 12) & 0xf;
			    u.i[0] = (ARM_CC_AL|ARM_VMOV_F|ARM_V_F64 |
				      ((foff >> 1) << 12) | reg);
			    foff += 2;
			    continue;
			}
		    }
		    else {
			if (ioff & 1)
			    ++ioff;
			if (ioff < 4) {
			    u.i[0] = ((u.i[0] & 0xfff0ff00) |
				      (JIT_FP << 16) | ioff);
			    ioff += 2;
			    continue;
			}
		    }
		    if (offset & 7)
			offset += sizeof(int);
		    u.i[0] = (u.i[0] & 0xffffff00) | (offset >> 2);
		    break;
		case ARM_CC_AL|ARM_STRI|ARM_P:
		arm_stri:
		    if (size == 8 && (ioff & 1))
			++ioff;
		    if (ioff < 4) {
			u.i[0] = ((u.i[0] & 0xfff0f000) |
				  (JIT_FP << 16) | (ioff << 2));
			++ioff;
			if (size == 8) {
			    u.i[1] = ((u.i[1] & 0xfff0f000) |
				      (JIT_FP << 16) | (ioff << 2));
			    ++ioff;
			}
			continue;
		    }
		    if (size == 8 && (offset & 7))
			offset += sizeof(int);
		    u.i[0] = (u.i[0] & 0xfffff000) | offset;
		    if (size == 8)
			u.i[1] = (u.i[1] & 0xfffff000) | (offset + 4);
		    break;
		default:
		    /* offset too large */
		    if ((u.i[0] & 0xfff00000) == (ARM_CC_AL|ARM_STRI|ARM_P))
			goto arm_stri;
		    abort();
	    }
	}
	offset += size;
    }
    _jitl.reglist = ((1 << ioff) - 1) & 0xf;
    if (_jitl.stack_length < offset) {
	_jitl.stack_length = offset;
	jit_patch_movi((jit_insn *)_jitl.stack, (void *)
		       ((_jitl.alloca_offset +
			 _jitl.stack_length + 7) & -8));
    }
}

#define jit_finishr(rs)			arm_finishr(_jit, rs)
__jit_inline void
arm_finishr(jit_state_t _jit, jit_gpr_t r0)
{
    assert(!_jitl.stack_offset);
    if (r0 < 4) {
	jit_movr_i(JIT_TMP, r0);
	r0 = JIT_TMP;
    }
    arm_patch_arguments(_jit);
    _jitl.nextarg_put = 0;
    if (_jitl.reglist) {
	if (jit_thumb_p())
	    T2_LDMIA(JIT_FP, _jitl.reglist);
	else
	    _LDMIA(JIT_FP, _jitl.reglist);
	_jitl.reglist = 0;
    }
    jit_callr(r0);
}

#define jit_finish(i0)			arm_finishi(_jit, i0)
__jit_inline jit_insn *
arm_finishi(jit_state_t _jit, void *i0)
{
    assert(!_jitl.stack_offset);
    arm_patch_arguments(_jit);
    _jitl.nextarg_put = 0;
    if (_jitl.reglist) {
	if (jit_thumb_p())
	    T2_LDMIA(JIT_FP, _jitl.reglist);
	else
	    _LDMIA(JIT_FP, _jitl.reglist);
	_jitl.reglist = 0;
    }
    return (jit_calli(i0));
}

#define jit_retval_i(r0)		jit_movr_i(r0, JIT_RET)
#define jit_ret()			arm_ret(_jit)
__jit_inline void
arm_ret(jit_state_t _jit)
{
    /* do not restore arguments */
    jit_addi_i(JIT_SP, JIT_FP, 16);
    if (jit_hardfp_p())
	_VPOP_F64(_D8, 8);
    if (jit_thumb_p())
	T2_POP(/* callee save */
	       (1<<_R4)|(1<<_R5)|(1<<_R6)|(1<<_R7)|(1<<_R8)|(1<<_R9)|
	       /* previous fp and return address */
	       (1<<JIT_FP)|(1<<JIT_PC));
    else
	_POP(/* callee save */
	     (1<<_R4)|(1<<_R5)|(1<<_R6)|(1<<_R7)|(1<<_R8)|(1<<_R9)|
	     /* previous fp and return address */
	     (1<<JIT_FP)|(1<<JIT_PC));
    if (jit_thumb_p() && ((uintptr_t)_jit->x.pc & 2))
	T1_NOP();
}

/* just to pass make check... */
#ifdef JIT_NEED_PUSH_POP
# define jit_pushr_i(r0)		arm_pushr_i(_jit, r0)
__jit_inline int
arm_pushr_i(jit_state_t _jit, jit_gpr_t r0)
{
    int		offset;
    assert(_jitl.pop < sizeof(_jitl.push) / sizeof(_jitl.push[0]));
    offset = jit_allocai(4);
    _jitl.push[_jitl.pop++] = offset;
    jit_stxi_i(offset, JIT_FP, r0);
}

# define jit_popr_i(r0)			arm_popr_i(_jit, r0)
__jit_inline int
arm_popr_i(jit_state_t _jit, jit_gpr_t r0)
{
    int		offset;
    assert(_jitl.pop > 0);
    offset = _jitl.push[--_jitl.pop];
    jit_ldxi_i(r0, JIT_FP, offset);
}
#endif

#endif /* __lightning_core_arm_h */
