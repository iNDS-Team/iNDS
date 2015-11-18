/******************************** -*- C -*- ****************************
 *
 *	Floating-point support (mips)
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

#ifndef __lightning_fp_mips_h
#define __lightning_fp_mips_h

#define jit_movf_p()			jit_cpu.movf
#define jit_aligned_double_p()		jit_cpu.algndbl
#define next_fpr(fn)			((jit_fpr_t)((fn) + 1))


#define JIT_FPR_NUM			6
static const jit_fpr_t
jit_f_order[JIT_FPR_NUM] = {
    _F0, _F2,  _F4,  _F6, _F8, _F10
};

#define JIT_FPR(n)			jit_f_order[n]

#define JIT_FPRET			_F0

#define JIT_FTMP0			_F28
#define JIT_FTMP1			_F30

#define jit_addr_f(f0, f1, f2)		_ADD_S(f0, f1, f2)
#define jit_addr_d(f0, f1, f2)		_ADD_D(f0, f1, f2)
#define jit_subr_f(f0, f1, f2)		_SUB_S(f0, f1, f2)
#define jit_subr_d(f0, f1, f2)		_SUB_D(f0, f1, f2)
#define jit_mulr_f(f0, f1, f2)		_MUL_S(f0, f1, f2)
#define jit_mulr_d(f0, f1, f2)		_MUL_D(f0, f1, f2)
#define jit_divr_f(f0, f1, f2)		_DIV_S(f0, f1, f2)
#define jit_divr_d(f0, f1, f2)		_DIV_D(f0, f1, f2)
#define jit_sqrtr_f(f0, f1)		_SQRT_S(f0, f1)
#define jit_sqrtr_d(f0, f1)		_SQRT_D(f0, f1)
#define jit_absr_f(f0, f1)		_ABS_S(f0, f1)
#define jit_absr_d(f0, f1)		_ABS_D(f0, f1)

#define jit_movr_f(f0, f1)		mips_movr_f(_jit, f0, f1)
__jit_inline void
mips_movr_f(jit_state_t _jit, jit_fpr_t f0, jit_fpr_t f1)
{
    if (f0 != f1)
	_MOV_S(f0, f1);
}

#define jit_movi_f(f0, i0)		mips_movi_f(_jit, f0, i0)
__jit_inline void
mips_movi_f(jit_state_t _jit, jit_fpr_t f0, float i0)
{
    union {
	int	i;
	float	f;
    } data;
    data.f = i0;
    if (data.i) {
	jit_movi_i(JIT_RTEMP, data.i);
	_MTC1(JIT_RTEMP, f0);
    }
    else
	_MTC1(JIT_RZERO, f0);
}

#define jit_movr_d(f0, f1)		mips_movr_d(_jit, f0, f1)
__jit_inline void
mips_movr_d(jit_state_t _jit, jit_fpr_t f0, jit_fpr_t f1)
{
    if (f0 != f1)
	_MOV_D(f0, f1);
}

#define jit_negr_f(f0, f1)	_NEG_S(f0, f1)
#define jit_negr_d(f0, f1)	_NEG_D(f0, f1)

#define jit_ldr_f(f0, r0)	mips_ldr_f(_jit, f0, r0)
__jit_inline void
mips_ldr_f(jit_state_t _jit, jit_fpr_t f0, jit_gpr_t r0)
{
    _LWC1(f0, 0, r0);
}

#define jit_ldr_d(f0, r0)	mips_ldr_d(_jit, f0, r0)
__jit_inline void
mips_ldr_d(jit_state_t _jit, jit_fpr_t f0, jit_gpr_t r0)
{
    if (jit_aligned_double_p())
	_LDC1(f0, 0, r0);
    else {
	_LWC1(f0, 0, r0);
	_LWC1(next_fpr(f0), 4, r0);
    }
}

#define jit_ldi_f(f0, i0)	mips_ldi_f(_jit, f0, i0)
__jit_inline void
mips_ldi_f(jit_state_t _jit, jit_fpr_t f0, void *i0)
{
    long	ds = (long)i0;

    if (_s16P(ds))
	_LWC1(f0, _jit_US(ds), JIT_RZERO);
    else {
	jit_movi_i(JIT_RTEMP, ds);
	_LWC1(f0, 0, JIT_RTEMP);
    }
}

#define jit_ldi_d(f0, i0)		mips_ldi_d(_jit, f0, i0)
__jit_inline void
mips_ldi_d(jit_state_t _jit, jit_fpr_t f0, void *i0)
{
    long	ds = (long)i0;

    if (jit_aligned_double_p()) {
	if (_s16P(ds))
	    _LDC1(f0, _jit_US(ds), JIT_RZERO);
	else {
	    jit_movi_i(JIT_RTEMP, ds);
	    _LDC1(f0, 0, JIT_RTEMP);
	}
    }
    else {
	if (_s16P(ds) && _s16P(ds + 4)) {
	    _LWC1(f0, _jit_US(ds), JIT_RZERO);
	    _LWC1(next_fpr(f0), _jit_US(ds + 4), JIT_RZERO);
	}
	else {
	    jit_movi_i(JIT_RTEMP, ds);
	    _LWC1(f0, 0, JIT_RTEMP);
	    _LWC1(next_fpr(f0), 4, JIT_RTEMP);
	}
    }
}

#define jit_ldxr_f(f0, r0, r1)		mips_ldxr_f(_jit, f0, r0, r1)
__jit_inline void
mips_ldxr_f(jit_state_t _jit, jit_fpr_t f0, jit_gpr_t r0, jit_gpr_t r1)
{
    if (jit_mips2_p())
	_LWXC1(f0, r1, r0);
    else {
	jit_addr_i(JIT_RTEMP, r0, r1);
	_LWC1(f0, 0, JIT_RTEMP);
    }
}

#define jit_ldxr_d(f0, r0, r1)		mips_ldxr_d(_jit, f0, r0, r1)
__jit_inline void
mips_ldxr_d(jit_state_t _jit, jit_fpr_t f0, jit_gpr_t r0, jit_gpr_t r1)
{
    if (jit_aligned_double_p()) {
	if (jit_mips2_p())
	    _LDXC1(f0, r1, r0);
	else {
	    jit_addr_i(JIT_RTEMP, r0, r1);
	    _LDC1(f0, 0, JIT_RTEMP);
	}
    }
    else {
	jit_addr_i(JIT_RTEMP, r0, r1);
	_LWC1(f0, 0, JIT_RTEMP);
	_LWC1(next_fpr(f0), 4, JIT_RTEMP);
    }
}

#define jit_ldxi_f(f0, r0, i0)		mips_ldxi_f(_jit, f0, r0, i0)
__jit_inline void
mips_ldxi_f(jit_state_t _jit, jit_fpr_t f0, jit_gpr_t r0, long i0)
{
    if (_s16P(i0))
	_LWC1(f0, _jit_US(i0), r0);
    else if (jit_mips2_p()) {
	jit_movi_i(JIT_RTEMP, i0);
	_LWXC1(f0, JIT_RTEMP, r0);
    }
    else {
	jit_addi_i(JIT_RTEMP, r0, i0);
	_LWC1(f0, 0, JIT_RTEMP);
    }
}

#define jit_ldxi_d(f0, r0, i0)		mips_ldxi_d(_jit, f0, r0, i0)
__jit_inline void
mips_ldxi_d(jit_state_t _jit, jit_fpr_t f0, jit_gpr_t r0, long i0)
{
    if (jit_aligned_double_p()) {
	if (_s16P(i0))
	    _LDC1(f0, _jit_US(i0), r0);
	else if (jit_mips2_p()) {
	    jit_movi_i(JIT_RTEMP, i0);
	    _LDXC1(f0, JIT_RTEMP, r0);
	}
	else {
	    jit_addi_i(JIT_RTEMP, r0, i0);
	    _LDC1(f0, 0, JIT_RTEMP);
	}
    }
    else {
	if (_s16P(i0) && _s16P(i0 + 4)) {
	    _LWC1(f0, _jit_US(i0), r0);
	    _LWC1(next_fpr(f0), _jit_US(i0 + 4), r0);
	}
	else {
	    jit_addi_i(JIT_RTEMP, r0, i0);
	    _LWC1(f0, 0, JIT_RTEMP);
	    _LWC1(next_fpr(f0), 4, JIT_RTEMP);
	}
    }

}

#define jit_str_f(r0, f0)		mips_str_f(_jit, r0, f0)
__jit_inline void
mips_str_f(jit_state_t _jit, jit_gpr_t r0, jit_fpr_t f0)
{
    _SWC1(f0, 0, r0);
}

#define jit_str_d(r0, f0)		mips_str_d(_jit, r0, f0)
__jit_inline void
mips_str_d(jit_state_t _jit, jit_gpr_t r0, jit_fpr_t f0)
{
    if (jit_aligned_double_p())
	_SDC1(f0, 0, r0);
    else {
	_SWC1(f0, 0, r0);
	_SWC1(next_fpr(f0), 4, r0);
    }
}

#define jit_sti_f(i0, f0)		mips_sti_f(_jit, i0, f0)
__jit_inline void
mips_sti_f(jit_state_t _jit, void *i0, jit_fpr_t f0)
{
    long	ds = (long)i0;

    if (_s16P(ds))
	_SWC1(f0, _jit_US(ds), JIT_RZERO);
    else {
	jit_movi_i(JIT_RTEMP, ds);
	_SWC1(f0, 0, JIT_RTEMP);
    }
}

#define jit_sti_d(i0, f0)		mips_sti_d(_jit, i0, f0)
__jit_inline void
mips_sti_d(jit_state_t _jit, void *i0, jit_fpr_t f0)
{
    long	ds = (long)i0;

    if (jit_aligned_double_p()) {
	if (_s16P(ds))
	    _SDC1(f0, _jit_US(ds), JIT_RZERO);
	else {
	    jit_movi_i(JIT_RTEMP, ds);
	    _SDC1(f0, 0, JIT_RTEMP);
	}
    }
    else {
	if (_s16P(ds) && _s16P(ds + 4)) {
	    _SWC1(f0, _jit_US(ds), JIT_RZERO);
	    _SWC1(next_fpr(f0), _jit_US(ds + 4), JIT_RZERO);
	}
	else {
	    jit_movi_i(JIT_RTEMP, ds);
	    _SWC1(f0, 0, JIT_RTEMP);
	    _SWC1(next_fpr(f0), 4, JIT_RTEMP);
	}
    }
}

#define jit_stxr_f(r0, r1, f0)		mips_stxr_f(_jit, r0, r1, f0)
__jit_inline void
mips_stxr_f(jit_state_t _jit, jit_gpr_t r0, jit_gpr_t r1, jit_fpr_t f0)
{
    if (jit_mips2_p())
	_SWXC1(f0, r1, r0);
    else {
	jit_addr_i(JIT_RTEMP, r0, r1);
	_SWC1(f0, 0, JIT_RTEMP);
    }
}

#define jit_stxr_d(r0, r1, f0)		mips_stxr_d(_jit, r0, r1, f0)
__jit_inline void
mips_stxr_d(jit_state_t _jit, jit_gpr_t r0, jit_gpr_t r1, jit_fpr_t f0)
{
    if (jit_aligned_double_p()) {
	if (jit_mips2_p())
	    _SDXC1(f0, r1, r0);
	else {
	    jit_addr_i(JIT_RTEMP, r0, r1);
	    _SDC1(f0, 0, JIT_RTEMP);
	}
    }
    else {
	jit_addr_i(JIT_RTEMP, r0, r1);
	_SWC1(f0, 0, JIT_RTEMP);
	_SWC1(next_fpr(f0), 4, JIT_RTEMP);
    }
}

#define jit_stxi_f(i0, r0, f0)		mips_stxi_f(_jit, i0, r0, f0)
__jit_inline void
mips_stxi_f(jit_state_t _jit, int i0, jit_gpr_t r0, jit_fpr_t f0)
{
    if (_s16P(i0))
	_SWC1(f0, _jit_US(i0), r0);
    else if (jit_mips2_p()) {
	jit_movi_i(JIT_RTEMP, i0);
	_SWXC1(f0, JIT_RTEMP, r0);
    }
    else {
	jit_addi_i(JIT_RTEMP, r0, i0);
	_SWC1(f0, 0, JIT_RTEMP);
    }
}

#define jit_stxi_d(i0, r0, f0)		mips_stxi_d(_jit, i0, r0, f0)
__jit_inline void
mips_stxi_d(jit_state_t _jit, int i0, jit_gpr_t r0, jit_fpr_t f0)
{
    if (jit_aligned_double_p()) {
	if (_s16P(i0))
	    _SDC1(f0, _jit_US(i0), r0);
	else if (jit_mips2_p()) {
	    jit_movi_i(JIT_RTEMP, i0);
	    _SDXC1(f0, JIT_RTEMP, r0);
	}
	else {
	    jit_addi_i(JIT_RTEMP, r0, i0);
	    _SDC1(f0, 0, JIT_RTEMP);
	}
    }
    else {
	if (_s16P(i0) && _s16P(i0 + 4)) {
	    _SWC1(f0, _jit_US(i0), r0);
	    _SWC1(next_fpr(f0), _jit_US(i0 + 4), r0);
	}
	else {
	    jit_addi_i(JIT_RTEMP, r0, i0);
	    _SWC1(f0, 0, JIT_RTEMP);
	    _SWC1(next_fpr(f0), 4, JIT_RTEMP);
	}

    }
}

#define jit_extr_i_f(f0, r0)		mips_extr_i_f(_jit, f0, r0)
__jit_inline void
mips_extr_i_f(jit_state_t _jit, jit_fpr_t f0, jit_gpr_t r0)
{
    _MTC1(r0, JIT_FTMP0);
    _CVT_S_W(f0, JIT_FTMP0);
}

#define jit_extr_i_d(f0, r0)		mips_extr_i_d(_jit, f0, r0)
__jit_inline void
mips_extr_i_d(jit_state_t _jit, jit_fpr_t f0, jit_gpr_t r0)
{
    _MTC1(r0, JIT_FTMP0);
    _CVT_D_W(f0, JIT_FTMP0);
}

#define jit_extr_f_d(f0, r0)		mips_extr_f_d(_jit, f0, r0)
__jit_inline void
mips_extr_f_d(jit_state_t _jit, jit_fpr_t f0, jit_fpr_t f1)
{
    _CVT_D_S(f0, f1);
}

#define jit_extr_d_f(f0, r0)		mips_extr_d_f(_jit, f0, r0)
__jit_inline void
mips_extr_d_f(jit_state_t _jit, jit_fpr_t f0, jit_fpr_t f1)
{
    _CVT_S_D(f0, f1);
}

#define jit_roundr_f_i(r0, f0)		mips_roundr_f_i(_jit, r0, f0)
__jit_inline void
mips_roundr_f_i(jit_state_t _jit, jit_gpr_t r0, jit_fpr_t f0)
{
#if 0	/* if round to nearest */
    _ROUND_W_S(JIT_FTMP0, f0);
#else
    jit_insn	*l;
    _MTC1(JIT_RZERO, (jit_fpr_t)JIT_FTMP0);
    jit_movi_i(JIT_RTEMP, 0xbf000000);
    _MTC1(JIT_RTEMP, JIT_FTMP1);
    _C_OLT_S(f0, JIT_FTMP0);
    l = _jit->x.pc;
    _BC1T(0);
    jit_nop(1);
    _NEG_S(JIT_FTMP1, JIT_FTMP1);
    jit_patch(l);
    _ADD_S(JIT_FTMP0, f0, JIT_FTMP1);
    _TRUNC_W_S(JIT_FTMP0, JIT_FTMP0);
#endif
    _MFC1(r0, JIT_FTMP0);
}

#define jit_roundr_d_i(r0, f0)		mips_roundr_d_i(_jit, r0, f0)
__jit_inline void
mips_roundr_d_i(jit_state_t _jit, jit_gpr_t r0, jit_fpr_t f0)
{
#if 0	/* if round to nearest */
    _ROUND_W_D(JIT_FTMP0, f0);
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
    _C_OLT_D(f0, JIT_FTMP0);
    l = _jit->x.pc;
    _BC1T(0);
    jit_nop(1);
    _NEG_D(JIT_FTMP1, JIT_FTMP1);
    jit_patch(l);
    _ADD_D(JIT_FTMP0, f0, JIT_FTMP1);
    _TRUNC_W_D(JIT_FTMP0, JIT_FTMP0);
#endif
    _MFC1(r0, JIT_FTMP0);
}

#define jit_rintr_f_i(r0, f0)		mips_rintr_f_i(_jit, r0, f0)
__jit_inline void
mips_rintr_f_i(jit_state_t _jit, jit_gpr_t r0, jit_fpr_t f0)
{
    _CVT_W_S(JIT_FTMP0, f0);
    _MFC1(r0, JIT_FTMP0);
}

#define jit_rintr_d_i(r0, f0)		mips_rintr_d_i(_jit, r0, f0)
__jit_inline void
mips_rintr_d_i(jit_state_t _jit, jit_gpr_t r0, jit_fpr_t f0)
{
    _CVT_W_D(JIT_FTMP0, f0);
    _MFC1(r0, JIT_FTMP0);
}

#define jit_truncr_f_i(r0, f0)		mips_truncr_f_i(_jit, r0, f0)
__jit_inline void
mips_truncr_f_i(jit_state_t _jit, jit_gpr_t r0, jit_fpr_t f0)
{
    _TRUNC_W_S(JIT_FTMP0, f0);
    _MFC1(r0, JIT_FTMP0);
}

#define jit_truncr_d_i(r0, f0)		mips_truncr_d_i(_jit, r0, f0)
__jit_inline void
mips_truncr_d_i(jit_state_t _jit, jit_gpr_t r0, jit_fpr_t f0)
{
    _TRUNC_W_D(JIT_FTMP0, f0);
    _MFC1(r0, JIT_FTMP0);
}

#define jit_ceilr_f_i(r0, f0)		mips_ceilr_f_i(_jit, r0, f0)
__jit_inline void
mips_ceilr_f_i(jit_state_t _jit, jit_gpr_t r0, jit_fpr_t f0)
{
    _CEIL_W_S(JIT_FTMP0, f0);
    _MFC1(r0, JIT_FTMP0);
}

#define jit_ceilr_d_i(r0, f0)		mips_ceilr_d_i(_jit, r0, f0)
__jit_inline void
mips_ceilr_d_i(jit_state_t _jit, jit_gpr_t r0, jit_fpr_t f0)
{
    _CEIL_W_D(JIT_FTMP0, f0);
    _MFC1(r0, JIT_FTMP0);
}

#define jit_floorr_f_i(r0, f0)		mips_floorr_f_i(_jit, r0, f0)
__jit_inline void
mips_floorr_f_i(jit_state_t _jit, jit_gpr_t r0, jit_fpr_t f0)
{
    _FLOOR_W_S(JIT_FTMP0, f0);
    _MFC1(r0, JIT_FTMP0);
}

#define jit_floorr_d_i(r0, f0)		mips_floorr_d_i(_jit, r0, f0)
__jit_inline void
mips_floorr_d_i(jit_state_t _jit, jit_gpr_t r0, jit_fpr_t f0)
{
    _FLOOR_W_D(JIT_FTMP0, f0);
    _MFC1(r0, JIT_FTMP0);
}

#define jit_ltr_f(r0, f0, f1)		mips_ltr_f(_jit, r0, f0, f1)
__jit_inline void
mips_ltr_f(jit_state_t _jit, jit_gpr_t r0, jit_fpr_t f0, jit_fpr_t f1)
{
    jit_insn	*l;
    if (jit_movf_p())
	jit_movi_i(r0, 1);
    _C_OLT_S(f0, f1);
    if (jit_movf_p())
	_MOVF(r0, JIT_RZERO);
    else {
	l = _jit->x.pc;
	_BC1T(0);

	/*delay*/
	jit_movi_i(r0, 1);

	jit_movi_i(r0, 0);
	jit_patch(l);
    }
}

#define jit_ltr_d(r0, f0, f1)		mips_ltr_d(_jit, r0, f0, f1)
__jit_inline void
mips_ltr_d(jit_state_t _jit, jit_gpr_t r0, jit_fpr_t f0, jit_fpr_t f1)
{
    jit_insn	*l;
    if (jit_movf_p())
	jit_movi_i(r0, 1);
    _C_OLT_D(f0, f1);
    if (jit_movf_p())
	_MOVF(r0, JIT_RZERO);
    else {
	l = _jit->x.pc;
	_BC1T(0);

	/*delay*/
	jit_movi_i(r0, 1);

	jit_movi_i(r0, 0);
	jit_patch(l);
    }
}

#define jit_ler_f(r0, f0, f1)		mips_ler_f(_jit, r0, f0, f1)
__jit_inline void
mips_ler_f(jit_state_t _jit, jit_gpr_t r0, jit_fpr_t f0, jit_fpr_t f1)
{
    jit_insn	*l;
    if (jit_movf_p())
	jit_movi_i(r0, 1);
    _C_OLE_S(f0, f1);
    if (jit_movf_p())
	_MOVF(r0, JIT_RZERO);
    else {
	l = _jit->x.pc;
	_BC1T(0);

	/*delay*/
	jit_movi_i(r0, 1);

	jit_movi_i(r0, 0);
	jit_patch(l);
    }
}

#define jit_ler_d(r0, f0, f1)		mips_ler_d(_jit, r0, f0, f1)
__jit_inline void
mips_ler_d(jit_state_t _jit, jit_gpr_t r0, jit_fpr_t f0, jit_fpr_t f1)
{
    jit_insn	*l;
    if (jit_movf_p())
	jit_movi_i(r0, 1);
    _C_OLE_D(f0, f1);
    if (jit_movf_p())
	_MOVF(r0, JIT_RZERO);
    else {
	l = _jit->x.pc;
	_BC1T(0);

	/*delay*/
	jit_movi_i(r0, 1);

	jit_movi_i(r0, 0);
	jit_patch(l);
    }
}

#define jit_eqr_f(r0, f0, f1)		mips_eqr_f(_jit, r0, f0, f1)
__jit_inline void
mips_eqr_f(jit_state_t _jit, jit_gpr_t r0, jit_fpr_t f0, jit_fpr_t f1)
{
    jit_insn	*l;
    if (jit_movf_p())
	jit_movi_i(r0, 1);
    _C_EQ_S(f0, f1);
    if (jit_movf_p())
	_MOVF(r0, JIT_RZERO);
    else {
	l = _jit->x.pc;
	_BC1T(0);

	/*delay*/
	jit_movi_i(r0, 1);

	jit_movi_i(r0, 0);
	jit_patch(l);
    }
}

#define jit_eqr_d(r0, f0, f1)		mips_eqr_d(_jit, r0, f0, f1)
__jit_inline void
mips_eqr_d(jit_state_t _jit, jit_gpr_t r0, jit_fpr_t f0, jit_fpr_t f1)
{
    jit_insn	*l;
    if (jit_movf_p())
	jit_movi_i(r0, 1);
    _C_EQ_D(f0, f1);
    if (jit_movf_p())
	_MOVF(r0, JIT_RZERO);
    else {
	l = _jit->x.pc;
	_BC1T(0);

	/*delay*/
	jit_movi_i(r0, 1);

	jit_movi_i(r0, 0);
	jit_patch(l);
    }
}

#define jit_ger_f(r0, f0, f1)		mips_ger_f(_jit, r0, f0, f1)
__jit_inline void
mips_ger_f(jit_state_t _jit, jit_gpr_t r0, jit_fpr_t f0, jit_fpr_t f1)
{
    jit_insn	*l;
    if (jit_movf_p())
	jit_movi_i(r0, 1);
    _C_ULT_S(f0, f1);
    if (jit_movf_p())
	_MOVT(r0, JIT_RZERO);
    else {
	l = _jit->x.pc;
	_BC1F(0);

	/*delay*/
	jit_movi_i(r0, 1);

	jit_movi_i(r0, 0);
	jit_patch(l);
    }
}

#define jit_ger_d(r0, f0, f1)		mips_ger_d(_jit, r0, f0, f1)
__jit_inline void
mips_ger_d(jit_state_t _jit, jit_gpr_t r0, jit_fpr_t f0, jit_fpr_t f1)
{
    jit_insn	*l;
    if (jit_movf_p())
	jit_movi_i(r0, 1);
    _C_ULT_D(f0, f1);
    if (jit_movf_p())
	_MOVT(r0, JIT_RZERO);
    else {
	l = _jit->x.pc;
	_BC1F(0);

	/*delay*/
	jit_movi_i(r0, 1);

	jit_movi_i(r0, 0);
	jit_patch(l);
    }
}

#define jit_gtr_f(r0, f0, f1)		mips_gtr_f(_jit, r0, f0, f1)
__jit_inline void
mips_gtr_f(jit_state_t _jit, jit_gpr_t r0, jit_fpr_t f0, jit_fpr_t f1)
{
    jit_insn	*l;
    if (jit_movf_p())
	jit_movi_i(r0, 1);
    _C_ULE_S(f0, f1);
    if (jit_movf_p())
	_MOVT(r0, JIT_RZERO);
    else {
	l = _jit->x.pc;
	_BC1F(0);

	/*delay*/
	jit_movi_i(r0, 1);

	jit_movi_i(r0, 0);
	jit_patch(l);
    }
}

#define jit_gtr_d(r0, f0, f1)		mips_gtr_d(_jit, r0, f0, f1)
__jit_inline void
mips_gtr_d(jit_state_t _jit, jit_gpr_t r0, jit_fpr_t f0, jit_fpr_t f1)
{
    jit_insn	*l;
    if (jit_movf_p())
	jit_movi_i(r0, 1);
    _C_ULE_D(f0, f1);
    if (jit_movf_p())
	_MOVT(r0, JIT_RZERO);
    else {
	l = _jit->x.pc;
	_BC1F(0);

	/*delay*/
	jit_movi_i(r0, 1);

	jit_movi_i(r0, 0);
	jit_patch(l);
    }
}

#define jit_ner_f(r0, f0, f1)		mips_ner_f(_jit, r0, f0, f1)
__jit_inline void
mips_ner_f(jit_state_t _jit, jit_gpr_t r0, jit_fpr_t f0, jit_fpr_t f1)
{
    jit_insn	*l;
    if (jit_movf_p())
	jit_movi_i(r0, 1);
    _C_EQ_S(f0, f1);
    if (jit_movf_p())
	_MOVT(r0, JIT_RZERO);
    else {
	l = _jit->x.pc;
	_BC1F(0);

	/*delay*/
	jit_movi_i(r0, 1);

	jit_movi_i(r0, 0);
	jit_patch(l);
    }
}

#define jit_ner_d(r0, f0, f1)		mips_ner_d(_jit, r0, f0, f1)
__jit_inline void
mips_ner_d(jit_state_t _jit, jit_gpr_t r0, jit_fpr_t f0, jit_fpr_t f1)
{
    jit_insn	*l;
    if (jit_movf_p())
	jit_movi_i(r0, 1);
    _C_EQ_D(f0, f1);
    if (jit_movf_p())
	_MOVT(r0, JIT_RZERO);
    else {
	l = _jit->x.pc;
	_BC1F(0);

	/*delay*/
	jit_movi_i(r0, 1);

	jit_movi_i(r0, 0);
	jit_patch(l);
    }
}

#define jit_unltr_f(r0, f0, f1)		mips_unltr_f(_jit, r0, f0, f1)
__jit_inline void
mips_unltr_f(jit_state_t _jit, jit_gpr_t r0, jit_fpr_t f0, jit_fpr_t f1)
{
    jit_insn	*l;
    if (jit_movf_p())
	jit_movi_i(r0, 1);
    _C_ULT_S(f0, f1);
    if (jit_movf_p())
	_MOVF(r0, JIT_RZERO);
    else {
	l = _jit->x.pc;
	_BC1T(0);

	/*delay*/
	jit_movi_i(r0, 1);

	jit_movi_i(r0, 0);
	jit_patch(l);
    }
}

#define jit_unltr_d(r0, f0, f1)		mips_unltr_d(_jit, r0, f0, f1)
__jit_inline void
mips_unltr_d(jit_state_t _jit, jit_gpr_t r0, jit_fpr_t f0, jit_fpr_t f1)
{
    jit_insn	*l;
    if (jit_movf_p())
	jit_movi_i(r0, 1);
    _C_ULT_D(f0, f1);
    if (jit_movf_p())
	_MOVF(r0, JIT_RZERO);
    else {
	l = _jit->x.pc;
	_BC1T(0);

	/*delay*/
	jit_movi_i(r0, 1);

	jit_movi_i(r0, 0);
	jit_patch(l);
    }
}

#define jit_unler_f(r0, f0, f1)		mips_unler_f(_jit, r0, f0, f1)
__jit_inline void
mips_unler_f(jit_state_t _jit, jit_gpr_t r0, jit_fpr_t f0, jit_fpr_t f1)
{
    jit_insn	*l;
    if (jit_movf_p())
	jit_movi_i(r0, 1);
    _C_ULE_S(f0, f1);
    if (jit_movf_p())
	_MOVF(r0, JIT_RZERO);
    else {
	l = _jit->x.pc;
	_BC1T(0);

	/*delay*/
	jit_movi_i(r0, 1);

	jit_movi_i(r0, 0);
	jit_patch(l);
    }
}

#define jit_unler_d(r0, f0, f1)		mips_unler_d(_jit, r0, f0, f1)
__jit_inline void
mips_unler_d(jit_state_t _jit, jit_gpr_t r0, jit_fpr_t f0, jit_fpr_t f1)
{
    jit_insn	*l;
    if (jit_movf_p())
	jit_movi_i(r0, 1);
    _C_ULE_D(f0, f1);
    if (jit_movf_p())
	_MOVF(r0, JIT_RZERO);
    else {
	l = _jit->x.pc;
	_BC1T(0);

	/*delay*/
	jit_movi_i(r0, 1);

	jit_movi_i(r0, 0);
	jit_patch(l);
    }
}

#define jit_uneqr_f(r0, f0, f1)		mips_uneqr_f(_jit, r0, f0, f1)
__jit_inline void
mips_uneqr_f(jit_state_t _jit, jit_gpr_t r0, jit_fpr_t f0, jit_fpr_t f1)
{
    jit_insn	*l;
    if (jit_movf_p())
	jit_movi_i(r0, 1);
    _C_UEQ_S(f0, f1);
    if (jit_movf_p())
	_MOVF(r0, JIT_RZERO);
    else {
	l = _jit->x.pc;
	_BC1T(0);

	/*delay*/
	jit_movi_i(r0, 1);

	jit_movi_i(r0, 0);
	jit_patch(l);
    }
}

#define jit_uneqr_d(r0, f0, f1)		mips_uneqr_d(_jit, r0, f0, f1)
__jit_inline void
mips_uneqr_d(jit_state_t _jit, jit_gpr_t r0, jit_fpr_t f0, jit_fpr_t f1)
{
    jit_insn	*l;
    if (jit_movf_p())
	jit_movi_i(r0, 1);
    _C_UEQ_D(f0, f1);
    if (jit_movf_p())
	_MOVF(r0, JIT_RZERO);
    else {
	l = _jit->x.pc;
	_BC1T(0);

	/*delay*/
	jit_movi_i(r0, 1);

	jit_movi_i(r0, 0);
	jit_patch(l);
    }
}

#define jit_unger_f(r0, f0, f1)		mips_unger_f(_jit, r0, f0, f1)
__jit_inline void
mips_unger_f(jit_state_t _jit, jit_gpr_t r0, jit_fpr_t f0, jit_fpr_t f1)
{
    jit_insn	*l;
    if (jit_movf_p())
	jit_movi_i(r0, 1);
    _C_OLT_S(f0, f1);
    if (jit_movf_p())
	_MOVT(r0, JIT_RZERO);
    else {
	l = _jit->x.pc;
	_BC1F(0);

	/*delay*/
	jit_movi_i(r0, 1);

	jit_movi_i(r0, 0);
	jit_patch(l);
    }
}

#define jit_unger_d(r0, f0, f1)		mips_unger_d(_jit, r0, f0, f1)
__jit_inline void
mips_unger_d(jit_state_t _jit, jit_gpr_t r0, jit_fpr_t f0, jit_fpr_t f1)
{
    jit_insn	*l;
    if (jit_movf_p())
	jit_movi_i(r0, 1);
    _C_OLT_D(f0, f1);
    if (jit_movf_p())
	_MOVT(r0, JIT_RZERO);
    else {
	l = _jit->x.pc;
	_BC1F(0);

	/*delay*/
	jit_movi_i(r0, 1);

	jit_movi_i(r0, 0);
	jit_patch(l);
    }
}

#define jit_ungtr_f(r0, f0, f1)		mips_ungtr_f(_jit, r0, f0, f1)
__jit_inline void
mips_ungtr_f(jit_state_t _jit, jit_gpr_t r0, jit_fpr_t f0, jit_fpr_t f1)
{
    jit_insn	*l;
    if (jit_movf_p())
	jit_movi_i(r0, 1);
    _C_OLE_S(f0, f1);
    if (jit_movf_p())
	_MOVT(r0, JIT_RZERO);
    else {
	l = _jit->x.pc;
	_BC1F(0);

	/*delay*/
	jit_movi_i(r0, 1);

	jit_movi_i(r0, 0);
	jit_patch(l);
    }
}

#define jit_ungtr_d(r0, f0, f1)		mips_ungtr_d(_jit, r0, f0, f1)
__jit_inline void
mips_ungtr_d(jit_state_t _jit, jit_gpr_t r0, jit_fpr_t f0, jit_fpr_t f1)
{
    jit_insn	*l;
    if (jit_movf_p())
	jit_movi_i(r0, 1);
    _C_OLE_D(f0, f1);
    if (jit_movf_p())
	_MOVT(r0, JIT_RZERO);
    else {
	l = _jit->x.pc;
	_BC1F(0);

	/*delay*/
	jit_movi_i(r0, 1);

	jit_movi_i(r0, 0);
	jit_patch(l);
    }
}

#define jit_ltgtr_f(r0, f0, f1)		mips_ltgtr_f(_jit, r0, f0, f1)
__jit_inline void
mips_ltgtr_f(jit_state_t _jit, jit_gpr_t r0, jit_fpr_t f0, jit_fpr_t f1)
{
    jit_insn	*l;
    if (jit_movf_p())
	jit_movi_i(r0, 1);
    _C_UEQ_S(f0, f1);
    if (jit_movf_p())
	_MOVT(r0, JIT_RZERO);
    else {
	l = _jit->x.pc;
	_BC1F(0);

	/*delay*/
	jit_movi_i(r0, 1);

	jit_movi_i(r0, 0);
	jit_patch(l);
    }
}

#define jit_ltgtr_d(r0, f0, f1)		mips_ltgtr_d(_jit, r0, f0, f1)
__jit_inline void
mips_ltgtr_d(jit_state_t _jit, jit_gpr_t r0, jit_fpr_t f0, jit_fpr_t f1)
{
    jit_insn	*l;
    if (jit_movf_p())
	jit_movi_i(r0, 1);
    _C_UEQ_D(f0, f1);
    if (jit_movf_p())
	_MOVT(r0, JIT_RZERO);
    else {
	l = _jit->x.pc;
	_BC1F(0);

	/*delay*/
	jit_movi_i(r0, 1);

	jit_movi_i(r0, 0);
	jit_patch(l);
    }
}

#define jit_ordr_f(r0, f0, f1)		mips_ordr_f(_jit, r0, f0, f1)
__jit_inline void
mips_ordr_f(jit_state_t _jit, jit_gpr_t r0, jit_fpr_t f0, jit_fpr_t f1)
{
    jit_insn	*l;
    if (jit_movf_p())
	jit_movi_i(r0, 1);
    _C_UN_S(f0, f1);
    if (jit_movf_p())
	_MOVT(r0, JIT_RZERO);
    else {
	l = _jit->x.pc;
	_BC1F(0);

	/*delay*/
	jit_movi_i(r0, 1);

	jit_movi_i(r0, 0);
	jit_patch(l);
    }
}

#define jit_ordr_d(r0, f0, f1)		mips_ordr_d(_jit, r0, f0, f1)
__jit_inline void
mips_ordr_d(jit_state_t _jit, jit_gpr_t r0, jit_fpr_t f0, jit_fpr_t f1)
{
    jit_insn	*l;
    if (jit_movf_p())
	jit_movi_i(r0, 1);
    _C_UN_D(f0, f1);
    if (jit_movf_p())
	_MOVT(r0, JIT_RZERO);
    else {
	l = _jit->x.pc;
	_BC1F(0);

	/*delay*/
	jit_movi_i(r0, 1);

	jit_movi_i(r0, 0);
	jit_patch(l);
    }
}

#define jit_unordr_f(r0, f0, f1)	mips_unordr_f(_jit, r0, f0, f1)
__jit_inline void
mips_unordr_f(jit_state_t _jit, jit_gpr_t r0, jit_fpr_t f0, jit_fpr_t f1)
{
    jit_insn	*l;
    if (jit_movf_p())
	jit_movi_i(r0, 1);
    _C_UN_S(f0, f1);
    if (jit_movf_p())
	_MOVF(r0, JIT_RZERO);
    else {
	l = _jit->x.pc;
	_BC1T(0);

	/*delay*/
	jit_movi_i(r0, 1);

	jit_movi_i(r0, 0);
	jit_patch(l);
    }
}

#define jit_unordr_d(r0, f0, f1)	mips_unordr_d(_jit, r0, f0, f1)
__jit_inline void
mips_unordr_d(jit_state_t _jit, jit_gpr_t r0, jit_fpr_t f0, jit_fpr_t f1)
{
    jit_insn	*l;
    if (jit_movf_p())
	jit_movi_i(r0, 1);
    _C_UN_D(f0, f1);
    if (jit_movf_p())
	_MOVF(r0, JIT_RZERO);
    else {
	l = _jit->x.pc;
	_BC1T(0);

	/*delay*/
	jit_movi_i(r0, 1);

	jit_movi_i(r0, 0);
	jit_patch(l);
    }
}

#define jit_bltr_f(i0, f0, f1)		mips_bltr_f(_jit, i0, f0, f1)
__jit_inline jit_insn *
mips_bltr_f(jit_state_t _jit, jit_insn *i0, jit_fpr_t f0, jit_fpr_t f1)
{
    jit_insn	*l;
    long	 d;
    _C_OLT_S(f0, f1);
    l = _jit->x.pc;
    d = (((long)i0 - (long)l) >> 2) - 1;
    assert(_s16P(d));
    _BC1T(_jit_US(d));
    jit_nop(1);
    return (l);
}

#define jit_bltr_d(i0, f0, f1)		mips_bltr_d(_jit, i0, f0, f1)
__jit_inline jit_insn *
mips_bltr_d(jit_state_t _jit, jit_insn * i0, jit_fpr_t f0, jit_fpr_t f1)
{
    jit_insn	*l;
    long	 d;
    _C_OLT_D(f0, f1);
    l = _jit->x.pc;
    d = (((long)i0 - (long)l) >> 2) - 1;
    assert(_s16P(d));
    _BC1T(_jit_US(d));
    jit_nop(1);
    return (l);
}

#define jit_bler_f(i0, f0, f1)		mips_bler_f(_jit, i0, f0, f1)
__jit_inline jit_insn *
mips_bler_f(jit_state_t _jit, jit_insn * i0, jit_fpr_t f0, jit_fpr_t f1)
{
    jit_insn	*l;
    long	 d;
    _C_OLE_S(f0, f1);
    l = _jit->x.pc;
    d = (((long)i0 - (long)l) >> 2) - 1;
    assert(_s16P(d));
    _BC1T(_jit_US(d));
    jit_nop(1);
    return (l);
}

#define jit_bler_d(i0, f0, f1)		mips_bler_d(_jit, i0, f0, f1)
__jit_inline jit_insn *
mips_bler_d(jit_state_t _jit, jit_insn * i0, jit_fpr_t f0, jit_fpr_t f1)
{
    jit_insn	*l;
    long	 d;
    _C_OLE_D(f0, f1);
    l = _jit->x.pc;
    d = (((long)i0 - (long)l) >> 2) - 1;
    assert(_s16P(d));
    _BC1T(_jit_US(d));
    jit_nop(1);
    return (l);
}

#define jit_beqr_f(i0, f0, f1)		mips_beqr_f(_jit, i0, f0, f1)
__jit_inline jit_insn *
mips_beqr_f(jit_state_t _jit, jit_insn * i0, jit_fpr_t f0, jit_fpr_t f1)
{
    jit_insn	*l;
    long	 d;
    _C_EQ_S(f0, f1);
    l = _jit->x.pc;
    d = (((long)i0 - (long)l) >> 2) - 1;
    assert(_s16P(d));
    _BC1T(_jit_US(d));
    jit_nop(1);
    return (l);
}

#define jit_beqr_d(i0, f0, f1)		mips_beqr_d(_jit, i0, f0, f1)
__jit_inline jit_insn *
mips_beqr_d(jit_state_t _jit, jit_insn * i0, jit_fpr_t f0, jit_fpr_t f1)
{
    jit_insn	*l;
    long	 d;
    _C_EQ_D(f0, f1);
    l = _jit->x.pc;
    d = (((long)i0 - (long)l) >> 2) - 1;
    assert(_s16P(d));
    _BC1T(_jit_US(d));
    jit_nop(1);
    return (l);
}

#define jit_bger_f(i0, f0, f1)		mips_bger_f(_jit, i0, f0, f1)
__jit_inline jit_insn *
mips_bger_f(jit_state_t _jit, jit_insn * i0, jit_fpr_t f0, jit_fpr_t f1)
{
    jit_insn	*l;
    long	 d;
    _C_ULT_S(f0, f1);
    l = _jit->x.pc;
    d = (((long)i0 - (long)l) >> 2) - 1;
    assert(_s16P(d));
    _BC1F(_jit_US(d));
    jit_nop(1);
    return (l);
}

#define jit_bger_d(i0, f0, f1)		mips_bger_d(_jit, i0, f0, f1)
__jit_inline jit_insn *
mips_bger_d(jit_state_t _jit, jit_insn * i0, jit_fpr_t f0, jit_fpr_t f1)
{
    jit_insn	*l;
    long	 d;
    _C_ULT_D(f0, f1);
    l = _jit->x.pc;
    d = (((long)i0 - (long)l) >> 2) - 1;
    assert(_s16P(d));
    _BC1F(_jit_US(d));
    jit_nop(1);
    return (l);
}

#define jit_bgtr_f(i0, f0, f1)		mips_bgtr_f(_jit, i0, f0, f1)
__jit_inline jit_insn *
mips_bgtr_f(jit_state_t _jit, jit_insn * i0, jit_fpr_t f0, jit_fpr_t f1)
{
    jit_insn	*l;
    long	 d;
    _C_ULE_S(f0, f1);
    l = _jit->x.pc;
    d = (((long)i0 - (long)l) >> 2) - 1;
    assert(_s16P(d));
    _BC1F(_jit_US(d));
    jit_nop(1);
    return (l);
}

#define jit_bgtr_d(i0, f0, f1)		mips_bgtr_d(_jit, i0, f0, f1)
__jit_inline jit_insn *
mips_bgtr_d(jit_state_t _jit, jit_insn * i0, jit_fpr_t f0, jit_fpr_t f1)
{
    jit_insn	*l;
    long	 d;
    _C_ULE_D(f0, f1);
    l = _jit->x.pc;
    d = (((long)i0 - (long)l) >> 2) - 1;
    assert(_s16P(d));
    _BC1F(_jit_US(d));
    jit_nop(1);
    return (l);
}

#define jit_bner_f(i0, f0, f1)		mips_bner_f(_jit, i0, f0, f1)
__jit_inline jit_insn *
mips_bner_f(jit_state_t _jit, jit_insn * i0, jit_fpr_t f0, jit_fpr_t f1)
{
    jit_insn	*l;
    long	 d;
    _C_EQ_S(f0, f1);
    l = _jit->x.pc;
    d = (((long)i0 - (long)l) >> 2) - 1;
    assert(_s16P(d));
    _BC1F(_jit_US(d));
    jit_nop(1);
    return (l);
}

#define jit_bner_d(i0, f0, f1)		mips_bner_d(_jit, i0, f0, f1)
__jit_inline jit_insn *
mips_bner_d(jit_state_t _jit, jit_insn * i0, jit_fpr_t f0, jit_fpr_t f1)
{
    jit_insn	*l;
    long	 d;
    _C_EQ_D(f0, f1);
    l = _jit->x.pc;
    d = (((long)i0 - (long)l) >> 2) - 1;
    assert(_s16P(d));
    _BC1F(_jit_US(d));
    jit_nop(1);
    return (l);
}

#define jit_bunltr_f(i0, f0, f1)	mips_bunltr_f(_jit, i0, f0, f1)
__jit_inline jit_insn *
mips_bunltr_f(jit_state_t _jit, jit_insn * i0, jit_fpr_t f0, jit_fpr_t f1)
{
    jit_insn	*l;
    long	 d;
    _C_ULT_S(f0, f1);
    l = _jit->x.pc;
    d = (((long)i0 - (long)l) >> 2) - 1;
    assert(_s16P(d));
    _BC1T(_jit_US(d));
    jit_nop(1);
    return (l);
}

#define jit_bunltr_d(i0, f0, f1)	mips_bunltr_d(_jit, i0, f0, f1)
__jit_inline jit_insn *
mips_bunltr_d(jit_state_t _jit, jit_insn * i0, jit_fpr_t f0, jit_fpr_t f1)
{
    jit_insn	*l;
    long	 d;
    _C_ULT_D(f0, f1);
    l = _jit->x.pc;
    d = (((long)i0 - (long)l) >> 2) - 1;
    assert(_s16P(d));
    _BC1T(_jit_US(d));
    jit_nop(1);
    return (l);
}

#define jit_bunler_f(i0, f0, f1)	mips_bunler_f(_jit, i0, f0, f1)
__jit_inline jit_insn *
mips_bunler_f(jit_state_t _jit, jit_insn * i0, jit_fpr_t f0, jit_fpr_t f1)
{
    jit_insn	*l;
    long	 d;
    _C_ULE_S(f0, f1);
    l = _jit->x.pc;
    d = (((long)i0 - (long)l) >> 2) - 1;
    assert(_s16P(d));
    _BC1T(_jit_US(d));
    jit_nop(1);
    return (l);
}

#define jit_bunler_d(i0, f0, f1)	mips_bunler_d(_jit, i0, f0, f1)
__jit_inline jit_insn *
mips_bunler_d(jit_state_t _jit, jit_insn * i0, jit_fpr_t f0, jit_fpr_t f1)
{
    jit_insn	*l;
    long	 d;
    _C_ULE_D(f0, f1);
    l = _jit->x.pc;
    d = (((long)i0 - (long)l) >> 2) - 1;
    assert(_s16P(d));
    _BC1T(_jit_US(d));
    jit_nop(1);
    return (l);
}

#define jit_buneqr_f(i0, f0, f1)	mips_buneqr_f(_jit, i0, f0, f1)
__jit_inline jit_insn *
mips_buneqr_f(jit_state_t _jit, jit_insn * i0, jit_fpr_t f0, jit_fpr_t f1)
{
    jit_insn	*l;
    long	 d;
    _C_UEQ_S(f0, f1);
    l = _jit->x.pc;
    d = (((long)i0 - (long)l) >> 2) - 1;
    assert(_s16P(d));
    _BC1T(_jit_US(d));
    jit_nop(1);
    return (l);
}

#define jit_buneqr_d(i0, f0, f1)	mips_buneqr_d(_jit, i0, f0, f1)
__jit_inline jit_insn *
mips_buneqr_d(jit_state_t _jit, jit_insn * i0, jit_fpr_t f0, jit_fpr_t f1)
{
    jit_insn	*l;
    long	 d;
    _C_UEQ_D(f0, f1);
    l = _jit->x.pc;
    d = (((long)i0 - (long)l) >> 2) - 1;
    assert(_s16P(d));
    _BC1T(_jit_US(d));
    jit_nop(1);
    return (l);
}

#define jit_bunger_f(i0, f0, f1)	mips_bunger_f(_jit, i0, f0, f1)
__jit_inline jit_insn *
mips_bunger_f(jit_state_t _jit, jit_insn * i0, jit_fpr_t f0, jit_fpr_t f1)
{
    jit_insn	*l;
    long	 d;
    _C_OLT_S(f0, f1);
    l = _jit->x.pc;
    d = (((long)i0 - (long)l) >> 2) - 1;
    assert(_s16P(d));
    _BC1F(_jit_US(d));
    jit_nop(1);
    return (l);
}

#define jit_bunger_d(i0, f0, f1)	mips_bunger_d(_jit, i0, f0, f1)
__jit_inline jit_insn *
mips_bunger_d(jit_state_t _jit, jit_insn * i0, jit_fpr_t f0, jit_fpr_t f1)
{
    jit_insn	*l;
    long	 d;
    _C_OLT_D(f0, f1);
    l = _jit->x.pc;
    d = (((long)i0 - (long)l) >> 2) - 1;
    assert(_s16P(d));
    _BC1F(_jit_US(d));
    jit_nop(1);
    return (l);
}

#define jit_bungtr_f(i0, f0, f1)	mips_bungtr_f(_jit, i0, f0, f1)
__jit_inline jit_insn *
mips_bungtr_f(jit_state_t _jit, jit_insn * i0, jit_fpr_t f0, jit_fpr_t f1)
{
    jit_insn	*l;
    long	 d;
    _C_OLE_S(f0, f1);
    l = _jit->x.pc;
    d = (((long)i0 - (long)l) >> 2) - 1;
    assert(_s16P(d));
    _BC1F(_jit_US(d));
    jit_nop(1);
    return (l);
}

#define jit_bungtr_d(i0, f0, f1)	mips_bungtr_d(_jit, i0, f0, f1)
__jit_inline jit_insn *
mips_bungtr_d(jit_state_t _jit, jit_insn * i0, jit_fpr_t f0, jit_fpr_t f1)
{
    jit_insn	*l;
    long	 d;
    _C_OLE_D(f0, f1);
    l = _jit->x.pc;
    d = (((long)i0 - (long)l) >> 2) - 1;
    assert(_s16P(d));
    _BC1F(_jit_US(d));
    jit_nop(1);
    return (l);
}

#define jit_bltgtr_f(i0, f0, f1)	mips_bltgtr_f(_jit, i0, f0, f1)
__jit_inline jit_insn *
mips_bltgtr_f(jit_state_t _jit, jit_insn * i0, jit_fpr_t f0, jit_fpr_t f1)
{
    jit_insn	*l;
    long	 d;
    _C_UEQ_S(f0, f1);
    l = _jit->x.pc;
    d = (((long)i0 - (long)l) >> 2) - 1;
    assert(_s16P(d));
    _BC1F(_jit_US(d));
    jit_nop(1);
    return (l);
}

#define jit_bltgtr_d(i0, f0, f1)	mips_bltgtr_d(_jit, i0, f0, f1)
__jit_inline jit_insn *
mips_bltgtr_d(jit_state_t _jit, jit_insn * i0, jit_fpr_t f0, jit_fpr_t f1)
{
    jit_insn	*l;
    long	 d;
    _C_UEQ_D(f0, f1);
    l = _jit->x.pc;
    d = (((long)i0 - (long)l) >> 2) - 1;
    assert(_s16P(d));
    _BC1F(_jit_US(d));
    jit_nop(1);
    return (l);
}

#define jit_bordr_f(i0, f0, f1)		mips_bordr_f(_jit, i0, f0, f1)
__jit_inline jit_insn *
mips_bordr_f(jit_state_t _jit, jit_insn * i0, jit_fpr_t f0, jit_fpr_t f1)
{
    jit_insn	*l;
    long	 d;
    _C_UN_S(f0, f1);
    l = _jit->x.pc;
    d = (((long)i0 - (long)l) >> 2) - 1;
    assert(_s16P(d));
    _BC1F(_jit_US(d));
    jit_nop(1);
    return (l);
}

#define jit_bordr_d(i0, f0, f1)		mips_bordr_d(_jit, i0, f0, f1)
__jit_inline jit_insn *
mips_bordr_d(jit_state_t _jit, jit_insn * i0, jit_fpr_t f0, jit_fpr_t f1)
{
    jit_insn	*l;
    long	 d;
    _C_UN_D(f0, f1);
    l = _jit->x.pc;
    d = (((long)i0 - (long)l) >> 2) - 1;
    assert(_s16P(d));
    _BC1F(_jit_US(d));
    jit_nop(1);
    return (l);
}

#define jit_bunordr_f(i0, f0, f1)	mips_bunordr_f(_jit, i0, f0, f1)
__jit_inline jit_insn *
mips_bunordr_f(jit_state_t _jit, jit_insn * i0, jit_fpr_t f0, jit_fpr_t f1)
{
    jit_insn	*l;
    long	 d;
    _C_UN_S(f0, f1);
    l = _jit->x.pc;
    d = (((long)i0 - (long)l) >> 2) - 1;
    assert(_s16P(d));
    _BC1T(_jit_US(d));
    jit_nop(1);
    return (l);
}

#define jit_bunordr_d(i0, f0, f1)	mips_bunordr_d(_jit, i0, f0, f1)
__jit_inline jit_insn *
mips_bunordr_d(jit_state_t _jit, jit_insn * i0, jit_fpr_t f0, jit_fpr_t f1)
{
    jit_insn	*l;
    long	 d;
    _C_UN_D(f0, f1);
    l = _jit->x.pc;
    d = (((long)i0 - (long)l) >> 2) - 1;
    assert(_s16P(d));
    _BC1T(_jit_US(d));
    jit_nop(1);
    return (l);
}

#if LIGHTNING_CROSS \
	? LIGHTNING_TARGET == LIGHTNING_MIPS64 \
	: defined (__mips64)
#  include "fp-64.h"
#else
#  include "fp-32.h"
#endif

#endif /* __lightning_fp_mips_h */
