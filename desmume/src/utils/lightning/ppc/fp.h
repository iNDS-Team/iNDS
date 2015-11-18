/******************************** -*- C -*- ****************************
 *
 *	Run-time assembler & support macros for the PowerPC math unit
 *
 ***********************************************************************/


/***********************************************************************
 *
 * Copyright 2000, 2001, 2002 Free Software Foundation, Inc.
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




#ifndef __lightning_asm_fp_h
#define __lightning_asm_fp_h


#define JIT_FPR_NUM	       6
#define JIT_FPRET	       1
#define JIT_FPR(i)	       (8+(i))


/* Make space for 1 or 2 words, store address in REG */
#define jit_data(REG, D1)	        (_FBA	(18, 8, 0, 1),  _jit_L(D1), MFLRr(REG))

#define jit_addr_d(rd,s1,s2)  FADDDrrr((rd),(s1),(s2))
#define jit_subr_d(rd,s1,s2)  FSUBDrrr((rd),(s1),(s2))
#define jit_mulr_d(rd,s1,s2)  FMULDrrr((rd),(s1),(s2))
#define jit_divr_d(rd,s1,s2)  FDIVDrrr((rd),(s1),(s2))

#define jit_addr_f(rd,s1,s2)  FADDSrrr((rd),(s1),(s2))
#define jit_subr_f(rd,s1,s2)  FSUBSrrr((rd),(s1),(s2))
#define jit_mulr_f(rd,s1,s2)  FMULSrrr((rd),(s1),(s2))
#define jit_divr_f(rd,s1,s2)  FDIVSrrr((rd),(s1),(s2))

#define jit_movr_d(rd,rs)     ( (rd) == (rs) ? 0 : FMOVErr((rd),(rs)))
#define jit_movi_d(reg0,d) do {                   \
      double _v = (d);                            \
      _FBA (18, 12, 0, 1); 			  \
      memcpy(_jit->x.uc_pc, &_v, sizeof (double)); \
      _jit->x.uc_pc += sizeof (double);            \
      MFLRr (JIT_AUX);				  \
      jit_ldxi_d((reg0), JIT_AUX, 0);		  \
   } while(0) 


#define jit_movr_f(rd,rs)     ( (rd) == (rs) ? 0 : FMOVErr((rd),(rs)))
#define jit_movi_f(reg0,f) do {                   \
      float _v = (f);                             \
      _FBA (18, 8, 0, 1); 			  \
      memcpy(_jit->x.uc_pc, &_v, sizeof (float));  \
      _jit->x.uc_pc += sizeof (float);             \
      MFLRr (JIT_AUX);				  \
      jit_ldxi_f((reg0), JIT_AUX, 0);		  \
   } while(0) 


#define jit_absr_d(rd,rs)		FABSrr((rd),(rs))
#define jit_negr_d(rd,rs)		FNEGrr((rd),(rs))
#define jit_sqrtr_d(rd,rs)		FSQRTDrr((rd),(rs))

#define jit_ltr_d(r0, r1, r2)		ppc_ltr_d(_jit, r0, r1, r2)
__jit_inline void
ppc_ltr_d(jit_state_t _jit, int r0, int r1, int r2)
{
    FCMPOrrr(_cr0, r1, r2);
    MFCRr(r0);
    EXTRWIrrii(r0, r0, 1, _lt);
}

#define jit_ler_d(r0, r1, r2)		ppc_ler_d(_jit, r0, r1, r2)
__jit_inline void
ppc_ler_d(jit_state_t _jit, int r0, int r1, int r2)
{
    FCMPOrrr(_cr0, r1, r2);
    CREQViii(_gt, _gt, _un);
    MFCRr(r0);
    EXTRWIrrii(r0, r0, 1, _gt);
}

#define jit_eqr_d(r0, r1, r2)		ppc_eqr_d(_jit, r0, r1, r2)
__jit_inline void
ppc_eqr_d(jit_state_t _jit, int r0, int r1, int r2)
{
    FCMPOrrr(_cr0, r1, r2);
    MFCRr(r0);
    EXTRWIrrii(r0, r0, 1, _eq);
}

#define jit_ger_d(r0, r1, r2)		ppc_ger_d(_jit, r0, r1, r2)
__jit_inline void
ppc_ger_d(jit_state_t _jit, int r0, int r1, int r2)
{
    FCMPOrrr(_cr0, r1, r2);
    CREQViii(_lt, _lt, _un);
    MFCRr(r0);
    EXTRWIrrii(r0, r0, 1, _lt);
}

#define jit_gtr_d(r0, r1, r2)		ppc_gtr_d(_jit, r0, r1, r2)
__jit_inline void
ppc_gtr_d(jit_state_t _jit, int r0, int r1, int r2)
{
    FCMPOrrr(_cr0, r1, r2);
    MFCRr(r0);
    EXTRWIrrii(r0, r0, 1, _gt);
}

#define jit_ner_d(r0, r1, r2)		ppc_ner_d(_jit, r0, r1, r2)
__jit_inline void
ppc_ner_d(jit_state_t _jit, int r0, int r1, int r2)
{
    FCMPOrrr(_cr0, r1, r2);
    CRNOTii(_eq, _eq);
    MFCRr(r0);
    EXTRWIrrii(r0, r0, 1, _eq);
}

#define jit_unltr_d(r0, r1, r2)		ppc_unltr_d(_jit, r0, r1, r2)
__jit_inline void
ppc_unltr_d(jit_state_t _jit, int r0, int r1, int r2)
{
    FCMPUrrr(_cr0, r1, r2);
    CRORiii(_lt, _lt, _un);
    MFCRr(r0);
    EXTRWIrrii(r0, r0, 1, _lt);
}

#define jit_unler_d(r0, r1, r2)		ppc_unler_d(_jit, r0, r1, r2)
__jit_inline void
ppc_unler_d(jit_state_t _jit, int r0, int r1, int r2)
{
    FCMPUrrr(_cr0, r1, r2);
    CRNOTii(_gt, _gt);
    MFCRr(r0);
    EXTRWIrrii(r0, r0, 1, _gt);
}

#define jit_uneqr_d(r0, r1, r2)		ppc_uneqr_d(_jit, r0, r1, r2)
__jit_inline void
ppc_uneqr_d(jit_state_t _jit, int r0, int r1, int r2)
{
    FCMPUrrr(_cr0, r1, r2);
    CRORiii(_eq, _eq, _un);
    MFCRr(r0);
    EXTRWIrrii(r0, r0, 1, _eq);
}

#define jit_unger_d(r0, r1, r2)		ppc_unger_d(_jit, r0, r1, r2)
__jit_inline void
ppc_unger_d(jit_state_t _jit, int r0, int r1, int r2)
{
    FCMPUrrr(_cr0, r1, r2);
    CRNOTii(_lt, _lt);
    MFCRr(r0);
    EXTRWIrrii(r0, r0, 1, _lt);
}

#define jit_ungtr_d(r0, r1, r2)		ppc_ungtr_d(_jit, r0, r1, r2)
__jit_inline void
ppc_ungtr_d(jit_state_t _jit, int r0, int r1, int r2)
{
    FCMPUrrr(_cr0, r1, r2);
    CRORiii(_gt, _gt, _un);
    MFCRr(r0);
    EXTRWIrrii(r0, r0, 1, _gt);
}

#define jit_ltgtr_d(r0, r1, r2)		ppc_ltgtr_d(_jit, r0, r1, r2)
__jit_inline void
ppc_ltgtr_d(jit_state_t _jit, int r0, int r1, int r2)
{
    FCMPUrrr(_cr0, r1, r2);
    CRORiii(_gt, _gt, _lt);
    MFCRr(r0);
    EXTRWIrrii(r0, r0, 1, _gt);
}

#define jit_ordr_d(r0, r1, r2)		ppc_ordr_d(_jit, r0, r1, r2)
__jit_inline void
ppc_ordr_d(jit_state_t _jit, int r0, int r1, int r2)
{
    FCMPUrrr(_cr0, r1, r2);
    CRNOTii(_un, _un);
    MFCRr(r0);
    EXTRWIrrii(r0, r0, 1, _un);
}

#define jit_unordr_d(r0, r1, r2)	ppc_unordr_d(_jit, r0, r1, r2)
__jit_inline void
ppc_unordr_d(jit_state_t _jit, int r0, int r1, int r2)
{
    FCMPUrrr(_cr0, r1, r2);
    MFCRr(r0);
    EXTRWIrrii(r0, r0, 1, _un);
}

#define jit_bltr_d(i0, r0, r1)		ppc_bltr_d(_jit, i0, r0, r1)
__jit_inline jit_insn *
ppc_bltr_d(jit_state_t _jit, jit_insn *i0, int r0, int r1)
{
    FCMPOrrr(_cr0, r0, r1);
    BLTi(i0);
    return (_jit->x.pc);
}

#define jit_bler_d(i0, r0, r1)		ppc_bler_d(_jit, i0, r0, r1)
__jit_inline jit_insn *
ppc_bler_d(jit_state_t _jit, jit_insn *i0, int r0, int r1)
{
    FCMPOrrr(_cr0, r0, r1);
    CREQViii(_gt, _gt, _un);
    BGTi(i0);
    return (_jit->x.pc);
}

#define jit_beqr_d(i0, r0, r1)		ppc_beqr_d(_jit, i0, r0, r1)
__jit_inline jit_insn *
ppc_beqr_d(jit_state_t _jit, jit_insn *i0, int r0, int r1)
{
    FCMPOrrr(_cr0, r0, r1);
    BEQi(i0);
    return (_jit->x.pc);
}

#define jit_bger_d(i0, r0, r1)		ppc_bger_d(_jit, i0, r0, r1)
__jit_inline jit_insn *
ppc_bger_d(jit_state_t _jit, jit_insn *i0, int r0, int r1)
{
    FCMPOrrr(_cr0, r0, r1);
    CREQViii(_lt, _lt, _un);
    BLTi(i0);
    return (_jit->x.pc);
}

#define jit_bgtr_d(i0, r0, r1)		ppc_bgtr_d(_jit, i0, r0, r1)
__jit_inline jit_insn *
ppc_bgtr_d(jit_state_t _jit, jit_insn *i0, int r0, int r1)
{
    FCMPOrrr(_cr0, r0, r1);
    BGTi(i0);
    return (_jit->x.pc);
}

#define jit_bner_d(i0, r0, r1)		ppc_bner_d(_jit, i0, r0, r1)
__jit_inline jit_insn *
ppc_bner_d(jit_state_t _jit, jit_insn *i0, int r0, int r1)
{
    FCMPOrrr(_cr0, r0, r1);
    BNEi(i0);
    return (_jit->x.pc);
}

#define jit_bunltr_d(i0, r0, r1)	ppc_bunltr_d(_jit, i0, r0, r1)
__jit_inline jit_insn *
ppc_bunltr_d(jit_state_t _jit, jit_insn *i0, int r0, int r1)
{
    FCMPUrrr(_cr0, r0, r1);
    CRORiii(_lt, _lt, _un);
    BLTi(i0);
    return (_jit->x.pc);
}

#define jit_bunler_d(i0, r0, r1)	ppc_bunler_d(_jit, i0, r0, r1)
__jit_inline jit_insn *
ppc_bunler_d(jit_state_t _jit, jit_insn *i0, int r0, int r1)
{
    FCMPUrrr(_cr0, r0, r1);
    BLEi(i0);
    return (_jit->x.pc);
}

#define jit_buneqr_d(i0, r0, r1)	ppc_buneqr_d(_jit, i0, r0, r1)
__jit_inline jit_insn *
ppc_buneqr_d(jit_state_t _jit, jit_insn *i0, int r0, int r1)
{
    FCMPUrrr(_cr0, r0, r1);
    CRORiii(_eq, _eq, _un);
    BEQi(i0);
    return (_jit->x.pc);
}

#define jit_bunger_d(i0, r0, r1)	ppc_bunger_d(_jit, i0, r0, r1)
__jit_inline jit_insn *
ppc_bunger_d(jit_state_t _jit, jit_insn *i0, int r0, int r1)
{
    FCMPUrrr(_cr0, r0, r1);
    BGEi(i0);
    return (_jit->x.pc);
}

#define jit_bungtr_d(i0, r0, r1)	ppc_bungtr_d(_jit, i0, r0, r1)
__jit_inline jit_insn *
ppc_bungtr_d(jit_state_t _jit, jit_insn *i0, int r0, int r1)
{
    FCMPUrrr(_cr0, r0, r1);
    CRORiii(_gt, _gt, _un);
    BGTi(i0);
    return (_jit->x.pc);
}

#define jit_bltgtr_d(i0, r0, r1)	ppc_bltgtr_d(_jit, i0, r0, r1)
__jit_inline jit_insn *
ppc_bltgtr_d(jit_state_t _jit, jit_insn *i0, int r0, int r1)
{
    FCMPUrrr(_cr0, r0, r1);
    CRORiii(_eq, _lt, _gt);
    BEQi(i0);
    return (_jit->x.pc);
}

#define jit_bordr_d(i0, r0, r1)		ppc_bordr_d(_jit, i0, r0, r1)
__jit_inline jit_insn *
ppc_bordr_d(jit_state_t _jit, jit_insn *i0, int r0, int r1)
{
    FCMPUrrr(_cr0, r0, r1);
    BNUi(i0);
    return (_jit->x.pc);
}

#define jit_bunordr_d(i0, r0, r1)	ppc_bunordr_d(_jit, i0, r0, r1)
__jit_inline jit_insn *
ppc_bunordr_d(jit_state_t _jit, jit_insn *i0, int r0, int r1)
{
    FCMPUrrr(_cr0, r0, r1);
    BUNi(i0);
    return (_jit->x.pc);
}

#define jit_ldr_f(r0, r1)		LFSxrrr(r0, 0, r1)
#define jit_ldi_f(r0, i0)		ppc_ldi_f(_jit, r0, i0)
__jit_inline void
ppc_ldi_f(jit_state_t _jit, int r0, void *i0)
{
    int		lo, hi;
    if (jit_can_sign_extend_short_p((long)i0))
	LFSrri(r0, 0, i0);
    else {
	hi = ((_ui)i0 >> 16) + ((_us)((_ui)i0) >> 15);
	lo = (_us)((_ui)i0 - (hi << 16));
	LISri(JIT_AUX, hi);
	LFSrri(r0, JIT_AUX, lo);
    }
}

#define jit_ldxr_f(r0, r1, r2)		ppc_ldxr_f(_jit, r0, r1, r2)
__jit_inline void
ppc_ldxr_f(jit_state_t _jit, int r0, int r1, int r2)
{
    if (r1 == 0) {
	if (r2 != 0)
	    LFSxrrr(r0, r2, r1);
	else {
	    jit_movr_i(JIT_AUX, r1);
	    LFSxrrr(r0, JIT_AUX, r2);
	}
    }
    else
	LFSxrrr(r0, r1, r2);
}

#define jit_ldxi_f(r0, r1, i0)		ppc_ldxi_f(_jit, r0, r1, i0)
__jit_inline void
ppc_ldxi_f(jit_state_t _jit, int r0, int r1, int i0)
{
    if (i0 == 0)
	jit_ldr_f(r0, r1);
    else if (jit_can_sign_extend_short_p(i0)) {
	if (r1 == 0) {
	    jit_movr_i(JIT_AUX, r1);
	    LFSrri(r0, JIT_AUX, i0);
	}
	else
	    LFSrri(r0, r1, i0);
    }
    else {
	jit_movi_i(JIT_AUX, i0);
	jit_ldxr_f(r0, r1, JIT_AUX);
    }
}

#define jit_str_f(r0, r1)		STFSxrrr(r1, 0, r0)
#define jit_sti_f(i0, r0)		ppc_sti_f(_jit, i0, r0)
__jit_inline void
ppc_sti_f(jit_state_t _jit, void *i0, int r0)
{
    int		lo, hi;
    if (jit_can_sign_extend_short_p((long)i0))
	STFSrri(r0, 0, i0);
    else {
	hi = ((_ui)i0 >> 16) + ((_us)((_ui)i0) >> 15);
	lo = (_us)((_ui)i0 - (hi << 16));
	LISri(JIT_AUX, hi);
	STFSrri(r0, JIT_AUX, lo);
    }
}

#define jit_stxr_f(r0, r1, r2)		ppc_stxr_f(_jit, r0, r1, r2)
__jit_inline void
ppc_stxr_f(jit_state_t _jit, int r0, int r1, int r2)
{
    if (r0 == 0) {
	if (r1 != 0)
	    STFSxrrr(r2, r1, r0);
	else {
	    jit_movr_i(JIT_AUX, r1);
	    STFSxrrr(r2, JIT_AUX, r0);
	}
    }
    else
	STFSxrrr(r2, r0, r1);
}

#define jit_stxi_f(i0, r0, r1)		ppc_stxi_f(_jit, i0, r0, r1)
__jit_inline void
ppc_stxi_f(jit_state_t _jit, int i0, int r0, int r1)
{
    if (i0 == 0)
	jit_str_f(r0, r1);
    else if (jit_can_sign_extend_short_p(i0)) {
	if (r0 == 0) {
	    jit_movr_i(JIT_AUX, r0);
	    STFSrri(r1, JIT_AUX, i0);
	}
	else
	    STFSrri(r1, r0, i0);
    }
    else {
	jit_movi_i(JIT_AUX, i0);
	jit_stxr_f(JIT_AUX, r0, r1);
    }
}

#define jit_ldr_d(r0, r1)		LFDxrrr(r0, 0, r1)
#define jit_ldi_d(r0, i0)		ppc_ldi_d(_jit, r0, i0)
__jit_inline void
ppc_ldi_d(jit_state_t _jit, int r0, void *i0)
{
    int		lo, hi;
    if (jit_can_sign_extend_short_p((long)i0))
	LFDrri(r0, 0, i0);
    else {
	hi = ((_ui)i0 >> 16) + ((_us)((_ui)i0) >> 15);
	lo = (_us)((_ui)i0 - (hi << 16));
	LISri(JIT_AUX, hi);
	LFDrri(r0, JIT_AUX, lo);
    }
}

#define jit_ldxr_d(r0, r1, r2)		ppc_ldxr_d(_jit, r0, r1, r2)
__jit_inline void
ppc_ldxr_d(jit_state_t _jit, int r0, int r1, int r2)
{
    if (r1 == 0) {
	if (r2 != 0)
	    LFDxrrr(r0, r2, r1);
	else {
	    jit_movr_i(JIT_AUX, r1);
	    LFDxrrr(r0, JIT_AUX, r2);
	}
    }
    else
	LFDxrrr(r0, r1, r2);
}

#define jit_ldxi_d(r0, r1, i0)		ppc_ldxi_d(_jit, r0, r1, i0)
__jit_inline void
ppc_ldxi_d(jit_state_t _jit, int r0, int r1, int i0)
{
    if (i0 == 0)
	jit_ldr_d(r0, r1);
    else if (jit_can_sign_extend_short_p(i0)) {
	if (r1 == 0) {
	    jit_movr_i(JIT_AUX, r1);
	    LFDrri(r0, JIT_AUX, i0);
	}
	else
	    LFDrri(r0, r1, i0);
    }
    else {
	jit_movi_i(JIT_AUX, i0);
	jit_ldxr_d(r0, r1, JIT_AUX);
    }
}

#define jit_str_d(r0, r1)		STFDxrrr(r1, 0, r0)
#define jit_sti_d(i0, r0)		ppc_sti_d(_jit, i0, r0)
__jit_inline void
ppc_sti_d(jit_state_t _jit, void *i0, int r0)
{
    int		lo, hi;
    if (jit_can_sign_extend_short_p((long)i0))
	STFDrri(r0, 0, i0);
    else {
	hi = ((_ui)i0 >> 16) + ((_us)((_ui)i0) >> 15);
	lo = (_us)((_ui)i0 - (hi << 16));
	LISri(JIT_AUX, hi);
	STFDrri(r0, JIT_AUX, lo);
    }
}

#define jit_stxr_d(r0, r1, r2)		ppc_stxr_d(_jit, r0, r1, r2)
__jit_inline void
ppc_stxr_d(jit_state_t _jit, int r0, int r1, int r2)
{
    if (r0 == 0) {
	if (r1 != 0)
	    STFDxrrr(r2, r1, r0);
	else {
	    jit_movr_i(JIT_AUX, r0);
	    STFDxrrr(r2, 0, JIT_AUX);
	}
    }
    else
	STFDxrrr(r2, r0, r1);
}

#define jit_stxi_d(i0, r0, r1)		ppc_stxi_d(_jit, i0, r0, r1)
__jit_inline void
ppc_stxi_d(jit_state_t _jit, int i0, int r0, int r1)
{
    if (i0 == 0)
	jit_str_d(r0, r1);
    else if (jit_can_sign_extend_short_p(i0)) {
	if (r0 == 0) {
	    jit_movr_i(JIT_AUX, r0);
	    STFDrri(r1, JIT_AUX, i0);
	}
	else
	    STFDrri(r1, r0, i0);
    }
    else {
	jit_movi_i(JIT_AUX, i0);
	jit_stxr_d(JIT_AUX, r0, r1);
    }
}

#define jit_pusharg_d(rs)	     (_jitl.nextarg_putd--,jit_movr_d((_jitl.nextarg_putf+_jitl.nextarg_putd+1), (rs)))
#define jit_pusharg_f(rs)	     (_jitl.nextarg_putf--,jit_movr_f((_jitl.nextarg_putf+_jitl.nextarg_putd+1), (rs)))


#define jit_floorr_d_i(rd,rs)  (MTFSFIri(7,3), \
                                  FCTIWrr(7,(rs)),    \
                                  MOVEIri(JIT_AUX,-4), \
                                  STFIWXrrr(7,JIT_SP,JIT_AUX),   \
                                  LWZrm((rd),-4,JIT_SP))

#define jit_ceilr_d_i(rd,rs)   (MTFSFIri(7,2), \
                                  FCTIWrr(7,(rs)),    \
                                  MOVEIri(JIT_AUX,-4), \
                                  STFIWXrrr(7,JIT_SP,JIT_AUX),   \
                                  LWZrm((rd),-4,JIT_SP))

#define jit_roundr_d_i(rd,rs)  (MTFSFIri(7,0), \
                                  FCTIWrr(7,(rs)),    \
                                  MOVEIri(JIT_AUX,-4), \
                                  STFIWXrrr(7,JIT_SP,JIT_AUX),   \
                                  LWZrm((rd),-4,JIT_SP))

#define jit_truncr_d_i(rd,rs)  (FCTIWZrr(7,(rs)), \
                                  MOVEIri(JIT_AUX,-4), \
                                  STFIWXrrr(7,JIT_SP,JIT_AUX),   \
                                  LWZrm((rd),-4,JIT_SP))

/* FIXME 64 bit instruction, should work on recent processors,
 * explicitly documented as not working in 32 bit mode, but
 * works on test Darwin PPC host..., otherwise, need to inline
 * or call some integer to float function */
#define jit_extr_i_d(rd,rs)	(jit_stxi_i(-4, JIT_SP, (rs)),		\
				 jit_rshi_i(JIT_AUX, (rs), 31),		\
				 jit_stxi_i(-8, JIT_SP, JIT_AUX),	\
				 jit_ldxi_d((rd), JIT_SP, -8),		\
				 FCFIDrr((rd), (rd)))

#endif /* __lightning_asm_h */
