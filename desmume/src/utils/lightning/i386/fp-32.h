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


#ifndef __lightning_fp_h
#define __lightning_fp_h

#define JIT_FPRET			_ST0

#define JIT_FPR_NUM			6
static const jit_fpr_t
jit_x87_order[JIT_FPR_NUM] = {
    _ST0, _ST1, _ST2, _ST3, _ST4, _ST5
};
static const jit_fpr_t
jit_sse_order[JIT_FPR_NUM] = {
    _XMM0, _XMM1, _XMM2, _XMM3, _XMM4, _XMM5
};
#define JIT_FPR(i)							\
    (jit_sse2_p() ? jit_sse_order[i] : jit_x87_order[i])

#define jit_retval_f(f0)		x86_retval_f(_jit, f0)
__jit_inline void
x86_retval_f(jit_state_t _jit, jit_fpr_t f0)
{
    if (jit_sse_reg_p(f0)) {
	FSTPr(_ST1);
	sse_from_x87_f(_jit, f0, JIT_FPRET);
    }
    else
	FSTPr((jit_fpr_t)(f0 + 1));
}

#define jit_retval_d(f0)		x86_retval_d(_jit, f0)
__jit_inline void
x86_retval_d(jit_state_t _jit, jit_fpr_t f0)
{
    if (jit_sse_reg_p(f0)) {
	FSTPr(_ST1);
	sse_from_x87_d(_jit, f0, JIT_FPRET);
    }
    else
	FSTPr((jit_fpr_t)(f0 + 1));
}

#define jit_pusharg_f(f0)		x86_pusharg_f(_jit, f0)
__jit_inline void
x86_pusharg_f(jit_state_t _jit, jit_fpr_t f0)
{
    _jitl.stack_offset -= sizeof(float);
    assert(_jitl.stack_offset >= 0);
    if (jit_push_pop_p()) {
	int	pad = -_jitl.stack_length & 15;
	_jitl.stack_length += pad;
	jit_subi_l(JIT_SP, JIT_SP, pad + sizeof(float));
	jit_str_f(JIT_SP, f0);
    }
    else
	jit_stxi_f(_jitl.stack_offset, JIT_SP, f0);
}

#define jit_pusharg_d(f0)		x86_pusharg_d(_jit, f0)
__jit_inline void
x86_pusharg_d(jit_state_t _jit, jit_fpr_t f0)
{
    _jitl.stack_offset -= sizeof(double);
    assert(_jitl.stack_offset >= 0);
    if (jit_push_pop_p()) {
	int	pad = -_jitl.stack_length & 15;
	_jitl.stack_length += pad;
	jit_subi_l(JIT_SP, JIT_SP, pad + sizeof(double));
	jit_str_d(JIT_SP, f0);
    }
    else
	jit_stxi_d(_jitl.stack_offset, JIT_SP, f0);
}

#define jit_prepare_f(nf)		x86_prepare_f(_jit, nf)
__jit_inline void
x86_prepare_f(jit_state_t _jit, int count)
{
    assert(count >= 0);
    _jitl.stack_offset += count << 2;
    if (jit_push_pop_p())
	_jitl.stack_length = _jitl.stack_offset;
    else  if (_jitl.stack_length < _jitl.stack_offset) {
	_jitl.stack_length = _jitl.stack_offset;
	*_jitl.stack = 12 + ((_jitl.alloca_offset +
			      _jitl.stack_length + 15) & ~15);
    }
}

#define jit_prepare_d(nd)		x86_prepare_d(_jit, nd)
__jit_inline void
x86_prepare_d(jit_state_t _jit, int count)
{
    assert(count >= 0);
    _jitl.stack_offset += count << 3;
    if (jit_push_pop_p())
	_jitl.stack_length = _jitl.stack_offset;
    else  if (_jitl.stack_length < _jitl.stack_offset) {
	_jitl.stack_length = _jitl.stack_offset;
	*_jitl.stack = 12 + ((_jitl.alloca_offset +
			      _jitl.stack_length + 15) & ~15);
    }
}

#define jit_arg_f()			x86_arg_f(_jit)
__jit_inline int
x86_arg_f(jit_state_t _jit)
{
    int		ofs = _jitl.framesize;
    _jitl.framesize += sizeof(float);
    return (ofs);
}

#define jit_arg_d()			x86_arg_d(_jit)
__jit_inline int
x86_arg_d(jit_state_t _jit)
{
    int		ofs = _jitl.framesize;
    _jitl.framesize += sizeof(double);
    return (ofs);
}

#define jit_getarg_f(f0, ofs)		x86_getarg_f(_jit, f0, ofs)
__jit_inline void
x86_getarg_f(jit_state_t _jit, jit_fpr_t f0, int ofs)
{
    jit_ldxi_f(f0, JIT_FP, ofs);
}

#define jit_getarg_d(f0, ofs)		x86_getarg_d(_jit, f0, ofs)
__jit_inline void
x86_getarg_d(jit_state_t _jit, jit_fpr_t f0, int ofs)
{
    jit_ldxi_d(f0, JIT_FP, ofs);
}

#endif /* __lightning_fp_h */
