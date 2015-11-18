/******************************** -*- C -*- ****************************
 *
 *	Run-time assembler for the i386
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

#define CALLsr(R)			CALLLsr(R)
#define JMPsr(R)			JMPLsr(R)

/* --- Increment/Decrement instructions ------------------------------------ */
__jit_inline void
x86_dec_sil_r(jit_state_t _jit, jit_gpr_t rd)
{
    _Or(0x48, _rA(rd));
}

__jit_inline void
x86_inc_sil_r(jit_state_t _jit, jit_gpr_t rd)
{
    _Or(0x40, _rA(rd));
}

/* --- REX prefixes -------------------------------------------------------- */
#define _REXBrr(rr, mr)			x86_REXBrr(_jit, rr, mr)
__jit_inline void
x86_REXBrr(jit_state_t _jit, jit_gpr_t rr, jit_gpr_t mr)
{
}

#define _REXBmr(rb, ri, rd)		x86_REXBmr(_jit, rb, ri, rd)
__jit_inline void
x86_REXBmr(jit_state_t _jit, jit_gpr_t rb, jit_gpr_t ri, jit_gpr_t rd)
{
}

#define _REXBrm(rs, rb, ri)		x86_REXBrm(_jit, rs, rb, ri)
__jit_inline void
x86_REXBrm(jit_state_t _jit, jit_gpr_t rs, jit_gpr_t rb, jit_gpr_t ri)
{
}

#define _REXLr(rr)			x86_REXLr(_jit, rr)
__jit_inline void
x86_REXLr(jit_state_t _jit, jit_gpr_t rr)
{
}

#define _REXLm(rb, ri)			x86_REXLr(_jit, rb, ri)
__jit_inline void
x86_REXLm(jit_state_t _jit, jit_gpr_t rb, jit_gpr_t ri)
{
}

#define _REXBLrr(rr, mr)		x86_REXBLrr(_jit, rr, mr)
__jit_inline void
x86_REXBLrr(jit_state_t _jit, jit_gpr_t rr, jit_gpr_t mr)
{
}

#define _REXLrr(rr, mr)			x86_REXLrr(_jit, rr, mr)
__jit_inline void
x86_REXLrr(jit_state_t _jit, jit_gpr_t rr, jit_gpr_t mr)
{
}

#define _REXLmr(rb, ri, rd)		x86_REXLmr(_jit, rb, ri, rd)
__jit_inline void
x86_REXLmr(jit_state_t _jit, jit_gpr_t rb, jit_gpr_t ri, jit_gpr_t rd)
{
}

#define _REXLrm(rs, rb, ri)		x86_REXLrm(_jit, rs, rb, ri)
__jit_inline void
x86_REXLrm(jit_state_t _jit, jit_gpr_t rs, jit_gpr_t rb, jit_gpr_t ri)
{
}

#define _rex_if_rr(rr, mr)		x86_rex_if_rr(_jit, rr, mr)
__jit_inline void
x86_rex_if_rr(jit_state_t _jit, jit_gpr_t rr, jit_fpr_t mr)
{
}

#define _rex_ff_rr(rr, mr)		x86_rex_ff_rr(_jit, rr, mr)
__jit_inline void
x86_rex_ff_rr(jit_state_t _jit, jit_fpr_t rr, jit_fpr_t mr)
{
}

#define _rex_fi_rr(rr, mr)		x86_rex_fi_rr(_jit, rr, mr)
__jit_inline void
x86_rex_fi_rr(jit_state_t _jit, jit_fpr_t rr, jit_gpr_t mr)
{
}

#define _rex_if_mr(rb, ri, rd)		x86_rex_if_mr(_jit, rb, ri, rd)
__jit_inline void
x86_rex_if_mr(jit_state_t _jit, jit_gpr_t rb, jit_gpr_t ri, jit_fpr_t rd)
{
}

#define _rex_fi_rm(rs, rb, ri)		x86_rex_fi_rm(_jit, rs, rb, ri)
__jit_inline void
x86_rex_fi_rm(jit_state_t _jit, jit_fpr_t rs, jit_gpr_t rb, jit_gpr_t ri)
{
}

/* --- Push/Pop instructions ----------------------------------------------- */
#define POPWr(rd)			x86_POPWr(_jit, rd)
__jit_inline void
x86_POPWr(jit_state_t _jit, jit_gpr_t rd)
{
    _d16();
    x86_pop_sil_r(_jit, rd);
}

#define POPWm(md, rb, ri, ms)		x86_POPWm(_jit, md, rb, ri, ms)
__jit_inline void
x86_POPWm(jit_state_t _jit, long md, jit_gpr_t rb, jit_gpr_t ri, jit_scl_t ms)
{
    _d16();
    x86_pop_sil_m(_jit, md, rb, ri, ms);
}

#define POPLr(rd)			x86_POPLr(_jit, rd)
__jit_inline void
x86_POPLr(jit_state_t _jit, jit_gpr_t rd)
{
    x86_pop_sil_r(_jit, rd);
}

#define POPLm(md, rb, ri, ms)		x86_POPLm(_jit, md, rb, ri, ms)
__jit_inline void
x86_POPLm(jit_state_t _jit, long md, jit_gpr_t rb, jit_gpr_t ri, jit_scl_t ms)
{
    x86_pop_sil_m(_jit, md, rb, ri, ms);
}

#define PUSHWr(rd)			x86_PUSHWr(_jit, rd)
__jit_inline void
x86_PUSHWr(jit_state_t _jit, jit_gpr_t rs)
{
    _d16();
    x86_push_sil_r(_jit, rs);
}

#define PUSHWm(md, rb, ri, ms)		x86_PUSHWm(_jit, md, rb, ri, ms)
__jit_inline void
x86_PUSHWm(jit_state_t _jit, long md, jit_gpr_t rb, jit_gpr_t ri, jit_scl_t ms)
{
    _d16();
    x86_push_sil_m(_jit, md, rb, ri, ms);
}

#define PUSHWi(im)			x86_PUSHWi(_jit, im)
__jit_inline void
x86_PUSHWi(jit_state_t _jit, long im)
{
    if (_s8P(im))
	x86_push_c_i(_jit, im);
    else {
	_d16();
	_O(0x68);
	_jit_W(_s16(im));
    }
}

#define PUSHLr(rd)			x86_PUSHLr(_jit, rd)
__jit_inline void
x86_PUSHLr(jit_state_t _jit, jit_gpr_t rs)
{
    x86_push_sil_r(_jit, rs);
}

#define PUSHLm(md, rb, ri, ms)		x86_PUSHLm(_jit, md, rb, ri, ms)
__jit_inline void
x86_PUSHLm(jit_state_t _jit, long md, jit_gpr_t rb, jit_gpr_t ri, jit_scl_t ms)
{
    x86_push_sil_m(_jit, md, rb, ri, ms);
}

#define PUSHLi(im)			x86_PUSHLi(_jit, im)
__jit_inline void
x86_PUSHLi(jit_state_t _jit, long im)
{
    x86_push_il_i(_jit, im);
}

#endif	/* LIGHTNING_DEBUG */
#endif	/* __lightning_asm_h */
