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



#ifndef __lightning_core_i386_h
#define __lightning_core_i386_h

#include "asm.h"

/* ffs* prototype */
#if !defined(__GNU_SOURCE)
#  define __GNU_SOURCE			1
#  define __GNU_SOURCE_NOT_DEFINED	1
#endif
#include <string.h>
#if __GNU_SOURCE_NOT_DEFINED
#  undef __GNU_SOURCE
#endif
#ifdef _MSC_VER
#include <intrin.h>
#pragma intrinsic(_BitScanForward)
__jit_inline int ffsl(long x)
{
	unsigned long i;
	if (_BitScanForward(&i, x))
		return i + 1;

	return 0;
}

__jit_inline int ffs(int x)
{
	return ffsl(x);
}
#endif

#define JIT_FP			_RBP
#define JIT_SP			_RSP
#define JIT_RET			_RAX

#define jit_push_pop_p()		jit_flags.push_pop

#if __WORDSIZE == 64
#define jit_can_zero_extend_char_p(im)					\
    ((im) >= 0 && (im) <= 0x80)

#define jit_can_sign_extend_char_p(im)					\
    (((im) >= 0 && (im) <=  0x7f) ||					\
     ((im) <  0 && (im) >= -0x80))

#define jit_can_zero_extend_short_p(im)					\
    ((im) >= 0 && (im) <= 0x8000)

#define jit_can_sign_extend_short_p(im)					\
    (((im) >= 0 && (im) <=  0x7fff) ||					\
     ((im) <  0 && (im) >= -0x8000))

#define jit_can_zero_extend_int_p(im)					\
    ((im) >= 0 && (im) <= 0x80000000L)

#define jit_can_sign_extend_int_p(im)					\
    (((im) >= 0 && (im) <=  0x7fffffffL) ||				\
     ((im) <  0 && (im) >= -0x80000000L))
#endif

#define jit_movr_i(r0, r1)		x86_movr_i(_jit, r0, r1)
#define jit_pushr_i(r0)			x86_pushr_i(_jit, r0)
#define jit_pushi_i(i0)			x86_pushi_i(_jit, i0)
#define jit_popr_i(r0)			x86_popr_i(_jit, r0)
#define jit_xchgr_i(r0, r1)		x86_xchgr_i(_jit, r0, r1)
#if __WORDSIZE == 32
__jit_inline void
x86_movr_i(jit_state_t _jit, jit_gpr_t r0, jit_gpr_t r1)
{
    if (r0 != r1)
	MOVLrr(r1, r0);
}

__jit_inline void
x86_pushr_i(jit_state_t _jit, jit_gpr_t r0)
{
    PUSHLr(r0);
}

__jit_inline void
x86_pushi_i(jit_state_t _jit, int i0)
{
    PUSHLi(i0);
}

__jit_inline void
x86_popr_i(jit_state_t _jit, jit_gpr_t r0)
{
    POPLr(r0);
}

__jit_inline void
x86_xchgr_i(jit_state_t _jit, jit_gpr_t r0, jit_gpr_t r1)
{
    XCHGLrr(r0, r1);
}
#else
#  define jit_movr_l(r0, r1)		x86_movr_i(_jit, r0, r1)
#  define jit_movr_ul(r0, r1)		x86_movr_i(_jit, r0, r1)
#  define jit_movr_p(r0, r1)		x86_movr_i(_jit, r0, r1)
__jit_inline void
x86_movr_i(jit_state_t _jit, jit_gpr_t r0, jit_gpr_t r1)
{
    if (r0 != r1)
	MOVQrr(r1, r0);
}

#define jit_pushr_l(r0)			x86_pushr_i(_jit, r0)
__jit_inline void
x86_pushr_i(jit_state_t _jit, jit_gpr_t r0)
{
    PUSHQr(r0);
}

__jit_inline void
x86_pushi_i(jit_state_t _jit, long i0)
{
    PUSHQi(i0);
}

#define jit_popr_l(r0)			x86_popr_i(_jit, r0)
__jit_inline void
x86_popr_i(jit_state_t _jit, jit_gpr_t r0)
{
    POPQr(r0);
}

#define jit_xchgr_l(r0, r1)		x86_xchgr_i(_jit, r0, r1)
__jit_inline void
x86_xchgr_i(jit_state_t _jit, jit_gpr_t r0, jit_gpr_t r1)
{
    XCHGQrr(r0, r1);
}
#endif	/* __WORDSIZE == 32 */

#define jit_patch_abs_long_at(jump, label)				\
	x86_patch_abs_long_at(_jit, jump, label)
__jit_inline void
x86_patch_abs_long_at(jit_state_t _jit, jit_insn *jump, jit_insn *label)
{
    *(long *)(jump - sizeof(long)) = (long)label;
}

#define jit_patch_rel_int_at(jump, label)				\
	x86_patch_rel_int_at(_jit, jump, label)
__jit_inline void
x86_patch_rel_int_at(jit_state_t _jit, jit_insn *jump, jit_insn *label)
{
    *(int *)(jump - sizeof(int)) = (int)(long)(label - jump);
}

#define jit_patch_rel_char_at(jump, label)				\
	x86_patch_rel_char_at(_jit, jump, label)
__jit_inline void
x86_patch_rel_char_at(jit_state_t _jit, jit_insn *jump, jit_insn *label)
{
    *(char *)(jump - 1) = (char)(long)(label - jump);
}

#define jit_patch_movi(address, label)	x86_patch_movi(_jit, address, label)
__jit_inline void
x86_patch_movi(jit_state_t _jit, jit_insn *address, jit_insn *label)
{
    jit_patch_abs_long_at(address, label);
}

#define jit_jmpi(label)			x86_jmpi(_jit, label)
__jit_inline jit_insn *
x86_jmpi(jit_state_t _jit, jit_insn *label)
{
    JMPm(label);
    return (_jit->x.pc);
}

#define jit_jmpr(r0)			x86_jmpr(_jit, r0)
__jit_inline void
x86_jmpr(jit_state_t _jit, jit_gpr_t r0)
{
    JMPsr(r0);
}

/* Stack */
#define jit_retval_i(r0)		x86_retval_i(_jit, r0)
__jit_inline void
x86_retval_i(jit_state_t _jit, jit_gpr_t r0)
{
    jit_movr_i(r0, _RAX);
}

/* ALU */
#define jit_negr_i(r0, r1)		x86_negr_i(_jit, r0, r1)
__jit_inline void
x86_negr_i(jit_state_t _jit, jit_gpr_t r0, jit_gpr_t r1)
{
    if (r0 == r1)
	NEGLr(r0);
    else {
	XORLrr(r0, r0);
	SUBLrr(r1, r0);
    }
}

#define jit_notr_i(r0, r1)		x86_notr_i(_jit, r0, r1)
__jit_inline void
x86_notr_i(jit_state_t _jit, jit_gpr_t r0, jit_gpr_t r1)
{
    jit_movr_i(r0, r1);
    NOTLr(r0);
}

#define jit_addi_i(r0, r1, i0)		x86_addi_i(_jit, r0, r1, i0)
__jit_inline void
x86_addi_i(jit_state_t _jit, jit_gpr_t r0, jit_gpr_t r1, int i0)
{
    if (i0 == 0)
	jit_movr_i(r0, r1);
    else if (i0 == 1) {
	jit_movr_i(r0, r1);
	INCLr(r0);
    }
    else if (i0 == -1) {
	jit_movr_i(r0, r1);
	DECLr(r0);
    }
    else if (r0 == r1)
	ADDLir(i0, r0);
    else
	LEALmr(i0, r1, _NOREG, _SCL1, r0);
}

#define jit_addr_i(r0, r1, r2)		x86_addr_i(_jit, r0, r1, r2)
__jit_inline void
x86_addr_i(jit_state_t _jit, jit_gpr_t r0, jit_gpr_t r1, jit_gpr_t r2)
{
    if (r0 == r1)
	ADDLrr(r2, r0);
    else if (r0 == r2)
	ADDLrr(r1, r0);
    else
	LEALmr(0, r1, r2, _SCL1, r0);
}

#define jit_subi_i(r0, r1, i0)		x86_subi_i(_jit, r0, r1, i0)
__jit_inline void
x86_subi_i(jit_state_t _jit, jit_gpr_t r0, jit_gpr_t r1, int i0)
{
    if (i0 == 0)
	jit_movr_i(r0, r1);
    else if (i0 == 1) {
	jit_movr_i(r0, r1);
	DECLr(r0);
    }
    else if (i0 == -1) {
	jit_movr_i(r0, r1);
	INCLr(r0);
    }
    else if (r0 == r1)
	SUBLir(i0, r0);
    else
	LEALmr(-i0, r1, _NOREG, _SCL1, r0);
}

#define jit_subr_i(r0, r1, r2)		x86_subr_i(_jit, r0, r1, r2)
__jit_inline void
x86_subr_i(jit_state_t _jit, jit_gpr_t r0, jit_gpr_t r1, jit_gpr_t r2)
{
    if (r1 == r2)
	XORLrr(r0, r0);
    else if (r0 == r2) {
	SUBLrr(r1, r0);
	NEGLr(r0);
    }
    else {
	jit_movr_i(r0, r1);
	SUBLrr(r2, r0);
    }
}

#define jit_addci_ui(r0, r1, i0)	x86_addci_ui(_jit, r0, r1, i0)
__jit_inline void
x86_addci_ui(jit_state_t _jit, jit_gpr_t r0, jit_gpr_t r1, unsigned int i0)
{
    if (r0 == r1)
	ADDLir((int)i0, r0);
    else {
	MOVLir(i0, r0);
	ADDLrr(r1, r0);
    }
}

#define jit_addcr_ui(r0, r1, r2)	x86_addcr_ui(_jit, r0, r1, r2)
__jit_inline void
x86_addcr_ui(jit_state_t _jit, jit_gpr_t r0, jit_gpr_t r1, jit_gpr_t r2)
{
    if (r0 == r2)
	ADDLrr(r1, r0);
    else if (r0 == r1)
	ADDLrr(r2, r0);
    else {
	MOVLrr(r1, r0);
	ADDLrr(r2, r0);
    }
}

#define jit_addxi_ui(r0, r1, i0)	x86_addxi_ui(_jit, r0, r1, i0)
__jit_inline void
x86_addxi_ui(jit_state_t _jit, jit_gpr_t r0, jit_gpr_t r1, unsigned int i0)
{
    if (r0 == r1)
	ADCLir((int)i0, r0);
    else {
	MOVLir(i0, r0);
	ADCLrr(r1, r0);
    }
}

#define jit_addxr_ui(r0, r1, r2)	x86_addxr_ui(_jit, r0, r1, r2)
__jit_inline void
x86_addxr_ui(jit_state_t _jit, jit_gpr_t r0, jit_gpr_t r1, jit_gpr_t r2)
{
    if (r0 == r2)
	ADCLrr(r1, r0);
    else if (r0 == r1)
	ADCLrr(r2, r0);
    else {
	MOVLrr(r1, r0);
	ADCLrr(r2, r0);
    }
}

#define jit_subci_ui(r0, r1, i0)	x86_subci_ui(_jit, r0, r1, i0)
__jit_inline void
x86_subci_ui(jit_state_t _jit, jit_gpr_t r0, jit_gpr_t r1, unsigned int i0)
{
    if (r0 == r1)
	SUBLir((int)i0, r0);
    else {
	MOVLir(i0, r0);
	SUBLrr(r1, r0);
    }
}

#define jit_subcr_ui(r0, r1, r2)	x86_subcr_ui(_jit, r0, r1, r2)
__jit_inline void
x86_subcr_ui(jit_state_t _jit, jit_gpr_t r0, jit_gpr_t r1, jit_gpr_t r2)
{
    if (r0 == r1)
	SUBLrr(r2, r0);
    else if (r0 != r2) {
	MOVLrr(r1, r0);
	SUBLrr(r2, r0);
    }
    else {
	jit_pushr_i(r1);
	jit_xchgr_i(r0, r1);
	SUBLrr(r1, r0);
	jit_popr_i(r1);
    }
}

#define jit_subxi_ui(r0, r1, i0)	x86_subxi_ui(_jit, r0, r1, i0)
__jit_inline void
x86_subxi_ui(jit_state_t _jit, jit_gpr_t r0, jit_gpr_t r1, unsigned int i0)
{
    if (r0 == r1)
	SBBLir((int)i0, r0);
    else {
	MOVLir(i0, r0);
	SBBLrr(r1, r0);
    }
}

#define jit_subxr_ui(r0, r1, r2)	x86_subxr_ui(_jit, r0, r1, r2)
__jit_inline void
x86_subxr_ui(jit_state_t _jit, jit_gpr_t r0, jit_gpr_t r1, jit_gpr_t r2)
{
    if (r0 == r1)
	SBBLrr(r2, r0);
    else if (r0 != r2) {
	MOVLrr(r1, r0);
	SBBLrr(r2, r0);
    }
    else {
	jit_pushr_i(r1);
	jit_xchgr_i(r0, r1);
	SBBLrr(r1, r0);
	jit_popr_i(r1);
    }
}

#define jit_andi_i(r0, r1, i0)		x86_andi_i(_jit, r0, r1, i0)
__jit_inline void
x86_andi_i(jit_state_t _jit, jit_gpr_t r0, jit_gpr_t r1, int i0)
{
    if (i0 == 0)
	XORLrr(r0, r0);
    else {
	jit_movr_i(r0, r1);
	if (i0 != -1)
	    ANDLir(i0, r0);
    }
}

#define jit_andr_i(r0, r1, r2)		x86_andr_i(_jit, r0, r1, r2)
__jit_inline void
x86_andr_i(jit_state_t _jit, jit_gpr_t r0, jit_gpr_t r1, jit_gpr_t r2)
{
    if (r1 == r2)
	jit_movr_i(r0, r1);
    else if (r0 == r1)
	ANDLrr(r2, r0);
    else if (r0 == r2)
	ANDLrr(r1, r0);
    else {
	MOVLrr(r1, r0);
	ANDLrr(r2, r0);
    }
}

#define jit_ori_i(r0, r1, i0)		x86_ori_i(_jit, r0, r1, i0)
__jit_inline void
x86_ori_i(jit_state_t _jit, jit_gpr_t r0, jit_gpr_t r1, int i0)
{
    if (i0 == 0)
	jit_movr_i(r0, r1);
    else if (i0 == -1)
	MOVLir(0xffffffff, r0);
    else {
	jit_movr_i(r0, r1);
	if (jit_reg8_p(r0) && _u8P(i0))
	    ORBir(i0, r0);
#if __WORDSIZE == 32
	else if (_u16P(i0))
	    ORWir(i0, r0);
#endif
	else
	    ORLir(i0, r0);
    }
}

#define jit_orr_i(r0, r1, r2)		x86_orr_i(_jit, r0, r1, r2)
__jit_inline void
x86_orr_i(jit_state_t _jit, jit_gpr_t r0, jit_gpr_t r1, jit_gpr_t r2)
{
    if (r1 == r2)
	jit_movr_i(r0, r1);
    else if (r0 == r1)
	ORLrr(r2, r0);
    else if (r0 == r2)
	ORLrr(r1, r0);
    else {
	MOVLrr(r1, r0);
	ORLrr(r2, r0);
    }
}

#define jit_xori_i(r0, r1, i0)		x86_xori_i(_jit, r0, r1, i0)
__jit_inline void
x86_xori_i(jit_state_t _jit, jit_gpr_t r0, jit_gpr_t r1, int i0)
{
    if (i0 == 0)
	jit_movr_i(r0, r1);
    else if (i0 == -1) {
	jit_movr_i(r0, r1);
	NOTLr(r0);
    }
    else {
	jit_movr_i(r0, r1);
	if (jit_reg8_p(r0) && _u8P(i0))
	    XORBir(i0, r0);
#if __WORDSIZE == 32
	else if (_u16P(i0))
	    XORWir(i0, r0);
#endif
	else
	    XORLir(i0, r0);
    }
}

#define jit_xorr_i(r0, r1, r2)		x86_xorr_i(_jit, r0, r1, r2)
__jit_inline void
x86_xorr_i(jit_state_t _jit, jit_gpr_t r0, jit_gpr_t r1, jit_gpr_t r2)
{
    if (r1 == r2)
	XORLrr(r0, r0);
    else if (r0 == r1)
	XORLrr(r2, r0);
    else if (r0 == r2)
	XORLrr(r1, r0);
    else {
	MOVLrr(r1, r0);
	XORLrr(r2, r0);
    }
}

/*  Instruction format is:
 *	imul reg32/mem32
 *  and the result is stored in %edx:%eax
 *  %eax = low 32 bits
 *  %edx = high 32 bits
 */
#define jit_muli_i_(r0, i0)		x86_muli_i_(_jit, r0, i0)
__jit_inline void
x86_muli_i_(jit_state_t _jit, jit_gpr_t r0, int i0)
{
    if (r0 == _RAX) {
	MOVLir((unsigned)i0, _RDX);
	IMULLr(_RDX);
    }
    else {
	MOVLir((unsigned)i0, _RAX);
	IMULLr(r0);
    }
}

#define jit_hmuli_i(r0, r1, i0)		x86_hmuli_i(_jit, r0, r1, i0)
__jit_inline void
x86_hmuli_i(jit_state_t _jit, jit_gpr_t r0, jit_gpr_t r1, int i0)
{
    if (r0 == _RDX) {
	jit_pushr_i(_RAX);
	jit_muli_i_(r1, i0);
	jit_popr_i(_RAX);
    }
    else if (r0 == _RAX) {
	jit_pushr_i(_RDX);
	jit_muli_i_(r1, i0);
	MOVLrr(_RDX, _RAX);
	jit_popr_i(_RDX);
    }
    else {
	jit_pushr_i(_RDX);
	jit_pushr_i(_RAX);
	jit_muli_i_(r1, i0);
	MOVLrr(_RDX, r0);
	jit_popr_i(_RAX);
	jit_popr_i(_RDX);
    }
}

#define jit_mulr_i_(r0, r1)		x86_mulr_i_(_jit, r0, r1)
__jit_inline void
x86_mulr_i_(jit_state_t _jit, jit_gpr_t r0, jit_gpr_t r1)
{
    if (r1 == _RAX)
	IMULLr(r0);
    else if (r0 == _RAX)
	IMULLr(r1);
    else {
	MOVLrr(r1, _RAX);
	IMULLr(r0);
    }
}

#define jit_hmulr_i(r0, r1, r2)		x86_hmulr_i(_jit, r0, r1, r2)
__jit_inline void
x86_hmulr_i(jit_state_t _jit, jit_gpr_t r0, jit_gpr_t r1, jit_gpr_t r2)
{
    if (r0 == _RDX) {
	jit_pushr_i(_RAX);
	jit_mulr_i_(r1, r2);
	jit_popr_i(_RAX);
    }
    else if (r0 == _RAX) {
	jit_pushr_i(_RDX);
	jit_mulr_i_(r1, r2);
	MOVLrr(_RDX, _RAX);
	jit_popr_i(_RDX);
    }
    else {
	jit_pushr_i(_RDX);
	jit_pushr_i(_RAX);
	jit_mulr_i_(r1, r2);
	MOVLrr(_RDX, r0);
	jit_popr_i(_RAX);
	jit_popr_i(_RDX);
    }
}

/*  Instruction format is:
 *	mul reg32/mem32
 *  and the result is stored in %edx:%eax
 *  %eax = low 32 bits
 *  %edx = high 32 bits
 */
#define jit_muli_ui_(r0, i0)		x86_muli_ui_(_jit, r0, i0)
__jit_inline void
x86_muli_ui_(jit_state_t _jit, jit_gpr_t r0, unsigned int i0)
{
    if (r0 == _RAX) {
	MOVLir(i0, _RDX);
	MULLr(_RDX);
    }
    else {
	MOVLir(i0, _RAX);
	MULLr(r0);
    }
}

#define jit_hmuli_ui(r0, r1, i0)	x86_hmuli_ui(_jit, r0, r1, i0)
__jit_inline void
x86_hmuli_ui(jit_state_t _jit, jit_gpr_t r0, jit_gpr_t r1, unsigned int i0)
{
    if (r0 == _RDX) {
	jit_pushr_i(_RAX);
	jit_muli_ui_(r1, i0);
	jit_popr_i(_RAX);
    }
    else if (r0 == _RAX) {
	jit_pushr_i(_RDX);
	jit_muli_ui_(r1, i0);
	MOVLrr(_RDX, _RAX);
	jit_popr_i(_RDX);
    }
    else {
	jit_pushr_i(_RDX);
	jit_pushr_i(_RAX);
	jit_muli_ui_(r1, i0);
	MOVLrr(_RDX, r0);
	jit_popr_i(_RAX);
	jit_popr_i(_RDX);
    }
}

#define jit_mulr_ui_(r0, r1)		x86_mulr_ui_(_jit, r0, r1)
__jit_inline void
x86_mulr_ui_(jit_state_t _jit, jit_gpr_t r0, jit_gpr_t r1)
{
    if (r1 == _RAX)
	MULLr(r0);
    else if (r0 == _RAX)
	MULLr(r1);
    else {
	MOVLrr(r1, _RAX);
	MULLr(r0);
    }
}

#define jit_hmulr_ui(r0, r1, r2)	x86_hmulr_ui(_jit, r0, r1, r2)
__jit_inline void
x86_hmulr_ui(jit_state_t _jit, jit_gpr_t r0, jit_gpr_t r1, jit_gpr_t r2)
{
    if (r0 == _RDX) {
	jit_pushr_i(_RAX);
	jit_mulr_ui_(r1, r2);
	jit_popr_i(_RAX);
    }
    else if (r0 == _RAX) {
	jit_pushr_i(_RDX);
	jit_mulr_ui_(r1, r2);
	MOVLrr(_RDX, _RAX);
	jit_popr_i(_RDX);
    }
    else {
	jit_pushr_i(_RDX);
	jit_pushr_i(_RAX);
	jit_mulr_ui_(r1, r2);
	MOVLrr(_RDX, r0);
	jit_popr_i(_RAX);
	jit_popr_i(_RDX);
    }
}

#define jit_muli_i(r0, r1, i0)		x86_muli_i(_jit, r0, r1, i0)
#define jit_muli_ui(r0, r1, i0)		x86_muli_i(_jit, r0, r1, i0)
__jit_inline void
x86_muli_i(jit_state_t _jit, jit_gpr_t r0, jit_gpr_t r1, int i0)
{
    switch (i0) {
	case 0:
	    XORLrr(r0, r0);
	    break;
	case 1:
	    jit_movr_i(r0, r1);
	    break;
	case -1:
	    jit_negr_i(r0, r1);
	    break;
	case 2:
	    LEALmr(0, _NOREG, r1, _SCL2, r0);
	    break;
	case 4:
	    LEALmr(0, _NOREG, r1, _SCL4, r0);
	    break;
	case 8:
	    LEALmr(0, _NOREG, r1, _SCL8, r0);
	    break;
	default:
	    if (i0 > 0 && !(i0 & (i0 - 1))) {
		jit_movr_i(r0, r1);
		SHLLir(ffs(i0) - 1, r0);
	    }
	    else
		IMULLirr(i0, r1, r0);
	    break;
    }
}

#define jit_mulr_i(r0, r1, r2)		x86_mulr_i(_jit, r0, r1, r2)
#define jit_mulr_ui(r0, r1, r2)		x86_mulr_i(_jit, r0, r1, r2)
__jit_inline void
x86_mulr_i(jit_state_t _jit, jit_gpr_t r0, jit_gpr_t r1, jit_gpr_t r2)
{
    if (r0 == r1)
	IMULLrr(r2, r0);
    else if (r0 == r2)
	IMULLrr(r1, r0);
    else {
	MOVLrr(r1, r0);
	IMULLrr(r2, r0);
    }
}

#define jit_divi_i_(r0, r1, i0, i1, i2)	x86_divi_i_(_jit, r0, r1, i0, i1, i2)
__jit_inline void
x86_divi_i_(jit_state_t _jit, jit_gpr_t r0, jit_gpr_t r1, int i0,
	    int sign, int divide)
{
    jit_gpr_t	div;
    int		pop;

    if (divide) {
	switch (i0) {
	    case 1:
		jit_movr_i(r0, r1);
		return;
	    case -1:
		if (sign) {
		    jit_negr_i(r0, r1);
		    return;
		}
		break;
	    default:
		if (i0 > 0 && !(i0 & (i0 - 1))) {
		    jit_movr_i(r0, r1);
		    _ROTSHILir(sign ? X86_SAR : X86_SHR, ffs(i0) - 1, r0);
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
	jit_movr_i(r0, r1);
	ANDLir(i0 - 1, r0);
	return;
    }

    if (r0 != _RDX)
	jit_pushr_i(_RDX);
    if (r0 != _RAX)
	jit_pushr_i(_RAX);

    if (r0 == _RAX || r0 == _RDX) {
	div = _RCX;
	pop = 1;
    }
    else {
	div = r0;
	pop = 0;
    }

    if (pop)
	jit_pushr_i(div);
    if (r1 != _RAX)
	MOVLrr(r1, _RAX);
    MOVLir((unsigned)i0, div);

    if (sign) {
	CDQ_();
	IDIVLr(div);
    }
    else {
	XORLrr(_RDX, _RDX);
	DIVLr(div);
    }

    if (pop)
	jit_popr_i(div);

    if (r0 != _RAX) {
	if (divide)
	    MOVLrr(_RAX, r0);
	jit_popr_i(_RAX);
    }
    if (r0 != _RDX) {
	if (!divide)
	    MOVLrr(_RDX, r0);
	jit_popr_i(_RDX);
    }
}

#define jit_divr_i_(r0, r1, r2, i0, i1)	x86_divr_i_(_jit, r0, r1, r2, i0, i1)
__jit_inline void
x86_divr_i_(jit_state_t _jit, jit_gpr_t r0, jit_gpr_t r1, jit_gpr_t r2,
	    int sign, int divide)
{
    jit_gpr_t	div;
    int		pop;

    if (r0 != _RDX)
	jit_pushr_i(_RDX);
    if (r0 != _RAX)
	jit_pushr_i(_RAX);

    if (r2 == _RAX) {
	if (r0 == _RAX || r0 == _RDX) {
	    div = r1 == _RCX ? _RBX : _RCX;
	    jit_pushr_i(div);
	    MOVLrr(_RAX, div);
	    if (r1 != _RAX)
		MOVLrr(r1, _RAX);
	    pop = 1;
	}
	else {
	    if (r0 == r1)
		jit_xchgr_i(_RAX, r0);
	    else {
		if (r0 != _RAX)
		    MOVLrr(_RAX, r0);
		if (r1 != _RAX)
		    MOVLrr(r1, _RAX);
	    }
	    div = r0;
	    pop = 0;
	}
    }
    else if (r2 == _RDX) {
	if (r0 == _RAX || r0 == _RDX) {
	    div = r1 == _RCX ? _RBX : _RCX;
	    jit_pushr_i(div);
	    MOVLrr(_RDX, div);
	    if (r1 != _RAX)
		MOVLrr(r1, _RAX);
	    pop = 1;
	}
	else {
	    if (r1 != _RAX)
		MOVLrr(r1, _RAX);
	    MOVLrr(_RDX, r0);
	    div = r0;
	    pop = 0;
	}
    }
    else {
	if (r1 != _RAX)
	    MOVLrr(r1, _RAX);
	div = r2;
	pop = 0;
    }

    if (sign) {
	CDQ_();
	IDIVLr(div);
    }
    else {
	XORLrr(_RDX, _RDX);
	DIVLr(div);
    }

    if (pop)
	jit_popr_i(div);
    if (r0 != _RAX) {
	if (divide)
	    MOVLrr(_RAX, r0);
	jit_popr_i(_RAX);
    }
    if (r0 != _RDX) {
	if (!divide)
	    MOVLrr(_RDX, r0);
	jit_popr_i(_RDX);
    }
}

#define jit_divi_i(r0, r1, i0)		x86_divi_i(_jit, r0, r1, i0)
__jit_inline void
x86_divi_i(jit_state_t _jit, jit_gpr_t r0, jit_gpr_t r1, int i0)
{
    jit_divi_i_(r0, r1, i0, 1, 1);
}

#define jit_divr_i(r0, r1, r2)		x86_divr_i(_jit, r0, r1, r2)
__jit_inline void
x86_divr_i(jit_state_t _jit, jit_gpr_t r0, jit_gpr_t r1, jit_gpr_t r2)
{
    jit_divr_i_(r0, r1, r2, 1, 1);
}

#define jit_divi_ui(r0, r1, i0)		x86_divi_ui(_jit, r0, r1, i0)
__jit_inline void
x86_divi_ui(jit_state_t _jit, jit_gpr_t r0, jit_gpr_t r1, unsigned int i0)
{
    jit_divi_i_(r0, r1, i0, 0, 1);
}

#define jit_divr_ui(r0, r1, r2)		x86_divr_ui(_jit, r0, r1, r2)
__jit_inline void
x86_divr_ui(jit_state_t _jit, jit_gpr_t r0, jit_gpr_t r1, jit_gpr_t r2)
{
    jit_divr_i_(r0, r1, r2, 0, 1);
}

#define jit_modi_i(r0, r1, i0)		x86_modi_i(_jit, r0, r1, i0)
__jit_inline void
x86_modi_i(jit_state_t _jit, jit_gpr_t r0, jit_gpr_t r1, int i0)
{
    jit_divi_i_(r0, r1, i0, 1, 0);
}

#define jit_modr_i(r0, r1, r2)		x86_modr_i(_jit, r0, r1, r2)
__jit_inline void
x86_modr_i(jit_state_t _jit, jit_gpr_t r0, jit_gpr_t r1, jit_gpr_t r2)
{
    jit_divr_i_(r0, r1, r2, 1, 0);
}

#define jit_modi_ui(r0, r1, i0)		x86_modi_ui(_jit, r0, r1, i0)
__jit_inline void
x86_modi_ui(jit_state_t _jit, jit_gpr_t r0, jit_gpr_t r1, unsigned int i0)
{
    jit_divi_i_(r0, r1, i0, 0, 0);
}

#define jit_modr_ui(r0, r1, r2)		x86_modr_ui(_jit, r0, r1, r2)
__jit_inline void
x86_modr_ui(jit_state_t _jit, jit_gpr_t r0, jit_gpr_t r1, jit_gpr_t r2)
{
    jit_divr_i_(r0, r1, r2, 0, 0);
}

/* Shifts */
#define jit_shift32(r0, r1, r2, i0)	x86_shift32(_jit, r0, r1, r2, i0)
__jit_inline void
x86_shift32(jit_state_t _jit, jit_gpr_t r0, jit_gpr_t r1, jit_gpr_t r2,
	    x86_rotsh_t code)
{
    jit_gpr_t	lsh;

    if (r0 != _RCX && r2 != _RCX)
	jit_pushr_i(_RCX);

    if (r1 == _RCX) {
	if (r0 != _RCX) {
	    if (r0 == r2)
		jit_xchgr_i(_RCX, r0);
	    else {
		MOVLrr(_RCX, r0);
		MOVLrr(r2, _RCX);
	    }
	    lsh = r0;
	}
	/* r0 == _RCX */
	else if (r2 == _RCX) {
	    jit_pushr_i(_RAX);
	    MOVLrr(_RCX, _RAX);
	    lsh = _RAX;
	}
	else {
	    jit_pushr_i(r2);
	    jit_xchgr_i(_RCX, r2);
	    lsh = r2;
	}
    }
    /* r1 != _RCX */
    else if (r0 == _RCX) {
	jit_pushr_i(r1);
	if (r2 != _RCX)
	    MOVLrr(r2, _RCX);
	lsh = r1;
    }
    else {
	if (r2 != _RCX)
	    MOVLrr(r2, _RCX);
	if (r0 != r1)
	    MOVLrr(r1, r0);
	lsh = r0;
    }

    _ROTSHILrr(code, _RCX, lsh);

    if (lsh != r0) {
	MOVLrr(lsh, r0);
	jit_popr_i(lsh);
    }

    if (r0 != _RCX && r2 != _RCX)
	jit_popr_i(_RCX);
}

#define jit_lshi_i(r0, r1, i0)		x86_lshi_i(_jit, r0, r1, i0)
__jit_inline void
x86_lshi_i(jit_state_t _jit, jit_gpr_t r0, jit_gpr_t r1, unsigned char i0)
{
    if (i0 == 0)
	jit_movr_i(r0, r1);
    else if (i0 <= 3)
	LEALmr(0, _NOREG, r1, i0 == 1 ? _SCL2 : i0 == 2 ? _SCL4 : _SCL8, r0);
    else {
	jit_movr_i(r0, r1);
	SHLLir(i0, r0);
    }
}

#define jit_lshr_i(r0, r1, r2)		x86_lshr_i(_jit, r0, r1, r2)
__jit_inline void
x86_lshr_i(jit_state_t _jit, jit_gpr_t r0, jit_gpr_t r1, jit_gpr_t r2)
{
    jit_shift32(r0, r1, r2, X86_SHL);
}

#define jit_rshi_i(r0, r1, i0)		x86_rshi_i(_jit, r0, r1, i0)
__jit_inline void
x86_rshi_i(jit_state_t _jit, jit_gpr_t r0, jit_gpr_t r1, unsigned char i0)
{
    jit_movr_i(r0, r1);
    if (i0)
	SARLir(i0, r0);
}

#define jit_rshr_i(r0, r1, r2)		x86_rshr_i(_jit, r0, r1, r2)
__jit_inline void
x86_rshr_i(jit_state_t _jit, jit_gpr_t r0, jit_gpr_t r1, jit_gpr_t r2)
{
    jit_shift32(r0, r1, r2, X86_SAR);
}

#define jit_rshi_ui(r0, r1, i0)		x86_rshi_ui(_jit, r0, r1, i0)
__jit_inline void
x86_rshi_ui(jit_state_t _jit, jit_gpr_t r0, jit_gpr_t r1, unsigned char i0)
{
    jit_movr_i(r0, r1);
    if (i0)
	SHRLir(i0, r0);
}

#define jit_rshr_ui(r0, r1, r2)		x86_rshr_ui(_jit, r0, r1, r2)
__jit_inline void
x86_rshr_ui(jit_state_t _jit, jit_gpr_t r0, jit_gpr_t r1, jit_gpr_t r2)
{
    jit_shift32(r0, r1, r2, X86_SHR);
}

/* Boolean */
#define jit_cmp_ri32(r0, r1, i0, i1)	x86_cmp_ri32(_jit, r0, r1, i0, i1)
__jit_inline void
x86_cmp_ri32(jit_state_t _jit, jit_gpr_t r0, jit_gpr_t r1, int i0, x86_cc_t cc)
{
    int		op;
    jit_gpr_t	reg;

    op = r0 == r1;
    if (jit_reg8_p(r0)) {
	if (!op)
	    XORLrr(r0, r0);
	CMPLir(i0, r1);
	if (op)
	    MOVLir(0, r0);
	SETCCir(cc, r0);
    }
    else {
	reg = r1 == _RAX ? _RDX : _RAX;
	if (op)
	    jit_pushr_i(reg);
	else
	    MOVLrr(reg, r0);
	XORLrr(reg, reg);
	CMPLir(i0, r1);
	SETCCir(cc, reg);
	if (op) {
	    MOVLrr(reg, r0);
	    jit_popr_i(reg);
	}
	else
	    jit_xchgr_i(reg, r0);
    }
}

#define jit_test_r32(r0, r1, i0)	x86_test_r32(_jit, r0, r1, i0)
__jit_inline void
x86_test_r32(jit_state_t _jit, jit_gpr_t r0, jit_gpr_t r1, x86_cc_t cc)
{
    int		op;
    jit_gpr_t	reg;

    op = r0 == r1;
    if (jit_reg8_p(r0)) {
	if (!op)
	    XORLrr(r0, r0);
	TESTLrr(r1, r1);
	if (op)
	    MOVLir(0, r0);
	SETCCir(cc, r0);
    }
    else {
	reg = r1 == _RAX ? _RDX : _RAX;
	if (op)
	    jit_pushr_i(reg);
	else
	    MOVLrr(reg, r0);
	XORLrr(reg, reg);
	TESTLrr(r1, r1);
	SETCCir(cc, reg);
	if (op) {
	    MOVLrr(reg, r0);
	    jit_popr_i(reg);
	}
	else
	    jit_xchgr_i(reg, r0);
    }
}

#define jit_cmp_rr32(r0, r1, r2, i0)	x86_cmp_rr32(_jit, r0, r1, r2, i0)
__jit_inline void
x86_cmp_rr32(jit_state_t _jit, jit_gpr_t r0, jit_gpr_t r1, jit_gpr_t r2,
	     x86_cc_t cc)
{
    int		op;
    jit_gpr_t	reg;

    op = r0 == r1 || r0 == r2;
    if (jit_reg8_p(r0)) {
	if (!op)
	    XORLrr(r0, r0);
	CMPLrr(r2, r1);
	if (op)
	    MOVLir(0, r0);
	SETCCir(cc, r0);
    }
    else {
	if (r1 == _RAX || r2 == _RAX) {
	    if (r1 == _RDX || r2 == _RDX)
		reg = _RCX;
	    else
		reg = _RDX;
	}
	else
	    reg = _RAX;
	if (op)
	    jit_pushr_i(reg);
	else
	    MOVLrr(reg, r0);
	XORLrr(reg, reg);
	CMPLrr(r2, r1);
	SETCCir(cc, reg);
	if (op) {
	    MOVLrr(reg, r0);
	    jit_popr_i(reg);
	}
	else
	    jit_xchgr_i(reg, r0);
    }
}

#define jit_lti_i(r0, r1, i0)		x86_lti_i(_jit, r0, r1, i0)
__jit_inline void
x86_lti_i(jit_state_t _jit, jit_gpr_t r0, jit_gpr_t r1, int i0)
{
    if (i0)
	jit_cmp_ri32(r0, r1, i0,	X86_CC_L);
    else
	jit_test_r32(r0, r1,		X86_CC_S);
}

#define jit_ltr_i(r0, r1, r2)		x86_ltr_i(_jit, r0, r1, r2)
__jit_inline void
x86_ltr_i(jit_state_t _jit, jit_gpr_t r0, jit_gpr_t r1, jit_gpr_t r2)
{
    jit_cmp_rr32(r0, r1, r2,		X86_CC_L);
}

#define jit_lei_i(r0, r1, i0)		x86_lei_i(_jit, r0, r1, i0)
__jit_inline void
x86_lei_i(jit_state_t _jit, jit_gpr_t r0, jit_gpr_t r1, int i0)
{
    jit_cmp_ri32(r0, r1, i0,		X86_CC_LE);
}

#define jit_ler_i(r0, r1, r2)		x86_ler_i(_jit, r0, r1, r2)
__jit_inline void
x86_ler_i(jit_state_t _jit, jit_gpr_t r0, jit_gpr_t r1, jit_gpr_t r2)
{
    if (r1 == r2)
	MOVLir(1, r0);
    else
	jit_cmp_rr32(r0, r1, r2,	X86_CC_LE);
}

#define jit_eqi_i(r0, r1, i0)		x86_eqi_i(_jit, r0, r1, i0)
__jit_inline void
x86_eqi_i(jit_state_t _jit, jit_gpr_t r0, jit_gpr_t r1, int i0)
{
    if (i0)
	jit_cmp_ri32(r0, r1, i0,	X86_CC_E);
    else
	jit_test_r32(r0, r1,		X86_CC_E);
}

#define jit_eqr_i(r0, r1, r2)		x86_eqr_i(_jit, r0, r1, r2)
__jit_inline void
x86_eqr_i(jit_state_t _jit, jit_gpr_t r0, jit_gpr_t r1, jit_gpr_t r2)
{
    if (r1 == r2)
	MOVLir(1, r0);
    else
	jit_cmp_rr32(r0, r1, r2,	X86_CC_E);
}

#define jit_gei_i(r0, r1, i0)		x86_gei_i(_jit, r0, r1, i0)
__jit_inline void
x86_gei_i(jit_state_t _jit, jit_gpr_t r0, jit_gpr_t r1, int i0)
{
    if (i0)
	jit_cmp_ri32(r0, r1, i0,	X86_CC_GE);
    else
	jit_test_r32(r0, r1,		X86_CC_NS);
}

#define jit_ger_i(r0, r1, r2)		x86_ger_i(_jit, r0, r1, r2)
__jit_inline void
x86_ger_i(jit_state_t _jit, jit_gpr_t r0, jit_gpr_t r1, jit_gpr_t r2)
{
    if (r1 == r2)
	MOVLir(1, r0);
    else
	jit_cmp_rr32(r0, r1, r2,	X86_CC_GE);
}

#define jit_gti_i(r0, r1, i0)		x86_gti_i(_jit, r0, r1, i0)
__jit_inline void
x86_gti_i(jit_state_t _jit, jit_gpr_t r0, jit_gpr_t r1, int i0)
{
    jit_cmp_ri32(r0, r1, i0,		X86_CC_G);
}

#define jit_gtr_i(r0, r1, r2)		x86_gtr_i(_jit, r0, r1, r2)
__jit_inline void
x86_gtr_i(jit_state_t _jit, jit_gpr_t r0, jit_gpr_t r1, jit_gpr_t r2)
{
    jit_cmp_rr32(r0, r1, r2,		X86_CC_G);
}

#define jit_nei_i(r0, r1, i0)		x86_nei_i(_jit, r0, r1, i0)
__jit_inline void
x86_nei_i(jit_state_t _jit, jit_gpr_t r0, jit_gpr_t r1, int i0)
{
    if (i0)
	jit_cmp_ri32(r0, r1, i0,	X86_CC_NE);
    else
	jit_test_r32(r0, r1,		X86_CC_NE);
}

#define jit_ner_i(r0, r1, r2)		x86_ner_i(_jit, r0, r1, r2)
__jit_inline void
x86_ner_i(jit_state_t _jit, jit_gpr_t r0, jit_gpr_t r1, jit_gpr_t r2)
{
    if (r1 == r2)
	XORLrr(r0, r0);
    else
	jit_cmp_rr32(r0, r1, r2,	X86_CC_NE);
}

#define jit_lti_ui(r0, r1, i0)		x86_lti_ui(_jit, r0, r1, i0)
__jit_inline void
x86_lti_ui(jit_state_t _jit, jit_gpr_t r0, jit_gpr_t r1, unsigned int i0)
{
    jit_cmp_ri32(r0, r1, i0,		X86_CC_B);
}

#define jit_ltr_ui(r0, r1, r2)		x86_ltr_ui(_jit, r0, r1, r2)
__jit_inline void
x86_ltr_ui(jit_state_t _jit, jit_gpr_t r0, jit_gpr_t r1, jit_gpr_t r2)
{
    jit_cmp_rr32(r0, r1, r2,		X86_CC_B);
}

#define jit_lei_ui(r0, r1, i0)		x86_lei_ui(_jit, r0, r1, i0)
__jit_inline void
x86_lei_ui(jit_state_t _jit, jit_gpr_t r0, jit_gpr_t r1, unsigned int i0)
{
    if (i0)
	jit_cmp_ri32(r0, r1, i0,	X86_CC_BE);
    else
	jit_test_r32(r0, r1,		X86_CC_E);
}

#define jit_ler_ui(r0, r1, r2)		x86_ler_ui(_jit, r0, r1, r2)
__jit_inline void
x86_ler_ui(jit_state_t _jit, jit_gpr_t r0, jit_gpr_t r1, jit_gpr_t r2)
{
    if (r1 == r2)
	MOVLir(1, r0);
    else
	jit_cmp_rr32(r0, r1, r2,	X86_CC_BE);
}

#define jit_gei_ui(r0, r1, i0)		x86_gei_ui(_jit, r0, r1, i0)
__jit_inline void
x86_gei_ui(jit_state_t _jit,
	   jit_gpr_t r0, jit_gpr_t r1, unsigned int i0)
{
    if (i0)
	jit_cmp_ri32(r0, r1, i0,	X86_CC_AE);
    else
	jit_test_r32(r0, r1,		X86_CC_NB);
}

#define jit_ger_ui(r0, r1, r2)		x86_ger_ui(_jit, r0, r1, r2)
__jit_inline void
x86_ger_ui(jit_state_t _jit, jit_gpr_t r0, jit_gpr_t r1, jit_gpr_t r2)
{
    if (r1 == r2)
	MOVLir(1, r0);
    else
	jit_cmp_rr32(r0, r1, r2,	X86_CC_AE);
}

#define jit_gti_ui(r0, r1, i0)		x86_gti_ui(_jit, r0, r1, i0)
__jit_inline void
x86_gti_ui(jit_state_t _jit, jit_gpr_t r0, jit_gpr_t r1, unsigned int i0)
{
    if (i0)
	jit_cmp_ri32(r0, r1, i0,	X86_CC_A);
    else
	jit_test_r32(r0, r1,		X86_CC_NE);
}

#define jit_gtr_ui(r0, r1, r2)		x86_gtr_ui(_jit, r0, r1, r2)
__jit_inline void
x86_gtr_ui(jit_state_t _jit, jit_gpr_t r0, jit_gpr_t r1, jit_gpr_t r2)
{
    jit_cmp_rr32(r0, r1, r2,		X86_CC_A);
}

/* Jump */
#define jit_bcmp_ri32(i0, r0, i1, i2)	x86_bcmp_ri32(_jit, i0, r0, i1, i2)
__jit_inline void
x86_bcmp_ri32(jit_state_t _jit, jit_insn *label, jit_gpr_t r0, int i0,
	      x86_cc_t cc)
{
    CMPLir(i0, r0);
    JCCim(cc, label);
}

#define jit_btest_r32(i0, r0, i1)	x86_btest_r32(_jit, i0, r0, i1)
__jit_inline void
x86_btest_r32(jit_state_t _jit, jit_insn *label, jit_gpr_t r0, x86_cc_t cc)
{
    TESTLrr(r0, r0);
    JCCim(cc, label);
}

#define jit_bcmp_rr32(i0, r0, r1, i1)	x86_bcmp_rr32(_jit, i0, r0, r1, i1)
__jit_inline void
x86_bcmp_rr32(jit_state_t _jit, jit_insn *label, jit_gpr_t r0, jit_gpr_t r1,
	      x86_cc_t cc)
{
    CMPLrr(r1, r0);
    JCCim(cc, label);
}

#define jit_blti_i(label, r0, i0)	x86_blti_i(_jit, label, r0, i0)
__jit_inline jit_insn *
x86_blti_i(jit_state_t _jit, jit_insn *label, jit_gpr_t r0, int i0)
{
    if (i0)
	jit_bcmp_ri32(label, r0, i0,	X86_CC_L);
    else
	jit_btest_r32(label, r0,	X86_CC_S);
    return (_jit->x.pc);
}

#define jit_bltr_i(label, r0, r1)	x86_bltr_i(_jit, label, r0, r1)
__jit_inline jit_insn *
x86_bltr_i(jit_state_t _jit, jit_insn *label, jit_gpr_t r0, jit_gpr_t r1)
{
    jit_bcmp_rr32(label, r0, r1,	X86_CC_L);
    return (_jit->x.pc);
}

#define jit_blei_i(label, r0, i0)	x86_blei_i(_jit, label, r0, i0)
__jit_inline jit_insn *
x86_blei_i(jit_state_t _jit, jit_insn *label, jit_gpr_t r0, int i0)
{
    jit_bcmp_ri32(label, r0, i0,	X86_CC_LE);
    return (_jit->x.pc);
}

#define jit_bler_i(label, r0, r1)	x86_bler_i(_jit, label, r0, r1)
__jit_inline jit_insn *
x86_bler_i(jit_state_t _jit, jit_insn *label, jit_gpr_t r0, jit_gpr_t r1)
{
    if (r0 == r1)
	JMPm(label);
    else
	jit_bcmp_rr32(label, r0, r1,	X86_CC_LE);
    return (_jit->x.pc);
}

#define jit_beqi_i(label, r0, i0)	x86_beqi_i(_jit, label, r0, i0)
__jit_inline jit_insn *
x86_beqi_i(jit_state_t _jit, jit_insn *label, jit_gpr_t r0, int i0)
{
    if (i0)
	jit_bcmp_ri32(label, r0, i0,	X86_CC_E);
    else
	jit_btest_r32(label, r0,	X86_CC_E);
    return (_jit->x.pc);
}

#define jit_beqr_i(label, r0, r1)	x86_beqr_i(_jit, label, r0, r1)
__jit_inline jit_insn *
x86_beqr_i(jit_state_t _jit, jit_insn *label, jit_gpr_t r0, jit_gpr_t r1)
{
    if (r0 == r1)
	JMPm(label);
    else
	jit_bcmp_rr32(label, r0, r1,	X86_CC_E);
    return (_jit->x.pc);
}

#define jit_bgei_i(label, r0, i0)	x86_bgei_i(_jit, label, r0, i0)
__jit_inline jit_insn *
x86_bgei_i(jit_state_t _jit, jit_insn *label, jit_gpr_t r0, int i0)
{
    if (i0)
	jit_bcmp_ri32(label, r0, i0,	X86_CC_GE);
    else
	jit_btest_r32(label, r0,	X86_CC_NS);
    return (_jit->x.pc);
}

#define jit_bger_i(label, r0, r1)	x86_bger_i(_jit, label, r0, r1)
__jit_inline jit_insn *
x86_bger_i(jit_state_t _jit, jit_insn *label, jit_gpr_t r0, jit_gpr_t r1)
{
    if (r0 == r1)
	JMPm(label);
    else
	jit_bcmp_rr32(label, r0, r1,	X86_CC_GE);
    return (_jit->x.pc);
}

#define jit_bgti_i(label, r0, i0)	x86_bgti_i(_jit, label, r0, i0)
__jit_inline jit_insn *
x86_bgti_i(jit_state_t _jit, jit_insn *label, jit_gpr_t r0, int i0)
{
    jit_bcmp_ri32(label, r0, i0,	X86_CC_G);
    return (_jit->x.pc);
}

#define jit_bgtr_i(label, r0, r1)	x86_bgtr_i(_jit, label, r0, r1)
__jit_inline jit_insn *
x86_bgtr_i(jit_state_t _jit, jit_insn *label, jit_gpr_t r0, jit_gpr_t r1)
{
    jit_bcmp_rr32(label, r0, r1,	X86_CC_G);
    return (_jit->x.pc);
}

#define jit_bnei_i(label, r0, i0)	x86_bnei_i(_jit, label, r0, i0)
__jit_inline jit_insn *
x86_bnei_i(jit_state_t _jit, jit_insn *label, jit_gpr_t r0, int i0)
{
    if (i0)
	jit_bcmp_ri32(label, r0, i0,	X86_CC_NE);
    else
	jit_btest_r32(label, r0,	X86_CC_NE);
    return (_jit->x.pc);
}

#define jit_bner_i(label, r0, r1)	x86_bner_i(_jit, label, r0, r1)
__jit_inline jit_insn *
x86_bner_i(jit_state_t _jit, jit_insn *label, jit_gpr_t r0, jit_gpr_t r1)
{
    /* need to return a patchable address even if r0 == r1 */
    jit_bcmp_rr32(label, r0, r1,	X86_CC_NE);
    return (_jit->x.pc);
}

#define jit_blti_ui(label, r0, i0)	x86_blti_ui(_jit, label, r0, i0)
__jit_inline jit_insn *
x86_blti_ui(jit_state_t _jit, jit_insn *label, jit_gpr_t r0, unsigned int i0)
{
    /* need to return a patchable address even if i0 == 0 */
    jit_bcmp_ri32(label, r0, i0,	X86_CC_B);
    return (_jit->x.pc);
}

#define jit_bltr_ui(label, r0, r1)	x86_bltr_ui(_jit, label, r0, r1)
__jit_inline jit_insn *
x86_bltr_ui(jit_state_t _jit, jit_insn *label, jit_gpr_t r0, jit_gpr_t r1)
{
    jit_bcmp_rr32(label, r0, r1,	X86_CC_B);
    return (_jit->x.pc);
}

#define jit_blei_ui(label, r0, i0)	x86_blei_ui(_jit, label, r0, i0)
__jit_inline jit_insn *
x86_blei_ui(jit_state_t _jit, jit_insn *label, jit_gpr_t r0, unsigned int i0)
{
    /* need to return a patchable address even if i0 == 0 */
    jit_bcmp_ri32(label, r0, i0,	X86_CC_BE);
    return (_jit->x.pc);
}

#define jit_bler_ui(label, r0, r1)	x86_bler_ui(_jit, label, r0, r1)
__jit_inline jit_insn *
x86_bler_ui(jit_state_t _jit, jit_insn *label, jit_gpr_t r0, jit_gpr_t r1)
{
    if (r0 == r1)
	JMPm(label);
    else
	jit_bcmp_rr32(label, r0, r1,	X86_CC_BE);
    return (_jit->x.pc);
}

#define jit_bgei_ui(label, r0, i0)	x86_bgei_ui(_jit, label, r0, i0)
__jit_inline jit_insn *
x86_bgei_ui(jit_state_t _jit, jit_insn *label, jit_gpr_t r0, unsigned int i0)
{
    if (i0 == 0)
	JMPm(label);
    else
	jit_bcmp_ri32(label, r0, i0,	X86_CC_AE);
    return (_jit->x.pc);
}

#define jit_bger_ui(label, r0, r1)	x86_bger_ui(_jit, label, r0, r1)
__jit_inline jit_insn *
x86_bger_ui(jit_state_t _jit, jit_insn *label, jit_gpr_t r0, jit_gpr_t r1)
{
    if (r0 == r1)
	JMPm(label);
    else
	jit_bcmp_rr32(label, r0, r1,	X86_CC_AE);
    return (_jit->x.pc);
}

#define jit_bgti_ui(label, r0, i0)	x86_bgti_ui(_jit, label, r0, i0)
__jit_inline jit_insn *
x86_bgti_ui(jit_state_t _jit, jit_insn *label, jit_gpr_t r0, unsigned int i0)
{
    if (i0)
	jit_bcmp_ri32(label, r0, i0,	X86_CC_A);
    else
	jit_btest_r32(label, r0,	X86_CC_NE);
    return (_jit->x.pc);
}

#define jit_bgtr_ui(label, r0, r1)	x86_bgtr_ui(_jit, label, r0, r1)
__jit_inline jit_insn *
x86_bgtr_ui(jit_state_t _jit, jit_insn *label, jit_gpr_t r0, jit_gpr_t r1)
{
    jit_bcmp_rr32(label, r0, r1,	X86_CC_A);
    return (_jit->x.pc);
}

#define jit_boaddi_i(label, r0, i0)	x86_boaddi_i(_jit, label, r0, i0)
__jit_inline jit_insn *
x86_boaddi_i(jit_state_t _jit, jit_insn *label, jit_gpr_t r0, int i0)
{
    ADDLir(i0, r0);
    JOm(label);
    return (_jit->x.pc);
}

#define jit_boaddr_i(label, r0, r1)	x86_boaddr_i(_jit, label, r0, r1)
__jit_inline jit_insn *
x86_boaddr_i(jit_state_t _jit, jit_insn *label, jit_gpr_t r0, jit_gpr_t r1)
{
    ADDLrr(r1, r0);
    JOm(label);
    return (_jit->x.pc);
}

#define jit_bosubi_i(label, r0, i0)	x86_bosubi_i(_jit, label, r0, i0)
__jit_inline jit_insn *
x86_bosubi_i(jit_state_t _jit, jit_insn *label, jit_gpr_t r0, int i0)
{
    SUBLir(i0, r0);
    JOm(label);
    return (_jit->x.pc);
}

#define jit_bosubr_i(label, r0, r1)	x86_bosubr_i(_jit, label, r0, r1)
__jit_inline jit_insn *
x86_bosubr_i(jit_state_t _jit, jit_insn *label, jit_gpr_t r0, jit_gpr_t r1)
{
    SUBLrr(r1, r0);
    JOm(label);
    return (_jit->x.pc);
}

#define jit_boaddi_ui(label, r0, i0)	x86_boaddi_ui(_jit, label, r0, i0)
__jit_inline jit_insn *
x86_boaddi_ui(jit_state_t _jit, jit_insn *label, jit_gpr_t r0, unsigned int i0)
{
    ADDLir((int)i0, r0);
    JCm(label);
    return (_jit->x.pc);
}

#define jit_boaddr_ui(label, r0, r1)	x86_boaddr_ui(_jit, label, r0, r1)
__jit_inline jit_insn *
x86_boaddr_ui(jit_state_t _jit, jit_insn *label, jit_gpr_t r0, jit_gpr_t r1)
{
    ADDLrr(r1, r0);
    JCm(label);
    return (_jit->x.pc);
}

#define jit_bosubi_ui(label, r0, i0)	x86_bosubi_ui(_jit, label, r0, i0)
__jit_inline jit_insn *
x86_bosubi_ui(jit_state_t _jit, jit_insn *label, jit_gpr_t r0, unsigned int i0)
{
    SUBLir((int)i0, r0);
    JCm(label);
    return (_jit->x.pc);
}

#define jit_bosubr_ui(label, r0, r1)	x86_bosubr_ui(_jit, label, r0, r1)
__jit_inline jit_insn *
x86_bosubr_ui(jit_state_t _jit, jit_insn *label, jit_gpr_t r0, jit_gpr_t r1)
{
    SUBLrr(r1, r0);
    JCm(label);
    return (_jit->x.pc);
}

#define jit_bmsi_i(label, r0, i0)	x86_bmsi_i(_jit, label, r0, i0)
__jit_inline jit_insn *
x86_bmsi_i(jit_state_t _jit, jit_insn *label, jit_gpr_t r0, int i0)
{
    if (jit_reg8_p(r0) && _u8P(i0))
	TESTBir(i0, r0);
    /* valid in 64 bits mode */
    else if (_u16P(i0))
	TESTWir(i0, r0);
    else
	TESTLir(i0, r0);
    JNZm(label);
    return (_jit->x.pc);
}

#define jit_bmsr_i(label, r0, r1)	x86_bmsr_i(_jit, label, r0, r1)
__jit_inline jit_insn *
x86_bmsr_i(jit_state_t _jit, jit_insn *label, jit_gpr_t r0, jit_gpr_t r1)
{
    TESTLrr(r1, r0);
    JNZm(label);
    return (_jit->x.pc);
}

#define jit_bmci_i(label, r0, i0)	x86_bmci_i(_jit, label, r0, i0)
__jit_inline jit_insn *
x86_bmci_i(jit_state_t _jit, jit_insn *label, jit_gpr_t r0, int i0)
{
    if (jit_reg8_p(r0) && _u8P(i0))
	TESTBir(i0, r0);
    /* valid in 64 bits mode */
    else if (_u16P(i0))
	TESTWir(i0, r0);
    else
	TESTLir(i0, r0);
    JZm(label);
    return (_jit->x.pc);
}

#define jit_bmcr_i(label, r0, r1)	x86_bmcr_i(_jit, label, r0, r1)
__jit_inline jit_insn *
x86_bmcr_i(jit_state_t _jit, jit_insn *label, jit_gpr_t r0, jit_gpr_t r1)
{
    TESTLrr(r1, r0);
    JZm(label);
    return (_jit->x.pc);
}

/* Memory */
#define jit_ntoh_us(r0, r1)		x86_ntoh_us(_jit, r0, r1)
__jit_inline void
x86_ntoh_us(jit_state_t _jit, jit_gpr_t r0, jit_gpr_t r1)
{
    jit_movr_i(r0, r1);
    RORWir(8, r0);
}

#define jit_ntoh_ui(r0, r1)		x86_ntoh_ui(_jit, r0, r1)
__jit_inline void
x86_ntoh_ui(jit_state_t _jit, jit_gpr_t r0, jit_gpr_t r1)
{
    jit_movr_i(r0, r1);
    BSWAPLr(r0);
}

#define jit_extr_c_i(r0, r1)		x86_extr_c_i(_jit, r0, r1)
__jit_inline void
x86_extr_c_i(jit_state_t _jit, jit_gpr_t r0, jit_gpr_t r1)
{
    jit_gpr_t	rep;

    if (jit_reg8_p(r1))
	MOVSBLrr(r1, r0);
    else {
	if (r0 == _RAX)
	    rep = _RDX;
	else
	    rep = _RAX;
	if (r0 != r1)
	    jit_xchgr_i(rep, r1);
	else {
	    jit_pushr_i(rep);
	    MOVLrr(r1, rep);
	}
	MOVSBLrr(rep, r0);
	if (r0 != r1)
	    jit_xchgr_i(rep, r1);
	else
	    jit_popr_i(rep);
    }
}

#define jit_extr_c_ui(r0, r1)		x86_extr_c_ui(_jit, r0, r1)
__jit_inline void
x86_extr_c_ui(jit_state_t _jit, jit_gpr_t r0, jit_gpr_t r1)
{
    jit_gpr_t	rep;

    if (jit_reg8_p(r1))
	MOVZBLrr(r1, r0);
    else {
	if (r0 == _RAX)
	    rep = _RDX;
	else
	    rep = _RAX;
	if (r0 != r1)
	    jit_xchgr_i(rep, r1);
	else {
	    jit_pushr_i(rep);
	    MOVLrr(r1, rep);
	}
	MOVZBLrr(rep, r0);
	if (r0 != r1)
	    jit_xchgr_i(rep, r1);
	else
	    jit_popr_i(rep);
    }
}

#define jit_extr_s_i(r0, r1)		x86_extr_s_i(_jit, r0, r1)
__jit_inline void
x86_extr_s_i(jit_state_t _jit, jit_gpr_t r0, jit_gpr_t r1)
{
    MOVSWLrr(r1, r0);
}

#define jit_extr_s_ui(r0, r1)		x86_extr_s_ui(_jit, r0, r1)
__jit_inline void
x86_extr_s_ui(jit_state_t _jit, jit_gpr_t r0, jit_gpr_t r1)
{
    MOVZWLrr(r1, r0);
}

#define jit_ldr_uc(r0, r1)		x86_ldr_uc(_jit, r0, r1)
__jit_inline void
x86_ldr_uc(jit_state_t _jit, jit_gpr_t r0, jit_gpr_t r1)
{
    MOVZBLmr(0, r1, _NOREG, _SCL1, r0);
}

#define jit_ldxr_uc(r0, r1, r2)		x86_ldxr_uc(_jit, r0, r1, r2)
__jit_inline void
x86_ldxr_uc(jit_state_t _jit, jit_gpr_t r0, jit_gpr_t r1, jit_gpr_t r2)
{
    MOVZBLmr(0, r1, r2, _SCL1, r0);
}

#define jit_ldr_us(r0, r1)		x86_ldr_us(_jit, r0, r1)
__jit_inline void
x86_ldr_us(jit_state_t _jit, jit_gpr_t r0, jit_gpr_t r1)
{
    MOVZWLmr(0, r1, _NOREG, _SCL1, r0);
}

#define jit_ldxr_us(r0, r1, r2)		x86_ldxr_us(_jit, r0, r1, r2)
__jit_inline void
x86_ldxr_us(jit_state_t _jit, jit_gpr_t r0, jit_gpr_t r1, jit_gpr_t r2)
{
    MOVZWLmr(0, r1, r2, _SCL1, r0);
}

#define jit_str_s(r0, r1)		x86_str_s(_jit, r0, r1)
__jit_inline void
x86_str_s(jit_state_t _jit, jit_gpr_t r0, jit_gpr_t r1)
{
    MOVWrm(r1, 0, r0, _NOREG, _SCL1);
}

#define jit_stxr_s(r0, r1, r2)		x86_stxr_s(_jit, r0, r1, r2)
__jit_inline void
x86_stxr_s(jit_state_t _jit, jit_gpr_t r0, jit_gpr_t r1, jit_gpr_t r2)
{
    MOVWrm(r2, 0, r0, r1, _SCL1);
}

#define jit_str_i(r0, r1)		x86_str_i(_jit, r0, r1)
__jit_inline void
x86_str_i(jit_state_t _jit, jit_gpr_t r0, jit_gpr_t r1)
{
    MOVLrm(r1, 0, r0, _NOREG, _SCL1);
}

#define jit_stxr_i(r0, r1, r2)		x86_stxr_i(_jit, r0, r1, r2)
__jit_inline void
x86_stxr_i(jit_state_t _jit, jit_gpr_t r0, jit_gpr_t r1, jit_gpr_t r2)
{
    MOVLrm(r2, 0, r0, r1, _SCL1);
}

/* Extra */
#define jit_nop(n)			x86_nop(_jit, n)
__jit_inline void
x86_nop(jit_state_t _jit)
{
    NOP_();
}

#define jit_align(n) 			x86_align(_jit, n)
__jit_inline void
x86_align(jit_state_t _jit, int n)
{
    int		align = ((((_ul)_jit->x.pc) ^ _MASK(4)) + 1) & _MASK(n);

    NOPi(align);
}

#if __WORDSIZE == 64
#include "core-64.h"
#else
#include "core-32.h"
#endif

#endif /* __lightning_core_i386_h */
