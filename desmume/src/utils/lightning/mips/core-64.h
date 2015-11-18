/******************************** -*- C -*- ****************************
 *
 *	Platform-independent layer (mips64 version)
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
 * Free Software Foundation, 59 Temple Place - Suite 330, Boston,
 * MA 02111-1307, USA.
 *
 * Authors:
 *	Paulo Cesar Pereira de Andrade
 ***********************************************************************/

#ifndef __lightning_core_h
#define __lightning_core_h

#define JIT_FRAMESIZE			96

#define jit_movr_l(r0, r1)		mips_movr_i(_jit, r0, r1)

#define jit_movi_l(r0, i0)		mips_movi_l(_jit, r0, i0)
__jit_inline void
mips_movi_l(jit_state_t _jit, jit_gpr_t r0, long i0)
{
    unsigned long	ms;

    ms = i0 & 0xffffffff00000000L;
    /* LUI sign extends */
    if ((ms == 0	  && !(i0 & 0x80000000))  ||
	(ms == 0xffffffff &&  (i0 & 0x80000000)))
	jit_movi_i(r0, i0);
    else {
	jit_movi_i(r0, ms >> 32);
	if ((ms = i0 & 0xffff0000)) {
	    _SLL(r0, r0, 16);
	    _ORI(r0, r0, ms >> 16);
	    _SLL(r0, r0, 16);
	}
	else
	    _SLL(r0, r0, 16);
	if ((ms = _jit_US(i0)))
	    _ORI(r0, r0, ms);
    }
}

#define jit_movi_p(r0, i0)		mips_movi_p(_jit, r0, i0)
__jit_inline jit_insn *
mips_movi_p(jit_state_t _jit, jit_gpr_t r0, void *i0)
{
    unsigned long	ms;

    ms = i0 & 0xffff000000000000L;
    _ORI(r0, r0, ms >> 48);
    _SLL(r0, r0, 16);
    ms = i0 & 0x0000ffff00000000L;
    _ORI(r0, r0, ms >> 32);
    _SLL(r0, r0, 16);
    ms = i0 & 0x00000000ffff0000L;
    _ORI(r0, r0, ms >> 16);
    _SLL(r0, r0, 16);
    ms = i0 & 0x000000000000ffffL;
    _ORI(r0, r0, ms);
    return (_jit->x.pc);
}

#define jit_negr_l(r0, r1)		mips_negr_l(_jit, r0, r1)
__jit_inline void
mips_negr_l(jit_state_t _jit, jit_gpr_t r0, jit_gpr_t r1)
{
    _DSUBU(r0, JIT_RZERO, r1);
}

#define jit_addr_l(r0, r1, r2)		mips_addr_l(_jit, r0, r1, r2)
__jit_inline void
mips_addr_l(jit_state_t _jit, jit_gpr_t r0, jit_gpr_t r1, jit_gpr_t r2)
{
    _DADDU(r0, r1, r2);
}

#define jit_addi_l(r0, r1, i0)		mips_addi_l(_jit, r0, r1, i0)
__jit_inline void
mips_addi_l(jit_state_t _jit, jit_gpr_t r0, jit_gpr_t r1, long i0)
{
    if (_s16P(i0))
	_DADDIU(r0, r1, _jit_US(i0));
    else {
	jit_movi_l(JIT_RTEMP, i0);
	jit_addr_l(r0, r1, JIT_RTEMP);
    }
}

#define jit_subr_l(r0, r1, r2)		mips_subr_l(_jit, r0, r1, r2)
__jit_inline void
mips_subr_l(jit_state_t _jit, jit_gpr_t r0, jit_gpr_t r1, jit_gpr_t r2)
{
    _DSUBU(r0, r1, r2);
}

#define jit_subi_l(r0, r1, i0)		mips_subi_l(_jit, r0, r1, i0)
__jit_inline void
mips_subi_l(jit_state_t _jit, jit_gpr_t r0, jit_gpr_t r1, long i0)
{
    if (_s16P(i0) && _jit_US(i0) != 0x8000)
	_DADDIU(r0, r1, _jit_US(-i0));
    else {
	jit_movi_l(JIT_RTEMP, i0);
	jit_subr_l(r0, r1, JIT_RTEMP);
    }
}

#define jit_addci_ul(r0, r1, i0)	mips_addci_ul(_jit, r0, r1, i0)
__jit_inline void
mips_addci_ul(jit_state_t _jit, jit_gpr_t r0, jit_gpr_t r1, unsigned long i0)
{
    if (r0 == r1) {
	if (_s16P(i0))
	    _DADDIU(JIT_RTEMP, r1, _jit_US(i0));
	else {
	    jit_movi_l(JIT_RTEMP, i0);
	    jit_addr_l(JIT_RTEMP, r1, JIT_RTEMP);
	}
	_SLTU(_T8, JIT_RTEMP, r1);
	jit_movr_l(r0, JIT_RTEMP);
    }
    else {
	if (_s16P(i0))
	    _DADDIU(r0, r1, _jit_US(i0));
	else {
	    jit_movi_l(JIT_RTEMP, i0);
	    jit_addr_l(r0, r1, JIT_RTEMP);
	}
	_SLTU(_T8, r0, r1);
    }
}

#define jit_addcr_ul(r0, r1, r2)	mips_addcr_ul(_jit, r0, r1, r2)
__jit_inline void
mips_addcr_ul(jit_state_t _jit, jit_gpr_t r0, jit_gpr_t r1, jit_gpr_t r2)
{
    if (r0 == r1) {
	_DADDU(JIT_RTEMP, r1, r2);
	_SLTU(_T8, JIT_RTEMP, r1);
	jit_movr_l(r0, JIT_RTEMP);
    }
    else {
	_DADDU(r0, r1, r2);
	_SLTU(_T8, r0, r1);
    }
}

#define jit_addxi_ul(r0, r1, i0)	mips_addxi_ul(_jit, r0, r1, i0)
__jit_inline void
mips_addxi_ui(jit_state_t _jit, jit_gpr_t r0, jit_gpr_t r1, unsigned long i0)
{
    jit_movr_l(_T9, _T8);
    jit_addci_ul(r0, r1, i0);
    jit_addcr_ul(r0, r0, _T9);
}

#define jit_addxr_ul(r0, r1, r2)	mips_addxr_ul(_jit, r0, r1, r2)
__jit_inline void
mips_addxr_ul(jit_state_t _jit, jit_gpr_t r0, jit_gpr_t r1, jit_gpr_t r2)
{
    jit_movr_l(_T9, _T8);
    jit_addcr_ul(r0, r1, r2);
    jit_addcr_ul(r0, r0, _T9);
}

#define jit_subci_ul(r0, r1, i0)	mips_subci_ul(_jit, r0, r1, i0)
__jit_inline void
mips_subci_ul(jit_state_t _jit, jit_gpr_t r0, jit_gpr_t r1, unsigned long i0)
{
    if (r0 == r1) {
	if (_s16P(i0) && _jit_US(i0) != 0x8000)
	    _DADDIU(JIT_RTEMP, r1, _jit_US(-i0));
	else {
	    jit_movi_l(JIT_RTEMP, i0);
	    jit_subr_l(JIT_RTEMP, r1, JIT_RTEMP);
	}
	_SLTU(_T8, r1, JIT_RTEMP);
	jit_movr_l(r0, JIT_RTEMP);
    }
    else {
	if (_s16P(i0) && _jit_US(i0) != 0x8000)
	    _DADDIU(r0, r1, _jit_US(-i0));
	else {
	    jit_movi_l(JIT_RTEMP, i0);
	    jit_subr_l(r0, r1, JIT_RTEMP);
	}
	_SLTU(_T8, r1, r0);
    }
}

#define jit_subcr_ul(r0, r1, r2)	mips_subcr_ul(_jit, r0, r1, r2)
__jit_inline void
mips_subcr_ul(jit_state_t _jit, jit_gpr_t r0, jit_gpr_t r1, jit_gpr_t r2)
{
    if (r0 == r1) {
	_DSUBU(JIT_RTEMP, r1, r2);
	_SLTU(_T8, r1, JIT_RTEMP);
	jit_movr_l(r0, JIT_RTEMP);
    }
    else {
	_DSUBU(r0, r1, r2);
	_SLTU(_T8, r1, r0);
    }
}

#define jit_subxi_ul(r0, r1, i0)	mips_subxi_ul(_jit, r0, r1, i0)
__jit_inline void
mips_subxi_ul(jit_state_t _jit, jit_gpr_t r0, jit_gpr_t r1, unsigned long i0)
{
    jit_movr_l(_T9, _T8);
    jit_subci_ul(r0, r1, i0);
    jit_subcr_ul(r0, r0, _T9);
}

#define jit_subxr_ul(r0, r1, r2)	mips_subxr_ul(_jit, r0, r1, r2)
__jit_inline void
mips_subxr_ul(jit_state_t _jit, jit_gpr_t r0, jit_gpr_t r1, jit_gpr_t r2)
{
    jit_movr_l(_T9, _T8);
    jit_subcr_ul(r0, r1, r2);
    jit_subcr_ul(r0, r0, _T9);
}

#define jit_mulr_l(r0, r1, r2)		mips_mulr_l(_jit, r0, r1, r2)
__jit_inline void
mips_mulr_l(jit_state_t _jit, jit_gpr_t r0, jit_gpr_t r1, jit_gpr_t r2)
{
    _DMULTU(r1, r2);
    _MFLO(r0);
}

#define jit_muli_l(r0, r1, i0)		mips_muli_l(_jit, r0, r1, i0)
__jit_inline void
mips_muli_l(jit_state_t _jit, jit_gpr_t r0, jit_gpr_t r1, long i0)
{
    jit_movi_l(JIT_RTEMP, i0);
    jit_mulr_l(r0, r1, JIT_RTEMP);
}

#define jit_hmulr_l(r0, r1, r2)		mips_hmulr_l(_jit, r0, r1, r2)
__jit_inline void
mips_hmulr_l(jit_state_t _jit, jit_gpr_t r0, jit_gpr_t r1, jit_gpr_t r2)
{
    _DMULT(r1, r2);
    _MFHI(r0);
}

#define jit_hmuli_l(r0, r1, i0)		mips_hmuli_l(_jit, r0, r1, i0)
__jit_inline void
mips_hmuli_l(jit_state_t _jit, jit_gpr_t r0, jit_gpr_t r1, long i0)
{
    jit_movi_l(JIT_RTEMP, i0);
    jit_hmulr_l(r0, r1, JIT_RTEMP);
}

#define jit_hmulr_ul(r0, r1, r2)	mips_hmulr_ul(_jit, r0, r1, r2)
__jit_inline void
mips_hmulr_ul(jit_state_t _jit, jit_gpr_t r0, jit_gpr_t r1, jit_gpr_t r2)
{
    _DMULTU(r1, r2);
    _MFHI(r0);
}

#define jit_hmuli_ul(r0, r1, i0)	mips_hmuli_ul(_jit, r0, r1, i0)
__jit_inline void
mips_hmuli_ul(jit_state_t _jit, jit_gpr_t r0, jit_gpr_t r1, unsigned long i0)
{
    jit_movi_l(JIT_RTEMP, i0);
    jit_hmulr_ul(r0, r1, JIT_RTEMP);
}

#define jit_divr_l(r0, r1, r2)		mips_divr_l(_jit, r0, r1, r2)
__jit_inline void
mips_divr_l(jit_state_t _jit, jit_gpr_t r0, jit_gpr_t r1, jit_gpr_t r2)
{
    _DDDIV(r1, r2);
    _MFLO(r0);
}

#define jit_divi_l(r0, r1, i0)		mips_divi_l(_jit, r0, r1, i0)
__jit_inline void
mips_divi_l(jit_state_t _jit, jit_gpr_t r0, jit_gpr_t r1, long i0)
{
    jit_movi_l(JIT_RTEMP, i0);
    jit_divr_l(r0, r1, JIT_RTEMP);
}

#define jit_divr_ul(r0, r1, r2)		mips_divr_ul(_jit, r0, r1, r2)
__jit_inline void
mips_divr_ul(jit_state_t _jit, jit_gpr_t r0, jit_gpr_t r1, jit_gpr_t r2)
{
    _DDDIVU(r1, r2);
    _MFLO(r0);
}

#define jit_divi_ul(r0, r1, i0)		mips_divi_ul(_jit, r0, r1, i0)
__jit_inline void
mips_divi_ul(jit_state_t _jit, jit_gpr_t r0, jit_gpr_t r1, unsigned long i0)
{
    jit_movi_l(JIT_RTEMP, i0);
    jit_divr_ul(r0, r1, JIT_RTEMP);
}

#define jit_modr_l(r0, r1, r2)		mips_modr_l(_jit, r0, r1, r2)
__jit_inline void
mips_modr_l(jit_state_t _jit, jit_gpr_t r0, jit_gpr_t r1, jit_gpr_t r2)
{
    _DDDIV(r1, r2);
    _MFHI(r0);
}

#define jit_modi_l(r0, r1, i0)		mips_modi_l(_jit, r0, r1, i0)
__jit_inline void
mips_modi_l(jit_state_t _jit, jit_gpr_t r0, jit_gpr_t r1, long i0)
{
    jit_movi_l(JIT_RTEMP, i0);
    jit_modr_l(r0, r1, JIT_RTEMP);
}

#define jit_modr_ul(r0, r1, r2)		mips_modr_ul(_jit, r0, r1, r2)
__jit_inline void
mips_modr_ul(jit_state_t _jit, jit_gpr_t r0, jit_gpr_t r1, jit_gpr_t r2)
{
    _DDDIVU(r1, r2);
    _MFHI(r0);
}

#define jit_modi_ul(r0, r1, i0)		mips_modi_ul(_jit, r0, r1, i0)
__jit_inline void
mips_modi_ul(jit_state_t _jit, jit_gpr_t r0, jit_gpr_t r1, unsigned long i0)
{
    jit_movi_l(JIT_RTEMP, i0);
    jit_modr_ul(r0, r1, JIT_RTEMP);
}

#define jit_andr_l(r0, r1, r2)		jit_andr_i(r0, r1, r2)
#define jit_andi_l(r0, r1, i0)		mips_andi_l(_jit, r0, r1, i0)
__jit_inline void
mips_andi_l(jit_state_t _jit, jit_gpr_t r0, jit_gpr_t r1, long i0)
{
    if (_u16P(i0))
	_ANDI(r0, r1, i0);
    else {
	jit_movi_l(JIT_RTEMP, i0);
	jit_andr_l(r0, r1, JIT_RTEMP);
    }
}

#define jit_orr_l(r0, r1, r2)		jit_orr_i(r0, r1, r2)
#define jit_ori_l(r0, r1, i0)		mips_ori_i(_jit, r0, r1, i0)
__jit_inline void
mips_ori_l(jit_state_t _jit, jit_gpr_t r0, jit_gpr_t r1, long i0)
{
    if (_u16P(i0))
	_ORI(r0, r1, i0);
    else {
	jit_movi_l(JIT_RTEMP, i0);
	jit_orr_l(r0, r1, JIT_RTEMP);
    }
}

#define jit_xorr_l(r0, r1, r2)		jit_xorr_i(r0, r1, r2)
#define jit_xori_l(r0, r1, i0)		mips_xori_l(_jit, r0, r1, i0)
__jit_inline void
mips_xori_l(jit_state_t _jit, jit_gpr_t r0, jit_gpr_t r1, long i0)
{
    if (_u16P(i0))
	_XORI(r0, r1, i0);
    else {
	jit_movi_l(JIT_RTEMP, i0);
	jit_xorr_l(r0, r1, JIT_RTEMP);
    }
}

#define jit_lshr_l(r0, r1, r2)		mips_lshr_l(_jit, r0, r1, r2)
__jit_inline void
mips_lshr_l(jit_state_t _jit, jit_gpr_t r0, jit_gpr_t r1, jit_gpr_t r2)
{
    _DSLLV(r0, r1, r2);
}

#define jit_lshi_l(r0, r1, i0)		mips_lshi_l(_jit, r0, r1, i0)
__jit_inline void
mips_lshi_l(jit_state_t _jit, jit_gpr_t r0, jit_gpr_t r1, unsigned char i0)
{
    assert(i0 <= 63);
    if (i0 < 32)
	_DSLL(r0, r1, i0);
    else
	_DSLL32(r0, r1, i0);
}

#define jit_rshr_l(r0, r1, r2)		mips_rshr_l(_jit, r0, r1, r2)
__jit_inline void
mips_rshr_l(jit_state_t _jit, jit_gpr_t r0, jit_gpr_t r1, jit_gpr_t r2)
{
    _DSRAV(r0, r1, r2);
}

#define jit_rshi_l(r0, r1, i0)		mips_rshi_l(_jit, r0, r1, i0)
__jit_inline void
mips_rshi_l(jit_state_t _jit, jit_gpr_t r0, jit_gpr_t r1, unsigned char i0)
{
    assert(i0 <= 63);
    if (i0 < 32)
	_DSRA(r0, r1, i0);
    else
	_DSRA32(r0, r1, i0);
}

#define jit_rshr_ul(r0, r1, r2)		mips_rshr_ul(_jit, r0, r1, r2)
__jit_inline void
mips_rshr_ul(jit_state_t _jit, jit_gpr_t r0, jit_gpr_t r1, jit_gpr_t r2)
{
    _DSRLV(r0, r1, r2);
}

#define jit_rshi_ul(r0, r1, i0)		mips_rshi_ul(_jit, r0, r1, i0)
__jit_inline void
mips_rshi_ul(jit_state_t _jit, jit_gpr_t r0, jit_gpr_t r1, unsigned char i0)
{
    assert(i0 <= 63);
    if (i0 < 32)
	_DSRL(r0, r1, i0);
    else
	_DSRL32(r0, r1, i0);
}

#define jit_ltr_l(r0, r1, r2)		mips_ltr_i(_jit, r0, r1, r2)
#define jit_lti_l(r0, r1, i0)		mips_lti_l(_jit, r0, r1, i0)
__jit_inline void
mips_lti_l(jit_state_t _jit, jit_gpr_t r0, jit_gpr_t r1, long i0)
{
    if (_s16P(i0))
	_SLTI(r0, r1, _jit_US(i0));
    else {
	jit_movi_l(JIT_RTEMP, i0);
	jit_ltr_l(r0, r1, JIT_RTEMP);
    }
}

#define jit_ltr_ul(r0, r1, r2)		mips_ltr_ui(_jit, r0, r1, r2)
#define jit_lti_ul(r0, r1, i0)		mips_lti_ul(_jit, r0, r1, i0)
__jit_inline void
mips_lti_ul(jit_state_t _jit, jit_gpr_t r0, jit_gpr_t r1, unsigned long i0)
{
    if (_s16P(i0))
	_SLTIU(r0, r1, _jit_US(i0));
    else {
	jit_movi_l(JIT_RTEMP, i0);
	jit_ltr_ul(r0, r1, JIT_RTEMP);
    }
}

#define jit_ler_l(r0, r1, r2)		mips_ler_i(_jit, r0, r1, r2)
#define jit_lei_l(r0, r1, i0)		mips_lei_l(_jit, r0, r1, i0)
__jit_inline void
mips_lei_l(jit_state_t _jit, jit_gpr_t r0, jit_gpr_t r1, long i0)
{
    if (i0 == 0) {
	_SLT(r0, JIT_RZERO, r1);
	_XORI(r0, r0, 1);
    }
    else {
	jit_movi_l(JIT_RTEMP, i0);
	jit_ler_l(r0, r1, JIT_RTEMP);
    }
}

#define jit_ler_ul(r0, r1, r2)		mips_ler_ui(_jit, r0, r1, r2)
#define jit_lei_ul(r0, r1, i0)		mips_lei_ul(_jit, r0, r1, i0)
__jit_inline void
mips_lei_ul(jit_state_t _jit, jit_gpr_t r0, jit_gpr_t r1, unsigned long i0)
{
    if (i0 == 0) {
	_SLTU(r0, JIT_RZERO, r1);
	_XORI(r0, r0, 1);
    }
    else {
	jit_movi_l(JIT_RTEMP, i0);
	jit_ler_ul(r0, r1, JIT_RTEMP);
    }
}

#define jit_eqr_l(r0, r1, r2)		mips_eqr_l(_jit, r0, r1, r2)
__jit_inline void
mips_eqr_l(jit_state_t _jit, jit_gpr_t r0, jit_gpr_t r1, jit_gpr_t r2)
{
    jit_subr_l(r0, r1, r2);
    _SLTU(r0, JIT_RZERO, r0);
    _XORI(r0, r0, 1);
}

#define jit_eqi_l(r0, r1, i0)		mips_eqi_l(_jit, r0, r1, i0)
__jit_inline void
mips_eqi_l(jit_state_t _jit, jit_gpr_t r0, jit_gpr_t r1, long i0)
{
    if (i0) {
	jit_subi_l(r0, r1, i0);
	_SLTU(r0, JIT_RZERO, r0);
    }
    else
	_SLTU(r0, JIT_RZERO, r1);
    _XORI(r0, r0, 1);
}

#define jit_ger_l(r0, r1, r2)		mips_ger_i(_jit, r0, r1, r2)
#define jit_gei_l(r0, r1, i0)		mips_gei_l(_jit, r0, r1, i0)
__jit_inline void
mips_gei_l(jit_state_t _jit, jit_gpr_t r0, jit_gpr_t r1, long i0)
{
    jit_movi_l(JIT_RTEMP, i0);
    jit_ger_l(r0, r1, JIT_RTEMP);
}

#define jit_ger_ul(r0, r1, i0)		mips_ger_ui(_jit, r0, r1, i0)
#define jit_gei_ul(r0, r1, i0)		mips_gei_ul(_jit, r0, r1, i0)
__jit_inline void
mips_gei_ul(jit_state_t _jit, jit_gpr_t r0, jit_gpr_t r1, unsigned long i0)
{
    jit_movi_l(JIT_RTEMP, i0);
    jit_ger_ul(r0, r1, JIT_RTEMP);
}

#define jit_gtr_l(r0, r1, r2)		mips_gtr_i(_jit, r0, r1, r2)
#define jit_gti_l(r0, r1, i0)		mips_gti_l(_jit, r0, r1, i0)
__jit_inline void
mips_gti_l(jit_state_t _jit, jit_gpr_t r0, jit_gpr_t r1, long i0)
{
    if (i0 == 0)
	_SLT(r0, JIT_RZERO, r1);
    else {
	jit_movi_l(JIT_RTEMP, i0);
	_SLT(r0, JIT_RTEMP, r1);
    }
}

#define jit_gtr_ul(r0, r1, r2)		mips_gtr_ui(_jit, r0, r1, r2)
#define jit_gti_ul(r0, r1, i0)		mips_gti_ul(_jit, r0, r1, i0)
__jit_inline void
mips_gti_ul(jit_state_t _jit, jit_gpr_t r0, jit_gpr_t r1, unsigned long i0)
{
    if (i0 == 0)
	_SLTU(r0, JIT_RZERO, r1);
    else {
	jit_movi_l(JIT_RTEMP, i0);
	_SLTU(r0, JIT_RTEMP, r1);
    }
}

#define jit_ner_l(r0, r1, r2)		mips_ner_l(_jit, r0, r1, r2)
__jit_inline void
mips_ner_l(jit_state_t _jit, jit_gpr_t r0, jit_gpr_t r1, jit_gpr_t r2)
{
    jit_subr_l(r0, r1, r2);
    _SLTU(r0, JIT_RZERO, r0);
}

#define jit_nei_l(r0, r1, i0)		mips_nei_l(_jit, r0, r1, i0)
__jit_inline void
mips_nei_l(jit_state_t _jit, jit_gpr_t r0, jit_gpr_t r1, long i0)
{
    if (i0)
	jit_subi_l(r0, r1, i0);
    _SLTU(r0, JIT_RZERO, r0);
}

#define jit_bltr_l(i0, r0, r1)		mips_bltr_i(_jit, i0, r0, r1)
#define jit_blti_l(i0, r0, i1)		mips_blti_l(_jit, i0, r0, i1)
__jit_inline jit_insn *
mips_blti_l(jit_state_t _jit, jit_insn *i0, jit_gpr_t r0, long i1)
{
    if (_s16P(i1))
	return (jit_blti_i(i0, r0, i1));
    jit_movi_l(JIT_RTEMP, i1);
    return (jit_bltr_l(i0, r0, JIT_RTEMP));
}

#define jit_bltr_ul(i0, r0, r1)		mips_bltr_ui(_jit, i0, r0, r1)
+#define jit_blti_ul(i0, r0, i1)		mips_blti_ul(_jit, i0, r0, i1)
__jit_inline jit_insn *
mips_blti_ul(jit_state_t _jit, jit_insn *i0, jit_gpr_t r0, unsigned long i1)
{
    if (_s16P(i1))
	return (jit_blti_ui(i0, r0, i1));
    jit_movi_l(JIT_RTEMP, i1);
    return (jit_bltr_ul(i0, r0, JIT_RTEMP));
}

#define jit_bler_l(i0, r0, r1)		mips_bler_i(_jit, i0, r0, r1)
#define jit_blei_l(i0, r0, i1)		mips_blei_l(_jit, i0, r0, i1)
__jit_inline jit_insn *
mips_blei_l(jit_state_t _jit, jit_insn *i0, jit_gpr_t r0, long i1)
{
    if (_s16P(i1))
	return (jit_blei_i(i0, r0, i1));
    jit_movi_l(JIT_RTEMP, i1);
    return (jit_bler_l(i0, r0, JIT_RTEMP));
}

#define jit_bler_ul(i0, r0, r1)		mips_bler_ui(_jit, i0, r0, r1)
#define jit_blei_ul(i0, r0, i1)		mips_blei_ul(_jit, i0, r0, i1)
__jit_inline jit_insn *
mips_blei_ul(jit_state_t _jit, jit_insn *i0, jit_gpr_t r0, unsigned long i1)
{
    if (i1 == 0)
	return (jit_blei_i(i0, r0, i1));
    jit_movi_l(JIT_RTEMP, i1);
    return (jit_bler_ul(i0, r0, JIT_RTEMP));
}

#define jit_beqr_l(i0, r0, r1)		mips_beqr_i(_jit, i0, r0, r1)
#define jit_beqi_l(i0, r0, i1)		mips_beqi_l(_jit, i0, r0, i1)
__jit_inline jit_insn *
mips_beqi_l(jit_state_t _jit, jit_insn *i0, jit_gpr_t r0, long i1)
{
    if (i1 == 0)
	return (jit_beqi_i(i0, r0, i1));
    jit_movi_l(JIT_RTEMP, i1);
    return (jit_beqr_l(i0, r0, JIT_RTEMP));
}

#define jit_bger_l(i0, r0, r1)		mips_bger_i(_jit, i0, r0, r1)
#define jit_bgei_l(i0, r0, i1)		mips_bgei_l(_jit, i0, r0, i1)
__jit_inline jit_insn *
mips_bgei_l(jit_state_t _jit, jit_insn *i0, jit_gpr_t r0, long i1)
{
    if (_s16P(i1))
	return (jit_bgei_i(i0, r0, i1));
    jit_movi_l(JIT_RTEMP, i1);
    return (jit_bger_l(i0, r0, JIT_RTEMP));
}

#define jit_bger_ul(i0, r0, r1)		mips_bger_ui(_jit, i0, r0, r1)
#define jit_bgei_ul(i0, r0, i1)		mips_bgei_ul(_jit, i0, r0, i1)
__jit_inline jit_insn *
mips_bgei_ul(jit_state_t _jit, jit_insn *i0, jit_gpr_t r0, unsigned long i1)
{
    if (_s16P(i1))
	return (jit_bgei_ui(i0, r0, i1));
    jit_movi_l(JIT_RTEMP, i1);
    return (jit_bger_ul(i0, r0, JIT_RTEMP));
}

#define jit_bgtr_l(i0, r0, r1)		mips_bgtr_i(_jit, i0, r0, r1)
#define jit_bgti_l(i0, r0, i1)		mips_bgti_l(_jit, i0, r0, i1)
__jit_inline jit_insn *
mips_bgti_l(jit_state_t _jit, jit_insn *i0, jit_gpr_t r0, long i1)
{
    if (i1 == 0)
	return (jit_bgti_i(i0, r0, i1));
    jit_movi_l(JIT_RTEMP, i1);
    return (jit_bgtr_l(i0, r0, JIT_RTEMP));
}

#define jit_bgtr_ul(i0, r0, r1)		mips_bgtr_ui(_jit, i0, r0, r1)
#define jit_bgti_ul(i0, r0, i1)		mips_bgti_ul(_jit, i0, r0, i1)
__jit_inline jit_insn *
mips_bgti_ul(jit_state_t _jit, jit_insn *i0, jit_gpr_t r0, unsigned long i1)
{
    if (i1 == 0)
	return (jit_bgti_ui(i0, r0, i1));
    jit_movi_l(JIT_RTEMP, i1);
    return (jit_bgtr_ul(i0, r0, JIT_RTEMP));
}

#define jit_bner_l(i0, r0, r1)		mips_bner_i(_jit, i0, r0, r1)
#define jit_bnei_l(i0, r0, i1)		mips_bnei_l(_jit, i0, r0, i1)
__jit_inline jit_insn *
mips_bnei_l(jit_state_t _jit, jit_insn *i0, jit_gpr_t r0, long i1)
{
    if (i1 == 0)
	return (jit_bnei_i(i0, r0, i1));
    jit_movi_l(JIT_RTEMP, i1);
    return (jit_bner_l(i0, r0, JIT_RTEMP));
}

#define jit_boaddr_l(i0, r0, r1)	mips_boaddr_l(_jit, i0, r0, r1)
__jit_inline jit_insn *
mips_boaddr_l(jit_state_t _jit, jit_insn *i0, jit_gpr_t r0, jit_gpr_t r1)
{
    jit_insn	*l;
    long	 d;
    _SLT(_T8, r1, _ZERO);
    _DADDU(_AT, r0, r1);
    _SLT(_T9, _AT, r0);
    _SLT(_AT, r0, _AT);
    _MOVZ(_AT, _T9, _T8);
    l = _jit->x.pc;
    d = (((long)i0 - (long)l) >> 2) - 1;
    assert(_s16P(d));
    _BNE(_AT, _ZERO, _jit_US(d));
    _DADDU(r0, r0, r1);
    return (l);
}

#define jit_boaddr_ul(i0, r0, r1)	mips_boaddr_ul(_jit, i0, r0, r1)
__jit_inline jit_insn *
mips_boaddr_ul(jit_state_t _jit, jit_insn *i0, jit_gpr_t r0, jit_gpr_t r1)
{
    jit_insn	*l;
    long	 d;
    _DADDU(JIT_RTEMP, r0, r1);
    _SLTU(_T8, JIT_RTEMP, r0);
    l = _jit->x.pc;
    d = (((long)i0 - (long)l) >> 2) - 1;
    assert(_s16P(d));
    _BNE(JIT_RZERO, _T8, _jit_US(d));
    jit_movr_l(r0, JIT_RTEMP);
    return (l);
}

#define jit_boaddi_l(i0, r0, i1)	mips_boaddi_l(_jit, i0, r0, i1)
__jit_inline jit_insn *
mips_boaddi_l(jit_state_t _jit, jit_insn *i0, jit_gpr_t r0, long i1)
{
    jit_insn	*l;
    long	 d;
    if (_s16P(i1)) {
	_SLTI(_T8, _ZERO, _jit_US(i1));
	_DADDIU(_AT, r0, _jit_US(i1));
	_SLT(_T9, r0, _AT);
	_SLT(_AT, _AT, r0);
	_MOVZ(_AT, _T9, _T8);
	l = _jit->x.pc;
	d = (((long)i0 - (long)l) >> 2) - 1;
	assert(_s16P(d));
	_BNE(_AT, _ZERO, _jit_US(d));
	_DADDIU(r0, r0, _jit_US(i1));
	return (l);
    }
    jit_movi_l(_T7, i1);
    return (jit_boaddr_l(i0, r0, _T7));
}

#define jit_boaddi_ul(i0, r0, i1)	mips_boaddi_ul(_jit, i0, r0, i1)
__jit_inline jit_insn *
mips_boaddi_ul(jit_state_t _jit, jit_insn *i0, jit_gpr_t r0, unsigned long i1)
{
    jit_insn	*l;
    long	 d;
    if (_s16P(i1)) {
	_DADDIU(JIT_RTEMP, r0, _jit_US(i1));
	_SLTU(_T8, JIT_RTEMP, r0);
	l = _jit->x.pc;
	d = (((long)i0 - (long)l) >> 2) - 1;
	assert(_s16P(d));
	_BNE(JIT_RZERO, _T8, _jit_US(d));
	jit_movr_l(r0, JIT_RTEMP);
	return (l);
    }
    jit_movi_l(JIT_RTEMP, i1);
    return (jit_boaddr_ul(i0, r0, JIT_RTEMP));
}

#define jit_bosubr_l(i0, r0, r1)	mips_bosubr_l(_jit, i0, r0, r1)
__jit_inline jit_insn *
mips_bosubr_l(jit_state_t _jit, jit_insn *i0, jit_gpr_t r0, jit_gpr_t r1)
{
    jit_insn	*l;
    long	 d;
    _SLT(_T8, _ZERO, r1);
    _DSUBU(_AT, r0, r1);
    _SLT(_T9, _AT, r0);
    _SLT(_AT, r0, _AT);
    _MOVZ(_AT, _T9, _T8);
    l = _jit->x.pc;
    d = (((long)i0 - (long)l) >> 2) - 1;
    assert(_s16P(d));
    _BNE(_AT, _ZERO, _jit_US(d));
    _DSUBU(r0, r0, r1);
    return (l);
}

#define jit_bosubr_ul(i0, r0, r1)	mips_bosubr_ul(_jit, i0, r0, r1)
__jit_inline jit_insn *
mips_bosubr_ul(jit_state_t _jit, jit_insn *i0, jit_gpr_t r0, jit_gpr_t r1)
{
    jit_insn	*l;
    long	 d;
    _DSUBU(JIT_RTEMP, r0, r1);
    _SLTU(_T8, r0, JIT_RTEMP);
    l = _jit->x.pc;
    d = (((long)i0 - (long)l) >> 2) - 1;
    assert(_s16P(d));
    _BNE(JIT_RZERO, _T8, _jit_US(d));
    jit_movr_l(r0, JIT_RTEMP);
    return (l);
}

#define jit_bosubi_l(i0, r0, i1)	mips_bosubi_l(_jit, i0, r0, i1)
__jit_inline jit_insn *
mips_bosubi_l(jit_state_t _jit, jit_insn *i0, jit_gpr_t r0, long i1)
{
    jit_insn	*l;
    long	 d;
    if (_s16P(i1) && _jit_US(i1) != 0x8000) {
	_SLTI(_T8, _ZERO, _jit_US(i1));
	_DADDIU(_AT, r0, _jit_US(-i1));
	_SLT(_T9, _AT, r0);
	_SLT(_AT, r0, _AT);
	_MOVZ(_AT, _T9, _T8);
	l = _jit->x.pc;
	d = (((long)i0 - (long)l) >> 2) - 1;
	assert(_s16P(d));
	_BNE(_AT, _ZERO, _jit_US(d));
	_DADDIU(r0, r0, _jit_US(-i1));
	return (l);
    }
    jit_movi_l(_T7, i1);
    return (jit_bosubr_l(i0, r0, _T7));
}

#define jit_bosubi_ul(i0, r0, i1)	mips_bosubi_ul(_jit, i0, r0, i1)
__jit_inline jit_insn *
mips_bosubi_ul(jit_state_t _jit, jit_insn *i0, jit_gpr_t r0, unsigned long i1)
{
    jit_insn	*l;
    long	 d;
    if (_s16P(i1) && _jit_US(i1) != 0x8000) {
	_DADDIU(JIT_RTEMP, r0, _jit_US(-i1));
	_SLTU(_T8, r0, JIT_RTEMP);
	l = _jit->x.pc;
	d = (((long)i0 - (long)l) >> 2) - 1;
	assert(_s16P(d));
	_BNE(JIT_RZERO, _T8, _jit_US(d));
	jit_movr_l(r0, JIT_RTEMP);
	return (l);
    }
    jit_movi_l(JIT_RTEMP, i1);
    return (jit_bosubr_ul(i0, r0, JIT_RTEMP));
}

#define jit_bmsr_l(i0, r0, r1)		mips_bmsr_i(_jit, i0, r0, r1)
#define jit_bmsi_l(i0, r0, i1)		mips_bmsi_l(_jit, i0, r0, i1)
__jit_inline jit_insn *
mips_bmsi_l(jit_state_t _jit, jit_insn *i0, jit_gpr_t r0, long i1)
{
    if (_u16P(i1))
	return (jit_bmsi_i(i0, r0, i1));
    jit_movi_l(JIT_RTEMP, i1);
    return (jit_bmsr_l(i0, r0, JIT_RTEMP));
}

#define jit_bmcr_l(i0, r0, r1)		mips_bmcr_i(_jit, i0, r0, r1)
#define jit_bmci_l(i0, r0, i1)		mips_bmci_l(_jit, i0, r0, i1)
__jit_inline jit_insn *
mips_bmci_l(jit_state_t _jit, jit_insn *i0, jit_gpr_t r0, long i1)
{
    if (_u16P(i1))
	return (jit_bmci_i(i0, r0, i1));
    jit_movi_l(JIT_RTEMP, i1);
    return (jit_bmcr_l(i0, r0, JIT_RTEMP));
}

#define jit_ldr_ui(r0, r1)		mips_ldr_ui(_jit, r0, r1)
__jit_inline void
mips_ldr_ui(jit_state_t _jit, jit_gpr_t r0, jit_gpr_t r1)
{
    _LWU(r0, 0, r1);
}

#define jit_ldr_l(r0, r1)		mips_ldr_l(_jit, r0, r1)
__jit_inline void
mips_ldr_l(jit_state_t _jit, jit_gpr_t r0, jit_gpr_t r1)
{
    _LD(r0, 0, r1);
}

#define jit_ldi_ui(r0, i0)		mips_ldi_ui(_jit, r0, i0)
__jit_inline void
mips_ldi_ui(jit_state_t _jit, jit_gpr_t r0, void *i0)
{
    long	ds = (long)i0;

    if (_s16P(ds))
	_LWU(r0, _jit_US(ds), JIT_RZERO);
    else {
	jit_movi_i(JIT_RTEMP, ds);
	_LWU(r0, 0, JIT_RTEMP);
    }
}

#define jit_ldi_l(r0, i0)		mips_ldi_l(_jit, r0, i0)
__jit_inline void
mips_ldi_l(jit_state_t _jit, jit_gpr_t r0, void *i0)
{
    long	ds = (long)i0;

    if (_s16P(ds))
	_LD(r0, _jit_US(ds), JIT_RZERO);
    else {
	jit_movi_i(JIT_RTEMP, ds);
	_LD(r0, 0, JIT_RTEMP);
    }
}

#define jit_ldxr_ui(r0, r1, r2)		mips_ldxr_ui(_jit, r0, r1, r2)
__jit_inline void
mips_ldxr_ui(jit_state_t _jit, jit_gpr_t r0, jit_gpr_t r1, jit_gpr_t r2)
{
    jit_addr_i(JIT_RTEMP, r1, r2);
    _LWU(r0, 0, JIT_RTEMP);
}

#define jit_ldxr_l(r0, r1, r2)		mips_ldxr_l(_jit, r0, r1, r2)
__jit_inline void
mips_ldxr_l(jit_state_t _jit, jit_gpr_t r0, jit_gpr_t r1, jit_gpr_t r2)
{
    jit_addr_i(JIT_RTEMP, r1, r2);
    _LD(r0, 0, JIT_RTEMP);
}

#define jit_ldxi_ui(r0, r1, i0)		mips_ldxi_ui(_jit, r0, r1, i0)
__jit_inline void
mips_ldxi_ui(jit_state_t _jit, jit_gpr_t r0, jit_gpr_t r1, long i0)
{
    if (_s16P(i0))
	_LWU(r0, _jit_US(i0), r1);
    else {
	jit_addi_i(JIT_RTEMP, r1, i0);
	_LWU(r0, 0, JIT_RTEMP);
    }
}

#define jit_ldxi_l(r0, r1, i0)		mips_ldxi_l(_jit, r0, r1, i0)
__jit_inline void
mips_ldxi_l(jit_state_t _jit, jit_gpr_t r0, jit_gpr_t r1, long i0)
{
    if (_s16P(i0))
	_LD(r0, _jit_US(i0), r1);
    else {
	jit_addi_i(JIT_RTEMP, r1, i0);
	_LD(r0, 0, JIT_RTEMP);
    }
}

#define jit_str_l(r0, r1)		mips_str_l(_jit, r0, r1)
__jit_inline void
mips_str_l(jit_state_t _jit, jit_gpr_t r0, jit_gpr_t r1)
{
    _SD(r1, 0, r0);
}

#define jit_sti_l(i0, r0)		mips_sti_l(_jit, i0, r0)
__jit_inline void
mips_sti_l(jit_state_t _jit, void *i0, jit_gpr_t r0)
{
    long	ds = (long)i0;

    if (_s16P(ds))
	_SD(r0, _jit_US(ds), JIT_RZERO);
    else {
	jit_movi_i(JIT_RTEMP, ds);
	_SD(r0, 0, JIT_RTEMP);
    }
}

#define jit_stxr_l(r0, r1, r2)		mips_stxr_l(_jit, r0, r1, r2)
__jit_inline void
mips_stxr_l(jit_state_t _jit, jit_gpr_t r0, jit_gpr_t r1, jit_gpr_t r2)
{
    jit_addr_i(JIT_RTEMP, r0, r1);
    _SD(r2, 0, JIT_RTEMP);
}

#define jit_stxi_l(i0, r0, r1)		mips_stxi_l(_jit, i0, r0, r1)
__jit_inline void
mips_stxi_l(jit_state_t _jit, int i0, jit_gpr_t r0, jit_gpr_t r1)
{
    if (_s16P(i0))
	_SD(r1, _jit_US(i0), r0);
    else {
	jit_addi_i(JIT_RTEMP, r0, i0);
	_SD(r1, 0, JIT_RTEMP);
    }
}

#define jit_extr_c_l(r0, r1)		mips_extr_c_l(_jit, r0, r1)
__jit_inline void
mips_extr_c_l(jit_state_t _jit, jit_gpr_t r0, jit_gpr_t r1)
{
    jit_extr_c_i(r0, r1);
    _SLL(r0, r0, 0);
}

#define jit_extr_c_ul(r0, r1)		mips_extr_c_ul(_jit, r0, r1)
__jit_inline void
mips_extr_c_ul(jit_state_t _jit, jit_gpr_t r0, jit_gpr_t r1)
{
    jit_extr_c_ui(r0, r1);
    _SLL(r0, r0, 0);
}

#define jit_extr_s_l(r0, r1)		mips_extr_s_l(_jit, r0, r1)
__jit_inline void
mips_extr_s_l(jit_state_t _jit, jit_gpr_t r0, jit_gpr_t r1)
{
    jit_extr_s_i(r0, r1);
    _SLL(r0, r0, 0);
}

#define jit_extr_s_ul(r0, r1)		mips_extr_s_ul(_jit, r0, r1)
#define jit_extr_us_ul(r0, r1)		mips_extr_us_ul(_jit, r0, r1)
__jit_inline void
mips_extr_s_ul(jit_state_t _jit, jit_gpr_t r0, jit_gpr_t r1)
{
    jit_extr_s_ui(r0, r1);
    _SLL(r0, r0, 0);
}

#define jit_extr_i_l(r0, r1)		mips_extr_i_l(_jit, r0, r1)
__jit_inline void
mips_extr_i_l(jit_state_t _jit, jit_gpr_t r0, jit_gpr_t r1)
{
    _SLL(r0, r0, 0);
}

#define jit_extr_i_ul(r0, r1)		mips_extr_i_ul(_jit, r0, r1)
#define jit_extr_ui_ul(r0, r1)		mips_extr_ui_ul(_jit, r0, r1)
__jit_inline void
mips_extr_i_ul(jit_state_t _jit, jit_gpr_t r0, jit_gpr_t r1)
{
    jit_movr_l(r0, r1);
    _DINS(r0, JIT_RZERO, 32, 32);
}

#define jit_prolog(n)			mips_prolog(_jit, n)
__jit_inline void
mips_prolog(jit_state_t _jit, int n)
{
    _jitl.framesize = JIT_FRAMESIZE;
#ifdef JIT_NEED_PUSH_POP
    _jitl.pop = 0;
#endif

    jit_subi_l(JIT_SP, JIT_SP, JIT_FRAMESIZE);
    jit_stxi_l(88, JIT_SP, _RA);
    jit_stxi_l(80, JIT_SP, _FP);
    jit_stxi_l(72, JIT_SP, _S7);
    jit_stxi_l(64, JIT_SP, _S6);
    jit_stxi_l(56, JIT_SP, _S5);
    jit_stxi_l(48, JIT_SP, _S4);
    jit_stxi_l(40, JIT_SP, _S3);
    jit_stxi_l(32, JIT_SP, _S2);
    jit_stxi_l(24, JIT_SP, _S1);
    jit_stxi_l(16, JIT_SP, _S0);
    _SDC1(_F30, 8, JIT_SP);
    _SDC1(_F28, 0, JIT_SP);
    jit_movr_l(JIT_FP, JIT_SP);

    /* patch alloca and stack adjustment */
    _jitl.stack = (int *)_jit->x.pc;
    jit_movi_p(JIT_RTEMP, 0);
    jit_subr_l(JIT_SP, JIT_SP, JIT_RTEMP);
    _jitl.alloca_offset = _jitl.stack_offset = _jitl.stack_length = 0;
}

#define jit_prepare_i(count)		mips_prepare_i(_jit, count)
__jit_inline void
mips_prepare_i(jit_state_t _jit, int count)
{
    assert(count		>= 0 &&
	   _jitl.stack_offset	== 0 &&
	   _jitl.nextarg_put	== 0);

    _jitl.nextarg_put = count;
    if (_jitl.nextarg_put > JIT_A_NUM) {
	_jitl.stack_offset = (_jitl.nextarg_put - JIT_A_NUM) << 3;
	if (_jitl.stack_length < _jitl.stack_offset) {
	    _jitl.stack_length = _jitl.stack_offset;
	    mips_set_stack(_jit, (_jitl.alloca_offset +
				  _jitl.stack_length + 7) & ~7);
	}
    }
}

#define jit_pusharg_i(r0)		mips_pusharg_l(_jit, r0)
#define jit_pusharg_l(r0)		mips_pusharg_l(_jit, r0)
__jit_inline void
mips_pusharg_l(jit_state_t _jit, jit_gpr_t r0)
{
    assert(_jitl.nextarg_put > 0);
    if (--_jitl.nextarg_put >= JIT_A_NUM) {
	_jitl.stack_offset -= sizeof(long);
	assert(_jitl.stack_offset >= 0);
	jit_stxi_l(_jitl.stack_offset, JIT_SP, r0);
    }
    else
	jit_movr_l(jit_a_order[_jitl.nextarg_put], r0);
}

#define jit_arg_l()			mips_arg_l(_jit)
#define jit_arg_i()			mips_arg_l(_jit)
#define jit_arg_c()			mips_arg_l(_jit)
#define jit_arg_uc()			mips_arg_l(_jit)
#define jit_arg_s()			mips_arg_l(_jit)
#define jit_arg_us()			mips_arg_l(_jit)
#define jit_arg_ui()			mips_arg_l(_jit)
#define jit_arg_ul()			mips_arg_l(_jit)
__jit_inline int
mips_arg_l(jit_state_t _jit)
{
    int		ofs;
    int		reg;

    reg = (_jitl.framesize - JIT_FRAMESIZE) >> 3;
    if (reg < JIT_A_NUM)
	ofs = reg;
    else
	ofs = _jitl.framesize;
    _jitl.framesize += sizeof(long);

    return (ofs);
}

#define jit_getarg_ul(r0, ofs)		mips_getarg_l(_jit, r0, ofs)
#define jit_getarg_l(r0, ofs)		mips_getarg_l(_jit, r0, ofs)
__jit_inline void
mips_getarg_l(jit_state_t _jit, jit_gpr_t r0, int ofs)
{
    if (ofs < JIT_A_NUM)
	jit_movr_l(r0, jit_a_order[ofs]);
    else
	jit_ldxi_l(r0, JIT_FP, ofs);
}

#define jit_finishr(rs)			mips_finishr(_jit, rs)
__jit_inline void
mips_finishr(jit_state_t _jit, jit_gpr_t r0)
{
    assert(_jitl.stack_offset == 0 && _jitl.nextarg_put == 0);
    jit_callr(r0);
}

#define jit_finish(i0)			mips_finish(_jit, i0)
__jit_inline jit_insn *
mips_finish(jit_state_t _jit, void *i0)
{
    assert(_jitl.stack_offset == 0 && _jitl.nextarg_put == 0);
    return (jit_calli(i0));
}

#define jit_retval_i(r0)		mips_retval_l(_jit, r0)
__jit_inline void
mips_retval_l(jit_state_t _jit, jit_gpr_t r0)
{
    jit_movr_l(r0, JIT_RET);
}

#define jit_ret()			mips_ret(_jit)
__jit_inline void
mips_ret(jit_state_t jit)
{
    jit_movr_l(JIT_SP, JIT_FP);
    _LDC1(_F28, 0, JIT_SP);
    _LDC1(_F30, 8, JIT_SP);
    jit_ldxi_l(_S0, JIT_SP, 16);
    jit_ldxi_l(_S1, JIT_SP, 24);
    jit_ldxi_l(_S2, JIT_SP, 32);
    jit_ldxi_l(_S3, JIT_SP, 40);
    jit_ldxi_l(_S4, JIT_SP, 48);
    jit_ldxi_l(_S5, JIT_SP, 56);
    jit_ldxi_l(_S6, JIT_SP, 64);
    jit_ldxi_l(_S7, JIT_SP, 72);
    jit_ldxi_l(_FP, JIT_SP, 80);
    jit_ldxi_l(_RA, JIT_SP, 88);
    _JR(_RA);
    /* restore sp in delay slot */
    jit_addi_l(JIT_SP, JIT_SP, JIT_FRAMESIZE);
}

#endif /* __lightning_core_h */
