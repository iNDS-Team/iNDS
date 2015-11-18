/******************************** -*- C -*- ****************************
 *
 *	Run-time assembler for the x86-64
 *
 ***********************************************************************/


/***********************************************************************
 *
 * Copyright 2003 Gwenole Beauchesne
 * Copyright 2006,2010 Free Software Foundation, Inc.
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
 *	Paolo Bonzini
 *	Paulo Cesar Pereira de Andrade
 ***********************************************************************/

#ifndef __lightning_asm_h
#define __lightning_asm_h
#ifndef LIGHTNING_DEBUG

/*	OPCODE	+ i		= immediate operand
 *		+ r		= register operand
 *		+ m		= memory operand (disp,base,index,scale)
 *		+ sr/sm		= a star preceding a register or memory
 */

#define CALLsr(R)			CALLQsr(R)
#define JMPsr(R)			JMPQsr(R)

#if 0
#  define _r0P(R)		((int)(R) == (int)_NOREG)
#  define _rIP(R)		((int)(R) == (int)_RIP)
#  define _rspP(R)		(_rR(R) == _rR(_RSP))
#  define _rsp12P(R)		(_rN(R) == _rN(_RSP))

#  define _x86_RIP_addressing_possible(D,O)	(X86_RIP_RELATIVE_ADDR && \
						((unsigned long)x86_get_target() + 4 + (O) - (D) <= 0xffffffff))

#  define _r_X(   R, D,B,I,S,O)	(_r0P(I) ? (_r0P(B)    ? (!X86_TARGET_64BIT ? _r_D(R,D) : \
					                 (_x86_RIP_addressing_possible(D, O) ? \
				                          _r_D(R, (D) - ((unsigned long)x86_get_target() + 4 + (O))) : \
				                          _r_DSIB(R,D))) : \
					                 _r_DSIB(R,D                ))  : \
				           (_rIP(B)    ? _r_D   (R,D                )   : \
				           (_rsp12P(B) ? _r_DBIS(R,D,_RSP,_RSP,1)   : \
						         _r_DB  (R,D,     B       ))))  : \
				 (_r0P(B)	       ? _r_4IS (R,D,	         I,S)   : \
				 (!_rspP(I)            ? _r_DBIS(R,D,     B,     I,S)   : \
						         JITFAIL("illegal index register: %esp"))))
#endif

/* --- Increment/Decrement instructions ------------------------------------ */
__jit_inline void
x86_dec_sil_r(jit_state_t _jit, jit_gpr_t rd)
{
    _O(0xff);
    _Mrm(_b11, _b001, _rA(rd));
}

__jit_inline void
x86_inc_sil_r(jit_state_t _jit, jit_gpr_t rd)
{
    _O(0xff);
    _Mrm(_b11, _b000, _rA(rd));
}

/* --- REX prefixes -------------------------------------------------------- */
#define _BIT(X)			(!!(X))
__jit_inline void
x86_REXwrxb(jit_state_t _jit, int l, int w, int r, int x, int b)
{
    int		rex = (w << 3) | (r << 2) | (x << 1) | b;

    if (rex || l)
	_jit_B(0x40 | rex);
}

__jit_inline void
x86_REXwrx_(jit_state_t _jit, int l, int w, int r, int x, int mr)
{
    int		b = mr == _RIP ? 0 : _rXP(mr);

    x86_REXwrxb(_jit, l, w, r, x, _BIT(b));
}

__jit_inline void
x86_REXw_x_(jit_state_t _jit, int l, int w, int r, int x, int mr)
{
    x86_REXwrx_(_jit, l, w, _BIT(_rXP(r)), x, mr);
}

__jit_inline void
x86_REX_reg(jit_state_t _jit, int rr)
{
    x86_REXwrxb(_jit, 0, 0, 0, 0, _BIT(_rXP(rr)));
}

__jit_inline void
x86_REX_mem(jit_state_t _jit, int mb, int mi)
{
    x86_REXwrxb(_jit, 0, 0, 0, _BIT(_rXP(mi)), _BIT(_rXP(mb)));
}

__jit_inline void
x86_rex_b_rr(jit_state_t _jit, int rr, int mr)
{
    x86_REXw_x_(_jit, 0, 0, rr, 0, mr);
}

__jit_inline void
x86_rex_b_mr(jit_state_t _jit, int rb, int ri, int rd)
{
    x86_REXw_x_(_jit, 0, 0, rd, _BIT(_rXP(ri)), rb);
}

__jit_inline void
x86_rex_b_rm(jit_state_t _jit, int rs, int rb, int ri)
{
    x86_rex_b_mr(_jit, rb, ri, rs);
}

__jit_inline void
x86_rex_bl_rr(jit_state_t _jit, int rr, int mr)
{
    x86_REXw_x_(_jit, 0, 0, rr, 0, mr);
}

__jit_inline void
x86_rex_l_r(jit_state_t _jit, int rr)
{
    x86_REX_reg(_jit, rr);
}

__jit_inline void
x86_rex_l_m(jit_state_t _jit, int rb, int ri)
{
    x86_REX_mem(_jit, rb, ri);
}

__jit_inline void
x86_rex_l_rr(jit_state_t _jit, int rr, int mr)
{
    x86_REXw_x_(_jit, 0, 0, rr, 0, mr);
}

__jit_inline void
x86_rex_l_mr(jit_state_t _jit, int rb, int ri, int rd)
{
    x86_REXw_x_(_jit, 0, 0, rd, _BIT(_rXP(ri)), rb);
}

__jit_inline void
x86_rex_l_rm(jit_state_t _jit, int rs, int rb, int ri)
{
    x86_rex_l_mr(_jit, rb, ri, rs);
}

__jit_inline void
x86_rex_q_rr(jit_state_t _jit, int rr, int mr)
{
    x86_REXw_x_(_jit, 0, 1, rr, 0, mr);
}

__jit_inline void
x86_rex_q_mr(jit_state_t _jit, int rb, int ri, int rd)
{
    x86_REXw_x_(_jit, 0, 1, rd, _BIT(_rXP(ri)), rb);
}

__jit_inline void
x86_rex_q_rm(jit_state_t _jit, int rs, int rb, int ri)
{
    x86_rex_q_mr(_jit, rb, ri, rs);
}

__jit_inline void
x86_rex_q_r(jit_state_t _jit, int rr)
{
    x86_REX_reg(_jit, rr);
}

__jit_inline void
x86_rex_q_m(jit_state_t _jit, int rb, int ri)
{
    x86_REX_mem(_jit, rb, ri);
}

#define _REXBrr(rr, mr)			x86_REXBrr(_jit, rr, mr)
__jit_inline void
x86_REXBrr(jit_state_t _jit, jit_gpr_t rr, jit_gpr_t mr)
{
    x86_rex_b_rr(_jit, (int)rr, (int)mr);
}

#define _REXBmr(rb, ri, rd)		x86_REXBmr(_jit, rb, ri, rd)
__jit_inline void
x86_REXBmr(jit_state_t _jit, jit_gpr_t rb, jit_gpr_t ri, jit_gpr_t rd)
{
    x86_rex_b_mr(_jit, (int)rb, (int)ri, (int)rd);
}

#define _REXBrm(rs, rb, ri)		x86_REXBrm(_jit, rs, rb, ri)
__jit_inline void
x86_REXBrm(jit_state_t _jit, jit_gpr_t rs, jit_gpr_t rb, jit_gpr_t ri)
{
    x86_rex_b_rm(_jit, (int)rs, (int)rb, (int)ri);
}

#define _REXBLrr(rr, mr)		x86_REXBLrr(_jit, rr, mr)
__jit_inline void
x86_REXBLrr(jit_state_t _jit, jit_gpr_t rr, jit_gpr_t mr)
{
    x86_rex_bl_rr(_jit, (int)rr, (int)mr);
}

#define _REXLr(rr)			x86_REXLr(_jit, rr)
__jit_inline void
x86_REXLr(jit_state_t _jit, jit_gpr_t rr)
{
    x86_rex_l_r(_jit, (int)rr);
}

#define _REXLm(rb, ri)			x86_REXLr(_jit, rb, ri)
__jit_inline void
x86_REXLm(jit_state_t _jit, jit_gpr_t rb, jit_gpr_t ri)
{
    x86_rex_l_m(_jit, (int)rb, (int)ri);
}

#define _REXLrr(rr, mr)			x86_REXLrr(_jit, rr, mr)
__jit_inline void
x86_REXLrr(jit_state_t _jit, jit_gpr_t rr, jit_gpr_t mr)
{
    x86_rex_l_rr(_jit, (int)rr, (int)mr);
}

#define _REXLmr(rb, ri, rd)		x86_REXLmr(_jit, rb, ri, rd)
__jit_inline void
x86_REXLmr(jit_state_t _jit, jit_gpr_t rb, jit_gpr_t ri, jit_gpr_t rd)
{
    x86_rex_l_mr(_jit, (int)rb, (int)ri, (int)rd);
}

#define _REXLrm(rs, rb, ri)		x86_REXLrm(_jit, rs, rb, ri)
__jit_inline void
x86_REXLrm(jit_state_t _jit, jit_gpr_t rs, jit_gpr_t rb, jit_gpr_t ri)
{
    x86_rex_l_rm(_jit, (int)rs, (int)rb, (int)ri);
}

#define _REXQrr(rr, mr)			x86_REXQrr(_jit, rr, mr)
__jit_inline void
x86_REXQrr(jit_state_t _jit, jit_gpr_t rr, jit_gpr_t mr)
{
    x86_rex_q_rr(_jit, (int)rr, (int)mr);
}

#define _REXQmr(rb, ri, rd)		x86_REXQmr(_jit, rb, ri, rd)
__jit_inline void
x86_REXQmr(jit_state_t _jit, jit_gpr_t rb, jit_gpr_t ri, jit_gpr_t rd)
{
    x86_rex_q_mr(_jit, (int)rb, (int)ri, (int)rd);
}

#define _REXQrm(rs, rb, ri)		x86_REXQrm(_jit, rs, rb, ri)
__jit_inline void
x86_REXQrm(jit_state_t _jit, jit_gpr_t rs, jit_gpr_t rb, jit_gpr_t ri)
{
    x86_rex_q_rm(_jit, (int)rs, (int)rb, (int)ri);
}

#define _REXQr(rr)			x86_REXQr(_jit, rr)
__jit_inline void
x86_REXQr(jit_state_t _jit, jit_gpr_t rr)
{
    x86_rex_q_r(_jit, (int)rr);
}

#define _REXQm(rb, ri)			x86_REXQm(_jit, rb, ri)
__jit_inline void
x86_REXQm(jit_state_t _jit, jit_gpr_t rb, jit_gpr_t ri)
{
    x86_rex_q_m(_jit, (int)rb, (int)ri);
}

#define _rex_if_rr(rr, mr)		x86_rex_if_rr(_jit, rr, mr)
__jit_inline void
x86_rex_if_rr(jit_state_t _jit, jit_gpr_t rr, jit_fpr_t mr)
{
    x86_rex_l_rr(_jit, (int)rr, (int)mr);
}

#define _rex_ld_rr(rr, mr)		x86_rex_ld_rr(_jit, rr, mr)
__jit_inline void
x86_rex_ld_rr(jit_state_t _jit, jit_gpr_t rr, jit_fpr_t mr)
{
    x86_rex_q_rr(_jit, (int)rr, (int)mr);
}

#define _rex_ff_rr(rr, mr)		x86_rex_ff_rr(_jit, rr, mr)
__jit_inline void
x86_rex_ff_rr(jit_state_t _jit, jit_fpr_t rr, jit_fpr_t mr)
{
    x86_rex_l_rr(_jit, (int)rr, (int)mr);
}

#define _rex_fi_rr(rr, mr)		x86_rex_fi_rr(_jit, rr, mr)
__jit_inline void
x86_rex_fi_rr(jit_state_t _jit, jit_fpr_t rr, jit_gpr_t mr)
{
    x86_rex_l_rr(_jit, (int)rr, (int)mr);
}

#define _rex_if_mr(rb, ri, rd)		x86_rex_if_mr(_jit, rb, ri, rd)
__jit_inline void
x86_rex_if_mr(jit_state_t _jit, jit_gpr_t rb, jit_gpr_t ri, jit_fpr_t rd)
{
    x86_rex_l_mr(_jit, (int)rb, (int)ri, (int)rd);
}

#define _rex_fi_rm(rs, rb, ri)		x86_rex_fi_rm(_jit, rs, rb, ri)
__jit_inline void
x86_rex_fi_rm(jit_state_t _jit, jit_fpr_t rs, jit_gpr_t rb, jit_gpr_t ri)
{
    x86_rex_l_rm(_jit, (int)rs, (int)rb, (int)ri);
}

#define _rex_dl_rr(rr, mr)		x86_rex_dl_rr(_jit, rr, mr)
__jit_inline void
x86_rex_dl_rr(jit_state_t _jit, jit_fpr_t rr, jit_gpr_t mr)
{
    x86_rex_q_rr(_jit, (int)rr, (int)mr);
}

#define _rex_dd_rr(rr, mr)		x86_rex_dd_rr(_jit, rr, mr)
__jit_inline void
x86_rex_dd_rr(jit_state_t _jit, jit_fpr_t rr, jit_fpr_t mr)
{
    x86_rex_q_rr(_jit, (int)rr, (int)mr);
}

#define _rex_ld_mr(rb, ri, rd)		x86_rex_ld_mr(_jit, rb, ri, rd)
__jit_inline void
x86_rex_ld_mr(jit_state_t _jit, jit_gpr_t rb, jit_gpr_t ri, jit_fpr_t rd)
{
    x86_rex_q_mr(_jit, (int)rb, (int)ri, (int)rd);
}

#define rex_dl_rm(rs, rb, ri)		x86rex_dl_rm(_jit, rs, rb, ri)
__jit_inline void
x86rex_dl_rm(jit_state_t _jit, jit_fpr_t rs, jit_gpr_t rb, jit_gpr_t ri)
{
    x86_rex_q_rm(_jit, (int)rs, (int)rb, (int)ri);
}

/* --- ALU instructions ---------------------------------------------------- */
#define _ALUQrr(op, rs, rd)		x86_ALUQrr(_jit, op, rs, rd)
__jit_inline void
x86_ALUQrr(jit_state_t _jit, x86_alu_t op, jit_gpr_t rs, jit_gpr_t rd)
{
    _REXQrr(rs, rd);
    x86_alu_sil_rr(_jit, op, rs, rd);
}

#define _ALUQmr(op, md, rb, ri, ms, rd)	x86_ALUQmr(_jit, op, md, rb, ri, ms, rd)
__jit_inline void
x86_ALUQmr(jit_state_t _jit, x86_alu_t op,
	   long md, jit_gpr_t rb, jit_gpr_t ri, jit_scl_t ms, jit_gpr_t rd)
{
    _REXQmr(rb, ri, rd);
    x86_alu_sil_mr(_jit, op, md, rb, ri, ms, rd);
}

#define _ALUQrm(op, rs, md, rb, ri, ms)	x86_ALUQrm(_jit, op, rs, md, rb, ri, ms)
__jit_inline void
x86_ALUQrm(jit_state_t _jit, x86_alu_t op,
	   jit_gpr_t rs, long md, jit_gpr_t rb, jit_gpr_t ri, jit_scl_t ms)
{
    _REXQrm(rs, rb, ri);
    x86_alu_sil_rm(_jit, op, rs, md, rb, ri, ms);
}

#define _ALUQir(op, im, rd)		x86_ALUQir(_jit, op, im, rd)
__jit_inline void
x86_ALUQir(jit_state_t _jit, x86_alu_t op, long im, jit_gpr_t rd)
{
    _REXQrr(_NOREG, rd);
    x86_alu_il_ir(_jit, op, im, rd);
}

#define _ALUQim(op, im, md, rb, ri, ms)	x86_ALUQim(_jit, op, im, md, rb, ri, ms)
__jit_inline void
x86_ALUQim(jit_state_t _jit, x86_alu_t op,
	   long im, long md, jit_gpr_t rb, jit_gpr_t ri, jit_scl_t ms)
{
    _REXQrm(_NOREG, rb, ri);
    x86_alu_il_im(_jit, op, im, md, rb, ri, ms);
}

#define ADCQrr(RS, RD)			_ALUQrr(X86_ADC, RS, RD)
#define ADCQmr(MD, MB, MI, MS, RD)	_ALUQmr(X86_ADC, MD, MB, MI, MS, RD)
#define ADCQrm(RS, MD, MB, MI, MS)	_ALUQrm(X86_ADC, RS, MD, MB, MI, MS)
#define ADCQir(IM, RD)			_ALUQir(X86_ADC, IM, RD)
#define ADCQim(IM, MD, MB, MI, MS)	_ALUQim(X86_ADC, IM, MD, MB, MI, MS)

#define ADDQrr(RS, RD)			_ALUQrr(X86_ADD, RS, RD)
#define ADDQmr(MD, MB, MI, MS, RD)	_ALUQmr(X86_ADD, MD, MB, MI, MS, RD)
#define ADDQrm(RS, MD, MB, MI, MS)	_ALUQrm(X86_ADD, RS, MD, MB, MI, MS)
#define ADDQir(IM, RD)			_ALUQir(X86_ADD, IM, RD)
#define ADDQim(IM, MD, MB, MI, MS)	_ALUQim(X86_ADD, IM, MD, MB, MI, MS)

#define ANDQrr(RS, RD)			_ALUQrr(X86_AND, RS, RD)
#define ANDQmr(MD, MB, MI, MS, RD)	_ALUQmr(X86_AND, MD, MB, MI, MS, RD)
#define ANDQrm(RS, MD, MB, MI, MS)	_ALUQrm(X86_AND, RS, MD, MB, MI, MS)
#define ANDQir(IM, RD)			_ALUQir(X86_AND, IM, RD)
#define ANDQim(IM, MD, MB, MI, MS)	_ALUQim(X86_AND, IM, MD, MB, MI, MS)

#define CMPQrr(RS, RD)			_ALUQrr(X86_CMP, RS, RD)
#define CMPQmr(MD, MB, MI, MS, RD)	_ALUQmr(X86_CMP, MD, MB, MI, MS, RD)
#define CMPQrm(RS, MD, MB, MI, MS)	_ALUQrm(X86_CMP, RS, MD, MB, MI, MS)
#define CMPQir(IM, RD)			_ALUQir(X86_CMP, IM, RD)
#define CMPQim(IM, MD, MB, MI, MS)	_ALUQim(X86_CMP, IM, MD, MB, MI, MS)

#define ORQrr(RS, RD)			_ALUQrr(X86_OR, RS, RD)
#define ORQmr(MD, MB, MI, MS, RD)	_ALUQmr(X86_OR, MD, MB, MI, MS, RD)
#define ORQrm(RS, MD, MB, MI, MS)	_ALUQrm(X86_OR, RS, MD, MB, MI, MS)
#define ORQir(IM, RD)			_ALUQir(X86_OR, IM, RD)
#define ORQim(IM, MD, MB, MI, MS)	_ALUQim(X86_OR, IM, MD, MB, MI, MS)

#define SBBQrr(RS, RD)			_ALUQrr(X86_SBB, RS, RD)
#define SBBQmr(MD, MB, MI, MS, RD)	_ALUQmr(X86_SBB, MD, MB, MI, MS, RD)
#define SBBQrm(RS, MD, MB, MI, MS)	_ALUQrm(X86_SBB, RS, MD, MB, MI, MS)
#define SBBQir(IM, RD)			_ALUQir(X86_SBB, IM, RD)
#define SBBQim(IM, MD, MB, MI, MS)	_ALUQim(X86_SBB, IM, MD, MB, MI, MS)

#define SUBQrr(RS, RD)			_ALUQrr(X86_SUB, RS, RD)
#define SUBQmr(MD, MB, MI, MS, RD)	_ALUQmr(X86_SUB, MD, MB, MI, MS, RD)
#define SUBQrm(RS, MD, MB, MI, MS)	_ALUQrm(X86_SUB, RS, MD, MB, MI, MS)
#define SUBQir(IM, RD)			_ALUQir(X86_SUB, IM, RD)
#define SUBQim(IM, MD, MB, MI, MS)	_ALUQim(X86_SUB, IM, MD, MB, MI, MS)

#define XORQrr(RS, RD)			_ALUQrr(X86_XOR, RS, RD)
#define XORQmr(MD, MB, MI, MS, RD)	_ALUQmr(X86_XOR, MD, MB, MI, MS, RD)
#define XORQrm(RS, MD, MB, MI, MS)	_ALUQrm(X86_XOR, RS, MD, MB, MI, MS)
#define XORQir(IM, RD)			_ALUQir(X86_XOR, IM, RD)
#define XORQim(IM, MD, MB, MI, MS)	_ALUQim(X86_XOR, IM, MD, MB, MI, MS)

/* --- Shift/Rotate instructions ------------------------------------------- */
#define _ROTSHIQir(op, im, rd)		x86_ROTSHIQir(_jit, op, im, rd)
__jit_inline void
x86_ROTSHIQir(jit_state_t _jit, x86_rotsh_t op, long im, jit_gpr_t rd)
{
    _REXQrr(_NOREG, rd);
    x86_rotsh_sil_ir(_jit, op, im, rd);
}

#define _ROTSHIQim(op,im,md,rb,ri,ms)	x86_ROTSHIQim(_jit,op,im,md,rb,ri,ms)
__jit_inline void
x86_ROTSHIQim(jit_state_t _jit, x86_rotsh_t op,
	      long im, long md, jit_gpr_t rb, jit_gpr_t ri, jit_scl_t ms)
{
    _REXQrm(_NOREG, rb, ri);
    x86_rotsh_sil_im(_jit, op, im, md, rb, ri, ms);
}

#define _ROTSHIQrr(op, rs, rd)		x86_ROTSHIQrr(_jit, op, rs, rd)
__jit_inline void
x86_ROTSHIQrr(jit_state_t _jit, x86_rotsh_t op, jit_gpr_t rs, jit_gpr_t rd)
{
    _REXQrr(rs, rd);
    x86_rotsh_sil_rr(_jit, op, rs, rd);
}

#define _ROTSHIQrm(op,rs,md,rb,ri,ms)	x86_ROTSHIQrm(_jit,op,rs,md,rb,ri,ms)
__jit_inline void
x86_ROTSHIQrm(jit_state_t _jit, x86_rotsh_t op,
	      jit_gpr_t rs, long md, jit_gpr_t rb, jit_gpr_t ri, jit_scl_t ms)
{
    _REXQrm(rs, rb, ri);
    x86_rotsh_sil_rm(_jit, op, rs, md, rb, ri, ms);
}

#define ROLQir(IM, RD)			_ROTSHIQir(X86_ROL, IM, RD)
#define ROLQim(IM, MD, MB, MI, MS)	_ROTSHIQim(X86_ROL, IM, MD, MB, MI, MS)
#define ROLQrr(RS, RD)			_ROTSHIQrr(X86_ROL, RS, RD)
#define ROLQrm(RS, MD, MB, MI, MS)	_ROTSHIQrm(X86_ROL, RS, MD, MB, MI, MS)

#define RORQir(IM, RD)			_ROTSHIQir(X86_ROR, IM, RD)
#define RORQim(IM, MD, MB, MI, MS)	_ROTSHIQim(X86_ROR, IM, MD, MB, MI, MS)
#define RORQrr(RS, RD)			_ROTSHIQrr(X86_ROR, RS, RD)
#define RORQrm(RS, MD, MB, MI, MS)	_ROTSHIQrm(X86_ROR, RS, MD, MB, MI, MS)

#define RCLQir(IM, RD)			_ROTSHIQir(X86_RCL, IM, RD)
#define RCLQim(IM, MD, MB, MI, MS)	_ROTSHIQim(X86_RCL, IM, MD, MB, MI, MS)
#define RCLQrr(RS, RD)			_ROTSHIQrr(X86_RCL, RS, RD)
#define RCLQrm(RS, MD, MB, MI, MS)	_ROTSHIQrm(X86_RCL, RS, MD, MB, MI, MS)

#define RCRQir(IM, RD)			_ROTSHIQir(X86_RCR, IM, RD)
#define RCRQim(IM, MD, MB, MI, MS)	_ROTSHIQim(X86_RCR, IM, MD, MB, MI, MS)
#define RCRQrr(RS, RD)			_ROTSHIQrr(X86_RCR, RS, RD)
#define RCRQrm(RS, MD, MB, MI, MS)	_ROTSHIQrm(X86_RCR, RS, MD, MB, MI, MS)

#define SHLQir(IM, RD)			_ROTSHIQir(X86_SHL, IM, RD)
#define SHLQim(IM, MD, MB, MI, MS)	_ROTSHIQim(X86_SHL, IM, MD, MB, MI, MS)
#define SHLQrr(RS, RD)			_ROTSHIQrr(X86_SHL, RS, RD)
#define SHLQrm(RS, MD, MB, MI, MS)	_ROTSHIQrm(X86_SHL, RS, MD, MB, MI, MS)

#define SHRQir(IM, RD)			_ROTSHIQir(X86_SHR, IM, RD)
#define SHRQim(IM, MD, MB, MI, MS)	_ROTSHIQim(X86_SHR, IM, MD, MB, MI, MS)
#define SHRQrr(RS, RD)			_ROTSHIQrr(X86_SHR, RS, RD)
#define SHRQrm(RS, MD, MB, MI, MS)	_ROTSHIQrm(X86_SHR, RS, MD, MB, MI, MS)

#define SALQir				SHLQir
#define SALQim				SHLQim
#define SALQrr				SHLQrr
#define SALQrm				SHLQrm

#define SARQir(IM, RD)			_ROTSHIQir(X86_SAR, IM, RD)
#define SARQim(IM, MD, MB, MI, MS)	_ROTSHIQim(X86_SAR, IM, MD, MB, MI, MS)
#define SARQrr(RS, RD)			_ROTSHIQrr(X86_SAR, RS, RD)
#define SARQrm(RS, MD, MB, MI, MS)	_ROTSHIQrm(X86_SAR, RS, MD, MB, MI, MS)

/* --- Bit test instructions ----------------------------------------------- */
#define _BTQir(op, im, rd)		x86_BTQir(_jit, op, im, rd)
__jit_inline void
x86_BTQir(jit_state_t _jit, x86_bt_t op, long im, jit_gpr_t rd)
{
    _REXQrr(_NOREG, rd);
    x86_bt_sil_ir(_jit, op, im, rd);
}

#define _BTQim(op, im, md, rb, ri, ms)	x86_BTQim(_jit, op, im, md, rb, ri, ms)
__jit_inline void
x86_BTQim(jit_state_t _jit, x86_bt_t op,
	  long im, long md, jit_gpr_t rb, jit_gpr_t ri, jit_scl_t ms)
{
    _REXQrm(_NOREG, rb, ri);
    x86_bt_sil_im(_jit, op, im, md, rb, ri, ms);
}

#define _BTQrr(op, rs, rd)		x86_BTQrr(_jit, op, rs, rd)
__jit_inline void
x86_BTQrr(jit_state_t _jit, x86_bt_t op, jit_gpr_t rs, jit_gpr_t rd)
{
    _REXQrr(rs, rd);
    x86_bt_sil_rr(_jit, op, rs, rd);
}

#define _BTQrm(op, rs, md, rb, ri, ms)	x86_BTQrm(_jit, op, rs, md, rb, ri, ms)
__jit_inline void
x86_BTQrm(jit_state_t _jit, x86_bt_t op,
	  jit_gpr_t rs, long md, jit_gpr_t rb, jit_gpr_t ri, jit_scl_t ms)
{
    _REXQrm(rs, rb, ri);
    x86_bt_sil_rm(_jit, op, rs, md, rb, ri, ms);
}

#define BTQir(IM, RD)			_BTQir(X86_BT, IM, RD)
#define BTQim(IM, MD, MB, MI, MS)	_BTQim(X86_BT, IM, MD, MB, MI, MS)
#define BTQrr(RS, RD)			_BTQrr(X86_BT, RS, RD)
#define BTQrm(RS, MD, MB, MI, MS)	_BTQrm(X86_BT, RS, MD, MB, MI, MS)

#define BTCQir(IM, RD)			_BTQir(X86_BTC, IM, RD)
#define BTCQim(IM, MD, MB, MI, MS)	_BTQim(X86_BTC, IM, MD, MB, MI, MS)
#define BTCQrr(RS, RD)			_BTQrr(X86_BTC, RS, RD)
#define BTCQrm(RS, MD, MB, MI, MS)	_BTQrm(X86_BTC, RS, MD, MB, MI, MS)

#define BTRQir(IM, RD)			_BTQir(X86_BTR, IM, RD)
#define BTRQim(IM, MD, MB, MI, MS)	_BTQim(X86_BTR, IM, MD, MB, MI, MS)
#define BTRQrr(RS, RD)			_BTQrr(X86_BTR, RS, RD)
#define BTRQrm(RS, MD, MB, MI, MS)	_BTQrm(X86_BTR, RS, MD, MB, MI, MS)

#define BTSQir(IM, RD)			_BTQir(X86_BTS, IM, RD)
#define BTSQim(IM, MD, MB, MI, MS)	_BTQim(X86_BTS, IM, MD, MB, MI, MS)
#define BTSQrr(RS, RD)			_BTQrr(X86_BTS, RS, RD)
#define BTSQrm(RS, MD, MB, MI, MS)	_BTQrm(X86_BTS, RS, MD, MB, MI, MS)

/* --- Move instructions --------------------------------------------------- */
#define MOVQrr(rs, rd)			x86_MOVQrr(_jit, rs, rd)
__jit_inline void
x86_MOVQrr(jit_state_t _jit, jit_gpr_t rs, jit_gpr_t rd)
{
    _REXQrr(rs, rd);
    x86_mov_sil_rr(_jit, rs, rd);
}

#define MOVQmr(md, rb, ri, ms, rd)	x86_MOVQmr(_jit, md, rb, ri, ms, rd)
__jit_inline void
x86_MOVQmr(jit_state_t _jit,
	   long md, jit_gpr_t rb, jit_gpr_t ri, jit_scl_t ms, jit_gpr_t rd)
{
    _REXQmr(rb, ri, rd);
    x86_mov_sil_mr(_jit, md, rb, ri, ms, rd);
}

#define MOVQrm(rs, md, rb, ri, ms)	x86_MOVQrm(_jit, rs, md, rb, ri, ms)
__jit_inline void
x86_MOVQrm(jit_state_t _jit,
	   jit_gpr_t rs, long md, jit_gpr_t rb, jit_gpr_t ri, jit_scl_t ms)
{
    _REXQrm(rs, rb, ri);
    x86_mov_sil_rm(_jit, rs, md, rb, ri, ms);
}

#define MOVQir(im, rd)			x86_MOVQir(_jit, im, rd)
__jit_inline void
x86_MOVQir(jit_state_t _jit, long im, jit_gpr_t rd)
{
    _REXQrr(_NOREG, rd);
    _Or(0xb8, _r8(rd));
    _jit_L(im);
}

#define MOVQim(im, md, rb, ri, ms)	x86_MOVQim(_jit, im, md, rb, ri, ms)
__jit_inline void
x86_MOVQim(jit_state_t _jit,
	   long im, long md, jit_gpr_t rb, jit_gpr_t ri, jit_scl_t ms)
{
    _REXQrm(_NOREG, rb, ri);
    _O(0xc7);
    _i_X(_b000, md, rb, ri, ms);
    _jit_I(_s32(im));
}

/* --- Unary and Multiply/Divide instructions ------------------------------ */
#define _UNARYQr(op, rs)		x86_UNARYQr(_jit, op, rs)
__jit_inline void
x86_UNARYQr(jit_state_t _jit, x86_unary_t op, jit_gpr_t rs)
{
    _REXQrr(_NOREG, rs);
    x86_unary_sil_r(_jit, op, rs);
}

#define _UNARYQm(op, md, rb, ri, ms)	x86_UNARYQm(_jit, op, md, rb, ri, ms)
__jit_inline void
x86_UNARYQm(jit_state_t _jit, x86_unary_t op,
	    long md, jit_gpr_t rb, jit_gpr_t ri, jit_scl_t ms)
{
    _REXQmr(rb, ri, _NOREG);
    x86_unary_sil_m(_jit, op, md, rb, ri, ms);
}

#define NOTQr(RS)			_UNARYQr(X86_NOT, RS)
#define NOTQm(MD, MB, MI, MS)		_UNARYQm(X86_NOT, MD, MB, MI, MS)

#define NEGQr(RS)			_UNARYQr(X86_NEG, RS)
#define NEGQm(MD, MB, MI, MS)		_UNARYQm(X86_NEG, MD, MB, MI, MS)

#define MULQr(RS)			_UNARYQr(X86_MUL, RS)
#define MULQm(MD, MB, MI, MS)		_UNARYQm(X86_MUL, MD, MB, MI, MS)

#define IMULQr(RS)			_UNARYQr(X86_IMUL, RS)
#define IMULQm(MD, MB, MI, MS)		_UNARYQm(X86_IMUL, MD, MB, MI, MS)

#define DIVQr(RS)			_UNARYQr(X86_DIV, RS)
#define DIVQm(MD, MB, MI, MS)		_UNARYQm(X86_DIV, MD, MB, MI, MS)

#define IDIVQr(RS)			_UNARYQr(X86_IDIV, RS)
#define IDIVQm(MD, MB, MI, MS)		_UNARYQm(X86_IDIV, MD, MB, MI, MS)

#define IMULQrr(rs, rd)			x86_IMULQrr(_jit, rs, rd)
__jit_inline void
x86_IMULQrr(jit_state_t _jit, jit_gpr_t rs, jit_gpr_t rd)
{
    _REXQrr(rd, rs);
    x86_imul_sil_rr(_jit, rs, rd);
}

#define IMULQmr(md, rb, ri, ms, rd)	x86_IMULQmr(_jit, md, rb, ri, ms, rd)
__jit_inline void
x86_IMULQmr(jit_state_t _jit,
	    long md, jit_gpr_t rb, jit_gpr_t ri, jit_scl_t ms, jit_gpr_t rd)
{
    _REXQmr(rb, ri, rd);
    x86_imul_sil_mr(_jit, md, rb, ri, ms, rd);
}

#define IMULQir(im, rd)			x86_IMULQirr(_jit, im, rd)
#define IMULQirr(im, rs, rd)		x86_IMULQirr(_jit, im, rs, rd)
__jit_inline void
x86_IMULQirr(jit_state_t _jit, long im, jit_gpr_t rs, jit_gpr_t rd)
{
    _REXQrr(rd, rs);
    x86_imul_il_irr(_jit, im, rs, rd);
}

/* --- Control Flow related instructions ----------------------------------- */
#define CALLQsr(rs)			x86_CALLQsr(_jit, rs)
__jit_inline void
x86_CALLQsr(jit_state_t _jit, jit_gpr_t rs)
{
    _REXQrr(_NOREG, rs);
    x86_call_il_sr(_jit, rs);
}

#define JMPQsr(rs)			x86_JMPQsr(_jit, rs)
__jit_inline void
x86_JMPQsr(jit_state_t _jit, jit_gpr_t rs)
{
    _REXQrr(_NOREG, rs);
    x86_jmp_il_sr(_jit, rs);
}

#define CMOVQrr(cc, rs, rd)		x86_CMOVQrr(_jit, cc, rs, rd)
__jit_inline void
x86_CMOVQrr(jit_state_t _jit, x86_cc_t cc, jit_gpr_t rs, jit_gpr_t rd)
{
    _REXQrr(rd, rs);
    x86_cmov_sil_rr(_jit, cc, rs, rd);
}

#define CMOVQmr(cc, md, rb, ri, ms, rd)	x86_CMOVQmr(_jit, cc, md, rb, ri, ms, rd)
__jit_inline void
x86_CMOVQmr(jit_state_t _jit, x86_cc_t cc,
	    long md, jit_gpr_t rb, jit_gpr_t ri, jit_scl_t ms, jit_gpr_t rd)
{
    _REXQmr(rb, ri, rd);
    x86_cmov_sil_mr(_jit, cc, md, rb, ri, ms, rd);
}

/* --- Push/Pop instructions ----------------------------------------------- */
#define POPQr(rd)			x86_POPQr(_jit, rd)
__jit_inline void
x86_POPQr(jit_state_t _jit, jit_gpr_t rd)
{
    _REXQr(rd);
    x86_pop_sil_r(_jit, rd);
}

#define POPQm(md, rb, ri, ms)		x86_POPQm(_jit, md, rb, ri, ms)
__jit_inline void
x86_POPQm(jit_state_t _jit, long md, jit_gpr_t rb, jit_gpr_t ri, jit_scl_t ms)
{
    _REXQm(rb, ri);
    x86_pop_sil_m(_jit, md, rb, ri, ms);
}

#define PUSHQr(rd)			x86_PUSHQr(_jit, rd)
__jit_inline void
x86_PUSHQr(jit_state_t _jit, jit_gpr_t rs)
{
    _REXQr(rs);
    x86_push_sil_r(_jit, rs);
}

#define PUSHQm(md, rb, ri, ms)		x86_PUSHQm(_jit, md, rb, ri, ms)
__jit_inline void
x86_PUSHQm(jit_state_t _jit, long md, jit_gpr_t rb, jit_gpr_t ri, jit_scl_t ms)
{
    _REXQm(rb, ri);
    x86_push_sil_m(_jit, md, rb, ri, ms);
}

#define PUSHQi(im)			x86_PUSHQi(_jit, im)
__jit_inline void
x86_PUSHQi(jit_state_t _jit, long im)
{
    x86_push_il_i(_jit, im);
}

/* --- Test instructions --------------------------------------------------- */
#define TESTQrr(rs, rd)			x86_TESTQrr(_jit, rs, rd)
__jit_inline void
x86_TESTQrr(jit_state_t _jit, jit_gpr_t rs, jit_gpr_t rd)
{
    _REXQrr(rs, rd);
    x86_test_sil_rr(_jit, rs, rd);
}

#define TESTQrm(rs, md, rb, ri, ms)	x86_TESTQrm(_jit, rs, md, rb, ri, ms)
__jit_inline void
x86_TESTQrm(jit_state_t _jit,
	    jit_gpr_t rs, long md, jit_gpr_t rb, jit_gpr_t ri, jit_scl_t ms)
{
    _REXQrm(rs, rb, ri);
    x86_test_sil_rm(_jit, rs, md, rb, ri, ms);
}

#define TESTQir(im, rd)			x86_TESTQir(_jit, im, rd)
__jit_inline void
x86_TESTQir(jit_state_t _jit, long im, jit_gpr_t rd)
{
    _REXQrr(_NOREG, rd);
    x86_test_il_ir(_jit, im, rd);
}

#define TESTQim(im, md, rb, ri, ms)	x86_TESTQim(_jit, im, md, rb, ri, ms)
__jit_inline void
x86_TESTQim(jit_state_t _jit,
	    long im, long md, jit_gpr_t rb, jit_gpr_t ri, jit_scl_t ms)
{
    _REXQrm(_NOREG, rb, ri);
    x86_test_il_im(_jit, im, md, rb, ri, ms);
}

/* --- Exchange instructions ----------------------------------------------- */
#define CMPXCHGQrr(rs, rd)		x86_CMPXCHGQrr(_jit, rs, rd)
__jit_inline void
x86_CMPXCHGQrr(jit_state_t _jit, jit_gpr_t rs, jit_gpr_t rd)
{
    _REXQrr(rs, rd);
    x86_cmpxchg_sil_rr(_jit, rs, rd);
}

#define CMPXCHGQrm(rs, md, rb, ri, ms)	x86_CMPXCHGQrm(_jit, rs, md, rb, ri, ms)
__jit_inline void
x86_CMPXCHGQrm(jit_state_t _jit,
	       jit_gpr_t rs, long md, jit_gpr_t rb, jit_gpr_t ri, jit_scl_t ms)
{
    _REXQrm(rs, rb, ri);
    x86_cmpxchg_sil_rm(_jit, rs, md, rb, ri, ms);
}

#define XADDQrr(rs, rd)			x86_XADDQrr(_jit, rs, rd)
__jit_inline void
x86_XADDQrr(jit_state_t _jit, jit_gpr_t rs, jit_gpr_t rd)
{
    _REXQrr(rs, rd);
    x86_xadd_sil_rr(_jit, rs, rd);
}

#define XADDQrm(rs, md, rb, ri, ms)	x86_XADDQrm(_jit, rs, md, rb, ri, ms)
__jit_inline void
x86_XADDQrm(jit_state_t _jit,
	    jit_gpr_t rs, long md, jit_gpr_t rb, jit_gpr_t ri, jit_scl_t ms)
{
    _REXQrm(rs, rb, ri);
    x86_xadd_sil_rm(_jit, rs, md, rb, ri, ms);
}

#define XCHGQrr(rs, rd)			x86_XCHGQrr(_jit, rs, rd)
__jit_inline void
x86_XCHGQrr(jit_state_t _jit, jit_gpr_t rs, jit_gpr_t rd)
{
    _REXQrr(rs, rd);
    x86_xchg_sil_rr(_jit, rs, rd);
}

#define XCHGQrm(rs, md, rb, ri, ms)	x86_XCHGQrm(_jit, rs, md, rb, ri, ms)
__jit_inline void
x86_XCHGQrm(jit_state_t _jit,
	    jit_gpr_t rs, long md, jit_gpr_t rb, jit_gpr_t ri, jit_scl_t ms)
{
    _REXQrm(rs, rb, ri);
    x86_xchg_sil_rm(_jit, rs, md, rb, ri, ms);
}

/* --- Increment/Decrement instructions ------------------------------------ */
#define DECQr(rd)			x86_DECQr(_jit, rd)
__jit_inline void
x86_DECQr(jit_state_t _jit, jit_gpr_t rd)
{
    _REXQrr(_NOREG, rd);
    x86_dec_sil_r(_jit, rd);
}

#define DECQm(md, rb, ri, ms)		x86_DECQm(_jit, md, rb, ri, ms)
__jit_inline void
x86_DECQm(jit_state_t _jit, long md, jit_gpr_t rb, jit_gpr_t ri, jit_scl_t ms)
{
    _REXQrm(_NOREG, rb, ri);
    x86_dec_sil_m(_jit, md, rb, ri, ms);
}

#define INCQr(rd)			x86_INCQr(_jit, rd)
__jit_inline void
x86_INCQr(jit_state_t _jit, jit_gpr_t rd)
{
    _REXQrr(_NOREG, rd);
    x86_inc_sil_r(_jit, rd);
}

#define INCQm(md, rb, ri, ms)		x86_INCQm(_jit, md, rb, ri, ms)
__jit_inline void
x86_INCQm(jit_state_t _jit, long md, jit_gpr_t rb, jit_gpr_t ri, jit_scl_t ms)
{
    _REXQrm(_NOREG, rb, ri);
    x86_inc_sil_m(_jit, md, rb, ri, ms);
}

/* --- Misc instructions --------------------------------------------------- */
#define BSFQrr(rs, rd)			x86_BSFQrr(_jit, rs, rd)
__jit_inline void
x86_BSFQrr(jit_state_t _jit, jit_gpr_t rs, jit_gpr_t rd)
{
    _REXQrr(rd, rs);
    x86_bsf_sil_rr(_jit, rs, rd);
}

#define BSFQmr(md, rb, ri, ms, rd)	x86_BSFQmr(_jit, md, rb, ri, ms, rd)
__jit_inline void
x86_BSFQmr(jit_state_t _jit,
	   long md, jit_gpr_t rb, jit_gpr_t ri, jit_scl_t ms, jit_gpr_t rd)
{
    _REXQmr(rb, ri, rd);
    x86_bsf_sil_mr(_jit, md, rb, ri, ms, rd);
}

#define BSRQrr(rs, rd)			x86_BSRQrr(_jit, rs, rd)
__jit_inline void
x86_BSRQrr(jit_state_t _jit, jit_gpr_t rs, jit_gpr_t rd)
{
    _REXQrr(rd, rs);
    x86_bsr_sil_rr(_jit, rs, rd);
}

#define BSRQmr(md, rb, ri, ms, rd)	x86_BSRQmr(_jit, md, rb, ri, ms, rd)
__jit_inline void
x86_BSRQmr(jit_state_t _jit,
	   long md, jit_gpr_t rb, jit_gpr_t ri, jit_scl_t ms, jit_gpr_t rd)
{
    _REXQmr(rb, ri, rd);
    x86_bsr_sil_mr(_jit, md, rb, ri, ms, rd);
}

/* long rd = (int)rs */
__jit_inline void
x86_movsd_l_rr(jit_state_t _jit, jit_gpr_t rs, jit_gpr_t rd)
{
    _O(0x63);
    _Mrm(_b11, _rA(rd), _rA(rs));
}

/* long rd = (int)*rs */
__jit_inline void
x86_movsd_l_mr(jit_state_t _jit,
	       long md, jit_gpr_t rb, jit_gpr_t ri, jit_scl_t ms, jit_gpr_t rd)
{
    _O(0x63);
    _r_X(rd, md, rb, ri, ms);
}

#define MOVSBQrr(rs, rd)		x86_MOVSBQrr(_jit, rs, rd)
__jit_inline void
x86_MOVSBQrr(jit_state_t _jit, jit_gpr_t rs, jit_gpr_t rd)		
{
    _REXQrr(rd, rs);
    x86_movsb_sil_rr(_jit, rs, rd);
}

#define MOVSBQmr(md, rb, ri, ms, rd)	x86_MOVSBQmr(_jit, md, rb, ri, ms, rd)
__jit_inline void
x86_MOVSBQmr(jit_state_t _jit,
	     long md, jit_gpr_t rb, jit_gpr_t ri, jit_scl_t ms, jit_gpr_t rd)
{
    _REXQmr(rb, ri, rd);
    x86_movsb_sil_mr(_jit, md, rb, ri, ms, rd);
}

#define MOVSWQrr(rs, rd)		x86_MOVSWQrr(_jit, rs, rd)
__jit_inline void
x86_MOVSWQrr(jit_state_t _jit, jit_gpr_t rs, jit_gpr_t rd)
{
    _REXQrr(rd, rs);
    x86_movsw_il_rr(_jit, rs, rd);
}

#define MOVSWQmr(md, rb, ri, ms, rd)	x86_MOVSWQmr(_jit, md, rb, ri, ms, rd)
__jit_inline void
x86_MOVSWQmr(jit_state_t _jit,
	     long md, jit_gpr_t rb, jit_gpr_t ri, jit_scl_t ms, jit_gpr_t rd)
{
    _REXQmr(rb, ri, rd);
    x86_movsw_il_mr(_jit, md, rb, ri, ms, rd);
}

#define MOVSLQrr(rs, rd)		x86_MOVSLQrr(_jit, rs, rd)
__jit_inline void
x86_MOVSLQrr(jit_state_t _jit, jit_gpr_t rs, jit_gpr_t rd)
{
    _REXQrr(rd, rs);
    x86_movsd_l_rr(_jit, rs, rd);
}

#define MOVSLQmr(md, rb, ri, ms, rd)	x86_MOVSLQmr(_jit, md, rb, ri, ms, rd)
__jit_inline void
x86_MOVSLQmr(jit_state_t _jit,
	     long md, jit_gpr_t rb, jit_gpr_t ri, jit_scl_t ms, jit_gpr_t rd)
{
    _REXQmr(rb, ri, rd);
    x86_movsd_l_mr(_jit, md, rb, ri, ms, rd);
}

#define MOVZBQrr(rs, rd)		x86_MOVZBQrr(_jit, rs, rd)
__jit_inline void
x86_MOVZBQrr(jit_state_t _jit, jit_gpr_t rs, jit_gpr_t rd)		
{
    _REXQrr(rd, rs);
    x86_movzb_sil_rr(_jit, rs, rd);
}

#define MOVZBQmr(md, rb, ri, ms, rd)	x86_MOVZBQmr(_jit, md, rb, ri, ms, rd)
__jit_inline void
x86_MOVZBQmr(jit_state_t _jit,
	     long md, jit_gpr_t rb, jit_gpr_t ri, jit_scl_t ms, jit_gpr_t rd)
{
    _REXQmr(rb, ri, rd);
    x86_movzb_sil_mr(_jit, md, rb, ri, ms, rd);
}

#define MOVZWQrr(rs, rd)		x86_MOVZWQrr(_jit, rs, rd)
__jit_inline void
x86_MOVZWQrr(jit_state_t _jit, jit_gpr_t rs, jit_gpr_t rd)
{
    _REXQrr(rd, rs);
    x86_movzw_il_rr(_jit, rs, rd);
}

#define MOVZWQmr(md, rb, ri, ms, rd)	x86_MOVZWQmr(_jit, md, rb, ri, ms, rd)
__jit_inline void
x86_MOVZWQmr(jit_state_t _jit,
	     long md, jit_gpr_t rb, jit_gpr_t ri, jit_scl_t ms, jit_gpr_t rd)
{
    _REXQmr(rb, ri, rd);
    x86_movzw_il_mr(_jit, md, rb, ri, ms, rd);
}

#define LEAQmr(md, rb, ri, ms, rd)	x86_LEAQmr(_jit, md, rb, ri, ms, rd)
__jit_inline void
x86_LEAQmr(jit_state_t _jit,
	   long md, jit_gpr_t rb, jit_gpr_t ri, jit_scl_t ms, jit_gpr_t rd)
{
    _REXQmr(rb, ri, rd);
    x86_lea_il_mr(_jit, md, rb, ri, ms, rd);
}

#define BSWAPQr(rd)			x86_BSWAPQr(_jit, rd)
__jit_inline void
x86_BSWAPQr(jit_state_t _jit, jit_gpr_t rd)
{
    _REXQrr(_NOREG, rd);
    x86_bswap_il_r(_jit, rd);
}

/* long rax = (int)eax */
#define CLTQ_()				x86_CDQE_(_jit)
#define CDQE_()				x86_CDQE_(_jit)
__jit_inline void
x86_CDQE_(jit_state_t _jit)
{
    _REXQrr(_NOREG, _NOREG);
    _sign_extend_rax();
}

#define CQTO_()				x86_CQO_(_jit)
#define CQO_()				x86_CQO_(_jit)
/* long rdx:rax = rax */
__jit_inline void
x86_CQO_(jit_state_t _jit)
{
    _REXQrr(_NOREG, _NOREG);
    _sign_extend_rdx_rax();
}

#define __sse_dd_rr(op, rs, rd)		x86__sse_dd_rr(_jit, op, rs, rd)
__jit_inline void
x86__sse_dd_rr(jit_state_t _jit, x86_sse_t op, jit_fpr_t rs, jit_fpr_t rd)
{
    _rex_dd_rr(rd, rs);
    _O(0x0f);
    _O(op);
    _Mrm(_b11, _rX(rd), _rX(rs));
}

#define __sse_lf_rr(op, rs, rd)		x86__sse_ld_rr(_jit, op, rs, rd)
#define __sse_ld_rr(op, rs, rd)		x86__sse_ld_rr(_jit, op, rs, rd)
__jit_inline void
x86__sse_ld_rr(jit_state_t _jit, x86_sse_t op, jit_fpr_t rs, jit_gpr_t rd)
{
    _rex_ld_rr(rd, rs);
    _O(0x0f);
    _O(op);
    _Mrm(_b11, _rA(rd), _rX(rs));
}

#define __sse_fl_rr(op, rs, rd)		x86__sse_dl_rr(_jit, op, rs, rd)
#define __sse_dl_rr(op, rs, rd)		x86__sse_dl_rr(_jit, op, rs, rd)
__jit_inline void
x86__sse_dl_rr(jit_state_t _jit, x86_sse_t op, jit_gpr_t rs, jit_fpr_t rd)
{
    _rex_dl_rr(rd, rs);
    _O(0x0f);
    _O(op);
    _Mrm(_b11, _rX(rd), _rA(rs));
}

#define __sse_lf_mr(op, md, rb, mi, ms, rd)				\
    x86__sse_ld_mr(_jit, op, md, rb, mi, ms, rd)
#define __sse_ld_mr(op, md, rb, mi, ms, rd)				\
    x86__sse_ld_mr(_jit, op, md, rb, mi, ms, rd)
__jit_inline void
x86__sse_ld_mr(jit_state_t _jit, x86_sse_t op,
	       long md, jit_gpr_t rb, jit_gpr_t ri, jit_scl_t ms, jit_fpr_t rd)
{
    _rex_ld_mr(rb, ri, rd);
    _O(0x0f);
    _O(op);
    _f_X(rd, md, rb, ri, ms);
}

#define __sse_dl_rm(op, rs, md, rb, mi, ms)				\
    x86__sse_dl_rm(_jit, op, rs, md, rb, mi, ms)
__jit_inline void
x86__sse_dl_rm(jit_state_t _jit, x86_sse_t op,
	       jit_fpr_t rs, long md, jit_gpr_t rb, jit_gpr_t ri, jit_scl_t ms)
{
    rex_dl_rm(rs, rb, ri);
    _O(0x0f);
    _O(op);
    _f_X(rs, md, rb, ri, ms);
}

#define __sse1_dl_rm(op, rs, md, mb, mi, ms)				\
    x86__sse1_dl_rm(_jit, op, rs, md, mb, mi, ms)
__jit_inline void
x86__sse1_dl_rm(jit_state_t _jit, x86_sse_t op,
		jit_fpr_t rs, long md, jit_gpr_t rb, jit_gpr_t ri, jit_scl_t ms)
{
    rex_dl_rm(rs, rb, ri);
    _O(0x0f);
    _O(0x01 | op);
    _f_X(rs, md, rb, ri, ms);
}

#define _sse_dd_rr(px, op, rs, rd)	x86_sse_dd_rr(_jit, px, op, rs, rd)
__jit_inline void
x86_sse_dd_rr(jit_state_t _jit, _uc px,
	      x86_sse_t op, jit_fpr_t rs, jit_fpr_t rd)
{
    _jit_B(px);
    x86__sse_dd_rr(_jit, op, rs, rd);
}

#define _sse_lf_rr(px, op, rs, rd)	x86_sse_ld_rr(_jit, px, op, rs, rd)
#define _sse_ld_rr(px, op, rs, rd)	x86_sse_ld_rr(_jit, px, op, rs, rd)
__jit_inline void
x86_sse_ld_rr(jit_state_t _jit,
	      _uc px, x86_sse_t op, jit_fpr_t rs, jit_gpr_t rd)
{
    _jit_B(px);
    x86__sse_ld_rr(_jit, op, rs, rd);
}

#define _sse_fl_rr(px, op, rs, rd)	x86_sse_dl_rr(_jit, px, op, rs, rd)
#define _sse_dl_rr(px, op, rs, rd)	x86_sse_dl_rr(_jit, px, op, rs, rd)
__jit_inline void
x86_sse_dl_rr(jit_state_t _jit,
	      _uc px, x86_sse_t op, jit_gpr_t rs, jit_fpr_t rd)
{
    _jit_B(px);
    x86__sse_dl_rr(_jit, op, rs, rd);
}

#define _sse_ld_mr(px, op, md, rb, mi, ms, rd)				\
    x86_sse_ld_mr(_jit, px, op, md, rb, mi, ms, rd)
__jit_inline void
x86_sse_ld_mr(jit_state_t _jit, _uc px, x86_sse_t op,
	      long md, jit_gpr_t rb, jit_gpr_t ri, jit_scl_t ms, jit_fpr_t rd)
{
    _jit_B(px);
    x86__sse_ld_mr(_jit, op, md, rb, ri, ms, rd);
}

#define _sse_dl_rm(px, op, rs, md, rb, mi, ms)				\
    x86_sse_dl_rm(_jit, px, op, rs, md, rb, mi, ms)
__jit_inline void
x86_sse_dl_rm(jit_state_t _jit, _uc px, x86_sse_t op,
	      jit_fpr_t rs, long md, jit_gpr_t rb, jit_gpr_t ri, jit_scl_t ms)
{
    _jit_B(px);
    x86__sse_dl_rm(_jit, op, rs, md, rb, ri, ms);
}

#define _sse1_dl_rm(px, op, rs, md, mb, mi, ms)				\
    x86_sse1_dl_rm(_jit, px, op, rs, md, mb, mi, ms)
__jit_inline void
x86_sse1_dl_rm(jit_state_t _jit, _uc px, x86_sse_t op,
	       jit_fpr_t rs, long md, jit_gpr_t rb, jit_gpr_t ri, jit_scl_t ms)
{
    _jit_B(px);
    x86__sse1_dl_rm(_jit, op, rs, md, rb, ri, ms);
}

#define CVTTSS2SIQrr(RS, RD)		 _sse_lf_rr(0xf3, X86_SSE_CVTTSI, RS, RD)
#define CVTTSS2SIQmr(MD, MB, MI, MS, RD) _sse_lf_mr(0xf3, X86_SSE_CVTTSI, MD, MB, MI, MS, RD)
#define CVTTSD2SIQrr(RS, RD)		 _sse_lf_rr(0xf2, X86_SSE_CVTTSI, RS, RD)
#define CVTTSD2SIQmr(MD, MB, MI, MS, RD) _sse_lf_mr(0xf2, X86_SSE_CVTTSI, MD, MB, MI, MS, RD)

#define CVTSS2SIQrr(RS, RD)		 _sse_lf_rr(0xf3, X86_SSE_CVTSI, RS, RD)
#define CVTSS2SIQmr(MD, MB, MI, MS, RD)	 _sse_lf_mr(0xf3, X86_SSE_CVTSI, MD, MB, MI, MS, RD)
#define CVTSD2SIQrr(RS, RD)		 _sse_lf_rr(0xf2, X86_SSE_CVTSI, RS, RD)
#define CVTSD2SIQmr(MD, MB, MI, MS, RD)	 _sse_lf_mr(0xf2, X86_SSE_CVTSI, MD, MB, MI, MS, RD)

#define CVTSI2SSQrr(RS, RD)		 _sse_fl_rr(0xf3, X86_SSE_CVTIS, RS, RD)
#define CVTSI2SSQmr(MD, MB, MI, MS, RD)	 _sse_lf_mr(0xf3, X86_SSE_CVTIS, MD, MB, MI, MS, RD)
#define CVTSI2SDQrr(RS, RD)		 _sse_dl_rr(0xf2, X86_SSE_CVTIS, RS, RD)
#define CVTSI2SDQmr(MD, MB, MI, MS, RD)	 _sse_ld_mr(0xf2, X86_SSE_CVTIS, MD, MB, MI, MS, RD)

#define MOVDQXrr(RS, RD)		 _sse_dl_rr(0x66, X86_SSE_X2G, RS, RD)
#define MOVDQXmr(MD, MB, MI, MS, RD)	 _sse_ld_mr(0x66, X86_SSE_X2G, MD, MB, MI, MS, RD)

#define MOVDXQrr(RS, RD)		 _sse_ld_rr(0x66, X86_SSE_G2X, RS, RD)
#define MOVDXQrm(RS, MD, MB, MI, MS)	 _sse_dl_rm(0x66, X86_SSE_G2X, RS, MD, MB, MI, MS)
#define MOVDQMrr(RS, RD)		__sse_dl_rr(      X86_SSE_X2G, RS, RD)
#define MOVDQMmr(MD, MB, MI, MS, RD)	__sse_ld_mr(      X86_SSE_X2G, MD, MB, MI, MS, RD)
#define MOVDMQrr(RS, RD)		__sse_dd_rr(      X86_SSE_G2X, RS, RD)
#define MOVDMQrm(RS, MD, MB, MI, MS)	__sse_dl_rm(      X86_SSE_G2X, RS, MD, MB, MI, MS)

#endif	/* LIGHTNING_DEBUG */
#endif	/* __lightning_asm_h */
