/******************************** -*- C -*- ****************************
 *
 *	Platform-independent layer (i386 version)
 *
 ***********************************************************************/


/***********************************************************************
 *
 * Copyright 2000,2001,2002,2003,2006,2010 Free Software Foundation, Inc.
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
 *	Matthew Flatt
 *	Paolo Bonzini
 *	Paulo Cesar Pereira de Andrade
 ***********************************************************************/



#ifndef __lightning_core_h
#define __lightning_core_h

static const jit_gpr_t
jit_arg_reg_order[] = {
    _RDI, _RSI, _RDX, _RCX, _R8, _R9
};

#define JIT_REXTMP			_R12

/* Number or integer argument registers */
#define JIT_ARG_MAX			6

/* Number of float argument registers */
#define JIT_FP_ARG_MAX			8

#define JIT_R_NUM			3
static const jit_gpr_t
jit_r_order[JIT_R_NUM] = {
    _RAX, _R10, _R11
};
#define JIT_R(i)			jit_r_order[i]

#define JIT_V_NUM			3
static const jit_gpr_t
jit_v_order[JIT_R_NUM] = {
    _RBX, _R13, _R14
};
#define JIT_V(i)			jit_v_order[i]

#define jit_allocai(n)			x86_allocai(_jit, n)
__jit_inline int
x86_allocai(jit_state_t _jit, int length)
{
    assert(length >= 0);
    _jitl.alloca_offset += length;
    if (_jitl.alloca_offset + _jitl.stack_length > *_jitl.stack)
	*_jitl.stack += (length + 15) & ~15;
    return (-_jitl.alloca_offset);
}

#define jit_movi_p(r0, i0)		x86_movi_p(_jit, r0, i0)
__jit_inline jit_insn *
x86_movi_p(jit_state_t _jit, jit_gpr_t r0, void *i0)
{
    MOVQir((long)i0, r0);
    return (_jit->x.pc);
}

#define jit_movi_l(r0, i0)		x86_movi_l(_jit, r0, i0)
#define jit_movi_ul(r0, i0)		x86_movi_l(_jit, r0, i0)
/* ensure proper zero/sign extension */
#define jit_movi_i(r0, i0)		x86_movi_l(_jit, r0, (long)(int)i0)
#define jit_movi_ui(r0, i0)		x86_movi_l(_jit, r0, (_ul)(_ui)i0)
__jit_inline void
x86_movi_l(jit_state_t _jit, jit_gpr_t r0, long i0)
{
    if (i0) {
	if (jit_can_zero_extend_int_p(i0))
	    MOVLir(i0, r0);
	else
	    MOVQir(i0, r0);
    }
    else
	XORQrr(r0, r0);
}

/* Return address is 8 bytes, plus 5 registers = 40 bytes, total = 48 bytes. */
#define jit_prolog(n)			x86_prolog(_jit, n)
__jit_inline void
x86_prolog(jit_state_t _jit, int n)
{
    /* offset of stack arguments */
    _jitl.framesize = 48;
    /* offsets of arguments */
    _jitl.nextarg_getfp = _jitl.nextarg_geti = 0;
    /* stack frame */
    PUSHQr(_RBX);
    PUSHQr(_R12);
    PUSHQr(_R13);
    PUSHQr(_R14);
    PUSHQr(JIT_FP);
    MOVQrr(JIT_SP, JIT_FP);

    /*  Adjust stack only once for alloca and stack arguments */
    _REXQrr(_NOREG, JIT_SP);
    _O(0x81);
    _Mrm(_b11, X86_SUB, _rA(JIT_SP));
    _jit_I(0);

    _jitl.stack = ((int *)_jit->x.pc) - 1;
    _jitl.alloca_offset = _jitl.stack_offset = _jitl.stack_length = 0;
    _jitl.float_offset = 0;
}

#define jit_ret()			x86_ret(_jit)
__jit_inline void
x86_ret(jit_state_t _jit)
{
    LEAVE_();
    POPQr(_R14);
    POPQr(_R13);
    POPQr(_R12);
    POPQr(_RBX);
    RET_();
}

#define jit_calli(i0)			x86_calli(_jit, i0)
__jit_inline jit_insn *
x86_calli(jit_state_t _jit, void *i0)
{
    MOVQir((long)i0, JIT_REXTMP);
    _jitl.label = _jit->x.pc;
    CALLsr(JIT_REXTMP);
    return (_jitl.label);
}

#define jit_callr(r0)			x86_callr(_jit, r0)
__jit_inline void
x86_callr(jit_state_t _jit, jit_gpr_t r0)
{
    CALLsr(r0);
}

#define jit_patch_calli(call, label)	x86_patch_calli(_jit, call, label)
__jit_inline void
x86_patch_calli(jit_state_t _jit, jit_insn *call, jit_insn *label)
{
    jit_patch_movi(call, label);
}

#define jit_prepare_i(ni)		x86_prepare_i(_jit, ni)
__jit_inline void
x86_prepare_i(jit_state_t _jit, int count)
{
    assert(count		>= 0 &&
	   _jitl.stack_offset	== 0 &&
	   _jitl.nextarg_puti	== 0 &&
	   _jitl.nextarg_putfp	== 0 &&
	   _jitl.fprssize	== 0);

    /* offset of right to left integer argument */
    _jitl.nextarg_puti = count;

    /* update stack offset and check if need to patch stack adjustment */
    if (_jitl.nextarg_puti > JIT_ARG_MAX) {
	_jitl.stack_offset = (_jitl.nextarg_puti - JIT_ARG_MAX) << 3;
	if (jit_push_pop_p())
	    _jitl.stack_length = _jitl.stack_offset;
	else if (_jitl.stack_length < _jitl.stack_offset) {
	    _jitl.stack_length = _jitl.stack_offset;
	    *_jitl.stack = (_jitl.alloca_offset +
			    _jitl.stack_length + 15) & ~15;
	}
    }
}

#define jit_patch_at(jump, label)	x86_patch_at(_jit, jump, label)
__jit_inline void
x86_patch_at(jit_state_t _jit, jit_insn *jump, jit_insn *label)
{
    if (_jitl.long_jumps)
	jit_patch_abs_long_at(jump - 3, label);
    else
	jit_patch_rel_int_at(jump, label);
}

/* ALU */
#define jit_negr_l(r0, r1)		x86_negr_l(_jit, r0, r1)
__jit_inline void
x86_negr_l(jit_state_t _jit, jit_gpr_t r0, jit_gpr_t r1)
{
    if (r0 == r1)
	NEGQr(r0);
    else {
	XORLrr(r0, r0);
	SUBQrr(r1, r0);
    }
}

#define jit_notr_l(r0, r1)		x86_notr_l(_jit, r0, r1)
__jit_inline void
x86_notr_l(jit_state_t _jit, jit_gpr_t r0, jit_gpr_t r1)
{
    jit_movr_l(r0, r1);
    NOTQr(r0);
}

#define jit_addi_l(r0, r1, i0)		x86_addi_l(_jit, r0, r1, i0)
__jit_inline void
x86_addi_l(jit_state_t _jit, jit_gpr_t r0, jit_gpr_t r1, long i0)
{
    if (i0 == 0)
	jit_movr_l(r0, r1);
    else if (i0 == 1) {
	jit_movr_l(r0, r1);
	INCQr(r0);
    }
    else if (i0 == -1) {
	jit_movr_l(r0, r1);
	DECQr(r0);
    }
    else if (jit_can_sign_extend_int_p(i0)) {
	if (r0 == r1)
	    ADDQir(i0, r0);
	else
	    LEAQmr(i0, r1, _NOREG, _SCL1, r0);
    }
    else if (r0 != r1) {
	MOVQir(i0, r0);
	ADDQrr(r1, r0);
    }
    else {
	MOVQir(i0, JIT_REXTMP);
	ADDQrr(JIT_REXTMP, r0);
    }
}

#define jit_addr_l(r0, r1, r2)		x86_addr_l(_jit, r0, r1, r2)
__jit_inline void
x86_addr_l(jit_state_t _jit, jit_gpr_t r0, jit_gpr_t r1, jit_gpr_t r2)
{
    if (r0 == r1)
	ADDQrr(r2, r0);
    else if (r0 == r2)
	ADDQrr(r1, r0);
    else
	LEAQmr(0, r1, r2, _SCL1, r0);
}

#define jit_subi_l(r0, r1, i0)		x86_subi_l(_jit, r0, r1, i0)
__jit_inline void
x86_subi_l(jit_state_t _jit, jit_gpr_t r0, jit_gpr_t r1, long i0)
{
    if (i0 == 0)
	jit_movr_l(r0, r1);
    else if (i0 == 1) {
	jit_movr_l(r0, r1);
	DECQr(r0);
    }
    else if (i0 == -1) {
	jit_movr_l(r0, r1);
	INCQr(r0);
    }
    else if (jit_can_sign_extend_int_p(i0)) {
	if (r0 == r1)
	    SUBQir(i0, r0);
	else
	    LEAQmr(-i0, r1, _NOREG, _SCL1, r0);
    }
    else if (r0 != r1) {
	MOVQir(-i0, r0);
	ADDQrr(r1, r0);
    }
    else {
	MOVQir(i0, JIT_REXTMP);
	SUBQrr(JIT_REXTMP, r0);
    }
}

#define jit_subr_l(r0, r1, r2)		x86_subr_l(_jit, r0, r1, r2)
__jit_inline void
x86_subr_l(jit_state_t _jit, jit_gpr_t r0, jit_gpr_t r1, jit_gpr_t r2)
{
    if (r1 == r2)
	XORQrr(r0, r0);
    else if (r0 == r2) {
	SUBQrr(r1, r0);
	NEGQr(r0);
    }
    else {
	jit_movr_l(r0, r1);
	SUBQrr(r2, r0);
    }
}

/* o Immediates are sign extended
 * o CF (C)arry (F)lag is set when interpreting it as unsigned addition
 * o OF (O)verflow (F)lag is set when interpreting it as signed addition
 */
/* Commutative */
#define jit_addci_ul(r0, r1, i0)	x86_addci_ul(_jit, r0, r1, i0)
__jit_inline void
x86_addci_ul(jit_state_t _jit, jit_gpr_t r0, jit_gpr_t r1, unsigned long i0)
{
    if (jit_can_sign_extend_int_p(i0)) {
	jit_movr_l(r0, r1);
	ADDQir(i0, r0);
    }
    else if (r0 == r1) {
	MOVQir(i0, JIT_REXTMP);
	ADDQrr(JIT_REXTMP, r0);
    }
    else {
	MOVQir(i0, r0);
	ADDQrr(r1, r0);
    }
}

#define jit_addcr_ul(r0, r1, r2)	x86_addcr_ul(_jit, r0, r1, r2)
__jit_inline void
x86_addcr_ul(jit_state_t _jit, jit_gpr_t r0, jit_gpr_t r1, jit_gpr_t r2)
{
    if (r0 == r2)
	ADDQrr(r1, r0);
    else if (r0 == r1)
	ADDQrr(r2, r0);
    else {
	MOVQrr(r1, r0);
	ADDQrr(r2, r0);
    }
}

#define jit_addxi_ul(r0, r1, i0)	x86_addxi_ul(_jit, r0, r1, i0)
__jit_inline void
x86_addxi_ul(jit_state_t _jit, jit_gpr_t r0, jit_gpr_t r1, unsigned long i0)
{
    if (jit_can_sign_extend_int_p(i0)) {
	jit_movr_l(r0, r1);
	ADCQir(i0, r0);
    }
    else if (r0 == r1) {
	MOVQir(i0, JIT_REXTMP);
	ADCQrr(JIT_REXTMP, r0);
    }
    else {
	MOVQir(i0, r0);
	ADCQrr(r1, r0);
    }
}

#define jit_addxr_ul(r0, r1, r2)	x86_addxr_ul(_jit, r0, r1, r2)
__jit_inline void
x86_addxr_ul(jit_state_t _jit, jit_gpr_t r0, jit_gpr_t r1, jit_gpr_t r2)
{
    if (r0 == r2)
	ADCQrr(r1, r0);
    else if (r0 == r1)
	ADCQrr(r2, r0);
    else {
	MOVQrr(r1, r0);
	ADCQrr(r2, r0);
    }
}

/* Non commutative */
#define jit_subci_ul(r0, r1, i0)	x86_subci_ul(_jit, r0, r1, i0)
__jit_inline void
x86_subci_ul(jit_state_t _jit, jit_gpr_t r0, jit_gpr_t r1, unsigned long i0)
{
    jit_movr_l(r0, r1);
    if (jit_can_sign_extend_int_p(i0))
	SUBQir(i0, r0);
    else {
	MOVQir(i0, JIT_REXTMP);
	SUBQrr(JIT_REXTMP, r0);
    }
}

#define jit_subcr_ul(r0, r1, r2)	x86_subcr_ul(_jit, r0, r1, r2)
__jit_inline void
x86_subcr_ul(jit_state_t _jit, jit_gpr_t r0, jit_gpr_t r1, jit_gpr_t r2)
{
    if (r0 == r2 && r0 != r1) {
	MOVQrr(r0, JIT_REXTMP);
	MOVQrr(r1, r0);
	SUBQrr(JIT_REXTMP, r0);
    }
    else {
	jit_movr_l(r0, r1);
	SUBQrr(r2, r0);
    }
}

#define jit_subxi_ul(r0, r1, i0)	x86_subxi_ul(_jit, r0, r1, i0)
__jit_inline void
x86_subxi_ul(jit_state_t _jit, jit_gpr_t r0, jit_gpr_t r1, unsigned long i0)
{
    jit_movr_l(r0, r1);
    if (jit_can_sign_extend_int_p(i0))
	SBBQir(i0, r0);
    else {
	MOVQir(i0, JIT_REXTMP);
	SBBQrr(JIT_REXTMP, r0);
    }
}

#define jit_subxr_ul(r0, r1, r2)	x86_subxr_ul(_jit, r0, r1, r2)
__jit_inline void
x86_subxr_ul(jit_state_t _jit, jit_gpr_t r0, jit_gpr_t r1, jit_gpr_t r2)
{
    if (r0 == r2 && r0 != r1) {
	MOVQrr(r0, JIT_REXTMP);
	MOVQrr(r1, r0);
	SBBQrr(JIT_REXTMP, r0);
    }
    else {
	jit_movr_l(r0, r1);
	SBBQrr(r2, r0);
    }
}

#define jit_andi_l(r0, r1, i0)		x86_andi_l(_jit, r0, r1, i0)
__jit_inline void
x86_andi_l(jit_state_t _jit, jit_gpr_t r0, jit_gpr_t r1, long i0)
{
    if (i0 == 0)
	XORQrr(r0, r0);
    else if (i0 == -1)
	jit_movr_l(r0, r1);
    else if (r0 == r1) {
	if (jit_can_sign_extend_int_p(i0))
	    ANDQir(i0, r0);
	else {
	    MOVQir(i0, JIT_REXTMP);
	    ANDQrr(JIT_REXTMP, r0);
	}
    }
    else {
	MOVQir(i0, r0);
	ANDQrr(r1, r0);
    }
}

#define jit_andr_l(r0, r1, r2)		x86_andr_l(_jit, r0, r1, r2)
__jit_inline void
x86_andr_l(jit_state_t _jit, jit_gpr_t r0, jit_gpr_t r1, jit_gpr_t r2)
{
    if (r1 == r2)
	jit_movr_l(r0, r1);
    else if (r0 == r1)
	ANDQrr(r2, r0);
    else if (r0 == r2)
	ANDQrr(r1, r0);
    else {
	MOVQrr(r1, r0);
	ANDQrr(r2, r0);
    }
}

#define jit_ori_l(r0, r1, i0)		x86_ori_l(_jit, r0, r1, i0)
__jit_inline void
x86_ori_l(jit_state_t _jit, jit_gpr_t r0, jit_gpr_t r1, long i0)
{
    if (i0 == 0)
	jit_movr_l(r0, r1);
    else if (i0 == -1)
	MOVQir(-1, r0);
    else if (r0 == r1) {
	if (jit_can_sign_extend_char_p(i0))
	    ORBir(i0, r0);
	else if (jit_can_sign_extend_int_p(i0))
	    ORQir(i0, r0);
	else {
	    MOVQir(i0, JIT_REXTMP);
	    ORQrr(JIT_REXTMP, r0);
	}
    }
    else {
	MOVQir(i0, r0);
	ORQrr(r1, r0);
    }
}

#define jit_orr_l(r0, r1, r2)		x86_orr_l(_jit, r0, r1, r2)
__jit_inline void
x86_orr_l(jit_state_t _jit, jit_gpr_t r0, jit_gpr_t r1, jit_gpr_t r2)
{
    if (r1 == r2)
	jit_movr_l(r0, r1);
    else if (r0 == r1)
	ORQrr(r2, r0);
    else if (r0 == r2)
	ORQrr(r1, r0);
    else {
	MOVQrr(r1, r0);
	ORQrr(r2, r0);
    }
}

#define jit_xori_l(r0, r1, i0)		x86_xori_l(_jit, r0, r1, i0)
__jit_inline void
x86_xori_l(jit_state_t _jit, jit_gpr_t r0, jit_gpr_t r1, long i0)
{
    if (i0 == 0)
	jit_movr_l(r0, r1);
    else if (i0 == -1) {
	jit_movr_l(r0, r1);
	NOTQr(r0);
    }
    else {
	if (jit_can_sign_extend_char_p(i0)) {
	    jit_movr_l(r0, r1);
	    XORBir(i0, r0);
	}
	else if (jit_can_sign_extend_int_p(i0)) {
	    jit_movr_l(r0, r1);
	    XORQir(i0, r0);
	}
	else {
	    if (r0 == r1) {
		MOVQir(i0, JIT_REXTMP);
		XORQrr(JIT_REXTMP, r0);
	    }
	    else {
		MOVQir(i0, r0);
		XORQrr(r1, r0);
	    }
	}
    }
}

#define jit_xorr_l(r0, r1, r2)		x86_xorr_l(_jit, r0, r1, r2)
__jit_inline void
x86_xorr_l(jit_state_t _jit, jit_gpr_t r0, jit_gpr_t r1, jit_gpr_t r2)
{
    if (r1 == r2)
	XORLrr(r0, r0);
    else if (r0 == r1)
	XORQrr(r2, r0);
    else if (r0 == r2)
	XORQrr(r1, r0);
    else {
	MOVQrr(r1, r0);
	XORQrr(r2, r0);
    }
}

#define jit_muli_l(r0, r1, i0)		x86_muli_l(_jit, r0, r1, i0)
#define jit_muli_ul(r0, r1, i0)		x86_muli_l(_jit, r0, r1, i0)
__jit_inline void
x86_muli_l(jit_state_t _jit, jit_gpr_t r0, jit_gpr_t r1, long i0)
{
    switch (i0) {
	case 0:
	    XORLrr(r0, r0);
	    break;
	case 1:
	    jit_movr_l(r0, r1);
	    break;
	case -1:
	    jit_negr_l(r0, r1);
	    break;
	case 2:
	    LEAQmr(0, _NOREG, r1, _SCL2, r0);
	    break;
	case 4:
	    LEAQmr(0, _NOREG, r1, _SCL4, r0);
	    break;
	case 8:
	    LEAQmr(0, _NOREG, r1, _SCL8, r0);
	    break;
	default:
	    if (i0 > 0 && !(i0 & (i0 - 1))) {
		jit_movr_l(r0, r1);
		SHLQir(ffsl(i0) - 1, r0);
	    }
	    else if (jit_can_sign_extend_int_p(i0))
		IMULQirr(i0, r1, r0);
	    else if (r0 == r1) {
		MOVQir(i0, JIT_REXTMP);
		IMULQrr(JIT_REXTMP, r0);
	    }
	    else {
		MOVQir(i0, r0);
		IMULQrr(r1, r0);
	    }
	    break;
    }
}

#define jit_mulr_l(r0, r1, r2)		x86_mulr_l(_jit, r0, r1, r2)
#define jit_mulr_ul(r0, r1, r2)		x86_mulr_l(_jit, r0, r1, r2)
__jit_inline void
x86_mulr_l(jit_state_t _jit, jit_gpr_t r0, jit_gpr_t r1, jit_gpr_t r2)
{
    if (r0 == r1)
	IMULQrr(r2, r0);
    else if (r0 == r2)
	IMULQrr(r1, r0);
    else {
	MOVQrr(r1, r0);
	IMULQrr(r2, r0);
    }
}

/*  Instruction format is:
 *	imul reg64/mem64
 *  and the result is stored in %rdx:%rax
 *  %rax = low 64 bits
 *  %rdx = high 64 bits
 */
#define jit_muli_l_(r0, i0)		x86_muli_l_(_jit, r0, i0)
__jit_inline void
x86_muli_l_(jit_state_t _jit, jit_gpr_t r0, long i0)
{
    if (r0 == _RAX) {
	jit_movi_l(_RDX, i0);
	IMULQr(_RDX);
    }
    else {
	jit_movi_l(_RAX, i0);
	IMULQr(r0);
    }
}

#define jit_hmuli_l(r0, r1, i0)		x86_hmuli_l(_jit, r0, r1, i0)
__jit_inline void
x86_hmuli_l(jit_state_t _jit, jit_gpr_t r0, jit_gpr_t r1, long i0)
{
    if (r0 == _RDX) {
	MOVQrr(_RAX, JIT_REXTMP);
	jit_muli_l_(r1, i0);
	MOVQrr(JIT_REXTMP, _RAX);
    }
    else if (r0 == _RAX) {
	MOVQrr(_RDX, JIT_REXTMP);
	jit_muli_l_(r1, i0);
	MOVQrr(_RDX, _RAX);
	MOVQrr(JIT_REXTMP, _RDX);
    }
    else {
	MOVQrr(_RDX, JIT_REXTMP);
	jit_pushr_l(_RAX);
	jit_muli_l_(r1, i0);
	MOVQrr(_RDX, r0);
	jit_popr_l(_RAX);
	MOVQrr(JIT_REXTMP, _RDX);
    }
}

#define jit_mulr_l_(r0, r1)		x86_mulr_l_(_jit, r0, r1)
__jit_inline void
x86_mulr_l_(jit_state_t _jit, jit_gpr_t r0, jit_gpr_t r1)
{
    if (r1 == _RAX)
	IMULQr(r0);
    else if (r0 == _RAX)
	IMULQr(r1);
    else {
	MOVQrr(r1, _RAX);
	IMULQr(r0);
    }
}

#define jit_hmulr_l(r0, r1, r2)		x86_hmulr_l(_jit, r0, r1, r2)
__jit_inline void
x86_hmulr_l(jit_state_t _jit, jit_gpr_t r0, jit_gpr_t r1, jit_gpr_t r2)
{
    if (r0 == _RDX) {
	MOVQrr(_RAX, JIT_REXTMP);
	jit_mulr_l_(r1, r2);
	MOVQrr(JIT_REXTMP, _RAX);
    }
    else if (r0 == _RAX) {
	MOVQrr(_RDX, JIT_REXTMP);
	jit_mulr_l_(r1, r2);
	MOVQrr(_RDX, _RAX);
	MOVQrr(JIT_REXTMP, _RDX);
    }
    else {
	MOVQrr(_RDX, JIT_REXTMP);
	jit_pushr_l(_RAX);
	jit_mulr_l_(r1, r2);
	MOVQrr(_RDX, r0);
	jit_popr_l(_RAX);
	MOVQrr(JIT_REXTMP, _RDX);
    }
}

/*  Instruction format is:
 *	mul reg64/mem64
 *  and the result is stored in %rdx:%rax
 *  %rax = low 64 bits
 *  %rdx = high 64 bits
 */
#define jit_muli_ul_(r0, i0)		x86_muli_ul_(_jit, r0, i0)
__jit_inline void
x86_muli_ul_(jit_state_t _jit, jit_gpr_t r0, unsigned long i0)
{
    if (r0 == _RAX) {
	jit_movi_ul(_RDX, i0);
	MULQr(_RDX);
    }
    else {
	jit_movi_ul(_RAX, i0);
	MULQr(r0);
    }
}

#define jit_hmuli_ul(r0, r1, i0)	x86_hmuli_ul(_jit, r0, r1, i0)
__jit_inline void
x86_hmuli_ul(jit_state_t _jit, jit_gpr_t r0, jit_gpr_t r1, unsigned long i0)
{
    if (r0 == _RDX) {
	MOVQrr(_RAX, JIT_REXTMP);
	jit_muli_ul_(r1, i0);
	MOVQrr(JIT_REXTMP, _RAX);
    }
    else if (r0 == _RAX) {
	MOVQrr(_RDX, JIT_REXTMP);
	jit_muli_ul_(r1, i0);
	MOVQrr(_RDX, _RAX);
	MOVQrr(JIT_REXTMP, _RDX);
    }
    else {
	MOVQrr(_RDX, JIT_REXTMP);
	jit_pushr_l(_RAX);
	jit_muli_ul_(r1, i0);
	MOVQrr(_RDX, r0);
	jit_popr_l(_RAX);
	MOVQrr(JIT_REXTMP, _RDX);
    }
}

#define jit_mulr_ul_(r0, r1)		x86_mulr_ul_(_jit, r0, r1)
__jit_inline void
x86_mulr_ul_(jit_state_t _jit, jit_gpr_t r0, jit_gpr_t r1)
{
    if (r1 == _RAX)
	MULQr(r0);
    else if (r0 == _RAX)
	MULQr(r1);
    else {
	MOVQrr(r1, _RAX);
	MULQr(r0);
    }
}

#define jit_hmulr_ul(r0, r1, r2)	x86_hmulr_ul(_jit, r0, r1, r2)
__jit_inline void
x86_hmulr_ul(jit_state_t _jit, jit_gpr_t r0, jit_gpr_t r1, jit_gpr_t r2)
{
    if (r0 == _RDX) {
	MOVQrr(_RAX, JIT_REXTMP);
	jit_mulr_ul_(r1, r2);
	MOVQrr(JIT_REXTMP, _RAX);
    }
    else if (r0 == _RAX) {
	MOVQrr(_RDX, JIT_REXTMP);
	jit_mulr_ul_(r1, r2);
	MOVQrr(_RDX, _RAX);
	MOVQrr(JIT_REXTMP, _RDX);
    }
    else {
	MOVQrr(_RDX, JIT_REXTMP);
	jit_pushr_l(_RAX);
	jit_mulr_ul_(r1, r2);
	MOVQrr(_RDX, r0);
	jit_popr_l(_RAX);
	MOVQrr(JIT_REXTMP, _RDX);
    }
}

#define jit_divi_l_(r0, r1, i0, i1, i2)	x86_divi_l_(_jit, r0, r1, i0, i1, i2)
__jit_inline void
x86_divi_l_(jit_state_t _jit, jit_gpr_t r0, jit_gpr_t r1, long i0,
	    int sign, int divide)
{
    jit_gpr_t	div;

    if (divide) {
	switch (i0) {
	    case 1:
		jit_movr_l(r0, r1);
		return;
	    case -1:
		if (sign) {
		    jit_negr_l(r0, r1);
		    return;
		}
		break;
	    default:
		if (i0 > 0 && !(i0 & (i0 - 1))) {
		    jit_movr_l(r0, r1);
		    _ROTSHIQir(sign ? X86_SAR : X86_SHR, ffsl(i0) - 1, r0);
		    return;
		}
		break;
	}
    }
    else if (i0 == 1 || (sign && i0 == -1)) {
	XORLrr(r0, r0);
	return;
    }
    else if (!sign && i0 > 0 && !(i0 & (i0 - 1))) {
	if (jit_can_sign_extend_int_p(i0)) {
	    jit_movr_l(r0, r1);
	    ANDQir(i0 - 1, r0);
	}
	else if (r0 != r1) {
	    MOVQir(i0 - 1, r0);
	    ANDQrr(r1, r0);
	}
	else {
	    MOVQir(i0 - 1, JIT_REXTMP);
	    ANDQrr(JIT_REXTMP, r0);
	}
	return;
    }

    if (r0 == _RAX) {
	jit_pushr_l(_RDX);
	div = JIT_REXTMP;
    }
    else if (r0 == _RDX) {
	jit_pushr_l(_RAX);
	div = JIT_REXTMP;
    }
    else if (r0 == r1) {
	jit_pushr_l(_RDX);
	jit_pushr_l(_RAX);
	div = JIT_REXTMP;
    }
    else {
	jit_pushr_l(_RDX);
	MOVQrr(_RAX, JIT_REXTMP);
	div = r0;
    }

    MOVQir(i0, div);
    jit_movr_l(_RAX, r1);

    if (sign) {
	CQO_();
	IDIVQr(div);
    }
    else {
	XORQrr(_RDX, _RDX);
	DIVQr(div);
    }

    if (r0 != _RAX) {
	if (divide)
	    MOVQrr(_RAX, r0);
	if (div == JIT_REXTMP)
	    jit_popr_l(_RAX);
	else
	    MOVQrr(JIT_REXTMP, _RAX);
    }
    if (r0 != _RDX) {
	if (!divide)
	    MOVQrr(_RDX, r0);
	jit_popr_l(_RDX);
    }
}

#define jit_divr_l_(r0, r1, r2, i0, i1)	x86_divr_l_(_jit, r0, r1, r2, i0, i1)
__jit_inline void
x86_divr_l_(jit_state_t _jit, jit_gpr_t r0, jit_gpr_t r1, jit_gpr_t r2,
	     int sign, int divide)
{
    jit_gpr_t	div;

    if (r0 != _RDX)
	jit_pushr_l(_RDX);
    if (r0 != _RAX)
	jit_pushr_l(_RAX);

    if (r2 == _RAX) {
	if (r0 == _RAX || r0 == _RDX) {
	    div = JIT_REXTMP;
	    MOVQrr(_RAX, div);
	    if (r1 != _RAX)
		MOVQrr(r1, _RAX);
	}
	else {
	    if (r0 == r1)
		jit_xchgr_l(_RAX, r0);
	    else {
		if (r0 != _RAX)
		    MOVQrr(_RAX, r0);
		if (r1 != _RAX)
		    MOVQrr(r1, _RAX);
	    }
	    div = r0;
	}
    }
    else if (r2 == _RDX) {
	if (r0 == _RAX || r0 == _RDX) {
	    div = JIT_REXTMP;
	    MOVQrr(_RDX, div);
	    if (r1 != _RAX)
		MOVQrr(r1, _RAX);
	}
	else {
	    if (r1 != _RAX)
		MOVQrr(r1, _RAX);
	    MOVQrr(_RDX, r0);
	    div = r0;
	}
    }
    else {
	if (r1 != _RAX)
	    MOVQrr(r1, _RAX);
	div = r2;
    }

    if (sign) {
	CQO_();
	IDIVQr(div);
    }
    else {
	XORLrr(_RDX, _RDX);
	DIVQr(div);
    }

    if (r0 != _RAX) {
	if (divide)
	    MOVQrr(_RAX, r0);
	jit_popr_l(_RAX);
    }
    if (r0 != _RDX) {
	if (!divide)
	    MOVQrr(_RDX, r0);
	jit_popr_l(_RDX);
    }
}

#define jit_divi_l(r0, r1, i0)		x86_divi_l(_jit, r0, r1, i0)
__jit_inline void
x86_divi_l(jit_state_t _jit, jit_gpr_t r0, jit_gpr_t r1, long i0)
{
    jit_divi_l_(r0, r1, i0, 1, 1);
}

#define jit_divr_l(r0, r1, r2)		x86_divr_l(_jit, r0, r1, r2)
__jit_inline void
x86_divr_l(jit_state_t _jit, jit_gpr_t r0, jit_gpr_t r1, jit_gpr_t r2)
{
    jit_divr_l_(r0, r1, r2, 1, 1);
}

#define jit_divi_ul(r0, r1, i0)		x86_divi_ul(_jit, r0, r1, i0)
__jit_inline void
x86_divi_ul(jit_state_t _jit, jit_gpr_t r0, jit_gpr_t r1, unsigned long i0)
{
    jit_divi_l_(r0, r1, i0, 0, 1);
}

#define jit_divr_ul(r0, r1, r2)		x86_divr_ul(_jit, r0, r1, r2)
__jit_inline void
x86_divr_ul(jit_state_t _jit, jit_gpr_t r0, jit_gpr_t r1, jit_gpr_t r2)
{
    jit_divr_l_(r0, r1, r2, 0, 1);
}

#define jit_modi_l(r0, r1, i0)		x86_modi_l(_jit, r0, r1, i0)
__jit_inline void
x86_modi_l(jit_state_t _jit, jit_gpr_t r0, jit_gpr_t r1, long i0)
{
    jit_divi_l_(r0, r1, i0, 1, 0);
}

#define jit_modr_l(r0, r1, r2)		x86_modr_l(_jit, r0, r1, r2)
__jit_inline void
x86_modr_l(jit_state_t _jit, jit_gpr_t r0, jit_gpr_t r1, jit_gpr_t r2)
{
    jit_divr_l_(r0, r1, r2, 1, 0);
}

#define jit_modi_ul(r0, r1, i0)		x86_modi_ul(_jit, r0, r1, i0)
__jit_inline void
x86_modi_ul(jit_state_t _jit, jit_gpr_t r0, jit_gpr_t r1, unsigned long i0)
{
    jit_divi_l_(r0, r1, i0, 0, 0);
}

#define jit_modr_ul(r0, r1, r2)		x86_modr_ul(_jit, r0, r1, r2)
__jit_inline void
x86_modr_ul(jit_state_t _jit, jit_gpr_t r0, jit_gpr_t r1, jit_gpr_t r2)
{
    jit_divr_l_(r0, r1, r2, 0, 0);
}

/*  Instruction format is:
 *  <shift> %r0 %r1
 *	%r0 <shift>= %r1
 *  only %cl can be used as %r1
 */
#define jit_shift64(r0, r1, r2, i0)	x86_shift64(_jit, r0, r1, r2, i0)
__jit_inline void
x86_shift64(jit_state_t _jit, jit_gpr_t r0, jit_gpr_t r1, jit_gpr_t r2,
	    x86_rotsh_t cc)
{
    if (r0 == _RCX) {
	MOVQrr(r1, JIT_REXTMP);
	if (r2 != _RCX)
	    MOVBrr(r2, _RCX);
	_ROTSHIQrr(cc, _RCX, JIT_REXTMP);
	MOVQrr(JIT_REXTMP, _RCX);
    }
    else if (r2 != _RCX) {
	MOVQrr(_RCX, JIT_REXTMP);
	MOVBrr(r2, _RCX);
	jit_movr_l(r0, r1);
	_ROTSHIQrr(cc, _RCX, r0);
	MOVQrr(JIT_REXTMP, _RCX);
    }
    else {
	jit_movr_l(r0, r1);
	_ROTSHIQrr(cc, _RCX, r0);
    }
}

#define jit_lshi_l(r0, r1, i0)		x86_lshi_l(_jit, r0, r1, i0)
__jit_inline void
x86_lshi_l(jit_state_t _jit, jit_gpr_t r0, jit_gpr_t r1, unsigned char i0)
{
    if (i0 == 0)
	jit_movr_l(r0, r1);
    else if (i0 <= 3)
	LEAQmr(0, _NOREG, r1, i0 == 1 ? _SCL2 : i0 == 2 ? _SCL4 : _SCL8, r0);
    else {
	jit_movr_l(r0, r1);
	SHLQir(i0, r0);
    }
}

#define jit_lshr_l(r0, r1, r2)		x86_lshr_l(_jit, r0, r1, r2)
__jit_inline void
x86_lshr_l(jit_state_t _jit, jit_gpr_t r0, jit_gpr_t r1, jit_gpr_t r2)
{
    jit_shift64(r0, r1, r2, X86_SHL);
}

#define jit_rshi_l(r0, r1, i0)		x86_rshi_l(_jit, r0, r1, i0)
__jit_inline void
x86_rshi_l(jit_state_t _jit, jit_gpr_t r0, jit_gpr_t r1, unsigned char i0)
{
    jit_movr_l(r0, r1);
    if (i0)
	SARQir(i0, r0);
}

#define jit_rshr_l(r0, r1, r2)		x86_rshr_l(_jit, r0, r1, r2)
__jit_inline void
x86_rshr_l(jit_state_t _jit, jit_gpr_t r0, jit_gpr_t r1, jit_gpr_t r2)
{
    jit_shift64(r0, r1, r2, X86_SAR);
}

#define jit_rshi_ul(r0, r1, i0)		x86_rshi_ul(_jit, r0, r1, i0)
__jit_inline void
x86_rshi_ul(jit_state_t _jit, jit_gpr_t r0, jit_gpr_t r1, unsigned char i0)
{
    jit_movr_l(r0, r1);
    if (i0)
	SHRQir(i0, r0);
}

#define jit_rshr_ul(r0, r1, r2)		x86_rshr_ul(_jit, r0, r1, r2)
__jit_inline void
x86_rshr_ul(jit_state_t _jit, jit_gpr_t r0, jit_gpr_t r1, jit_gpr_t r2)
{
    jit_shift64(r0, r1, r2, X86_SHR);
}

/* Boolean */
#define jit_cmp_ri64(r0, r1, i0, i1)	x86_cmp_ri64(_jit, r0, r1, i0, i1)
__jit_inline void
x86_cmp_ri64(jit_state_t _jit, jit_gpr_t r0, jit_gpr_t r1, long i0, x86_cc_t cc)
{
    int		same = r0 == r1;

    if (!same)
	/* XORLrr is cheaper */
	XORLrr(r0, r0);
    if (jit_can_sign_extend_int_p(i0))
	CMPQir(i0, r1);
    else {
	MOVQir(i0, JIT_REXTMP);
	CMPQrr(JIT_REXTMP, r1);
    }
    if (same)
	/* MOVLir is cheaper */
	MOVLir(0, r0);
    SETCCir(cc, r0);
}

#define jit_test_r64(r0, r1, i0)	x86_test_r64(_jit, r0, r1, i0)
__jit_inline void
x86_test_r64(jit_state_t _jit, jit_gpr_t r0, jit_gpr_t r1, x86_cc_t cc)
{
    int		same = r0 == r1;

    if (!same)
	XORLrr(r0, r0);
    TESTQrr(r1, r1);
    if (same)
	MOVLir(0, r0);
    SETCCir(cc, r0);
}

#define jit_cmp_rr64(r0, r1, r2, i0)	x86_cmp_rr64(_jit, r0, r1, r2, i0)
__jit_inline void
x86_cmp_rr64(jit_state_t _jit, jit_gpr_t r0, jit_gpr_t r1, jit_gpr_t r2,
	     x86_cc_t cc)
{
    int		same = r0 == r1 || r0 == r2;

    if (!same)
	XORLrr(r0, r0);
    CMPQrr(r2, r1);
    if (same)
	MOVLir(0, r0);
    SETCCir(cc, r0);
}

#define jit_lti_l(r0, r1, i0)		x86_lti_l(_jit, r0, r1, i0)
__jit_inline void
x86_lti_l(jit_state_t _jit, jit_gpr_t r0, jit_gpr_t r1, long i0)
{
    if (i0)
	jit_cmp_ri64(r0, r1, i0,	X86_CC_L);
    else
	jit_test_r64(r0, r1,		X86_CC_S);
}

#define jit_ltr_l(r0, r1, r2)		x86_ltr_l(_jit, r0, r1, r2)
__jit_inline void
x86_ltr_l(jit_state_t _jit, jit_gpr_t r0, jit_gpr_t r1, jit_gpr_t r2)
{
    jit_cmp_rr64(r0, r1, r2,		X86_CC_L);
}

#define jit_lei_l(r0, r1, i0)		x86_lei_l(_jit, r0, r1, i0)
__jit_inline void
x86_lei_l(jit_state_t _jit, jit_gpr_t r0, jit_gpr_t r1, long i0)
{
    jit_cmp_ri64(r0, r1, i0,		X86_CC_LE);
}

#define jit_ler_l(r0, r1, r2)		x86_ler_l(_jit, r0, r1, r2)
__jit_inline void
x86_ler_l(jit_state_t _jit, jit_gpr_t r0, jit_gpr_t r1, jit_gpr_t r2)
{
    if (r1 == r2)
	MOVLir(1, r0);
    else
	jit_cmp_rr64(r0, r1, r2,	X86_CC_LE);
}

#define jit_eqi_l(r0, r1, i0)		x86_eqi_l(_jit, r0, r1, i0)
__jit_inline void
x86_eqi_l(jit_state_t _jit, jit_gpr_t r0, jit_gpr_t r1, long i0)
{
    if (i0)
	jit_cmp_ri64(r0, r1, i0,	X86_CC_E);
    else
	jit_test_r64(r0, r1,		X86_CC_E);
}

#define jit_eqr_l(r0, r1, r2)		x86_eqr_l(_jit, r0, r1, r2)
__jit_inline void
x86_eqr_l(jit_state_t _jit, jit_gpr_t r0, jit_gpr_t r1, jit_gpr_t r2)
{
    if (r1 == r2)
	MOVLir(1, r0);
    else
	jit_cmp_rr64(r0, r1, r2,	X86_CC_E);
}

#define jit_gei_l(r0, r1, i0)		x86_gei_l(_jit, r0, r1, i0)
__jit_inline void
x86_gei_l(jit_state_t _jit, jit_gpr_t r0, jit_gpr_t r1, long i0)
{
    if (i0)
	jit_cmp_ri64(r0, r1, i0,	X86_CC_GE);
    else
	jit_test_r64(r0, r1,		X86_CC_NS);
}

#define jit_ger_l(r0, r1, r2)		x86_ger_l(_jit, r0, r1, r2)
__jit_inline void
x86_ger_l(jit_state_t _jit, jit_gpr_t r0, jit_gpr_t r1, jit_gpr_t r2)
{
    if (r1 == r2)
	MOVLir(1, r0);
    else
	jit_cmp_rr64(r0, r1, r2,	X86_CC_GE);
}

#define jit_gti_l(r0, r1, i0)		x86_gti_l(_jit, r0, r1, i0)
__jit_inline void
x86_gti_l(jit_state_t _jit, jit_gpr_t r0, jit_gpr_t r1, long i0)
{
    jit_cmp_ri64(r0, r1, i0,		X86_CC_G);
}

#define jit_gtr_l(r0, r1, r2)		x86_gtr_l(_jit, r0, r1, r2)
__jit_inline void
x86_gtr_l(jit_state_t _jit, jit_gpr_t r0, jit_gpr_t r1, jit_gpr_t r2)
{
    jit_cmp_rr64(r0, r1, r2,		X86_CC_G);
}

#define jit_nei_l(r0, r1, i0)		x86_nei_l(_jit, r0, r1, i0)
__jit_inline void
x86_nei_l(jit_state_t _jit, jit_gpr_t r0, jit_gpr_t r1, long i0)
{
    if (i0)
	jit_cmp_ri64(r0, r1, i0,	X86_CC_NE);
    else
	jit_test_r64(r0, r1,		X86_CC_NE);
}

#define jit_ner_l(r0, r1, r2)		x86_ner_l(_jit, r0, r1, r2)
__jit_inline void
x86_ner_l(jit_state_t _jit, jit_gpr_t r0, jit_gpr_t r1, jit_gpr_t r2)
{
    if (r1 == r2)
	XORLrr(r0, r0);
    else
	jit_cmp_rr64(r0, r1, r2,	X86_CC_NE);
}

#define jit_lti_ul(r0, r1, i0)		x86_lti_ul(_jit, r0, r1, i0)
__jit_inline void
x86_lti_ul(jit_state_t _jit, jit_gpr_t r0, jit_gpr_t r1, unsigned long i0)
{
    jit_cmp_ri64(r0, r1, i0,		X86_CC_B);
}

#define jit_ltr_ul(r0, r1, r2)		x86_ltr_ul(_jit, r0, r1, r2)
__jit_inline void
x86_ltr_ul(jit_state_t _jit, jit_gpr_t r0, jit_gpr_t r1, jit_gpr_t r2)
{
    jit_cmp_rr64(r0, r1, r2,		X86_CC_B);
}

#define jit_lei_ul(r0, r1, i0)		x86_lei_ul(_jit, r0, r1, i0)
__jit_inline void
x86_lei_ul(jit_state_t _jit,
	   jit_gpr_t r0, jit_gpr_t r1, unsigned long i0)
{
    if (i0)
	jit_cmp_ri64(r0, r1, i0,	X86_CC_BE);
    else
	jit_test_r64(r0, r1,		X86_CC_E);
}

#define jit_ler_ul(r0, r1, r2)		x86_ler_ul(_jit, r0, r1, r2)
__jit_inline void
x86_ler_ul(jit_state_t _jit, jit_gpr_t r0, jit_gpr_t r1, jit_gpr_t r2)
{
    if (r1 == r2)
	MOVLir(1, r0);
    else
	jit_cmp_rr64(r0, r1, r2,	X86_CC_BE);
}

#define jit_gei_ul(r0, r1, i0)		x86_gei_ul(_jit, r0, r1, i0)
__jit_inline void
x86_gei_ul(jit_state_t _jit, jit_gpr_t r0, jit_gpr_t r1, unsigned long i0)
{
    if (i0)
	jit_cmp_ri64(r0, r1, i0,	X86_CC_AE);
    else
	jit_test_r64(r0, r1,		X86_CC_NB);
}

#define jit_ger_ul(r0, r1, r2)		x86_ger_ul(_jit, r0, r1, r2)
__jit_inline void
x86_ger_ul(jit_state_t _jit, jit_gpr_t r0, jit_gpr_t r1, jit_gpr_t r2)
{
    if (r1 == r2)
	MOVLir(1, r0);
    else
	jit_cmp_rr64(r0, r1, r2,	X86_CC_AE);
}

#define jit_gti_ul(r0, r1, i0)		x86_gti_ul(_jit, r0, r1, i0)
__jit_inline void
x86_gti_ul(jit_state_t _jit, jit_gpr_t r0, jit_gpr_t r1, unsigned long i0)
{
    if (i0)
	jit_cmp_ri64(r0, r1, i0,	X86_CC_A);
    else
	jit_test_r64(r0, r1,		X86_CC_NE);
}

#define jit_gtr_ul(r0, r1, r2)		x86_gtr_ul(_jit, r0, r1, r2)
__jit_inline void
x86_gtr_ul(jit_state_t _jit, jit_gpr_t r0, jit_gpr_t r1, jit_gpr_t r2)
{
    jit_cmp_rr64(r0, r1, r2,		X86_CC_A);
}

/* Jump */
#define jit_bcmp_ri64(i0, r0, i1, i2)	x86_bcmp_ri64(_jit, i0, r0, i1, i2)
__jit_inline void
x86_bcmp_ri64(jit_state_t _jit,
	      jit_insn *label, jit_gpr_t r0, long i0, x86_cc_t cc)
{
    if (jit_can_sign_extend_int_p(i0))
	CMPQir(i0, r0);
    else {
	MOVQir(i0, JIT_REXTMP);
	CMPQrr(JIT_REXTMP, r0);
    }
    JCCim(cc, label);
}

#define jit_btest_r64(i0, r0, i1)	x86_btest_r64(_jit, i0, r0, i1)
__jit_inline void
x86_btest_r64(jit_state_t _jit, jit_insn *label, jit_gpr_t r0, x86_cc_t cc)
{
    TESTQrr(r0, r0);
    JCCim(cc, label);
}

#define jit_bcmp_rr64(i0, r0, r1, i1)	x86_bcmp_rr64(_jit, i0, r0, r1, i1)
__jit_inline void
x86_bcmp_rr64(jit_state_t _jit, jit_insn *label, jit_gpr_t r0, jit_gpr_t r1,
	      x86_cc_t cc)
{
    CMPQrr(r1, r0);
    JCCim(cc, label);
}

#define jit_blti_l(label, r0, i0)	x86_blti_l(_jit, label, r0, i0)
__jit_inline jit_insn *
x86_blti_l(jit_state_t _jit, jit_insn *label, jit_gpr_t r0, long i0)
{
    if (i0)
	jit_bcmp_ri64(label, r0, i0,	X86_CC_L);
    else
	jit_btest_r64(label, r0,	X86_CC_S);
    return (_jit->x.pc);
}

#define jit_bltr_l(label, r0, r1)	x86_bltr_l(_jit, label, r0, r1)
__jit_inline jit_insn *
x86_bltr_l(jit_state_t _jit, jit_insn *label, jit_gpr_t r0, jit_gpr_t r1)
{
    jit_bcmp_rr64(label, r0, r1,	X86_CC_L);
    return (_jit->x.pc);
}

#define jit_blei_l(label, r0, i0)	x86_blei_l(_jit, label, r0, i0)
__jit_inline jit_insn *
x86_blei_l(jit_state_t _jit, jit_insn *label, jit_gpr_t r0, long i0)
{
    jit_bcmp_ri64(label, r0, i0,	X86_CC_LE);
    return (_jit->x.pc);
}

#define jit_bler_l(label, r0, r1)	x86_bler_l(_jit, label, r0, r1)
__jit_inline jit_insn *
x86_bler_l(jit_state_t _jit, jit_insn *label, jit_gpr_t r0, jit_gpr_t r1)
{
    if (r0 == r1)
	JMPm(label);
    else
	jit_bcmp_rr64(label, r0, r1,	X86_CC_LE);
    return (_jit->x.pc);
}

#define jit_beqi_l(label, r0, i0)	x86_beqi_l(_jit, label, r0, i0)
__jit_inline jit_insn *
x86_beqi_l(jit_state_t _jit, jit_insn *label, jit_gpr_t r0, long i0)
{
    if (i0)
	jit_bcmp_ri64(label, r0, i0,	X86_CC_E);
    else
	jit_btest_r64(label, r0,	X86_CC_E);
    return (_jit->x.pc);
}

#define jit_beqr_l(label, r0, r1)	x86_beqr_l(_jit, label, r0, r1)
__jit_inline jit_insn *
x86_beqr_l(jit_state_t _jit, jit_insn *label, jit_gpr_t r0, jit_gpr_t r1)
{
    if (r0 == r1)
	JMPm(label);
    else
	jit_bcmp_rr64(label, r0, r1,	X86_CC_E);
    return (_jit->x.pc);
}

#define jit_bgei_l(label, r0, i0)	x86_bgei_l(_jit, label, r0, i0)
__jit_inline jit_insn *
x86_bgei_l(jit_state_t _jit, jit_insn *label, jit_gpr_t r0, long i0)
{
    if (i0)
	jit_bcmp_ri64(label, r0, i0,	X86_CC_GE);
    else
	jit_btest_r64(label, r0,	X86_CC_NS);
    return (_jit->x.pc);
}

#define jit_bger_l(label, r0, r1)	x86_bger_l(_jit, label, r0, r1)
__jit_inline jit_insn *
x86_bger_l(jit_state_t _jit, jit_insn *label, jit_gpr_t r0, jit_gpr_t r1)
{
    if (r0 == r1)
	JMPm(label);
    else
	jit_bcmp_rr64(label, r0, r1,	X86_CC_GE);
    return (_jit->x.pc);
}

#define jit_bgti_l(label, r0, i0)	x86_bgti_l(_jit, label, r0, i0)
__jit_inline jit_insn *
x86_bgti_l(jit_state_t _jit, jit_insn *label, jit_gpr_t r0, long i0)
{
    jit_bcmp_ri64(label, r0, i0,	X86_CC_G);
    return (_jit->x.pc);
}

#define jit_bgtr_l(label, r0, r1)	x86_bgtr_l(_jit, label, r0, r1)
__jit_inline jit_insn *
x86_bgtr_l(jit_state_t _jit, jit_insn *label, jit_gpr_t r0, jit_gpr_t r1)
{
    jit_bcmp_rr64(label, r0, r1,	X86_CC_G);
    return (_jit->x.pc);
}

#define jit_bnei_l(label, r0, i0)	x86_bnei_l(_jit, label, r0, i0)
__jit_inline jit_insn *
x86_bnei_l(jit_state_t _jit, jit_insn *label, jit_gpr_t r0, long i0)
{
    if (i0)
	jit_bcmp_ri64(label, r0, i0,	X86_CC_NE);
    else
	jit_btest_r64(label, r0,	X86_CC_NE);
    return (_jit->x.pc);
}

#define jit_bner_l(label, r0, r1)	x86_bner_l(_jit, label, r0, r1)
__jit_inline jit_insn *
x86_bner_l(jit_state_t _jit, jit_insn *label, jit_gpr_t r0, jit_gpr_t r1)
{
    /* need to return a patchable address even if r0 == r1 */
    jit_bcmp_rr64(label, r0, r1,	X86_CC_NE);
    return (_jit->x.pc);
}

#define jit_blti_ul(label, r0, i0)	x86_blti_ul(_jit, label, r0, i0)
__jit_inline jit_insn *
x86_blti_ul(jit_state_t _jit, jit_insn *label, jit_gpr_t r0, unsigned long i0)
{
    /* need to return a patchable address even if i0 == 0 */
    jit_bcmp_ri64(label, r0, i0,	X86_CC_B);
    return (_jit->x.pc);
}

#define jit_bltr_ul(label, r0, r1)	x86_bltr_ul(_jit, label, r0, r1)
__jit_inline jit_insn *
x86_bltr_ul(jit_state_t _jit, jit_insn *label, jit_gpr_t r0, jit_gpr_t r1)
{
    jit_bcmp_rr64(label, r0, r1,	X86_CC_B);
    return (_jit->x.pc);
}

#define jit_blei_ul(label, r0, i0)	x86_blei_ul(_jit, label, r0, i0)
__jit_inline jit_insn *
x86_blei_ul(jit_state_t _jit, jit_insn *label, jit_gpr_t r0, unsigned long i0)
{
    /* need to return a patchable address even if i0 == 0 */
    jit_bcmp_ri64(label, r0, i0,	X86_CC_BE);
    return (_jit->x.pc);
}

#define jit_bler_ul(label, r0, r1)	x86_bler_ul(_jit, label, r0, r1)
__jit_inline jit_insn *
x86_bler_ul(jit_state_t _jit, jit_insn *label, jit_gpr_t r0, jit_gpr_t r1)
{
    if (r0 == r1)
	JMPm(label);
    else
	jit_bcmp_rr64(label, r0, r1,	X86_CC_BE);
    return (_jit->x.pc);
}

#define jit_bgei_ul(label, r0, i0)	x86_bgei_ul(_jit, label, r0, i0)
__jit_inline jit_insn *
x86_bgei_ul(jit_state_t _jit, jit_insn *label, jit_gpr_t r0, unsigned long i0)
{
    if (i0 == 0)
	JMPm(label);
    else
	jit_bcmp_ri64(label, r0, i0,	X86_CC_AE);
    return (_jit->x.pc);
}

#define jit_bger_ul(label, r0, r1)	x86_bger_ul(_jit, label, r0, r1)
__jit_inline jit_insn *
x86_bger_ul(jit_state_t _jit, jit_insn *label, jit_gpr_t r0, jit_gpr_t r1)
{
    if (r0 == r1)
	JMPm(label);
    else
	jit_bcmp_rr64(label, r0, r1,	X86_CC_AE);
    return (_jit->x.pc);
}

#define jit_bgti_ul(label, r0, i0)	x86_bgti_ul(_jit, label, r0, i0)
__jit_inline jit_insn *
x86_bgti_ul(jit_state_t _jit, jit_insn *label, jit_gpr_t r0, unsigned long i0)
{
    if (i0)
	jit_bcmp_ri64(label, r0, i0,	X86_CC_A);
    else
	jit_btest_r64(label, r0,	X86_CC_NE);
    return (_jit->x.pc);
}

#define jit_bgtr_ul(label, r0, r1)	x86_bgtr_ul(_jit, label, r0, r1)
__jit_inline jit_insn *
x86_bgtr_ul(jit_state_t _jit, jit_insn *label, jit_gpr_t r0, jit_gpr_t r1)
{
    jit_bcmp_rr64(label, r0, r1,	X86_CC_A);
    return (_jit->x.pc);
}

#define jit_boaddi_l(label, r0, i0)	x86_boaddi_l(_jit, label, r0, i0)
__jit_inline jit_insn *
x86_boaddi_l(jit_state_t _jit,
	     jit_insn *label, jit_gpr_t r0, long i0)
{
    if (jit_can_sign_extend_int_p(i0))
	ADDQir(i0, r0);
    else {
	MOVQir(i0, JIT_REXTMP);
	ADDQrr(JIT_REXTMP, r0);
    }
    JOm(label);
    return (_jit->x.pc);
}

#define jit_boaddr_l(label, r0, r1)	x86_boaddr_l(_jit, label, r0, r1)
__jit_inline jit_insn *
x86_boaddr_l(jit_state_t _jit, jit_insn *label, jit_gpr_t r0, jit_gpr_t r1)
{
    ADDQrr(r1, r0);
    JOm(label);
    return (_jit->x.pc);
}

#define jit_bosubi_l(label, r0, i0)	x86_bosubi_l(_jit, label, r0, i0)
__jit_inline jit_insn *
x86_bosubi_l(jit_state_t _jit, jit_insn *label, jit_gpr_t r0, long i0)
{
    if (jit_can_sign_extend_int_p(i0))
	SUBQir(i0, r0);
    else {
	MOVQir(i0, JIT_REXTMP);
	SUBQrr(JIT_REXTMP, r0);
    }
    JOm(label);
    return (_jit->x.pc);
}

#define jit_bosubr_l(label, r0, r1)	x86_bosubr_l(_jit, label, r0, r1)
__jit_inline jit_insn *
x86_bosubr_l(jit_state_t _jit, jit_insn *label, jit_gpr_t r0, jit_gpr_t r1)
{
    SUBQrr(r1, r0);
    JOm(label);
    return (_jit->x.pc);
}

#define jit_boaddi_ul(label, r0, i0)	x86_boaddi_ul(_jit, label, r0, i0)
__jit_inline jit_insn *
x86_boaddi_ul(jit_state_t _jit, jit_insn *label, jit_gpr_t r0, unsigned long i0)
{
    if (jit_can_sign_extend_int_p(i0))
	ADDQir(i0, r0);
    else {
	MOVQir(i0, JIT_REXTMP);
	ADDQrr(JIT_REXTMP, r0);
    }
    JCm(label);
    return (_jit->x.pc);
}

#define jit_boaddr_ul(label, r0, r1)	x86_boaddr_ul(_jit, label, r0, r1)
__jit_inline jit_insn *
x86_boaddr_ul(jit_state_t _jit, jit_insn *label, jit_gpr_t r0, jit_gpr_t r1)
{
    ADDQrr(r1, r0);
    JCm(label);
    return (_jit->x.pc);
}

#define jit_bosubi_ul(label, r0, i0)	x86_bosubi_ul(_jit, label, r0, i0)
__jit_inline jit_insn *
x86_bosubi_ul(jit_state_t _jit, jit_insn *label, jit_gpr_t r0, unsigned long i0)
{
    if (jit_can_sign_extend_int_p(i0))
	SUBQir(i0, r0);
    else {
	MOVQir(i0, JIT_REXTMP);
	SUBQrr(JIT_REXTMP, r0);
    }
    JCm(label);
    return (_jit->x.pc);
}

#define jit_bosubr_ul(label, r0, r1)	x86_bosubr_ul(_jit, label, r0, r1)
__jit_inline jit_insn *
x86_bosubr_ul(jit_state_t _jit, jit_insn *label, jit_gpr_t r0, jit_gpr_t r1)
{
    SUBQrr(r1, r0);
    JCm(label);
    return (_jit->x.pc);
}

#define jit_bmsi_l(label, r0, i0)	x86_bmsi_l(_jit, label, r0, i0)
__jit_inline jit_insn *
x86_bmsi_l(jit_state_t _jit, jit_insn *label, jit_gpr_t r0, long i0)
{
    if (jit_can_zero_extend_char_p(i0))
	TESTBir(i0, r0);
    else if (jit_can_zero_extend_short_p(i0))
	TESTWir(i0, r0);
    else if (jit_can_sign_extend_int_p(i0))
	TESTLir(i0, r0);
    else {
	MOVQir(i0, JIT_REXTMP);
	TESTQrr(JIT_REXTMP, r0);
    }
    JNZm(label);
    return (_jit->x.pc);
}

#define jit_bmsr_l(label, r0, r1)	x86_bmsr_l(_jit, label, r0, r1)
__jit_inline jit_insn *
x86_bmsr_l(jit_state_t _jit, jit_insn *label, jit_gpr_t r0, jit_gpr_t r1)
{
    TESTQrr(r1, r0);
    JNZm(label);
    return (_jit->x.pc);
}

#define jit_bmci_l(label, r0, i0)	x86_bmci_l(_jit, label, r0, i0)
__jit_inline jit_insn *
x86_bmci_l(jit_state_t _jit, jit_insn *label, jit_gpr_t r0, long i0)
{
    if (jit_can_zero_extend_char_p(i0))
	TESTBir(i0, r0);
    else if (jit_can_zero_extend_short_p(i0))
	TESTWir(i0, r0);
    else if (jit_can_zero_extend_int_p(i0))
	TESTLir(i0, r0);
    else if (jit_can_sign_extend_int_p(i0))
	TESTQir(i0, r0);
    else {
	MOVQir(i0, JIT_REXTMP);
	TESTQrr(JIT_REXTMP, r0);
    }
    JZm(label);
    return (_jit->x.pc);
}

#define jit_bmcr_l(label, r0, r1)	x86_bmcr_l(_jit, label, r0, r1)
__jit_inline jit_insn *
x86_bmcr_l(jit_state_t _jit, jit_insn *label, jit_gpr_t r0, jit_gpr_t r1)
{
    TESTQrr(r1, r0);
    JZm(label);
    return (_jit->x.pc);
}

/* Memory */
#define jit_ntoh_ul(r0, r1)		x86_ntoh_ul(_jit, r0, r1)
__jit_inline void
x86_ntoh_ul(jit_state_t _jit, jit_gpr_t r0, jit_gpr_t r1)
{
    jit_movr_l(r0, r1);
    BSWAPQr(r0);
}

#define jit_ldr_c(r0, r1)		x86_ldr_c(_jit, r0, r1)
__jit_inline void
x86_ldr_c(jit_state_t _jit, jit_gpr_t r0, jit_gpr_t r1)
{
    MOVSBQmr(0, r1, _NOREG,  _SCL1, r0);
}

#define jit_ldxr_c(r0, r1, r2)		x86_ldxr_c(_jit, r0, r1, r2)
__jit_inline void
x86_ldxr_c(jit_state_t _jit, jit_gpr_t r0, jit_gpr_t r1, jit_gpr_t r2)
{
    MOVSBQmr(0, r1, r2, _SCL1, r0);
}

#define jit_ldr_s(r0, r1)		x86_ldr_s(_jit, r0, r1)
__jit_inline void
x86_ldr_s(jit_state_t _jit, jit_gpr_t r0, jit_gpr_t r1)
{
    MOVSWQmr(0, r1, _NOREG, _SCL1, r0);
}

#define jit_ldxr_s(r0, r1, r2)		x86_ldxr_s(_jit, r0, r1, r2)
__jit_inline void
x86_ldxr_s(jit_state_t _jit, jit_gpr_t r0, jit_gpr_t r1, jit_gpr_t r2)
{
    MOVSWQmr(0, r1, r2, _SCL1, r0);
}

#define jit_ldi_c(r0, i0)		x86_ldi_c(_jit, r0, i0)
__jit_inline void
x86_ldi_c(jit_state_t _jit, jit_gpr_t r0, void *i0)
{
    if (jit_can_sign_extend_int_p((long)i0))
	MOVSBQmr((long)i0, _NOREG, _NOREG, _SCL1, r0);
    else {
	MOVQir((long)i0, JIT_REXTMP);
	jit_ldr_c(r0, JIT_REXTMP);
    }
}

#define jit_ldxi_c(r0, r1, i0)		x86_ldxi_c(_jit, r0, r1, i0)
__jit_inline void
x86_ldxi_c(jit_state_t _jit, jit_gpr_t r0, jit_gpr_t r1, long i0)
{
    if (jit_can_sign_extend_int_p(i0))
	MOVSBQmr(i0, r1, _NOREG,  _SCL1, r0);
    else {
	MOVQir(i0, JIT_REXTMP);
	jit_ldxr_c(r0, r1, JIT_REXTMP);
    }
}

#define jit_ldi_uc(r0, i0)		x86_ldi_uc(_jit, r0, i0)
__jit_inline void
x86_ldi_uc(jit_state_t _jit, jit_gpr_t r0, void *i0)
{
    if (jit_can_sign_extend_int_p((long)i0))
	MOVZBLmr((long)i0, _NOREG, _NOREG, _SCL1, r0);
    else {
	MOVQir((long)i0, JIT_REXTMP);
	jit_ldr_uc(r0, JIT_REXTMP);
    }
}

#define jit_ldxi_uc(r0, r1, i0)		x86_ldxi_uc(_jit, r0, r1, i0)
__jit_inline void
x86_ldxi_uc(jit_state_t _jit, jit_gpr_t r0, jit_gpr_t r1, long i0)
{
    if (jit_can_sign_extend_int_p(i0))
	MOVZBLmr(i0, r1, _NOREG, _SCL1, r0);
    else {
	MOVQir(i0, JIT_REXTMP);
	jit_ldxr_uc(r0, r1, JIT_REXTMP);
    }
}

#define jit_str_c(r0, r1)		x86_str_c(_jit, r0, r1)
__jit_inline void
x86_str_c(jit_state_t _jit, jit_gpr_t r0, jit_gpr_t r1)
{
    MOVBrm(r1, 0, r0, _NOREG, _SCL1);
}

#define jit_sti_c(i0, r0)		x86_sti_c(_jit, i0, r0)
__jit_inline void
x86_sti_c(jit_state_t _jit, void *i0, jit_gpr_t r0)
{
    if (jit_can_sign_extend_int_p((long)i0))
	MOVBrm(r0, (long)i0, _NOREG, _NOREG, _SCL1);
    else {
	MOVQir((long)i0, JIT_REXTMP);
	jit_str_c(JIT_REXTMP, r0);
    }
}

#define jit_stxr_c(r0, r1, r2)		x86_stxr_c(_jit, r0, r1, r2)
__jit_inline void
x86_stxr_c(jit_state_t _jit, jit_gpr_t r0, jit_gpr_t r1, jit_gpr_t r2)
{
    MOVBrm(r2, 0, r0, r1, _SCL1);
}

#define jit_stxi_c(i0, r0, r1)		x86_stxi_c(_jit, i0, r0, r1)
__jit_inline void
x86_stxi_c(jit_state_t _jit, long i0, jit_gpr_t r0, jit_gpr_t r1)
{
    if (jit_can_sign_extend_int_p(i0))
	MOVBrm(r1, i0, r0, _NOREG, _SCL1);
    else {
	MOVQir(i0, JIT_REXTMP);
	jit_stxr_c(JIT_REXTMP, r0, r1);
    }
}

#define jit_ldi_s(r0, i0)		x86_ldi_s(_jit, r0, i0)
__jit_inline void
x86_ldi_s(jit_state_t _jit, jit_gpr_t r0, void *i0)
{
    if (jit_can_sign_extend_int_p((long)i0))
	MOVSWQmr((long)i0, _NOREG, _NOREG, _SCL1, r0);
    else {
	MOVQir((long)i0, JIT_REXTMP);
	jit_ldr_s(r0, JIT_REXTMP);
    }
}

#define jit_ldxi_s(r0, r1, i0)		x86_ldxi_s(_jit, r0, r1, i0)
__jit_inline void
x86_ldxi_s(jit_state_t _jit, jit_gpr_t r0, jit_gpr_t r1, long i0)
{
    if (jit_can_sign_extend_int_p(i0))
	MOVSWQmr(i0, r1, _NOREG, _SCL1, r0);
    else {
	MOVQir(i0, JIT_REXTMP);
	jit_ldxr_s(r0, r1, JIT_REXTMP);
    }
}

#define jit_ldi_us(r0, i0)		x86_ldi_us(_jit, r0, i0)
__jit_inline void
x86_ldi_us(jit_state_t _jit, jit_gpr_t r0, void *i0)
{
    if (jit_can_sign_extend_int_p((long)i0))
	MOVZWLmr((long)i0, _NOREG, _NOREG, _SCL1, r0);
    else {
	MOVQir((long)i0, JIT_REXTMP);
	jit_ldr_us(r0, JIT_REXTMP);
    }
}

#define jit_ldxi_us(r0, r1, i0)		x86_ldxi_us(_jit, r0, r1, i0)
__jit_inline void
x86_ldxi_us(jit_state_t _jit, jit_gpr_t r0, jit_gpr_t r1, long i0)
{
    if (jit_can_sign_extend_int_p(i0))
	MOVZWLmr(i0, r1, _NOREG, _SCL1, r0);
    else  {
	MOVQir(i0, JIT_REXTMP);
	jit_ldxr_us(r0, r1, JIT_REXTMP);
    }
}

#define jit_sti_s(i0, r0)		x86_sti_s(_jit, i0, r0)
__jit_inline void
x86_sti_s(jit_state_t _jit, void *i0, jit_gpr_t r0)
{
    if (jit_can_sign_extend_int_p((long)i0))
	MOVWrm(r0, (long)i0, _NOREG, _NOREG, _SCL1);
    else {
	MOVQir((long)i0, JIT_REXTMP);
	jit_str_s(JIT_REXTMP, r0);
    }
}

#define jit_stxi_s(i0, r0, r1)		x86_stxi_s(_jit, i0, r0, r1)
__jit_inline void
x86_stxi_s(jit_state_t _jit, long i0, jit_gpr_t r0, jit_gpr_t r1)
{
    if (jit_can_sign_extend_int_p(i0))
	MOVWrm(r1, i0, r0, _NOREG, _SCL1);
    else {
	MOVQir(i0, JIT_REXTMP);
	jit_stxr_s(JIT_REXTMP, r0, r1);
    }
}

#define jit_ldr_i(r0, r1)		x86_ldr_i(_jit, r0, r1)
__jit_inline void
x86_ldr_i(jit_state_t _jit, jit_gpr_t r0, jit_gpr_t r1)
{
    MOVSLQmr(0, r1, _NOREG, _SCL1, r0);
}

#define jit_ldi_i(r0, i0)		x86_ldi_i(_jit, r0, i0)
__jit_inline void
x86_ldi_i(jit_state_t _jit, jit_gpr_t r0, void *i0)
{
    if (jit_can_sign_extend_int_p((long)i0))
	MOVSLQmr((long)i0, _NOREG, _NOREG, _SCL1, r0);
    else {
	MOVQir((long)i0, JIT_REXTMP);
	jit_ldr_i(r0, JIT_REXTMP);
    }
}

#define jit_ldxr_i(r0, r1, r2)		x86_ldxr_i(_jit, r0, r1, r2)
__jit_inline void
x86_ldxr_i(jit_state_t _jit, jit_gpr_t r0, jit_gpr_t r1, jit_gpr_t r2)
{
    MOVSLQmr(0, r1, r2, _SCL1, r0);
}

#define jit_ldxi_i(r0, r1, i0)		x86_ldxi_i(_jit, r0, r1, i0)
__jit_inline void
x86_ldxi_i(jit_state_t _jit, jit_gpr_t r0, jit_gpr_t r1, long i0)
{
    if (jit_can_sign_extend_int_p(i0))
	MOVSLQmr(i0, r1, _NOREG, _SCL1, r0);
    else {
	MOVQir(i0, JIT_REXTMP);
	jit_ldxr_i(r0, r1, JIT_REXTMP);
    }
}

#define jit_ldr_ui(r0, r1)		x86_ldr_ui(_jit, r0, r1)
__jit_inline void
x86_ldr_ui(jit_state_t _jit, jit_gpr_t r0, jit_gpr_t r1)
{
    MOVLmr(0, r1, _NOREG, _SCL1, r0);
}

#define jit_ldi_ui(r0, i0)		x86_ldi_ui(_jit, r0, i0)
__jit_inline void
x86_ldi_ui(jit_state_t _jit, jit_gpr_t r0, void *i0)
{
    if (jit_can_sign_extend_int_p((long)i0))
	MOVLmr((long)i0, _NOREG, _NOREG, _SCL1, r0);
    else {
	MOVQir((long)i0, JIT_REXTMP);
	jit_ldr_ui(r0, JIT_REXTMP);
    }
}

#define jit_ldxr_ui(r0, r1, r2)		x86_ldxr_ui(_jit, r0, r1, r2)
__jit_inline void
x86_ldxr_ui(jit_state_t _jit, jit_gpr_t r0, jit_gpr_t r1, jit_gpr_t r2)
{
    MOVLmr(0, r1, r2, _SCL1, r0);
}

#define jit_ldxi_ui(r0, r1, i0)		x86_ldxi_ui(_jit, r0, r1, i0)
__jit_inline void
x86_ldxi_ui(jit_state_t _jit, jit_gpr_t r0, jit_gpr_t r1, long i0)
{
    if (jit_can_sign_extend_int_p(i0))
	MOVLmr(i0, r1, _NOREG, _SCL1, r0);
    else  {
	MOVQir(i0, JIT_REXTMP);
	jit_ldxr_ui(r0, r1, JIT_REXTMP);
    }
}

#define jit_sti_i(i0, r0)		x86_sti_i(_jit, i0, r0)
__jit_inline void
x86_sti_i(jit_state_t _jit, void *i0, jit_gpr_t r0)
{
    if (jit_can_sign_extend_int_p((long)i0))
	MOVLrm(r0, (long)i0, _NOREG, _NOREG, _SCL1);
    else {
	MOVQir((long)i0, JIT_REXTMP);
	jit_str_i(JIT_REXTMP, r0);
    }
}

#define jit_stxi_i(i0, r0, r1)		x86_stxi_i(_jit, i0, r0, r1)
__jit_inline void
x86_stxi_i(jit_state_t _jit, long i0, jit_gpr_t r0, jit_gpr_t r1)
{
    if (jit_can_sign_extend_int_p(i0))
	MOVLrm(r1, i0, r0, _NOREG, _SCL1);
    else {
	MOVQir(i0, JIT_REXTMP);
	jit_stxr_i(JIT_REXTMP, r0, r1);
    }
}

#define jit_ldr_l(r0, r1)		x86_ldr_l(_jit, r0, r1)
__jit_inline void
x86_ldr_l(jit_state_t _jit, jit_gpr_t r0, jit_gpr_t r1)
{
    MOVQmr(0, r1, _NOREG, _SCL1, r0);
}

#define jit_ldi_l(r0, i0)		x86_ldi_l(_jit, r0, i0)
__jit_inline void
x86_ldi_l(jit_state_t _jit, jit_gpr_t r0, void *i0)
{
    if (jit_can_sign_extend_int_p((long)i0))
	MOVQmr((long)i0, _NOREG, _NOREG, _SCL1, r0);
    else {
	MOVQir((long)i0, JIT_REXTMP);
	jit_ldr_l(r0, JIT_REXTMP);
    }
}

#define jit_ldxr_l(r0, r1, r2)		x86_ldxr_l(_jit, r0, r1, r2)
__jit_inline void
x86_ldxr_l(jit_state_t _jit, jit_gpr_t r0, jit_gpr_t r1, jit_gpr_t r2)
{
    MOVQmr(0, r1, r2, _SCL1, r0);
}

#define jit_ldxi_l(r0, r1, i0)		x86_ldxi_l(_jit, r0, r1, i0)
#define jit_ldxi_ul(r0, r1, i0)		x86_ldxi_l(_jit, r0, r1, i0)
#define jit_ldxi_p(r0, r1, i0)		x86_ldxi_l(_jit, r0, r1, i0)
__jit_inline void
x86_ldxi_l(jit_state_t _jit, jit_gpr_t r0, jit_gpr_t r1, long i0)
{
    if (jit_can_sign_extend_int_p(i0))
	MOVQmr(i0, r1, _NOREG, _SCL1, r0);
    else  {
	MOVQir(i0, JIT_REXTMP);
	jit_ldxr_l(r0, r1, JIT_REXTMP);
    }
}

#define jit_str_l(r0, r1)		x86_str_l(_jit, r0, r1)
__jit_inline void
x86_str_l(jit_state_t _jit, jit_gpr_t r0, jit_gpr_t r1)
{
    MOVQrm(r1, 0, r0, _NOREG, _SCL1);
}

#define jit_sti_l(i0, r0)		x86_sti_l(_jit, i0, r0)
__jit_inline void
x86_sti_l(jit_state_t _jit, void *i0, jit_gpr_t r0)
{
    if (jit_can_sign_extend_int_p((long)i0))
	MOVQrm(r0, (long)i0, _NOREG, _NOREG, _SCL1);
    else {
	MOVQir((long)i0, JIT_REXTMP);
	jit_str_l(JIT_REXTMP, r0);
    }
}

#define jit_stxr_l(r0, r1, r2)		x86_stxr_l(_jit, r0, r1, r2)
__jit_inline void
x86_stxr_l(jit_state_t _jit, jit_gpr_t r0, jit_gpr_t r1, jit_gpr_t r2)
{
    MOVQrm(r2, 0, r0, r1, _SCL1);
}

#define jit_stxi_l(i0, r0, r1)		x86_stxi_l(_jit, i0, r0, r1)
__jit_inline void
x86_stxi_l(jit_state_t _jit, long i0, jit_gpr_t r0, jit_gpr_t r1)
{
    if (jit_can_sign_extend_int_p(i0))
	MOVQrm(r1, i0, r0, _NOREG, _SCL1);
    else {
	MOVQir(i0, JIT_REXTMP);
	jit_stxr_l(JIT_REXTMP, r0, r1);
    }
}

#define jit_extr_c_l(r0, r1)		x86_extr_c_l(_jit, r0, r1)
__jit_inline void
x86_extr_c_l(jit_state_t _jit, jit_gpr_t r0, jit_gpr_t r1)
{
    MOVSBQrr(r1, r0);
}

#define jit_extr_c_ul(r0, r1)		x86_extr_c_ul(_jit, r0, r1)
__jit_inline void
x86_extr_c_ul(jit_state_t _jit, jit_gpr_t r0, jit_gpr_t r1)
{
    MOVZBQrr(r1, r0);
}

#define jit_extr_s_l(r0, r1)		x86_extr_s_l(_jit, r0, r1)
__jit_inline void
x86_extr_s_l(jit_state_t _jit, jit_gpr_t r0, jit_gpr_t r1)
{
    MOVSWQrr(r1, r0);
}

#define jit_extr_s_ul(r0, r1)		x86_extr_s_ul(_jit, r0, r1)
__jit_inline void
x86_extr_s_ul(jit_state_t _jit, jit_gpr_t r0, jit_gpr_t r1)
{
    MOVZWQrr(r1, r0);
}

#define jit_extr_s_l(r0, r1)		x86_extr_s_l(_jit, r0, r1)
__jit_inline void
x86_extr_i_l(jit_state_t _jit, jit_gpr_t r0, jit_gpr_t r1)
{
    MOVSLQrr(r1, r0);
}

#define jit_extr_s_ul(r0, r1)		x86_extr_s_ul(_jit, r0, r1)
__jit_inline void
x86_extr_i_ul(jit_state_t _jit, jit_gpr_t r0, jit_gpr_t r1)
{
    MOVLrr(r1, r0);
}

#define jit_finish(i0)			x86_finish(_jit, i0)
__jit_inline jit_insn *
x86_finish(jit_state_t _jit, void *i0)
{
    assert(_jitl.stack_offset	== 0 &&
	   _jitl.nextarg_puti	== 0 &&
	   _jitl.nextarg_putfp	== 0);
    if (_jitl.fprssize) {
	MOVBir(_jitl.fprssize, _RAX);
	_jitl.fprssize = 0;
    }
    else
	MOVBir(0, _RAX);
    jit_calli(i0);
    if (jit_push_pop_p() && _jitl.stack_length) {
	jit_addi_l(JIT_SP, JIT_SP, _jitl.stack_length);
	_jitl.stack_length = 0;
    }

    return (_jitl.label);
}

#define jit_finishr(rs)			x86_finishr(_jit, rs)
__jit_inline void
x86_finishr(jit_state_t _jit, jit_gpr_t r0)
{
    assert(_jitl.stack_offset	== 0 &&
	   _jitl.nextarg_puti	== 0 &&
	   _jitl.nextarg_putfp	== 0);
    if (r0 == _RAX) {
	/* clobbered with # of fp registers (for varargs) */
	MOVQrr(_RAX, JIT_REXTMP);
	r0 = JIT_REXTMP;
    }
    if (_jitl.fprssize) {
	MOVBir(_jitl.fprssize, _RAX);
	_jitl.fprssize = 0;
    }
    else
	MOVBir(0, _RAX);
    jit_callr(r0);
    if (jit_push_pop_p() && _jitl.stack_length) {
	jit_addi_l(JIT_SP, JIT_SP, _jitl.stack_length);
	_jitl.stack_length = 0;
    }
}

#define jit_pusharg_i(r0)		x86_pusharg_i(_jit, r0)
#define jit_pusharg_l(r0)		x86_pusharg_i(_jit, r0)
__jit_inline void
x86_pusharg_i(jit_state_t _jit, jit_gpr_t r0)
{
    assert(_jitl.nextarg_puti > 0);
    if (--_jitl.nextarg_puti >= JIT_ARG_MAX) {
	_jitl.stack_offset -= sizeof(long);
	assert(_jitl.stack_offset >= 0);
	if (jit_push_pop_p()) {
	    int	pad = -_jitl.stack_length & 15;
	    if (pad) {
		jit_subi_l(JIT_SP, JIT_SP, pad + sizeof(long));
		_jitl.stack_length += pad;
		jit_str_l(JIT_SP, r0);
	    }
	    else
		jit_pushr_l(r0);
	}
	else
	    jit_stxi_l(_jitl.stack_offset, JIT_SP, r0);
    }
    else
	jit_movr_l(jit_arg_reg_order[_jitl.nextarg_puti], r0);
}

#define jit_retval_l(r0)		x86_retval_l(_jit, r0)
__jit_inline void
x86_retval_l(jit_state_t _jit, jit_gpr_t r0)
{
    jit_movr_l(r0, _RAX);
}

#define jit_arg_i()			x86_arg_i(_jit)
#define jit_arg_c()			x86_arg_i(_jit)
#define jit_arg_uc()			x86_arg_i(_jit)
#define jit_arg_s()			x86_arg_i(_jit)
#define jit_arg_us()			x86_arg_i(_jit)
#define jit_arg_ui()			x86_arg_i(_jit)
#define jit_arg_l()			x86_arg_i(_jit)
#define jit_arg_ul()			x86_arg_i(_jit)
#define jit_arg_p()			x86_arg_i(_jit)
__jit_inline int
x86_arg_i(jit_state_t _jit)
{
    int		ofs;

    if (_jitl.nextarg_geti < JIT_ARG_MAX) {
	ofs = _jitl.nextarg_geti;
	++_jitl.nextarg_geti;
    }
    else {
	ofs = _jitl.framesize;
	_jitl.framesize += sizeof(long);
    }

    return (ofs);
}

#define jit_getarg_c(r0, ofs)		x86_getarg_c(_jit, r0, ofs)
__jit_inline void
x86_getarg_c(jit_state_t _jit, jit_gpr_t r0, int ofs)
{
    if (ofs < JIT_ARG_MAX)
	jit_extr_c_l(r0, jit_arg_reg_order[ofs]);
    else
	jit_ldxi_c(r0, JIT_FP, ofs);
}

#define jit_getarg_uc(r0, ofs)		x86_getarg_uc(_jit, r0, ofs)
__jit_inline void
x86_getarg_uc(jit_state_t _jit, jit_gpr_t r0, int ofs)
{
    if (ofs < JIT_ARG_MAX)
	jit_extr_c_ul(r0, jit_arg_reg_order[ofs]);
    else
	jit_ldxi_uc(r0, JIT_FP, ofs);
}

#define jit_getarg_s(r0, ofs)		x86_getarg_s(_jit, r0, ofs)
__jit_inline void
x86_getarg_s(jit_state_t _jit, jit_gpr_t r0, int ofs)
{
    if (ofs < JIT_ARG_MAX)
	jit_extr_s_l(r0, jit_arg_reg_order[ofs]);
    else
	jit_ldxi_s(r0, JIT_FP, ofs);
}

#define jit_getarg_us(r0, ofs)		x86_getarg_us(_jit, r0, ofs)
__jit_inline void
x86_getarg_us(jit_state_t _jit, jit_gpr_t r0, int ofs)
{
    if (ofs < JIT_ARG_MAX)
	jit_extr_s_ul(r0, jit_arg_reg_order[ofs]);
    else
	jit_ldxi_us(r0, JIT_FP, ofs);
}

#define jit_getarg_i(r0, ofs)		x86_getarg_i(_jit, r0, ofs)
__jit_inline void
x86_getarg_i(jit_state_t _jit, jit_gpr_t r0, int ofs)
{
    if (ofs < JIT_ARG_MAX)
	jit_movr_l(r0, jit_arg_reg_order[ofs]);
    else
	jit_ldxi_i(r0, JIT_FP, ofs);
}

#define jit_getarg_ui(r0, ofs)		x86_getarg_ui(_jit, r0, ofs)
__jit_inline void
x86_getarg_ui(jit_state_t _jit, jit_gpr_t r0, int ofs)
{
    if (ofs < JIT_ARG_MAX)
	jit_movr_ul(r0, jit_arg_reg_order[ofs]);
    else
	jit_ldxi_ui(r0, JIT_FP, ofs);
}

#define jit_getarg_l(r0, ofs)		x86_getarg_l(_jit, r0, ofs)
__jit_inline void
x86_getarg_l(jit_state_t _jit, jit_gpr_t r0, int ofs)
{
    if (ofs < JIT_ARG_MAX)
	jit_movr_l(r0, jit_arg_reg_order[ofs]);
    else
	jit_ldxi_l(r0, JIT_FP, ofs);
}

#define jit_getarg_ul(r0, ofs)		x86_getarg_ul(_jit, r0, ofs)
__jit_inline void
x86_getarg_ul(jit_state_t _jit, jit_gpr_t r0, int ofs)
{
    if (ofs < JIT_ARG_MAX)
	jit_movr_ul(r0, jit_arg_reg_order[ofs]);
    else
	jit_ldxi_ul(r0, JIT_FP, ofs);
}

#define jit_getarg_p(r0, ofs)		x86_getarg_p(_jit, r0, ofs)
__jit_inline void
x86_getarg_p(jit_state_t _jit, jit_gpr_t r0, int ofs)
{
    if (ofs < JIT_ARG_MAX)
	jit_movr_p(r0, jit_arg_reg_order[ofs]);
    else
	jit_ldxi_p(r0, JIT_FP, ofs);
}

#endif /* __lightning_core_h */
