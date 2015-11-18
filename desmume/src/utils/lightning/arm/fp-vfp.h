/******************************** -*- C -*- ****************************
 *
 *	Support macros for arm VFP floating-point math
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
 * Free Software Foundation, 51 Franklin Street, Fifth Floor, Boston,
 * MA 02110-1301, USA.
 *
 * Authors:
 *	Paulo Cesar Pereira de Andrade
 ***********************************************************************/


#ifndef __lightning_fp_vfp_h
#define __lightning_fp_vfp_h

__jit_inline void
vfp_movr_f(jit_state_t _jit, jit_fpr_t r0, jit_fpr_t r1)
{
    if (r0 != r1) {
	if (r0 == JIT_FPRET) {
	    /* jit_ret() must follow! */
	    if (!jit_hardfp_p())
		_VMOV_A_S(_R0, r1);
	    else
		_VMOV_F32(_D0, r1);
	}
	else
	    _VMOV_F32(r0, r1);
    }
}

__jit_inline void
vfp_movr_d(jit_state_t _jit, jit_fpr_t r0, jit_fpr_t r1)
{
    if (r0 != r1) {
	if (r0 == JIT_FPRET) {
	    /* jit_ret() must follow! */
	    if (!jit_hardfp_p())
		_VMOV_AA_D(_R0, _R1, r1);
	    else
		_VMOV_F64(_D0, r1);
	}
	else
	    _VMOV_F64(r0, r1);
    }
}

__jit_inline void
vfp_movi_f(jit_state_t _jit, jit_fpr_t r0, float i0)
{
    union {
	int	i;
	float	f;
    } u;
    int		code;
    u.f = i0;
    if (!jit_hardfp_p() && r0 == JIT_FPRET)
	/* jit_ret() must follow! */
	jit_movi_i(_R0, u.i);
    else {
	if (r0 == JIT_FPRET)
	    r0 = _D0;
	if ((code = encode_vfp_double(1, 0, u.i, u.i)) != -1 ||
	    (code = encode_vfp_double(1, 1, ~u.i, ~u.i)) != -1)
	    _VIMM(code, r0);
	else {
	    jit_movi_i(JIT_FTMP, u.i);
	    _VMOV_S_A(r0, JIT_FTMP);
	}
    }
}

__jit_inline void
vfp_movi_d(jit_state_t _jit, jit_fpr_t r0, double i0)
{
    union {
	int	i[2];
	double	d;
    } u;
    int		code;
    u.d = i0;
    if (!jit_hardfp_p() && r0 == JIT_FPRET) {
	/* jit_ret() must follow! */
	jit_movi_i(_R0, u.i[0]);
	jit_movi_i(_R1, u.i[1]);
    }
    else {
	if (r0 == JIT_FPRET)
	    r0 = _D0;
	if ((code = encode_vfp_double(1, 0, u.i[0], u.i[1])) != -1 ||
	    (code = encode_vfp_double(1, 1, ~u.i[0], ~u.i[1])) != -1)
	    _VIMM(code, r0);
	else {
	    jit_movi_i(JIT_TMP, u.i[0]);
	    jit_movi_i(JIT_FTMP, u.i[1]);
	    _VMOV_D_AA(r0, JIT_TMP, JIT_FTMP);
	}
    }
}

__jit_inline void
vfp_extr_i_f(jit_state_t _jit, jit_fpr_t r0, jit_gpr_t r1)
{
    _VMOV_V_I32(r0, r1);
    _VCVT_F32_S32(r0, r0);
}

__jit_inline void
vfp_extr_i_d(jit_state_t _jit, jit_fpr_t r0, jit_gpr_t r1)
{
    _VMOV_V_I32(r0, r1);
    _VCVT_F64_S32(r0, r0);
}

#define vfp_extr_d_f(_jit, r0, r1)	_VCVT_F64_F32(r0, r1)
#define vfp_extr_f_d(_jit, r0, r1)	_VCVT_F32_F64(r0, r1)

/* FIXME keeping for now, in case there exists, and/or to support
 * only 4 double precision registers */
#define vfp_get_tmp(r, n)						\
    jit_fpr_t		 tmp;						\
    if (0) {								\
	tmp = r == _D0 ? _D1 : _D0;					\
	_VPUSH_F##n(tmp, 2);						\
    }									\
    else								\
	tmp = _D15
#define vfp_unget_tmp(n)						\
    if (0)								\
	_VPOP_F##n(tmp, 2)

__jit_inline void
vfp_rintr_f_i(jit_state_t _jit, jit_gpr_t r0, jit_fpr_t r1)
{
    vfp_get_tmp(r1, 32);
    _VCVT_S32_F32(tmp, r1);
    _VMOV_A_S32(r0, tmp);
    vfp_unget_tmp(32);
}

__jit_inline void
vfp_rintr_d_i(jit_state_t _jit, jit_gpr_t r0, jit_fpr_t r1)
{
    vfp_get_tmp(r1, 64);
    _VCVT_S32_F64(tmp, r1);
    _VMOV_A_S32(r0, tmp);
    vfp_unget_tmp(64);
}

__jit_inline void
vfp_roundr_f_i(jit_state_t _jit, jit_gpr_t r0, jit_fpr_t r1)
{
    vfp_get_tmp(r1, 32);
    _VMOV_A_S(JIT_FTMP, r1);
    if (jit_thumb_p()) {
	T2_TSTI(JIT_FTMP, encode_thumb_immediate(0x80000000));
	_ITE(ARM_CC_NE);
	/* add -0.5 if negative */
	T2_MOVI(JIT_FTMP, encode_thumb_immediate(0xbf000000));
	/* add 0.5 if positive */
	T2_MOVI(JIT_FTMP, encode_thumb_immediate(0x3f000000));
    }
    else {
	if (jit_armv6_p())
	    _TSTI(JIT_FTMP, encode_arm_immediate(0x80000000));
	else
	    _ANDSI(JIT_FTMP, JIT_FTMP, encode_arm_immediate(0x80000000));
	/* add -0.5 if negative */
	_CC_MOVI(ARM_CC_NE, JIT_FTMP, encode_arm_immediate(0xbf000000));
	/* add 0.5 if positive */
	_CC_MOVI(ARM_CC_EQ, JIT_FTMP, encode_arm_immediate(0x3f000000));
    }
    _VMOV_S_A(tmp, JIT_FTMP);
    _VADD_F32(tmp, r1, tmp);
    /* truncate to zero */
    _VCVT_S32_F32(tmp, tmp);
    _VMOV_A_S32(r0, tmp);
    vfp_unget_tmp(32);
}

__jit_inline void
vfp_roundr_d_i(jit_state_t _jit, jit_gpr_t r0, jit_fpr_t r1)
{
    vfp_get_tmp(r1, 64);
    _VMOV_AA_D(JIT_TMP, JIT_FTMP, r1);
    if (jit_thumb_p()) {
	T2_TSTI(JIT_FTMP, encode_thumb_immediate(0x80000000));
	T2_MOVI(JIT_FTMP, encode_thumb_immediate(0x0fe00000));
	_ITE(ARM_CC_NE);
	/* add -0.5 if negative */
	T2_ORRI(JIT_FTMP, JIT_FTMP, encode_thumb_immediate(0xb0000000));
	/* add 0.5 if positive */
	T2_ORRI(JIT_FTMP, JIT_FTMP, encode_thumb_immediate(0x30000000));
    }
    else {
	if (jit_armv6_p())
	    _TSTI(JIT_FTMP, encode_arm_immediate(0x80000000));
	else
	    _ANDSI(JIT_FTMP, JIT_FTMP, encode_arm_immediate(0x80000000));
	_MOVI(JIT_FTMP, encode_arm_immediate(0x0fe00000));
	/* add -0.5 if negative */
	_CC_ORRI(ARM_CC_NE, JIT_FTMP, JIT_FTMP,
		 encode_arm_immediate(0xb0000000));
	/* add 0.5 if positive */
	_CC_ORRI(ARM_CC_EQ, JIT_FTMP, JIT_FTMP,
		 encode_arm_immediate(0x30000000));
    }
    jit_movi_i(JIT_TMP, 0);
    _VMOV_D_AA(tmp, JIT_TMP, JIT_FTMP);
    _VADD_F64(tmp, r1, tmp);
    /* truncate to zero */
    _VCVT_S32_F64(tmp, tmp);
    _VMOV_A_S32(r0, tmp);
    vfp_unget_tmp(64);
}

#define vfp_truncr_f_i(_jit, r0, r1)	vfp_rintr_f_i(_jit, r0, r1)
#define vfp_truncr_d_i(_jit, r0, r1)	vfp_rintr_d_i(_jit, r0, r1)

__jit_inline void
vfp_ceilr_f_i(jit_state_t _jit, jit_gpr_t r0, jit_fpr_t r1)
{
    vfp_get_tmp(r1, 32);
    _VMRS(JIT_TMP);
    if (jit_thumb_p()) {
	T2_BICI(JIT_FTMP, JIT_TMP, encode_thumb_immediate(FPSCR_RMASK));
	T2_ORRI(JIT_FTMP, JIT_FTMP, encode_thumb_immediate(FPSCR_RP));
    }
    else {
	_BICI(JIT_FTMP, JIT_TMP, encode_arm_immediate(FPSCR_RMASK));
	_ORRI(JIT_FTMP, JIT_FTMP, encode_arm_immediate(FPSCR_RP));
    }
    _VMSR(JIT_FTMP);
    _VCVTR_S32_F32(tmp, r1);
    _VMOV_A_S32(r0, tmp);
    _VMSR(JIT_TMP);
    vfp_unget_tmp(32);
}

__jit_inline void
vfp_ceilr_d_i(jit_state_t _jit, jit_gpr_t r0, jit_fpr_t r1)
{
    vfp_get_tmp(r1, 64);
    _VMRS(JIT_TMP);
    if (jit_thumb_p()) {
	T2_BICI(JIT_FTMP, JIT_TMP, encode_thumb_immediate(FPSCR_RMASK));
	T2_ORRI(JIT_FTMP, JIT_FTMP, encode_thumb_immediate(FPSCR_RP));
    }
    else {
	_BICI(JIT_FTMP, JIT_TMP, encode_arm_immediate(FPSCR_RMASK));
	_ORRI(JIT_FTMP, JIT_FTMP, encode_arm_immediate(FPSCR_RP));
    }
    _VMSR(JIT_FTMP);
    _VCVTR_S32_F64(tmp, r1);
    _VMOV_A_S32(r0, tmp);
    _VMSR(JIT_TMP);
    vfp_unget_tmp(64);
}

__jit_inline void
vfp_floorr_f_i(jit_state_t _jit, jit_gpr_t r0, jit_fpr_t r1)
{
    vfp_get_tmp(r1, 32);
    _VMRS(JIT_TMP);
    if (jit_thumb_p()) {
	T2_BICI(JIT_FTMP, JIT_TMP, encode_thumb_immediate(FPSCR_RMASK));
	T2_ORRI(JIT_FTMP, JIT_FTMP, encode_thumb_immediate(FPSCR_RM));
    }
    else {
	_BICI(JIT_FTMP, JIT_TMP, encode_arm_immediate(FPSCR_RMASK));
	_ORRI(JIT_FTMP, JIT_FTMP, encode_arm_immediate(FPSCR_RM));
    }
    _VMSR(JIT_FTMP);
    _VCVTR_S32_F32(tmp, r1);
    _VMOV_A_S32(r0, tmp);
    _VMSR(JIT_TMP);
    vfp_unget_tmp(32);
}

__jit_inline void
vfp_floorr_d_i(jit_state_t _jit, jit_gpr_t r0, jit_fpr_t r1)
{
    vfp_get_tmp(r1, 64);
    _VMRS(JIT_TMP);
    if (jit_thumb_p()) {
	T2_BICI(JIT_FTMP, JIT_TMP, encode_thumb_immediate(FPSCR_RMASK));
	T2_ORRI(JIT_FTMP, JIT_FTMP, encode_thumb_immediate(FPSCR_RM));
    }
    else {
	_BICI(JIT_FTMP, JIT_TMP, encode_arm_immediate(FPSCR_RMASK));
	_ORRI(JIT_FTMP, JIT_FTMP, encode_arm_immediate(FPSCR_RM));
    }
    _VMSR(JIT_FTMP);
    _VCVTR_S32_F64(tmp, r1);
    _VMOV_A_S32(r0, tmp);
    _VMSR(JIT_TMP);
    vfp_unget_tmp(64);
}

#define vfp_absr_f(_jit, r0, r1)	_VABS_F32(r0, r1)
#define vfp_absr_d(_jit, r0, r1)	_VABS_F64(r0, r1)
#define vfp_negr_f(_jit, r0, r1)	_VNEG_F32(r0, r1)
#define vfp_negr_d(_jit, r0, r1)	_VNEG_F64(r0, r1)
#define vfp_sqrtr_f(_jit, r0, r1)	_VSQRT_F32(r0, r1)
#define vfp_sqrtr_d(_jit, r0, r1)	_VSQRT_F64(r0, r1)
#define vfp_addr_f(_jit, r0, r1, r2)	_VADD_F32(r0, r1, r2)
#define vfp_addr_d(_jit, r0, r1, r2)	_VADD_F64(r0, r1, r2)
#define vfp_subr_f(_jit, r0, r1, r2)	_VSUB_F32(r0, r1, r2)
#define vfp_subr_d(_jit, r0, r1, r2)	_VSUB_F64(r0, r1, r2)
#define vfp_mulr_f(_jit, r0, r1, r2)	_VMUL_F32(r0, r1, r2)
#define vfp_mulr_d(_jit, r0, r1, r2)	_VMUL_F64(r0, r1, r2)
#define vfp_divr_f(_jit, r0, r1, r2)	_VDIV_F32(r0, r1, r2)
#define vfp_divr_d(_jit, r0, r1, r2)	_VDIV_F64(r0, r1, r2)

static void
_vcmp01_x(jit_state_t _jit, int c0, int c1, jit_gpr_t r0)
{
    _VMRS(_R15);
    if (jit_thumb_p()) {
	if ((c0 ^ c1) >> 28 == 1) {
	    _ITE(c0);
	    if (r0 < 8) {
		T1_MOVI(r0, 0);
		T1_MOVI(r0, 1);
	    }
	    else {
		T2_MOVI(r0, 0);
		T2_MOVI(r0, 1);
	    }
	}
	else {
	    if (r0 < 8) {
		_IT(c0);
		T1_MOVI(r0, 0);
		_IT(c1);
		T1_MOVI(r0, 1);
	    }
	    else {
		_IT(c0);
		T2_MOVI(r0, 0);
		_IT(c1);
		T2_MOVI(r0, 1);
	    }
	}
    }
    else {
	_CC_MOVI(c0, r0, 0);
	_CC_MOVI(c1, r0, 1);
    }
}

static void
_vcmp01_f(jit_state_t _jit, int c0, int c1,
	  jit_gpr_t r0, jit_fpr_t r1, jit_fpr_t r2)
{
    _VCMP_F32(r1, r2);
    _vcmp01_x(_jit, c0, c1, r0);
}

static void
_vcmp01_d(jit_state_t _jit, int c0, int c1,
	  jit_gpr_t r0, jit_fpr_t r1, jit_fpr_t r2)
{
    _VCMP_F64(r1, r2);
    _vcmp01_x(_jit, c0, c1, r0);
}

#define vfp_ltr_f(_jit,r0,r1,r2)	_vcmp01_f(_jit,ARM_CC_PL,ARM_CC_MI,r0,r1,r2)
#define vfp_ltr_d(_jit,r0,r1,r2)	_vcmp01_d(_jit,ARM_CC_PL,ARM_CC_MI,r0,r1,r2)
#define vfp_ler_f(_jit,r0,r1,r2)	_vcmp01_f(_jit,ARM_CC_HS,ARM_CC_LS,r0,r1,r2)
#define vfp_ler_d(_jit,r0,r1,r2)	_vcmp01_d(_jit,ARM_CC_HS,ARM_CC_LS,r0,r1,r2)
#define vfp_eqr_f(_jit,r0,r1,r2)	_vcmp01_f(_jit,ARM_CC_NE,ARM_CC_EQ,r0,r1,r2)
#define vfp_eqr_d(_jit,r0,r1,r2)	_vcmp01_d(_jit,ARM_CC_NE,ARM_CC_EQ,r0,r1,r2)
#define vfp_ger_f(_jit,r0,r1,r2)	_vcmp01_f(_jit,ARM_CC_LT,ARM_CC_GE,r0,r1,r2)
#define vfp_ger_d(_jit,r0,r1,r2)	_vcmp01_d(_jit,ARM_CC_LT,ARM_CC_GE,r0,r1,r2)
#define vfp_gtr_f(_jit,r0,r1,r2)	_vcmp01_f(_jit,ARM_CC_LE,ARM_CC_GT,r0,r1,r2)
#define vfp_gtr_d(_jit,r0,r1,r2)	_vcmp01_d(_jit,ARM_CC_LE,ARM_CC_GT,r0,r1,r2)
#define vfp_ner_f(_jit,r0,r1,r2)	_vcmp01_f(_jit,ARM_CC_EQ,ARM_CC_NE,r0,r1,r2)
#define vfp_ner_d(_jit,r0,r1,r2)	_vcmp01_d(_jit,ARM_CC_EQ,ARM_CC_NE,r0,r1,r2)

static void
_vcmp10_x(jit_state_t _jit, int cc, jit_gpr_t r0)
{
    if (jit_thumb_p()) {
	if (r0 < 8) {
	    T1_MOVI(r0, 1);
	    _VMRS(_R15);
	    _IT(cc);
	    T1_MOVI(r0, 0);
	}
	else {
	    T2_MOVI(r0, 1);
	    _VMRS(_R15);
	    _IT(cc);
	    T2_MOVI(r0, 0);
	}
    }
    else {
	_VMRS(_R15);
	_MOVI(r0, 1);
	_CC_MOVI(cc, r0, 0);
    }
}

static void
_vcmp_10_f(jit_state_t _jit, int cc, jit_gpr_t r0, jit_fpr_t r1, jit_fpr_t r2)
{
    _VCMP_F32(r1, r2);
    _vcmp10_x(_jit, cc, r0);
}

static void
_vcmp_10_d(jit_state_t _jit, int cc, jit_gpr_t r0, jit_fpr_t r1, jit_fpr_t r2)
{
    _VCMP_F64(r1, r2);
    _vcmp10_x(_jit, cc, r0);
}

#define vfp_unltr_f(_jit,r0,r1,r2)	_vcmp_10_f(_jit,ARM_CC_GE,r0,r1,r2)
#define vfp_unltr_d(_jit,r0,r1,r2)	_vcmp_10_d(_jit,ARM_CC_GE,r0,r1,r2)
#define vfp_unler_f(_jit,r0,r1,r2)	_vcmp_10_f(_jit,ARM_CC_GT,r0,r1,r2)
#define vfp_unler_d(_jit,r0,r1,r2)	_vcmp_10_d(_jit,ARM_CC_GT,r0,r1,r2)

static void
vfp_uneqr_x(jit_state_t _jit, jit_gpr_t r0)
{
    _VMRS(_R15);
    if (jit_thumb_p()) {
	_ITE(ARM_CC_NE);
	if (r0 < 8) {
	    T1_MOVI(r0, 0);
	    T1_MOVI(r0, 1);
	    _IT(ARM_CC_VS);
	    T1_MOVI(r0, 1);
	}
	else {
	    T2_MOVI(r0, 0);
	    T2_MOVI(r0, 1);
	    _IT(ARM_CC_VS);
	    T2_MOVI(r0, 1);
	}
    }
    else {
	_CC_MOVI(ARM_CC_NE, r0, 0);
	_CC_MOVI(ARM_CC_EQ, r0, 1);
	_CC_MOVI(ARM_CC_VS, r0, 1);
    }
}

__jit_inline void
vfp_uneqr_f(jit_state_t _jit, jit_gpr_t r0, jit_fpr_t r1, jit_fpr_t r2)
{
    _VCMP_F32(r1, r2);
    vfp_uneqr_x(_jit, r0);
}

__jit_inline void
vfp_uneqr_d(jit_state_t _jit, jit_gpr_t r0, jit_fpr_t r1, jit_fpr_t r2)
{
    _VCMP_F64(r1, r2);
    vfp_uneqr_x(_jit, r0);
}

static void
_vcmp_01_x(jit_state_t _jit, int cc, jit_gpr_t r0)
{
    if (jit_thumb_p()) {
	if (r0 < 8) {
	    T1_MOVI(r0, 0);
	    _VMRS(_R15);
	    _IT(cc);
	    T1_MOVI(r0, 1);
	}
	else {
	    T2_MOVI(r0, 0);
	    _VMRS(_R15);
	    _IT(cc);
	    T2_MOVI(r0, 1);
	}
    }
    else {
	_VMRS(_R15);
	_MOVI(r0, 0);
	_CC_MOVI(cc, r0, 1);
    }
}

static void
_vcmp_01_f(jit_state_t _jit, int cc, jit_gpr_t r0, jit_fpr_t r1, jit_fpr_t r2)
{
    _VCMP_F32(r1, r2);
    _vcmp_01_x(_jit, cc, r0);
}

static void
_vcmp_01_d(jit_state_t _jit, int cc, jit_gpr_t r0, jit_fpr_t r1, jit_fpr_t r2)
{
    _VCMP_F64(r1, r2);
    _vcmp_01_x(_jit, cc, r0);
}

#define vfp_unger_f(_jit,r0,r1,r2)	_vcmp_01_f(_jit,ARM_CC_CS,r0,r1,r2)
#define vfp_unger_d(_jit,r0,r1,r2)	_vcmp_01_d(_jit,ARM_CC_CS,r0,r1,r2)
#define vfp_ungtr_f(_jit,r0,r1,r2)	_vcmp_01_f(_jit,ARM_CC_HI,r0,r1,r2)
#define vfp_ungtr_d(_jit,r0,r1,r2)	_vcmp_01_d(_jit,ARM_CC_HI,r0,r1,r2)

static void
_vfp_ltgtr_x(jit_state_t _jit, jit_gpr_t r0)
{
    _VMRS(_R15);
    if (jit_thumb_p()) {
	_ITE(ARM_CC_NE);
	if (r0 < 8) {
	    T1_MOVI(r0, 1);
	    T1_MOVI(r0, 0);
	    _IT(ARM_CC_VS);
	    T1_MOVI(r0, 0);
	}
	else {
	    T2_MOVI(r0, 1);
	    T2_MOVI(r0, 0);
	    _IT(ARM_CC_VS);
	    T2_MOVI(r0, 0);
	}
    }
    else {
	_CC_MOVI(ARM_CC_NE, r0, 1);
	_CC_MOVI(ARM_CC_EQ, r0, 0);
	_CC_MOVI(ARM_CC_VS, r0, 0);
    }
}

__jit_inline void
vfp_ltgtr_f(jit_state_t _jit, jit_gpr_t r0, jit_fpr_t r1, jit_fpr_t r2)
{
    _VCMP_F32(r1, r2);
    _vfp_ltgtr_x(_jit, r0);
}

__jit_inline void
vfp_ltgtr_d(jit_state_t _jit, jit_gpr_t r0, jit_fpr_t r1, jit_fpr_t r2)
{
    _VCMP_F64(r1, r2);
    _vfp_ltgtr_x(_jit, r0);
}

__jit_inline void
vfp_ordr_f(jit_state_t _jit, jit_gpr_t r0, jit_fpr_t r1, jit_fpr_t r2)
{
    _VCMP_F32(r1, r2);
    _vcmp10_x(_jit, ARM_CC_VS, r0);
}

__jit_inline void
vfp_ordr_d(jit_state_t _jit, jit_gpr_t r0, jit_fpr_t r1, jit_fpr_t r2)
{
    _VCMP_F64(r1, r2);
    _vcmp10_x(_jit, ARM_CC_VS, r0);
}

__jit_inline void
vfp_unordr_f(jit_state_t _jit, jit_gpr_t r0, jit_fpr_t r1, jit_fpr_t r2)
{
    _VCMP_F32(r1, r2);
    _vcmp_01_x(_jit, ARM_CC_VS, r0);
}

__jit_inline void
vfp_unordr_d(jit_state_t _jit, jit_gpr_t r0, jit_fpr_t r1, jit_fpr_t r2)
{
    _VCMP_F64(r1, r2);
    _vcmp_01_x(_jit, ARM_CC_VS, r0);
}

static jit_insn *
_vbcmp_x(jit_state_t _jit, int cc, jit_insn *i0)
{
    jit_insn	*l;
    long	 d;
    _VMRS(_R15);
    l = _jit->x.pc;
    if (jit_thumb_p()) {
	d = (((long)i0 - (long)l) >> 1) - 2;
	assert(_s20P(d));
	T2_CC_B(cc, encode_thumb_cc_jump(d));
    }
    else {
	d = (((long)i0 - (long)l) >> 2) - 2;
	assert(_s24P(d));
	_CC_B(cc, d & 0x00ffffff);
    }
    return (l);
}

__jit_inline jit_insn *
_vbcmp_f(jit_state_t _jit, int cc, jit_insn *i0, jit_fpr_t r0, jit_fpr_t r1)
{
    _VCMP_F32(r0, r1);
    return (_vbcmp_x(_jit, cc, i0));
}

__jit_inline jit_insn *
_vbcmp_d(jit_state_t _jit, int cc, jit_insn *i0, jit_fpr_t r0, jit_fpr_t r1)
{
    _VCMP_F64(r0, r1);
    return (_vbcmp_x(_jit, cc, i0));
}

#define vfp_bltr_f(_jit,i0,r0,r1)	_vbcmp_f(_jit,ARM_CC_MI,i0,r0,r1)
#define vfp_bltr_d(_jit,i0,r0,r1)	_vbcmp_d(_jit,ARM_CC_MI,i0,r0,r1)
#define vfp_bler_f(_jit,i0,r0,r1)	_vbcmp_f(_jit,ARM_CC_LS,i0,r0,r1)
#define vfp_bler_d(_jit,i0,r0,r1)	_vbcmp_d(_jit,ARM_CC_LS,i0,r0,r1)
#define vfp_beqr_f(_jit,i0,r0,r1)	_vbcmp_f(_jit,ARM_CC_EQ,i0,r0,r1)
#define vfp_beqr_d(_jit,i0,r0,r1)	_vbcmp_d(_jit,ARM_CC_EQ,i0,r0,r1)
#define vfp_bger_f(_jit,i0,r0,r1)	_vbcmp_f(_jit,ARM_CC_GE,i0,r0,r1)
#define vfp_bger_d(_jit,i0,r0,r1)	_vbcmp_d(_jit,ARM_CC_GE,i0,r0,r1)
#define vfp_bgtr_f(_jit,i0,r0,r1)	_vbcmp_f(_jit,ARM_CC_GT,i0,r0,r1)
#define vfp_bgtr_d(_jit,i0,r0,r1)	_vbcmp_d(_jit,ARM_CC_GT,i0,r0,r1)
#define vfp_bner_f(_jit,i0,r0,r1)	_vbcmp_f(_jit,ARM_CC_NE,i0,r0,r1)
#define vfp_bner_d(_jit,i0,r0,r1)	_vbcmp_d(_jit,ARM_CC_NE,i0,r0,r1)

static jit_insn *
_vbncmp_x(jit_state_t _jit, int cc, jit_insn *i0)
{
    jit_insn	*i;
    jit_insn	*l;
    long	 d;
    _VMRS(_R15);
    i = _jit->x.pc;
    if (jit_thumb_p()) {
	T2_CC_B(cc, 0);
	l = _jit->x.pc;
	d = (((long)i0 - (long)l) >> 1) - 2;
	assert(_s24P(d));
	T2_B(encode_thumb_jump(d));
    }
    else {
	_CC_B(cc, 0);
	l = _jit->x.pc;
	d = (((long)i0 - (long)l) >> 2) - 2;
	assert(_s24P(d));
	_B(d & 0x00ffffff);
    }
    jit_patch(i);
    return (l);
}

__jit_inline jit_insn *
_vbncmp_f(jit_state_t _jit, int cc, jit_insn *i0, jit_fpr_t r0, jit_fpr_t r1)
{
    _VCMP_F32(r0, r1);
    return (_vbncmp_x(_jit, cc, i0));
}

__jit_inline jit_insn *
_vbncmp_d(jit_state_t _jit, int cc, jit_insn *i0, jit_fpr_t r0, jit_fpr_t r1)
{
    _VCMP_F64(r0, r1);
    return (_vbncmp_x(_jit, cc, i0));
}

#define vfp_bunltr_f(_jit,i0,r0,r1)	_vbncmp_f(_jit,ARM_CC_GE,i0,r0,r1)
#define vfp_bunltr_d(_jit,i0,r0,r1)	_vbncmp_d(_jit,ARM_CC_GE,i0,r0,r1)
#define vfp_bunler_f(_jit,i0,r0,r1)	_vbncmp_f(_jit,ARM_CC_GT,i0,r0,r1)
#define vfp_bunler_d(_jit,i0,r0,r1)	_vbncmp_d(_jit,ARM_CC_GT,i0,r0,r1)

static jit_insn *
vfp_buneqr_x(jit_state_t _jit, jit_insn *i0)
{
    jit_insn	*i;
    jit_insn	*j;
    jit_insn	*l;
    long	 d;
    _VMRS(_R15);
    i = _jit->x.pc;
    if (jit_thumb_p()) {
	T2_CC_B(ARM_CC_VS, 0);
	j = _jit->x.pc;
	T2_CC_B(ARM_CC_NE, 0);
	jit_patch(i);
	l = _jit->x.pc;
	d = (((long)i0 - (long)l) >> 1) - 2;
	assert(_s20P(d));
	T2_B(encode_thumb_jump(d));
    }
    else {
	_CC_B(ARM_CC_VS, 0);
	j = _jit->x.pc;
	_CC_B(ARM_CC_NE, 0);
	jit_patch(i);
	l = _jit->x.pc;
	d = (((long)i0 - (long)l) >> 2) - 2;
	assert(_s24P(d));
	_B(d & 0x00ffffff);
    }
    jit_patch(j);
    return (l);
}

__jit_inline jit_insn *
vfp_buneqr_f(jit_state_t _jit, jit_insn *i0, jit_fpr_t r0, jit_fpr_t r1)
{
    _VCMP_F32(r0, r1);
    return (vfp_buneqr_x(_jit, i0));
}

__jit_inline jit_insn *
vfp_buneqr_d(jit_state_t _jit, jit_insn *i0, jit_fpr_t r0, jit_fpr_t r1)
{
    _VCMP_F64(r0, r1);
    return (vfp_buneqr_x(_jit, i0));
}

static jit_insn *
vfp_bunger_x(jit_state_t _jit, jit_insn *i0)
{
    jit_insn	*i;
    jit_insn	*l;
    long	 d;
    _VMRS(_R15);
    i = _jit->x.pc;
    if (jit_thumb_p()) {
	T2_CC_B(ARM_CC_MI, 0);
	l = _jit->x.pc;
	d = (((long)i0 - (long)l) >> 1) - 2;
	assert(_s20P(d));
	T2_CC_B(ARM_CC_HS, encode_thumb_cc_jump(d));

    }
    else {
	_CC_B(ARM_CC_MI, 0);
	l = _jit->x.pc;
	d = (((long)i0 - (long)l) >> 2) - 2;
	assert(_s24P(d));
	_CC_B(ARM_CC_HS, d & 0x00ffffff);
    }
    jit_patch(i);
    return (l);
}

__jit_inline jit_insn *
vfp_bunger_f(jit_state_t _jit, jit_insn *i0, jit_fpr_t r0, jit_fpr_t r1)
{
    _VCMP_F32(r0, r1);
    return (vfp_bunger_x(_jit, i0));
}

__jit_inline jit_insn *
vfp_bunger_d(jit_state_t _jit, jit_insn *i0, jit_fpr_t r0, jit_fpr_t r1)
{
    _VCMP_F64(r0, r1);
    return (vfp_bunger_x(_jit, i0));
}

#define vfp_bungtr_f(_jit,i0,r0,r1)	_vbcmp_f(_jit,ARM_CC_HI,i0,r0,r1)
#define vfp_bungtr_d(_jit,i0,r0,r1)	_vbcmp_d(_jit,ARM_CC_HI,i0,r0,r1)

static jit_insn *
vfp_bltgtr_x(jit_state_t _jit, jit_insn *i0)
{
    jit_insn	*i;
    jit_insn	*j;
    jit_insn	*l;
    long	 d;
    _VMRS(_R15);
    i = _jit->x.pc;
    if (jit_thumb_p()) {
	T2_CC_B(ARM_CC_VS, 0);
	j = _jit->x.pc;
	T2_CC_B(ARM_CC_EQ, 0);
	l = _jit->x.pc;
	d = (((long)i0 - (long)l) >> 1) - 2;
	assert(_s24P(d));
	T2_B(encode_thumb_jump(d));
    }
    else {
	_CC_B(ARM_CC_VS, 0);
	j = _jit->x.pc;
	_CC_B(ARM_CC_EQ, 0);
	l = _jit->x.pc;
	d = (((long)i0 - (long)l) >> 2) - 2;
	assert(_s24P(d));
	_B(d & 0x00ffffff);
    }
    jit_patch(i);
    jit_patch(j);
    return (l);
}

__jit_inline jit_insn *
vfp_bltgtr_f(jit_state_t _jit, jit_insn *i0, jit_fpr_t r0, jit_fpr_t r1)
{
    _VCMP_F32(r0, r1);
    return (vfp_bltgtr_x(_jit, i0));
}

__jit_inline jit_insn *
vfp_bltgtr_d(jit_state_t _jit, jit_insn *i0, jit_fpr_t r0, jit_fpr_t r1)
{
    _VCMP_F64(r0, r1);
    return (vfp_bltgtr_x(_jit, i0));
}

#define vfp_bordr_f(_jit,i0,r0,r1)	_vbcmp_f(_jit,ARM_CC_VC,i0,r0,r1)
#define vfp_bordr_d(_jit,i0,r0,r1)	_vbcmp_d(_jit,ARM_CC_VC,i0,r0,r1)
#define vfp_bunordr_f(_jit,i0,r0,r1)	_vbcmp_f(_jit,ARM_CC_VS,i0,r0,r1)
#define vfp_bunordr_d(_jit,i0,r0,r1)	_vbcmp_d(_jit,ARM_CC_VS,i0,r0,r1)
#define vfp_ldr_f(_jit, r0, r1)		_VLDR_F32(r0, r1, 0)
#define vfp_ldr_d(_jit, r0, r1)		_VLDR_F64(r0, r1, 0)

__jit_inline void
vfp_ldi_f(jit_state_t _jit, jit_fpr_t r0, void *i0)
{
    jit_movi_i(JIT_FTMP, (int)i0);
    _VLDR_F32(r0, JIT_FTMP, 0);
}

__jit_inline void
vfp_ldi_d(jit_state_t _jit, jit_fpr_t r0, void *i0)
{
    jit_movi_i(JIT_FTMP, (int)i0);
    _VLDR_F64(r0, JIT_FTMP, 0);
}

__jit_inline void
vfp_ldxr_f(jit_state_t _jit, jit_fpr_t r0, jit_gpr_t r1, jit_gpr_t r2)
{
    jit_addr_i(JIT_FTMP, r1, r2);
    _VLDR_F32(r0, JIT_FTMP, 0);
}

__jit_inline void
vfp_ldxr_d(jit_state_t _jit, jit_fpr_t r0, jit_gpr_t r1, jit_gpr_t r2)
{
    jit_addr_i(JIT_FTMP, r1, r2);
    _VLDR_F64(r0, JIT_FTMP, 0);
}

__jit_inline void
vfp_ldxi_f(jit_state_t _jit, jit_fpr_t r0, jit_gpr_t r1, int i0)
{
    if (i0 >= 0) {
	assert(!(i0 & 3));
	i0 >>= 2;
	if (i0 < 256)
	    _VLDR_F32(r0, r1, i0);
	else {
	    jit_addi_i(JIT_FTMP, r1, i0);
	    _VLDR_F32(r0, JIT_FTMP, 0);
	}
    }
    else {
	i0 = -i0;
	assert(!(i0 & 3));
	i0 >>= 2;
	if (i0 < 256)
	    _VLDRN_F32(r0, r1, i0);
	else {
	    jit_subi_i(JIT_FTMP, r1, i0);
	    _VLDR_F32(r0, JIT_FTMP, 0);
	}
    }
}

__jit_inline void
vfp_ldxi_d(jit_state_t _jit, jit_fpr_t r0, jit_gpr_t r1, int i0)
{
    if (i0 >= 0) {
	assert(!(i0 & 3));
	i0 >>= 2;
	if (i0 < 256)
	    _VLDR_F64(r0, r1, i0);
	else {
	    jit_addi_i(JIT_FTMP, r1, i0);
	    _VLDR_F64(r0, JIT_FTMP, 0);
	}
    }
    else {
	i0 = -i0;
	assert(!(i0 & 3));
	i0 >>= 2;
	if (i0 < 256)
	    _VLDRN_F64(r0, r1, i0);
	else {
	    jit_subi_i(JIT_FTMP, r1, i0);
	    _VLDR_F64(r0, JIT_FTMP, 0);
	}
    }
}

#define vfp_str_f(_jit, r0, r1)		_VSTR_F32(r1, r0, 0)
#define vfp_str_d(_jit, r0, r1)		_VSTR_F64(r1, r0, 0)

__jit_inline void
vfp_sti_f(jit_state_t _jit, void *i0, jit_fpr_t r0)
{
    jit_movi_i(JIT_FTMP, (int)i0);
    _VSTR_F32(r0, JIT_FTMP, 0);
}

__jit_inline void
vfp_sti_d(jit_state_t _jit, void *i0, jit_fpr_t r0)
{
    jit_movi_i(JIT_FTMP, (int)i0);
    _VSTR_F64(r0, JIT_FTMP, 0);
}

__jit_inline void
vfp_stxr_f(jit_state_t _jit, jit_gpr_t r0, jit_gpr_t r1, jit_fpr_t r2)
{
    jit_addr_i(JIT_FTMP, r0, r1);
    _VSTR_F32(r2, JIT_FTMP, 0);
}

__jit_inline void
vfp_stxr_d(jit_state_t _jit, jit_gpr_t r0, jit_gpr_t r1, jit_fpr_t r2)
{
    jit_addr_i(JIT_FTMP, r0, r1);
    _VSTR_F64(r2, JIT_FTMP, 0);
}

__jit_inline void
vfp_stxi_f(jit_state_t _jit, int i0, jit_gpr_t r0, jit_fpr_t r1)
{
    if (i0 >= 0) {
	assert(!(i0 & 3));
	i0 >>= 2;
	if (i0 < 256)
	    _VSTR_F32(r1, r0, i0);
	else {
	    jit_addi_i(JIT_FTMP, r0, i0);
	    _VSTR_F32(r1, JIT_FTMP, 0);
	}
    }
    else {
	i0 = -i0;
	assert(!(i0 & 3));
	i0 >>= 2;
	if (i0 < 256)
	    _VSTRN_F32(r1, r0, i0);
	else {
	    jit_subi_i(JIT_FTMP, r0, i0);
	    _VSTR_F32(r1, JIT_FTMP, 0);
	}
    }
}

__jit_inline void
vfp_stxi_d(jit_state_t _jit, int i0, jit_gpr_t r0, jit_fpr_t r1)
{
    if (i0 >= 0) {
	assert(!(i0 & 3));
	i0 >>= 2;
	if (i0 < 256)
	    _VSTR_F64(r1, r0, i0);
	else {
	    jit_addi_i(JIT_FTMP, r0, i0);
	    _VSTR_F64(r1, JIT_FTMP, 0);
	}
    }
    else {
	i0 = -i0;
	assert(!(i0 & 3));
	i0 >>= 2;
	if (i0 < 256)
	    _VSTRN_F64(r1, r0, i0);
	else {
	    jit_subi_i(JIT_FTMP, r0, i0);
	    _VSTR_F64(r1, JIT_FTMP, 0);
	}
    }
}

__jit_inline void
vfp_getarg_f(jit_state_t _jit, jit_fpr_t r0, int i0)
{
    if (jit_hardfp_p()) {
	if (i0 < 16) {
	    if (r0 != (jit_fpr_t)i0)
		_VMOV_F32(r0, (jit_fpr_t)i0);
	    return;
	}
    }
    else if (i0 < 4) {
	/* registers are already saved on stack and argument registers
	 * may have been clobbered */
#if 0
	_VMOV_S_A(r0, i0);
#else
	vfp_ldxi_f(_jit, r0, JIT_FP, i0 << 2);
#endif
	return;
    }
    vfp_ldxi_f(_jit, r0, JIT_FP, i0);
}

__jit_inline void
vfp_getarg_d(jit_state_t _jit, jit_fpr_t r0, int i0)
{
    if (jit_hardfp_p()) {
	if (i0 < 16) {
	    if (r0 != (jit_fpr_t)i0)
		_VMOV_F64(r0, (jit_fpr_t)i0);
	    return;
	}
    }
    else if (i0 < 4) {
	/* registers are already saved on stack and argument registers
	 * may have been clobbered */
#if 0
	_VMOV_D_AA(r0, i0, i0 + 1);
#else
	vfp_ldxi_d(_jit, r0, JIT_FP, i0 << 2);
#endif
	return;
    }
    vfp_ldxi_d(_jit, r0, JIT_FP, i0);
}

__jit_inline void
vfp_pusharg_f(jit_state_t _jit, jit_fpr_t r0)
{
    int		ofs = _jitl.nextarg_put++;
    assert(ofs < 256);
    _jitl.stack_offset -= sizeof(float);
    _jitl.arguments[ofs] = (int *)_jit->x.pc;
    _jitl.types[ofs >> 5] &= ~(1 << (ofs & 31));
    _VSTR_F32(r0, JIT_SP, 0);
}

__jit_inline void
vfp_pusharg_d(jit_state_t _jit, jit_fpr_t r0)
{
    int		ofs = _jitl.nextarg_put++;
    assert(ofs < 256);
    _jitl.stack_offset -= sizeof(double);
    _jitl.arguments[ofs] = (int *)_jit->x.pc;
    _jitl.types[ofs >> 5] |= 1 << (ofs & 31);
    _VSTR_F64(r0, JIT_SP, 0);
}

__jit_inline void
vfp_retval_f(jit_state_t _jit, jit_fpr_t r0)
{
    if (jit_hardfp_p()) {
	if (r0 != _D0)
	    _VMOV_F32(r0, _D0);
    }
    else
	_VMOV_S_A(r0, _R0);
}

__jit_inline void
vfp_retval_d(jit_state_t _jit, jit_fpr_t r0)
{
    if (jit_hardfp_p()) {
	if (r0 != _D0)
	    _VMOV_F64(r0, _D0);
    }
    else
	_VMOV_D_AA(r0, _R0, _R1);
}

#undef vfp_unget_tmp
#undef vfp_get_tmp

#endif /* __lightning_fp_vfp_h */
