/******************************** -*- C -*- ****************************
 *
 *	Run-time assembler for the mips
 *
 ***********************************************************************/

/***********************************************************************
 *
 * Copyright 2010 Free Software Foundation
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

#ifndef __lightning_asm_h
#define __lightning_asm_h

typedef enum {
    _ZERO	= 0x00,		/* constant 0 */
    _AT		= 0x01,		/* assembly temporary */
    _V0		= 0x02,		/* function return and expression evaluation */
    _V1		= 0x03,
    _A0		= 0x04,		/* function arguments */
    _A1		= 0x05,
    _A2		= 0x06,
    _A3		= 0x07,
    _T0		= 0x08,		/* temporaries */
    _T1		= 0x09,
    _T2		= 0x0a,
    _T3		= 0x0b,
    _T4		= 0x0c,
    _T5		= 0x0d,
    _T6		= 0x0e,
    _T7		= 0x0f,
    _S0		= 0x10,		/* (callee saved) temporaries */
    _S1		= 0x11,
    _S2		= 0x12,
    _S3		= 0x13,
    _S4		= 0x14,
    _S5		= 0x15,
    _S6		= 0x16,
    _S7		= 0x17,
    _T8		= 0x18,		/* temporaries */
    _T9		= 0x19,
    _K0		= 0x1a,		/* reserved for OS kernel */
    _K1		= 0x1b,
    _GP		= 0x1c,		/* (callee saved) global pointer */
    _SP		= 0x1d,		/* (callee saved) stack pointer */
    _FP		= 0x1e,		/* (callee saved) frame pointer */
    _RA		= 0x1f,		/* return address */
} jit_gpr_t;

typedef enum {
    _F0		= 0x00,
    _F1		= 0x01,
    _F2		= 0x02,
    _F3		= 0x03,
    _F4		= 0x04,
    _F5		= 0x05,
    _F6		= 0x06,
    _F7		= 0x07,
    _F8		= 0x08,
    _F9		= 0x09,
    _F10	= 0x0a,
    _F11	= 0x0b,
    _F12	= 0x0c,
    _F13	= 0x0d,
    _F14	= 0x0e,
    _F15	= 0x0f,
    _F16	= 0x10,
    _F17	= 0x11,
    _F18	= 0x12,
    _F19	= 0x13,
    _F20	= 0x14,
    _F21	= 0x15,
    _F22	= 0x16,
    _F23	= 0x17,
    _F24	= 0x18,
    _F25	= 0x19,
    _F26	= 0x1a,
    _F27	= 0x1b,
    _F28	= 0x1c,
    _F29	= 0x1d,
    _F30	= 0x1e,
    _F31	= 0x1f,
} jit_fpr_t;

typedef union {
    struct {	_ui _:26;	_ui	b :  6; } hc;
    struct {	_ui _:21;	_ui	b :  5; } rs;
    struct {	_ui _:21;	_ui	b :  5; } fm;
    struct {	_ui _:16;	_ui	b :  5; } rt;
    struct {	_ui _:16;	_ui	b :  5; } ft;
    struct {	_ui _:11;	_ui	b :  5; } rd;
    struct {	_ui _:11;	_ui	b :  5; } fs;
    struct {	_ui _: 6;	_ui	b :  5; } ic;
    struct {	_ui _: 6;	_ui	b :  5; } fd;
    struct {	_ui _: 6;	_ui	b : 10; } tr;
    struct {	_ui _: 6;	_ui	b : 20; } br;
    struct {			_ui	b :  6; } tc;
    struct {			_ui	b : 11; } cc;
    struct {			_ui	b : 16; } is;
    struct {			_ui	b : 26; } ii;
    int						  op;
} mips_code_t;

typedef enum {
    MIPS_SPECIAL	= 0x00,
    MIPS_REGIMM		= 0x01,
    MIPS_J		= 0x02,
    MIPS_SRL		= 0x02,
    MIPS_JAL		= 0x03,
    MIPS_SRA		= 0x03,
    MIPS_BEQ		= 0x04,
    MIPS_BNE		= 0x05,
    MIPS_BLEZ		= 0x06,
    MIPS_BGTZ		= 0x07,
    MIPS_ADDI		= 0x08,
    MIPS_ADDIU		= 0x09,
    MIPS_SLTI		= 0x0a,
    MIPS_SLTIU		= 0x0b,
    MIPS_ANDI		= 0x0c,
    MIPS_ORI		= 0x0d,
    MIPS_XORI		= 0x0e,
    MIPS_LUI		= 0x0f,
    MIPS_COP0		= 0x10,
    MIPS_COP1		= 0x11,
    MIPS_COP2		= 0x12,
    MIPS_COP1X		= 0x13,
    MIPS_BEQL		= 0x14,
    MIPS_BNEL		= 0x15,
    MIPS_BLEZL		= 0x16,
    MIPS_BGTZL		= 0x17,
    MIPS_DADDI		= 0x18,
    MIPS_DADDIU		= 0x19,
    MIPS_LDL		= 0x1a,
    MIPS_LDR		= 0x1b,
    MIPS_SPECIAL2	= 0x1c,
    MIPS_JALX		= 0x1d,
    MIPS_SPECIAL3	= 0x1f,
    MIPS_LB		= 0x20,
    MIPS_LH		= 0x21,
    MIPS_LWL		= 0x22,
    MIPS_LW		= 0x23,
    MIPS_LBU		= 0x24,
    MIPS_LHU		= 0x25,
    MIPS_LWR		= 0x26,
    MIPS_LWU		= 0x27,
    MIPS_SB		= 0x28,
    MIPS_SH		= 0x29,
    MIPS_SWL		= 0x2a,
    MIPS_SW		= 0x2b,
    MIPS_SWR		= 0x2e,
    MIPS_CACHE		= 0x2f,
    MIPS_LL		= 0x30,
    MIPS_LWC1		= 0x31,
    MIPS_LWC2		= 0x32,
    MIPS_PREF		= 0x33,
    MIPS_LLD		= 0x34,
    MIPS_LDC1		= 0x35,
    MIPS_LDC2		= 0x36,
    MIPS_LD		= 0x37,
    MIPS_SC		= 0x38,
    MIPS_SCD		= 0x3c,
    MIPS_SDC1		= 0x3d,
    MIPS_SDC2		= 0x3e,
    MIPS_SWC1		= 0x39,
    MIPS_SWC2		= 0x3a,
    MIPS_SD		= 0x3f,
} mips_hc_t;

typedef enum {
    MIPS_MF		= 0x00,
    MIPS_DMF		= 0x01,
    MIPS_CF		= 0x02,
    MIPS_MFH		= 0x03,
    MIPS_MT		= 0x04,
    MIPS_DMT		= 0x05,
    MIPS_CT		= 0x06,
    MIPS_MTH		= 0x07,
    MIPS_BC		= 0x08,
    MIPS_WRPGPR		= 0x0e,
    MIPS_BGZAL		= 0x11,
    MIPS_MFMC0		= 0x11,
} mips_r1_t;

typedef enum {
    MIPS_BCF		= 0x00,
    MIPS_BLTZ		= 0x00,
    MIPS_BCT		= 0x01,
    MIPS_BGEZ		= 0x01,
    MIPS_BCFL		= 0x02,
    MIPS_BLTZL		= 0x02,
    MIPS_BCTL		= 0x03,
    MIPS_BGEZL		= 0x03,
    MIPS_TGEI		= 0x08,
    MIPS_TGEIU		= 0x09,
    MIPS_TLTI		= 0x0a,
    MIPS_TLTIU		= 0x0b,
    MIPS_TEQI		= 0x0c,
    MIPS_TNEI		= 0x0e,
    MIPS_BLTZAL		= 0x10,
    MIPS_BGEZAL		= 0x11,
    MIPS_BLTZALL	= 0x12,
    MIPS_BGEZALL	= 0x13,
    MIPS_SYNCI		= 0x1f,
} mips_r2_t;

#define MIPS_WSBH		0x02
#define MIPS_DBSH		0x02
#define MIPS_DSHD		0x05
#define MIPS_SEB		0x10
#define MIPS_SEH		0x18

typedef enum {
    MIPS_MADD		= 0x00,
    MIPS_SLL		= 0x00,
    MIPS_EXT		= 0x00,
    MIPS_DEXTM		= 0x01,
    MIPS_MADDU		= 0x01,
    MIPS_MOVFT		= 0x01,
    MIPS_TLBR		= 0x01,
    MIPS_MUL		= 0x02,
    MIPS_DEXTU		= 0x02,
    MIPS_TLBWI		= 0x02,
    MIPS_DEXT		= 0x03,
    MIPS_SLLV		= 0x04,
    MIPS_INS		= 0x04,
    MIPS_MSUB		= 0x04,
    MIPS_DINSM		= 0x05,
    MIPS_MSUBU		= 0x05,
    MIPS_SRLV		= 0x06,
    MIPS_DINSU		= 0x06,
    MIPS_TLBWR		= 0x06,
    MIPS_SRAV		= 0x07,
    MIPS_DINS		= 0x07,
    MIPS_JR		= 0x08,
    MIPS_TLBP		= 0x08,
    MIPS_JALR		= 0x09,
    MIPS_MOVZ		= 0x0a,
    MIPS_MOVN		= 0x0b,
    MIPS_SYSCALL	= 0x0c,
    MIPS_BREAK		= 0x0d,
    MIPS_PREFX		= 0x0f,
    MIPS_SYNC		= 0x0f,
    MIPS_MFHI		= 0x10,
    MIPS_MTHI		= 0x11,
    MIPS_MFLO		= 0x12,
    MIPS_MTLO		= 0x13,
    MIPS_DSLLV		= 0x14,
    MIPS_DSRLV		= 0x16,
    MIPS_DSRAV		= 0x17,
    MIPS_MULT		= 0x18,
    MIPS_ERET		= 0x18,
    MIPS_MULTU		= 0x19,
    MIPS_DIV		= 0x1a,
    MIPS_DIVU		= 0x1b,
    MIPS_DMULT		= 0x1c,
    MIPS_DMULTU		= 0x1d,
    MIPS_DDIV		= 0x1e,
    MIPS_DDIVU		= 0x1f,
    MIPS_DERET		= 0x1f,
    MIPS_ADD		= 0x20,
    MIPS_CLZ		= 0x20,
    MIPS_BSHFL		= 0x20,
    MIPS_ADDU		= 0x21,
    MIPS_CLO		= 0x21,
    MIPS_SUB		= 0x22,
    MIPS_SUBU		= 0x23,
    MIPS_AND		= 0x24,
    MIPS_DCLZ		= 0x24,
    MIPS_DBSHFL		= 0x24,
    MIPS_OR		= 0x25,
    MIPS_DCLO		= 0x25,
    MIPS_XOR		= 0x26,
    MIPS_NOR		= 0x27,
    MIPS_SLT		= 0x2a,
    MIPS_SLTU		= 0x2b,
    MIPS_DADD		= 0x2c,
    MIPS_DADDU		= 0x2d,
    MIPS_DSUB		= 0x2e,
    MIPS_DSUBU		= 0x2f,
    MIPS_TGE		= 0x30,
    MIPS_TGEU		= 0x31,
    MIPS_TLT		= 0x32,
    MIPS_TLTU		= 0x33,
    MIPS_TEQ		= 0x34,
    MIPS_TNE		= 0x36,
    MIPS_DSLL		= 0x38,
    MIPS_DSRL		= 0x3a,
    MIPS_DSRA		= 0x3b,
    MIPS_DSLL32		= 0x3c,
    MIPS_DSRL32		= 0x3e,
    MIPS_DSRA32		= 0x3f,
    MIPS_SDBPP		= 0x3f,
} mips_tc_t;

typedef enum {
    MIPS_fmt_S		= 0x10,		/* float32 */
    MIPS_fmt_D		= 0x11,		/* float64 */
    MIPS_fmt_W		= 0x14,		/* int32 */
    MIPS_fmt_L		= 0x15,		/* int64 */
    MIPS_fmt_PS		= 0x16,		/* 2 x float32 */
    MIPS_fmt_S_PU	= 0x20,
    MIPS_fmt_S_PL	= 0x26,
} mips_fmt_t;

typedef enum {
    MIPS_ADD_fmt	= 0x00,
    MIPS_LWXC1		= 0x00,
    MIPS_SUB_fmt	= 0x01,
    MIPS_LDXC1		= 0x01,
    MIPS_MUL_fmt	= 0x02,
    MIPS_DIV_fmt	= 0x03,
    MIPS_SQRT_fmt	= 0x04,
    MIPS_ABS_fmt	= 0x05,
    MIPS_LUXC1		= 0x05,
    MIPS_MOV_fmt	= 0x06,
    MIPS_NEG_fmt	= 0x07,
    MIPS_SWXC1		= 0x08,
    MIPS_ROUND_fmt_L	= 0x08,
    MIPS_TRUNC_fmt_L	= 0x09,
    MIPS_SDXC1		= 0x09,
    MIPS_CEIL_fmt_L	= 0x0a,
    MIPS_FLOOR_fmt_L	= 0x0b,
    MIPS_ROUND_fmt_W	= 0x0c,
    MIPS_TRUNC_fmt_W	= 0x0d,
    MIPS_SUXC1		= 0x0d,
    MIPS_CEIL_fmt_W	= 0x0e,
    MIPS_FLOOR_fmt_W	= 0x0f,
    MIPS_RECIP		= 0x15,
    MIPS_RSQRT		= 0x16,
    MIPS_ALNV_PS	= 0x1e,
    MIPS_CVT_fmt_S	= 0x20,
    MIPS_CVT_fmt_D	= 0x21,
    MIPS_CVT_fmt_W	= 0x24,
    MIPS_CVT_fmt_L	= 0x25,
    MIPS_PLL		= 0x2c,
    MIPS_PLU		= 0x2d,
    MIPS_PUL		= 0x2e,
    MIPS_PUU		= 0x2f,
    MIPS_MADD_fmt_S	= 0x20 | MIPS_fmt_S,
    MIPS_MADD_fmt_D	= 0x20 | MIPS_fmt_D,
    MIPS_MADD_fmt_PS	= 0x20 | MIPS_fmt_PS,
    MIPS_MSUB_fmt_S	= 0x28 | MIPS_fmt_S,
    MIPS_MSUB_fmt_D	= 0x28 | MIPS_fmt_D,
    MIPS_MSUB_fmt_PS	= 0x28 | MIPS_fmt_PS,
    MIPS_NMADD_fmt_S	= 0x30 | MIPS_fmt_S,
    MIPS_NMADD_fmt_D	= 0x30 | MIPS_fmt_D,
    MIPS_NMADD_fmt_PS	= 0x30 | MIPS_fmt_PS,
    MIPS_NMSUB_fmt_S	= 0x38 | MIPS_fmt_S,
    MIPS_NMSUB_fmt_D	= 0x38 | MIPS_fmt_D,
    MIPS_NMSUB_fmt_PS	= 0x38 | MIPS_fmt_PS,
} mips_fc_t;

typedef enum {
    MIPS_cond_F		= 0x30,
    MIPS_cond_UN	= 0x31,
    MIPS_cond_EQ	= 0x32,
    MIPS_cond_UEQ	= 0x33,
    MIPS_cond_OLT	= 0x34,
    MIPS_cond_ULT	= 0x35,
    MIPS_cond_OLE	= 0x36,
    MIPS_cond_ULE	= 0x37,
    MIPS_cond_SF	= 0x38,
    MIPS_cond_NGLE	= 0x39,
    MIPS_cond_SEQ	= 0x3a,
    MIPS_cond_NGL	= 0x3b,
    MIPS_cond_LT	= 0x3c,
    MIPS_cond_NGE	= 0x3d,
    MIPS_cond_LE	= 0x3e,
    MIPS_cond_UGT	= 0x3f,
} mips_cond_t;

__jit_inline void
mips___r_t(jit_state_t _jit, jit_gpr_t rd, mips_tc_t tc)
{
    mips_code_t		c;
    c.op = 0;
    c.tc.b = tc;
    c.rd.b = rd;
    _jit_I(c.op);
}

__jit_inline void
mips_r___t(jit_state_t _jit, jit_gpr_t rs, mips_tc_t tc)
{
    mips_code_t		c;
    c.op = 0;
    c.rs.b = rs;
    c.tc.b = tc;
    _jit_I(c.op);
}

__jit_inline void
mipsh_ri(jit_state_t _jit, mips_hc_t hc, jit_gpr_t rt, int im)
{
    mips_code_t		c;
    c.is.b = _u16(im);
    c.rt.b = rt;
    c.rs.b = 0;		/* ignored */
    c.hc.b = hc;
    _jit_I(c.op);
}

__jit_inline void
mipshrri(jit_state_t _jit, mips_hc_t hc, jit_gpr_t rs, jit_gpr_t rt, int im)
{
    mips_code_t		c;
    c.is.b = _u16(im);
    c.rt.b = rt;
    c.rs.b = rs;
    c.hc.b = hc;
    _jit_I(c.op);
}

__jit_inline void
mipshrrr_t(jit_state_t _jit, mips_hc_t hc,
	   jit_gpr_t rs, jit_gpr_t rt, jit_gpr_t rd, mips_tc_t tc)
{
    mips_code_t		c;
    c.tc.b = tc;
    c.ic.b = 0;
    c.rd.b = rd;
    c.rt.b = rt;
    c.rs.b = rs;
    c.hc.b = hc;
    _jit_I(c.op);
}

__jit_inline void
mipshrr__t(jit_state_t _jit, mips_hc_t hc,
	   jit_gpr_t rs, jit_gpr_t rt, mips_tc_t tc)
{
    mips_code_t		c;
    c.op = 0;
    c.tc.b = tc;
    c.rt.b = rt;
    c.rs.b = rs;
    c.hc.b = hc;
    _jit_I(c.op);
}

__jit_inline void
mips_hrrrit(jit_state_t _jit, mips_hc_t hc,
	    jit_gpr_t rs, jit_gpr_t rt, jit_gpr_t rd, int ic, mips_tc_t tc)
{
    mips_code_t		c;
    c.tc.b = tc;
    c.ic.b = _u5(ic);
    c.rd.b = rd;
    c.rt.b = rt;
    c.rs.b = rs;
    c.hc.b = hc;
    _jit_I(c.op);
}

__jit_inline void
mips_rrr_t(jit_state_t _jit, jit_gpr_t rs,
	   jit_gpr_t rt, jit_gpr_t rd, mips_tc_t tc)
{
    mips_code_t		c;
    c.op = 0;
    c.tc.b = tc;
    c.rd.b = rd;
    c.rt.b = rt;
    c.rs.b = rs;
    _jit_I(c.op);
}

__jit_inline void
mips__rrit(jit_state_t _jit, jit_gpr_t rt, jit_gpr_t rd, int im, mips_tc_t tc)
{
    mips_code_t		c;
    c.op = 0;
    c.tc.b = tc;
    c.ic.b = _u5(im);
    c.rd.b = rd;
    c.rt.b = rt;
    _jit_I(c.op);
}

__jit_inline void
mips_r_it(jit_state_t _jit, jit_gpr_t rs, int im, mips_tc_t tc)
{
    mips_code_t		c;
    c.op = 0;
    c.tc.b = tc;
    c.ic.b = _u5(im);	/* hint */
    c.rs.b = rs;
    _jit_I(c.op);
}

__jit_inline void
mips_r_rit(jit_state_t _jit, jit_gpr_t rs, jit_gpr_t rd, int im, mips_tc_t tc)
{
    mips_code_t		c;
    c.op = 0;
    c.tc.b = tc;
    c.ic.b = _u5(im);
    c.rd.b = rd;
    c.rs.b = rs;
    _jit_I(c.op);
}

__jit_inline void
mipshi(jit_state_t _jit, mips_hc_t hc, int im)
{
    mips_code_t		c;
    c.ii.b = _u26(im);
    c.hc.b = hc;
    _jit_I(c.op);
}

__jit_inline void
mipshrriit(jit_state_t _jit, mips_hc_t hc, jit_gpr_t rs,
	   jit_gpr_t rt, int rd, int im, mips_tc_t tc)
{
    mips_code_t		c;
    c.tc.b = tc;
    c.ic.b = _u5(im);
    c.rd.b = _u5(rd);
    c.rt.b = rt;
    c.rs.b = rs;
    c.hc.b = hc;
    _jit_I(c.op);
}

__jit_inline void
mips_hirrit(jit_state_t _jit, mips_hc_t hc, int rs,
	    jit_gpr_t rt, jit_gpr_t rd, int im, mips_tc_t tc)
{
    mips_code_t		c;
    c.tc.b = tc;
    c.ic.b = _u5(im);
    c.rd.b = rd;
    c.rt.b = rt;
    c.rs.b = rs;
    c.hc.b = hc;
    _jit_I(c.op);
}

__jit_inline void
mips_fp1(jit_state_t _jit, mips_fmt_t fm,
	 jit_fpr_t fs, jit_fpr_t fd, mips_fc_t tc)
{
    mips_code_t		c;
    c.tc.b = tc;
    c.fd.b = fd;
    c.fs.b = fs;
    c.ft.b = 0;
    c.fm.b = fm;
    c.hc.b = MIPS_COP1;
    _jit_I(c.op);
}

__jit_inline void
mips_fp2(jit_state_t _jit, mips_fmt_t fm,
	 jit_fpr_t ft, jit_fpr_t fs, jit_fpr_t fd, mips_fc_t tc)
{
    mips_code_t		c;
    c.tc.b = tc;
    c.fd.b = fd;
    c.fs.b = fs;
    c.ft.b = ft;
    c.fm.b = fm;
    c.hc.b = MIPS_COP1;
    _jit_I(c.op);
}

__jit_inline void
mips_fp2x(jit_state_t _jit, jit_fpr_t fr,
	  jit_fpr_t ft, jit_fpr_t fs, jit_fpr_t fd, mips_fc_t tc)
{
    mips_code_t		c;
    c.tc.b = tc;
    c.fd.b = fd;
    c.fs.b = fs;
    c.ft.b = ft;
    c.fm.b = fr;
    c.hc.b = MIPS_COP1X;
    _jit_I(c.op);
}

__jit_inline void
mips_xrf(jit_state_t _jit, mips_fmt_t fm, jit_gpr_t rt, jit_gpr_t fs)
{
    mips_code_t		c;
    c.op = 0;
    c.fs.b = fs;
    c.ft.b = rt;
    c.fm.b = fm;
    c.hc.b = MIPS_COP1;
    _jit_I(c.op);
}

__jit_inline void
mipshrfi(jit_state_t _jit, mips_hc_t hc, jit_gpr_t rs, jit_gpr_t ft, int im)
{
    mips_code_t		c;
    c.is.b = _u16(im);
    c.rs.b = rs;
    c.rt.b = ft;
    c.hc.b = hc;
    _jit_I(c.op);
}

__jit_inline void
mips_rr_f(jit_state_t _jit, jit_gpr_t rs,
	  jit_gpr_t rt, jit_fpr_t fd, mips_fc_t tc)
{
    mips_code_t		c;
    c.tc.b = tc;
    c.is.b = fd;
    c.rd.b = 0;
    c.rt.b = rt;
    c.rs.b = rs;
    c.hc.b = MIPS_COP1X;
    _jit_I(c.op);
}

__jit_inline void
mips_rffft(jit_state_t _jit, jit_gpr_t rs,
	   jit_fpr_t ft, jit_fpr_t fs, jit_fpr_t fd, mips_fc_t tc)
{
    mips_code_t		c;
    c.tc.b = tc;
    c.fd.b = fd;
    c.fs.b = fs;
    c.ft.b = ft;
    c.fm.b = rs;
    c.hc.b = MIPS_COP1X;
    _jit_I(c.op);
}

__jit_inline void
mips_hxxs(jit_state_t _jit, mips_hc_t hc, mips_r1_t r1, mips_r2_t r2, int im)
{
    mips_code_t		c;
    c.is.b = _u16(im);
    c.rt.b = r2;
    c.rs.b = r1;
    c.hc.b = hc;
    _jit_I(c.op);
}

__jit_inline void
mips_hrxs(jit_state_t _jit, mips_hc_t hc, jit_gpr_t rs, mips_r2_t r2, int im)
{
    mips_code_t		c;
    c.is.b = _u16(im);
    c.rt.b = r2;
    c.rs.b = rs;
    c.hc.b = hc;
    _jit_I(c.op);
}

__jit_inline void
mips_hxrr_(jit_state_t _jit, mips_hc_t hc, mips_r1_t r1,
	   jit_gpr_t rt, jit_gpr_t rd)
{
    mips_code_t		c;
    c.cc.b = 0;
    c.rd.b = rd;
    c.rt.b = rt;
    c.rs.b = r1;
    c.hc.b = hc;
    _jit_I(c.op);
}

__jit_inline void
mips_hxrf_(jit_state_t _jit, mips_hc_t hc, mips_r1_t r1,
	   jit_gpr_t rt, jit_fpr_t fs)
{
    mips_code_t		c;
    c.cc.b = 0;
    c.fs.b = fs;
    c.rt.b = rt;
    c.rs.b = r1;
    c.hc.b = hc;
    _jit_I(c.op);
}

__jit_inline void
mips_hxrs(jit_state_t _jit, mips_hc_t hc, mips_r1_t r1, jit_gpr_t rt, int impl)
{
    mips_code_t		c;
    c.is.b = _u16(impl);
    c.rt.b = rt;
    c.rs.b = r1;
    c.hc.b = hc;
    _jit_I(c.op);
}

__jit_inline void
mips_c_cond_fmt(jit_state_t _jit, mips_fmt_t fm,
		jit_fpr_t ft, jit_fpr_t fs, mips_cond_t cc)
{
    mips_code_t		c;
    c.cc.b = cc;
    c.fs.b = fs;
    c.ft.b = ft;
    c.fm.b = fm;
    c.hc.b = MIPS_COP1;
    _jit_I(c.op);
}

__jit_inline void
mips_mov_fmt(jit_state_t _jit, mips_fmt_t fm,
	     jit_gpr_t rt, jit_fpr_t fs, jit_fpr_t fd, mips_tc_t tc)
{
    mips_code_t		c;
    c.tc.b = tc;
    c.fd.b = fd;
    c.fs.b = fs;
    c.rt.b = rt;
    c.fm.b = fm;
    c.hc.b = MIPS_COP1;
    _jit_I(c.op);
}

__jit_inline void
mips_rirt(jit_state_t _jit, jit_gpr_t rs, int im, jit_gpr_t rd, mips_tc_t tc)
{
    mips_code_t		c;
    c.op = 0;
    c.tc.b = tc;
    c.rd.b = rd;
    c.rt.b = im;
    c.rs.b = rs;
    _jit_I(c.op);
}

__jit_inline void
mips_fp1x(jit_state_t _jit, mips_fmt_t fm,
	  jit_fpr_t fs, int im, jit_fpr_t fd, mips_fc_t tc)
{
    mips_code_t		c;
    c.tc.b = tc;
    c.fd.b = fd;
    c.fs.b = fs;
    c.ft.b = im;
    c.fm.b = fm;
    c.hc.b = MIPS_COP1;
    _jit_I(c.op);
}

__jit_inline void
mips_ext(jit_state_t _jit, jit_gpr_t rs, jit_gpr_t rt,
	 int size, int pos, mips_tc_t tc)
{
    mips_code_t		c;
    c.tc.b = tc;
    c.ic.b = _u5(pos);
    c.rd.b = _u5(size);
    c.rt.b = rt;
    c.rs.b = rs;
    c.hc.b = MIPS_SPECIAL3;
    _jit_I(c.op);
}

__jit_inline void
mips_hrii(jit_state_t _jit, mips_hc_t hc, jit_gpr_t rs, int ic, int is)
{
    mips_code_t		c;
    c.is.b = _u16(is);
    c.rt.b = _u5(ic);
    c.rs.b = rs;
    c.hc.b = hc;
    _jit_I(c.op);
}

__jit_inline void
mips_hrri_t(jit_state_t _jit, mips_hc_t hc, jit_gpr_t rs, jit_gpr_t rt,
	    int ic, mips_tc_t tc)
{
    mips_code_t		c;
    c.tc.b = tc;
    c.ic.b = 0;
    c.rd.b = _u5(ic);
    c.rt.b = rt;
    c.rs.b = rs;
    c.hc.b = hc;
    _jit_I(c.op);
}

__jit_inline void
mips_trap_cond(jit_state_t _jit, jit_gpr_t rs, jit_gpr_t rt, mips_tc_t tc)
{
    mips_code_t		c;
    c.op = 0;
    c.tc.b = tc;
    c.rt.b = rt;
    c.rs.b = rs;
    c.hc.b = MIPS_SPECIAL;
    _jit_I(c.op);
}

__jit_inline void
mips_tlb(jit_state_t _jit, mips_tc_t tc)
{
    mips_code_t		c;
    c.tc.b = tc;
    c.br.b = 1 << 19;
    c.hc.b = MIPS_COP0;
    _jit_I(c.op);
}

#define _ABS_S(fd,fs)		mips_fp1(_jit,MIPS_fmt_S,fs,fd,MIPS_ABS_fmt)
#define _ABS_D(fd,fs)		mips_fp1(_jit,MIPS_fmt_D,fs,fd,MIPS_ABS_fmt)
#define _ABS_PS(fd,fs)		mips_fp1(_jit,MIPS_fmt_PS,fs,fd,MIPS_ABS_fmt)
#define _ADD(rd,rs,rt)		mips_rrr_t(_jit,rs,rt,rd,MIPS_ADD)
#define _ADD_S(fd,fs,ft)	mips_fp2(_jit,MIPS_fmt_S,ft,fs,fd,MIPS_ADD_fmt)
#define _ADD_D(fd,fs,ft)	mips_fp2(_jit,MIPS_fmt_D,ft,fs,fd,MIPS_ADD_fmt)
#define _ADD_PS(fd,fs,ft)	mips_fp2(_jit,MIPS_fmt_PS,ft,fs,fd,MIPS_ADD_fmt)
#define _ADDI(rt,rs,im)		mipshrri(_jit,MIPS_ADDI,rs,rt,im)
#define _ADDIU(rt,rs,im)	mipshrri(_jit,MIPS_ADDIU,rs,rt,im)
#define _ADDU(rd,rs,rt)		mips_rrr_t(_jit,rs,rt,rd,MIPS_ADDU)
#define _ALNV_PS(fd,fs,ft,rs)	mips_rffft(_jit,rs,ft,fs,fd,MIPS_ALNV_PS)
#define _AND(rd,rs,rt)		mips_rrr_t(_jit,rs,rt,rd,MIPS_AND)
#define _ANDI(rt,rs,im)		mipshrri(_jit,MIPS_ANDI,rs,rt,im)
#define _B(im)			mipshrri(_jit, MIPS_BEQ,_ZERO,_ZERO,im)
#define _BAL(im)		mipshrri(_jit,REGIMM,_ZERO,(jit_gpr_t)MIPS_BGZALL,im>>2)
#define _BC1F(im)		mips_hxxs(_jit,MIPS_COP1,MIPS_BC,MIPS_BCF,im)
#define _BC1FL(im)		mips_hxxs(_jit,MIPS_COP1,MIPS_BC,MIPS_BCFL,im)
#define _BC1T(im)		mips_hxxs(_jit,MIPS_COP1,MIPS_BC,MIPS_BCT,im)
#define _BC1TL(im)		mips_hxxs(_jit,MIPS_COP1,MIPS_BC,MIPS_BCTL,im)
#define _BC2F(im)		mips_hxxs(_jit,MIPS_COP2,MIPS_BC,MIPS_BCF,im)
#define _BC2FL(im)		mips_hxxs(_jit,MIPS_COP2,MIPS_BC,MIPS_BCFL,im)
#define _BC2T(im)		mips_hxxs(_jit,MIPS_COP2,MIPS_BC,MIPS_BCT,im)
#define _BC2TL(im)		mips_hxxs(_jit,MIPS_COP2,MIPS_BC,MIPS_BCTL,im)
#define _BEQ(rs,rt,im)		mipshrri(_jit,MIPS_BEQ,rs,rt,im)
#define _BEQL(rs,rt,im)		mipshrri(_jit,MIPS_BEQL,rs,rt,im)
#define _BGEZ(rs,im)		mips_hrxs(_jit,MIPS_REGIMM,rs,MIPS_BGEZ,im)
#define _BGEZAL(rs,im)		mips_hrxs(_jit,MIPS_REGIMM,rs,MIPS_BGEZAL,im)
#define _BGEZALL(rs,im)		mips_hrxs(_jit,MIPS_REGIMM,rs,MIPS_BGEZALL,im)
#define _BGEZL(rs,im)		mips_hrxs(_jit,MIPS_REGIMM,rs,MIPS_BGEZL,im)
#define _BGTZ(rs,im)		mipshrri(_jit,MIPS_BGTZ,rs,_ZERO,im)
#define _BGTZL(rs,im)		mipshrri(_jit,MIPS_BGTZL,rs,_ZERO,im)
#define _BLEZ(rs,im)		mipshrri(_jit,MIPS_BLEZ,rs,_ZERO,im)
#define _BLEZL(rs,im)		mipshrri(_jit,MIPS_BLEZL,rs,_ZERO,im)
#define _BLTZ(rs,im)		mips_hrxs(_jit,MIPS_REGIMM,rs,MIPS_BLTZ,im)
#define _BLTZAL(rs,im)		mips_hrxs(_jit,MIPS_REGIMM,rs,MIPS_BLTZAL,im)
#define _BLTZALL(rs,im)		mips_hrxs(_jit,MIPS_REGIMM,rs,MIPS_BLTZALL,im)
#define _BLTZL(rs,im)		mips_hrxs(_jit,MIPS_REGIMM,rs,MIPS_BLTZL,im)
#define _BNE(rs,rt,im)		mipshrri(_jit,MIPS_BNE,rs,rt,im)
#define _BNEL(rs,rt,im)		mipshrri(_jit,MIPS_BNEL,rs,rt,im)
__jit_inline void
mips_break(jit_state_t _jit, int code)
{
    mips_code_t		c;
    c.tc.b = MIPS_BREAK;
    c.br.b = _u20(code);
    c.hc.b = 0;
    _jit_I(c.op);
}
#define _BREAK(code)		mips_break(_jit, code)
#define _C_F_S(fs,ft)		mips_c_cond_fmt(_jit,MIPS_fmt_S,ft,fs,MIPS_cond_F)
#define _C_F_D(fs,ft)		mips_c_cond_fmt(_jit,MIPS_fmt_D,ft,fs,MIPS_cond_F)
#define _C_F_PS(fs,ft)		mips_c_cond_fmt(_jit,MIPS_fmt_PS,ft,fs,MIPS_cond_F)
#define _C_UN_S(fs,ft)		mips_c_cond_fmt(_jit,MIPS_fmt_S,ft,fs,MIPS_cond_UN)
#define _C_UN_D(fs,ft)		mips_c_cond_fmt(_jit,MIPS_fmt_D,ft,fs,MIPS_cond_UN)
#define _C_UN_PS(fs,ft)		mips_c_cond_fmt(_jit,MIPS_fmt_PS,ft,fs,MIPS_cond_UN)
#define _C_EQ_S(fs,ft)		mips_c_cond_fmt(_jit,MIPS_fmt_S,ft,fs,MIPS_cond_EQ)
#define _C_EQ_D(fs,ft)		mips_c_cond_fmt(_jit,MIPS_fmt_D,ft,fs,MIPS_cond_EQ)
#define _C_EQ_PS(fs,ft)		mips_c_cond_fmt(_jit,MIPS_fmt_PS,ft,fs,MIPS_cond_EQ)
#define _C_UEQ_S(fs,ft)		mips_c_cond_fmt(_jit,MIPS_fmt_S,ft,fs,MIPS_cond_UEQ)
#define _C_UEQ_D(fs,ft)		mips_c_cond_fmt(_jit,MIPS_fmt_D,ft,fs,MIPS_cond_UEQ)
#define _C_UEQ_PS(fs,ft)	mips_c_cond_fmt(_jit,MIPS_fmt_PS,ft,fs,MIPS_cond_UEQ)
#define _C_OLT_S(fs,ft)		mips_c_cond_fmt(_jit,MIPS_fmt_S,ft,fs,MIPS_cond_OLT)
#define _C_OLT_D(fs,ft)		mips_c_cond_fmt(_jit,MIPS_fmt_D,ft,fs,MIPS_cond_OLT)
#define _C_OLT_PS(fs,ft)	mips_c_cond_fmt(_jit,MIPS_fmt_PS,ft,fs,MIPS_cond_OLT)
#define _C_ULT_S(fs,ft)		mips_c_cond_fmt(_jit,MIPS_fmt_S,ft,fs,MIPS_cond_ULT)
#define _C_ULT_D(fs,ft)		mips_c_cond_fmt(_jit,MIPS_fmt_D,ft,fs,MIPS_cond_ULT)
#define _C_ULT_PS(fs,ft)	mips_c_cond_fmt(_jit,MIPS_fmt_PS,ft,fs,MIPS_cond_ULT)
#define _C_OLE_S(fs,ft)		mips_c_cond_fmt(_jit,MIPS_fmt_S,ft,fs,MIPS_cond_OLE)
#define _C_OLE_D(fs,ft)		mips_c_cond_fmt(_jit,MIPS_fmt_D,ft,fs,MIPS_cond_OLE)
#define _C_OLE_PS(fs,ft)	mips_c_cond_fmt(_jit,MIPS_fmt_PS,ft,fs,MIPS_cond_OLE)
#define _C_ULE_S(fs,ft)		mips_c_cond_fmt(_jit,MIPS_fmt_S,ft,fs,MIPS_cond_ULE)
#define _C_ULE_D(fs,ft)		mips_c_cond_fmt(_jit,MIPS_fmt_D,ft,fs,MIPS_cond_ULE)
#define _C_ULE_PS(fs,ft)	mips_c_cond_fmt(_jit,MIPS_fmt_PS,ft,fs,MIPS_cond_ULE)
#define _C_SF_S(fs,ft)		mips_c_cond_fmt(_jit,MIPS_fmt_S,ft,fs,MIPS_cond_SF)
#define _C_SF_D(fs,ft)		mips_c_cond_fmt(_jit,MIPS_fmt_D,ft,fs,MIPS_cond_SF)
#define _C_SF_PS(fs,ft)		mips_c_cond_fmt(_jit,MIPS_fmt_PS,ft,fs,MIPS_cond_SF)
#define _C_NGLE_S(fs,ft)	mips_c_cond_fmt(_jit,MIPS_fmt_S,ft,fs,MIPS_cond_NGLE)
#define _C_NGLE_D(fs,ft)	mips_c_cond_fmt(_jit,MIPS_fmt_D,ft,fs,MIPS_cond_NGLE)
#define _C_NGLE_PS(fs,ft)	mips_c_cond_fmt(_jit,MIPS_fmt_PS,ft,fs,MIPS_cond_NGLE)
#define _C_SEQ_S(fs,ft)		mips_c_cond_fmt(_jit,MIPS_fmt_S,ft,fs,MIPS_cond_SEQ)
#define _C_SEQ_D(fs,ft)		mips_c_cond_fmt(_jit,MIPS_fmt_D,ft,fs,MIPS_cond_SEQ)
#define _C_SEQ_PS(fs,ft)	mips_c_cond_fmt(_jit,MIPS_fmt_PS,ft,fs,MIPS_cond_SEQ)
#define _C_NGL_S(fs,ft)		mips_c_cond_fmt(_jit,MIPS_fmt_S,ft,fs,MIPS_cond_NGL)
#define _C_NGL_D(fs,ft)		mips_c_cond_fmt(_jit,MIPS_fmt_D,ft,fs,MIPS_cond_NGL)
#define _C_NGL_PS(fs,ft)	mips_c_cond_fmt(_jit,MIPS_fmt_PS,ft,fs,MIPS_cond_NGL)
#define _C_NLT_S(fs,ft)		mips_c_cond_fmt(_jit,MIPS_fmt_S,ft,fs,MIPS_cond_NLT)
#define _C_NLT_D(fs,ft)		mips_c_cond_fmt(_jit,MIPS_fmt_D,ft,fs,MIPS_cond_NLT)
#define _C_NLT_PS(fs,ft)	mips_c_cond_fmt(_jit,MIPS_fmt_PS,ft,fs,MIPS_cond_NLT)
#define _C_NGE_S(fs,ft)		mips_c_cond_fmt(_jit,MIPS_fmt_S,ft,fs,MIPS_cond_NGE)
#define _C_NGE_D(fs,ft)		mips_c_cond_fmt(_jit,MIPS_fmt_D,ft,fs,MIPS_cond_NGE)
#define _C_NGE_PS(fs,ft)	mips_c_cond_fmt(_jit,MIPS_fmt_PS,ft,fs,MIPS_cond_NGE)
#define _C_NLE_S(fs,ft)		mips_c_cond_fmt(_jit,MIPS_fmt_S,ft,fs,MIPS_cond_NLE)
#define _C_NLE_D(fs,ft)		mips_c_cond_fmt(_jit,MIPS_fmt_D,ft,fs,MIPS_cond_NLE)
#define _C_NLE_PS(fs,ft)	mips_c_cond_fmt(_jit,MIPS_fmt_PS,ft,fs,MIPS_cond_NLE)
#define _C_UGT_S(fs,ft)		mips_c_cond_fmt(_jit,MIPS_fmt_S,ft,fs,MIPS_cond_UGT)
#define _C_UGT_D(fs,ft)		mips_c_cond_fmt(_jit,MIPS_fmt_D,ft,fs,MIPS_cond_UGT)
#define _C_UGT_PS(fs,ft)	mips_c_cond_fmt(_jit,MIPS_fmt_PS,ft,fs,MIPS_cond_UGT)
#define _CACHE(op,offset,base)	mips_cache(_jit,op,base,offset)
__jit_inline void
mips_cache(jit_state_t _jit, jit_gpr_t base, int op, int offset)
{
    mips_code_t		c;
    c.is.b = _u16(offset);
    c.rt.b = _u5(op);
    c.rs.b = _u16(base);
    c.hc.b = MIPS_CACHE;
    _jit_I(c.op);
}
#define _CEIL_L_S(fd,fs)	mips_fp1(_jit,MIPS_fmt_S,fs,fd,MIPS_CEIL_fmt_L)
#define _CEIL_L_D(fd,fs)	mips_fp1(_jit,MIPS_fmt_D,fs,fd,MIPS_CEIL_fmt_L)
#define _CEIL_W_S(fd,fs)	mips_fp1(_jit,MIPS_fmt_S,fs,fd,MIPS_CEIL_fmt_W)
#define _CEIL_W_D(fd,fs)	mips_fp1(_jit,MIPS_fmt_D,fs,fd,MIPS_CEIL_fmt_W)
#define _CFC1(rt,fs)		mips_cfc1(_jit,rt,fs)
__jit_inline void
mips_cfc1(jit_state_t _jit, jit_gpr_t rt, jit_fpr_t fs)
{
    mips_code_t		c;
    c.cc.b = 0;
    c.fs.b = fs;
    c.rt.b = rt;
    c.rs.b = MIPS_CF;
    c.hc.b = MIPS_COP1;
    _jit_I(c.op);
}
#define _CFC2(rt,impl)		mips_cfc2(_jit,rt,impl)
__jit_inline void
mips_cfc2(jit_state_t _jit, jit_gpr_t rt, int impl)
{
    mips_code_t		c;
    c.is.b = _u16(impl);
    c.rs.b = MIPS_CF;
    c.hc.b = MIPS_COP2;
    _jit_I(c.op);
}
#define _CLO(rd,rs)		mipshrrr_t(_jit,MIPS_SPECIAL2,rs,rd,rd,MIPS_CLO)
#define _COP2(func)		mips_cop2(_jit,func)
__jit_inline void
mips_cop2(jit_state_t _jit, int func)
{
    mips_code_t		c;
    c.ii.b = (1 << 25) | _u25(func);
    c.hc.b = MIPS_COP2;
    _jit_I(c.op);
}
#define _CLZ(rd,rs)		mipshrrr_t(_jit,MIPS_SPECIAL2,rs,rd,rd,MIPS_CLZ)
#define _CTC1(rt,fs)		mips_ctc1(_jit,rt,fs)
__jit_inline void
mips_ctc1(jit_state_t _jit, jit_gpr_t rt, jit_fpr_t fs)
{
    mips_code_t		c;
    c.cc.b = 0;
    c.fs.b = fs;
    c.rt.b = rt;
    c.rs.b = MIPS_CT;
    c.hc.b = MIPS_COP1;
    _jit_I(c.op);
}
#define _CTC2(rt,imp)		mips_ctc2(rt,imp)
__jit_inline void
mips_ctc2(jit_state_t _jit, jit_gpr_t rt, int imp)
{
    mips_code_t		c;
    c.is.b = imp;
    c.rt.b = rt;
    c.hc.b = MIPS_COP2;
    _jit_I(c.op);
}
#define _CVT_D_S(fd,fs)		mips_fp1(_jit,MIPS_fmt_S,fs,fd,MIPS_CVT_fmt_D)
#define _CVT_D_W(fd,fs)		mips_fp1(_jit,MIPS_fmt_W,fs,fd,MIPS_CVT_fmt_D)
#define _CVT_D_L(fd,fs)		mips_fp1(_jit,MIPS_fmt_L,fs,fd,MIPS_CVT_fmt_D)
#define _CVT_L_S(fd,fs)		mips_fp1(_jit,MIPS_fmt_S,fs,fd,MIPS_CVT_fmt_L)
#define _CVT_L_D(fd,fs)		mips_fp1(_jit,MIPS_fmt_D,fs,fd,MIPS_CVT_fmt_L)
#define _CVT_PS_S(fd,fs)	mips_fp1(_jit,MIPS_fmt_S,fs,fd,MIPS_CVT_fmt_PS)
#define _CVT_S_D(fd,fs)		mips_fp1(_jit,MIPS_fmt_D,fs,fd,MIPS_CVT_fmt_S)
#define _CVT_S_W(fd,fs)		mips_fp1(_jit,MIPS_fmt_W,fs,fd,MIPS_CVT_fmt_S)
#define _CVT_S_L(fd,fs)		mips_fp1(_jit,MIPS_fmt_L,fs,fd,MIPS_CVT_fmt_S)
#define _CVT_S_PL(fd,fs)	mips_fp1(_jit,MIPS_fmt_PS,fs,fd,MIPS_CVT_fmt_S_PL)
#define _CVT_S_PU(fd,fs)	mips_fp1(_jit,MIPS_fmt_PS,fs,fd,MIPS_CVT_fmt_S_PU)
#define _CVT_W_S(fd,fs)		mips_fp1(_jit,MIPS_fmt_S,fs,fd,MIPS_CVT_fmt_W)
#define _CVT_W_D(fd,fs)		mips_fp1(_jit,MIPS_fmt_D,fs,fd,MIPS_CVT_fmt_W)
#define _DADD(rd,rs,rt)		mips_rrr_t(_jit,rs,rt,rd,MIPS_DADD)
#define _DADDI(rt,rs,im)	mipshrri(_jit,MIPS_DADDI,rs,rt,im)
#define _DADDU(rd,rs,rt)	mips_rrr_t(_jit,rs,rt,rd,MIPS_DADDU)
#define _DADDIU(rt,rs,im)	mipshrri(_jit,MIPS_DADDIU,rs,rt,im)
#define _DCLO(rd,rs)		mipshrrr_t(_jit,MIPS_SPECIAL2,rs,rd,rd,MIPS_DCLO)
#define _DCLZ(rd,rs)		mipshrrr_t(_jit,MIPS_SPECIAL2,rs,rd,rd,MIPS_DCLZ)
#define _DDIV(rs,rt)		mips_rrr_t(_jit,rs,rt,JIT_RZERO,MIPS_DDIV)
#define _DDIVU(rs,rt)		mips_rrr_t(_jit,rs,rt,JIT_RZERO,MIPS_DDIVU)
#define _DERET()		mips_return(_jit,MIPS_DERET)
__jit_inline void
mips_return(jit_state_t _jit, mips_tc_t tc)
{
    mips_code_t		c;
    c.tc.b = tc;
    c.br.b = 1 << 19;
    c.hc.b = MIPS_COP0;
    _jit_I(c.op);
}
#define _DEXT(rt,rs,pos,size)	mips_ext(rs,rt,pos,size-1,MIPS_DEXT)
#define _DEXTM(rt,rs,pos,size)	mips_ext(rs,rt,pos,size-1-32,MIPS_DEXTM)
#define _DEXTU(rt,rs,pos,size)	mips_ext(rs,rt,pos-32,size-1,MIPS_DEXTU)
#define _DI(rt)			mips_interrupt(_jit,rt,0)
__jit_inline void
mips_interrupt(jit_state_t _jit, jit_gpr_t rt, int enable)
{
    mips_code_t		c;
    c.cc.b = (!!enable) << 5;
    c.rd.b = 12;
    c.rt.b = rt;
    c.rs.b = MIPS_MFMC0;
    c.hc.b = MIPS_COP0;
    _jit_I(c.op);
}
#define _DINS(rt,rs,pos,size)	mipshrriit(_jit,MIPS_SPECIAL3,rs,rt,pos,pos+size-1,MIPS_DINS)
#define _DINSM(rt,rs,pos,size)	mipshrriit(_jit,MIPS_SPECIAL3,rs,rt,pos,pos+size-33,MIPS_DINSM)
#define _DINSU(rt,rs,pos,size)	mipshrriit(_jit,MIPS_SPECIAL3,rs,rt,pos-32,pos+size-33,MIPS_DINSU)
#define _DIV(rs,rt)		mips_rrr_t(_jit,rs,rt,JIT_RZERO,MIPS_DIV)
#define _DIV_S(fd,fs,ft)	mips_fp2(_jit,MIPS_fmt_S,ft,fs,fd,MIPS_DIV_fmt)
#define _DIV_D(fd,fs,ft)	mips_fp2(_jit,MIPS_fmt_D,ft,fs,fd,MIPS_DIV_fmt)
#define _DIVU(rs,rt)		mips_rrr_t(_jit,rs,rt,JIT_RZERO,MIPS_DIVU)
#define _DMFC0(rt,rd)		mips_hxrr_(_jit,MIPS_COP0,MIPS_DMF,rt,rd)
#define _DMFC1(rt,fs)		mips_hxrf_(_jit,MIPS_COP1,MIPS_DMF,rt,fs)
#define _DMFC2(rt,impl)		mips_hxrs(_jit,MIPS_COP2,MIPS_DMF,rt,impl)
#define _DMTC0(rt,rd)		mips_hxrr_(_jit,MIPS_COP0,MIPS_DMT,rt,rd)
#define _DMTC1(rt,fs)		mips_hxrf_(_jit,MIPS_COP1,MIPS_DMT,rt,fs)
#define _DMTC2(rt,impl)		mips_hxrs(_jit,MIPS_COP2,MIPS_DMT,rt,impl)
#define _DMULT(rs, rt)		mips_rrr_t(_jit,rs,rt,JIT_RZERO,MIPS_DMULT)
#define _DMULTU(rs, rt)		mips_rrr_t(_jit,rs,rt,JIT_RZERO,MIPS_DMULTU)
#define _DROTR(rd,rt,sa)	mips_hirrit(_jit,MIPS_SPECIAL,1,rt,rd,sa,MIPS_DSRL)
#define _DROTR32(rd,rt,sa)	mips_hirrit(_jit,MIPS_SPECIAL,1,rt,rd,sa-32,MIPS_DSRL32)
#define _DROTRV(rd,rt,rs)	mips_hrrrit(_jit,MIPS_SPECIAL,rs,rt,rd,1,MIPS_DSRLV)
#define _DSBH(rd,rt)		mips_hirrit(_jit,MIPS_SPECIAL3,0,rt,rd,MIPS_DBSH,MIPS_DBSHFL)
#define _DSHD(rd,rt)		mips_hirrit(_jit,MIPS_SPECIAL3,0,rt,rd,MIPS_DBHD,MIPS_DBSHFL)
#define _DSLL(rd,rt,sa)		mips__rrit(_jit,rt,rd,sa,MIPS_DSLL)
#define _DSLL32(rd,rt,sa)	mips__rrit(_jit,rt,rd,sa-32,MIPS_DSLL32)
#define _DSLLV(rd,rt,rs)	mips_rrr_t(_jit,rs,rt,rd,MIPS_DSLLV)
#define _DSRA(rd,rt,sa)		mips__rrit(_jit,rt,rd,sa,MIPS_DSRA)
#define _DSRA32(rd,rt,sa)	mips__rrit(_jit,rt,rd,sa-32,MIPS_DSRA32)
#define _DSRAV(rd,rt,rs)	mips_rrr_t(_jit,rs,rt,rd,MIPS_DSRAV)
#define _DSRL(rd,rt,sa)		mips__rrit(_jit,rt,rd,sa,MIPS_DSRL)
#define _DSRL32(rd,rt,sa)	mips__rrit(_jit,rt,rd,sa-32,MIPS_DSRL32)
#define _DSRLV(rd,rt,rs)	mips_rrr_t(_jit,rs,rt,rd,MIPS_DSRLV)
#define _DSUB(rd,rs,rt)		mips_rrr_t(_jit,rs,rt,rd,MIPS_DSUB)
#define _DSUBU(rd,rs,rt)	mips_rrr_t(_jit,rs,rt,rd,MIPS_DSUBU)
#define _EHB()			_SLL(JIT_RZERO,JIT_RZERO,3)
#define _EI()			mips_interrupt(_jit,rt,1)
#define _ERET()			mips_return(_jit,MIPS_ERET)
#define _EXT(rt,rs,pos,size)	mips_ext(rs,rt,pos,size-1,MIPS_EXT)
#define _FLOOR_L_S(fd,fs)	mips_fp1(_jit,MIPS_fmt_S,fs,fd,MIPS_FLOOR_fmt_L)
#define _FLOOR_L_D(fd,fs)	mips_fp1(_jit,MIPS_fmt_D,fs,fd,MIPS_FLOOR_fmt_L)
#define _FLOOR_W_S(fd,fs)	mips_fp1(_jit,MIPS_fmt_S,fs,fd,MIPS_FLOOR_fmt_W)
#define _FLOOR_W_D(fd,fs)	mips_fp1(_jit,MIPS_fmt_D,fs,fd,MIPS_FLOOR_fmt_W)
#define _INS(rt,rs,pos,size)	mipshrriit(_jit,MIPS_SPECIAL3,rs,rt,pos,pos+size-1,MIPS_INS)
#define _J(target)		mipshi(_jit,MIPS_J,target)
#define _JAL(target)		mipshi(_jit,MIPS_JAL,target)
#define _JALR(rs)		mips_r_rit(_jit,rs,_RA,0,MIPS_JALR)
#define _JALR_HB(rs)		mips_r_rit(_jit,rs,_RA,1<<4,MIPS_JALR)
#define _JALX(target)		mipshi(_jit,MIPS_JALX,target)
#define _JR(rs)			mips_r_it(_jit,rs,0,MIPS_JR)
#define _JR_HB(rs)		mips_r_it(_jit,rs,1<<4,MIPS_JR)
#define _LB(rt,offset,base)	mipshrri(_jit,MIPS_LB,base,rt,offset)
#define _LBU(rt,offset,base)	mipshrri(_jit,MIPS_LBU,base,rt,offset)
#define _LD(rt,offset,base)	mipshrri(_jit,MIPS_LD,base,rt,offset)
#define _LDC1(ft,offset,base)	mipshrfi(_jit,MIPS_LDC1,base,ft,offset)
#define _LDC2(ft,offset,base)	mipshrfi(_jit,MIPS_LDC2,base,ft,offset)
#define _LDL(rt,offset,base)	mipshrri(_jit,MIPS_LDL,base,rt,offset)
#define _LDR(rt,offset,base)	mipshrri(_jit,MIPS_LDR,base,rt,offset)
#define _LDXC1(fd,index,base)	mips_rr_f(_jit,base,index,fd,MIPS_LDXC1)
#define _LH(rt,offset,base)	mipshrri(_jit,MIPS_LH,base,rt,offset)
#define _LHU(rt,offset,base)	mipshrri(_jit,MIPS_LHU,base,rt,offset)
#define _LL(rt,offset,base)	mipshrri(_jit,MIPS_LL,base,rt,offset)
#define _LLD(rt,offset,base)	mipshrri(_jit,MIPS_LLD,base,rt,offset)
#define _LUI(rt,im)		mipsh_ri(_jit,MIPS_LUI,rt,im)
#define _LUXC1(fd,index,base)	mips_rr_f(_jit,base,index,fd,MIPS_LUXC1)
#define _LW(rt,offset,base)	mipshrri(_jit,MIPS_LW,base,rt,offset)
#define _LWC1(ft,offset,base)	mipshrfi(_jit,MIPS_LWC1,base,ft,offset)
#define _LWC2(ft,offset,base)	mipshrfi(_jit,MIPS_LWC2,base,ft,offset)
#define _LWL(rt,offset,base)	mipshrri(_jit,MIPS_LWL,base,rt,offset)
#define _LWR(rt,offset,base)	mipshrri(_jit,MIPS_LWR,base,rt,offset)
#define _LWU(rt,offset,base)	mipshrri(_jit,MIPS_LWU,base,rt,offset)
#define _LWXC1(fd,index,base)	mips_rr_f(_jit,base,index,fd,MIPS_LWXC1)
#define _MADD(rs,rt)		mipshrr__t(_jit,MIPS_SPECIAL2,rs,rt,MIPS_MADD)
#define _MADD_S(fd,fr,fs,ft)	mips_fp2x(_jit,fr,ft,fs,fd,MIPS_MADD_fmt_S)
#define _MADD_D(fd,fr,fs,ft)	mips_fp2x(_jit,fr,ft,fs,fd,MIPS_MADD_fmt_D)
#define _MADD_PS(fd,fr,fs,ft)	mips_fp2x(_jit,fr,ft,fs,fd,MIPS_MADD_fmt_PS)
#define _MADDU(rs,rt)		mipshrr__t(_jit,MIPS_SPECIAL2,rs,rt,MIPS_MADDU)
#define _MFC0(rt,rd)		mips_hxrr_(_jit,MIPS_COP0,MIPS_MF,rt,rd)
#define _MFC1(rt,fs)		mips_hxrf_(_jit,MIPS_COP1,MIPS_MF,rt,fs)
#define _MFC2(rt,impl)		mips_hxrs(_jit,MIPS_COP2,MIPS_MF,rt,impl)
#define _MFHC1(rt,fs)		mips_hxrf_(_jit,MIPS_COP1,MIPS_MFH,rt,fs)
#define _MFHC2(rt,impl)		mips_hxrs(_jit,MIPS_COP2,MIPS_MFH,rt,impl)
#define _MFHI(rd)		mips___r_t(_jit, rd, MIPS_MFHI)
#define _MFLO(rd)		mips___r_t(_jit, rd, MIPS_MFLO)
#define _MOV_S(fd,fs)		mips_fp1(_jit,MIPS_fmt_S,fs,fd,MIPS_MOV_fmt)
#define _MOV_D(fd,fs)		mips_fp1(_jit,MIPS_fmt_D,fs,fd,MIPS_MOV_fmt)
#define _MOV_PS(fd,fs)		mips_fp1(_jit,MIPS_fmt_PS,fs,fd,MIPS_MOV_fmt)
#define _MOVF(rd,rs)		mips_rirt(_jit,rs,0,rd,MIPS_MOVFT)
#define _MOVF_S(fd,fs)		mips_fp1x(_jit,MIPS_fmt_S,0,fs,fd,MIPS_MOVFT)
#define _MOVF_D(fd,fs)		mips_fp1x(_jit,MIPS_fmt_D,0,fs,fd,MIPS_MOVFT)
#define _MOVF_PS(fd,fs)		mips_fp1x(_jit,MIPS_fmt_PS,0,fs,fd,MIPS_MOVFT)
#define _MOVN(rd,rs,rt)		mips_rrr_t(_jit,rs,rt,rd,MIPS_MOVN)
#define _MOVN_S(fd,fs,rt)	mips_mov_fnt(_jit,MIPS_fmt_S,rt,fs,fd,MIPS_MOVN)
#define _MOVN_D(fd,fs,rt)	mips_mov_fnt(_jit,MIPS_fmt_D,rt,fs,fd,MIPS_MOVN)
#define _MOVN_PS(fd,fs,rt)	mips_mov_fnt(_jit,MIPS_fmt_PS,rt,fs,fd,MIPS_MOVN)
#define _MOVT(rd,rs)		mips_rirt(_jit,rs,1,rd,MIPS_MOVFT)
#define _MOVT_S(fd,fs)		mips_fp1x(_jit,MIPS_fmt_S,1,fs,fd,MIPS_MOVFT)
#define _MOVT_D(fd,fs)		mips_fp1x(_jit,MIPS_fmt_D,1,fs,fd,MIPS_MOVFT)
#define _MOVT_PS(fd,fs)		mips_fp1x(_jit,MIPS_fmt_PS,1,fs,fd,MIPS_MOVFT)
#define _MOVZ(rd,rs,rt)		mips_rrr_t(_jit,rs,rt,rd,MIPS_MOVZ)
#define _MOVZ_S(fd,fs,rt)	mips_mov_fnt(_jit,MIPS_fmt_S,rt,fs,fd,MIPS_MOVZ)
#define _MOVZ_D(fd,fs,rt)	mips_mov_fnt(_jit,MIPS_fmt_D,rt,fs,fd,MIPS_MOVZ)
#define _MOVZ_PS(fd,fs,rt)	mips_mov_fnt(_jit,MIPS_fmt_PS,rt,fs,fd,MIPS_MOVZ)
#define _MSUB(rs,rt)		mipshrr__t(_jit,MIPS_SPECIAL2,rs,rt,MIPS_MSUB)
#define _MSUB_S(fd,fr,fs,ft)	mips_fp2x(_jit,fr,ft,fs,fd,MIPS_MSUB_fmt_S)
#define _MSUB_D(fd,fr,fs,ft)	mips_fp2x(_jit,fr,ft,fs,fd,MIPS_MSUB_fmt_D)
#define _MSUB_PS(fd,fr,fs,ft)	mips_fp2x(_jit,fr,ft,fs,fd,MIPS_MSUB_fmt_PS)
#define _MSUBU(rs,rt)		mipshrr__t(_jit,MIPS_SPECIAL2,rs,rt,MIPS_MSUBU)
#define _MTC0(rt,rd)		mips_hxrr_(_jit,MIPS_COP0,MIPS_MT,rt,rd)
#define _MTC1(rt,fs)		mips_hxrf_(_jit,MIPS_COP1,MIPS_MT,rt,fs)
#define _MTC2(rt,impl)		mips_hxrs(_jit,MIPS_COP2,MIPS_MT,rt,impl)
#define _MTHC1(rt,fs)		mips_hxrf_(_jit,MIPS_COP1,MIPS_MTH,rt,fs)
#define _MTCH2(rt,impl)		mips_hxrs(_jit,MIPS_COP2,MIPS_MTH,rt,impl)
#define _MTHI(rs)		mips_r___t(_jit, rd, MIPS_MTHI)
#define _MTLO(rs)		mips_r___t(_jit, rd, MIPS_MTLO)
#define _MUL(rd,rs,rt)		mipshrrr_t(_jit,MIPS_SPECIAL2,rs,rt,rd,MIPS_MUL)
#define _MUL_S(fd,fs,ft)	mips_fp2(_jit,MIPS_fmt_S,ft,fs,fd,MIPS_MUL_fmt)
#define _MUL_D(fd,fs,ft)	mips_fp2(_jit,MIPS_fmt_D,ft,fs,fd,MIPS_MUL_fmt)
#define _MUL_PS(fd,fs,ft)	mips_fp2(_jit,MIPS_fmt_PS,ft,fs,fd,MIPS_MUL_fmt)
#define _MULT(rs,rt)		mips_rrr_t(_jit,rs,rt,JIT_RZERO,MIPS_MULT)
#define _MULTU(rs,rt)		mips_rrr_t(_jit,rs,rt,JIT_RZERO,MIPS_MULTU)
#define _NEG_S(fd,fs)		mips_fp1(_jit,MIPS_fmt_S,fs,fd,MIPS_NEG_fmt)
#define _NEG_D(fd,fs)		mips_fp1(_jit,MIPS_fmt_D,fs,fd,MIPS_NEG_fmt)
#define _NEG_PS(fd,fs)		mips_fp1(_jit,MIPS_fmt_PS,fs,fd,MIPS_NEG_fmt)
#define _NMADD_S(fd,fr,fs,ft)	mips_fp2x(_jit,fr,ft,fs,fd,MIPS_NMADD_fmt_S)
#define _NMADD_D(fd,fr,fs,ft)	mips_fp2x(_jit,fr,ft,fs,fd,MIPS_NMADD_fmt_D)
#define _NMADD_PS(fd,fr,fs,ft)	mips_fp2x(_jit,fr,ft,fs,fd,MIPS_NMADD_fmt_PS)
#define _NMSUB_S(fd,fr,fs,ft)	mips_fp2x(_jit,fr,ft,fs,fd,MIPS_NMSUB_fmt_S)
#define _NMSUB_D(fd,fr,fs,ft)	mips_fp2x(_jit,fr,ft,fs,fd,MIPS_NMSUB_fmt_D)
#define _NMSUB_PS(fd,fr,fs,ft)	mips_fp2x(_jit,fr,ft,fs,fd,MIPS_NMSUB_fmt_PS)
#define _NOP()			_jit_I(0)
#define _NOR(rd,rs,rt)		mips_rrr_t(_jit,rs,rt,rd,MIPS_NOR)
#define _OR(rd,rs,rt)		mips_rrr_t(_jit,rs,rt,rd,MIPS_OR)
#define _ORI(rt,rs,im)		mipshrri(_jit,MIPS_ORI,rs,rt,im)
#define _PAUSE()		_jit_I(0x140)
#define _PLL_PS(fd,fs,ft)	mips_fp2(_jit,MIPS_fmt_PS,ft,fs,fd,MIPS_PLL)
#define _PLU_PS(fd,fs,ft)	mips_fp2(_jit,MIPS_fmt_PS,ft,fs,fd,MIPS_PLU)
#define _PREF(hint,offset,base)	mips_hrii(_jit,MIPS_PREF,base,offset)
#define _PREFX(hint,index,base)	mips_hrri_t(_jit,MIPS_COP1X,base,index,hint,MIPS_PREFX)
#define _PUL_PS(fd,fs,ft)	mips_fp2(_jit,MIPS_fmt_PS,ft,fs,fd,MIPS_PUL)
#define _PUU_PS(fd,fs,ft)	mips_fp2(_jit,MIPS_fmt_PS,ft,fs,fd,MIPS_PUU)
/* _RDHWR(...) privileged mode */
/* _RDPGPR(...) privileged mode */
#define _RECIP_S(fd,fs)		mips_fp1(_jit,MIPS_fmt_S,fs,fd,MIPS_RECIP)
#define _RECIP_D(fd,fs)		mips_fp1(_jit,MIPS_fmt_D,fs,fd,MIPS_RECIP)
#define _RECIP_PS(fd,fs)	mips_fp1(_jit,MIPS_fmt_PS,fs,fd,MIPS_RECIP)
#define _ROTR(rd,rt,sa)		mips_hirrit(_jit,MIPS_SPECIAL,1,rt,rd,sa,MIPS_SRL)
#define _ROTRV(rd,rt,rs)	mips_hrrrit(_jit,MIPS_SPECIAL,rs,rt,rd,1,MIPS_SRLV)
#define _ROUND_L_S(fd,fs)	mips_fp1(_jit,MIPS_fmt_S,fs,fd,MIPS_ROUND_fmt_L)
#define _ROUND_L_D(fd,fs)	mips_fp1(_jit,MIPS_fmt_D,fs,fd,MIPS_ROUND_fmt_L)
#define _ROUND_W_S(fd,fs)	mips_fp1(_jit,MIPS_fmt_S,fs,fd,MIPS_ROUND_fmt_W)
#define _ROUND_W_D(fd,fs)	mips_fp1(_jit,MIPS_fmt_D,fs,fd,MIPS_ROUND_fmt_W)
#define _RSQRT_S(fd,fs)		mips_fp1(_jit,MIPS_fmt_S,fs,fd,MIPS_RSQRT_fmt)
#define _RSQRT_D(fd,fs)		mips_fp1(_jit,MIPS_fmt_D,fs,fd,MIPS_RSQRT_fmt)
#define _SB(rt,offset,base)	mipshrri(_jit,MIPS_SB,base,rt,offset)
#define _SC(rt,offset,base)	mipshrri(_jit,MIPS_SC,base,rt,offset)
#define _SCD(rt,offset,base)	mipshrri(_jit,MIPS_SCD,base,rt,offset)
#define _SD(rt,offset,base)	mipshrri(_jit,MIPS_SD,base,rt,offset)
#define _SDBPP(code)		mips_sbbpp(_jit,code)
__jit_inline void
mips_sdbpp(jit_state_t _jit, int ii)
{
    mips_code_t		c;
    c.tc.b = MIPS_SDBPP;
    c.br.b = _u20(ii);
    c.hc.b = MIPS_SPECIAL2;
    _jit_I(c.op);
}
#define _SDC1(ft,offset,base)	mipshrfi(_jit,MIPS_SDC1,base,ft,offset)
#define _SDC2(ft,offset,base)	mipshrfi(_jit,MIPS_SDC2,base,ft,offset)
#define _SDL(rt,offset,base)	mipshrri(_jit,MIPS_SDL,base,rt,offset)
#define _SDR(rt,offset,base)	mipshrri(_jit,MIPS_SDR,base,rt,offset)
#define _SDXC1(fd,index,base)	mips_rr_f(_jit,base,index,fd,MIPS_SDXC1)
#define _SEB(rd,rt)		mips_hrrrit(_jit,MIPS_SPECIAL3,_ZERO,rt,rd,MIPS_SEB,MIPS_BSHFL)
#define _SEH(rd,rt)		mips_hrrrit(_jit,MIPS_SPECIAL3,_ZERO,rt,rd,MIPS_SEH,MIPS_BSHFL)
#define _SH(rt,offset,base)	mipshrri(_jit,MIPS_SH,base,rt,offset)
#define _SLL(rd,rt,sa)		mips__rrit(_jit,rt,rd,sa,MIPS_SLL)
#define _SLLV(rd,rt,rs)		mips_rrr_t(_jit,rs,rt,rd,MIPS_SLLV)
#define _SLT(rd,rs,rt)		mips_rrr_t(_jit,rs,rt,rd,MIPS_SLT)
#define _SLTI(rt,rs,im)		mipshrri(_jit,MIPS_SLTI,rs,rt,im)
#define _SLTU(rd,rs,rt)		mips_rrr_t(_jit,rs,rt,rd,MIPS_SLTU)
#define _SLTIU(rt,rs,im)	mipshrri(_jit,MIPS_SLTIU,rs,rt,im)
#define _SQRT_S(fd,fs)		mips_fp1(_jit,MIPS_fmt_S,fs,fd,MIPS_SQRT_fmt)
#define _SQRT_D(fd,fs)		mips_fp1(_jit,MIPS_fmt_D,fs,fd,MIPS_SQRT_fmt)
#define _SRA(rd,rt,sa)		mips__rrit(_jit,rt,rd,sa,MIPS_SRA)
#define _SRAV(rd,rt,rs)		mips_rrr_t(_jit,rs,rt,rd,MIPS_SRAV)
#define _SRL(rd,rt,sa)		mips__rrit(_jit,rt,rd,sa,MIPS_SRL)
#define _SRLV(rd,rt,rs)		mips_rrr_t(_jit,rs,rt,rd,MIPS_SRLV)
#define _SSNOP()		_jit_I(1 << 6)
#define _SUB(rd,rs,rt)		mips_rrr_t(_jit,rs,rt,rd,MIPS_SUB)
#define _SUB_S(fd,fs,ft)	mips_fp2(_jit,MIPS_fmt_S,ft,fs,fd,MIPS_SUB_fmt)
#define _SUB_D(fd,fs,ft)	mips_fp2(_jit,MIPS_fmt_D,ft,fs,fd,MIPS_SUB_fmt)
#define _SUB_PS(fd,fs,ft)	mips_fp2(_jit,MIPS_fmt_PS,ft,fs,fd,MIPS_SUB_fmt)
#define _SUBU(rd,rs,rt)		mips_rrr_t(_jit,rs,rt,rd,MIPS_SUBU)
#define _SUXC1(fd,index,base)	mips_rr_f(_jit,base,index,fd,MIPS_SUXC1)
#define _SW(rt,offset,base)	mipshrri(_jit,MIPS_SW,base,rt,offset)
#define _SWC1(ft,offset,base)	mipshrfi(_jit,MIPS_SWC1,base,ft,offset)
#define _SWC2(ft,offset,base)	mipshrfi(_jit,MIPS_SWC2,base,ft,offset)
#define _SWL(rt,offset,base)	mipshrri(_jit,MIPS_SWL,base,rt,offset)
#define _SWR(rt,offset,base)	mipshrri(_jit,MIPS_SWR,base,rt,offset)
#define _SWXC1(fd,index,base)	mips_rr_f(_jit,base,index,fd,MIPS_SWXC1)
#define _SYNC(stype)		mips_sync(stype)
__jit_inline void
mips_sync(jit_state_t _jit, int stype)
{
    mips_code_t		c;
    c.op = 0;
    c.tc.b = MIPS_SYNC;
    c.ic.b = _u5(stype);
    _jit_I(c.op);    
}
#define _SYNCI(offset,base)	mipshrxs(_jit,MIPS_REGIMM,base,offset,MIPS_SYNCI)
#define _SYSCALL(code)		mips_syscall(_jit,code)
__jit_inline void
mips_syscall(jit_state_t _jit, int ii)
{
    mips_code_t		c;
    c.tc.b = MIPS_SYSCALL;
    c.br.b = _u20(ii);
    c.hc.b = MIPS_SPECIAL;
    _jit_I(c.op);
}
#define _TEQ(rs,rt)		mips_trap_cond(_jit,rs,rt,MIPS_TEQ)
#define _TEQI(rs,imm)		mips_hrxs(_jit,MIPS_REGIMM,rs,MIPS_TEQI,im)
#define _TGE(rs,rt)		mips_trap_cond(_jit,rs,rt,MIPS_TGE)
#define _TGEI(rs,imm)		mips_hrxs(_jit,MIPS_REGIMM,rs,MIPS_TGEI,im)
#define _TGEIU(rs,imm)		mips_hrxs(_jit,MIPS_REGIMM,rs,MIPS_TGEIU,im)
#define _TGEU(rs,rt)		mips_trap_cond(_jit,rs,rt,MIPS_TGEU)
#define _TLBP()			mips_tlb(_jit,MIPS_TLBP)
#define _TLBR()			mips_tlb(_jit,MIPS_TLBR)
#define _TLBWI()		mips_tlb(_jit,MIPS_TLBWI)
#define _TLBWR()		mips_tlb(_jit,MIPS_TLBWR)
#define _TLT(rs,rt)		mips_trap_cond(_jit,rs,rt,MIPS_TLT)
#define _TLTI(rs,imm)		mips_hrxs(_jit,MIPS_REGIMM,rs,MIPS_TLTI,im)
#define _TLTIU(rs,imm)		mips_hrxs(_jit,MIPS_REGIMM,rs,MIPS_TLTIU,im)
#define _TLTU(rs,rt)		mips_trap_cond(_jit,rs,rt,MIPS_TLTU)
#define _TNE(rs,rt)		mips_trap_cond(_jit,rs,rt,MIPS_TNE)
#define _TNEI(rs,imm)		mips_hrxs(_jit,MIPS_REGIMM,rs,MIPS_TNEI,im)
#define _TRUNC_L_S(fd,fs)	mips_fp1(_jit,MIPS_fmt_S,fs,fd,MIPS_TRUNC_fmt_L)
#define _TRUNC_L_D(fd,fs)	mips_fp1(_jit,MIPS_fmt_D,fs,fd,MIPS_TRUNC_fmt_L)
#define _TRUNC_W_S(fd,fs)	mips_fp1(_jit,MIPS_fmt_S,fs,fd,MIPS_TRUNC_fmt_W)
#define _TRUNC_W_D(fd,fs)	mips_fp1(_jit,MIPS_fmt_D,fs,fd,MIPS_TRUNC_fmt_W)
#define _WAIT()			_jit_I(1<<25)
#define _WRPGPR(rd,rt)		mips_hxrr_(_jit, MIPS_COP0,MIPS_WRPGPR,rt,rd)
#define _WSBH(rd,rt)		mips_hirrit(_jit,MIPS_SPECIAL3,0,rt,rd,MIPS_WSBH,MIPS_BSHFL)
#define _XOR(rd,rs,rt)		mips_rrr_t(_jit,rs,rt,rd,MIPS_XOR)
#define _XORI(rt,rs,im)		mipshrri(_jit,MIPS_XORI,rs,rt,im)

/* Reference:
 *	http://www.mrc.uidaho.edu/mrc/people/jff/digital/MIPSir.html
 *	MIPS32(r) Architecture Volume II: The MIPS32(r) Instrunction Set
 *	MIPS64(r) Architecture Volume II: The MIPS64(r) Instrunction Set
 */

#endif /* __lightning_asm_h */
