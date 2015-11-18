/*
	Copyright (C) 2008-2010 DeSmuME team

	This file is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 2 of the License, or
	(at your option) any later version.

	This file is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with the this software.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "ArmAnalyze.h"
#include "armcpu.h"
#include "instructions.h"
#include "Disassembler.h"
#include <assert.h>

///////////////////////////////////////////////////////////////////////////////////////////////
typedef u32 (FASTCALL* OpDecoder)(armcpu_t *armcpu, const OPCODE opcode, struct _Decoded* d);
#define TEMPLATE template<int PROCNUM> 
///////////////////////////////////////////////////////////////////////////////////////////////
#define DCL_UNDEF_OP(name) \
	TEMPLATE u32 FASTCALL name(armcpu_t *armcpu, const OPCODE opcode, struct _Decoded* d) \
	{\
		d->IROp = IR_UND;\
		d->ExecuteCycles = 1;\
		return 1;\
	}

#define DCL_OP1_ARG4(name, op, arg1, arg2, arg3, arg4) \
	TEMPLATE u32 FASTCALL name(armcpu_t *armcpu, const OPCODE opcode, struct _Decoded* d) \
	{\
		op(arg1, arg2, arg3, arg4)\
		return 1;\
	}

#define DCL_OP1_ARG5(name, op, arg1, arg2, arg3, arg4, arg5) \
	TEMPLATE u32 FASTCALL name(armcpu_t *armcpu, const OPCODE opcode, struct _Decoded* d) \
	{\
		op(arg1, arg2, arg3, arg4, arg5)\
		return 1;\
	}

#define DCL_OP1_ARG6(name, op, arg1, arg2, arg3, arg4, arg5, arg6) \
	TEMPLATE u32 FASTCALL name(armcpu_t *armcpu, const OPCODE opcode, struct _Decoded* d) \
	{\
		op(arg1, arg2, arg3, arg4, arg5, arg6)\
		return 1;\
	}

#define DCL_OP2_ARG1(name, op1, op2, arg) \
	TEMPLATE u32 FASTCALL name(armcpu_t *armcpu, const OPCODE opcode, struct _Decoded* d) \
	{\
		op1\
		op2(arg)\
		return 1;\
	}

#define DCL_OP2_ARG2(name, op1, op2, arg1, arg2) \
	TEMPLATE u32 FASTCALL name(armcpu_t *armcpu, const OPCODE opcode, struct _Decoded* d) \
	{\
		op1\
		op2(arg1, arg2)\
		return 1;\
	}

#define DCL_OP2_ARG4(name, op1, op2, arg1, arg2, arg3, arg4) \
	TEMPLATE u32 FASTCALL name(armcpu_t *armcpu, const OPCODE opcode, struct _Decoded* d) \
	{\
		op1\
		op2(arg1, arg2, arg3, arg4)\
		return 1;\
	}

#define DCL_OP2_ARG5(name, op1, op2, arg1, arg2, arg3, arg4, arg5) \
	TEMPLATE u32 FASTCALL name(armcpu_t *armcpu, const OPCODE opcode, struct _Decoded* d) \
	{\
		op1\
		op2(arg1, arg2, arg3, arg4, arg5)\
		return 1;\
	}

#define DCL_OP2_ARG6(name, op1, op2, arg1, arg2, arg3, arg4, arg5, arg6) \
	TEMPLATE u32 FASTCALL name(armcpu_t *armcpu, const OPCODE opcode, struct _Decoded* d) \
	{\
		op1\
		op2(arg1, arg2, arg3, arg4, arg5, arg6)\
		return 1;\
	}

#define THUMB_REGPOS(i, n)	(((i)>>n)&0x7)
#define ARM_REGPOS(i, n)	(((i)>>n)&0xF)

#define LOAD_CPSR \
	d->FlagsSet |= ALL_FLAGS;\
	d->TbitModified = 1;\
	d->Reschedule = 1;
///////////////////////////////////////////////////////////////////////////////////////////////
//------------------------------------------------------------
//                         THUMB
//------------------------------------------------------------
namespace ThumbOpDecoder
{
//-----------------------------------------------------------------------------
//   Undefined instruction
//-----------------------------------------------------------------------------
	DCL_UNDEF_OP(OP_UND_THUMB)

//-----------------------------------------------------------------------------
//   LSL
//-----------------------------------------------------------------------------
	TEMPLATE u32 FASTCALL OP_LSL_0(armcpu_t *armcpu, const OPCODE opcode, struct _Decoded* d)
	{
		d->IROp = IR_MOV;
		d->Rd = THUMB_REGPOS(opcode.ThumbOp, 0);
		d->Rm = THUMB_REGPOS(opcode.ThumbOp, 3);
		d->Rn = 0;
		d->Immediate = 0;
		d->I = 0;
		d->R = 0;
		d->Typ = IRSHIFT_LSL;
		d->S = 1;
		d->FlagsSet |= FLAG_N | FLAG_Z;
		d->ExecuteCycles = 1;
		return 1;
	}

	TEMPLATE u32 FASTCALL OP_LSL(armcpu_t *armcpu, const OPCODE opcode, struct _Decoded* d)
	{
		d->IROp = IR_MOV;
		d->Rd = THUMB_REGPOS(opcode.ThumbOp, 0);
		d->Rm = THUMB_REGPOS(opcode.ThumbOp, 3);
		d->Rn = 0;
		d->Immediate = (opcode.ThumbOp>>6) & 0x1F;
		d->I = 0;
		d->R = 0;
		d->Typ = IRSHIFT_LSL;
		d->S = 1;
		if (!d->Immediate)
			d->FlagsNeeded |= FLAG_C;
		d->FlagsSet |= FLAG_N | FLAG_Z;
		d->ExecuteCycles = 1;
		return 1;
	}

	TEMPLATE u32 FASTCALL OP_LSL_REG(armcpu_t *armcpu, const OPCODE opcode, struct _Decoded* d)
	{
		d->IROp = IR_MOV;
		d->Rd = THUMB_REGPOS(opcode.ThumbOp, 0);
		d->Rm = d->Rd;
		d->Rs = THUMB_REGPOS(opcode.ThumbOp, 3);
		d->Rn = 0;
		d->I = 0;
		d->R = 1;
		d->Typ = IRSHIFT_LSL;
		d->S = 1;
		d->FlagsSet |= FLAG_C | FLAG_N | FLAG_Z;
		d->ExecuteCycles = 2;
		return 1;
	}

//-----------------------------------------------------------------------------
//   LSR
//-----------------------------------------------------------------------------
	TEMPLATE u32 FASTCALL OP_LSR_0(armcpu_t *armcpu, const OPCODE opcode, struct _Decoded* d)
	{
		d->IROp = IR_MOV;
		d->Rd = THUMB_REGPOS(opcode.ThumbOp, 0);
		d->Rm = THUMB_REGPOS(opcode.ThumbOp, 3);
		d->Rn = 0;
		d->Immediate = 0;
		d->I = 0;
		d->R = 0;
		d->Typ = IRSHIFT_LSR;
		d->S = 1;
		d->FlagsSet |= FLAG_C | FLAG_N | FLAG_Z;
		d->ExecuteCycles = 1;
		return 1;
	}

	TEMPLATE u32 FASTCALL OP_LSR(armcpu_t *armcpu, const OPCODE opcode, struct _Decoded* d)
	{
		d->IROp = IR_MOV;
		d->Rd = THUMB_REGPOS(opcode.ThumbOp, 0);
		d->Rm = THUMB_REGPOS(opcode.ThumbOp, 3);
		d->Rn = 0;
		d->Immediate = (opcode.ThumbOp>>6) & 0x1F;
		d->I = 0;
		d->R = 0;
		d->Typ = IRSHIFT_LSR;
		d->S = 1;
		d->FlagsSet |= FLAG_C | FLAG_N | FLAG_Z;
		d->ExecuteCycles = 1;
		return 1;
	}

	TEMPLATE u32 FASTCALL OP_LSR_REG(armcpu_t *armcpu, const OPCODE opcode, struct _Decoded* d)
	{
		d->IROp = IR_MOV;
		d->Rd = THUMB_REGPOS(opcode.ThumbOp, 0);
		d->Rm = d->Rd;
		d->Rs = THUMB_REGPOS(opcode.ThumbOp, 3);
		d->Rn = 0;
		d->I = 0;
		d->R = 1;
		d->Typ = IRSHIFT_LSR;
		d->S = 1;
		d->FlagsSet |= FLAG_C | FLAG_N | FLAG_Z;
		d->ExecuteCycles = 2;
		return 1;
	}

//-----------------------------------------------------------------------------
//   ASR
//-----------------------------------------------------------------------------
	TEMPLATE u32 FASTCALL OP_ASR_0(armcpu_t *armcpu, const OPCODE opcode, struct _Decoded* d)
	{
		d->IROp = IR_MOV;
		d->Rd = THUMB_REGPOS(opcode.ThumbOp, 0);
		d->Rm = THUMB_REGPOS(opcode.ThumbOp, 3);
		d->Rn = 0;
		d->Immediate = 0;
		d->I = 0;
		d->R = 0;
		d->Typ = IRSHIFT_ASR;
		d->S = 1;
		d->FlagsSet |= FLAG_C | FLAG_N | FLAG_Z;
		d->ExecuteCycles = 1;
		return 1;
	}

	TEMPLATE u32 FASTCALL OP_ASR(armcpu_t *armcpu, const OPCODE opcode, struct _Decoded* d)
	{
		d->IROp = IR_MOV;
		d->Rd = THUMB_REGPOS(opcode.ThumbOp, 0);
		d->Rm = THUMB_REGPOS(opcode.ThumbOp, 3);
		d->Rn = 0;
		d->Immediate = (opcode.ThumbOp>>6) & 0x1F;
		d->I = 0;
		d->R = 0;
		d->Typ = IRSHIFT_ASR;
		d->S = 1;
		d->FlagsSet |= FLAG_C | FLAG_N | FLAG_Z;
		d->ExecuteCycles = 1;
		return 1;
	}

	TEMPLATE u32 FASTCALL OP_ASR_REG(armcpu_t *armcpu, const OPCODE opcode, struct _Decoded* d)
	{
		d->IROp = IR_MOV;
		d->Rd = THUMB_REGPOS(opcode.ThumbOp, 0);
		d->Rm = d->Rd;
		d->Rs = THUMB_REGPOS(opcode.ThumbOp, 3);
		d->Rn = 0;
		d->I = 0;
		d->R = 1;
		d->Typ = IRSHIFT_ASR;
		d->S = 1;
		d->FlagsSet |= FLAG_C | FLAG_N | FLAG_Z;
		d->ExecuteCycles = 2;
		return 1;
	}

//-----------------------------------------------------------------------------
//   ADD
//-----------------------------------------------------------------------------
	TEMPLATE u32 FASTCALL OP_ADD_IMM3(armcpu_t *armcpu, const OPCODE opcode, struct _Decoded* d)
	{
		d->IROp = IR_ADD;
		d->Rd = THUMB_REGPOS(opcode.ThumbOp, 0);
		d->Rn = THUMB_REGPOS(opcode.ThumbOp, 3);
		d->Immediate = (opcode.ThumbOp >> 6) & 0x07;
		d->I = 1;
		d->S = 1;
		d->FlagsSet |= ALL_FLAGS;
		d->ExecuteCycles = 1;
		return 1;
	}

	TEMPLATE u32 FASTCALL OP_ADD_IMM8(armcpu_t *armcpu, const OPCODE opcode, struct _Decoded* d)
	{
		d->IROp = IR_ADD;
		d->Rd = THUMB_REGPOS(opcode.ThumbOp, 8);
		d->Rn = d->Rd;
		d->Immediate = (opcode.ThumbOp & 0xFF);
		d->I = 1;
		d->S = 1;
		d->FlagsSet |= ALL_FLAGS;
		d->ExecuteCycles = 1;
		return 1;
	}

	TEMPLATE u32 FASTCALL OP_ADD_REG(armcpu_t *armcpu, const OPCODE opcode, struct _Decoded* d)
	{
		d->IROp = IR_ADD;
		d->Rd = THUMB_REGPOS(opcode.ThumbOp, 0);
		d->Rn = THUMB_REGPOS(opcode.ThumbOp, 3);
		d->Rm = THUMB_REGPOS(opcode.ThumbOp, 6);
		d->Immediate = 0;
		d->I = 0;
		d->R = 0;
		d->Typ = IRSHIFT_LSL;
		d->S = 1;
		d->FlagsSet |= ALL_FLAGS;
		d->ExecuteCycles = 1;
		return 1;
	}

	TEMPLATE u32 FASTCALL OP_ADD_SPE(armcpu_t *armcpu, const OPCODE opcode, struct _Decoded* d)
	{
		d->IROp = IR_ADD;
		d->Rd = (THUMB_REGPOS(opcode.ThumbOp, 0) | ((opcode.ThumbOp>>4)&8));
		d->Rn = d->Rd;
		d->Rm = ARM_REGPOS(opcode.ThumbOp, 3);
		d->Immediate = 0;
		d->I = 0;
		d->R = 0;
		d->Typ = IRSHIFT_LSL;
		d->S = 0;
		if (d->Rd == 15)
		{
			d->R15Modified = 1;
			d->ExecuteCycles = 1;
		}
		else
			d->ExecuteCycles = 3;
		return 1;
	}

	TEMPLATE u32 FASTCALL OP_ADD_2PC(armcpu_t *armcpu, const OPCODE opcode, struct _Decoded* d)
	{
		d->IROp = IR_ADD;
		d->Rd = THUMB_REGPOS(opcode.ThumbOp, 8);
		d->Rn = 15;
		d->Immediate = ((opcode.ThumbOp&0xFF)<<2);
		d->I = 1;
		d->S = 0;
		d->ReadPCMask = 0xFFFFFFFC;
		d->R15Modified = 1;
		d->ExecuteCycles = 1;
		return 1;
	}

	TEMPLATE u32 FASTCALL OP_ADD_2SP(armcpu_t *armcpu, const OPCODE opcode, struct _Decoded* d)
	{
		d->IROp = IR_ADD;
		d->Rd = THUMB_REGPOS(opcode.ThumbOp, 8);
		d->Rn = 13;
		d->Immediate = ((opcode.ThumbOp&0xFF)<<2);
		d->I = 1;
		d->S = 0;
		d->ExecuteCycles = 1;
		return 1;
	}

//-----------------------------------------------------------------------------
//   SUB
//-----------------------------------------------------------------------------
	TEMPLATE u32 FASTCALL OP_SUB_IMM3(armcpu_t *armcpu, const OPCODE opcode, struct _Decoded* d)
	{
		d->IROp = IR_SUB;
		d->Rd = THUMB_REGPOS(opcode.ThumbOp, 0);
		d->Rn = THUMB_REGPOS(opcode.ThumbOp, 3);
		d->Immediate = (opcode.ThumbOp >> 6) & 0x07;
		d->I = 1;
		d->S = 1;
		d->FlagsSet |= ALL_FLAGS;
		d->ExecuteCycles = 1;
		return 1;
	}

	TEMPLATE u32 FASTCALL OP_SUB_IMM8(armcpu_t *armcpu, const OPCODE opcode, struct _Decoded* d)
	{
		d->IROp = IR_SUB;
		d->Rd = THUMB_REGPOS(opcode.ThumbOp, 8);
		d->Rn = d->Rd;
		d->Immediate = (opcode.ThumbOp & 0xFF);
		d->I = 1;
		d->S = 1;
		d->FlagsSet |= ALL_FLAGS;
		d->ExecuteCycles = 1;
		return 1;
	}

	TEMPLATE u32 FASTCALL OP_SUB_REG(armcpu_t *armcpu, const OPCODE opcode, struct _Decoded* d)
	{
		d->IROp = IR_SUB;
		d->Rd = THUMB_REGPOS(opcode.ThumbOp, 0);
		d->Rn = THUMB_REGPOS(opcode.ThumbOp, 3);
		d->Rm = THUMB_REGPOS(opcode.ThumbOp, 6);
		d->Immediate = 0;
		d->I = 0;
		d->R = 0;
		d->Typ = IRSHIFT_LSL;
		d->S = 1;
		d->FlagsSet |= ALL_FLAGS;
		d->ExecuteCycles = 1;
		return 1;
	}

//-----------------------------------------------------------------------------
//   MOV
//-----------------------------------------------------------------------------
	TEMPLATE u32 FASTCALL OP_MOV_IMM8(armcpu_t *armcpu, const OPCODE opcode, struct _Decoded* d)
	{
		d->IROp = IR_MOV;
		d->Rd = THUMB_REGPOS(opcode.ThumbOp, 8);
		d->Rn = 0;
		d->Immediate = (opcode.ThumbOp & 0xFF);
		d->I = 1;
		d->S = 1;
		d->FlagsSet |= FLAG_N | FLAG_Z;
		d->ExecuteCycles = 1;
		return 1;
	}

	TEMPLATE u32 FASTCALL OP_MOV_SPE(armcpu_t *armcpu, const OPCODE opcode, struct _Decoded* d)
	{
		if (opcode.ThumbOp == 0x46C0)
		{
			d->IROp = IR_NOP;
			d->ExecuteCycles = 1;
			return 1;
		}

		d->IROp = IR_MOV;
		d->Rd = (THUMB_REGPOS(opcode.ThumbOp, 0) | ((opcode.ThumbOp>>4)&8));
		d->Rn = 0;
		d->Rm = ARM_REGPOS(opcode.ThumbOp, 3);
		d->Immediate = 0;
		d->I = 0;
		d->R = 0;
		d->Typ = IRSHIFT_LSL;
		d->S = 0;
		if (d->Rd == 15)
		{
			d->R15Modified = 1;
			d->ExecuteCycles = 1;
		}
		else
			d->ExecuteCycles = 3;
		return 1;
	}

//-----------------------------------------------------------------------------
//   CMP
//-----------------------------------------------------------------------------
	TEMPLATE u32 FASTCALL OP_CMP_IMM8(armcpu_t *armcpu, const OPCODE opcode, struct _Decoded* d)
	{
		d->IROp = IR_CMP;
		d->Rd = 0;
		d->Rn = THUMB_REGPOS(opcode.ThumbOp, 8);
		d->Immediate = (opcode.ThumbOp & 0xFF);
		d->I = 1;
		d->S = 1;
		d->FlagsSet |= ALL_FLAGS;
		d->ExecuteCycles = 1;
		return 1;
	}

	TEMPLATE u32 FASTCALL OP_CMP(armcpu_t *armcpu, const OPCODE opcode, struct _Decoded* d)
	{
		d->IROp = IR_CMP;
		d->Rd = 0;
		d->Rn = THUMB_REGPOS(opcode.ThumbOp, 0);
		d->Rm = THUMB_REGPOS(opcode.ThumbOp, 3);
		d->Immediate = 0;
		d->I = 0;
		d->R = 0;
		d->Typ = IRSHIFT_LSL;
		d->S = 1;
		d->FlagsSet |= ALL_FLAGS;
		d->ExecuteCycles = 1;
		return 1;
	}

	TEMPLATE u32 FASTCALL OP_CMP_SPE(armcpu_t *armcpu, const OPCODE opcode, struct _Decoded* d)
	{
		d->IROp = IR_CMP;
		d->Rd = 0;
		d->Rn = (THUMB_REGPOS(opcode.ThumbOp, 0) | ((opcode.ThumbOp>>4)&8));
		d->Rm = ARM_REGPOS(opcode.ThumbOp, 3);
		d->Immediate = 0;
		d->I = 0;
		d->R = 0;
		d->Typ = IRSHIFT_LSL;
		d->S = 0;
		d->FlagsSet |= ALL_FLAGS;
		d->ExecuteCycles = 1;
		return 1;
	}

//-----------------------------------------------------------------------------
//   AND
//-----------------------------------------------------------------------------
	TEMPLATE u32 FASTCALL OP_AND(armcpu_t *armcpu, const OPCODE opcode, struct _Decoded* d)
	{
		d->IROp = IR_AND;
		d->Rd = THUMB_REGPOS(opcode.ThumbOp, 0);
		d->Rn = d->Rd;
		d->Rm = THUMB_REGPOS(opcode.ThumbOp, 3);
		d->Immediate = 0;
		d->I = 0;
		d->R = 0;
		d->Typ = IRSHIFT_LSL;
		d->S = 1;
		d->FlagsSet |= FLAG_N | FLAG_Z;
		d->ExecuteCycles = 1;
		return 1;
	}

//-----------------------------------------------------------------------------
//   EOR
//-----------------------------------------------------------------------------
	TEMPLATE u32 FASTCALL OP_EOR(armcpu_t *armcpu, const OPCODE opcode, struct _Decoded* d)
	{
		d->IROp = IR_EOR;
		d->Rd = THUMB_REGPOS(opcode.ThumbOp, 0);
		d->Rn = d->Rd;
		d->Rm = THUMB_REGPOS(opcode.ThumbOp, 3);
		d->Immediate = 0;
		d->I = 0;
		d->R = 0;
		d->Typ = IRSHIFT_LSL;
		d->S = 1;
		d->FlagsSet |= FLAG_N | FLAG_Z;
		d->ExecuteCycles = 1;
		return 1;
	}

//-----------------------------------------------------------------------------
//   ADC
//-----------------------------------------------------------------------------
	TEMPLATE u32 FASTCALL OP_ADC_REG(armcpu_t *armcpu, const OPCODE opcode, struct _Decoded* d)
	{
		d->IROp = IR_ADC;
		d->Rd = THUMB_REGPOS(opcode.ThumbOp, 0);
		d->Rn = d->Rd;
		d->Rm = THUMB_REGPOS(opcode.ThumbOp, 3);
		d->Immediate = 0;
		d->I = 0;
		d->R = 0;
		d->Typ = IRSHIFT_LSL;
		d->S = 1;
		d->FlagsNeeded |= FLAG_C;
		d->FlagsSet |= ALL_FLAGS;
		d->ExecuteCycles = 1;
		return 1;
	}

//-----------------------------------------------------------------------------
//   SBC
//-----------------------------------------------------------------------------
	TEMPLATE u32 FASTCALL OP_SBC_REG(armcpu_t *armcpu, const OPCODE opcode, struct _Decoded* d)
	{
		d->IROp = IR_SBC;
		d->Rd = THUMB_REGPOS(opcode.ThumbOp, 0);
		d->Rn = d->Rd;
		d->Rm = THUMB_REGPOS(opcode.ThumbOp, 3);
		d->Immediate = 0;
		d->I = 0;
		d->R = 0;
		d->Typ = IRSHIFT_LSL;
		d->S = 1;
		d->FlagsNeeded |= FLAG_C;
		d->FlagsSet |= ALL_FLAGS;
		d->ExecuteCycles = 1;
		return 1;
	}

//-----------------------------------------------------------------------------
//   ROR
//-----------------------------------------------------------------------------
	TEMPLATE u32 FASTCALL OP_ROR_REG(armcpu_t *armcpu, const OPCODE opcode, struct _Decoded* d)
	{
		d->IROp = IR_MOV;
		d->Rd = THUMB_REGPOS(opcode.ThumbOp, 0);
		d->Rm = d->Rd;
		d->Rs = THUMB_REGPOS(opcode.ThumbOp, 3);
		d->Rn = 0;
		d->I = 0;
		d->R = 1;
		d->Typ = IRSHIFT_ROR;
		d->S = 1;
		d->FlagsSet |= FLAG_C | FLAG_N | FLAG_Z;
		d->ExecuteCycles = 2;
		return 1;
	}

//-----------------------------------------------------------------------------
//   TST
//-----------------------------------------------------------------------------
	TEMPLATE u32 FASTCALL OP_TST(armcpu_t *armcpu, const OPCODE opcode, struct _Decoded* d)
	{
		d->IROp = IR_TST;
		d->Rd = 0;
		d->Rn = THUMB_REGPOS(opcode.ThumbOp, 0);
		d->Rm = THUMB_REGPOS(opcode.ThumbOp, 3);
		d->Immediate = 0;
		d->I = 0;
		d->R = 0;
		d->Typ = IRSHIFT_LSL;
		d->S = 1;
		d->FlagsSet |= FLAG_N | FLAG_Z;
		d->ExecuteCycles = 1;
		return 1;
	}

//-----------------------------------------------------------------------------
//   NEG
//-----------------------------------------------------------------------------
	TEMPLATE u32 FASTCALL OP_NEG(armcpu_t *armcpu, const OPCODE opcode, struct _Decoded* d)
	{
		d->IROp = IR_RSB;
		d->Rd = THUMB_REGPOS(opcode.ThumbOp, 0);
		d->Rn = THUMB_REGPOS(opcode.ThumbOp, 3);
		d->Immediate = 0;
		d->I = 1;
		d->S = 1;
		d->FlagsSet |= ALL_FLAGS;
		d->ExecuteCycles = 1;
		return 1;
	}

//-----------------------------------------------------------------------------
//   CMN
//-----------------------------------------------------------------------------
	TEMPLATE u32 FASTCALL OP_CMN(armcpu_t *armcpu, const OPCODE opcode, struct _Decoded* d)
	{
		d->IROp = IR_CMN;
		d->Rd = 0;
		d->Rn = THUMB_REGPOS(opcode.ThumbOp, 0);
		d->Rm = THUMB_REGPOS(opcode.ThumbOp, 3);
		d->Immediate = 0;
		d->I = 0;
		d->R = 0;
		d->Typ = IRSHIFT_LSL;
		d->S = 1;
		d->FlagsSet |= ALL_FLAGS;
		d->ExecuteCycles = 1;
		return 1;
	}

//-----------------------------------------------------------------------------
//   ORR
//-----------------------------------------------------------------------------
	TEMPLATE u32 FASTCALL OP_ORR(armcpu_t *armcpu, const OPCODE opcode, struct _Decoded* d)
	{
		d->IROp = IR_ORR;
		d->Rd = THUMB_REGPOS(opcode.ThumbOp, 0);
		d->Rn = d->Rd;
		d->Rm = THUMB_REGPOS(opcode.ThumbOp, 3);
		d->Immediate = 0;
		d->I = 0;
		d->R = 0;
		d->Typ = IRSHIFT_LSL;
		d->S = 1;
		d->FlagsSet |= FLAG_N | FLAG_Z;
		d->ExecuteCycles = 1;
		return 1;
	}

//-----------------------------------------------------------------------------
//   BIC
//-----------------------------------------------------------------------------
	TEMPLATE u32 FASTCALL OP_BIC(armcpu_t *armcpu, const OPCODE opcode, struct _Decoded* d)
	{
		d->IROp = IR_BIC;
		d->Rd = THUMB_REGPOS(opcode.ThumbOp, 0);
		d->Rn = d->Rd;
		d->Rm = THUMB_REGPOS(opcode.ThumbOp, 3);
		d->Immediate = 0;
		d->I = 0;
		d->R = 0;
		d->Typ = IRSHIFT_LSL;
		d->S = 1;
		d->FlagsSet |= FLAG_N | FLAG_Z;
		d->ExecuteCycles = 1;
		return 1;
	}

//-----------------------------------------------------------------------------
//   MVN
//-----------------------------------------------------------------------------
	TEMPLATE u32 FASTCALL OP_MVN(armcpu_t *armcpu, const OPCODE opcode, struct _Decoded* d)
	{
		d->IROp = IR_MVN;
		d->Rd = THUMB_REGPOS(opcode.ThumbOp, 0);
		d->Rn = d->Rd;
		d->Rm = THUMB_REGPOS(opcode.ThumbOp, 3);
		d->Immediate = 0;
		d->I = 0;
		d->R = 0;
		d->Typ = IRSHIFT_LSL;
		d->S = 1;
		d->FlagsSet |= FLAG_N | FLAG_Z;
		d->ExecuteCycles = 1;
		return 1;
	}

//-----------------------------------------------------------------------------
//   MUL
//-----------------------------------------------------------------------------
	TEMPLATE u32 FASTCALL OP_MUL_REG(armcpu_t *armcpu, const OPCODE opcode, struct _Decoded* d)
	{
		d->IROp = IR_MUL;
		d->Rd = THUMB_REGPOS(opcode.ThumbOp, 0);
		d->Rs = THUMB_REGPOS(opcode.ThumbOp, 3);
		d->Rm = d->Rd;
		d->S = 1;
		d->FlagsSet |= FLAG_N | FLAG_Z;
		d->VariableCycles = 1;
		return 1;
	}

//-----------------------------------------------------------------------------
//   STRB / LDRB
//-----------------------------------------------------------------------------
	TEMPLATE u32 FASTCALL OP_STRB_IMM_OFF(armcpu_t *armcpu, const OPCODE opcode, struct _Decoded* d)
	{
		d->IROp = IR_STR;
		d->Rn = THUMB_REGPOS(opcode.ThumbOp, 3);
		d->Rd = THUMB_REGPOS(opcode.ThumbOp, 0);
		d->Immediate = (opcode.ThumbOp>>6)&0x1F;
		d->I = 1;
		d->P = 1;
		d->U = 1;
		d->B = 1;
		d->W = 0;
		d->ExecuteCycles = 2;
		d->VariableCycles = 1;
		d->Reschedule = 2;
		return 1;
	}

	TEMPLATE u32 FASTCALL OP_LDRB_IMM_OFF(armcpu_t *armcpu, const OPCODE opcode, struct _Decoded* d)
	{
		d->IROp = IR_LDR;
		d->Rn = THUMB_REGPOS(opcode.ThumbOp, 3);
		d->Rd = THUMB_REGPOS(opcode.ThumbOp, 0);
		d->Immediate = (opcode.ThumbOp>>6)&0x1F;
		d->I = 1;
		d->P = 1;
		d->U = 1;
		d->B = 1;
		d->W = 0;
		d->ExecuteCycles = 3;
		d->VariableCycles = 1;
		d->Reschedule = 2;
		return 1;
	}

	TEMPLATE u32 FASTCALL OP_STRB_REG_OFF(armcpu_t *armcpu, const OPCODE opcode, struct _Decoded* d)
	{
		d->IROp = IR_STR;
		d->Rn = THUMB_REGPOS(opcode.ThumbOp, 3);
		d->Rd = THUMB_REGPOS(opcode.ThumbOp, 0);
		d->Rm = THUMB_REGPOS(opcode.ThumbOp, 6);
		d->Immediate = 0;
		d->I = 0;
		d->R = 0;
		d->Typ = IRSHIFT_LSL;
		d->P = 1;
		d->U = 1;
		d->B = 1;
		d->W = 0;
		d->ExecuteCycles = 2;
		d->VariableCycles = 1;
		d->Reschedule = 2;
		return 1;
	}

	TEMPLATE u32 FASTCALL OP_LDRB_REG_OFF(armcpu_t *armcpu, const OPCODE opcode, struct _Decoded* d)
	{
		d->IROp = IR_LDR;
		d->Rn = THUMB_REGPOS(opcode.ThumbOp, 3);
		d->Rd = THUMB_REGPOS(opcode.ThumbOp, 0);
		d->Rm = THUMB_REGPOS(opcode.ThumbOp, 6);
		d->Immediate = 0;
		d->I = 0;
		d->R = 0;
		d->Typ = IRSHIFT_LSL;
		d->P = 1;
		d->U = 1;
		d->B = 1;
		d->W = 0;
		d->ExecuteCycles = 3;
		d->VariableCycles = 1;
		d->Reschedule = 2;
		return 1;
	}

//-----------------------------------------------------------------------------
//   LDRSB
//-----------------------------------------------------------------------------
	TEMPLATE u32 FASTCALL OP_LDRSB_REG_OFF(armcpu_t *armcpu, const OPCODE opcode, struct _Decoded* d)
	{
		d->IROp = IR_LDRx;
		d->Rn = THUMB_REGPOS(opcode.ThumbOp, 3);
		d->Rd = THUMB_REGPOS(opcode.ThumbOp, 0);
		d->Rm = THUMB_REGPOS(opcode.ThumbOp, 6);
		d->I = 0;
		d->P = 1;
		d->U = 1;
		d->W = 0;
		d->S = 1;
		d->H = 0;
		d->ExecuteCycles = 3;
		d->VariableCycles = 1;
		d->Reschedule = 2;
		return 1;
	}

//-----------------------------------------------------------------------------
//   STRH / LDRH
//-----------------------------------------------------------------------------
	TEMPLATE u32 FASTCALL OP_STRH_IMM_OFF(armcpu_t *armcpu, const OPCODE opcode, struct _Decoded* d)
	{
		d->IROp = IR_STRx;
		d->Rn = THUMB_REGPOS(opcode.ThumbOp, 3);
		d->Rd = THUMB_REGPOS(opcode.ThumbOp, 0);
		d->Immediate = (opcode.ThumbOp>>5)&0x3E;
		d->I = 1;
		d->P = 1;
		d->U = 1;
		d->W = 0;
		d->S = 0;
		d->H = 1;
		d->ExecuteCycles = 2;
		d->VariableCycles = 1;
		d->Reschedule = 2;
		return 1;
	}

	TEMPLATE u32 FASTCALL OP_LDRH_IMM_OFF(armcpu_t *armcpu, const OPCODE opcode, struct _Decoded* d)
	{
		d->IROp = IR_LDRx;
		d->Rn = THUMB_REGPOS(opcode.ThumbOp, 3);
		d->Rd = THUMB_REGPOS(opcode.ThumbOp, 0);
		d->Immediate = (opcode.ThumbOp>>5)&0x3E;
		d->I = 1;
		d->P = 1;
		d->U = 1;
		d->W = 0;
		d->S = 0;
		d->H = 1;
		d->ExecuteCycles = 3;
		d->VariableCycles = 1;
		d->Reschedule = 2;
		return 1;
	}

	TEMPLATE u32 FASTCALL OP_STRH_REG_OFF(armcpu_t *armcpu, const OPCODE opcode, struct _Decoded* d)
	{
		d->IROp = IR_STRx;
		d->Rn = THUMB_REGPOS(opcode.ThumbOp, 3);
		d->Rd = THUMB_REGPOS(opcode.ThumbOp, 0);
		d->Rm = THUMB_REGPOS(opcode.ThumbOp, 6);
		d->I = 0;
		d->P = 1;
		d->U = 1;
		d->W = 0;
		d->S = 0;
		d->H = 1;
		d->ExecuteCycles = 2;
		d->VariableCycles = 1;
		d->Reschedule = 2;
		return 1;
	}

	TEMPLATE u32 FASTCALL OP_LDRH_REG_OFF(armcpu_t *armcpu, const OPCODE opcode, struct _Decoded* d)
	{
		d->IROp = IR_LDRx;
		d->Rn = THUMB_REGPOS(opcode.ThumbOp, 3);
		d->Rd = THUMB_REGPOS(opcode.ThumbOp, 0);
		d->Rm = THUMB_REGPOS(opcode.ThumbOp, 6);
		d->I = 0;
		d->P = 1;
		d->U = 1;
		d->W = 0;
		d->S = 0;
		d->H = 1;
		d->ExecuteCycles = 3;
		d->VariableCycles = 1;
		d->Reschedule = 2;
		return 1;
	}

//-----------------------------------------------------------------------------
//   LDRSH
//-----------------------------------------------------------------------------
	TEMPLATE u32 FASTCALL OP_LDRSH_REG_OFF(armcpu_t *armcpu, const OPCODE opcode, struct _Decoded* d)
	{
		d->IROp = IR_LDRx;
		d->Rn = THUMB_REGPOS(opcode.ThumbOp, 3);
		d->Rd = THUMB_REGPOS(opcode.ThumbOp, 0);
		d->Rm = THUMB_REGPOS(opcode.ThumbOp, 6);
		d->I = 0;
		d->P = 1;
		d->U = 1;
		d->W = 0;
		d->S = 1;
		d->H = 1;
		d->ExecuteCycles = 3;
		d->VariableCycles = 1;
		d->Reschedule = 2;
		return 1;
	}

//-----------------------------------------------------------------------------
//   STR / LDR
//-----------------------------------------------------------------------------
	TEMPLATE u32 FASTCALL OP_STR_IMM_OFF(armcpu_t *armcpu, const OPCODE opcode, struct _Decoded* d)
	{
		d->IROp = IR_STR;
		d->Rn = THUMB_REGPOS(opcode.ThumbOp, 3);
		d->Rd = THUMB_REGPOS(opcode.ThumbOp, 0);
		d->Immediate = (opcode.ThumbOp>>4)&0x7C;
		d->I = 1;
		d->P = 1;
		d->U = 1;
		d->B = 0;
		d->W = 0;
		d->ExecuteCycles = 2;
		d->VariableCycles = 1;
		d->Reschedule = 2;
		return 1;
	}

	TEMPLATE u32 FASTCALL OP_LDR_IMM_OFF(armcpu_t *armcpu, const OPCODE opcode, struct _Decoded* d)
	{
		d->IROp = IR_LDR;
		d->Rn = THUMB_REGPOS(opcode.ThumbOp, 3);
		d->Rd = THUMB_REGPOS(opcode.ThumbOp, 0);
		d->Immediate = (opcode.ThumbOp>>4)&0x7C;
		d->I = 1;
		d->P = 1;
		d->U = 1;
		d->B = 0;
		d->W = 0;
		d->ExecuteCycles = 3;
		d->VariableCycles = 1;
		d->Reschedule = 2;
		return 1;
	}

	TEMPLATE u32 FASTCALL OP_STR_REG_OFF(armcpu_t *armcpu, const OPCODE opcode, struct _Decoded* d)
	{
		d->IROp = IR_STR;
		d->Rn = THUMB_REGPOS(opcode.ThumbOp, 3);
		d->Rd = THUMB_REGPOS(opcode.ThumbOp, 0);
		d->Rm = THUMB_REGPOS(opcode.ThumbOp, 6);
		d->Immediate = 0;
		d->I = 0;
		d->R = 0;
		d->Typ = IRSHIFT_LSL;
		d->P = 1;
		d->U = 1;
		d->B = 0;
		d->W = 0;
		d->ExecuteCycles = 2;
		d->VariableCycles = 1;
		d->Reschedule = 2;
		return 1;
	}

	TEMPLATE u32 FASTCALL OP_LDR_REG_OFF(armcpu_t *armcpu, const OPCODE opcode, struct _Decoded* d)
	{
		d->IROp = IR_LDR;
		d->Rn = THUMB_REGPOS(opcode.ThumbOp, 3);
		d->Rd = THUMB_REGPOS(opcode.ThumbOp, 0);
		d->Rm = THUMB_REGPOS(opcode.ThumbOp, 6);
		d->Immediate = 0;
		d->I = 0;
		d->R = 0;
		d->Typ = IRSHIFT_LSL;
		d->P = 1;
		d->U = 1;
		d->B = 0;
		d->W = 0;
		d->ExecuteCycles = 3;
		d->VariableCycles = 1;
		d->Reschedule = 2;
		return 1;
	}

	TEMPLATE u32 FASTCALL OP_STR_SPREL(armcpu_t *armcpu, const OPCODE opcode, struct _Decoded* d)
	{
		d->IROp = IR_STR;
		d->Rn = 13;
		d->Rd = THUMB_REGPOS(opcode.ThumbOp, 8);
		d->Immediate = (opcode.ThumbOp&0xFF)<<2;
		d->I = 1;
		d->P = 1;
		d->U = 1;
		d->B = 0;
		d->W = 0;
		d->ExecuteCycles = 2;
		d->VariableCycles = 1;
		d->Reschedule = 2;
		return 1;
	}

	TEMPLATE u32 FASTCALL OP_LDR_SPREL(armcpu_t *armcpu, const OPCODE opcode, struct _Decoded* d)
	{
		d->IROp = IR_LDR;
		d->Rn = 13;
		d->Rd = THUMB_REGPOS(opcode.ThumbOp, 8);
		d->Immediate = (opcode.ThumbOp&0xFF)<<2;
		d->I = 1;
		d->P = 1;
		d->U = 1;
		d->B = 0;
		d->W = 0;
		d->ExecuteCycles = 3;
		d->VariableCycles = 1;
		d->Reschedule = 2;
		return 1;
	}

	TEMPLATE u32 FASTCALL OP_LDR_PCREL(armcpu_t *armcpu, const OPCODE opcode, struct _Decoded* d)
	{
		d->IROp = IR_LDR;
		d->Rn = 15;
		d->Rd = THUMB_REGPOS(opcode.ThumbOp, 8);
		d->Immediate = (opcode.ThumbOp&0xFF)<<2;
		d->I = 1;
		d->P = 1;
		d->U = 1;
		d->B = 0;
		d->W = 0;
		d->ReadPCMask = 0xFFFFFFFC;
		d->ExecuteCycles = 3;
		d->VariableCycles = 1;
		d->Reschedule = 2;
		d->Reschedule = 2;
		return 1;
	}

//-----------------------------------------------------------------------------
//   Adjust SP
//-----------------------------------------------------------------------------
	TEMPLATE u32 FASTCALL OP_ADJUST_P_SP(armcpu_t *armcpu, const OPCODE opcode, struct _Decoded* d)
	{
		d->IROp = IR_ADD;
		d->Rd = 13;
		d->Rn = 13;
		d->Immediate = (opcode.ThumbOp&0x7F)<<2;
		d->I = 1;
		d->S = 0;
		d->ExecuteCycles = 1;
		return 1;
	}

	TEMPLATE u32 FASTCALL OP_ADJUST_M_SP(armcpu_t *armcpu, const OPCODE opcode, struct _Decoded* d)
	{
		d->IROp = IR_SUB;
		d->Rd = 13;
		d->Rn = 13;
		d->Immediate = (opcode.ThumbOp&0x7F)<<2;
		d->I = 1;
		d->S = 0;
		d->ExecuteCycles = 1;
		return 1;
	}

//-----------------------------------------------------------------------------
//   PUSH / POP
//-----------------------------------------------------------------------------
	TEMPLATE u32 FASTCALL OP_PUSH(armcpu_t *armcpu, const OPCODE opcode, struct _Decoded* d)
	{
		d->IROp = IR_STM;
		d->Rn = 13;
		d->RegisterList = (opcode.ThumbOp & 0xFF);
		d->P = 1;
		d->U = 0;
		d->S = 0;
		d->W = 1;
		d->ExecuteCycles = 3;
		d->VariableCycles = 1;
		d->Reschedule = 2;
		return 1;
	}

	TEMPLATE u32 FASTCALL OP_PUSH_LR(armcpu_t *armcpu, const OPCODE opcode, struct _Decoded* d)
	{
		d->IROp = IR_STM;
		d->Rn = 13;
		d->RegisterList = (opcode.ThumbOp & 0xFF) | (1 << 14);
		d->P = 1;
		d->U = 0;
		d->S = 0;
		d->W = 1;
		d->ExecuteCycles = 4;
		d->VariableCycles = 1;
		d->Reschedule = 2;
		return 1;
	}

	TEMPLATE u32 FASTCALL OP_POP(armcpu_t *armcpu, const OPCODE opcode, struct _Decoded* d)
	{
		d->IROp = IR_LDM;
		d->Rn = 13;
		d->RegisterList = (opcode.ThumbOp & 0xFF);
		d->P = 0;
		d->U = 1;
		d->S = 0;
		d->W = 1;
		d->ExecuteCycles = 2;
		d->VariableCycles = 1;
		d->Reschedule = 2;
		return 1;
	}

	TEMPLATE u32 FASTCALL OP_POP_PC(armcpu_t *armcpu, const OPCODE opcode, struct _Decoded* d)
	{
		d->IROp = IR_LDM;
		d->Rn = 13;
		d->RegisterList = (opcode.ThumbOp & 0xFF) | (1 << 15);
		d->P = 0;
		d->U = 1;
		d->S = 0;
		d->W = 1;
		d->ExecuteCycles = 5;
		d->VariableCycles = 1;
		d->R15Modified = 1;
		if (PROCNUM == 0) d->TbitModified = 1;
		d->Reschedule = 2;
		return 1;
	}

//-----------------------------------------------------------------------------
//   STMIA / LDMIA
//-----------------------------------------------------------------------------
	TEMPLATE u32 FASTCALL OP_STMIA_THUMB(armcpu_t *armcpu, const OPCODE opcode, struct _Decoded* d)
	{
		d->IROp = IR_STM;
		d->Rn = THUMB_REGPOS(opcode.ThumbOp, 8);
		d->RegisterList = (opcode.ThumbOp & 0xFF);
		d->P = 0;
		d->U = 1;
		d->S = 0;
		d->W = 1;
		d->ExecuteCycles = 2;
		d->VariableCycles = 1;
		d->Reschedule = 2;
		return 1;
	}

	TEMPLATE u32 FASTCALL OP_LDMIA_THUMB(armcpu_t *armcpu, const OPCODE opcode, struct _Decoded* d)
	{
		d->IROp = IR_LDM;
		d->Rn = THUMB_REGPOS(opcode.ThumbOp, 8);
		d->RegisterList = (opcode.ThumbOp & 0xFF);
		d->P = 0;
		d->U = 1;
		d->S = 0;
		d->W = 1;
		d->ExecuteCycles = 2;
		d->VariableCycles = 1;
		d->Reschedule = 2;
		return 1;
	}

//-----------------------------------------------------------------------------
//   BKPT
//-----------------------------------------------------------------------------
	TEMPLATE u32 FASTCALL OP_BKPT_THUMB(armcpu_t *armcpu, const OPCODE opcode, struct _Decoded* d)
	{
		d->IROp = IR_BKPT;
		d->R15Modified = 1;
		d->ExecuteCycles = 4;
		return 1;
	}

//-----------------------------------------------------------------------------
//   SWI
//-----------------------------------------------------------------------------
	TEMPLATE u32 FASTCALL OP_SWI_THUMB(armcpu_t *armcpu, const OPCODE opcode, struct _Decoded* d)
	{
		d->IROp = IR_SWI;
		//d->Immediate = (opcode.ThumbOp & 0xFF);
		d->Immediate = (opcode.ThumbOp & 0xFF) & 0x1F;
		bool bypassBuiltinSWI = 
			(armcpu->intVector == 0x00000000 && armcpu->proc_ID==0)
			|| (armcpu->intVector == 0xFFFF0000 && armcpu->proc_ID==1);
		if (armcpu->swi_tab && !bypassBuiltinSWI)
		{
			if (d->Immediate == 0x04 || d->Immediate == 0x05 || d->Immediate == 0x06)
				d->Reschedule = 1;
			if (d->Immediate == 0x04 || d->Immediate == 0x05)
				d->MayHalt = 1;
		}
		else
		{
			d->R15Modified = 1;
			d->TbitModified = 1;
			d->Reschedule = 1;
		}
		d->VariableCycles = 1;
		d->ExecuteCycles = 3;
		return 1;
	}

//-----------------------------------------------------------------------------
//   Branch
//-----------------------------------------------------------------------------
	TEMPLATE u32 FASTCALL OP_B_COND(armcpu_t *armcpu, const OPCODE opcode, struct _Decoded* d)
	{
		u32 off = (u32)((s8)(opcode.ThumbOp&0xFF))<<1;
		d->IROp = IR_B;
		d->Cond = (opcode.ThumbOp>>8)&0xF;
		d->Immediate = d->CalcR15(*d) + off;
		d->R15Modified = 1;
		d->ExecuteCycles = 3;
		return 1;
	}

	TEMPLATE u32 FASTCALL OP_B_UNCOND(armcpu_t *armcpu, const OPCODE opcode, struct _Decoded* d)
	{
		u32 off = (((opcode.ThumbOp)&0x7FF) | (BIT10(opcode.ThumbOp) * 0xFFFFF800))<<1;
		d->IROp = IR_B;
		d->Immediate = d->CalcR15(*d) + off;
		d->R15Modified = 1;
		d->ExecuteCycles = 1;
		return 1;
	}

	TEMPLATE u32 FASTCALL OP_BLX(armcpu_t *armcpu, const OPCODE opcode, struct _Decoded* d)
	{
		d->IROp = IR_T32P2;
		d->R15Modified = 1;
		d->TbitModified = 1;
		d->ExecuteCycles = 3;
		return 1;
	}

	TEMPLATE u32 FASTCALL OP_BL_10(armcpu_t *armcpu, const OPCODE opcode, struct _Decoded* d)
	{
		d->IROp = IR_T32P1;
		d->ExecuteCycles = 1;
		return 1;
	}

	TEMPLATE u32 FASTCALL OP_BL_11(armcpu_t *armcpu, const OPCODE opcode, struct _Decoded* d)
	{
		d->IROp = IR_T32P2;
		d->R15Modified = 1;
		d->ExecuteCycles = 4;
		return 1;
	}

	TEMPLATE u32 FASTCALL OP_BX_THUMB(armcpu_t *armcpu, const OPCODE opcode, struct _Decoded* d)
	{
		d->IROp = IR_BX;
		d->Rn = ARM_REGPOS(opcode.ThumbOp, 3);
		d->R15Modified = 1;
		d->TbitModified = 1;
		d->ExecuteCycles = 3;
		return 1;
	}

	TEMPLATE u32 FASTCALL OP_BLX_THUMB(armcpu_t *armcpu, const OPCODE opcode, struct _Decoded* d)
	{
		d->IROp = IR_BLX;
		d->Rn = ARM_REGPOS(opcode.ArmOp, 3);
		d->R15Modified = 1;
		d->TbitModified = 1;
		d->ExecuteCycles = 3;
		return 1;
	}
};

//------------------------------------------------------------
//                         ARM
//------------------------------------------------------------
namespace ArmOpDecoder
{
#define LSL_IMM \
	d->Rm = ARM_REGPOS(opcode.ArmOp, 0);\
	d->Immediate = ((opcode.ArmOp>>7)&0x1F);\
	d->I = 0;\
	d->R = 0;\
	d->Typ = IRSHIFT_LSL;

#define S_LSL_IMM \
	d->Rm = ARM_REGPOS(opcode.ArmOp, 0);\
	d->Immediate = ((opcode.ArmOp>>7)&0x1F);\
	d->I = 0;\
	d->R = 0;\
	d->Typ = IRSHIFT_LSL;\
	d->S = 1;\
	if (!d->Immediate) \
		d->FlagsNeeded |= FLAG_C;

#define LSL_REG \
	d->Rm = ARM_REGPOS(opcode.ArmOp, 0);\
	d->Rs = ARM_REGPOS(opcode.ArmOp, 8);\
	d->I = 0;\
	d->R = 1;\
	d->Typ = IRSHIFT_LSL;

#define S_LSL_REG \
	d->Rm = ARM_REGPOS(opcode.ArmOp, 0);\
	d->Rs = ARM_REGPOS(opcode.ArmOp, 8);\
	d->I = 0;\
	d->R = 1;\
	d->Typ = IRSHIFT_LSL;\
	d->S = 1;\
	d->FlagsNeeded |= FLAG_C;

#define LSR_IMM \
	d->Rm = ARM_REGPOS(opcode.ArmOp, 0);\
	d->Immediate = ((opcode.ArmOp>>7)&0x1F);\
	d->I = 0;\
	d->R = 0;\
	d->Typ = IRSHIFT_LSR;

#define S_LSR_IMM \
	d->Rm = ARM_REGPOS(opcode.ArmOp, 0);\
	d->Immediate = ((opcode.ArmOp>>7)&0x1F);\
	d->I = 0;\
	d->R = 0;\
	d->Typ = IRSHIFT_LSR;\
	d->S = 1;

#define LSR_REG \
	d->Rm = ARM_REGPOS(opcode.ArmOp, 0);\
	d->Rs = ARM_REGPOS(opcode.ArmOp, 8);\
	d->I = 0;\
	d->R = 1;\
	d->Typ = IRSHIFT_LSR;

#define S_LSR_REG \
	d->Rm = ARM_REGPOS(opcode.ArmOp, 0);\
	d->Rs = ARM_REGPOS(opcode.ArmOp, 8);\
	d->I = 0;\
	d->R = 1;\
	d->Typ = IRSHIFT_LSR;\
	d->FlagsNeeded |= FLAG_C;

#define ASR_IMM \
	d->Rm = ARM_REGPOS(opcode.ArmOp, 0);\
	d->Immediate = ((opcode.ArmOp>>7)&0x1F);\
	d->I = 0;\
	d->R = 0;\
	d->Typ = IRSHIFT_ASR;

#define S_ASR_IMM \
	d->Rm = ARM_REGPOS(opcode.ArmOp, 0);\
	d->Immediate = ((opcode.ArmOp>>7)&0x1F);\
	d->I = 0;\
	d->R = 0;\
	d->Typ = IRSHIFT_ASR;\
	d->S = 1;

#define ASR_REG \
	d->Rm = ARM_REGPOS(opcode.ArmOp, 0);\
	d->Rs = ARM_REGPOS(opcode.ArmOp, 8);\
	d->I = 0;\
	d->R = 1;\
	d->Typ = IRSHIFT_ASR;

#define S_ASR_REG \
	d->Rm = ARM_REGPOS(opcode.ArmOp, 0);\
	d->Rs = ARM_REGPOS(opcode.ArmOp, 8);\
	d->I = 0;\
	d->R = 1;\
	d->Typ = IRSHIFT_ASR;\
	d->S = 1;\
	d->FlagsNeeded |= FLAG_C;

#define ROR_IMM \
	d->Rm = ARM_REGPOS(opcode.ArmOp, 0);\
	d->Immediate = ((opcode.ArmOp>>7)&0x1F);\
	d->I = 0;\
	d->R = 0;\
	d->Typ = IRSHIFT_ROR;\
	if (d->Immediate) \
		d->FlagsNeeded |= FLAG_C;

#define S_ROR_IMM \
	d->Rm = ARM_REGPOS(opcode.ArmOp, 0);\
	d->Immediate = ((opcode.ArmOp>>7)&0x1F);\
	d->I = 0;\
	d->R = 0;\
	d->Typ = IRSHIFT_ROR;\
	d->S = 1;\
	if (d->Immediate) \
		d->FlagsNeeded |= FLAG_C;

#define ROR_REG \
	d->Rm = ARM_REGPOS(opcode.ArmOp, 0);\
	d->Rs = ARM_REGPOS(opcode.ArmOp, 8);\
	d->I = 0;\
	d->R = 1;\
	d->Typ = IRSHIFT_ROR;

#define S_ROR_REG \
	d->Rm = ARM_REGPOS(opcode.ArmOp, 0);\
	d->Rs = ARM_REGPOS(opcode.ArmOp, 8);\
	d->I = 0;\
	d->R = 1;\
	d->Typ = IRSHIFT_ROR;\
	d->S = 1;\
	d->FlagsNeeded |= FLAG_C;

#define IMM_VALUE \
	d->Immediate = ROR((opcode.ArmOp&0xFF), (opcode.ArmOp>>7)&0x1E);\
	d->I = 1;

#define S_IMM_VALUE \
	d->Immediate = ROR((opcode.ArmOp&0xFF), (opcode.ArmOp>>7)&0x1E);\
	d->I = 1;\
	d->S = 1;\
	if (!((opcode.ArmOp>>8)&0xF)) \
		d->FlagsNeeded |= FLAG_C;

//-----------------------------------------------------------------------------
//   Undefined instruction
//-----------------------------------------------------------------------------
	DCL_UNDEF_OP(OP_UND)

//-----------------------------------------------------------------------------
//   AND / ANDS
//-----------------------------------------------------------------------------
#define OPDEF_AND(a, b) \
	d->IROp = IR_AND;\
	d->Rd = ARM_REGPOS(opcode.ArmOp, 12);\
	d->Rn = ARM_REGPOS(opcode.ArmOp, 16);\
	if (d->Rd == 15)\
	{\
		d->R15Modified = 1;\
		d->ExecuteCycles = b;\
		return 1;\
	}\
	d->ExecuteCycles = a;\
	return 1;

#define OPDEF_ANDS(a, b) \
	d->IROp = IR_AND;\
	d->Rd = ARM_REGPOS(opcode.ArmOp, 12);\
	d->Rn = ARM_REGPOS(opcode.ArmOp, 16);\
	d->S = 1;\
	if (d->Rd == 15)\
	{\
		d->R15Modified = 1;\
		LOAD_CPSR\
		d->ExecuteCycles = b;\
		return 1;\
	}\
	d->FlagsSet |= FLAG_C | FLAG_N | FLAG_Z;\
	d->ExecuteCycles = a;\
	return 1;

	DCL_OP2_ARG2(OP_AND_LSL_IMM, LSL_IMM, OPDEF_AND, 1, 3)
	DCL_OP2_ARG2(OP_AND_LSL_REG, LSL_REG, OPDEF_AND, 2, 4)
	DCL_OP2_ARG2(OP_AND_LSR_IMM, LSR_IMM, OPDEF_AND, 1, 3)
	DCL_OP2_ARG2(OP_AND_LSR_REG, LSR_REG, OPDEF_AND, 2, 4)
	DCL_OP2_ARG2(OP_AND_ASR_IMM, ASR_IMM, OPDEF_AND, 1, 3)
	DCL_OP2_ARG2(OP_AND_ASR_REG, ASR_REG, OPDEF_AND, 2, 4)
	DCL_OP2_ARG2(OP_AND_ROR_IMM, ROR_IMM, OPDEF_AND, 1, 3)
	DCL_OP2_ARG2(OP_AND_ROR_REG, ROR_REG, OPDEF_AND, 2, 4)
	DCL_OP2_ARG2(OP_AND_IMM_VAL, IMM_VALUE, OPDEF_AND, 1, 3)

	DCL_OP2_ARG2(OP_AND_S_LSL_IMM, S_LSL_IMM, OPDEF_ANDS, 1, 3)
	DCL_OP2_ARG2(OP_AND_S_LSL_REG, S_LSL_REG, OPDEF_ANDS, 2, 4)
	DCL_OP2_ARG2(OP_AND_S_LSR_IMM, S_LSR_IMM, OPDEF_ANDS, 1, 3)
	DCL_OP2_ARG2(OP_AND_S_LSR_REG, S_LSR_REG, OPDEF_ANDS, 2, 4)
	DCL_OP2_ARG2(OP_AND_S_ASR_IMM, S_ASR_IMM, OPDEF_ANDS, 1, 3)
	DCL_OP2_ARG2(OP_AND_S_ASR_REG, S_ASR_REG, OPDEF_ANDS, 2, 4)
	DCL_OP2_ARG2(OP_AND_S_ROR_IMM, S_ROR_IMM, OPDEF_ANDS, 1, 3)
	DCL_OP2_ARG2(OP_AND_S_ROR_REG, S_ROR_REG, OPDEF_ANDS, 2, 4)
	DCL_OP2_ARG2(OP_AND_S_IMM_VAL, S_IMM_VALUE, OPDEF_ANDS, 1, 3)

//-----------------------------------------------------------------------------
//   EOR / EORS
//-----------------------------------------------------------------------------
#define OPDEF_EOR(a, b) \
	d->IROp = IR_EOR;\
	d->Rd = ARM_REGPOS(opcode.ArmOp, 12);\
	d->Rn = ARM_REGPOS(opcode.ArmOp, 16);\
	if (d->Rd == 15)\
	{\
		d->R15Modified = 1;\
		d->ExecuteCycles = b;\
		return 1;\
	}\
	d->ExecuteCycles = a;\
	return 1;

#define OPDEF_EORS(a, b) \
	d->IROp = IR_EOR;\
	d->Rd = ARM_REGPOS(opcode.ArmOp, 12);\
	d->Rn = ARM_REGPOS(opcode.ArmOp, 16);\
	d->S = 1;\
	if (d->Rd == 15)\
	{\
		d->R15Modified = 1;\
		LOAD_CPSR\
		d->ExecuteCycles = b;\
		return 1;\
	}\
	d->FlagsSet |= FLAG_C | FLAG_N | FLAG_Z;\
	d->ExecuteCycles = a;\
	return 1;

	DCL_OP2_ARG2(OP_EOR_LSL_IMM, LSL_IMM, OPDEF_EOR, 1, 3)
	DCL_OP2_ARG2(OP_EOR_LSL_REG, LSL_REG, OPDEF_EOR, 2, 4)
	DCL_OP2_ARG2(OP_EOR_LSR_IMM, LSR_IMM, OPDEF_EOR, 1, 3)
	DCL_OP2_ARG2(OP_EOR_LSR_REG, LSR_REG, OPDEF_EOR, 2, 4)
	DCL_OP2_ARG2(OP_EOR_ASR_IMM, ASR_IMM, OPDEF_EOR, 1, 3)
	DCL_OP2_ARG2(OP_EOR_ASR_REG, ASR_REG, OPDEF_EOR, 2, 4)
	DCL_OP2_ARG2(OP_EOR_ROR_IMM, ROR_IMM, OPDEF_EOR, 1, 3)
	DCL_OP2_ARG2(OP_EOR_ROR_REG, ROR_REG, OPDEF_EOR, 2, 4)
	DCL_OP2_ARG2(OP_EOR_IMM_VAL, IMM_VALUE, OPDEF_EOR, 1, 3)

	DCL_OP2_ARG2(OP_EOR_S_LSL_IMM, S_LSL_IMM, OPDEF_EORS, 1, 3)
	DCL_OP2_ARG2(OP_EOR_S_LSL_REG, S_LSL_REG, OPDEF_EORS, 2, 4)
	DCL_OP2_ARG2(OP_EOR_S_LSR_IMM, S_LSR_IMM, OPDEF_EORS, 1, 3)
	DCL_OP2_ARG2(OP_EOR_S_LSR_REG, S_LSR_REG, OPDEF_EORS, 2, 4)
	DCL_OP2_ARG2(OP_EOR_S_ASR_IMM, S_ASR_IMM, OPDEF_EORS, 1, 3)
	DCL_OP2_ARG2(OP_EOR_S_ASR_REG, S_ASR_REG, OPDEF_EORS, 2, 4)
	DCL_OP2_ARG2(OP_EOR_S_ROR_IMM, S_ROR_IMM, OPDEF_EORS, 1, 3)
	DCL_OP2_ARG2(OP_EOR_S_ROR_REG, S_ROR_REG, OPDEF_EORS, 2, 4)
	DCL_OP2_ARG2(OP_EOR_S_IMM_VAL, S_IMM_VALUE, OPDEF_EORS, 1, 3)

//-----------------------------------------------------------------------------
//   SUB / SUBS
//-----------------------------------------------------------------------------
#define OPDEF_SUB(a, b) \
	d->IROp = IR_SUB;\
	d->Rd = ARM_REGPOS(opcode.ArmOp, 12);\
	d->Rn = ARM_REGPOS(opcode.ArmOp, 16);\
	if (d->Rd == 15)\
	{\
		d->R15Modified = 1;\
		d->ExecuteCycles = b;\
		return 1;\
	}\
	d->ExecuteCycles = a;\
	return 1;

#define OPDEF_SUBS(a, b) \
	d->IROp = IR_SUB;\
	d->Rd = ARM_REGPOS(opcode.ArmOp, 12);\
	d->Rn = ARM_REGPOS(opcode.ArmOp, 16);\
	d->S = 1;\
	if (d->Rd == 15)\
	{\
		d->R15Modified = 1;\
		LOAD_CPSR\
		d->ExecuteCycles = b;\
		return 1;\
	}\
	d->FlagsSet |= ALL_FLAGS;\
	d->ExecuteCycles = a;\
	return 1;

	DCL_OP2_ARG2(OP_SUB_LSL_IMM, LSL_IMM, OPDEF_SUB, 1, 3)
	DCL_OP2_ARG2(OP_SUB_LSL_REG, LSL_REG, OPDEF_SUB, 2, 4)
	DCL_OP2_ARG2(OP_SUB_LSR_IMM, LSR_IMM, OPDEF_SUB, 1, 3)
	DCL_OP2_ARG2(OP_SUB_LSR_REG, LSR_REG, OPDEF_SUB, 2, 4)
	DCL_OP2_ARG2(OP_SUB_ASR_IMM, ASR_IMM, OPDEF_SUB, 1, 3)
	DCL_OP2_ARG2(OP_SUB_ASR_REG, ASR_REG, OPDEF_SUB, 2, 4)
	DCL_OP2_ARG2(OP_SUB_ROR_IMM, ROR_IMM, OPDEF_SUB, 1, 3)
	DCL_OP2_ARG2(OP_SUB_ROR_REG, ROR_REG, OPDEF_SUB, 2, 4)
	DCL_OP2_ARG2(OP_SUB_IMM_VAL, IMM_VALUE, OPDEF_SUB, 1, 3)

	DCL_OP2_ARG2(OP_SUB_S_LSL_IMM, LSL_IMM, OPDEF_SUBS, 1, 3)
	DCL_OP2_ARG2(OP_SUB_S_LSL_REG, LSL_REG, OPDEF_SUBS, 2, 4)
	DCL_OP2_ARG2(OP_SUB_S_LSR_IMM, LSR_IMM, OPDEF_SUBS, 1, 3)
	DCL_OP2_ARG2(OP_SUB_S_LSR_REG, LSR_REG, OPDEF_SUBS, 2, 4)
	DCL_OP2_ARG2(OP_SUB_S_ASR_IMM, ASR_IMM, OPDEF_SUBS, 1, 3)
	DCL_OP2_ARG2(OP_SUB_S_ASR_REG, ASR_REG, OPDEF_SUBS, 2, 4)
	DCL_OP2_ARG2(OP_SUB_S_ROR_IMM, ROR_IMM, OPDEF_SUBS, 1, 3)
	DCL_OP2_ARG2(OP_SUB_S_ROR_REG, ROR_REG, OPDEF_SUBS, 2, 4)
	DCL_OP2_ARG2(OP_SUB_S_IMM_VAL, IMM_VALUE, OPDEF_SUBS, 1, 3)

//-----------------------------------------------------------------------------
//   RSB / RSBS
//-----------------------------------------------------------------------------
#define OPDEF_RSB(a, b) \
	d->IROp = IR_RSB;\
	d->Rd = ARM_REGPOS(opcode.ArmOp, 12);\
	d->Rn = ARM_REGPOS(opcode.ArmOp, 16);\
	if (d->Rd == 15)\
	{\
		d->R15Modified = 1;\
		d->ExecuteCycles = b;\
		return 1;\
	}\
	d->ExecuteCycles = a;\
	return 1;

#define OPDEF_RSBS(a, b) \
	d->IROp = IR_RSB;\
	d->Rd = ARM_REGPOS(opcode.ArmOp, 12);\
	d->Rn = ARM_REGPOS(opcode.ArmOp, 16);\
	d->S = 1;\
	if (d->Rd == 15)\
	{\
		d->R15Modified = 1;\
		LOAD_CPSR\
		d->ExecuteCycles = b;\
		return 1;\
	}\
	d->FlagsSet |= ALL_FLAGS;\
	d->ExecuteCycles = a;\
	return 1;

	DCL_OP2_ARG2(OP_RSB_LSL_IMM, LSL_IMM, OPDEF_RSB, 1, 3)
	DCL_OP2_ARG2(OP_RSB_LSL_REG, LSL_REG, OPDEF_RSB, 2, 4)
	DCL_OP2_ARG2(OP_RSB_LSR_IMM, LSR_IMM, OPDEF_RSB, 1, 3)
	DCL_OP2_ARG2(OP_RSB_LSR_REG, LSR_REG, OPDEF_RSB, 2, 4)
	DCL_OP2_ARG2(OP_RSB_ASR_IMM, ASR_IMM, OPDEF_RSB, 1, 3)
	DCL_OP2_ARG2(OP_RSB_ASR_REG, ASR_REG, OPDEF_RSB, 2, 4)
	DCL_OP2_ARG2(OP_RSB_ROR_IMM, ROR_IMM, OPDEF_RSB, 1, 3)
	DCL_OP2_ARG2(OP_RSB_ROR_REG, ROR_REG, OPDEF_RSB, 2, 4)
	DCL_OP2_ARG2(OP_RSB_IMM_VAL, IMM_VALUE, OPDEF_RSB, 1, 3)

	DCL_OP2_ARG2(OP_RSB_S_LSL_IMM, LSL_IMM, OPDEF_RSBS, 1, 3)
	DCL_OP2_ARG2(OP_RSB_S_LSL_REG, LSL_REG, OPDEF_RSBS, 2, 4)
	DCL_OP2_ARG2(OP_RSB_S_LSR_IMM, LSR_IMM, OPDEF_RSBS, 1, 3)
	DCL_OP2_ARG2(OP_RSB_S_LSR_REG, LSR_REG, OPDEF_RSBS, 2, 4)
	DCL_OP2_ARG2(OP_RSB_S_ASR_IMM, ASR_IMM, OPDEF_RSBS, 1, 3)
	DCL_OP2_ARG2(OP_RSB_S_ASR_REG, ASR_REG, OPDEF_RSBS, 2, 4)
	DCL_OP2_ARG2(OP_RSB_S_ROR_IMM, ROR_IMM, OPDEF_RSBS, 1, 3)
	DCL_OP2_ARG2(OP_RSB_S_ROR_REG, ROR_REG, OPDEF_RSBS, 2, 4)
	DCL_OP2_ARG2(OP_RSB_S_IMM_VAL, IMM_VALUE, OPDEF_RSBS, 1, 3)

//-----------------------------------------------------------------------------
//   ADD / ADDS
//-----------------------------------------------------------------------------
#define OPDEF_ADD(a, b) \
	d->IROp = IR_ADD;\
	d->Rd = ARM_REGPOS(opcode.ArmOp, 12);\
	d->Rn = ARM_REGPOS(opcode.ArmOp, 16);\
	if (d->Rd == 15)\
	{\
		d->R15Modified = 1;\
		d->ExecuteCycles = b;\
		return 1;\
	}\
	d->ExecuteCycles = a;\
	return 1;

#define OPDEF_ADDS(a, b) \
	d->IROp = IR_ADD;\
	d->Rd = ARM_REGPOS(opcode.ArmOp, 12);\
	d->Rn = ARM_REGPOS(opcode.ArmOp, 16);\
	d->S = 1;\
	if (d->Rd == 15)\
	{\
		d->R15Modified = 1;\
		LOAD_CPSR\
		d->ExecuteCycles = b;\
		return 1;\
	}\
	d->FlagsSet |= ALL_FLAGS;\
	d->ExecuteCycles = a;\
	return 1;

	DCL_OP2_ARG2(OP_ADD_LSL_IMM, LSL_IMM, OPDEF_ADD, 1, 3)
	DCL_OP2_ARG2(OP_ADD_LSL_REG, LSL_REG, OPDEF_ADD, 2, 4)
	DCL_OP2_ARG2(OP_ADD_LSR_IMM, LSR_IMM, OPDEF_ADD, 1, 3)
	DCL_OP2_ARG2(OP_ADD_LSR_REG, LSR_REG, OPDEF_ADD, 2, 4)
	DCL_OP2_ARG2(OP_ADD_ASR_IMM, ASR_IMM, OPDEF_ADD, 1, 3)
	DCL_OP2_ARG2(OP_ADD_ASR_REG, ASR_REG, OPDEF_ADD, 2, 4)
	DCL_OP2_ARG2(OP_ADD_ROR_IMM, ROR_IMM, OPDEF_ADD, 1, 3)
	DCL_OP2_ARG2(OP_ADD_ROR_REG, ROR_REG, OPDEF_ADD, 2, 4)
	DCL_OP2_ARG2(OP_ADD_IMM_VAL, IMM_VALUE, OPDEF_ADD, 1, 3)

	DCL_OP2_ARG2(OP_ADD_S_LSL_IMM, LSL_IMM, OPDEF_ADDS, 1, 3)
	DCL_OP2_ARG2(OP_ADD_S_LSL_REG, LSL_REG, OPDEF_ADDS, 2, 4)
	DCL_OP2_ARG2(OP_ADD_S_LSR_IMM, LSR_IMM, OPDEF_ADDS, 1, 3)
	DCL_OP2_ARG2(OP_ADD_S_LSR_REG, LSR_REG, OPDEF_ADDS, 2, 4)
	DCL_OP2_ARG2(OP_ADD_S_ASR_IMM, ASR_IMM, OPDEF_ADDS, 1, 3)
	DCL_OP2_ARG2(OP_ADD_S_ASR_REG, ASR_REG, OPDEF_ADDS, 2, 4)
	DCL_OP2_ARG2(OP_ADD_S_ROR_IMM, ROR_IMM, OPDEF_ADDS, 1, 3)
	DCL_OP2_ARG2(OP_ADD_S_ROR_REG, ROR_REG, OPDEF_ADDS, 2, 4)
	DCL_OP2_ARG2(OP_ADD_S_IMM_VAL, IMM_VALUE, OPDEF_ADDS, 1, 3)

//-----------------------------------------------------------------------------
//   ADC / ADCS
//-----------------------------------------------------------------------------
#define OPDEF_ADC(a, b) \
	d->IROp = IR_ADC;\
	d->Rd = ARM_REGPOS(opcode.ArmOp, 12);\
	d->Rn = ARM_REGPOS(opcode.ArmOp, 16);\
	d->FlagsNeeded |= FLAG_C;\
	if (d->Rd == 15)\
	{\
		d->R15Modified = 1;\
		d->ExecuteCycles = b;\
		return 1;\
	}\
	d->ExecuteCycles = a;\
	return 1;

#define OPDEF_ADCS(a, b) \
	d->IROp = IR_ADC;\
	d->Rd = ARM_REGPOS(opcode.ArmOp, 12);\
	d->Rn = ARM_REGPOS(opcode.ArmOp, 16);\
	d->S = 1;\
	d->FlagsNeeded |= FLAG_C;\
	if (d->Rd == 15)\
	{\
		d->R15Modified = 1;\
		LOAD_CPSR\
		d->ExecuteCycles = b;\
		return 1;\
	}\
	d->FlagsSet |= ALL_FLAGS;\
	d->ExecuteCycles = a;\
	return 1;

	DCL_OP2_ARG2(OP_ADC_LSL_IMM, LSL_IMM, OPDEF_ADC, 1, 3)
	DCL_OP2_ARG2(OP_ADC_LSL_REG, LSL_REG, OPDEF_ADC, 2, 4)
	DCL_OP2_ARG2(OP_ADC_LSR_IMM, LSR_IMM, OPDEF_ADC, 1, 3)
	DCL_OP2_ARG2(OP_ADC_LSR_REG, LSR_REG, OPDEF_ADC, 2, 4)
	DCL_OP2_ARG2(OP_ADC_ASR_IMM, ASR_IMM, OPDEF_ADC, 1, 3)
	DCL_OP2_ARG2(OP_ADC_ASR_REG, ASR_REG, OPDEF_ADC, 2, 4)
	DCL_OP2_ARG2(OP_ADC_ROR_IMM, ROR_IMM, OPDEF_ADC, 1, 3)
	DCL_OP2_ARG2(OP_ADC_ROR_REG, ROR_REG, OPDEF_ADC, 2, 4)
	DCL_OP2_ARG2(OP_ADC_IMM_VAL, IMM_VALUE, OPDEF_ADC, 1, 3)

	DCL_OP2_ARG2(OP_ADC_S_LSL_IMM, LSL_IMM, OPDEF_ADCS, 1, 3)
	DCL_OP2_ARG2(OP_ADC_S_LSL_REG, LSL_REG, OPDEF_ADCS, 2, 4)
	DCL_OP2_ARG2(OP_ADC_S_LSR_IMM, LSR_IMM, OPDEF_ADCS, 1, 3)
	DCL_OP2_ARG2(OP_ADC_S_LSR_REG, LSR_REG, OPDEF_ADCS, 2, 4)
	DCL_OP2_ARG2(OP_ADC_S_ASR_IMM, ASR_IMM, OPDEF_ADCS, 1, 3)
	DCL_OP2_ARG2(OP_ADC_S_ASR_REG, ASR_REG, OPDEF_ADCS, 2, 4)
	DCL_OP2_ARG2(OP_ADC_S_ROR_IMM, ROR_IMM, OPDEF_ADCS, 1, 3)
	DCL_OP2_ARG2(OP_ADC_S_ROR_REG, ROR_REG, OPDEF_ADCS, 2, 4)
	DCL_OP2_ARG2(OP_ADC_S_IMM_VAL, IMM_VALUE, OPDEF_ADCS, 1, 3)

//-----------------------------------------------------------------------------
//   SBC / SBCS
//-----------------------------------------------------------------------------
#define OPDEF_SBC(a, b) \
	d->IROp = IR_SBC;\
	d->Rd = ARM_REGPOS(opcode.ArmOp, 12);\
	d->Rn = ARM_REGPOS(opcode.ArmOp, 16);\
	d->FlagsNeeded |= FLAG_C;\
	if (d->Rd == 15)\
	{\
		d->R15Modified = 1;\
		d->ExecuteCycles = b;\
		return 1;\
	}\
	d->ExecuteCycles = a;\
	return 1;

#define OPDEF_SBCS(a, b) \
	d->IROp = IR_SBC;\
	d->Rd = ARM_REGPOS(opcode.ArmOp, 12);\
	d->Rn = ARM_REGPOS(opcode.ArmOp, 16);\
	d->S = 1;\
	d->FlagsNeeded |= FLAG_C;\
	if (d->Rd == 15)\
	{\
		d->R15Modified = 1;\
		LOAD_CPSR\
		d->ExecuteCycles = b;\
		return 1;\
	}\
	d->FlagsSet |= ALL_FLAGS;\
	d->ExecuteCycles = a;\
	return 1;

	DCL_OP2_ARG2(OP_SBC_LSL_IMM, LSL_IMM, OPDEF_SBC, 1, 3)
	DCL_OP2_ARG2(OP_SBC_LSL_REG, LSL_REG, OPDEF_SBC, 2, 4)
	DCL_OP2_ARG2(OP_SBC_LSR_IMM, LSR_IMM, OPDEF_SBC, 1, 3)
	DCL_OP2_ARG2(OP_SBC_LSR_REG, LSR_REG, OPDEF_SBC, 2, 4)
	DCL_OP2_ARG2(OP_SBC_ASR_IMM, ASR_IMM, OPDEF_SBC, 1, 3)
	DCL_OP2_ARG2(OP_SBC_ASR_REG, ASR_REG, OPDEF_SBC, 2, 4)
	DCL_OP2_ARG2(OP_SBC_ROR_IMM, ROR_IMM, OPDEF_SBC, 1, 3)
	DCL_OP2_ARG2(OP_SBC_ROR_REG, ROR_REG, OPDEF_SBC, 2, 4)
	DCL_OP2_ARG2(OP_SBC_IMM_VAL, IMM_VALUE, OPDEF_SBC, 1, 3)

	DCL_OP2_ARG2(OP_SBC_S_LSL_IMM, LSL_IMM, OPDEF_SBCS, 1, 3)
	DCL_OP2_ARG2(OP_SBC_S_LSL_REG, LSL_REG, OPDEF_SBCS, 2, 4)
	DCL_OP2_ARG2(OP_SBC_S_LSR_IMM, LSR_IMM, OPDEF_SBCS, 1, 3)
	DCL_OP2_ARG2(OP_SBC_S_LSR_REG, LSR_REG, OPDEF_SBCS, 2, 4)
	DCL_OP2_ARG2(OP_SBC_S_ASR_IMM, ASR_IMM, OPDEF_SBCS, 1, 3)
	DCL_OP2_ARG2(OP_SBC_S_ASR_REG, ASR_REG, OPDEF_SBCS, 2, 4)
	DCL_OP2_ARG2(OP_SBC_S_ROR_IMM, ROR_IMM, OPDEF_SBCS, 1, 3)
	DCL_OP2_ARG2(OP_SBC_S_ROR_REG, ROR_REG, OPDEF_SBCS, 2, 4)
	DCL_OP2_ARG2(OP_SBC_S_IMM_VAL, IMM_VALUE, OPDEF_SBCS, 1, 3)

//-----------------------------------------------------------------------------
//   RSC / RSCS
//-----------------------------------------------------------------------------
#define OPDEF_RSC(a, b) \
	d->IROp = IR_RSC;\
	d->Rd = ARM_REGPOS(opcode.ArmOp, 12);\
	d->Rn = ARM_REGPOS(opcode.ArmOp, 16);\
	d->FlagsNeeded |= FLAG_C;\
	if (d->Rd == 15)\
	{\
		d->R15Modified = 1;\
		d->ExecuteCycles = b;\
		return 1;\
	}\
	d->ExecuteCycles = a;\
	return 1;

#define OPDEF_RSCS(a, b) \
	d->IROp = IR_RSC;\
	d->Rd = ARM_REGPOS(opcode.ArmOp, 12);\
	d->Rn = ARM_REGPOS(opcode.ArmOp, 16);\
	d->S = 1;\
	d->FlagsNeeded |= FLAG_C;\
	if (d->Rd == 15)\
	{\
		d->R15Modified = 1;\
		LOAD_CPSR\
		d->ExecuteCycles = b;\
		return 1;\
	}\
	d->FlagsSet |= ALL_FLAGS;\
	d->ExecuteCycles = a;\
	return 1;

	DCL_OP2_ARG2(OP_RSC_LSL_IMM, LSL_IMM, OPDEF_RSC, 1, 3)
	DCL_OP2_ARG2(OP_RSC_LSL_REG, LSL_REG, OPDEF_RSC, 2, 4)
	DCL_OP2_ARG2(OP_RSC_LSR_IMM, LSR_IMM, OPDEF_RSC, 1, 3)
	DCL_OP2_ARG2(OP_RSC_LSR_REG, LSR_REG, OPDEF_RSC, 2, 4)
	DCL_OP2_ARG2(OP_RSC_ASR_IMM, ASR_IMM, OPDEF_RSC, 1, 3)
	DCL_OP2_ARG2(OP_RSC_ASR_REG, ASR_REG, OPDEF_RSC, 2, 4)
	DCL_OP2_ARG2(OP_RSC_ROR_IMM, ROR_IMM, OPDEF_RSC, 1, 3)
	DCL_OP2_ARG2(OP_RSC_ROR_REG, ROR_REG, OPDEF_RSC, 2, 4)
	DCL_OP2_ARG2(OP_RSC_IMM_VAL, IMM_VALUE, OPDEF_RSC, 1, 3)

	DCL_OP2_ARG2(OP_RSC_S_LSL_IMM, LSL_IMM, OPDEF_RSCS, 1, 3)
	DCL_OP2_ARG2(OP_RSC_S_LSL_REG, LSL_REG, OPDEF_RSCS, 2, 4)
	DCL_OP2_ARG2(OP_RSC_S_LSR_IMM, LSR_IMM, OPDEF_RSCS, 1, 3)
	DCL_OP2_ARG2(OP_RSC_S_LSR_REG, LSR_REG, OPDEF_RSCS, 2, 4)
	DCL_OP2_ARG2(OP_RSC_S_ASR_IMM, ASR_IMM, OPDEF_RSCS, 1, 3)
	DCL_OP2_ARG2(OP_RSC_S_ASR_REG, ASR_REG, OPDEF_RSCS, 2, 4)
	DCL_OP2_ARG2(OP_RSC_S_ROR_IMM, ROR_IMM, OPDEF_RSCS, 1, 3)
	DCL_OP2_ARG2(OP_RSC_S_ROR_REG, ROR_REG, OPDEF_RSCS, 2, 4)
	DCL_OP2_ARG2(OP_RSC_S_IMM_VAL, IMM_VALUE, OPDEF_RSCS, 1, 3)

//-----------------------------------------------------------------------------
//   TST
//-----------------------------------------------------------------------------
#define OPDEF_TST(a) \
	d->IROp = IR_TST;\
	d->Rn = ARM_REGPOS(opcode.ArmOp, 16);\
	d->FlagsSet |= FLAG_C | FLAG_N | FLAG_Z;\
	d->ExecuteCycles = a;\
	return 1;

	DCL_OP2_ARG1(OP_TST_LSL_IMM, S_LSL_IMM, OPDEF_TST, 1)
	DCL_OP2_ARG1(OP_TST_LSL_REG, S_LSL_REG, OPDEF_TST, 2)
	DCL_OP2_ARG1(OP_TST_LSR_IMM, S_LSR_IMM, OPDEF_TST, 1)
	DCL_OP2_ARG1(OP_TST_LSR_REG, S_LSR_REG, OPDEF_TST, 2)
	DCL_OP2_ARG1(OP_TST_ASR_IMM, S_ASR_IMM, OPDEF_TST, 1)
	DCL_OP2_ARG1(OP_TST_ASR_REG, S_ASR_REG, OPDEF_TST, 2)
	DCL_OP2_ARG1(OP_TST_ROR_IMM, S_ROR_IMM, OPDEF_TST, 1)
	DCL_OP2_ARG1(OP_TST_ROR_REG, S_ROR_REG, OPDEF_TST, 2)
	DCL_OP2_ARG1(OP_TST_IMM_VAL, S_IMM_VALUE, OPDEF_TST, 1)

//-----------------------------------------------------------------------------
//   TEQ
//-----------------------------------------------------------------------------
#define OPDEF_TEQ(a) \
	d->IROp = IR_TEQ;\
	d->Rn = ARM_REGPOS(opcode.ArmOp, 16);\
	d->FlagsSet |= FLAG_C | FLAG_N | FLAG_Z;\
	d->ExecuteCycles = a;\
	return 1;

	DCL_OP2_ARG1(OP_TEQ_LSL_IMM, S_LSL_IMM, OPDEF_TEQ, 1)
	DCL_OP2_ARG1(OP_TEQ_LSL_REG, S_LSL_REG, OPDEF_TEQ, 2)
	DCL_OP2_ARG1(OP_TEQ_LSR_IMM, S_LSR_IMM, OPDEF_TEQ, 1)
	DCL_OP2_ARG1(OP_TEQ_LSR_REG, S_LSR_REG, OPDEF_TEQ, 2)
	DCL_OP2_ARG1(OP_TEQ_ASR_IMM, S_ASR_IMM, OPDEF_TEQ, 1)
	DCL_OP2_ARG1(OP_TEQ_ASR_REG, S_ASR_REG, OPDEF_TEQ, 2)
	DCL_OP2_ARG1(OP_TEQ_ROR_IMM, S_ROR_IMM, OPDEF_TEQ, 1)
	DCL_OP2_ARG1(OP_TEQ_ROR_REG, S_ROR_REG, OPDEF_TEQ, 2)
	DCL_OP2_ARG1(OP_TEQ_IMM_VAL, S_IMM_VALUE, OPDEF_TEQ, 1)

//-----------------------------------------------------------------------------
//   CMP
//-----------------------------------------------------------------------------
#define OPDEF_CMP(a) \
	d->IROp = IR_CMP;\
	d->Rn = ARM_REGPOS(opcode.ArmOp, 16);\
	d->FlagsSet |= ALL_FLAGS;\
	d->ExecuteCycles = a;\
	return 1;

	DCL_OP2_ARG1(OP_CMP_LSL_IMM, LSL_IMM, OPDEF_CMP, 1)
	DCL_OP2_ARG1(OP_CMP_LSL_REG, LSL_REG, OPDEF_CMP, 2)
	DCL_OP2_ARG1(OP_CMP_LSR_IMM, LSR_IMM, OPDEF_CMP, 1)
	DCL_OP2_ARG1(OP_CMP_LSR_REG, LSR_REG, OPDEF_CMP, 2)
	DCL_OP2_ARG1(OP_CMP_ASR_IMM, ASR_IMM, OPDEF_CMP, 1)
	DCL_OP2_ARG1(OP_CMP_ASR_REG, ASR_REG, OPDEF_CMP, 2)
	DCL_OP2_ARG1(OP_CMP_ROR_IMM, ROR_IMM, OPDEF_CMP, 1)
	DCL_OP2_ARG1(OP_CMP_ROR_REG, ROR_REG, OPDEF_CMP, 2)
	DCL_OP2_ARG1(OP_CMP_IMM_VAL, IMM_VALUE, OPDEF_CMP, 1)

//-----------------------------------------------------------------------------
//   CMN
//-----------------------------------------------------------------------------
#define OPDEF_CMN(a) \
	d->IROp = IR_CMN;\
	d->Rn = ARM_REGPOS(opcode.ArmOp, 16);\
	d->FlagsSet |= ALL_FLAGS;\
	d->ExecuteCycles = a;\
	return 1;

	DCL_OP2_ARG1(OP_CMN_LSL_IMM, LSL_IMM, OPDEF_CMN, 1)
	DCL_OP2_ARG1(OP_CMN_LSL_REG, LSL_REG, OPDEF_CMN, 2)
	DCL_OP2_ARG1(OP_CMN_LSR_IMM, LSR_IMM, OPDEF_CMN, 1)
	DCL_OP2_ARG1(OP_CMN_LSR_REG, LSR_REG, OPDEF_CMN, 2)
	DCL_OP2_ARG1(OP_CMN_ASR_IMM, ASR_IMM, OPDEF_CMN, 1)
	DCL_OP2_ARG1(OP_CMN_ASR_REG, ASR_REG, OPDEF_CMN, 2)
	DCL_OP2_ARG1(OP_CMN_ROR_IMM, ROR_IMM, OPDEF_CMN, 1)
	DCL_OP2_ARG1(OP_CMN_ROR_REG, ROR_REG, OPDEF_CMN, 2)
	DCL_OP2_ARG1(OP_CMN_IMM_VAL, IMM_VALUE, OPDEF_CMN, 1)

//-----------------------------------------------------------------------------
//   ORR / ORRS
//-----------------------------------------------------------------------------
#define OPDEF_ORR(a, b) \
	d->IROp = IR_ORR;\
	d->Rd = ARM_REGPOS(opcode.ArmOp, 12);\
	d->Rn = ARM_REGPOS(opcode.ArmOp, 16);\
	if (d->Rd == 15)\
	{\
		d->R15Modified = 1;\
		d->ExecuteCycles = b;\
		return 1;\
	}\
	d->ExecuteCycles = a;\
	return 1;

#define OPDEF_ORRS(a, b) \
	d->IROp = IR_ORR;\
	d->Rd = ARM_REGPOS(opcode.ArmOp, 12);\
	d->Rn = ARM_REGPOS(opcode.ArmOp, 16);\
	d->S = 1;\
	if (d->Rd == 15)\
	{\
		d->R15Modified = 1;\
		LOAD_CPSR\
		d->ExecuteCycles = b;\
		return 1;\
	}\
	d->FlagsSet |= FLAG_C | FLAG_N | FLAG_Z;\
	d->ExecuteCycles = a;\
	return 1;

	DCL_OP2_ARG2(OP_ORR_LSL_IMM, LSL_IMM, OPDEF_ORR, 1, 3)
	DCL_OP2_ARG2(OP_ORR_LSL_REG, LSL_REG, OPDEF_ORR, 2, 4)
	DCL_OP2_ARG2(OP_ORR_LSR_IMM, LSR_IMM, OPDEF_ORR, 1, 3)
	DCL_OP2_ARG2(OP_ORR_LSR_REG, LSR_REG, OPDEF_ORR, 2, 4)
	DCL_OP2_ARG2(OP_ORR_ASR_IMM, ASR_IMM, OPDEF_ORR, 1, 3)
	DCL_OP2_ARG2(OP_ORR_ASR_REG, ASR_REG, OPDEF_ORR, 2, 4)
	DCL_OP2_ARG2(OP_ORR_ROR_IMM, ROR_IMM, OPDEF_ORR, 1, 3)
	DCL_OP2_ARG2(OP_ORR_ROR_REG, ROR_REG, OPDEF_ORR, 2, 4)
	DCL_OP2_ARG2(OP_ORR_IMM_VAL, IMM_VALUE, OPDEF_ORR, 1, 3)

	DCL_OP2_ARG2(OP_ORR_S_LSL_IMM, S_LSL_IMM, OPDEF_ORRS, 1, 3)
	DCL_OP2_ARG2(OP_ORR_S_LSL_REG, S_LSL_REG, OPDEF_ORRS, 2, 4)
	DCL_OP2_ARG2(OP_ORR_S_LSR_IMM, S_LSR_IMM, OPDEF_ORRS, 1, 3)
	DCL_OP2_ARG2(OP_ORR_S_LSR_REG, S_LSR_REG, OPDEF_ORRS, 2, 4)
	DCL_OP2_ARG2(OP_ORR_S_ASR_IMM, S_ASR_IMM, OPDEF_ORRS, 1, 3)
	DCL_OP2_ARG2(OP_ORR_S_ASR_REG, S_ASR_REG, OPDEF_ORRS, 2, 4)
	DCL_OP2_ARG2(OP_ORR_S_ROR_IMM, S_ROR_IMM, OPDEF_ORRS, 1, 3)
	DCL_OP2_ARG2(OP_ORR_S_ROR_REG, S_ROR_REG, OPDEF_ORRS, 2, 4)
	DCL_OP2_ARG2(OP_ORR_S_IMM_VAL, S_IMM_VALUE, OPDEF_ORRS, 1, 3)

//-----------------------------------------------------------------------------
//   MOV / MOVS
//-----------------------------------------------------------------------------
#define OPDEF_MOV(a, b) \
	if (opcode.ArmOp == 0xE1A00000)\
	{\
		/*nop: MOV R0, R0*/\
		d->IROp = IR_NOP;\
		d->ExecuteCycles = 1;\
		return 1;\
	}\
	d->IROp = IR_MOV;\
	d->Rd = ARM_REGPOS(opcode.ArmOp, 12);\
	if (d->Rd == 15)\
	{\
		d->R15Modified = 1;\
		d->ExecuteCycles = b;\
		return 1;\
	}\
	d->ExecuteCycles = a;\
	return 1;

#define OPDEF_MOVS(a, b) \
	d->IROp = IR_MOV;\
	d->Rd = ARM_REGPOS(opcode.ArmOp, 12);\
	d->S = 1;\
	if (d->Rd == 15)\
	{\
		d->R15Modified = 1;\
		LOAD_CPSR\
		d->ExecuteCycles = b;\
		return 1;\
	}\
	d->FlagsSet |= FLAG_C | FLAG_N | FLAG_Z;\
	d->ExecuteCycles = a;\
	return 1;

	DCL_OP2_ARG2(OP_MOV_LSL_IMM, LSL_IMM, OPDEF_MOV, 1, 3)
	DCL_OP2_ARG2(OP_MOV_LSL_REG, LSL_REG, OPDEF_MOV, 2, 4)
	DCL_OP2_ARG2(OP_MOV_LSR_IMM, LSR_IMM, OPDEF_MOV, 1, 3)
	DCL_OP2_ARG2(OP_MOV_LSR_REG, LSR_REG, OPDEF_MOV, 2, 4)
	DCL_OP2_ARG2(OP_MOV_ASR_IMM, ASR_IMM, OPDEF_MOV, 1, 3)
	DCL_OP2_ARG2(OP_MOV_ASR_REG, ASR_REG, OPDEF_MOV, 2, 4)
	DCL_OP2_ARG2(OP_MOV_ROR_IMM, ROR_IMM, OPDEF_MOV, 1, 3)
	DCL_OP2_ARG2(OP_MOV_ROR_REG, ROR_REG, OPDEF_MOV, 2, 4)
	DCL_OP2_ARG2(OP_MOV_IMM_VAL, IMM_VALUE, OPDEF_MOV, 1, 3)

	DCL_OP2_ARG2(OP_MOV_S_LSL_IMM, S_LSL_IMM, OPDEF_MOVS, 1, 3)
	DCL_OP2_ARG2(OP_MOV_S_LSL_REG, S_LSL_REG, OPDEF_MOVS, 2, 4)
	DCL_OP2_ARG2(OP_MOV_S_LSR_IMM, S_LSR_IMM, OPDEF_MOVS, 1, 3)
	DCL_OP2_ARG2(OP_MOV_S_LSR_REG, S_LSR_REG, OPDEF_MOVS, 2, 4)
	DCL_OP2_ARG2(OP_MOV_S_ASR_IMM, S_ASR_IMM, OPDEF_MOVS, 1, 3)
	DCL_OP2_ARG2(OP_MOV_S_ASR_REG, S_ASR_REG, OPDEF_MOVS, 2, 4)
	DCL_OP2_ARG2(OP_MOV_S_ROR_IMM, S_ROR_IMM, OPDEF_MOVS, 1, 3)
	DCL_OP2_ARG2(OP_MOV_S_ROR_REG, S_ROR_REG, OPDEF_MOVS, 2, 4)
	DCL_OP2_ARG2(OP_MOV_S_IMM_VAL, S_IMM_VALUE, OPDEF_MOVS, 1, 3)

//-----------------------------------------------------------------------------
//   BIC / BICS
//-----------------------------------------------------------------------------
#define OPDEF_BIC(a, b) \
	d->IROp = IR_BIC;\
	d->Rd = ARM_REGPOS(opcode.ArmOp, 12);\
	d->Rn = ARM_REGPOS(opcode.ArmOp, 16);\
	if (d->Rd == 15)\
	{\
		d->R15Modified = 1;\
		d->ExecuteCycles = b;\
		return 1;\
	}\
	d->ExecuteCycles = a;\
	return 1;

#define OPDEF_BICS(a, b) \
	d->IROp = IR_BIC;\
	d->Rd = ARM_REGPOS(opcode.ArmOp, 12);\
	d->Rn = ARM_REGPOS(opcode.ArmOp, 16);\
	d->S = 1;\
	if (d->Rd == 15)\
	{\
		d->R15Modified = 1;\
		LOAD_CPSR\
		d->ExecuteCycles = b;\
		return 1;\
	}\
	d->FlagsSet |= FLAG_C | FLAG_N | FLAG_Z;\
	d->ExecuteCycles = a;\
	return 1;

	DCL_OP2_ARG2(OP_BIC_LSL_IMM, LSL_IMM, OPDEF_BIC, 1, 3)
	DCL_OP2_ARG2(OP_BIC_LSL_REG, LSL_REG, OPDEF_BIC, 2, 4)
	DCL_OP2_ARG2(OP_BIC_LSR_IMM, LSR_IMM, OPDEF_BIC, 1, 3)
	DCL_OP2_ARG2(OP_BIC_LSR_REG, LSR_REG, OPDEF_BIC, 2, 4)
	DCL_OP2_ARG2(OP_BIC_ASR_IMM, ASR_IMM, OPDEF_BIC, 1, 3)
	DCL_OP2_ARG2(OP_BIC_ASR_REG, ASR_REG, OPDEF_BIC, 2, 4)
	DCL_OP2_ARG2(OP_BIC_ROR_IMM, ROR_IMM, OPDEF_BIC, 1, 3)
	DCL_OP2_ARG2(OP_BIC_ROR_REG, ROR_REG, OPDEF_BIC, 2, 4)
	DCL_OP2_ARG2(OP_BIC_IMM_VAL, IMM_VALUE, OPDEF_BIC, 1, 3)

	DCL_OP2_ARG2(OP_BIC_S_LSL_IMM, S_LSL_IMM, OPDEF_BICS, 1, 3)
	DCL_OP2_ARG2(OP_BIC_S_LSL_REG, S_LSL_REG, OPDEF_BICS, 2, 4)
	DCL_OP2_ARG2(OP_BIC_S_LSR_IMM, S_LSR_IMM, OPDEF_BICS, 1, 3)
	DCL_OP2_ARG2(OP_BIC_S_LSR_REG, S_LSR_REG, OPDEF_BICS, 2, 4)
	DCL_OP2_ARG2(OP_BIC_S_ASR_IMM, S_ASR_IMM, OPDEF_BICS, 1, 3)
	DCL_OP2_ARG2(OP_BIC_S_ASR_REG, S_ASR_REG, OPDEF_BICS, 2, 4)
	DCL_OP2_ARG2(OP_BIC_S_ROR_IMM, S_ROR_IMM, OPDEF_BICS, 1, 3)
	DCL_OP2_ARG2(OP_BIC_S_ROR_REG, S_ROR_REG, OPDEF_BICS, 2, 4)
	DCL_OP2_ARG2(OP_BIC_S_IMM_VAL, S_IMM_VALUE, OPDEF_BICS, 1, 3)

//-----------------------------------------------------------------------------
//   MVN / MVNS
//-----------------------------------------------------------------------------
#define OPDEF_MVN(a, b) \
	d->IROp = IR_MVN;\
	d->Rd = ARM_REGPOS(opcode.ArmOp, 12);\
	if (d->Rd == 15)\
	{\
		d->R15Modified = 1;\
		d->ExecuteCycles = b;\
		return 1;\
	}\
	d->ExecuteCycles = a;\
	return 1;

#define OPDEF_MVNS(a, b) \
	d->IROp = IR_MVN;\
	d->Rd = ARM_REGPOS(opcode.ArmOp, 12);\
	d->S = 1;\
	if (d->Rd == 15)\
	{\
		d->R15Modified = 1;\
		LOAD_CPSR\
		d->ExecuteCycles = b;\
		return 1;\
	}\
	d->FlagsSet |= FLAG_C | FLAG_N | FLAG_Z;\
	d->ExecuteCycles = a;\
	return 1;

	DCL_OP2_ARG2(OP_MVN_LSL_IMM, LSL_IMM, OPDEF_MVN, 1, 3)
	DCL_OP2_ARG2(OP_MVN_LSL_REG, LSL_REG, OPDEF_MVN, 2, 4)
	DCL_OP2_ARG2(OP_MVN_LSR_IMM, LSR_IMM, OPDEF_MVN, 1, 3)
	DCL_OP2_ARG2(OP_MVN_LSR_REG, LSR_REG, OPDEF_MVN, 2, 4)
	DCL_OP2_ARG2(OP_MVN_ASR_IMM, ASR_IMM, OPDEF_MVN, 1, 3)
	DCL_OP2_ARG2(OP_MVN_ASR_REG, ASR_REG, OPDEF_MVN, 2, 4)
	DCL_OP2_ARG2(OP_MVN_ROR_IMM, ROR_IMM, OPDEF_MVN, 1, 3)
	DCL_OP2_ARG2(OP_MVN_ROR_REG, ROR_REG, OPDEF_MVN, 2, 4)
	DCL_OP2_ARG2(OP_MVN_IMM_VAL, IMM_VALUE, OPDEF_MVN, 1, 3)

	DCL_OP2_ARG2(OP_MVN_S_LSL_IMM, S_LSL_IMM, OPDEF_MVNS, 1, 3)
	DCL_OP2_ARG2(OP_MVN_S_LSL_REG, S_LSL_REG, OPDEF_MVNS, 2, 4)
	DCL_OP2_ARG2(OP_MVN_S_LSR_IMM, S_LSR_IMM, OPDEF_MVNS, 1, 3)
	DCL_OP2_ARG2(OP_MVN_S_LSR_REG, S_LSR_REG, OPDEF_MVNS, 2, 4)
	DCL_OP2_ARG2(OP_MVN_S_ASR_IMM, S_ASR_IMM, OPDEF_MVNS, 1, 3)
	DCL_OP2_ARG2(OP_MVN_S_ASR_REG, S_ASR_REG, OPDEF_MVNS, 2, 4)
	DCL_OP2_ARG2(OP_MVN_S_ROR_IMM, S_ROR_IMM, OPDEF_MVNS, 1, 3)
	DCL_OP2_ARG2(OP_MVN_S_ROR_REG, S_ROR_REG, OPDEF_MVNS, 2, 4)
	DCL_OP2_ARG2(OP_MVN_S_IMM_VAL, S_IMM_VALUE, OPDEF_MVNS, 1, 3)

//-----------------------------------------------------------------------------
//   MUL / MULS / MLA / MLAS
//-----------------------------------------------------------------------------
	TEMPLATE u32 FASTCALL OP_MUL(armcpu_t *armcpu, const OPCODE opcode, struct _Decoded* d)
	{
		d->IROp = IR_MUL;
		d->Rd = ARM_REGPOS(opcode.ArmOp, 16);
		d->Rs = ARM_REGPOS(opcode.ArmOp, 8);
		d->Rm = ARM_REGPOS(opcode.ArmOp, 0);
		d->VariableCycles = 1;
		return 1;
	}

	TEMPLATE u32 FASTCALL OP_MLA(armcpu_t *armcpu, const OPCODE opcode, struct _Decoded* d)
	{
		d->IROp = IR_MLA;
		d->Rd = ARM_REGPOS(opcode.ArmOp, 16);
		d->Rn = ARM_REGPOS(opcode.ArmOp, 12);
		d->Rs = ARM_REGPOS(opcode.ArmOp, 8);
		d->Rm = ARM_REGPOS(opcode.ArmOp, 0);
		d->VariableCycles = 1;
		return 1;
	}

	TEMPLATE u32 FASTCALL OP_MUL_S(armcpu_t *armcpu, const OPCODE opcode, struct _Decoded* d)
	{
		d->IROp = IR_MUL;
		d->Rd = ARM_REGPOS(opcode.ArmOp, 16);
		d->Rs = ARM_REGPOS(opcode.ArmOp, 8);
		d->Rm = ARM_REGPOS(opcode.ArmOp, 0);
		d->S = 1;
		d->FlagsSet |= FLAG_N | FLAG_Z;
		d->VariableCycles = 1;
		return 1;
	}

	TEMPLATE u32 FASTCALL OP_MLA_S(armcpu_t *armcpu, const OPCODE opcode, struct _Decoded* d)
	{
		d->IROp = IR_MLA;
		d->Rd = ARM_REGPOS(opcode.ArmOp, 16);
		d->Rn = ARM_REGPOS(opcode.ArmOp, 12);
		d->Rs = ARM_REGPOS(opcode.ArmOp, 8);
		d->Rm = ARM_REGPOS(opcode.ArmOp, 0);
		d->S = 1;
		d->FlagsSet |= FLAG_N | FLAG_Z;
		d->VariableCycles = 1;
		return 1;
	}

//-----------------------------------------------------------------------------
//   UMULL / UMULLS / UMLAL / UMLALS
//-----------------------------------------------------------------------------
	TEMPLATE u32 FASTCALL OP_UMULL(armcpu_t *armcpu, const OPCODE opcode, struct _Decoded* d)
	{
		d->IROp = IR_UMULL;
		d->Rd = ARM_REGPOS(opcode.ArmOp, 16);
		d->Rn = ARM_REGPOS(opcode.ArmOp, 12);
		d->Rs = ARM_REGPOS(opcode.ArmOp, 8);
		d->Rm = ARM_REGPOS(opcode.ArmOp, 0);
		d->VariableCycles = 1;
		return 1;
	}

	TEMPLATE u32 FASTCALL OP_UMLAL(armcpu_t *armcpu, const OPCODE opcode, struct _Decoded* d)
	{
		d->IROp = IR_UMLAL;
		d->Rd = ARM_REGPOS(opcode.ArmOp, 16);
		d->Rn = ARM_REGPOS(opcode.ArmOp, 12);
		d->Rs = ARM_REGPOS(opcode.ArmOp, 8);
		d->Rm = ARM_REGPOS(opcode.ArmOp, 0);
		d->VariableCycles = 1;
		return 1;
	}

	TEMPLATE u32 FASTCALL OP_UMULL_S(armcpu_t *armcpu, const OPCODE opcode, struct _Decoded* d)
	{
		d->IROp = IR_UMULL;
		d->Rd = ARM_REGPOS(opcode.ArmOp, 16);
		d->Rn = ARM_REGPOS(opcode.ArmOp, 12);
		d->Rs = ARM_REGPOS(opcode.ArmOp, 8);
		d->Rm = ARM_REGPOS(opcode.ArmOp, 0);
		d->S = 1;
		d->FlagsSet |= FLAG_N | FLAG_Z;
		d->VariableCycles = 1;
		return 1;
	}

	TEMPLATE u32 FASTCALL OP_UMLAL_S(armcpu_t *armcpu, const OPCODE opcode, struct _Decoded* d)
	{
		d->IROp = IR_UMLAL;
		d->Rd = ARM_REGPOS(opcode.ArmOp, 16);
		d->Rn = ARM_REGPOS(opcode.ArmOp, 12);
		d->Rs = ARM_REGPOS(opcode.ArmOp, 8);
		d->Rm = ARM_REGPOS(opcode.ArmOp, 0);
		d->S = 1;
		d->FlagsSet |= FLAG_N | FLAG_Z;
		d->VariableCycles = 1;
		return 1;
	}

//-----------------------------------------------------------------------------
//   SMULL / SMULLS / SMLAL / SMLALS
//-----------------------------------------------------------------------------
	TEMPLATE u32 FASTCALL OP_SMULL(armcpu_t *armcpu, const OPCODE opcode, struct _Decoded* d)
	{
		d->IROp = IR_SMULL;
		d->Rd = ARM_REGPOS(opcode.ArmOp, 16);
		d->Rn = ARM_REGPOS(opcode.ArmOp, 12);
		d->Rs = ARM_REGPOS(opcode.ArmOp, 8);
		d->Rm = ARM_REGPOS(opcode.ArmOp, 0);
		d->VariableCycles = 1;
		return 1;
	}

	TEMPLATE u32 FASTCALL OP_SMLAL(armcpu_t *armcpu, const OPCODE opcode, struct _Decoded* d)
	{
		d->IROp = IR_SMLAL;
		d->Rd = ARM_REGPOS(opcode.ArmOp, 16);
		d->Rn = ARM_REGPOS(opcode.ArmOp, 12);
		d->Rs = ARM_REGPOS(opcode.ArmOp, 8);
		d->Rm = ARM_REGPOS(opcode.ArmOp, 0);
		d->VariableCycles = 1;
		return 1;
	}

	TEMPLATE u32 FASTCALL OP_SMULL_S(armcpu_t *armcpu, const OPCODE opcode, struct _Decoded* d)
	{
		d->IROp = IR_SMULL;
		d->Rd = ARM_REGPOS(opcode.ArmOp, 16);
		d->Rn = ARM_REGPOS(opcode.ArmOp, 12);
		d->Rs = ARM_REGPOS(opcode.ArmOp, 8);
		d->Rm = ARM_REGPOS(opcode.ArmOp, 0);
		d->S = 1;
		d->FlagsSet |= FLAG_N | FLAG_Z;
		d->VariableCycles = 1;
		return 1;
	}

	TEMPLATE u32 FASTCALL OP_SMLAL_S(armcpu_t *armcpu, const OPCODE opcode, struct _Decoded* d)
	{
		d->IROp = IR_SMLAL;
		d->Rd = ARM_REGPOS(opcode.ArmOp, 16);
		d->Rn = ARM_REGPOS(opcode.ArmOp, 12);
		d->Rs = ARM_REGPOS(opcode.ArmOp, 8);
		d->Rm = ARM_REGPOS(opcode.ArmOp, 0);
		d->S = 1;
		d->FlagsSet |= FLAG_N | FLAG_Z;
		d->VariableCycles = 1;
		return 1;
	}

//-----------------------------------------------------------------------------
//   SWP / SWPB
//-----------------------------------------------------------------------------
	TEMPLATE u32 FASTCALL OP_SWP(armcpu_t *armcpu, const OPCODE opcode, struct _Decoded* d)
	{
		d->IROp = IR_SWP;
		d->Rn = ARM_REGPOS(opcode.ArmOp, 16);
		d->Rd = ARM_REGPOS(opcode.ArmOp, 12);
		d->Rm = ARM_REGPOS(opcode.ArmOp, 0);
		d->B = 0;
		d->ExecuteCycles = 4;
		d->VariableCycles = 1;
		return 1;
	}

	TEMPLATE u32 FASTCALL OP_SWPB(armcpu_t *armcpu, const OPCODE opcode, struct _Decoded* d)
	{
		d->IROp = IR_SWP;
		d->Rn = ARM_REGPOS(opcode.ArmOp, 16);
		d->Rd = ARM_REGPOS(opcode.ArmOp, 12);
		d->Rm = ARM_REGPOS(opcode.ArmOp, 0);
		d->B = 1;
		d->ExecuteCycles = 4;
		d->VariableCycles = 1;
		return 1;
	}

//-----------------------------------------------------------------------------
//   LDRH
//-----------------------------------------------------------------------------
#define OPDEF_LDRH(p,u,i,w,cycle)\
	d->IROp = IR_LDRx;\
	d->Rn = ARM_REGPOS(opcode.ArmOp, 16);\
	d->Rd = ARM_REGPOS(opcode.ArmOp, 12);\
	d->P = p;\
	d->U = u;\
	d->I = i;\
	if (i)\
	{\
		d->Immediate = (((opcode.ArmOp>>4)&0xF0)+(opcode.ArmOp&0xF));\
	}\
	else\
	{\
		d->Rm = ARM_REGPOS(opcode.ArmOp, 0);\
	}\
	d->W = w;\
	d->S = 0;\
	d->H = 1;\
	d->ExecuteCycles = cycle;\
	d->VariableCycles = 1;\
	d->Reschedule = 2;

	DCL_OP1_ARG5(OP_LDRH_P_IMM_OFF, OPDEF_LDRH, 1, 1, 1, 0, 3)
	DCL_OP1_ARG5(OP_LDRH_M_IMM_OFF, OPDEF_LDRH, 1, 0, 1, 0, 3)
	DCL_OP1_ARG5(OP_LDRH_P_REG_OFF, OPDEF_LDRH, 1, 1, 0, 0, 3)
	DCL_OP1_ARG5(OP_LDRH_M_REG_OFF, OPDEF_LDRH, 1, 0, 0, 0, 3)
	DCL_OP1_ARG5(OP_LDRH_PRE_INDE_P_IMM_OFF, OPDEF_LDRH, 1, 1, 1, 1, 3)
	DCL_OP1_ARG5(OP_LDRH_PRE_INDE_M_IMM_OFF, OPDEF_LDRH, 1, 0, 1, 1, 3)
	DCL_OP1_ARG5(OP_LDRH_PRE_INDE_P_REG_OFF, OPDEF_LDRH, 1, 1, 0, 1, 3)
	DCL_OP1_ARG5(OP_LDRH_PRE_INDE_M_REG_OFF, OPDEF_LDRH, 1, 0, 0, 1, 3)
	DCL_OP1_ARG5(OP_LDRH_POS_INDE_P_IMM_OFF, OPDEF_LDRH, 0, 1, 1, 1, 3)
	DCL_OP1_ARG5(OP_LDRH_POS_INDE_M_IMM_OFF, OPDEF_LDRH, 0, 0, 1, 1, 3)
	DCL_OP1_ARG5(OP_LDRH_POS_INDE_P_REG_OFF, OPDEF_LDRH, 0, 1, 0, 1, 3)
	DCL_OP1_ARG5(OP_LDRH_POS_INDE_M_REG_OFF, OPDEF_LDRH, 0, 0, 0, 1, 3)

#undef OPDEF_LDRH
//-----------------------------------------------------------------------------
//   STRH
//-----------------------------------------------------------------------------
#define OPDEF_STRH(p,u,i,w,cycle)\
	d->IROp = IR_STRx;\
	d->Rn = ARM_REGPOS(opcode.ArmOp, 16);\
	d->Rd = ARM_REGPOS(opcode.ArmOp, 12);\
	d->P = p;\
	d->U = u;\
	d->I = i;\
	if (i)\
	{\
		d->Immediate = (((opcode.ArmOp>>4)&0xF0)+(opcode.ArmOp&0xF));\
	}\
	else\
	{\
		d->Rm = ARM_REGPOS(opcode.ArmOp, 0);\
	}\
	d->W = w;\
	d->S = 0;\
	d->H = 1;\
	d->ExecuteCycles = cycle;\
	d->VariableCycles = 1;\
	d->Reschedule = 2;

	DCL_OP1_ARG5(OP_STRH_P_IMM_OFF, OPDEF_STRH, 1, 1, 1, 0, 2)
	DCL_OP1_ARG5(OP_STRH_M_IMM_OFF, OPDEF_STRH, 1, 0, 1, 0, 2)
	DCL_OP1_ARG5(OP_STRH_P_REG_OFF, OPDEF_STRH, 1, 1, 0, 0, 2)
	DCL_OP1_ARG5(OP_STRH_M_REG_OFF, OPDEF_STRH, 1, 0, 0, 0, 2)
	DCL_OP1_ARG5(OP_STRH_PRE_INDE_P_IMM_OFF, OPDEF_STRH, 1, 1, 1, 1, 2)
	DCL_OP1_ARG5(OP_STRH_PRE_INDE_M_IMM_OFF, OPDEF_STRH, 1, 0, 1, 1, 2)
	DCL_OP1_ARG5(OP_STRH_PRE_INDE_P_REG_OFF, OPDEF_STRH, 1, 1, 0, 1, 2)
	DCL_OP1_ARG5(OP_STRH_PRE_INDE_M_REG_OFF, OPDEF_STRH, 1, 0, 0, 1, 2)
	DCL_OP1_ARG5(OP_STRH_POS_INDE_P_IMM_OFF, OPDEF_STRH, 0, 1, 1, 1, 2)
	DCL_OP1_ARG5(OP_STRH_POS_INDE_M_IMM_OFF, OPDEF_STRH, 0, 0, 1, 1, 2)
	DCL_OP1_ARG5(OP_STRH_POS_INDE_P_REG_OFF, OPDEF_STRH, 0, 1, 0, 1, 2)
	DCL_OP1_ARG5(OP_STRH_POS_INDE_M_REG_OFF, OPDEF_STRH, 0, 0, 0, 1, 2)

#undef OPDEF_STRH
//-----------------------------------------------------------------------------
//   LDRSH
//-----------------------------------------------------------------------------
#define OPDEF_LDRSH(p,u,i,w,cycle)\
	d->IROp = IR_LDRx;\
	d->Rn = ARM_REGPOS(opcode.ArmOp, 16);\
	d->Rd = ARM_REGPOS(opcode.ArmOp, 12);\
	d->P = p;\
	d->U = u;\
	d->I = i;\
	if (i)\
	{\
		d->Immediate = (((opcode.ArmOp>>4)&0xF0)+(opcode.ArmOp&0xF));\
	}\
	else\
	{\
		d->Rm = ARM_REGPOS(opcode.ArmOp, 0);\
	}\
	d->W = w;\
	d->S = 1;\
	d->H = 1;\
	d->ExecuteCycles = cycle;\
	d->VariableCycles = 1;\
	d->Reschedule = 2;

	DCL_OP1_ARG5(OP_LDRSH_P_IMM_OFF, OPDEF_LDRSH, 1, 1, 1, 0, 3)
	DCL_OP1_ARG5(OP_LDRSH_M_IMM_OFF, OPDEF_LDRSH, 1, 0, 1, 0, 3)
	DCL_OP1_ARG5(OP_LDRSH_P_REG_OFF, OPDEF_LDRSH, 1, 1, 0, 0, 3)
	DCL_OP1_ARG5(OP_LDRSH_M_REG_OFF, OPDEF_LDRSH, 1, 0, 0, 0, 3)
	DCL_OP1_ARG5(OP_LDRSH_PRE_INDE_P_IMM_OFF, OPDEF_LDRSH, 1, 1, 1, 1, 3)
	DCL_OP1_ARG5(OP_LDRSH_PRE_INDE_M_IMM_OFF, OPDEF_LDRSH, 1, 0, 1, 1, 3)
	DCL_OP1_ARG5(OP_LDRSH_PRE_INDE_P_REG_OFF, OPDEF_LDRSH, 1, 1, 0, 1, 3)
	DCL_OP1_ARG5(OP_LDRSH_PRE_INDE_M_REG_OFF, OPDEF_LDRSH, 1, 0, 0, 1, 3)
	DCL_OP1_ARG5(OP_LDRSH_POS_INDE_P_IMM_OFF, OPDEF_LDRSH, 0, 1, 1, 1, 3)
	DCL_OP1_ARG5(OP_LDRSH_POS_INDE_M_IMM_OFF, OPDEF_LDRSH, 0, 0, 1, 1, 3)
	DCL_OP1_ARG5(OP_LDRSH_POS_INDE_P_REG_OFF, OPDEF_LDRSH, 0, 1, 0, 1, 3)
	DCL_OP1_ARG5(OP_LDRSH_POS_INDE_M_REG_OFF, OPDEF_LDRSH, 0, 0, 0, 1, 3)

#undef OPDEF_LDRSH

//-----------------------------------------------------------------------------
//   LDRSB
//-----------------------------------------------------------------------------
#define OPDEF_LDRSB(p,u,i,w,cycle)\
	d->IROp = IR_LDRx;\
	d->Rn = ARM_REGPOS(opcode.ArmOp, 16);\
	d->Rd = ARM_REGPOS(opcode.ArmOp, 12);\
	d->P = p;\
	d->U = u;\
	d->I = i;\
	if (i)\
	{\
		d->Immediate = (((opcode.ArmOp>>4)&0xF0)+(opcode.ArmOp&0xF));\
	}\
	else\
	{\
		d->Rm = ARM_REGPOS(opcode.ArmOp, 0);\
	}\
	d->W = w;\
	d->S = 1;\
	d->H = 0;\
	d->ExecuteCycles = cycle;\
	d->VariableCycles = 1;\
	d->Reschedule = 2;

	DCL_OP1_ARG5(OP_LDRSB_P_IMM_OFF, OPDEF_LDRSB, 1, 1, 1, 0, 3)
	DCL_OP1_ARG5(OP_LDRSB_M_IMM_OFF, OPDEF_LDRSB, 1, 0, 1, 0, 3)
	DCL_OP1_ARG5(OP_LDRSB_P_REG_OFF, OPDEF_LDRSB, 1, 1, 0, 0, 3)
	DCL_OP1_ARG5(OP_LDRSB_M_REG_OFF, OPDEF_LDRSB, 1, 0, 0, 0, 3)
	DCL_OP1_ARG5(OP_LDRSB_PRE_INDE_P_IMM_OFF, OPDEF_LDRSB, 1, 1, 1, 1, 3)
	DCL_OP1_ARG5(OP_LDRSB_PRE_INDE_M_IMM_OFF, OPDEF_LDRSB, 1, 0, 1, 1, 3)
	DCL_OP1_ARG5(OP_LDRSB_PRE_INDE_P_REG_OFF, OPDEF_LDRSB, 1, 1, 0, 1, 3)
	DCL_OP1_ARG5(OP_LDRSB_PRE_INDE_M_REG_OFF, OPDEF_LDRSB, 1, 0, 0, 1, 3)
	DCL_OP1_ARG5(OP_LDRSB_POS_INDE_P_IMM_OFF, OPDEF_LDRSB, 0, 1, 1, 1, 3)
	DCL_OP1_ARG5(OP_LDRSB_POS_INDE_M_IMM_OFF, OPDEF_LDRSB, 0, 0, 1, 1, 3)
	DCL_OP1_ARG5(OP_LDRSB_POS_INDE_P_REG_OFF, OPDEF_LDRSB, 0, 1, 0, 1, 3)
	DCL_OP1_ARG5(OP_LDRSB_POS_INDE_M_REG_OFF, OPDEF_LDRSB, 0, 0, 0, 1, 3)

#undef OPDEF_LDRSB

//-----------------------------------------------------------------------------
//   MRS / MSR
//-----------------------------------------------------------------------------
	TEMPLATE u32 FASTCALL OP_MRS_CPSR(armcpu_t *armcpu, const OPCODE opcode, struct _Decoded* d)
	{
		d->IROp = IR_MRS;
		d->Rd = ARM_REGPOS(opcode.ArmOp, 12);
		//if (d->Rd == 15)
		//	d->R15Modified = 1;
		d->P = 0;
		d->FlagsNeeded |= ALL_FLAGS;
		d->ExecuteCycles = 1;
		return 1;
	}

	TEMPLATE u32 FASTCALL OP_MRS_SPSR(armcpu_t *armcpu, const OPCODE opcode, struct _Decoded* d)
	{
		d->IROp = IR_MRS;
		d->Rd = ARM_REGPOS(opcode.ArmOp, 12);
		d->P = 1;
		d->ExecuteCycles = 1;
		return 1;
	}

	TEMPLATE u32 FASTCALL OP_MSR_CPSR(armcpu_t *armcpu, const OPCODE opcode, struct _Decoded* d)
	{
		d->IROp = IR_MSR;
		d->Rm = ARM_REGPOS(opcode.ArmOp, 0);
		d->P = 0;
		d->OpData = (opcode.ArmOp >> 16) & 0xF;
		if (BIT19(opcode.ArmOp))
			d->FlagsSet |= ALL_FLAGS;
		if (BIT16(opcode.ArmOp))
			d->TbitModified = 1;
		d->Reschedule = 1;
		d->ExecuteCycles = 1;
		return 1;
	}

	TEMPLATE u32 FASTCALL OP_MSR_SPSR(armcpu_t *armcpu, const OPCODE opcode, struct _Decoded* d)
	{
		d->IROp = IR_MSR;
		d->Rm = ARM_REGPOS(opcode.ArmOp, 0);
		d->P = 1;
		d->OpData = (opcode.ArmOp >> 16) & 0xF;
		d->Reschedule = 1;
		d->ExecuteCycles = 1;
		return 1;
	}

	TEMPLATE u32 FASTCALL OP_MSR_CPSR_IMM_VAL(armcpu_t *armcpu, const OPCODE opcode, struct _Decoded* d)
	{
		d->IROp = IR_MSR;
		d->Immediate = ROR((opcode.ArmOp&0xFF), (opcode.ArmOp>>7)&0x1E);
		d->P = 0;
		d->I = 1;
		d->OpData = (opcode.ArmOp >> 16) & 0xF;
		if (BIT19(opcode.ArmOp))
			d->FlagsSet |= ALL_FLAGS;
		if (BIT16(opcode.ArmOp))
			d->TbitModified = 1;
		d->Reschedule = 1;
		d->ExecuteCycles = 1;
		return 1;
	}

	TEMPLATE u32 FASTCALL OP_MSR_SPSR_IMM_VAL(armcpu_t *armcpu, const OPCODE opcode, struct _Decoded* d)
	{
		d->IROp = IR_MSR;
		d->Immediate = ROR((opcode.ArmOp&0xFF), (opcode.ArmOp>>7)&0x1E);
		d->P = 1;
		d->I = 1;
		d->OpData = (opcode.ArmOp >> 16) & 0xF;
		d->Reschedule = 1;
		d->ExecuteCycles = 1;
		return 1;
	}

//-----------------------------------------------------------------------------
//   Branch
//-----------------------------------------------------------------------------
	TEMPLATE u32 FASTCALL OP_BX(armcpu_t *armcpu, const OPCODE opcode, struct _Decoded* d)
	{
		d->IROp = IR_BX;
		d->Rn = ARM_REGPOS(opcode.ArmOp, 0);
		d->R15Modified = 1;
		d->TbitModified = 1;
		d->ExecuteCycles = 3;
		return 1;
	}

	TEMPLATE u32 FASTCALL OP_BLX_REG(armcpu_t *armcpu, const OPCODE opcode, struct _Decoded* d)
	{
		d->IROp = IR_BLX;
		d->Rn = ARM_REGPOS(opcode.ArmOp, 0);
		d->R15Modified = 1;
		d->TbitModified = 1;
		d->ExecuteCycles = 3;
		return 1;
	}

	TEMPLATE u32 FASTCALL OP_B(armcpu_t *armcpu, const OPCODE opcode, struct _Decoded* d)
	{
		u32 off = (u32)(((s32)opcode.ArmOp<<8)>>8);
		d->IROp = IR_B;
		d->Immediate = (d->CalcR15(*d) + (off<<2))&0xFFFFFFFC;
		d->R15Modified = 1;
		d->ExecuteCycles = 3;
		return 1;
	}

	TEMPLATE u32 FASTCALL OP_BL(armcpu_t *armcpu, const OPCODE opcode, struct _Decoded* d)
	{
		u32 off = (u32)(((s32)opcode.ArmOp<<8)>>8);
		d->IROp = IR_BL;
		d->Immediate = (d->CalcR15(*d) + (off<<2))&0xFFFFFFFC;
		d->R15Modified = 1;
		d->ExecuteCycles = 3;
		return 1;
	}

//-----------------------------------------------------------------------------
//   CLZ
//-----------------------------------------------------------------------------
	TEMPLATE u32 FASTCALL OP_CLZ(armcpu_t *armcpu, const OPCODE opcode, struct _Decoded* d)
	{
		d->IROp = IR_CLZ;
		d->Rd = ARM_REGPOS(opcode.ArmOp, 12);
		d->Rm = ARM_REGPOS(opcode.ArmOp, 0);
		d->ExecuteCycles = 2;
		return 1;
	}

//-----------------------------------------------------------------------------
//   QADD / QDADD / QSUB / QDSUB
//-----------------------------------------------------------------------------
	TEMPLATE u32 FASTCALL OP_QADD(armcpu_t *armcpu, const OPCODE opcode, struct _Decoded* d)
	{
		d->IROp = IR_QADD;
		d->Rn = ARM_REGPOS(opcode.ArmOp, 16);
		d->Rd = ARM_REGPOS(opcode.ArmOp, 12);
		d->Rm = ARM_REGPOS(opcode.ArmOp, 0);
		if (d->Rd == 15)
		{
			d->R15Modified = 1;
			d->ExecuteCycles = 3;
		}
		else
			d->ExecuteCycles = 2;
		return 1;
	}

	TEMPLATE u32 FASTCALL OP_QSUB(armcpu_t *armcpu, const OPCODE opcode, struct _Decoded* d)
	{
		d->IROp = IR_QSUB;
		d->Rn = ARM_REGPOS(opcode.ArmOp, 16);
		d->Rd = ARM_REGPOS(opcode.ArmOp, 12);
		d->Rm = ARM_REGPOS(opcode.ArmOp, 0);
		if (d->Rd == 15)
		{
			d->R15Modified = 1;
			d->ExecuteCycles = 3;
		}
		else
			d->ExecuteCycles = 2;
		return 1;
	}

	TEMPLATE u32 FASTCALL OP_QDADD(armcpu_t *armcpu, const OPCODE opcode, struct _Decoded* d)
	{
		d->IROp = IR_QDADD;
		d->Rn = ARM_REGPOS(opcode.ArmOp, 16);
		d->Rd = ARM_REGPOS(opcode.ArmOp, 12);
		d->Rm = ARM_REGPOS(opcode.ArmOp, 0);
		if (d->Rd == 15)
		{
			d->R15Modified = 1;
			d->ExecuteCycles = 3;
		}
		else
			d->ExecuteCycles = 2;
		return 1;
	}

	TEMPLATE u32 FASTCALL OP_QDSUB(armcpu_t *armcpu, const OPCODE opcode, struct _Decoded* d)
	{
		d->IROp = IR_QDSUB;
		d->Rn = ARM_REGPOS(opcode.ArmOp, 16);
		d->Rd = ARM_REGPOS(opcode.ArmOp, 12);
		d->Rm = ARM_REGPOS(opcode.ArmOp, 0);
		if (d->Rd == 15)
		{
			d->R15Modified = 1;
			d->ExecuteCycles = 3;
		}
		else
			d->ExecuteCycles = 2;
		return 1;
	}

//-----------------------------------------------------------------------------
//   SMUL
//-----------------------------------------------------------------------------
	TEMPLATE u32 FASTCALL OP_SMUL_B_B(armcpu_t *armcpu, const OPCODE opcode, struct _Decoded* d)
	{
		d->IROp = IR_SMULxy;
		d->Rd = ARM_REGPOS(opcode.ArmOp, 16);
		d->Rs = ARM_REGPOS(opcode.ArmOp, 8);
		d->Rm = ARM_REGPOS(opcode.ArmOp, 0);
		d->X = 0;
		d->Y = 0;
		d->ExecuteCycles = 2;
		return 1;
	}

	TEMPLATE u32 FASTCALL OP_SMUL_B_T(armcpu_t *armcpu, const OPCODE opcode, struct _Decoded* d)
	{
		d->IROp = IR_SMULxy;
		d->Rd = ARM_REGPOS(opcode.ArmOp, 16);
		d->Rs = ARM_REGPOS(opcode.ArmOp, 8);
		d->Rm = ARM_REGPOS(opcode.ArmOp, 0);
		d->X = 0;
		d->Y = 1;
		d->ExecuteCycles = 2;
		return 1;
	}

	TEMPLATE u32 FASTCALL OP_SMUL_T_B(armcpu_t *armcpu, const OPCODE opcode, struct _Decoded* d)
	{
		d->IROp = IR_SMULxy;
		d->Rd = ARM_REGPOS(opcode.ArmOp, 16);
		d->Rs = ARM_REGPOS(opcode.ArmOp, 8);
		d->Rm = ARM_REGPOS(opcode.ArmOp, 0);
		d->X = 1;
		d->Y = 0;
		d->ExecuteCycles = 2;
		return 1;
	}

	TEMPLATE u32 FASTCALL OP_SMUL_T_T(armcpu_t *armcpu, const OPCODE opcode, struct _Decoded* d)
	{
		d->IROp = IR_SMULxy;
		d->Rd = ARM_REGPOS(opcode.ArmOp, 16);
		d->Rs = ARM_REGPOS(opcode.ArmOp, 8);
		d->Rm = ARM_REGPOS(opcode.ArmOp, 0);
		d->X = 1;
		d->Y = 1;
		d->ExecuteCycles = 2;
		return 1;
	}

//-----------------------------------------------------------------------------
//   SMLA
//-----------------------------------------------------------------------------
	TEMPLATE u32 FASTCALL OP_SMLA_B_B(armcpu_t *armcpu, const OPCODE opcode, struct _Decoded* d)
	{
		d->IROp = IR_SMLAxy;
		d->Rd = ARM_REGPOS(opcode.ArmOp, 16);
		d->Rn = ARM_REGPOS(opcode.ArmOp, 12);
		d->Rs = ARM_REGPOS(opcode.ArmOp, 8);
		d->Rm = ARM_REGPOS(opcode.ArmOp, 0);
		d->X = 0;
		d->Y = 0;
		d->ExecuteCycles = 2;
		return 1;
	}

	TEMPLATE u32 FASTCALL OP_SMLA_B_T(armcpu_t *armcpu, const OPCODE opcode, struct _Decoded* d)
	{
		d->IROp = IR_SMLAxy;
		d->Rd = ARM_REGPOS(opcode.ArmOp, 16);
		d->Rn = ARM_REGPOS(opcode.ArmOp, 12);
		d->Rs = ARM_REGPOS(opcode.ArmOp, 8);
		d->Rm = ARM_REGPOS(opcode.ArmOp, 0);
		d->X = 0;
		d->Y = 1;
		d->ExecuteCycles = 2;
		return 1;
	}

	TEMPLATE u32 FASTCALL OP_SMLA_T_B(armcpu_t *armcpu, const OPCODE opcode, struct _Decoded* d)
	{
		d->IROp = IR_SMLAxy;
		d->Rd = ARM_REGPOS(opcode.ArmOp, 16);
		d->Rn = ARM_REGPOS(opcode.ArmOp, 12);
		d->Rs = ARM_REGPOS(opcode.ArmOp, 8);
		d->Rm = ARM_REGPOS(opcode.ArmOp, 0);
		d->X = 1;
		d->Y = 0;
		d->ExecuteCycles = 2;
		return 1;
	}

	TEMPLATE u32 FASTCALL OP_SMLA_T_T(armcpu_t *armcpu, const OPCODE opcode, struct _Decoded* d)
	{
		d->IROp = IR_SMLAxy;
		d->Rd = ARM_REGPOS(opcode.ArmOp, 16);
		d->Rn = ARM_REGPOS(opcode.ArmOp, 12);
		d->Rs = ARM_REGPOS(opcode.ArmOp, 8);
		d->Rm = ARM_REGPOS(opcode.ArmOp, 0);
		d->X = 1;
		d->Y = 1;
		d->ExecuteCycles = 2;
		return 1;
	}

//-----------------------------------------------------------------------------
//   SMLAL
//-----------------------------------------------------------------------------
	TEMPLATE u32 FASTCALL OP_SMLAL_B_B(armcpu_t *armcpu, const OPCODE opcode, struct _Decoded* d)
	{
		d->IROp = IR_SMLALxy;
		d->Rd = ARM_REGPOS(opcode.ArmOp, 16);
		d->Rn = ARM_REGPOS(opcode.ArmOp, 12);
		d->Rs = ARM_REGPOS(opcode.ArmOp, 8);
		d->Rm = ARM_REGPOS(opcode.ArmOp, 0);
		d->X = 0;
		d->Y = 0;
		d->ExecuteCycles = 2;
		return 1;
	}

	TEMPLATE u32 FASTCALL OP_SMLAL_B_T(armcpu_t *armcpu, const OPCODE opcode, struct _Decoded* d)
	{
		d->IROp = IR_SMLALxy;
		d->Rd = ARM_REGPOS(opcode.ArmOp, 16);
		d->Rn = ARM_REGPOS(opcode.ArmOp, 12);
		d->Rs = ARM_REGPOS(opcode.ArmOp, 8);
		d->Rm = ARM_REGPOS(opcode.ArmOp, 0);
		d->X = 0;
		d->Y = 1;
		d->ExecuteCycles = 2;
		return 1;
	}

	TEMPLATE u32 FASTCALL OP_SMLAL_T_B(armcpu_t *armcpu, const OPCODE opcode, struct _Decoded* d)
	{
		d->IROp = IR_SMLALxy;
		d->Rd = ARM_REGPOS(opcode.ArmOp, 16);
		d->Rn = ARM_REGPOS(opcode.ArmOp, 12);
		d->Rs = ARM_REGPOS(opcode.ArmOp, 8);
		d->Rm = ARM_REGPOS(opcode.ArmOp, 0);
		d->X = 1;
		d->Y = 0;
		d->ExecuteCycles = 2;
		return 1;
	}

	TEMPLATE u32 FASTCALL OP_SMLAL_T_T(armcpu_t *armcpu, const OPCODE opcode, struct _Decoded* d)
	{
		d->IROp = IR_SMLALxy;
		d->Rd = ARM_REGPOS(opcode.ArmOp, 16);
		d->Rn = ARM_REGPOS(opcode.ArmOp, 12);
		d->Rs = ARM_REGPOS(opcode.ArmOp, 8);
		d->Rm = ARM_REGPOS(opcode.ArmOp, 0);
		d->X = 1;
		d->Y = 1;
		d->ExecuteCycles = 2;
		return 1;
	}

//-----------------------------------------------------------------------------
//   SMULW
//-----------------------------------------------------------------------------
	TEMPLATE u32 FASTCALL OP_SMULW_B(armcpu_t *armcpu, const OPCODE opcode, struct _Decoded* d)
	{
		d->IROp = IR_SMULWy;
		d->Rd = ARM_REGPOS(opcode.ArmOp, 16);
		d->Rs = ARM_REGPOS(opcode.ArmOp, 8);
		d->Rm = ARM_REGPOS(opcode.ArmOp, 0);
		d->Y = 0;
		d->ExecuteCycles = 2;
		return 1;
	}

	TEMPLATE u32 FASTCALL OP_SMULW_T(armcpu_t *armcpu, const OPCODE opcode, struct _Decoded* d)
	{
		d->IROp = IR_SMULWy;
		d->Rd = ARM_REGPOS(opcode.ArmOp, 16);
		d->Rs = ARM_REGPOS(opcode.ArmOp, 8);
		d->Rm = ARM_REGPOS(opcode.ArmOp, 0);
		d->Y = 1;
		d->ExecuteCycles = 2;
		return 1;
	}

//-----------------------------------------------------------------------------
//   SMLAW
//-----------------------------------------------------------------------------
	TEMPLATE u32 FASTCALL OP_SMLAW_B(armcpu_t *armcpu, const OPCODE opcode, struct _Decoded* d)
	{
		d->IROp = IR_SMLAWy;
		d->Rd = ARM_REGPOS(opcode.ArmOp, 16);
		d->Rn = ARM_REGPOS(opcode.ArmOp, 12);
		d->Rs = ARM_REGPOS(opcode.ArmOp, 8);
		d->Rm = ARM_REGPOS(opcode.ArmOp, 0);
		d->Y = 0;
		d->ExecuteCycles = 2;
		return 1;
	}

	TEMPLATE u32 FASTCALL OP_SMLAW_T(armcpu_t *armcpu, const OPCODE opcode, struct _Decoded* d)
	{
		d->IROp = IR_SMLAWy;
		d->Rd = ARM_REGPOS(opcode.ArmOp, 16);
		d->Rn = ARM_REGPOS(opcode.ArmOp, 12);
		d->Rs = ARM_REGPOS(opcode.ArmOp, 8);
		d->Rm = ARM_REGPOS(opcode.ArmOp, 0);
		d->Y = 1;
		d->ExecuteCycles = 2;
		return 1;
	}

//-----------------------------------------------------------------------------
//   LDR
//-----------------------------------------------------------------------------
#define IMM_OFF_12 \
	d->Immediate = ((opcode.ArmOp)&0xFFF);\
	d->I = 1;

#define OPDEF_LDR(p,u,b,w,cycle_a,cycle_b)\
	d->IROp = IR_LDR;\
	d->Rn = ARM_REGPOS(opcode.ArmOp, 16);\
	d->Rd = ARM_REGPOS(opcode.ArmOp, 12);\
	d->P = p;\
	d->U = u;\
	d->B = b;\
	d->W = w;\
	d->VariableCycles = 1;\
	d->ExecuteCycles = cycle_a;\
	if (!d->B && d->Rd == 15)\
	{\
		d->ExecuteCycles = cycle_b;\
		d->R15Modified = 1;\
		if (PROCNUM == 0) d->TbitModified = 1;\
	}\
	d->Reschedule = 2;

	DCL_OP2_ARG6(OP_LDR_P_IMM_OFF, IMM_OFF_12, OPDEF_LDR, 1, 1, 0, 0, 3, 5)
	DCL_OP2_ARG6(OP_LDR_M_IMM_OFF, IMM_OFF_12, OPDEF_LDR, 1, 0, 0, 0, 3, 5)
	DCL_OP2_ARG6(OP_LDR_P_LSL_IMM_OFF, LSL_IMM, OPDEF_LDR, 1, 1, 0, 0, 3, 5)
	DCL_OP2_ARG6(OP_LDR_M_LSL_IMM_OFF, LSL_IMM, OPDEF_LDR, 1, 0, 0, 0, 3, 5)
	DCL_OP2_ARG6(OP_LDR_P_LSR_IMM_OFF, LSR_IMM, OPDEF_LDR, 1, 1, 0, 0, 3, 5)
	DCL_OP2_ARG6(OP_LDR_M_LSR_IMM_OFF, LSR_IMM, OPDEF_LDR, 1, 0, 0, 0, 3, 5)
	DCL_OP2_ARG6(OP_LDR_P_ASR_IMM_OFF, ASR_IMM, OPDEF_LDR, 1, 1, 0, 0, 3, 5)
	DCL_OP2_ARG6(OP_LDR_M_ASR_IMM_OFF, ASR_IMM, OPDEF_LDR, 1, 0, 0, 0, 3, 5)
	DCL_OP2_ARG6(OP_LDR_P_ROR_IMM_OFF, ROR_IMM, OPDEF_LDR, 1, 1, 0, 0, 3, 5)
	DCL_OP2_ARG6(OP_LDR_M_ROR_IMM_OFF, ROR_IMM, OPDEF_LDR, 1, 0, 0, 0, 3, 5)

	DCL_OP2_ARG6(OP_LDR_P_IMM_OFF_PREIND, IMM_OFF_12, OPDEF_LDR, 1, 1, 0, 1, 3, 5)
	DCL_OP2_ARG6(OP_LDR_M_IMM_OFF_PREIND, IMM_OFF_12, OPDEF_LDR, 1, 0, 0, 1, 3, 5)
	DCL_OP2_ARG6(OP_LDR_P_LSL_IMM_OFF_PREIND, LSL_IMM, OPDEF_LDR, 1, 1, 0, 1, 3, 5)
	DCL_OP2_ARG6(OP_LDR_M_LSL_IMM_OFF_PREIND, LSL_IMM, OPDEF_LDR, 1, 0, 0, 1, 3, 5)
	DCL_OP2_ARG6(OP_LDR_P_LSR_IMM_OFF_PREIND, LSR_IMM, OPDEF_LDR, 1, 1, 0, 1, 3, 5)
	DCL_OP2_ARG6(OP_LDR_M_LSR_IMM_OFF_PREIND, LSR_IMM, OPDEF_LDR, 1, 0, 0, 1, 3, 5)
	DCL_OP2_ARG6(OP_LDR_P_ASR_IMM_OFF_PREIND, ASR_IMM, OPDEF_LDR, 1, 1, 0, 1, 3, 5)
	DCL_OP2_ARG6(OP_LDR_M_ASR_IMM_OFF_PREIND, ASR_IMM, OPDEF_LDR, 1, 0, 0, 1, 3, 5)
	DCL_OP2_ARG6(OP_LDR_P_ROR_IMM_OFF_PREIND, ROR_IMM, OPDEF_LDR, 1, 1, 0, 1, 3, 5)
	DCL_OP2_ARG6(OP_LDR_M_ROR_IMM_OFF_PREIND, ROR_IMM, OPDEF_LDR, 1, 0, 0, 1, 3, 5)

	DCL_OP2_ARG6(OP_LDR_P_IMM_OFF_POSTIND, IMM_OFF_12, OPDEF_LDR, 0, 1, 0, 1, 3, 5)
	DCL_OP2_ARG6(OP_LDR_M_IMM_OFF_POSTIND, IMM_OFF_12, OPDEF_LDR, 0, 0, 0, 1, 3, 5)
	DCL_OP2_ARG6(OP_LDR_P_LSL_IMM_OFF_POSTIND, LSL_IMM, OPDEF_LDR, 0, 1, 0, 1, 3, 5)
	DCL_OP2_ARG6(OP_LDR_M_LSL_IMM_OFF_POSTIND, LSL_IMM, OPDEF_LDR, 0, 0, 0, 1, 3, 5)
	DCL_OP2_ARG6(OP_LDR_P_LSR_IMM_OFF_POSTIND, LSR_IMM, OPDEF_LDR, 0, 1, 0, 1, 3, 5)
	DCL_OP2_ARG6(OP_LDR_M_LSR_IMM_OFF_POSTIND, LSR_IMM, OPDEF_LDR, 0, 0, 0, 1, 3, 5)
	DCL_OP2_ARG6(OP_LDR_P_ASR_IMM_OFF_POSTIND, ASR_IMM, OPDEF_LDR, 0, 1, 0, 1, 3, 5)
	DCL_OP2_ARG6(OP_LDR_M_ASR_IMM_OFF_POSTIND, ASR_IMM, OPDEF_LDR, 0, 0, 0, 1, 3, 5)
	DCL_OP2_ARG6(OP_LDR_P_ROR_IMM_OFF_POSTIND, ROR_IMM, OPDEF_LDR, 0, 1, 0, 1, 3, 5)
	DCL_OP2_ARG6(OP_LDR_M_ROR_IMM_OFF_POSTIND, ROR_IMM, OPDEF_LDR, 0, 0, 0, 1, 3, 5)

//-----------------------------------------------------------------------------
//   LDREX
//-----------------------------------------------------------------------------
	TEMPLATE u32 FASTCALL OP_LDREX(armcpu_t *armcpu, const OPCODE opcode, struct _Decoded* d)
	{
		d->IROp = IR_LDREX;
		d->Rn = ARM_REGPOS(opcode.ArmOp, 16);
		d->Rd = ARM_REGPOS(opcode.ArmOp, 12);
		d->VariableCycles = 1;
		d->ExecuteCycles = 3;
		d->Reschedule = 2;
		return 1;
	}

//-----------------------------------------------------------------------------
//   LDRB
//-----------------------------------------------------------------------------
	DCL_OP2_ARG6(OP_LDRB_P_IMM_OFF, IMM_OFF_12, OPDEF_LDR, 1, 1, 1, 0, 3, 3)
	DCL_OP2_ARG6(OP_LDRB_M_IMM_OFF, IMM_OFF_12, OPDEF_LDR, 1, 0, 1, 0, 3, 3)
	DCL_OP2_ARG6(OP_LDRB_P_LSL_IMM_OFF, LSL_IMM, OPDEF_LDR, 1, 1, 1, 0, 3, 3)
	DCL_OP2_ARG6(OP_LDRB_M_LSL_IMM_OFF, LSL_IMM, OPDEF_LDR, 1, 0, 1, 0, 3, 3)
	DCL_OP2_ARG6(OP_LDRB_P_LSR_IMM_OFF, LSR_IMM, OPDEF_LDR, 1, 1, 1, 0, 3, 3)
	DCL_OP2_ARG6(OP_LDRB_M_LSR_IMM_OFF, LSR_IMM, OPDEF_LDR, 1, 0, 1, 0, 3, 3)
	DCL_OP2_ARG6(OP_LDRB_P_ASR_IMM_OFF, ASR_IMM, OPDEF_LDR, 1, 1, 1, 0, 3, 3)
	DCL_OP2_ARG6(OP_LDRB_M_ASR_IMM_OFF, ASR_IMM, OPDEF_LDR, 1, 0, 1, 0, 3, 3)
	DCL_OP2_ARG6(OP_LDRB_P_ROR_IMM_OFF, ROR_IMM, OPDEF_LDR, 1, 1, 1, 0, 3, 3)
	DCL_OP2_ARG6(OP_LDRB_M_ROR_IMM_OFF, ROR_IMM, OPDEF_LDR, 1, 0, 1, 0, 3, 3)

	DCL_OP2_ARG6(OP_LDRB_P_IMM_OFF_PREIND, IMM_OFF_12, OPDEF_LDR, 1, 1, 1, 1, 3, 3)
	DCL_OP2_ARG6(OP_LDRB_M_IMM_OFF_PREIND, IMM_OFF_12, OPDEF_LDR, 1, 0, 1, 1, 3, 3)
	DCL_OP2_ARG6(OP_LDRB_P_LSL_IMM_OFF_PREIND, LSL_IMM, OPDEF_LDR, 1, 1, 1, 1, 3, 3)
	DCL_OP2_ARG6(OP_LDRB_M_LSL_IMM_OFF_PREIND, LSL_IMM, OPDEF_LDR, 1, 0, 1, 1, 3, 3)
	DCL_OP2_ARG6(OP_LDRB_P_LSR_IMM_OFF_PREIND, LSR_IMM, OPDEF_LDR, 1, 1, 1, 1, 3, 3)
	DCL_OP2_ARG6(OP_LDRB_M_LSR_IMM_OFF_PREIND, LSR_IMM, OPDEF_LDR, 1, 0, 1, 1, 3, 3)
	DCL_OP2_ARG6(OP_LDRB_P_ASR_IMM_OFF_PREIND, ASR_IMM, OPDEF_LDR, 1, 1, 1, 1, 3, 3)
	DCL_OP2_ARG6(OP_LDRB_M_ASR_IMM_OFF_PREIND, ASR_IMM, OPDEF_LDR, 1, 0, 1, 1, 3, 3)
	DCL_OP2_ARG6(OP_LDRB_P_ROR_IMM_OFF_PREIND, ROR_IMM, OPDEF_LDR, 1, 1, 1, 1, 3, 3)
	DCL_OP2_ARG6(OP_LDRB_M_ROR_IMM_OFF_PREIND, ROR_IMM, OPDEF_LDR, 1, 0, 1, 1, 3, 3)

	DCL_OP2_ARG6(OP_LDRB_P_IMM_OFF_POSTIND, IMM_OFF_12, OPDEF_LDR, 0, 1, 1, 1, 3, 3)
	DCL_OP2_ARG6(OP_LDRB_M_IMM_OFF_POSTIND, IMM_OFF_12, OPDEF_LDR, 0, 0, 1, 1, 3, 3)
	DCL_OP2_ARG6(OP_LDRB_P_LSL_IMM_OFF_POSTIND, LSL_IMM, OPDEF_LDR, 0, 1, 1, 1, 3, 3)
	DCL_OP2_ARG6(OP_LDRB_M_LSL_IMM_OFF_POSTIND, LSL_IMM, OPDEF_LDR, 0, 0, 1, 1, 3, 3)
	DCL_OP2_ARG6(OP_LDRB_P_LSR_IMM_OFF_POSTIND, LSR_IMM, OPDEF_LDR, 0, 1, 1, 1, 3, 3)
	DCL_OP2_ARG6(OP_LDRB_M_LSR_IMM_OFF_POSTIND, LSR_IMM, OPDEF_LDR, 0, 0, 1, 1, 3, 3)
	DCL_OP2_ARG6(OP_LDRB_P_ASR_IMM_OFF_POSTIND, ASR_IMM, OPDEF_LDR, 0, 1, 1, 1, 3, 3)
	DCL_OP2_ARG6(OP_LDRB_M_ASR_IMM_OFF_POSTIND, ASR_IMM, OPDEF_LDR, 0, 0, 1, 1, 3, 3)
	DCL_OP2_ARG6(OP_LDRB_P_ROR_IMM_OFF_POSTIND, ROR_IMM, OPDEF_LDR, 0, 1, 1, 1, 3, 3)
	DCL_OP2_ARG6(OP_LDRB_M_ROR_IMM_OFF_POSTIND, ROR_IMM, OPDEF_LDR, 0, 0, 1, 1, 3, 3)

//-----------------------------------------------------------------------------
//   STR
//-----------------------------------------------------------------------------
#define OPDEF_STR(p,u,b,w,cycle)\
	d->IROp = IR_STR;\
	d->Rn = ARM_REGPOS(opcode.ArmOp, 16);\
	d->Rd = ARM_REGPOS(opcode.ArmOp, 12);\
	d->P = p;\
	d->U = u;\
	d->B = b;\
	d->W = w;\
	d->VariableCycles = 1;\
	d->ExecuteCycles = cycle;\
	d->Reschedule = 2;

	DCL_OP2_ARG5(OP_STR_P_IMM_OFF, IMM_OFF_12, OPDEF_STR, 1, 1, 0, 0, 2)
	DCL_OP2_ARG5(OP_STR_M_IMM_OFF, IMM_OFF_12, OPDEF_STR, 1, 0, 0, 0, 2)
	DCL_OP2_ARG5(OP_STR_P_LSL_IMM_OFF, LSL_IMM, OPDEF_STR, 1, 1, 0, 0, 2)
	DCL_OP2_ARG5(OP_STR_M_LSL_IMM_OFF, LSL_IMM, OPDEF_STR, 1, 0, 0, 0, 2)
	DCL_OP2_ARG5(OP_STR_P_LSR_IMM_OFF, LSR_IMM, OPDEF_STR, 1, 1, 0, 0, 2)
	DCL_OP2_ARG5(OP_STR_M_LSR_IMM_OFF, LSR_IMM, OPDEF_STR, 1, 0, 0, 0, 2)
	DCL_OP2_ARG5(OP_STR_P_ASR_IMM_OFF, ASR_IMM, OPDEF_STR, 1, 1, 0, 0, 2)
	DCL_OP2_ARG5(OP_STR_M_ASR_IMM_OFF, ASR_IMM, OPDEF_STR, 1, 0, 0, 0, 2)
	DCL_OP2_ARG5(OP_STR_P_ROR_IMM_OFF, ROR_IMM, OPDEF_STR, 1, 1, 0, 0, 2)
	DCL_OP2_ARG5(OP_STR_M_ROR_IMM_OFF, ROR_IMM, OPDEF_STR, 1, 0, 0, 0, 2)

	DCL_OP2_ARG5(OP_STR_P_IMM_OFF_PREIND, IMM_OFF_12, OPDEF_STR, 1, 1, 0, 1, 2)
	DCL_OP2_ARG5(OP_STR_M_IMM_OFF_PREIND, IMM_OFF_12, OPDEF_STR, 1, 0, 0, 1, 2)
	DCL_OP2_ARG5(OP_STR_P_LSL_IMM_OFF_PREIND, LSL_IMM, OPDEF_STR, 1, 1, 0, 1, 2)
	DCL_OP2_ARG5(OP_STR_M_LSL_IMM_OFF_PREIND, LSL_IMM, OPDEF_STR, 1, 0, 0, 1, 2)
	DCL_OP2_ARG5(OP_STR_P_LSR_IMM_OFF_PREIND, LSR_IMM, OPDEF_STR, 1, 1, 0, 1, 2)
	DCL_OP2_ARG5(OP_STR_M_LSR_IMM_OFF_PREIND, LSR_IMM, OPDEF_STR, 1, 0, 0, 1, 2)
	DCL_OP2_ARG5(OP_STR_P_ASR_IMM_OFF_PREIND, ASR_IMM, OPDEF_STR, 1, 1, 0, 1, 2)
	DCL_OP2_ARG5(OP_STR_M_ASR_IMM_OFF_PREIND, ASR_IMM, OPDEF_STR, 1, 0, 0, 1, 2)
	DCL_OP2_ARG5(OP_STR_P_ROR_IMM_OFF_PREIND, ROR_IMM, OPDEF_STR, 1, 1, 0, 1, 2)
	DCL_OP2_ARG5(OP_STR_M_ROR_IMM_OFF_PREIND, ROR_IMM, OPDEF_STR, 1, 0, 0, 1, 2)

	DCL_OP2_ARG5(OP_STR_P_IMM_OFF_POSTIND, IMM_OFF_12, OPDEF_STR, 0, 1, 0, 1, 2)
	DCL_OP2_ARG5(OP_STR_M_IMM_OFF_POSTIND, IMM_OFF_12, OPDEF_STR, 0, 0, 0, 1, 2)
	DCL_OP2_ARG5(OP_STR_P_LSL_IMM_OFF_POSTIND, LSL_IMM, OPDEF_STR, 0, 1, 0, 1, 2)
	DCL_OP2_ARG5(OP_STR_M_LSL_IMM_OFF_POSTIND, LSL_IMM, OPDEF_STR, 0, 0, 0, 1, 2)
	DCL_OP2_ARG5(OP_STR_P_LSR_IMM_OFF_POSTIND, LSR_IMM, OPDEF_STR, 0, 1, 0, 1, 2)
	DCL_OP2_ARG5(OP_STR_M_LSR_IMM_OFF_POSTIND, LSR_IMM, OPDEF_STR, 0, 0, 0, 1, 2)
	DCL_OP2_ARG5(OP_STR_P_ASR_IMM_OFF_POSTIND, ASR_IMM, OPDEF_STR, 0, 1, 0, 1, 2)
	DCL_OP2_ARG5(OP_STR_M_ASR_IMM_OFF_POSTIND, ASR_IMM, OPDEF_STR, 0, 0, 0, 1, 2)
	DCL_OP2_ARG5(OP_STR_P_ROR_IMM_OFF_POSTIND, ROR_IMM, OPDEF_STR, 0, 1, 0, 1, 2)
	DCL_OP2_ARG5(OP_STR_M_ROR_IMM_OFF_POSTIND, ROR_IMM, OPDEF_STR, 0, 0, 0, 1, 2)

//-----------------------------------------------------------------------------
//   STREX
//-----------------------------------------------------------------------------
	TEMPLATE u32 FASTCALL OP_STREX(armcpu_t *armcpu, const OPCODE opcode, struct _Decoded* d)
	{
		d->IROp = IR_STREX;
		d->Rn = ARM_REGPOS(opcode.ArmOp, 16);
		d->Rm = ARM_REGPOS(opcode.ArmOp, 12);
		d->Rd = ARM_REGPOS(opcode.ArmOp, 0);
		d->VariableCycles = 1;
		d->ExecuteCycles = 2;
		d->Reschedule = 2;
		return 1;
	}

//-----------------------------------------------------------------------------
//   STRB
//-----------------------------------------------------------------------------
	DCL_OP2_ARG5(OP_STRB_P_IMM_OFF, IMM_OFF_12, OPDEF_STR, 1, 1, 1, 0, 2)
	DCL_OP2_ARG5(OP_STRB_M_IMM_OFF, IMM_OFF_12, OPDEF_STR, 1, 0, 1, 0, 2)
	DCL_OP2_ARG5(OP_STRB_P_LSL_IMM_OFF, LSL_IMM, OPDEF_STR, 1, 1, 1, 0, 2)
	DCL_OP2_ARG5(OP_STRB_M_LSL_IMM_OFF, LSL_IMM, OPDEF_STR, 1, 0, 1, 0, 2)
	DCL_OP2_ARG5(OP_STRB_P_LSR_IMM_OFF, LSR_IMM, OPDEF_STR, 1, 1, 1, 0, 2)
	DCL_OP2_ARG5(OP_STRB_M_LSR_IMM_OFF, LSR_IMM, OPDEF_STR, 1, 0, 1, 0, 2)
	DCL_OP2_ARG5(OP_STRB_P_ASR_IMM_OFF, ASR_IMM, OPDEF_STR, 1, 1, 1, 0, 2)
	DCL_OP2_ARG5(OP_STRB_M_ASR_IMM_OFF, ASR_IMM, OPDEF_STR, 1, 0, 1, 0, 2)
	DCL_OP2_ARG5(OP_STRB_P_ROR_IMM_OFF, ROR_IMM, OPDEF_STR, 1, 1, 1, 0, 2)
	DCL_OP2_ARG5(OP_STRB_M_ROR_IMM_OFF, ROR_IMM, OPDEF_STR, 1, 0, 1, 0, 2)

	DCL_OP2_ARG5(OP_STRB_P_IMM_OFF_PREIND, IMM_OFF_12, OPDEF_STR, 1, 1, 1, 1, 2)
	DCL_OP2_ARG5(OP_STRB_M_IMM_OFF_PREIND, IMM_OFF_12, OPDEF_STR, 1, 0, 1, 1, 2)
	DCL_OP2_ARG5(OP_STRB_P_LSL_IMM_OFF_PREIND, LSL_IMM, OPDEF_STR, 1, 1, 1, 1, 2)
	DCL_OP2_ARG5(OP_STRB_M_LSL_IMM_OFF_PREIND, LSL_IMM, OPDEF_STR, 1, 0, 1, 1, 2)
	DCL_OP2_ARG5(OP_STRB_P_LSR_IMM_OFF_PREIND, LSR_IMM, OPDEF_STR, 1, 1, 1, 1, 2)
	DCL_OP2_ARG5(OP_STRB_M_LSR_IMM_OFF_PREIND, LSR_IMM, OPDEF_STR, 1, 0, 1, 1, 2)
	DCL_OP2_ARG5(OP_STRB_P_ASR_IMM_OFF_PREIND, ASR_IMM, OPDEF_STR, 1, 1, 1, 1, 2)
	DCL_OP2_ARG5(OP_STRB_M_ASR_IMM_OFF_PREIND, ASR_IMM, OPDEF_STR, 1, 0, 1, 1, 2)
	DCL_OP2_ARG5(OP_STRB_P_ROR_IMM_OFF_PREIND, ROR_IMM, OPDEF_STR, 1, 1, 1, 1, 2)
	DCL_OP2_ARG5(OP_STRB_M_ROR_IMM_OFF_PREIND, ROR_IMM, OPDEF_STR, 1, 0, 1, 1, 2)

	DCL_OP2_ARG5(OP_STRB_P_IMM_OFF_POSTIND, IMM_OFF_12, OPDEF_STR, 0, 1, 1, 1, 2)
	DCL_OP2_ARG5(OP_STRB_M_IMM_OFF_POSTIND, IMM_OFF_12, OPDEF_STR, 0, 0, 1, 1, 2)
	DCL_OP2_ARG5(OP_STRB_P_LSL_IMM_OFF_POSTIND, LSL_IMM, OPDEF_STR, 0, 1, 1, 1, 2)
	DCL_OP2_ARG5(OP_STRB_M_LSL_IMM_OFF_POSTIND, LSL_IMM, OPDEF_STR, 0, 0, 1, 1, 2)
	DCL_OP2_ARG5(OP_STRB_P_LSR_IMM_OFF_POSTIND, LSR_IMM, OPDEF_STR, 0, 1, 1, 1, 2)
	DCL_OP2_ARG5(OP_STRB_M_LSR_IMM_OFF_POSTIND, LSR_IMM, OPDEF_STR, 0, 0, 1, 1, 2)
	DCL_OP2_ARG5(OP_STRB_P_ASR_IMM_OFF_POSTIND, ASR_IMM, OPDEF_STR, 0, 1, 1, 1, 2)
	DCL_OP2_ARG5(OP_STRB_M_ASR_IMM_OFF_POSTIND, ASR_IMM, OPDEF_STR, 0, 0, 1, 1, 2)
	DCL_OP2_ARG5(OP_STRB_P_ROR_IMM_OFF_POSTIND, ROR_IMM, OPDEF_STR, 0, 1, 1, 1, 2)
	DCL_OP2_ARG5(OP_STRB_M_ROR_IMM_OFF_POSTIND, ROR_IMM, OPDEF_STR, 0, 0, 1, 1, 2)

//-----------------------------------------------------------------------------
//   LDMIA / LDMIB / LDMDA / LDMDB
//-----------------------------------------------------------------------------
#define OPDEF_LDM(p,u,s,w,cycle_a,cycle_b)\
	d->IROp = IR_LDM;\
	d->Rn = ARM_REGPOS(opcode.ArmOp, 16);\
	d->RegisterList = (opcode.ArmOp & 0xFFFF);\
	d->P = p;\
	d->U = u;\
	d->S = s;\
	d->W = w;\
	d->VariableCycles = 1;\
	d->ExecuteCycles = cycle_a;\
	d->Reschedule = 2;\
	if (s) d->Reschedule = 1;\
	if (BIT15(opcode.ArmOp))\
	{\
		d->ExecuteCycles = cycle_b;\
		d->R15Modified = 1;\
		if (PROCNUM == 0) d->TbitModified = 1;\
		if (s) LOAD_CPSR\
	}

	DCL_OP1_ARG6(OP_LDMIA, OPDEF_LDM, 0, 1, 0, 0, 2, 2)
	DCL_OP1_ARG6(OP_LDMIB, OPDEF_LDM, 1, 1, 0, 0, 2, 2)
	DCL_OP1_ARG6(OP_LDMDA, OPDEF_LDM, 0, 0, 0, 0, 2, 2)
	DCL_OP1_ARG6(OP_LDMDB, OPDEF_LDM, 1, 0, 0, 0, 2, 2)
	DCL_OP1_ARG6(OP_LDMIA_W, OPDEF_LDM, 0, 1, 0, 1, 2, 4)
	DCL_OP1_ARG6(OP_LDMIB_W, OPDEF_LDM, 1, 1, 0, 1, 2, 4)
	DCL_OP1_ARG6(OP_LDMDA_W, OPDEF_LDM, 0, 0, 0, 1, 2, 2)
	DCL_OP1_ARG6(OP_LDMDB_W, OPDEF_LDM, 1, 0, 0, 1, 2, 2)
	DCL_OP1_ARG6(OP_LDMIA2, OPDEF_LDM, 0, 1, 1, 0, 2, 2)
	DCL_OP1_ARG6(OP_LDMIB2, OPDEF_LDM, 1, 1, 1, 0, 2, 2)
	DCL_OP1_ARG6(OP_LDMDA2, OPDEF_LDM, 0, 0, 1, 0, 2, 2)
	DCL_OP1_ARG6(OP_LDMDB2, OPDEF_LDM, 1, 0, 1, 0, 2, 2)
	DCL_OP1_ARG6(OP_LDMIA2_W, OPDEF_LDM, 0, 1, 1, 1, 2, 2)
	DCL_OP1_ARG6(OP_LDMIB2_W, OPDEF_LDM, 1, 1, 1, 1, 2, 2)
	DCL_OP1_ARG6(OP_LDMDA2_W, OPDEF_LDM, 0, 0, 1, 1, 2, 2)
	DCL_OP1_ARG6(OP_LDMDB2_W, OPDEF_LDM, 1, 0, 1, 1, 2, 2)

//-----------------------------------------------------------------------------
//   STMIA / STMIB / STMDA / STMDB
//-----------------------------------------------------------------------------
#define OPDEF_STM(p,u,s,w,cycle)\
	d->IROp = IR_STM;\
	d->Rn = ARM_REGPOS(opcode.ArmOp, 16);\
	d->RegisterList = (opcode.ArmOp & 0xFFFF);\
	d->P = p;\
	d->U = u;\
	d->S = s;\
	d->W = w;\
	d->Reschedule = 2;\
	if (s) d->Reschedule = 1;\
	d->VariableCycles = 1;\
	d->ExecuteCycles = cycle;

	DCL_OP1_ARG5(OP_STMIA, OPDEF_STM, 0, 1, 0, 0, 1)
	DCL_OP1_ARG5(OP_STMIB, OPDEF_STM, 1, 1, 0, 0, 1)
	DCL_OP1_ARG5(OP_STMDA, OPDEF_STM, 0, 0, 0, 0, 1)
	DCL_OP1_ARG5(OP_STMDB, OPDEF_STM, 1, 0, 0, 0, 1)
	DCL_OP1_ARG5(OP_STMIA_W, OPDEF_STM, 0, 1, 0, 1, 1)
	DCL_OP1_ARG5(OP_STMIB_W, OPDEF_STM, 1, 1, 0, 1, 1)
	DCL_OP1_ARG5(OP_STMDA_W, OPDEF_STM, 0, 0, 0, 1, 1)
	DCL_OP1_ARG5(OP_STMDB_W, OPDEF_STM, 1, 0, 0, 1, 1)
	DCL_OP1_ARG5(OP_STMIA2, OPDEF_STM, 0, 1, 1, 0, 1)
	DCL_OP1_ARG5(OP_STMIB2, OPDEF_STM, 1, 1, 1, 0, 1)
	DCL_OP1_ARG5(OP_STMDA2, OPDEF_STM, 0, 0, 1, 0, 1)
	DCL_OP1_ARG5(OP_STMDB2, OPDEF_STM, 1, 0, 1, 0, 1)
	DCL_OP1_ARG5(OP_STMIA2_W, OPDEF_STM, 0, 1, 1, 1, 1)
	DCL_OP1_ARG5(OP_STMIB2_W, OPDEF_STM, 1, 1, 1, 1, 1)
	DCL_OP1_ARG5(OP_STMDA2_W, OPDEF_STM, 0, 0, 1, 1, 1)
	DCL_OP1_ARG5(OP_STMDB2_W, OPDEF_STM, 1, 0, 1, 1, 1)

//-----------------------------------------------------------------------------
//   LDRD / STRD
//-----------------------------------------------------------------------------
	TEMPLATE u32 FASTCALL OP_LDRD_STRD_POST_INDEX(armcpu_t *armcpu, const OPCODE opcode, struct _Decoded* d)
	{
		if (BIT5(opcode.ArmOp)) 
			d->IROp = IR_STRD;
		else
			d->IROp = IR_LDRD;
		d->Rn = ARM_REGPOS(opcode.ArmOp, 16);
		d->Rd = ARM_REGPOS(opcode.ArmOp, 12);
		if (BIT22(opcode.ArmOp))
		{
			d->I = 1;
			d->Immediate = (((opcode.ArmOp>>4)&0xF0)+(opcode.ArmOp&0xF));
		}
		else
		{
			d->I = 0;
			d->Rm = ARM_REGPOS(opcode.ArmOp, 0);
		}
		d->U = BIT23(opcode.ArmOp);
		d->P = 0;
		d->W = 1;
		d->VariableCycles = 1;
		d->ExecuteCycles = 3;
		d->Reschedule = 2;
		return 1;
	}

	TEMPLATE u32 FASTCALL OP_LDRD_STRD_OFFSET_PRE_INDEX(armcpu_t *armcpu, const OPCODE opcode, struct _Decoded* d)
	{
		if (BIT5(opcode.ArmOp)) 
			d->IROp = IR_STRD;
		else
			d->IROp = IR_LDRD;
		d->Rn = ARM_REGPOS(opcode.ArmOp, 16);
		d->Rd = ARM_REGPOS(opcode.ArmOp, 12);
		if (BIT22(opcode.ArmOp))
		{
			d->I = 1;
			d->Immediate = (((opcode.ArmOp>>4)&0xF0)+(opcode.ArmOp&0xF));
		}
		else
		{
			d->I = 0;
			d->Rm = ARM_REGPOS(opcode.ArmOp, 0);
		}
		d->U = BIT23(opcode.ArmOp);
		d->P = 1;
		d->W = BIT21(opcode.ArmOp);
		d->VariableCycles = 1;
		d->ExecuteCycles = 3;
		d->Reschedule = 2;
		return 1;
	}

//-----------------------------------------------------------------------------
//   STC
//   the NDS has no coproc that responses to a STC, no feedback is given to the arm
//-----------------------------------------------------------------------------
	DCL_UNDEF_OP(OP_STC_P_IMM_OFF)
	DCL_UNDEF_OP(OP_STC_M_IMM_OFF)
	DCL_UNDEF_OP(OP_STC_P_PREIND)
	DCL_UNDEF_OP(OP_STC_M_PREIND)
	DCL_UNDEF_OP(OP_STC_P_POSTIND)
	DCL_UNDEF_OP(OP_STC_M_POSTIND)
	DCL_UNDEF_OP(OP_STC_OPTION)

//-----------------------------------------------------------------------------
//   LDC
//   the NDS has no coproc that responses to a LDC, no feedback is given to the arm
//-----------------------------------------------------------------------------
	DCL_UNDEF_OP(OP_LDC_P_IMM_OFF)
	DCL_UNDEF_OP(OP_LDC_M_IMM_OFF)
	DCL_UNDEF_OP(OP_LDC_P_PREIND)
	DCL_UNDEF_OP(OP_LDC_M_PREIND)
	DCL_UNDEF_OP(OP_LDC_P_POSTIND)
	DCL_UNDEF_OP(OP_LDC_M_POSTIND)
	DCL_UNDEF_OP(OP_LDC_OPTION)

//-----------------------------------------------------------------------------
//   MCR / MRC
//-----------------------------------------------------------------------------
	TEMPLATE u32 FASTCALL OP_MCR(armcpu_t *armcpu, const OPCODE opcode, struct _Decoded* d)
	{
		d->IROp = IR_MCR;
		d->CRm = ARM_REGPOS(opcode.ArmOp, 0);
		d->CP = (opcode.ArmOp>>5)&0x7;
		d->CPNum = ARM_REGPOS(opcode.ArmOp, 8);
		d->Rd = ARM_REGPOS(opcode.ArmOp, 12);
		d->CRn = ARM_REGPOS(opcode.ArmOp, 16);
		d->CPOpc = (opcode.ArmOp>>21)&0x7;
		if (d->CPNum == 15)
		{
			if (d->CRn == 1 && d->CRm == 0 && d->CPOpc == 0 && d->CP == 0)
				d->InvalidICache = 2;
			else if (d->CRn == 7 && d->CPOpc == 0)
			{
				if (d->CRm == 0 && d->CP == 4)
					d->Reschedule = 1;
				else if (d->CRm == 5 && d->CP >= 0 && d->CP <= 2)
					d->InvalidICache = 1;
			}
			else if (d->CRn == 9 && d->CRm == 1 && d->CPOpc == 0 && d->CP == 0)
				d->InvalidICache = 2;
		}
		d->ExecuteCycles = 2;
		return 1;
	}

	TEMPLATE u32 FASTCALL OP_MRC(armcpu_t *armcpu, const OPCODE opcode, struct _Decoded* d)
	{
		d->IROp = IR_MRC;
		d->CRm = ARM_REGPOS(opcode.ArmOp, 0);
		d->CP = (opcode.ArmOp>>5)&0x7;
		d->CPNum = ARM_REGPOS(opcode.ArmOp, 8);
		d->Rd = ARM_REGPOS(opcode.ArmOp, 12);
		d->CRn = ARM_REGPOS(opcode.ArmOp, 16);
		d->CPOpc = (opcode.ArmOp>>21)&0x7;
		d->ExecuteCycles = 4;
		if (d->Rd == 15)
		{
			d->R15Modified = 1;
			d->FlagsSet |= ALL_FLAGS;
		}
		return 1;
	}

//-----------------------------------------------------------------------------
//   SWI
//-----------------------------------------------------------------------------
	TEMPLATE u32 FASTCALL OP_SWI(armcpu_t *armcpu, const OPCODE opcode, struct _Decoded* d)
	{
		d->IROp = IR_SWI;
		//d->Immediate = (opcode.ArmOp>>16)&0xFF;
		d->Immediate = ((opcode.ArmOp>>16)&0xFF) & 0x1F;
		bool bypassBuiltinSWI = 
			(armcpu->intVector == 0x00000000 && armcpu->proc_ID==0)
			|| (armcpu->intVector == 0xFFFF0000 && armcpu->proc_ID==1);
		if (armcpu->swi_tab && !bypassBuiltinSWI)
		{
			if (d->Immediate == 0x04 || d->Immediate == 0x05 || d->Immediate == 0x06)
				d->Reschedule = 1;
			if (d->Immediate == 0x04 || d->Immediate == 0x05)
				d->MayHalt = 1;
		}
		else
		{
			d->R15Modified = 1;
			d->Reschedule = 1;
		}
		d->VariableCycles = 1;
		d->ExecuteCycles = 3;
		return 1;
	}

//-----------------------------------------------------------------------------
//   BKPT
//-----------------------------------------------------------------------------
	TEMPLATE u32 FASTCALL OP_BKPT(armcpu_t *armcpu, const OPCODE opcode, struct _Decoded* d)
	{
		d->IROp = IR_BKPT;
		d->R15Modified = 1;
		d->ExecuteCycles = 4;
		return 1;
	}

//-----------------------------------------------------------------------------
//   CDP
//-----------------------------------------------------------------------------
	DCL_UNDEF_OP(OP_CDP)
};

///////////////////////////////////////////////////////////////////////////////////////////////
static const OpDecoder thumb_opdecoder_set[2][1024] = {{
#define TABDECL(x) ThumbOpDecoder::x<0>
#include "thumb_tabdef.inc"
#undef TABDECL
},{
#define TABDECL(x) ThumbOpDecoder::x<1>
#include "thumb_tabdef.inc"
#undef TABDECL
}};
static const OpDecoder arm_cond_opdecoder_set[2][4096] = {{
#define TABDECL(x) ArmOpDecoder::x<0>
#include "instruction_tabdef.inc"
#undef TABDECL
},{
#define TABDECL(x) ArmOpDecoder::x<1>
#include "instruction_tabdef.inc"
#undef TABDECL
}};

///////////////////////////////////////////////////////////////////////////////////////////////
#define ARM_OPCODE_INDEX(i)		((((i)>>16)&0xFF0)|(((i)>>4)&0xF))
#define THUMB_OPCODE_INDEX(i)	((i)>>6)
#define ARM_CONDITION(i)		((i)>>28)
#define THUMB2_HEAD(i)			((i)>>11)

static const u32 FlagList[] = {
    FLAG_Z,
    FLAG_C,
    FLAG_N,
    FLAG_V,
    (FLAG_Z|FLAG_C),
    (FLAG_N|FLAG_V),
    (FLAG_Z|FLAG_N|FLAG_V),
    NO_FLAGS
};

static const char *CondStrings[] ={
    "(EQ) ",
    "(NE) ",
    "(CS) ",
    "(CC) ",
    "(MI) ",
    "(PL) ",
    "(VS) ",
    "(VC) ",
    "(HI) ",
    "(LS) ",
    "(GC) ",
    "(LT) ",
    "(GT) ",
    "(LE) ",
    "(AL) ",
	"(UNCOND)"
};

static const char *FlagStrings[] = {
    "(    )",
    "(   V)",
    "(  C )",
    "(  CV)",
    "( Z  )",
    "( Z V)",
    "( ZC )",
    "( ZCV)",
    "(N   )",
    "(N  V)",
    "(N C )",
    "(N CV)",
    "(NZ  )",
    "(NZ V)",
    "(NZC )",
    "(NZCV)"
};

typedef struct _ProcessorConfig {
	struct ARM
	{
		static u32 PCOffset[2];
		static u32 PCStoreOffset[2];	//IR_STR,IR_STRx,IR_STREX,IR_STM
	};

	struct THUMB
	{
		static u32 PCOffset[2];
	};
} ProcessorConfig;

u32 _ProcessorConfig::ARM::PCOffset[2] = {8,8};
u32 _ProcessorConfig::ARM::PCStoreOffset[2] = {8,12};
u32 _ProcessorConfig::THUMB::PCOffset[2] = {4,4};

u32 Decoded::CalcR15(const Decoded &d)
{
	if (d.ThumbFlag)
		return d.Address + ProcessorConfig::THUMB::PCOffset[d.ProcessID];

	switch (d.IROp)
	{
	case IR_STR:
	case IR_STRx:
	case IR_STREX:
	case IR_STM:
		return d.Address + ProcessorConfig::ARM::PCStoreOffset[d.ProcessID]; 

	default:
		return d.Address + ProcessorConfig::ARM::PCOffset[d.ProcessID];
	}
}

u32 Decoded::CalcNextInstruction(const Decoded &d)
{
	if (d.ThumbFlag)
		return d.Address + 2;
	else
		return d.Address + 4;
}

std::string ArmAnalyze::DumpInstruction(Decoded *Instructions, s32 InstructionsNum)
{
	if (InstructionsNum <= 0)
		return "";

	char dasmbuf[1024] = {0};
	char otherbuf[1024] = {0};
	std::string retbuf;

	retbuf.reserve(1024);

	sprintf(dasmbuf, "CPU : %s, Mode : %s, Count : %d\n", CPU_STR(Instructions[0].ProcessID), Instructions[0].ThumbFlag?"THUMB":"ARM", InstructionsNum);
	retbuf.append(dasmbuf);

	for (s32 i = 0; i < InstructionsNum; i++)
	{
		Decoded &Inst = Instructions[i];

		if (Inst.ThumbFlag)
			des_thumb_instructions_set[THUMB_OPCODE_INDEX(Inst.Instruction.ThumbOp)](Inst.Address, Inst.Instruction.ThumbOp, dasmbuf);
		else
			des_arm_instructions_set[ARM_OPCODE_INDEX(Inst.Instruction.ArmOp)](Inst.Address, Inst.Instruction.ArmOp, dasmbuf);

		sprintf(otherbuf, "%08X : ", Inst.Address);
		retbuf.append(otherbuf);
		retbuf.append(dasmbuf);
		retbuf.append("\n");
	}

	return retbuf;
}

ArmAnalyze::ArmAnalyze(s32 MaxInstructionsNum, s32 MaxBlocksNum)
{
	INFO("sizeof(armcpu_t) = %d\n", sizeof(armcpu_t));
	INFO("sizeof(Decoded) = %d\n", sizeof(Decoded));
	INFO("sizeof(JitBlock) = %d\n", sizeof(JitBlock));

	m_Optimize = false;
	m_OptimizeFlag = false;
	m_MergeSubBlocks = false;
	m_JumpEndDecode = false;

	m_Instructions = new Decoded[MaxInstructionsNum + 1];
	m_MaxInstructionsNum = MaxInstructionsNum + 1;
	m_InstructionsNum = 0;
	if (MaxBlocksNum <= 0) MaxBlocksNum = m_MaxInstructionsNum;
	m_BlockInfos = new BlockInfo[MaxBlocksNum];
	m_MaxBlocksNum = MaxBlocksNum;
	m_BlocksNum = 0;
}

ArmAnalyze::~ArmAnalyze()
{
	delete [] m_Instructions;
	delete [] m_BlockInfos;
}

bool ArmAnalyze::Decode(armcpu_t *armcpu)
{
	s32 InstNum = 0;
	u32 StartAddress = armcpu->instruct_adr;
	u32 AddressStep = 2;

	if (armcpu->CPSR.bits.T)
	{
		StartAddress &= 0xFFFFFFFE;
		AddressStep = 2;
	}
	else
	{
		StartAddress &= 0xFFFFFFFC;
		AddressStep = 4;
	}

	u32 MaxEndAddress = StartAddress + (m_MaxInstructionsNum - 1) * AddressStep;

	memset(m_Instructions, 0, sizeof(Decoded) * m_MaxInstructionsNum);

	bool forceLoad = false;
	bool newblock = true;
	for (InstNum = 0; forceLoad || (InstNum < m_MaxInstructionsNum - 1); InstNum++)
	{
		Decoded &Inst = m_Instructions[InstNum];

		Inst.ProcessID = armcpu->proc_ID;
		Inst.Address = StartAddress + InstNum * AddressStep;

		Inst.ThumbFlag = armcpu->CPSR.bits.T;

		if (newblock)
		{
			Inst.Block = 1;
			newblock = false;
		}

		armcpu->instruct_adr = Inst.Address;

		if (armcpu->CPSR.bits.T)
		{
			Inst.Instruction.ThumbOp = _MMU_read16(Inst.ProcessID, MMU_AT_CODE, Inst.Address);

			Inst.ReadPCMask = 0xFFFFFFFE;

			Inst.Cond = 0xE;

			// THUMB1
			u32 ret = thumb_opdecoder_set[Inst.ProcessID][THUMB_OPCODE_INDEX(Inst.Instruction.ThumbOp)](armcpu, Inst.Instruction, &Inst);
			if (ret == 0)
			{
				INFO("thumb opdecoder failed.\n");

				break;
			}

			// THUMB2
			if (Inst.IROp == IR_T32P1)
				forceLoad = true;
			else if (Inst.IROp == IR_T32P2)
			{
				forceLoad = false;

				if (InstNum == 0)
					INFO("thumb2 only has part2.\n");
				else
				{
					Decoded &InstP1 = m_Instructions[InstNum-1];
					if (InstP1.IROp == IR_T32P1)
					{
						if (THUMB2_HEAD(InstP1.Instruction.ThumbOp) == 0x1E 
							&& THUMB2_HEAD(Inst.Instruction.ThumbOp) == 0x1F)
						{
							//BL IMM
							InstP1.IROp = IR_DUMMY;
							Inst.IROp = IR_BL;

							u32 op1 = (u32)InstP1.Instruction.ThumbOp;
							u32 LR = InstP1.CalcR15(InstP1) + ((((s32)op1<<21)>>21)<<12);
							Inst.Immediate = LR + ((Inst.Instruction.ThumbOp&0x7FF)<<1);
						}
						else if (THUMB2_HEAD(InstP1.Instruction.ThumbOp) == 0x1E 
								&& THUMB2_HEAD(Inst.Instruction.ThumbOp) == 0x1D)
						{
							//BLX IMM
							InstP1.IROp = IR_DUMMY;
							Inst.IROp = IR_BLX_IMM;

							u32 op1 = (u32)InstP1.Instruction.ThumbOp;
							u32 LR = InstP1.CalcR15(InstP1) + ((((s32)op1<<21)>>21)<<12);
							Inst.Immediate = (LR + ((Inst.Instruction.ThumbOp&0x7FF)<<1))&0xFFFFFFFC;
						}
						else
							INFO("thumb2 opdecoder failed.\n");
					}
					else
						INFO("thumb2 only has part2.\n");
				}
			}
			else if (forceLoad && Inst.IROp != IR_T32P2)
			{
				INFO("thumb2 only has part1.\n");
				forceLoad = false;
			}
		}
		else
		{
			Inst.Instruction.ArmOp = _MMU_read32(Inst.ProcessID, MMU_AT_CODE, Inst.Address);

			Inst.ReadPCMask = 0xFFFFFFFC;

			Inst.Cond = ARM_CONDITION(Inst.Instruction.ArmOp);
			Inst.FlagsNeeded = FlagList[Inst.Cond / 2];

			if (Inst.Cond == 0xF)
			{
				//UnconditionalExtension
				Inst.Cond = 0xE;
				if ((Inst.Instruction.ArmOp >> 24) & 0xF)
				{
					u32 off = (((s32)Inst.Instruction.ArmOp<<8)>>8);
					u32 H = (Inst.Instruction.ArmOp >> 24) & 0x1;

					Inst.IROp = IR_BLX_IMM;
					Inst.Immediate = (Inst.CalcR15(Inst) + (off<<2) + H*2)&0xFFFFFFFE;
					Inst.R15Modified = 1;
					Inst.TbitModified = 1;
					Inst.ExecuteCycles = 3;
				}
				else
				{
					INFO("arm uncond opdecoder failed.\n");

					break;
				}
			}
			else
			{
				u32 ret = arm_cond_opdecoder_set[Inst.ProcessID][ARM_OPCODE_INDEX(Inst.Instruction.ArmOp)](armcpu, Inst.Instruction, &Inst);
				if (ret == 0)
				{
					INFO("arm opdecoder failed.\n");

					break;
				}
			}
		}

		if ((Inst.Rd == 15 || Inst.Rn == 15 || Inst.Rm == 15 || Inst.Rs == 15 || (Inst.RegisterList & (1<<15))))
			Inst.R15Used = 1;

		if (Inst.IROp == IR_B || Inst.IROp == IR_BL || Inst.IROp == IR_BLX_IMM)
		{
			if (Inst.Immediate >= StartAddress && Inst.Immediate <= MaxEndAddress)
			{
				u32 slot = (Inst.Immediate - StartAddress) / AddressStep;
				m_Instructions[slot].Block = 1;
			}
		}

		if (Inst.Reschedule == 1 && Inst.Cond != 0xE && Inst.Cond != 0xF)
			Inst.Reschedule = 2;
		else if (Inst.Reschedule == 1 && (Inst.Cond == 0xE || Inst.Cond == 0xF))
			newblock = true;

		if (Inst.MayHalt || Inst.InvalidICache)
			newblock = true;

		if (Inst.IROp == IR_UND)
			break;

		if ((Inst.R15Modified || Inst.TbitModified) 
			&& (m_JumpEndDecode || Inst.Cond == 0xE || Inst.Cond == 0xF))
		{
			InstNum++;
			break;
		}
	}

	armcpu->instruct_adr = StartAddress;

	IF_DEVELOPER(if(InstNum>m_MaxInstructionsNum) INFO("armanalyze overflow(%u, %u).\n", InstNum, m_MaxInstructionsNum););

	m_InstructionsNum = InstNum;

	return InstNum > 0;
}

bool ArmAnalyze::CreateBlocks()
{
	s32 CurBlock = -1;

	s32 InstNum = 0;
	s32 CurBlockInstNum = 0;
	
	for (; InstNum < m_InstructionsNum; InstNum++)
	{
		if (m_Instructions[InstNum].Block == 1)
		{
			if (CurBlock >= 0)
			{
				BlockInfo &Block = m_BlockInfos[CurBlock];

				Block.R15Num = OptimizeFlag(Block.Instructions, CurBlockInstNum);
				Block.SubBlocks = CreateSubBlocks(Block.Instructions, CurBlockInstNum);
				Block.InstructionsNum = Optimize(Block.Instructions, CurBlockInstNum);
			}

			CurBlock++;
			if (CurBlock >= m_MaxBlocksNum)
				break;
			CurBlockInstNum = 1;
			m_BlockInfos[CurBlock].Instructions = &m_Instructions[InstNum];
		}
		else
		{
			CurBlockInstNum++;
		}
	}

	if (CurBlock >= 0)
	{
		BlockInfo &Block = m_BlockInfos[CurBlock];

		Block.R15Num = OptimizeFlag(Block.Instructions, CurBlockInstNum);
		Block.SubBlocks = CreateSubBlocks(Block.Instructions, CurBlockInstNum);
		Block.InstructionsNum = Optimize(Block.Instructions, CurBlockInstNum);
	}

	m_BlocksNum = CurBlock + 1;

	return m_BlocksNum > 0;
}

void ArmAnalyze::GetInstructions(Decoded *&Instructions, s32 &InstructionsNum)
{
	Instructions = m_Instructions;
	InstructionsNum = m_InstructionsNum;
}

void ArmAnalyze::GetBlocks(BlockInfo *&BlockInfos, s32 &BlocksNum)
{
	BlockInfos = m_BlockInfos;
	BlocksNum = m_BlocksNum;
}

s32 ArmAnalyze::Optimize(Decoded *Instructions, s32 InstructionsNum)
{
	if (!m_Optimize)
		return InstructionsNum;

	for (s32 i = 0; i < InstructionsNum; i++)
	{
	}

	return InstructionsNum;
}

u32 ArmAnalyze::OptimizeFlag(Decoded *Instructions, s32 InstructionsNum)
{
	u32 R15Num = 0;

	if (!m_OptimizeFlag)
	{
		for (s32 i = InstructionsNum - 1; i >= 0; i--)
		{
			Decoded &Inst = Instructions[i];

			if (Inst.R15Modified || Inst.R15Used)
				R15Num++;
		}

		return R15Num;
	}

	u32 FlagsToGenerate = 0;
	u32 FlagsNeeded = ALL_FLAGS;
	for (s32 i = InstructionsNum - 1; i >= 0; i--)
	{
		Decoded &Inst = Instructions[i];

		if (Inst.R15Modified || Inst.R15Used)
			R15Num++;

		if (Inst.FlagsNeeded || Inst.FlagsSet || 
			Inst.R15Modified/* || Inst.TbitModified*/)
		{
			FlagsToGenerate = FlagsNeeded & Inst.FlagsSet;

			//INFO("%s", DumpInstruction(&Inst, 1).c_str());
			//INFO("ArmAnalyze::OptimizeFlag() : [%s %s]\n", FlagStrings[Inst.FlagsSet], FlagStrings[FlagsToGenerate]);

			Inst.FlagsSet = FlagsToGenerate;

			if (Inst.Cond != 0xE && Inst.Cond != 0xF)
			{
				FlagsNeeded |= Inst.FlagsNeeded;
			}
			else
			{
				FlagsNeeded = (FlagsNeeded & ~FlagsToGenerate) | Inst.FlagsNeeded;
			}

			if (Inst.R15Modified/* || Inst.TbitModified*/)
			{
				FlagsNeeded = ALL_FLAGS;
			}
		}
	}
	//INFO("\n\n");

	return R15Num;
}

s32 ArmAnalyze::CreateSubBlocks(Decoded *Instructions, s32 InstructionsNum)
{
	if (InstructionsNum <= 0)
		return 0;

	u32 nSubBlocks = 0;
	u32 Cond = Instructions[0].Cond;
	u32 CondFlg = FlagList[Cond / 2];
	bool bNewBlock = true;

	if (m_MergeSubBlocks)
	{
		for (s32 i = 0; i < InstructionsNum; i++)
		{
			Decoded &Inst = Instructions[i];

			if (Cond != Inst.Cond)
				bNewBlock = true;

			if (bNewBlock)
			{
				Cond = Inst.Cond;
				CondFlg = FlagList[Cond / 2];
				nSubBlocks++;

				bNewBlock = false;
			}

			Inst.SubBlock = nSubBlocks;

			if (CondFlg & Inst.FlagsSet)
				bNewBlock = true;
		}
	}
	else
	{
		for (s32 i = 0; i < InstructionsNum; i++)
		{
			Decoded &Inst = Instructions[i];

			if ((Cond != 0xE && Cond != 0xF) || 
				Cond != Inst.Cond)
				bNewBlock = true;

			if (bNewBlock)
			{
				Cond = Inst.Cond;
				nSubBlocks++;

				bNewBlock = false;
			}

			Inst.SubBlock = nSubBlocks;
		}
	}

	return nSubBlocks;
}
