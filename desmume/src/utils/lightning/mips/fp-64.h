/******************************** -*- C -*- ****************************
 *
 *	Floating-point support (mips64)
 *
 ***********************************************************************/

/***********************************************************************
 *
 * Copyright 2010 Free Software Foundation, Inc.
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
 *	Paulo Cesar Pereira de Andrade
 ***********************************************************************/

#ifndef __lightning_fp_h
#define __lightning_fp_h

#define jit_movi_d(f0, i0)		mips_movi_d(_jit, f0, i0)
__jit_inline void
mips_movi_d(jit_state_t _jit, jit_fpr_t f0, double i0)
{
    union {
	int	i[2];
	long	l;
	double	d;
    } data;

    data.d = i0;
    jit_movi_l(JIT_RTEMP, data.l);
    _DMTC1(JIT_RTEMP, f0);
}

#define jit_extr_l_f(f0, r0)		mips_extr_l_f(_jit, f0, r0)
__jit_inline void
mips_extr_l_f(jit_state_t _jit, jit_fpr_t f0, jit_gpr_t r0)
{
    _DMTC1(r0, JIT_FTMP0);
    _CVT_S_L(f0, JIT_FTMP0);
}

#define jit_extr_l_d(f0, r0)		mips_extr_l_d(_jit, f0, r0)
__jit_inline void
mips_extr_l_d(jit_state_t _jit, jit_fpr_t f0, jit_gpr_t r0)
{
    _DMTC1(r0, JIT_FTMP0);
    _CVT_D_L(f0, JIT_FTMP0);
}

#define jit_rintr_f_l(r0, f0)		mips_rintr_f_l(_jit, r0, f0)
__jit_inline void
mips_rintr_f_l(jit_state_t _jit, jit_gpr_t r0, jit_fpr_t f0)
{
    _CVT_L_S(JIT_FTMP0, f0);
    _DMFC1(r0, JIT_FTMP0);
}

#define jit_rintr_d_l(r0, f0)		mips_rintr_d_l(_jit, r0, f0)
__jit_inline void
mips_rintr_d_l(jit_state_t _jit, jit_gpr_t r0, jit_fpr_t f0)
{
    _CVT_L_D(JIT_FTMP0, f0);
    _DMFC1(r0, JIT_FTMP0);
}

#define jit_roundr_f_l(r0, f0)		mips_roundr_f_l(_jit, r0, f0)
__jit_inline void
mips_roundr_f_l(jit_state_t _jit, jit_gpr_t r0, jit_fpr_t f0)
{
#if 0	/* if round to nearest */
    _ROUND_L_S(JIT_FTMP0, f0);
#else
    jit_insn	*l;
    _MTC1(JIT_RZERO, (jit_fpr_t)JIT_FTMP0);
    jit_movi_i(JIT_RTEMP, 0xbf000000);
    _MTC1(JIT_RTEMP, JIT_FTMP1);
    _C_OLT_S(JIT_FTMP1, f0);
    l = _jit->x.pc;
    _BC1T(0);
    jit_nop(1);
    _NEG_S(JIT_FTMP1, JIT_FTMP1);
    jit_patch(l);
    _ADD_S(JIT_FTMP0, f0, JIT_FTMP1);
    _TRUNC_L_S(JIT_FTMP0, JIT_FTMP0);
#endif
    _DMFC1(r0, JIT_FTMP0);
}

#define jit_roundr_d_l(r0, f0)		mips_roundr_d_l(_jit, r0, f0)
__jit_inline void
mips_roundr_d_l(jit_state_t _jit, jit_gpr_t r0, jit_fpr_t f0)
{
#if 0	/* if round to nearest */
    _ROUND_L_D(JIT_FTMP0, f0);
#else
    jit_insn	*l;
    _MTC1(JIT_RZERO, (jit_fpr_t)JIT_FTMP0);
    _MTC1(JIT_RZERO, (jit_fpr_t)(JIT_FTMP0 + 1));
    jit_movi_i(JIT_RTEMP, 0xbfe00000);
#if __BYTEORDER == __LITTLE_ENDIAN
    _MTC1(JIT_RTEMP, JIT_FTMP1);
    _MTC1(JIT_RZERO, (jit_fpr_t)(JIT_FTMP1 + 1));
#else
    _MTC1(JIT_RZERO, JIT_FTMP1);
    _MTC1(JIT_RTEMP, (jit_fpr_t)(JIT_FTMP1 + 1));
#endif
    _C_OLT_D(JIT_FTMP1, f0);
    l = _jit->x.pc;
    _BC1T(0);
    jit_nop(1);
    _NEG_D(JIT_FTMP1, JIT_FTMP1);
    jit_patch(l);
    _ADD_D(JIT_FTMP0, f0, JIT_FTMP1);
    _TRUNC_L_D(JIT_FTMP0, JIT_FTMP0);
#endif
    _DMFC1(r0, JIT_FTMP0);
}

#define jit_truncr_f_l(r0, f0)		mips_truncr_f_l(_jit, r0, f0)
__jit_inline void
mips_truncr_f_l(jit_state_t _jit, jit_gpr_t r0, jit_fpr_t f0)
{
    _TRUNC_L_S(JIT_FTMP0, f0);
    _DMFC1(r0, JIT_FTMP0);
}

#define jit_truncr_d_l(r0, f0)		mips_truncr_d_l(_jit, r0, f0)
__jit_inline void
mips_truncr_d_l(jit_state_t _jit, jit_gpr_t r0, jit_fpr_t f0)
{
    _TRUNC_L_D(JIT_FTMP0, f0);
    _DMFC1(r0, JIT_FTMP0);
}

#define jit_ceilr_f_l(r0, f0)		mips_ceilr_f_l(_jit, r0, f0)
__jit_inline void
mips_ceilr_f_l(jit_state_t _jit, jit_gpr_t r0, jit_fpr_t f0)
{
    _CEIL_L_S(JIT_FTMP0, f0);
    _DMFC1(r0, JIT_FTMP0);
}

#define jit_ceilr_d_l(r0, f0)		mips_ceilr_d_l(_jit, r0, f0)
__jit_inline void
mips_ceilr_d_l(jit_state_t _jit, jit_gpr_t r0, jit_fpr_t f0)
{
    _CEIL_L_D(JIT_FTMP0, f0);
    _DMFC1(r0, JIT_FTMP0);
}

#define jit_floorr_f_l(r0, f0)		mips_floorr_f_l(_jit, r0, f0)
__jit_inline void
mips_floorr_f_l(jit_state_t _jit, jit_gpr_t r0, jit_fpr_t f0)
{
    _FLOOR_L_S(JIT_FTMP0, f0);
    _DMFC1(r0, JIT_FTMP0);
}

#define jit_floorr_d_l(r0, f0)		mips_floorr_d_l(_jit, r0, f0)
__jit_inline void
mips_floorr_d_l(jit_state_t _jit, jit_gpr_t r0, jit_fpr_t f0)
{
    _FLOOR_D_S(JIT_FTMP0, f0);
    _DMFC1(r0, JIT_FTMP0);
}

#define jit_prepare_d(count)		mips_prepare_d(_jit, count)
#define jit_prepare_f(count)		mips_prepare_d(_jit, count)
__jit_inline void
mips_prepare_d(jit_state_t _jit, int count)
{
    assert(count >= 0);
    _jitl.nextarg_put += count;
    _jitl.stack_offset = _jitl.nextarg_put << 3;
    if (_jitl.stack_length < _jitl.stack_offset) {
	_jitl.stack_length = _jitl.stack_offset;
	mips_set_stack(_jit, (_jitl.alloca_offset +
			      _jitl.stack_length + 7) & ~7);
    }
}

#define jit_arg_d()			mips_arg_d(_jit)
#define jit_arg_f()			mips_arg_d(_jit)
__jit_inline int
mips_arg_d(jit_state_t _jit)
{
    int		ofs;
    int		reg;

    reg = (_jitl.framesize - JIT_FRAMESIZE) >> 3;
    if (reg < JIT_A_NUM)
	ofs = reg;
    else
	ofs = _jitl.framesize;
    _jitl.framesize += sizeof(float);

    return (ofs);
}

#define jit_getarg_d(f0, ofs)		mips_getarg_d(_jit, f0, ofs)
__jit_inline void
mips_getarg_d(jit_state_t _jit, jit_fpr_t f0, int ofs)
{
    if (ofs < JIT_A_NUM)
	jit_movr_d(f0, jit_fa_order[ofs]);
    else
	jit_ldxi_d(f0, JIT_FP, ofs);
}

#define jit_getarg_f(f0, ofs)		mips_getarg_f(_jit, f0, ofs)
__jit_inline void
mips_getarg_f(jit_state_t _jit, jit_fpr_t f0, int ofs)
{
    if (ofs < JIT_A_NUM)
	jit_movr_f(f0, jit_fa_order[ofs]);
    else
	jit_ldxi_f(f0, JIT_FP, ofs);
}

#define jit_pusharg_d(f0)		mips_pusharg_d(_jit, f0)
__jit_inline void
mips_pusharg_d(jit_state_t _jit, jit_fpr_t f0)
{
    _jitl.nextarg_put -= 1;
    _jitl.stack_offset -= sizeof(double);
    assert(_jitl.nextarg_put	>= 0 &&
	   _jitl.stack_offset	>= 0);
    if (_jitl.nextarg_put >= JIT_A_NUM)
	jit_stxi_d(_jitl.stack_offset, JIT_SP, f0);
    else {
	jit_movr_d(jit_fa_order[_jitl.nextarg_put], f0);
	_MFC1(jit_a_order[_jitl.nextarg_put], f0);
	_MFC1(jit_a_order[_jitl.nextarg_put + 1], (jit_fpr_t)(f0 + 1));
    }
}

#define jit_pusharg_f(f0)		mips_pusharg_f(_jit, f0)
__jit_inline void
mips_pusharg_f(jit_state_t _jit, jit_fpr_t f0)
{
    _jitl.nextarg_put -= 1;
    _jitl.stack_offset -= sizeof(double);
    assert(_jitl.nextarg_put	>= 0 &&
	   _jitl.stack_offset	>= 0);
    if (_jitl.nextarg_put >= JIT_A_NUM)
	jit_stxi_f(_jitl.stack_offset, JIT_SP, f0);
    else {
	jit_movr_f(jit_fa_order[_jitl.nextarg_put], f0);
	_MFC1(jit_a_order[_jitl.nextarg_put], f0);
    }
}

#endif /* __lightning_fp_h */
