/******************************** -*- C -*- ****************************
 *
 *	Floating-point support (i386)
 *
 ***********************************************************************/


/***********************************************************************
 *
 * Copyright 2008,2010 Free Software Foundation, Inc.
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



#ifndef __lightning_fp_i386_h
#define __lightning_fp_i386_h

#define jit_sse_p()			jit_cpu.sse
#define jit_sse2_p()			jit_cpu.sse2
#define jit_sse4_1_p()			jit_cpu.sse4_1
#define jit_i686_p()			jit_cpu.cmov
#define jit_round_to_nearest_p()	jit_flags.rnd_near
#define jit_x87_reg_p(reg)		((reg) >= _ST0 && (reg) <= _ST7)

#if __WORDSIZE == 32
#  define jit_sse_reg_p(reg)		((reg) >= _XMM0 && (reg) <= _XMM7)
#  define JIT_FPTMP0			_XMM6
#  define JIT_FPTMP1			_XMM7
#else
#  define jit_sse_reg_p(reg)		((reg) >= _XMM0 && (reg) <= _XMM15)
#  define JIT_FPTMP0			_XMM14
#  define JIT_FPTMP1			_XMM15
#endif

#include "fp-sse.h"
#include "fp-x87.h"

/*   Rely on _ASM_SAFETY to trigger mixing of x87 and sse.
 *   If sse is enabled, usage of JIT_FPRET is only expected to
 * work when using it on operations that do not include an sse
 * register.
 */
__jit_inline void
sse_from_x87_f(jit_state_t _jit, jit_fpr_t f0, jit_fpr_t f1)
{
    if (jit_push_pop_p()) {
	jit_subi_l(JIT_SP, JIT_SP, sizeof(double));
	x87_str_f(_jit, JIT_SP, f1);
	sse_ldr_f(_jit, f0, JIT_SP);
	jit_addi_l(JIT_SP, JIT_SP, sizeof(double));
    }
    else {
	if (_jitl.float_offset == 0)
	    _jitl.float_offset = jit_allocai(8 + (-_jitl.alloca_offset & 7));
	x87_stxi_f(_jit, _jitl.float_offset, JIT_FP, f1);
	sse_ldxi_f(_jit, f0, JIT_FP, _jitl.float_offset);
    }
}

__jit_inline void
sse_from_x87_d(jit_state_t _jit, jit_fpr_t f0, jit_fpr_t f1)
{
    if (jit_push_pop_p()) {
	jit_subi_l(JIT_SP, JIT_SP, sizeof(double));
	x87_str_d(_jit, JIT_SP, f1);
	sse_ldr_d(_jit, f0, JIT_SP);
	jit_addi_l(JIT_SP, JIT_SP, sizeof(double));
    }
    else {
	if (_jitl.float_offset == 0)
	    _jitl.float_offset = jit_allocai(8 + (-_jitl.alloca_offset & 7));
	x87_stxi_d(_jit, _jitl.float_offset, JIT_FP, f1);
	sse_ldxi_d(_jit, f0, JIT_FP, _jitl.float_offset);
    }
}

__jit_inline void
x87_from_sse_f(jit_state_t _jit, jit_fpr_t f0, jit_fpr_t f1)
{
    if (jit_push_pop_p()) {
	jit_subi_l(JIT_SP, JIT_SP, sizeof(double));
	sse_str_f(_jit, JIT_SP, f1);
	x87_ldr_f(_jit, f0, JIT_SP);
	jit_addi_l(JIT_SP, JIT_SP, sizeof(double));
    }
    else {
	if (_jitl.float_offset == 0)
	    _jitl.float_offset = jit_allocai(8 + (-_jitl.alloca_offset & 7));
	sse_stxi_f(_jit, _jitl.float_offset, JIT_FP, f1);
	x87_ldxi_f(_jit, f0, JIT_FP, _jitl.float_offset);
    }
}

__jit_inline void
x87_from_sse_d(jit_state_t _jit, jit_fpr_t f0, jit_fpr_t f1)
{
    if (jit_push_pop_p()) {
	jit_subi_l(JIT_SP, JIT_SP, sizeof(double));
	sse_str_d(_jit, JIT_SP, f1);
	x87_ldr_d(_jit, f0, JIT_SP);
	jit_addi_l(JIT_SP, JIT_SP, sizeof(double));
    }
    else {
	if (_jitl.float_offset == 0)
	    _jitl.float_offset = jit_allocai(8 + (-_jitl.alloca_offset & 7));
	sse_stxi_d(_jit, _jitl.float_offset, JIT_FP, f1);
	x87_ldxi_d(_jit, f0, JIT_FP, _jitl.float_offset);
    }
}

#define jit_absr_f(f0, f1)		x86_absr_f(_jit, f0, f1)
__jit_inline void
x86_absr_f(jit_state_t _jit, jit_fpr_t f0, jit_fpr_t f1)
{
    if (jit_sse_reg_p(f0))
	sse_absr_f(_jit, f0, f1);
    else
	x87_absr_d(_jit, f0, f1);
}

#define jit_absr_d(f0, f1)		x86_absr_d(_jit, f0, f1)
__jit_inline void
x86_absr_d(jit_state_t _jit, jit_fpr_t f0, jit_fpr_t f1)
{
    if (jit_sse_reg_p(f0))
	sse_absr_d(_jit, f0, f1);
    else
	x87_absr_d(_jit, f0, f1);
}

#define jit_negr_f(f0, f1)		x86_negr_f(_jit, f0, f1)
__jit_inline void
x86_negr_f(jit_state_t _jit, jit_fpr_t f0, jit_fpr_t f1)
{
    if (jit_sse_reg_p(f0))
	sse_negr_f(_jit, f0, f1);
    else
	x87_negr_d(_jit, f0, f1);
}

#define jit_negr_d(f0, f1)		x86_negr_d(_jit, f0, f1)
__jit_inline void
x86_negr_d(jit_state_t _jit, jit_fpr_t f0, jit_fpr_t f1)
{
    if (jit_sse_reg_p(f0))
	sse_negr_d(_jit, f0, f1);
    else
	x87_negr_d(_jit, f0, f1);
}

#define jit_sqrtr_f(f0, f1)		x86_sqrtr_f(_jit, f0, f1)
__jit_inline void
x86_sqrtr_f(jit_state_t _jit, jit_fpr_t f0, jit_fpr_t f1)
{
    if (jit_sse_reg_p(f0))
	sse_sqrtr_f(_jit, f0, f1);
    else
	x87_sqrtr_d(_jit, f0, f1);
}

#define jit_sqrtr_d(f0, f1)		x86_sqrtr_d(_jit, f0, f1)
__jit_inline void
x86_sqrtr_d(jit_state_t _jit, jit_fpr_t f0, jit_fpr_t f1)
{
    if (jit_sse_reg_p(f0))
	sse_sqrtr_d(_jit, f0, f1);
    else
	x87_sqrtr_d(_jit, f0, f1);
}

#define jit_sinr_f(f0, f1)		x86_sinr_f(_jit, f0, f1)
__jit_inline void
x86_sinr_f(jit_state_t _jit, jit_fpr_t f0, jit_fpr_t f1)
{
    if (jit_sse_reg_p(f0)) {
	x87_from_sse_f(_jit, _ST0, f1);
	x87_sinr_d(_jit, _ST0, _ST0);
	sse_from_x87_f(_jit, f0, _ST0);
    }
    else
	x87_sinr_d(_jit, f0, f1);
}

#define jit_sinr_d(f0, f1)		x86_sinr_d(_jit, f0, f1)
__jit_inline void
x86_sinr_d(jit_state_t _jit, jit_fpr_t f0, jit_fpr_t f1)
{
    if (jit_sse_reg_p(f0)) {
	x87_from_sse_d(_jit, _ST0, f1);
	x87_sinr_d(_jit, _ST0, _ST0);
	sse_from_x87_d(_jit, f0, _ST0);
    }
    else
	x87_sinr_d(_jit, f0, f1);
}

#define jit_cosr_f(f0, f1)		x86_cosr_f(_jit, f0, f1)
__jit_inline void
x86_cosr_f(jit_state_t _jit, jit_fpr_t f0, jit_fpr_t f1)
{
    if (jit_sse_reg_p(f0)) {
	x87_from_sse_f(_jit, _ST0, f1);
	x87_cosr_d(_jit, _ST0, _ST0);
	sse_from_x87_f(_jit, f0, _ST0);
    }
    else
	x87_cosr_d(_jit, f0, f1);
}

#define jit_cosr_d(f0, f1)		x86_cosr_d(_jit, f0, f1)
__jit_inline void
x86_cosr_d(jit_state_t _jit, jit_fpr_t f0, jit_fpr_t f1)
{
    if (jit_sse_reg_p(f0)) {
	x87_from_sse_d(_jit, _ST0, f1);
	x87_cosr_d(_jit, _ST0, _ST0);
	sse_from_x87_d(_jit, f0, _ST0);
    }
    else
	x87_cosr_d(_jit, f0, f1);
}

#define jit_tanr_f(f0, f1)		x86_tanr_f(_jit, f0, f1)
__jit_inline void
x86_tanr_f(jit_state_t _jit, jit_fpr_t f0, jit_fpr_t f1)
{
    if (jit_sse_reg_p(f0)) {
	x87_from_sse_f(_jit, _ST0, f1);
	x87_tanr_d(_jit, _ST0, _ST0);
	sse_from_x87_f(_jit, f0, _ST0);
    }
    else
	x87_tanr_d(_jit, f0, f1);
}

#define jit_tanr_d(f0, f1)		x86_tanr_d(_jit, f0, f1)
__jit_inline void
x86_tanr_d(jit_state_t _jit, jit_fpr_t f0, jit_fpr_t f1)
{
    if (jit_sse_reg_p(f0)) {
	x87_from_sse_d(_jit, _ST0, f1);
	x87_tanr_d(_jit, _ST0, _ST0);
	sse_from_x87_d(_jit, f0, _ST0);
    }
    else
	x87_tanr_d(_jit, f0, f1);
}

#define jit_atanr_f(f0, f1)		x86_atanr_f(_jit, f0, f1)
__jit_inline void
x86_atanr_f(jit_state_t _jit, jit_fpr_t f0, jit_fpr_t f1)
{
    if (jit_sse_reg_p(f0)) {
	x87_from_sse_f(_jit, _ST0, f1);
	x87_atanr_d(_jit, _ST0, _ST0);
	sse_from_x87_f(_jit, f0, _ST0);
    }
    else
	x87_atanr_d(_jit, f0, f1);
}

#define jit_atanr_d(f0, f1)		x86_atanr_d(_jit, f0, f1)
__jit_inline void
x86_atanr_d(jit_state_t _jit, jit_fpr_t f0, jit_fpr_t f1)
{
    if (jit_sse_reg_p(f0)) {
	x87_from_sse_d(_jit, _ST0, f1);
	x87_atanr_d(_jit, _ST0, _ST0);
	sse_from_x87_d(_jit, f0, _ST0);
    }
    else
	x87_atanr_d(_jit, f0, f1);
}

#define jit_logr_f(f0, f1)		x86_logr_f(_jit, f0, f1)
__jit_inline void
x86_logr_f(jit_state_t _jit, jit_fpr_t f0, jit_fpr_t f1)
{
    if (jit_sse_reg_p(f0)) {
	x87_from_sse_f(_jit, _ST0, f1);
	x87_logr_d(_jit, _ST0, _ST0);
	sse_from_x87_f(_jit, f0, _ST0);
    }
    else
	x87_logr_d(_jit, f0, f1);
}

#define jit_logr_d(f0, f1)		x86_logr_d(_jit, f0, f1)
__jit_inline void
x86_logr_d(jit_state_t _jit, jit_fpr_t f0, jit_fpr_t f1)
{
    if (jit_sse_reg_p(f0)) {
	x87_from_sse_d(_jit, _ST0, f1);
	x87_logr_d(_jit, _ST0, _ST0);
	sse_from_x87_d(_jit, f0, _ST0);
    }
    else
	x87_logr_d(_jit, f0, f1);
}

#define jit_log2r_f(f0, f1)		x86_log2r_f(_jit, f0, f1)
__jit_inline void
x86_log2r_f(jit_state_t _jit, jit_fpr_t f0, jit_fpr_t f1)
{
    if (jit_sse_reg_p(f0)) {
	x87_from_sse_f(_jit, _ST0, f1);
	x87_log2r_d(_jit, _ST0, _ST0);
	sse_from_x87_f(_jit, f0, _ST0);
    }
    else
	x87_log2r_d(_jit, f0, f1);
}

#define jit_log2r_d(f0, f1)		x86_log2r_d(_jit, f0, f1)
__jit_inline void
x86_log2r_d(jit_state_t _jit, jit_fpr_t f0, jit_fpr_t f1)
{
    if (jit_sse_reg_p(f0)) {
	x87_from_sse_d(_jit, _ST0, f1);
	x87_log2r_d(_jit, _ST0, _ST0);
	sse_from_x87_d(_jit, f0, _ST0);
    }
    else
	x87_log2r_d(_jit, f0, f1);
}

#define jit_log10r_f(f0, f1)		x86_log10r_f(_jit, f0, f1)
__jit_inline void
x86_log10r_f(jit_state_t _jit, jit_fpr_t f0, jit_fpr_t f1)
{
    if (jit_sse_reg_p(f0)) {
	x87_from_sse_f(_jit, _ST0, f1);
	x87_log10r_d(_jit, _ST0, _ST0);
	sse_from_x87_f(_jit, f0, _ST0);
    }
    else
	x87_log10r_d(_jit, f0, f1);
}

#define jit_log10r_d(f0, f1)		x86_log10r_d(_jit, f0, f1)
__jit_inline void
x86_log10r_d(jit_state_t _jit, jit_fpr_t f0, jit_fpr_t f1)
{
    if (jit_sse_reg_p(f0)) {
	x87_from_sse_d(_jit, _ST0, f1);
	x87_log10r_d(_jit, _ST0, _ST0);
	sse_from_x87_d(_jit, f0, _ST0);
    }
    else
	x87_log10r_d(_jit, f0, f1);
}

#define jit_addr_f(f0, f1, f2)		x86_addr_f(_jit, f0, f1, f2)
__jit_inline void
x86_addr_f(jit_state_t _jit, jit_fpr_t f0, jit_fpr_t f1, jit_fpr_t f2)
{
    if (jit_sse_reg_p(f0))
	sse_addr_f(_jit, f0, f1, f2);
    else
	x87_addr_d(_jit, f0, f1, f2);
}

#define jit_addr_d(f0, f1, f2)		x86_addr_d(_jit, f0, f1, f2)
__jit_inline void
x86_addr_d(jit_state_t _jit, jit_fpr_t f0, jit_fpr_t f1, jit_fpr_t f2)
{
    if (jit_sse_reg_p(f0))
	sse_addr_d(_jit, f0, f1, f2);
    else
	x87_addr_d(_jit, f0, f1, f2);
}

#define jit_subr_f(f0, f1, f2)		x86_subr_f(_jit, f0, f1, f2)
__jit_inline void
x86_subr_f(jit_state_t _jit, jit_fpr_t f0, jit_fpr_t f1, jit_fpr_t f2)
{
    if (jit_sse_reg_p(f0))
	sse_subr_f(_jit, f0, f1, f2);
    else
	x87_subr_d(_jit, f0, f1, f2);
}

#define jit_subr_d(f0, f1, f2)		x86_subr_d(_jit, f0, f1, f2)
__jit_inline void
x86_subr_d(jit_state_t _jit, jit_fpr_t f0, jit_fpr_t f1, jit_fpr_t f2)
{
    if (jit_sse_reg_p(f0))
	sse_subr_d(_jit, f0, f1, f2);
    else
	x87_subr_d(_jit, f0, f1, f2);
}

#define jit_mulr_f(f0, f1, f2)		x86_mulr_f(_jit, f0, f1, f2)
__jit_inline void
x86_mulr_f(jit_state_t _jit, jit_fpr_t f0, jit_fpr_t f1, jit_fpr_t f2)
{
    if (jit_sse_reg_p(f0))
	sse_mulr_f(_jit, f0, f1, f2);
    else
	x87_mulr_d(_jit, f0, f1, f2);
}

#define jit_mulr_d(f0, f1, f2)		x86_mulr_d(_jit, f0, f1, f2)
__jit_inline void
x86_mulr_d(jit_state_t _jit, jit_fpr_t f0, jit_fpr_t f1, jit_fpr_t f2)
{
    if (jit_sse_reg_p(f0))
	sse_mulr_d(_jit, f0, f1, f2);
    else
	x87_mulr_d(_jit, f0, f1, f2);
}

#define jit_divr_f(f0, f1, f2)		x86_divr_f(_jit, f0, f1, f2)
__jit_inline void
x86_divr_f(jit_state_t _jit, jit_fpr_t f0, jit_fpr_t f1, jit_fpr_t f2)
{
    if (jit_sse_reg_p(f0))
	sse_divr_f(_jit, f0, f1, f2);
    else
	x87_divr_d(_jit, f0, f1, f2);
}

#define jit_divr_d(f0, f1, f2)		x86_divr_d(_jit, f0, f1, f2)
__jit_inline void
x86_divr_d(jit_state_t _jit, jit_fpr_t f0, jit_fpr_t f1, jit_fpr_t f2)
{
    if (jit_sse_reg_p(f0))
	sse_divr_d(_jit, f0, f1, f2);
    else
	x87_divr_d(_jit, f0, f1, f2);
}

#define jit_ldi_f(f0, i0)		x86_ldi_f(_jit, f0, i0)
__jit_inline void
x86_ldi_f(jit_state_t _jit, jit_fpr_t f0, void *i0)
{
    if (jit_sse_reg_p(f0))
	sse_ldi_f(_jit, f0, i0);
    else
	x87_ldi_f(_jit, f0, i0);
}

#define jit_ldr_f(f0, r0)		x86_ldr_f(_jit, f0, r0)
__jit_inline void
x86_ldr_f(jit_state_t _jit, jit_fpr_t f0, jit_gpr_t r0)
{
    if (jit_sse_reg_p(f0))
	sse_ldr_f(_jit, f0, r0);
    else
	x87_ldr_f(_jit, f0, r0);
}

#define jit_ldxi_f(f0, r0, i0)		x86_ldxi_f(_jit, f0, r0, i0)
__jit_inline void
x86_ldxi_f(jit_state_t _jit, jit_fpr_t f0, jit_gpr_t r0, int i0)
{
    if (jit_sse_reg_p(f0))
	sse_ldxi_f(_jit, f0, r0, i0);
    else
	x87_ldxi_f(_jit, f0, r0, i0);
}

#define jit_ldxr_f(f0, r0, r1)		x86_ldxr_f(_jit, f0, r0, r1)
__jit_inline void
x86_ldxr_f(jit_state_t _jit, jit_fpr_t f0, jit_gpr_t r0, jit_gpr_t r1)
{
    if (jit_sse_reg_p(f0))
	sse_ldxr_f(_jit, f0, r0, r1);
    else
	x87_ldxr_f(_jit, f0, r0, r1);
}

#define jit_ldi_d(f0, i0)		x86_ldi_d(_jit, f0, i0)
__jit_inline void
x86_ldi_d(jit_state_t _jit, jit_fpr_t f0, void *i0)
{
    if (jit_sse_reg_p(f0))
	sse_ldi_d(_jit, f0, i0);
    else
	x87_ldi_d(_jit, f0, i0);
}

#define jit_ldr_d(f0, r0)		x86_ldr_d(_jit, f0, r0)
__jit_inline void
x86_ldr_d(jit_state_t _jit, jit_fpr_t f0, jit_gpr_t r0)
{
    if (jit_sse_reg_p(f0))
	sse_ldr_d(_jit, f0, r0);
    else
	x87_ldr_d(_jit, f0, r0);
}

#define jit_ldxi_d(f0, r0, i0)		x86_ldxi_d(_jit, f0, r0, i0)
__jit_inline void
x86_ldxi_d(jit_state_t _jit, jit_fpr_t f0, jit_gpr_t r0, long i0)
{
    if (jit_sse_reg_p(f0))
	sse_ldxi_d(_jit, f0, r0, i0);
    else
	x87_ldxi_d(_jit, f0, r0, i0);
}

#define jit_ldxr_d(f0, r0, r1)		x86_ldxr_d(_jit, f0, r0, r1)
__jit_inline void
x86_ldxr_d(jit_state_t _jit, jit_fpr_t f0, jit_gpr_t r0, jit_gpr_t r1)
{
    if (jit_sse_reg_p(f0))
	sse_ldxr_d(_jit, f0, r0, r1);
    else
	x87_ldxr_d(_jit, f0, r0, r1);
}

#define jit_sti_f(i0, f0)		x86_sti_f(_jit, i0, f0)
__jit_inline void
x86_sti_f(jit_state_t _jit, void *i0, jit_fpr_t f0)
{
    if (jit_sse_reg_p(f0))
	sse_sti_f(_jit, i0, f0);
    else
	x87_sti_f(_jit, i0, f0);
}

#define jit_str_f(r0, f0)		x86_str_f(_jit, r0, f0)
__jit_inline void
x86_str_f(jit_state_t _jit, jit_gpr_t r0, jit_fpr_t f0)
{
    if (jit_sse_reg_p(f0))
	sse_str_f(_jit, r0, f0);
    else
	x87_str_f(_jit, r0, f0);
}

#define jit_stxi_f(i0, r0, f0)		x86_stxi_f(_jit, i0, r0, f0)
__jit_inline void
x86_stxi_f(jit_state_t _jit, long i0, jit_gpr_t r0, jit_fpr_t f0)
{
    if (jit_sse_reg_p(f0))
	sse_stxi_f(_jit, i0, r0, f0);
    else
	x87_stxi_f(_jit, i0, r0, f0);
}

#define jit_stxr_f(r0, r1, f0)		x86_stxr_f(_jit, r0, r1, f0)
__jit_inline void
x86_stxr_f(jit_state_t _jit, jit_gpr_t r0, jit_gpr_t r1, jit_fpr_t f0)
{
    if (jit_sse_reg_p(f0))
	sse_stxr_f(_jit, r0, r1, f0);
    else
	x87_stxr_f(_jit, r0, r1, f0);
}

#define jit_sti_d(i0, f0)		x86_sti_d(_jit, i0, f0)
__jit_inline void
x86_sti_d(jit_state_t _jit, void *i0, jit_fpr_t f0)
{
    if (jit_sse_reg_p(f0))
	sse_sti_d(_jit, i0, f0);
    else
	x87_sti_d(_jit, i0, f0);
}

#define jit_str_d(r0, f0)		x86_str_d(_jit, r0, f0)
__jit_inline void
x86_str_d(jit_state_t _jit, jit_gpr_t r0, jit_fpr_t f0)
{
    if (jit_sse_reg_p(f0))
	sse_str_d(_jit, r0, f0);
    else
	x87_str_d(_jit, r0, f0);
}

#define jit_stxi_d(i0, r0, f0)		x86_stxi_d(_jit, i0, r0, f0)
__jit_inline void
x86_stxi_d(jit_state_t _jit, long i0, jit_gpr_t r0, jit_fpr_t f0)
{
    if (jit_sse_reg_p(f0))
	sse_stxi_d(_jit, i0, r0, f0);
    else
	x87_stxi_d(_jit, i0, r0, f0);
}

#define jit_stxr_d(r0, r1, f0)		x86_stxr_d(_jit, r0, r1, f0)
__jit_inline void
x86_stxr_d(jit_state_t _jit, jit_gpr_t r0, jit_gpr_t r1, jit_fpr_t f0)
{
    if (jit_sse_reg_p(f0))
	sse_stxr_d(_jit, r0, r1, f0);
    else
	x87_stxr_d(_jit, r0, r1, f0);
}

#define jit_movi_f(f0, i0)		x86_movi_f(_jit, f0, i0)
__jit_inline void
x86_movi_f(jit_state_t _jit, jit_fpr_t f0, float i0)
{
    if (jit_sse_reg_p(f0))
	sse_movi_f(_jit, f0, i0);
    else
	x87_movi_f(_jit, f0, i0);
}

#define jit_movi_d(f0, i0)		x86_movi_d(_jit, f0, i0)
__jit_inline void
x86_movi_d(jit_state_t _jit, jit_fpr_t f0, double i0)
{
    if (jit_sse_reg_p(f0))
	sse_movi_d(_jit, f0, i0);
    else
	x87_movi_d(_jit, f0, i0);
}

#define jit_movr_f(f0, f1)		x86_movr_f(_jit, f0, f1)
__jit_inline void
x86_movr_f(jit_state_t _jit, jit_fpr_t f0, jit_fpr_t f1)
{
    if (jit_sse_reg_p(f0)) {
	if (jit_sse_reg_p(f1))
	    sse_movr_f(_jit, f0, f1);
	else
	    sse_from_x87_f(_jit, f0, f1);
    }
    else {
	if (jit_x87_reg_p(f1))
	    x87_movr_d(_jit, f0, f1);
	else
	    x87_from_sse_f(_jit, f0, f1);
    }
}

#define jit_movr_d(f0, f1)		x86_movr_d(_jit, f0, f1)
__jit_inline void
x86_movr_d(jit_state_t _jit, jit_fpr_t f0, jit_fpr_t f1)
{
    if (jit_sse_reg_p(f0)) {
	if (jit_sse_reg_p(f1))
	    sse_movr_d(_jit, f0, f1);
	else
	    sse_from_x87_d(_jit, f0, f1);
    }
    else {
	if (jit_x87_reg_p(f1))
	    x87_movr_d(_jit, f0, f1);
	else
	    x87_from_sse_d(_jit, f0, f1);
    }
}

#define jit_extr_i_f(f0, r0)		x86_extr_i_f(_jit, f0, r0)
__jit_inline void
x86_extr_i_f(jit_state_t _jit, jit_fpr_t f0, jit_gpr_t r0)
{
    if (jit_sse_reg_p(f0))
	sse_extr_i_f(_jit, f0, r0);
    else
	x87_extr_i_d(_jit, f0, r0);
}

#define jit_extr_i_d(f0, r0)		x86_extr_i_d(_jit, f0, r0)
__jit_inline void
x86_extr_i_d(jit_state_t _jit, jit_fpr_t f0, jit_gpr_t r0)
{
    if (jit_sse_reg_p(f0))
	sse_extr_i_d(_jit, f0, r0);
    else
	x87_extr_i_d(_jit, f0, r0);
}

#define jit_extr_f_d(f0, f1)		x86_extr_f_d(_jit, f0, f1)
__jit_inline void
x86_extr_f_d(jit_state_t _jit, jit_fpr_t f0, jit_fpr_t f1)
{
    if (jit_sse_reg_p(f0))
	sse_extr_f_d(_jit, f0, f1);
    else
	x87_movr_d(_jit, f0, f1);
}

#define jit_extr_d_f(f0, f1)		x86_extr_d_f(_jit, f0, f1)
__jit_inline void
x86_extr_d_f(jit_state_t _jit, jit_fpr_t f0, jit_fpr_t f1)
{
    if (jit_sse_reg_p(f0))
	sse_extr_d_f(_jit, f0, f1);
    else
	x87_movr_d(_jit, f0, f1);
}

#define jit_rintr_f_i(r0, f0)		x86_rintr_f_i(_jit, r0, f0)
__jit_inline void
x86_rintr_f_i(jit_state_t _jit, jit_gpr_t r0, jit_fpr_t f0)
{
    if (jit_sse_reg_p(f0))
	sse_rintr_f_i(_jit, r0, f0);
    else
	x87_rintr_d_i(_jit, r0, f0);
}

#define jit_rintr_d_i(r0, f0)		x86_rintr_d_i(_jit, r0, f0)
__jit_inline void
x86_rintr_d_i(jit_state_t _jit, jit_gpr_t r0, jit_fpr_t f0)
{
    if (jit_sse_reg_p(f0))
	sse_rintr_d_i(_jit, r0, f0);
    else
	x87_rintr_d_i(_jit, r0, f0);
}

#define jit_roundr_f_i(r0, f0)		x86_roundr_f_i(_jit, r0, f0)
__jit_inline void
x86_roundr_f_i(jit_state_t _jit, jit_gpr_t r0, jit_fpr_t f0)
{
    if (jit_sse_reg_p(f0))
	sse_roundr_f_i(_jit, r0, f0);
    else
	x87_roundr_d_i(_jit, r0, f0);
}

#define jit_roundr_d_i(r0, f0)		x86_roundr_d_i(_jit, r0, f0)
__jit_inline void
x86_roundr_d_i(jit_state_t _jit, jit_gpr_t r0, jit_fpr_t f0)
{
    if (jit_sse_reg_p(f0))
	sse_roundr_d_i(_jit, r0, f0);
    else
	x87_roundr_d_i(_jit, r0, f0);
}

#define jit_truncr_f_i(r0, f0)		x86_truncr_f_i(_jit, r0, f0)
__jit_inline void
x86_truncr_f_i(jit_state_t _jit, jit_gpr_t r0, jit_fpr_t f0)
{
    if (jit_sse_reg_p(f0))
	sse_truncr_f_i(_jit, r0, f0);
    else
	x87_truncr_d_i(_jit, r0, f0);
}

#define jit_truncr_d_i(r0, f0)		x86_truncr_d_i(_jit, r0, f0)
__jit_inline void
x86_truncr_d_i(jit_state_t _jit, jit_gpr_t r0, jit_fpr_t f0)
{
    if (jit_sse_reg_p(f0))
	sse_truncr_d_i(_jit, r0, f0);
    else
	x87_truncr_d_i(_jit, r0, f0);
}

#define jit_floorr_f_i(r0, f0)		x86_floorr_f_i(_jit, r0, f0)
__jit_inline void
x86_floorr_f_i(jit_state_t _jit, jit_gpr_t r0, jit_fpr_t f0)
{
    if (jit_sse_reg_p(f0))
	sse_floorr_f_i(_jit, r0, f0);
    else
	x87_floorr_d_i(_jit, r0, f0);
}

#define jit_floorr_d_i(r0, f0)		x86_floorr_d_i(_jit, r0, f0)
__jit_inline void
x86_floorr_d_i(jit_state_t _jit, jit_gpr_t r0, jit_fpr_t f0)
{
    if (jit_sse_reg_p(f0))
	sse_floorr_d_i(_jit, r0, f0);
    else
	x87_floorr_d_i(_jit, r0, f0);
}

#define jit_ceilr_f_i(r0, f0)		x86_ceilr_f_i(_jit, r0, f0)
__jit_inline void
x86_ceilr_f_i(jit_state_t _jit, jit_gpr_t r0, jit_fpr_t f0)
{
    if (jit_sse_reg_p(f0))
	sse_ceilr_f_i(_jit, r0, f0);
    else
	x87_ceilr_d_i(_jit, r0, f0);
}

#define jit_ceilr_d_i(r0, f0)		x86_ceilr_d_i(_jit, r0, f0)
__jit_inline void
x86_ceilr_d_i(jit_state_t _jit, jit_gpr_t r0, jit_fpr_t f0)
{
    if (jit_sse_reg_p(f0))
	sse_ceilr_d_i(_jit, r0, f0);
    else
	x87_ceilr_d_i(_jit, r0, f0);
}

#define jit_ltr_f(r0, f0, f1)		x86_ltr_f(_jit, r0, f0, f1)
__jit_inline void
x86_ltr_f(jit_state_t _jit, jit_gpr_t r0, jit_fpr_t f0, jit_fpr_t f1)
{
    if (jit_sse_reg_p(f0))
	sse_ltr_f(_jit, r0, f0, f1);
    else
	x87_ltr_d(_jit, r0, f0, f1);
}

#define jit_ltr_d(r0, f0, f1)		x86_ltr_d(_jit, r0, f0, f1)
__jit_inline void
x86_ltr_d(jit_state_t _jit, jit_gpr_t r0, jit_fpr_t f0, jit_fpr_t f1)
{
    if (jit_sse_reg_p(f0))
	sse_ltr_d(_jit, r0, f0, f1);
    else
	x87_ltr_d(_jit, r0, f0, f1);
}

#define jit_ler_f(r0, f0, f1)		x86_ler_f(_jit, r0, f0, f1)
__jit_inline void
x86_ler_f(jit_state_t _jit, jit_gpr_t r0, jit_fpr_t f0, jit_fpr_t f1)
{
    if (jit_sse_reg_p(f0))
	sse_ler_f(_jit, r0, f0, f1);
    else
	x87_ler_d(_jit, r0, f0, f1);
}

#define jit_ler_d(r0, f0, f1)		x86_ler_d(_jit, r0, f0, f1)
__jit_inline void
x86_ler_d(jit_state_t _jit, jit_gpr_t r0, jit_fpr_t f0, jit_fpr_t f1)
{
    if (jit_sse_reg_p(f0))
	sse_ler_d(_jit, r0, f0, f1);
    else
	x87_ler_d(_jit, r0, f0, f1);
}

#define jit_eqr_f(r0, f0, f1)		x86_eqr_f(_jit, r0, f0, f1)
__jit_inline void
x86_eqr_f(jit_state_t _jit, jit_gpr_t r0, jit_fpr_t f0, jit_fpr_t f1)
{
    if (jit_sse_reg_p(f0))
	sse_eqr_f(_jit, r0, f0, f1);
    else
	x87_eqr_d(_jit, r0, f0, f1);
}

#define jit_eqr_d(r0, f0, f1)		x86_eqr_d(_jit, r0, f0, f1)
__jit_inline void
x86_eqr_d(jit_state_t _jit, jit_gpr_t r0, jit_fpr_t f0, jit_fpr_t f1)
{
    if (jit_sse_reg_p(f0))
	sse_eqr_d(_jit, r0, f0, f1);
    else
	x87_eqr_d(_jit, r0, f0, f1);
}

#define jit_ger_f(r0, f0, f1)		x86_ger_f(_jit, r0, f0, f1)
__jit_inline void
x86_ger_f(jit_state_t _jit, jit_gpr_t r0, jit_fpr_t f0, jit_fpr_t f1)
{
    if (jit_sse_reg_p(f0))
	sse_ger_f(_jit, r0, f0, f1);
    else
	x87_ger_d(_jit, r0, f0, f1);
}

#define jit_ger_d(r0, f0, f1)		x86_ger_d(_jit, r0, f0, f1)
__jit_inline void
x86_ger_d(jit_state_t _jit, jit_gpr_t r0, jit_fpr_t f0, jit_fpr_t f1)
{
    if (jit_sse_reg_p(f0))
	sse_ger_d(_jit, r0, f0, f1);
    else
	x87_ger_d(_jit, r0, f0, f1);
}

#define jit_gtr_f(r0, f0, f1)		x86_gtr_f(_jit, r0, f0, f1)
__jit_inline void
x86_gtr_f(jit_state_t _jit, jit_gpr_t r0, jit_fpr_t f0, jit_fpr_t f1)
{
    if (jit_sse_reg_p(f0))
	sse_gtr_f(_jit, r0, f0, f1);
    else
	x87_gtr_d(_jit, r0, f0, f1);
}

#define jit_gtr_d(r0, f0, f1)		x86_gtr_d(_jit, r0, f0, f1)
__jit_inline void
x86_gtr_d(jit_state_t _jit, jit_gpr_t r0, jit_fpr_t f0, jit_fpr_t f1)
{
    if (jit_sse_reg_p(f0))
	sse_gtr_d(_jit, r0, f0, f1);
    else
	x87_gtr_d(_jit, r0, f0, f1);
}

#define jit_ner_f(r0, f0, f1)		x86_ner_f(_jit, r0, f0, f1)
__jit_inline void
x86_ner_f(jit_state_t _jit, jit_gpr_t r0, jit_fpr_t f0, jit_fpr_t f1)
{
    if (jit_sse_reg_p(f0))
	sse_ner_f(_jit, r0, f0, f1);
    else
	x87_ner_d(_jit, r0, f0, f1);
}

#define jit_ner_d(r0, f0, f1)		x86_ner_d(_jit, r0, f0, f1)
__jit_inline void
x86_ner_d(jit_state_t _jit, jit_gpr_t r0, jit_fpr_t f0, jit_fpr_t f1)
{
    if (jit_sse_reg_p(f0))
	sse_ner_d(_jit, r0, f0, f1);
    else
	x87_ner_d(_jit, r0, f0, f1);
}

#define jit_unltr_f(r0, f0, f1)		x86_unltr_f(_jit, r0, f0, f1)
__jit_inline void
x86_unltr_f(jit_state_t _jit, jit_gpr_t r0, jit_fpr_t f0, jit_fpr_t f1)
{
    if (jit_sse_reg_p(f0))
	sse_unltr_f(_jit, r0, f0, f1);
    else
	x87_unltr_d(_jit, r0, f0, f1);
}

#define jit_unltr_d(r0, f0, f1)		x86_unltr_d(_jit, r0, f0, f1)
__jit_inline void
x86_unltr_d(jit_state_t _jit, jit_gpr_t r0, jit_fpr_t f0, jit_fpr_t f1)
{
    if (jit_sse_reg_p(f0))
	sse_unltr_d(_jit, r0, f0, f1);
    else
	x87_unltr_d(_jit, r0, f0, f1);
}

#define jit_unler_f(r0, f0, f1)		x86_unler_f(_jit, r0, f0, f1)
__jit_inline void
x86_unler_f(jit_state_t _jit, jit_gpr_t r0, jit_fpr_t f0, jit_fpr_t f1)
{
    if (jit_sse_reg_p(f0))
	sse_unler_f(_jit, r0, f0, f1);
    else
	x87_unler_d(_jit, r0, f0, f1);
}

#define jit_unler_d(r0, f0, f1)		x86_unler_d(_jit, r0, f0, f1)
__jit_inline void
x86_unler_d(jit_state_t _jit, jit_gpr_t r0, jit_fpr_t f0, jit_fpr_t f1)
{
    if (jit_sse_reg_p(f0))
	sse_unler_d(_jit, r0, f0, f1);
    else
	x87_unler_d(_jit, r0, f0, f1);
}

#define jit_ltgtr_f(r0, f0, f1)		x86_ltgtr_f(_jit, r0, f0, f1)
__jit_inline void
x86_ltgtr_f(jit_state_t _jit, jit_gpr_t r0, jit_fpr_t f0, jit_fpr_t f1)
{
    if (jit_sse_reg_p(f0))
	sse_ltgtr_f(_jit, r0, f0, f1);
    else
	x87_ltgtr_d(_jit, r0, f0, f1);
}

#define jit_ltgtr_d(r0, f0, f1)		x86_ltgtr_d(_jit, r0, f0, f1)
__jit_inline void
x86_ltgtr_d(jit_state_t _jit, jit_gpr_t r0, jit_fpr_t f0, jit_fpr_t f1)
{
    if (jit_sse_reg_p(f0))
	sse_ltgtr_d(_jit, r0, f0, f1);
    else
	x87_ltgtr_d(_jit, r0, f0, f1);
}

#define jit_uneqr_f(r0, f0, f1)		x86_uneqr_f(_jit, r0, f0, f1)
__jit_inline void
x86_uneqr_f(jit_state_t _jit, jit_gpr_t r0, jit_fpr_t f0, jit_fpr_t f1)
{
    if (jit_sse_reg_p(f0))
	sse_uneqr_f(_jit, r0, f0, f1);
    else
	x87_uneqr_d(_jit, r0, f0, f1);
}

#define jit_uneqr_d(r0, f0, f1)		x86_uneqr_d(_jit, r0, f0, f1)
__jit_inline void
x86_uneqr_d(jit_state_t _jit, jit_gpr_t r0, jit_fpr_t f0, jit_fpr_t f1)
{
    if (jit_sse_reg_p(f0))
	sse_uneqr_d(_jit, r0, f0, f1);
    else
	x87_uneqr_d(_jit, r0, f0, f1);
}

#define jit_unger_f(r0, f0, f1)		x86_unger_f(_jit, r0, f0, f1)
__jit_inline void
x86_unger_f(jit_state_t _jit, jit_gpr_t r0, jit_fpr_t f0, jit_fpr_t f1)
{
    if (jit_sse_reg_p(f0))
	sse_unger_f(_jit, r0, f0, f1);
    else
	x87_unger_d(_jit, r0, f0, f1);
}

#define jit_unger_d(r0, f0, f1)		x86_unger_d(_jit, r0, f0, f1)
__jit_inline void
x86_unger_d(jit_state_t _jit, jit_gpr_t r0, jit_fpr_t f0, jit_fpr_t f1)
{
    if (jit_sse_reg_p(f0))
	sse_unger_d(_jit, r0, f0, f1);
    else
	x87_unger_d(_jit, r0, f0, f1);
}

#define jit_ungtr_f(r0, f0, f1)		x86_ungtr_f(_jit, r0, f0, f1)
__jit_inline void
x86_ungtr_f(jit_state_t _jit, jit_gpr_t r0, jit_fpr_t f0, jit_fpr_t f1)
{
    if (jit_sse_reg_p(f0))
	sse_ungtr_f(_jit, r0, f0, f1);
    else
	x87_ungtr_d(_jit, r0, f0, f1);
}

#define jit_ungtr_d(r0, f0, f1)		x86_ungtr_d(_jit, r0, f0, f1)
__jit_inline void
x86_ungtr_d(jit_state_t _jit, jit_gpr_t r0, jit_fpr_t f0, jit_fpr_t f1)
{
    if (jit_sse_reg_p(f0))
	sse_ungtr_d(_jit, r0, f0, f1);
    else
	x87_ungtr_d(_jit, r0, f0, f1);
}

#define jit_ordr_f(r0, f0, f1)		x86_ordr_f(_jit, r0, f0, f1)
__jit_inline void
x86_ordr_f(jit_state_t _jit, jit_gpr_t r0, jit_fpr_t f0, jit_fpr_t f1)
{
    if (jit_sse_reg_p(f0))
	sse_ordr_f(_jit, r0, f0, f1);
    else
	x87_ordr_d(_jit, r0, f0, f1);
}

#define jit_ordr_d(r0, f0, f1)		x86_ordr_d(_jit, r0, f0, f1)
__jit_inline void
x86_ordr_d(jit_state_t _jit, jit_gpr_t r0, jit_fpr_t f0, jit_fpr_t f1)
{
    if (jit_sse_reg_p(f0))
	sse_ordr_d(_jit, r0, f0, f1);
    else
	x87_ordr_d(_jit, r0, f0, f1);
}

#define jit_unordr_f(r0, f0, f1)	x86_unordr_f(_jit, r0, f0, f1)
__jit_inline void
x86_unordr_f(jit_state_t _jit, jit_gpr_t r0, jit_fpr_t f0, jit_fpr_t f1)
{
    if (jit_sse_reg_p(f0))
	sse_unordr_f(_jit, r0, f0, f1);
    else
	x87_unordr_d(_jit, r0, f0, f1);
}

#define jit_unordr_d(r0, f0, f1)	x86_unordr_d(_jit, r0, f0, f1)
__jit_inline void
x86_unordr_d(jit_state_t _jit, jit_gpr_t r0, jit_fpr_t f0, jit_fpr_t f1)
{
    if (jit_sse_reg_p(f0))
	sse_unordr_d(_jit, r0, f0, f1);
    else
	x87_unordr_d(_jit, r0, f0, f1);
}

#define jit_bltr_f(label, f0, f1)	x86_bltr_f(_jit, label, f0, f1)
__jit_inline jit_insn *
x86_bltr_f(jit_state_t _jit, jit_insn *label, jit_fpr_t f0, jit_fpr_t f1)
{
    if (jit_sse_reg_p(f0))
	return (sse_bltr_f(_jit, label, f0, f1));
    return (x87_bltr_d(_jit, label, f0, f1));
}

#define jit_bltr_d(label, f0, f1)	x86_bltr_d(_jit, label, f0, f1)
__jit_inline jit_insn *
x86_bltr_d(jit_state_t _jit, jit_insn *label, jit_fpr_t f0, jit_fpr_t f1)
{
    if (jit_sse_reg_p(f0))
	return (sse_bltr_d(_jit, label, f0, f1));
    return (x87_bltr_d(_jit, label, f0, f1));
}

#define jit_bler_f(label, f0, f1)	x86_bler_f(_jit, label, f0, f1)
__jit_inline jit_insn *
x86_bler_f(jit_state_t _jit, jit_insn *label, jit_fpr_t f0, jit_fpr_t f1)
{
    if (jit_sse_reg_p(f0))
	return (sse_bler_f(_jit, label, f0, f1));
    return (x87_bler_d(_jit, label, f0, f1));
}

#define jit_bler_d(label, f0, f1)	x86_bler_d(_jit, label, f0, f1)
__jit_inline jit_insn *
x86_bler_d(jit_state_t _jit, jit_insn *label, jit_fpr_t f0, jit_fpr_t f1)
{
    if (jit_sse_reg_p(f0))
	return (sse_bler_d(_jit, label, f0, f1));
    return (x87_bler_d(_jit, label, f0, f1));
}

#define jit_beqr_f(label, f0, f1)	x86_beqr_f(_jit, label, f0, f1)
__jit_inline jit_insn *
x86_beqr_f(jit_state_t _jit, jit_insn *label, jit_fpr_t f0, jit_fpr_t f1)
{
    if (jit_sse_reg_p(f0))
	return (sse_beqr_f(_jit, label, f0, f1));
    return (x87_beqr_d(_jit, label, f0, f1));
}

#define jit_beqr_d(label, f0, f1)	x86_beqr_d(_jit, label, f0, f1)
__jit_inline jit_insn *
x86_beqr_d(jit_state_t _jit, jit_insn *label, jit_fpr_t f0, jit_fpr_t f1)
{
    if (jit_sse_reg_p(f0))
	return (sse_beqr_d(_jit, label, f0, f1));
    return (x87_beqr_d(_jit, label, f0, f1));
}

#define jit_bger_f(label, f0, f1)	x86_bger_f(_jit, label, f0, f1)
__jit_inline jit_insn *
x86_bger_f(jit_state_t _jit, jit_insn *label, jit_fpr_t f0, jit_fpr_t f1)
{
    if (jit_sse_reg_p(f0))
	return (sse_bger_f(_jit, label, f0, f1));
    return (x87_bger_d(_jit, label, f0, f1));
}

#define jit_bger_d(label, f0, f1)	x86_bger_d(_jit, label, f0, f1)
__jit_inline jit_insn *
x86_bger_d(jit_state_t _jit, jit_insn *label, jit_fpr_t f0, jit_fpr_t f1)
{
    if (jit_sse_reg_p(f0))
	return (sse_bger_d(_jit, label, f0, f1));
    return (x87_bger_d(_jit, label, f0, f1));
}

#define jit_bgtr_f(label, f0, f1)	x86_bgtr_f(_jit, label, f0, f1)
__jit_inline jit_insn *
x86_bgtr_f(jit_state_t _jit, jit_insn *label, jit_fpr_t f0, jit_fpr_t f1)
{
    if (jit_sse_reg_p(f0))
	return (sse_bgtr_f(_jit, label, f0, f1));
    return (x87_bgtr_d(_jit, label, f0, f1));
}

#define jit_bgtr_d(label, f0, f1)	x86_bgtr_d(_jit, label, f0, f1)
__jit_inline jit_insn *
x86_bgtr_d(jit_state_t _jit, jit_insn *label, jit_fpr_t f0, jit_fpr_t f1)
{
    if (jit_sse_reg_p(f0))
	return (sse_bgtr_d(_jit, label, f0, f1));
    return (x87_bgtr_d(_jit, label, f0, f1));
}

#define jit_bner_f(label, f0, f1)	x86_bner_f(_jit, label, f0, f1)
__jit_inline jit_insn *
x86_bner_f(jit_state_t _jit, jit_insn *label, jit_fpr_t f0, jit_fpr_t f1)
{
    if (jit_sse_reg_p(f0))
	return (sse_bner_f(_jit, label, f0, f1));
    return (x87_bner_d(_jit, label, f0, f1));
}

#define jit_bner_d(label, f0, f1)	x86_bner_d(_jit, label, f0, f1)
__jit_inline jit_insn *
x86_bner_d(jit_state_t _jit, jit_insn *label, jit_fpr_t f0, jit_fpr_t f1)
{
    if (jit_sse_reg_p(f0))
	return (sse_bner_d(_jit, label, f0, f1));
    return (x87_bner_d(_jit, label, f0, f1));
}

#define jit_bunltr_f(label, f0, f1)	x86_bunltr_f(_jit, label, f0, f1)
__jit_inline jit_insn *
x86_bunltr_f(jit_state_t _jit, jit_insn *label, jit_fpr_t f0, jit_fpr_t f1)
{
    if (jit_sse_reg_p(f0))
	return (sse_bunltr_f(_jit, label, f0, f1));
    return (x87_bunltr_d(_jit, label, f0, f1));
}

#define jit_bunltr_d(label, f0, f1)	x86_bunltr_d(_jit, label, f0, f1)
__jit_inline jit_insn *
x86_bunltr_d(jit_state_t _jit, jit_insn *label, jit_fpr_t f0, jit_fpr_t f1)
{
    if (jit_sse_reg_p(f0))
	return (sse_bunltr_d(_jit, label, f0, f1));
    return (x87_bunltr_d(_jit, label, f0, f1));
}

#define jit_bunler_f(label, f0, f1)	x86_bunler_f(_jit, label, f0, f1)
__jit_inline jit_insn *
x86_bunler_f(jit_state_t _jit, jit_insn *label, jit_fpr_t f0, jit_fpr_t f1)
{
    if (jit_sse_reg_p(f0))
	return (sse_bunler_f(_jit, label, f0, f1));
    return (x87_bunler_d(_jit, label, f0, f1));
}

#define jit_bunler_d(label, f0, f1)	x86_bunler_d(_jit, label, f0, f1)
__jit_inline jit_insn *
x86_bunler_d(jit_state_t _jit, jit_insn *label, jit_fpr_t f0, jit_fpr_t f1)
{
    if (jit_sse_reg_p(f0))
	return (sse_bunler_d(_jit, label, f0, f1));
    return (x87_bunler_d(_jit, label, f0, f1));
}

#define jit_bltgtr_f(label, f0, f1)	x86_bltgtr_f(_jit, label, f0, f1)
__jit_inline jit_insn *
x86_bltgtr_f(jit_state_t _jit, jit_insn *label, jit_fpr_t f0, jit_fpr_t f1)
{
    if (jit_sse_reg_p(f0))
	return (sse_bltgtr_f(_jit, label, f0, f1));
    return (x87_bltgtr_d(_jit, label, f0, f1));
}

#define jit_bltgtr_d(label, f0, f1)	x86_bltgtr_d(_jit, label, f0, f1)
__jit_inline jit_insn *
x86_bltgtr_d(jit_state_t _jit, jit_insn *label, jit_fpr_t f0, jit_fpr_t f1)
{
    if (jit_sse_reg_p(f0))
	return (sse_bltgtr_d(_jit, label, f0, f1));
    return (x87_bltgtr_d(_jit, label, f0, f1));
}

#define jit_buneqr_f(label, f0, f1)	x86_buneqr_f(_jit, label, f0, f1)
__jit_inline jit_insn *
x86_buneqr_f(jit_state_t _jit, jit_insn *label, jit_fpr_t f0, jit_fpr_t f1)
{
    if (jit_sse_reg_p(f0))
	return (sse_buneqr_f(_jit, label, f0, f1));
    return (x87_buneqr_d(_jit, label, f0, f1));
}

#define jit_buneqr_d(label, f0, f1)	x86_buneqr_d(_jit, label, f0, f1)
__jit_inline jit_insn *
x86_buneqr_d(jit_state_t _jit, jit_insn *label, jit_fpr_t f0, jit_fpr_t f1)
{
    if (jit_sse_reg_p(f0))
	return (sse_buneqr_d(_jit, label, f0, f1));
    return (x87_buneqr_d(_jit, label, f0, f1));
}

#define jit_bunger_f(label, f0, f1)	x86_bunger_f(_jit, label, f0, f1)
__jit_inline jit_insn *
x86_bunger_f(jit_state_t _jit, jit_insn *label, jit_fpr_t f0, jit_fpr_t f1)
{
    if (jit_sse_reg_p(f0))
	return (sse_bunger_f(_jit, label, f0, f1));
    return (x87_bunger_d(_jit, label, f0, f1));
}

#define jit_bunger_d(label, f0, f1)	x86_bunger_d(_jit, label, f0, f1)
__jit_inline jit_insn *
x86_bunger_d(jit_state_t _jit, jit_insn *label, jit_fpr_t f0, jit_fpr_t f1)
{
    if (jit_sse_reg_p(f0))
	return (sse_bunger_d(_jit, label, f0, f1));
    return (x87_bunger_d(_jit, label, f0, f1));
}

#define jit_bungtr_f(label, f0, f1)	x86_bungtr_f(_jit, label, f0, f1)
__jit_inline jit_insn *
x86_bungtr_f(jit_state_t _jit, jit_insn *label, jit_fpr_t f0, jit_fpr_t f1)
{
    if (jit_sse_reg_p(f0))
	return (sse_bungtr_f(_jit, label, f0, f1));
    return (x87_bungtr_d(_jit, label, f0, f1));
}

#define jit_bungtr_d(label, f0, f1)	x86_bungtr_d(_jit, label, f0, f1)
__jit_inline jit_insn *
x86_bungtr_d(jit_state_t _jit, jit_insn *label, jit_fpr_t f0, jit_fpr_t f1)
{
    if (jit_sse_reg_p(f0))
	return (sse_bungtr_d(_jit, label, f0, f1));
    return (x87_bungtr_d(_jit, label, f0, f1));
}

#define jit_bordr_f(label, f0, f1)	x86_bordr_f(_jit, label, f0, f1)
__jit_inline jit_insn *
x86_bordr_f(jit_state_t _jit, jit_insn *label, jit_fpr_t f0, jit_fpr_t f1)
{
    if (jit_sse_reg_p(f0))
	return (sse_bordr_f(_jit, label, f0, f1));
    return (x87_bordr_d(_jit, label, f0, f1));
}

#define jit_bordr_d(label, f0, f1)	x86_bordr_d(_jit, label, f0, f1)
__jit_inline jit_insn *
x86_bordr_d(jit_state_t _jit, jit_insn *label, jit_fpr_t f0, jit_fpr_t f1)
{
    if (jit_sse_reg_p(f0))
	return (sse_bordr_d(_jit, label, f0, f1));
    return (x87_bordr_d(_jit, label, f0, f1));
}

#define jit_bunordr_f(label, f0, f1)	x86_bunordr_f(_jit, label, f0, f1)
__jit_inline jit_insn *
x86_bunordr_f(jit_state_t _jit, jit_insn *label, jit_fpr_t f0, jit_fpr_t f1)
{
    if (jit_sse_reg_p(f0))
	return (sse_bunordr_f(_jit, label, f0, f1));
    return (x87_bunordr_d(_jit, label, f0, f1));
}

#define jit_bunordr_d(label, f0, f1)	x86_bunordr_d(_jit, label, f0, f1)
__jit_inline jit_insn *
x86_bunordr_d(jit_state_t _jit, jit_insn *label, jit_fpr_t f0, jit_fpr_t f1)
{
    if (jit_sse_reg_p(f0))
	return (sse_bunordr_d(_jit, label, f0, f1));
    return (x87_bunordr_d(_jit, label, f0, f1));
}

#if __WORDSIZE == 64
#  include "fp-64.h"
#else
#  include "fp-32.h"
#endif

#endif /* __lightning_fp_i386_h */
