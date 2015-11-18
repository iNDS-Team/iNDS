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
#include "ArmCJit.h"
#include "ArmAnalyze.h"
#include "instructions.h"
#include "Disassembler.h"

#include "armcpu.h"
#include "MMU.h"
#include "MMU_timing.h"
#include "JitCommon.h"
#include "utils/MemBuffer.h"
#include "utils/tinycc/libtcc.h"

#ifdef HAVE_JIT

#define GETCPUPTR (&ARMPROC)
#define GETCPU (ARMPROC)

#define READREG(i) ((i)==15?(d.CalcR15(d)&d.ReadPCMask):GETCPU.R[(i)])

#define REG_R(i)	(i)==15?"_C":"",(i)==15?(u32*)(d.CalcR15(d)&d.ReadPCMask):&(GETCPU.R[(i)])
#define REG_W(i)	(&(GETCPU.R[(i)]))
#define REG(i)		(&(GETCPU.R[(i)]))
#define REGPTR(i)	(&(GETCPU.R[(i)]))

#define TEMPLATE template<u32 PROCNUM> 
#define OPCDECODER_DECL(name) void FASTCALL name##_CDecoder(const Decoded &d, char *&szCodeBuffer)
#define WRITE_CODE(...) szCodeBuffer += sprintf(szCodeBuffer, __VA_ARGS__)

typedef void (FASTCALL* IROpCDecoder)(const Decoded &d, char *&szCodeBuffer);

typedef u32 (FASTCALL* Interpreter)(const Decoded &d);

typedef u32 (FASTCALL* MemOp1)(u32, u32*);
typedef u32 (FASTCALL* MemOp2)(u32, u32);
typedef u32 (* MemOp3)(u32, u32, u32*);
typedef u32 (* MemOp4)(u32, u32*, u32);

// (*(u32*)0x11)
// ((u32 (FASTCALL *)(u32,u32))0x11)(1,1);

// #define REG_R(p)		(*(u32*)p)
// #define REG_SR(p)	(*(s32*)p)
// #define REG_R_C(p)	((u32)p)
// #define REG_SR_C(p)	((s32)p)
// #define REG_W(p)		(*(u32*)p)
// #define REG(p)		(*(u32*)p)
// #define REGPTR(p)	((u32*)p)

namespace ArmCJit
{
//------------------------------------------------------------
//                         Memory type
//------------------------------------------------------------
	enum {
		MEMTYPE_GENERIC = 0,	// no assumptions
		MEMTYPE_MAIN = 1,		// arm9:r/w arm7:r/w
		MEMTYPE_DTCM_ARM9 = 2,	// arm9:r/w
		MEMTYPE_ERAM_ARM7 = 3,	// arm7:r/w
		MEMTYPE_SWIRAM = 4,		// arm9:r/w arm7:r/w

		MEMTYPE_COUNT,
	};

	u32 GuessAddressArea(u32 PROCNUM, u32 adr)
	{
		if(PROCNUM==ARMCPU_ARM9 && (adr & ~0x3FFF) == MMU.DTCMRegion)
			return MEMTYPE_DTCM_ARM9;
		else if((adr & 0x0F000000) == 0x02000000)
			return MEMTYPE_MAIN;
		else if(PROCNUM==ARMCPU_ARM7 && (adr & 0xFF800000) == 0x03800000)
			return MEMTYPE_ERAM_ARM7;
		else if((adr & 0xFF800000) == 0x03000000)
			return MEMTYPE_SWIRAM;
		else
			return MEMTYPE_GENERIC;
	}

	u32 GuessAddressArea(u32 PROCNUM, u32 adr_s, u32 adr_e)
	{
		u32 mt_s = GuessAddressArea(PROCNUM, adr_s);
		u32 mt_e = GuessAddressArea(PROCNUM, adr_e);

		if (mt_s != mt_e)
			return MEMTYPE_GENERIC;

		return mt_s;
	}

//------------------------------------------------------------
//                         Help function
//------------------------------------------------------------
	u32 FASTCALL CalcShiftOp(const Decoded &d)
	{
		u32 PROCNUM = d.ProcessID;
		u32 shiftop = 0;

		switch (d.Typ)
		{
			case IRSHIFT_LSL:
				if (!d.R)
					shiftop = READREG(d.Rm)<<d.Immediate;
				else
				{
					shiftop = READREG(d.Rs)&0xFF;
					if (shiftop >= 32)
						shiftop = 0;
					else
						shiftop = READREG(d.Rm)<<shiftop;
				}
				break;
			case IRSHIFT_LSR:
				if (!d.R)
				{
					if (d.Immediate)
						shiftop = READREG(d.Rm)>>d.Immediate;
					else
						shiftop = 0;
				}
				else
				{
					shiftop = READREG(d.Rs)&0xFF;
					if (shiftop >= 32)
						shiftop = 0;
					else
						shiftop = READREG(d.Rm)>>shiftop;
				}
				break;
			case IRSHIFT_ASR:
				if (!d.R)
				{
					if (d.Immediate)
						shiftop = (u32)(((s32)READREG(d.Rm))>>d.Immediate);
					else
						shiftop = BIT31(READREG(d.Rm))*0xFFFFFFFF;
				}
				else
				{
					shiftop = READREG(d.Rs)&0xFF;
					if (shiftop == 32)
						shiftop = READREG(d.Rm);
					else if(shiftop<32)
						shiftop = (u32)(((s32)READREG(d.Rm))>>shiftop);
					else
						shiftop = BIT31(READREG(d.Rm))*0xFFFFFFFF;
				}
				break;
			case IRSHIFT_ROR:
				if (!d.R)
				{
					if (d.Immediate)
						shiftop = ROR(READREG(d.Rm),d.Immediate);
					else
						shiftop = ((u32)GETCPU.CPSR.bits.C<<31)|(READREG(d.Rm)>>1);
				}
				else
				{
					shiftop = READREG(d.Rs)&0x1F;
					if (shiftop == 0)
						shiftop = READREG(d.Rm);
					else
						shiftop = ROR(READREG(d.Rm),(shiftop&0x1F));
				}
				break;
		}

		return shiftop;
	}

	void FASTCALL IRShiftOpGenerate(const Decoded &d, char *&szCodeBuffer, bool clacCarry)
	{
		u32 PROCNUM = d.ProcessID;

		switch (d.Typ)
		{
		case IRSHIFT_LSL:
			if (!d.R)
			{
				if (clacCarry)
				{
					if (d.Immediate == 0)
						WRITE_CODE("u32 c = ((Status_Reg*)%#p)->bits.C;\n", &(GETCPU.CPSR));
					else
						WRITE_CODE("u32 c = BIT_N(REG_R%s(%#p), %u);\n", REG_R(d.Rm), 32-d.Immediate);
				}

				if (d.Immediate == 0)
					WRITE_CODE("u32 shift_op = REG_R%s(%#p);\n", REG_R(d.Rm));
				else
					WRITE_CODE("u32 shift_op = REG_R%s(%#p)<<%u;\n", REG_R(d.Rm), d.Immediate);
			}
			else
			{
				if (clacCarry)
				{
					WRITE_CODE("u32 c;\n");
					WRITE_CODE("u32 shift_op = REG_R%s(%#p)&0xFF;\n", REG_R(d.Rs));
					WRITE_CODE("if (shift_op == 0){\n");
					WRITE_CODE("c=((Status_Reg*)%#p)->bits.C;\n", &(GETCPU.CPSR));
					WRITE_CODE("shift_op=REG_R%s(%#p);\n", REG_R(d.Rm));
					WRITE_CODE("}else if (shift_op < 32){\n");
					WRITE_CODE("c = BIT_N(REG_R%s(%#p), 32-shift_op);\n", REG_R(d.Rm));
					WRITE_CODE("shift_op = REG_R%s(%#p)<<shift_op;\n", REG_R(d.Rm));
					WRITE_CODE("}else if (shift_op == 32){\n");
					WRITE_CODE("c = BIT0(REG_R%s(%#p));\n", REG_R(d.Rm));
					WRITE_CODE("shift_op=0;\n");
					WRITE_CODE("}else{\n");
					WRITE_CODE("shift_op=c=0;}\n");
				}
				else
				{
					WRITE_CODE("u32 shift_op = REG_R%s(%#p)&0xFF;\n", REG_R(d.Rs));
					WRITE_CODE("if (shift_op >= 32)\n");
					WRITE_CODE("shift_op=0;\n");
					WRITE_CODE("else\n");
					WRITE_CODE("shift_op=REG_R%s(%#p)<<shift_op;\n", REG_R(d.Rm));
				}
			}
			break;
		case IRSHIFT_LSR:
			if (!d.R)
			{
				if (clacCarry)
				{
					if (d.Immediate == 0)
						WRITE_CODE("u32 c = BIT31(REG_R%s(%#p));\n", REG_R(d.Rm));
					else
						WRITE_CODE("u32 c = BIT_N(REG_R%s(%#p), %u);\n", REG_R(d.Rm), d.Immediate-1);
				}

				if (d.Immediate == 0)
					WRITE_CODE("u32 shift_op = 0;\n");
				else
					WRITE_CODE("u32 shift_op = REG_R%s(%#p)>>%u;\n", REG_R(d.Rm), d.Immediate);
			}
			else
			{
				if (clacCarry)
				{
					WRITE_CODE("u32 c;\n");
					WRITE_CODE("u32 shift_op = REG_R%s(%#p)&0xFF;\n", REG_R(d.Rs));
					WRITE_CODE("if (shift_op == 0){\n");
					WRITE_CODE("c=((Status_Reg*)%#p)->bits.C;\n", &(GETCPU.CPSR));
					WRITE_CODE("shift_op=REG_R%s(%#p);\n", REG_R(d.Rm));
					WRITE_CODE("}else if (shift_op < 32){\n");
					WRITE_CODE("c = BIT_N(REG_R%s(%#p), shift_op-1);\n", REG_R(d.Rm));
					WRITE_CODE("shift_op = REG_R%s(%#p)>>shift_op;\n", REG_R(d.Rm));
					WRITE_CODE("}else if (shift_op == 32){\n");
					WRITE_CODE("c = BIT31(REG_R%s(%#p));\n", REG_R(d.Rm));
					WRITE_CODE("shift_op=0;\n");
					WRITE_CODE("}else{\n");
					WRITE_CODE("shift_op=c=0;}\n");
				}
				else
				{
					WRITE_CODE("u32 shift_op = REG_R%s(%#p)&0xFF;\n",REG_R(d.Rs));
					WRITE_CODE("if (shift_op >= 32)\n");
					WRITE_CODE("shift_op=0;\n");
					WRITE_CODE("else\n");
					WRITE_CODE("shift_op=REG_R%s(%#p)>>shift_op;\n", REG_R(d.Rm));
				}
			}
			break;
		case IRSHIFT_ASR:
			if (!d.R)
			{
				if (clacCarry)
				{
					if (d.Immediate == 0)
						WRITE_CODE("u32 c = BIT31(REG_R%s(%#p));\n", REG_R(d.Rm));
					else
						WRITE_CODE("u32 c = BIT_N(REG_R%s(%#p), %u);\n", REG_R(d.Rm), d.Immediate-1);
				}

				if (d.Immediate == 0)
					WRITE_CODE("u32 shift_op = BIT31(REG_R%s(%#p))*0xFFFFFFFF;\n", REG_R(d.Rm));
				else
					WRITE_CODE("u32 shift_op = (u32)(REG_SR%s(%#p)>>%u);\n", REG_R(d.Rm), d.Immediate);
			}
			else
			{
				if (clacCarry)
				{
					WRITE_CODE("u32 c;\n");
					WRITE_CODE("u32 shift_op = REG_R%s(%#p)&0xFF;\n", REG_R(d.Rs));
					WRITE_CODE("if (shift_op == 0){\n");
					WRITE_CODE("c=((Status_Reg*)%#p)->bits.C;\n", &(GETCPU.CPSR));
					WRITE_CODE("shift_op = REG_R%s(%#p);\n", REG_R(d.Rm));
					WRITE_CODE("}else if (shift_op < 32){\n");
					WRITE_CODE("c = BIT_N(REG_R%s(%#p), shift_op-1);\n", REG_R(d.Rm));
					WRITE_CODE("shift_op = (u32)(REG_SR%s(%#p)>>shift_op);\n", REG_R(d.Rm));
					WRITE_CODE("}else{\n");
					WRITE_CODE("c = BIT31(REG_R%s(%#p));\n", REG_R(d.Rm));
					WRITE_CODE("shift_op = BIT31(REG_R%s(%#p))*0xFFFFFFFF;}\n", REG_R(d.Rm));
				}
				else
				{
					WRITE_CODE("u32 shift_op = REG_R%s(%#p)&0xFF;\n", REG_R(d.Rs));
					WRITE_CODE("if (shift_op == 0)\n");
					WRITE_CODE("shift_op = REG_R%s(%#p);\n", REG_R(d.Rm));
					WRITE_CODE("else if (shift_op < 32)\n");
					WRITE_CODE("shift_op = (u32)(REG_SR%s(%#p)>>shift_op);\n", REG_R(d.Rm));
					WRITE_CODE("else\n");
					WRITE_CODE("shift_op = BIT31(REG_R%s(%#p))*0xFFFFFFFF;\n", REG_R(d.Rm));
				}
			}
			break;
		case IRSHIFT_ROR:
			if (!d.R)
			{
				if (clacCarry)
				{
					if (d.Immediate == 0)
						WRITE_CODE("u32 c = BIT0(REG_R%s(%#p));\n", REG_R(d.Rm));
					else
						WRITE_CODE("u32 c = BIT_N(REG_R%s(%#p), %u);\n", REG_R(d.Rm), d.Immediate-1);
				}

				if (d.Immediate == 0)
					WRITE_CODE("u32 shift_op = (((u32)((Status_Reg*)%#p)->bits.C)<<31)|(REG_R%s(%#p)>>1);\n", &(GETCPU.CPSR), REG_R(d.Rm));
				else
					WRITE_CODE("u32 shift_op = ROR(REG_R%s(%#p), %u);\n", REG_R(d.Rm), d.Immediate);
			}
			else
			{
				if (clacCarry)
				{
					WRITE_CODE("u32 c;\n");
					WRITE_CODE("u32 shift_op = REG_R%s(%#p)&0xFF;\n", REG_R(d.Rs));
					WRITE_CODE("if (shift_op == 0){\n");
					WRITE_CODE("c=((Status_Reg*)%#p)->bits.C;\n", &(GETCPU.CPSR));
					WRITE_CODE("shift_op = REG_R%s(%#p);\n", REG_R(d.Rm));
					WRITE_CODE("}else{\n");
					WRITE_CODE("shift_op &= 0x1F;\n");
					WRITE_CODE("if (shift_op != 0){\n");
					WRITE_CODE("c = BIT_N(REG_R%s(%#p), shift_op-1);\n", REG_R(d.Rm));
					WRITE_CODE("shift_op = ROR(REG_R%s(%#p), shift_op);\n", REG_R(d.Rm));
					WRITE_CODE("}else{\n");
					WRITE_CODE("c = BIT31(REG_R%s(%#p));\n", REG_R(d.Rm));
					WRITE_CODE("shift_op = REG_R%s(%#p);}}\n", REG_R(d.Rm));
				
				}
				else
				{
					WRITE_CODE("u32 shift_op = REG_R%s(%#p)&0x1F;\n", REG_R(d.Rs));
					WRITE_CODE("if (shift_op == 0)\n");
					WRITE_CODE("shift_op = REG_R%s(%#p);\n", REG_R(d.Rm));
					WRITE_CODE("else\n");
					WRITE_CODE("shift_op = ROR(REG_R%s(%#p), shift_op);\n", REG_R(d.Rm));
				}
			}
			break;
		default:
			INFO("Unknow Shift Op : %u.\n", d.Typ);
			if (clacCarry)
				WRITE_CODE("u32 c = 0;\n");
			WRITE_CODE("u32 shift_op = 0;\n");
			break;
		}
	}

	void FASTCALL DataProcessLoadCPSRGenerate(const Decoded &d, char *&szCodeBuffer)
	{
		u32 PROCNUM = d.ProcessID;

		WRITE_CODE("{\n");

		WRITE_CODE("Status_Reg SPSR;\n");
		WRITE_CODE("SPSR.val = ((Status_Reg*)%#p)->val;\n", &(GETCPU.SPSR));
		WRITE_CODE("((u32 (*)(void*,u8))%#p)((void*)%#p,SPSR.bits.mode);\n", armcpu_switchMode, GETCPUPTR);
		WRITE_CODE("((Status_Reg*)%#p)->val = SPSR.val;\n", &(GETCPU.CPSR));
		WRITE_CODE("((void (*)(void*))%#p)((void*)%#p);\n", armcpu_changeCPSR, GETCPUPTR);
		WRITE_CODE("REG_W(%#p)&=(0xFFFFFFFC|((((Status_Reg*)%#p)->bits.T)<<1));\n", REG_W(15), &(GETCPU.CPSR));

		WRITE_CODE("}\n");
	}

	void FASTCALL LDM_S_LoadCPSRGenerate(const Decoded &d, char *&szCodeBuffer)
	{
		u32 PROCNUM = d.ProcessID;

		DataProcessLoadCPSRGenerate(d, szCodeBuffer);
	}

	void FASTCALL R15ModifiedGenerate(const Decoded &d, char *&szCodeBuffer)
	{
		u32 PROCNUM = d.ProcessID;

		WRITE_CODE("(*(u32*)%#p) = REG(%#p);\n", &(GETCPU.instruct_adr), REG(15));
		WRITE_CODE("return ExecuteCycles;\n");
	}

//------------------------------------------------------------
//                         IROp decoder
//------------------------------------------------------------
	OPCDECODER_DECL(IR_UND)
	{
		u32 PROCNUM = d.ProcessID;

		INFO("IR_UND\n");

		WRITE_CODE("(*(u32*)%#p) = %u;\n", &(GETCPU.instruction), d.ThumbFlag?d.Instruction.ThumbOp:d.Instruction.ArmOp);
		WRITE_CODE("(*(u32*)%#p) = %u;\n", &(GETCPU.instruct_adr), d.Address);
		WRITE_CODE("((u32 (*)(void*))%#p)((void*)%#p);\n", TRAPUNDEF, GETCPUPTR);
		WRITE_CODE("return ExecuteCycles;\n");
	}

	OPCDECODER_DECL(IR_NOP)
	{
	}

	OPCDECODER_DECL(IR_DUMMY)
	{
	}

	OPCDECODER_DECL(IR_T32P1)
	{
		u32 PROCNUM = d.ProcessID;

		INFO("IR_T32P1\n");

		WRITE_CODE("(*(u32*)%#p) = %u;\n", &(GETCPU.instruction), d.ThumbFlag?d.Instruction.ThumbOp:d.Instruction.ArmOp);
		WRITE_CODE("(*(u32*)%#p) = %u;\n", &(GETCPU.instruct_adr), d.Address);
		WRITE_CODE("((u32 (*)(void*))%#p)((void*)%#p);\n", TRAPUNDEF, GETCPUPTR);
		WRITE_CODE("return ExecuteCycles;\n");
	}

	OPCDECODER_DECL(IR_T32P2)
	{
		u32 PROCNUM = d.ProcessID;

		INFO("IR_T32P2\n");

		WRITE_CODE("(*(u32*)%#p) = %u;\n", &(GETCPU.instruction), d.ThumbFlag?d.Instruction.ThumbOp:d.Instruction.ArmOp);
		WRITE_CODE("(*(u32*)%#p) = %u;\n", &(GETCPU.instruct_adr), d.Address);
		WRITE_CODE("((u32 (*)(void*))%#p)((void*)%#p);\n", TRAPUNDEF, GETCPUPTR);
		WRITE_CODE("return ExecuteCycles;\n");
	}

	OPCDECODER_DECL(IR_MOV)
	{
		u32 PROCNUM = d.ProcessID;

		if (d.I)
		{
			WRITE_CODE("REG_W(%#p)=%u;\n",REG_W(d.Rd), d.Immediate);
			if (d.S && !d.R15Modified)
			{
				if (d.FlagsSet & FLAG_C)
					WRITE_CODE("((Status_Reg*)%#p)->bits.C=%u;\n", &(GETCPU.CPSR), BIT31(d.Immediate));
				if (d.FlagsSet & FLAG_N)
					WRITE_CODE("((Status_Reg*)%#p)->bits.N=%u;\n", &(GETCPU.CPSR), BIT31(d.Immediate));
				if (d.FlagsSet & FLAG_Z)
					WRITE_CODE("((Status_Reg*)%#p)->bits.Z=%u;\n", &(GETCPU.CPSR), d.Immediate==0 ? 1 : 0);
			}
		}
		else
		{
			const bool clacCarry = d.S && !d.R15Modified && (d.FlagsSet & FLAG_C);
			IRShiftOpGenerate(d, szCodeBuffer, clacCarry);

			WRITE_CODE("REG_W(%#p)=shift_op;\n", REG_W(d.Rd));
			if (d.S && !d.R15Modified)
			{
				if (d.FlagsSet & FLAG_C)
					WRITE_CODE("((Status_Reg*)%#p)->bits.C=c;\n", &(GETCPU.CPSR));
				if (d.FlagsSet & FLAG_N)
					WRITE_CODE("((Status_Reg*)%#p)->bits.N=BIT31(shift_op);\n", &(GETCPU.CPSR));
				if (d.FlagsSet & FLAG_Z)
					WRITE_CODE("((Status_Reg*)%#p)->bits.Z=(shift_op==0);\n", &(GETCPU.CPSR));
			}
		}

		if (d.R15Modified)
		{
			if (d.S)
			{
				DataProcessLoadCPSRGenerate(d, szCodeBuffer);
			}

			R15ModifiedGenerate(d, szCodeBuffer);
		}
	}

	OPCDECODER_DECL(IR_MVN)
	{
		u32 PROCNUM = d.ProcessID;

		if (d.I)
		{
			WRITE_CODE("REG_W(%#p)=%u;\n", REG_W(d.Rd), ~d.Immediate);
			if (d.S && !d.R15Modified)
			{
				if (d.FlagsSet & FLAG_C)
					WRITE_CODE("((Status_Reg*)%#p)->bits.C=%u;\n", &(GETCPU.CPSR), BIT31(d.Immediate));
				if (d.FlagsSet & FLAG_N)
					WRITE_CODE("((Status_Reg*)%#p)->bits.N=%u;\n", &(GETCPU.CPSR), BIT31(~d.Immediate));
				if (d.FlagsSet & FLAG_Z)
					WRITE_CODE("((Status_Reg*)%#p)->bits.Z=%u;\n", &(GETCPU.CPSR), (~d.Immediate)==0 ? 1 : 0);
			}
		}
		else
		{
			const bool clacCarry = d.S && !d.R15Modified && (d.FlagsSet & FLAG_C);
			IRShiftOpGenerate(d, szCodeBuffer, clacCarry);

			WRITE_CODE("shift_op=REG_W(%#p)=~shift_op;\n", REG_W(d.Rd));
			if (d.S && !d.R15Modified)
			{
				if (d.FlagsSet & FLAG_C)
					WRITE_CODE("((Status_Reg*)%#p)->bits.C=c;\n", &(GETCPU.CPSR));
				if (d.FlagsSet & FLAG_N)
					WRITE_CODE("((Status_Reg*)%#p)->bits.N=BIT31(shift_op);\n", &(GETCPU.CPSR));
				if (d.FlagsSet & FLAG_Z)
					WRITE_CODE("((Status_Reg*)%#p)->bits.Z=(shift_op==0);\n", &(GETCPU.CPSR));
			}
		}

		if (d.R15Modified)
		{
			if (d.S)
			{
				DataProcessLoadCPSRGenerate(d, szCodeBuffer);
			}

			R15ModifiedGenerate(d, szCodeBuffer);
		}
	}

	OPCDECODER_DECL(IR_AND)
	{
		u32 PROCNUM = d.ProcessID;

		if (d.I)
		{
			WRITE_CODE("u32 shift_op=REG_W(%#p)=REG_R%s(%#p)&%u;\n", REG_W(d.Rd), REG_R(d.Rn), d.Immediate);
			if (d.S && !d.R15Modified)
			{
				if (d.FlagsSet & FLAG_C)
					WRITE_CODE("((Status_Reg*)%#p)->bits.C=%u;\n", &(GETCPU.CPSR), BIT31(d.Immediate));
				if (d.FlagsSet & FLAG_N)
					WRITE_CODE("((Status_Reg*)%#p)->bits.N=BIT31(shift_op);\n", &(GETCPU.CPSR));
				if (d.FlagsSet & FLAG_Z)
					WRITE_CODE("((Status_Reg*)%#p)->bits.Z=(shift_op==0);\n", &(GETCPU.CPSR));
			}
		}
		else
		{
			const bool clacCarry = d.S && !d.R15Modified && (d.FlagsSet & FLAG_C);
			IRShiftOpGenerate(d, szCodeBuffer, clacCarry);

			WRITE_CODE("shift_op=REG_W(%#p)=REG_R%s(%#p)&shift_op;\n", REG_W(d.Rd), REG_R(d.Rn));
			if (d.S && !d.R15Modified)
			{
				if (d.FlagsSet & FLAG_C)
					WRITE_CODE("((Status_Reg*)%#p)->bits.C=c;\n", &(GETCPU.CPSR));
				if (d.FlagsSet & FLAG_N)
					WRITE_CODE("((Status_Reg*)%#p)->bits.N=BIT31(shift_op);\n", &(GETCPU.CPSR));
				if (d.FlagsSet & FLAG_Z)
					WRITE_CODE("((Status_Reg*)%#p)->bits.Z=(shift_op==0);\n", &(GETCPU.CPSR));
			}
		}

		if (d.R15Modified)
		{
			if (d.S)
			{
				DataProcessLoadCPSRGenerate(d, szCodeBuffer);
			}

			R15ModifiedGenerate(d, szCodeBuffer);
		}
	}

	OPCDECODER_DECL(IR_TST)
	{
		u32 PROCNUM = d.ProcessID;

		if (d.I)
		{
			WRITE_CODE("u32 shift_op=REG_R%s(%#p)&%u;\n", REG_R(d.Rn), d.Immediate);

			{
				if (d.FlagsSet & FLAG_C)
					WRITE_CODE("((Status_Reg*)%#p)->bits.C=%u;\n", &(GETCPU.CPSR), BIT31(d.Immediate));
				if (d.FlagsSet & FLAG_N)
					WRITE_CODE("((Status_Reg*)%#p)->bits.N=BIT31(shift_op);\n", &(GETCPU.CPSR));
				if (d.FlagsSet & FLAG_Z)
					WRITE_CODE("((Status_Reg*)%#p)->bits.Z=(shift_op==0);\n", &(GETCPU.CPSR));
			}
		}
		else
		{
			const bool clacCarry = (d.FlagsSet & FLAG_C);
			IRShiftOpGenerate(d, szCodeBuffer, clacCarry);

			WRITE_CODE("shift_op=REG_R%s(%#p)&shift_op;\n", REG_R(d.Rn));
			
			{
				if (d.FlagsSet & FLAG_C)
					WRITE_CODE("((Status_Reg*)%#p)->bits.C=c;\n", &(GETCPU.CPSR));
				if (d.FlagsSet & FLAG_N)
					WRITE_CODE("((Status_Reg*)%#p)->bits.N=BIT31(shift_op);\n", &(GETCPU.CPSR));
				if (d.FlagsSet & FLAG_Z)
					WRITE_CODE("((Status_Reg*)%#p)->bits.Z=(shift_op==0);\n", &(GETCPU.CPSR));
			}
		}
	}

	OPCDECODER_DECL(IR_EOR)
	{
		u32 PROCNUM = d.ProcessID;

		if (d.I)
		{
			WRITE_CODE("u32 shift_op=REG_W(%#p)=REG_R%s(%#p)^%u;\n", REG_W(d.Rd), REG_R(d.Rn), d.Immediate);
			if (d.S && !d.R15Modified)
			{
				if (d.FlagsSet & FLAG_C)
					WRITE_CODE("((Status_Reg*)%#p)->bits.C=%u;\n", &(GETCPU.CPSR), BIT31(d.Immediate));
				if (d.FlagsSet & FLAG_N)
					WRITE_CODE("((Status_Reg*)%#p)->bits.N=BIT31(shift_op);\n", &(GETCPU.CPSR));
				if (d.FlagsSet & FLAG_Z)
					WRITE_CODE("((Status_Reg*)%#p)->bits.Z=(shift_op==0);\n", &(GETCPU.CPSR));
			}
		}
		else
		{
			const bool clacCarry = d.S && !d.R15Modified && (d.FlagsSet & FLAG_C);
			IRShiftOpGenerate(d, szCodeBuffer, clacCarry);

			WRITE_CODE("shift_op=REG_W(%#p)=REG_R%s(%#p)^shift_op;\n", REG_W(d.Rd), REG_R(d.Rn));
			if (d.S && !d.R15Modified)
			{
				if (d.FlagsSet & FLAG_C)
					WRITE_CODE("((Status_Reg*)%#p)->bits.C=c;\n", &(GETCPU.CPSR));
				if (d.FlagsSet & FLAG_N)
					WRITE_CODE("((Status_Reg*)%#p)->bits.N=BIT31(shift_op);\n", &(GETCPU.CPSR));
				if (d.FlagsSet & FLAG_Z)
					WRITE_CODE("((Status_Reg*)%#p)->bits.Z=(shift_op==0);\n", &(GETCPU.CPSR));
			}
		}

		if (d.R15Modified)
		{
			if (d.S)
			{
				DataProcessLoadCPSRGenerate(d, szCodeBuffer);
			}

			R15ModifiedGenerate(d, szCodeBuffer);
		}
	}

	OPCDECODER_DECL(IR_TEQ)
	{
		u32 PROCNUM = d.ProcessID;

		if (d.I)
		{
			WRITE_CODE("u32 shift_op=REG_R%s(%#p)^%u;\n", REG_R(d.Rn), d.Immediate);

			{
				if (d.FlagsSet & FLAG_C)
					WRITE_CODE("((Status_Reg*)%#p)->bits.C=%u;\n", &(GETCPU.CPSR), BIT31(d.Immediate));
				if (d.FlagsSet & FLAG_N)
					WRITE_CODE("((Status_Reg*)%#p)->bits.N=BIT31(shift_op);\n", &(GETCPU.CPSR));
				if (d.FlagsSet & FLAG_Z)
					WRITE_CODE("((Status_Reg*)%#p)->bits.Z=(shift_op==0);\n", &(GETCPU.CPSR));
			}
		}
		else
		{
			const bool clacCarry = (d.FlagsSet & FLAG_C);
			IRShiftOpGenerate(d, szCodeBuffer, clacCarry);

			WRITE_CODE("shift_op=REG_R%s(%#p)^shift_op;\n", REG_R(d.Rn));
			
			{
				if (d.FlagsSet & FLAG_C)
					WRITE_CODE("((Status_Reg*)%#p)->bits.C=c;\n", &(GETCPU.CPSR));
				if (d.FlagsSet & FLAG_N)
					WRITE_CODE("((Status_Reg*)%#p)->bits.N=BIT31(shift_op);\n", &(GETCPU.CPSR));
				if (d.FlagsSet & FLAG_Z)
					WRITE_CODE("((Status_Reg*)%#p)->bits.Z=(shift_op==0);\n", &(GETCPU.CPSR));
			}
		}
	}

	OPCDECODER_DECL(IR_ORR)
	{
		u32 PROCNUM = d.ProcessID;

		if (d.I)
		{
			WRITE_CODE("u32 shift_op=REG_W(%#p)=REG_R%s(%#p)|%u;\n", REG_W(d.Rd), REG_R(d.Rn), d.Immediate);
			if (d.S && !d.R15Modified)
			{
				if (d.FlagsSet & FLAG_C)
					WRITE_CODE("((Status_Reg*)%#p)->bits.C=%u;\n", &(GETCPU.CPSR), BIT31(d.Immediate));
				if (d.FlagsSet & FLAG_N)
					WRITE_CODE("((Status_Reg*)%#p)->bits.N=BIT31(shift_op);\n", &(GETCPU.CPSR));
				if (d.FlagsSet & FLAG_Z)
					WRITE_CODE("((Status_Reg*)%#p)->bits.Z=(shift_op==0);\n", &(GETCPU.CPSR));
			}
		}
		else
		{
			const bool clacCarry = d.S && !d.R15Modified && (d.FlagsSet & FLAG_C);
			IRShiftOpGenerate(d, szCodeBuffer, clacCarry);

			WRITE_CODE("shift_op=REG_W(%#p)=REG_R%s(%#p)|shift_op;\n", REG_W(d.Rd), REG_R(d.Rn));
			if (d.S && !d.R15Modified)
			{
				if (d.FlagsSet & FLAG_C)
					WRITE_CODE("((Status_Reg*)%#p)->bits.C=c;\n", &(GETCPU.CPSR));
				if (d.FlagsSet & FLAG_N)
					WRITE_CODE("((Status_Reg*)%#p)->bits.N=BIT31(shift_op);\n", &(GETCPU.CPSR));
				if (d.FlagsSet & FLAG_Z)
					WRITE_CODE("((Status_Reg*)%#p)->bits.Z=(shift_op==0);\n", &(GETCPU.CPSR));
			}
		}

		if (d.R15Modified)
		{
			if (d.S)
			{
				DataProcessLoadCPSRGenerate(d, szCodeBuffer);
			}

			R15ModifiedGenerate(d, szCodeBuffer);
		}
	}

	OPCDECODER_DECL(IR_BIC)
	{
		u32 PROCNUM = d.ProcessID;

		if (d.I)
		{
			WRITE_CODE("u32 shift_op=REG_W(%#p)=REG_R%s(%#p)&%u;\n", REG_W(d.Rd), REG_R(d.Rn), ~d.Immediate);
			if (d.S && !d.R15Modified)
			{
				if (d.FlagsSet & FLAG_C)
					WRITE_CODE("((Status_Reg*)%#p)->bits.C=%u;\n", &(GETCPU.CPSR), BIT31(d.Immediate));
				if (d.FlagsSet & FLAG_N)
					WRITE_CODE("((Status_Reg*)%#p)->bits.N=BIT31(shift_op);\n", &(GETCPU.CPSR));
				if (d.FlagsSet & FLAG_Z)
					WRITE_CODE("((Status_Reg*)%#p)->bits.Z=(shift_op==0);\n", &(GETCPU.CPSR));
			}
		}
		else
		{
			const bool clacCarry = d.S && !d.R15Modified && (d.FlagsSet & FLAG_C);
			IRShiftOpGenerate(d, szCodeBuffer, clacCarry);

			WRITE_CODE("shift_op=REG_W(%#p)=REG_R%s(%#p)&(~shift_op);\n", REG_W(d.Rd), REG_R(d.Rn));
			if (d.S && !d.R15Modified)
			{
				if (d.FlagsSet & FLAG_C)
					WRITE_CODE("((Status_Reg*)%#p)->bits.C=c;\n", &(GETCPU.CPSR));
				if (d.FlagsSet & FLAG_N)
					WRITE_CODE("((Status_Reg*)%#p)->bits.N=BIT31(shift_op);\n", &(GETCPU.CPSR));
				if (d.FlagsSet & FLAG_Z)
					WRITE_CODE("((Status_Reg*)%#p)->bits.Z=(shift_op==0);\n", &(GETCPU.CPSR));
			}
		}

		if (d.R15Modified)
		{
			if (d.S)
			{
				DataProcessLoadCPSRGenerate(d, szCodeBuffer);
			}

			R15ModifiedGenerate(d, szCodeBuffer);
		}
	}

	OPCDECODER_DECL(IR_ADD)
	{
		u32 PROCNUM = d.ProcessID;

		if (d.I)
		{
			if (d.S && !d.R15Modified && ((d.FlagsSet & FLAG_C) || (d.FlagsSet & FLAG_V)))
				WRITE_CODE("u32 v=REG_R%s(%#p);\n", REG_R(d.Rn));
			WRITE_CODE("REG_W(%#p)=REG_R%s(%#p)+%u;\n", REG_W(d.Rd), REG_R(d.Rn), d.Immediate);
			if (d.S && !d.R15Modified)
			{
				if (d.FlagsSet & FLAG_N)
					WRITE_CODE("((Status_Reg*)%#p)->bits.N=BIT31(REG(%#p));\n", &(GETCPU.CPSR), REG(d.Rd));
				if (d.FlagsSet & FLAG_Z)
					WRITE_CODE("((Status_Reg*)%#p)->bits.Z=(REG(%#p)==0);\n", &(GETCPU.CPSR), REG(d.Rd));
				if (d.FlagsSet & FLAG_C)
					WRITE_CODE("((Status_Reg*)%#p)->bits.C=CarryFrom(v, %u);\n", &(GETCPU.CPSR), d.Immediate);
				if (d.FlagsSet & FLAG_V)
					WRITE_CODE("((Status_Reg*)%#p)->bits.V=OverflowFromADD(REG(%#p), v, %u);\n", &(GETCPU.CPSR), REG(d.Rd), d.Immediate);
			}
		}
		else
		{
			IRShiftOpGenerate(d, szCodeBuffer, false);

			if (d.S && !d.R15Modified && ((d.FlagsSet & FLAG_C) || (d.FlagsSet & FLAG_V)))
				WRITE_CODE("u32 v=REG_R%s(%#p);\n", REG_R(d.Rn));
			WRITE_CODE("REG_W(%#p)=REG_R%s(%#p)+shift_op;\n", REG_W(d.Rd), REG_R(d.Rn));
			if (d.S && !d.R15Modified)
			{
				if (d.FlagsSet & FLAG_N)
					WRITE_CODE("((Status_Reg*)%#p)->bits.N=BIT31(REG(%#p));\n", &(GETCPU.CPSR), REG(d.Rd));
				if (d.FlagsSet & FLAG_Z)
					WRITE_CODE("((Status_Reg*)%#p)->bits.Z=(REG(%#p)==0);\n", &(GETCPU.CPSR), REG(d.Rd));
				if (d.FlagsSet & FLAG_C)
					WRITE_CODE("((Status_Reg*)%#p)->bits.C=CarryFrom(v, shift_op);\n", &(GETCPU.CPSR));
				if (d.FlagsSet & FLAG_V)
					WRITE_CODE("((Status_Reg*)%#p)->bits.V=OverflowFromADD(REG(%#p), v, shift_op);\n", &(GETCPU.CPSR), REG(d.Rd));
			}
		}

		if (d.R15Modified)
		{
			if (d.S)
			{
				DataProcessLoadCPSRGenerate(d, szCodeBuffer);
			}

			R15ModifiedGenerate(d, szCodeBuffer);
		}
	}

	OPCDECODER_DECL(IR_ADC)
	{
		u32 PROCNUM = d.ProcessID;

		if (d.I)
		{
			if (d.S && !d.R15Modified && ((d.FlagsSet & FLAG_C) || (d.FlagsSet & FLAG_V)))
				WRITE_CODE("u32 v=REG_R%s(%#p);\n", REG_R(d.Rn));
			WRITE_CODE("REG_W(%#p)=REG_R%s(%#p)+%u+((Status_Reg*)%#p)->bits.C;\n", REG_W(d.Rd), REG_R(d.Rn), d.Immediate, &(GETCPU.CPSR));
			if (d.S && !d.R15Modified)
			{
				if (d.FlagsSet & FLAG_N)
					WRITE_CODE("((Status_Reg*)%#p)->bits.N=BIT31(REG(%#p));\n", &(GETCPU.CPSR), REG(d.Rd));
				if (d.FlagsSet & FLAG_Z)
					WRITE_CODE("((Status_Reg*)%#p)->bits.Z=(REG(%#p)==0);\n", &(GETCPU.CPSR), REG(d.Rd));
				if (d.FlagsSet & FLAG_V)
					WRITE_CODE("((Status_Reg*)%#p)->bits.V=BIT31((v^%u^-1) & (v^REG(%#p)));\n", &(GETCPU.CPSR), d.Immediate, REG(d.Rd));
				if (d.FlagsSet & FLAG_C)
				{
					WRITE_CODE("if(((Status_Reg*)%#p)->bits.C)\n", &(GETCPU.CPSR));
					WRITE_CODE("((Status_Reg*)%#p)->bits.C=REG(%#p)<=v;\n", &(GETCPU.CPSR), REG(d.Rd));
					WRITE_CODE("else\n");
					WRITE_CODE("((Status_Reg*)%#p)->bits.C=REG(%#p)<v;\n", &(GETCPU.CPSR), REG(d.Rd));
				}
			}
		}
		else
		{
			IRShiftOpGenerate(d, szCodeBuffer, false);

			if (d.S && !d.R15Modified && ((d.FlagsSet & FLAG_C) || (d.FlagsSet & FLAG_V)))
				WRITE_CODE("u32 v=REG_R%s(%#p);\n", REG_R(d.Rn));
			WRITE_CODE("REG_W(%#p)=REG_R%s(%#p)+shift_op+((Status_Reg*)%#p)->bits.C;\n", REG_W(d.Rd), REG_R(d.Rn), &(GETCPU.CPSR));
			if (d.S && !d.R15Modified)
			{
				if (d.FlagsSet & FLAG_N)
					WRITE_CODE("((Status_Reg*)%#p)->bits.N=BIT31(REG(%#p));\n", &(GETCPU.CPSR), REG(d.Rd));
				if (d.FlagsSet & FLAG_Z)
					WRITE_CODE("((Status_Reg*)%#p)->bits.Z=(REG(%#p)==0);\n", &(GETCPU.CPSR), REG(d.Rd));
				if (d.FlagsSet & FLAG_V)
					WRITE_CODE("((Status_Reg*)%#p)->bits.V=BIT31((v^shift_op^-1) & (v^REG(%#p)));\n", &(GETCPU.CPSR), REG(d.Rd));
				if (d.FlagsSet & FLAG_C)
				{
					WRITE_CODE("if(((Status_Reg*)%#p)->bits.C)\n", &(GETCPU.CPSR));
					WRITE_CODE("((Status_Reg*)%#p)->bits.C=REG(%#p)<=v;\n", &(GETCPU.CPSR), REG(d.Rd));
					WRITE_CODE("else\n");
					WRITE_CODE("((Status_Reg*)%#p)->bits.C=REG(%#p)<v;\n", &(GETCPU.CPSR), REG(d.Rd));
				}
			}
		}

		if (d.R15Modified)
		{
			if (d.S)
			{
				DataProcessLoadCPSRGenerate(d, szCodeBuffer);
			}

			R15ModifiedGenerate(d, szCodeBuffer);
		}
	}

	OPCDECODER_DECL(IR_SUB)
	{
		u32 PROCNUM = d.ProcessID;

		if (d.I)
		{
			if (d.S && !d.R15Modified && ((d.FlagsSet & FLAG_C) || (d.FlagsSet & FLAG_V)))
				WRITE_CODE("u32 v=REG_R%s(%#p);\n", REG_R(d.Rn));
			WRITE_CODE("REG_W(%#p)=REG_R%s(%#p)-%u;\n", REG_W(d.Rd), REG_R(d.Rn), d.Immediate);
			if (d.S && !d.R15Modified)
			{
				if (d.FlagsSet & FLAG_N)
					WRITE_CODE("((Status_Reg*)%#p)->bits.N=BIT31(REG(%#p));\n", &(GETCPU.CPSR), REG(d.Rd));
				if (d.FlagsSet & FLAG_Z)
					WRITE_CODE("((Status_Reg*)%#p)->bits.Z=(REG(%#p)==0);\n", &(GETCPU.CPSR), REG(d.Rd));
				if (d.FlagsSet & FLAG_C)
					WRITE_CODE("((Status_Reg*)%#p)->bits.C=!BorrowFrom(v, %u);\n", &(GETCPU.CPSR), d.Immediate);
				if (d.FlagsSet & FLAG_V)
					WRITE_CODE("((Status_Reg*)%#p)->bits.V=OverflowFromSUB(REG(%#p), v, %u);\n", &(GETCPU.CPSR), REG(d.Rd), d.Immediate);
			}
		}
		else
		{
			IRShiftOpGenerate(d, szCodeBuffer, false);

			if (d.S && !d.R15Modified && ((d.FlagsSet & FLAG_C) || (d.FlagsSet & FLAG_V)))
				WRITE_CODE("u32 v=REG_R%s(%#p);\n", REG_R(d.Rn));
			WRITE_CODE("REG_W(%#p)=REG_R%s(%#p)-shift_op;\n", REG_W(d.Rd), REG_R(d.Rn));
			if (d.S && !d.R15Modified)
			{
				if (d.FlagsSet & FLAG_N)
					WRITE_CODE("((Status_Reg*)%#p)->bits.N=BIT31(REG(%#p));\n", &(GETCPU.CPSR), REG(d.Rd));
				if (d.FlagsSet & FLAG_Z)
					WRITE_CODE("((Status_Reg*)%#p)->bits.Z=(REG(%#p)==0);\n", &(GETCPU.CPSR), REG(d.Rd));
				if (d.FlagsSet & FLAG_C)
					WRITE_CODE("((Status_Reg*)%#p)->bits.C=!BorrowFrom(v, shift_op);\n", &(GETCPU.CPSR));
				if (d.FlagsSet & FLAG_V)
					WRITE_CODE("((Status_Reg*)%#p)->bits.V=OverflowFromSUB(REG(%#p), v, shift_op);\n", &(GETCPU.CPSR), REG(d.Rd));
			}
		}

		if (d.R15Modified)
		{
			if (d.S)
			{
				DataProcessLoadCPSRGenerate(d, szCodeBuffer);
			}

			R15ModifiedGenerate(d, szCodeBuffer);
		}
	}

	OPCDECODER_DECL(IR_SBC)
	{
		u32 PROCNUM = d.ProcessID;

		if (d.I)
		{
			if (d.S && !d.R15Modified && ((d.FlagsSet & FLAG_C) || (d.FlagsSet & FLAG_V)))
				WRITE_CODE("u32 v=REG_R%s(%#p);\n", REG_R(d.Rn));
			WRITE_CODE("REG_W(%#p)=REG_R%s(%#p)-%u-!((Status_Reg*)%#p)->bits.C;\n", REG_W(d.Rd), REG_R(d.Rn), d.Immediate, &(GETCPU.CPSR));
			if (d.S && !d.R15Modified)
			{
				if (d.FlagsSet & FLAG_N)
					WRITE_CODE("((Status_Reg*)%#p)->bits.N=BIT31(REG(%#p));\n", &(GETCPU.CPSR), REG(d.Rd));
				if (d.FlagsSet & FLAG_Z)
					WRITE_CODE("((Status_Reg*)%#p)->bits.Z=(REG(%#p)==0);\n", &(GETCPU.CPSR), REG(d.Rd));
				if (d.FlagsSet & FLAG_V)
					WRITE_CODE("((Status_Reg*)%#p)->bits.V=BIT31((v^%u) & (v^REG(%#p)));\n", &(GETCPU.CPSR), d.Immediate, REG(d.Rd));
				if (d.FlagsSet & FLAG_C)
				{
					WRITE_CODE("if(((Status_Reg*)%#p)->bits.C)\n", &(GETCPU.CPSR));
					WRITE_CODE("((Status_Reg*)%#p)->bits.C=v>=%u;\n", &(GETCPU.CPSR), d.Immediate);
					WRITE_CODE("else\n");
					WRITE_CODE("((Status_Reg*)%#p)->bits.C=v>%u;\n", &(GETCPU.CPSR), d.Immediate);
				}
			}
		}
		else
		{
			IRShiftOpGenerate(d, szCodeBuffer, false);

			if (d.S && !d.R15Modified && ((d.FlagsSet & FLAG_C) || (d.FlagsSet & FLAG_V)))
				WRITE_CODE("u32 v=REG_R%s(%#p);\n", REG_R(d.Rn));
			WRITE_CODE("REG_W(%#p)=REG_R%s(%#p)-shift_op-!((Status_Reg*)%#p)->bits.C;\n", REG_W(d.Rd), REG_R(d.Rn), &(GETCPU.CPSR));
			if (d.S && !d.R15Modified)
			{
				if (d.FlagsSet & FLAG_N)
					WRITE_CODE("((Status_Reg*)%#p)->bits.N=BIT31(REG(%#p));\n", &(GETCPU.CPSR), REG(d.Rd));
				if (d.FlagsSet & FLAG_Z)
					WRITE_CODE("((Status_Reg*)%#p)->bits.Z=(REG(%#p)==0);\n", &(GETCPU.CPSR), REG(d.Rd));
				if (d.FlagsSet & FLAG_V)
					WRITE_CODE("((Status_Reg*)%#p)->bits.V=BIT31((v^shift_op) & (v^REG(%#p)));\n", &(GETCPU.CPSR), REG(d.Rd));
				if (d.FlagsSet & FLAG_C)
				{
					WRITE_CODE("if(((Status_Reg*)%#p)->bits.C)\n", &(GETCPU.CPSR));
					WRITE_CODE("((Status_Reg*)%#p)->bits.C=v>=shift_op;\n", &(GETCPU.CPSR));
					WRITE_CODE("else\n");
					WRITE_CODE("((Status_Reg*)%#p)->bits.C=v>shift_op;\n", &(GETCPU.CPSR));
				}
			}
		}

		if (d.R15Modified)
		{
			if (d.S)
			{
				DataProcessLoadCPSRGenerate(d, szCodeBuffer);
			}

			R15ModifiedGenerate(d, szCodeBuffer);
		}
	}

	OPCDECODER_DECL(IR_RSB)
	{
		u32 PROCNUM = d.ProcessID;

		if (d.I)
		{
			if (d.S && !d.R15Modified && ((d.FlagsSet & FLAG_C) || (d.FlagsSet & FLAG_V)))
				WRITE_CODE("u32 v=REG_R%s(%#p);\n", REG_R(d.Rn));
			WRITE_CODE("REG_W(%#p)=%u-REG_R%s(%#p);\n", REG_W(d.Rd), d.Immediate, REG_R(d.Rn));
			if (d.S && !d.R15Modified)
			{
				if (d.FlagsSet & FLAG_N)
					WRITE_CODE("((Status_Reg*)%#p)->bits.N=BIT31(REG(%#p));\n", &(GETCPU.CPSR), REG(d.Rd));
				if (d.FlagsSet & FLAG_Z)
					WRITE_CODE("((Status_Reg*)%#p)->bits.Z=(REG(%#p)==0);\n", &(GETCPU.CPSR), REG(d.Rd));
				if (d.FlagsSet & FLAG_C)
					WRITE_CODE("((Status_Reg*)%#p)->bits.C=!BorrowFrom(%u, v);\n", &(GETCPU.CPSR), d.Immediate);
				if (d.FlagsSet & FLAG_V)
					WRITE_CODE("((Status_Reg*)%#p)->bits.V=OverflowFromSUB(REG(%#p), %u, v);\n", &(GETCPU.CPSR), REG(d.Rd), d.Immediate);
			}
		}
		else
		{
			IRShiftOpGenerate(d, szCodeBuffer, false);

			if (d.S && !d.R15Modified && ((d.FlagsSet & FLAG_C) || (d.FlagsSet & FLAG_V)))
				WRITE_CODE("u32 v=REG_R%s(%#p);\n", REG_R(d.Rn));
			WRITE_CODE("REG_W(%#p)=shift_op-REG_R%s(%#p);\n", REG_W(d.Rd), REG_R(d.Rn));
			if (d.S && !d.R15Modified)
			{
				if (d.FlagsSet & FLAG_N)
					WRITE_CODE("((Status_Reg*)%#p)->bits.N=BIT31(REG(%#p));\n", &(GETCPU.CPSR), REG(d.Rd));
				if (d.FlagsSet & FLAG_Z)
					WRITE_CODE("((Status_Reg*)%#p)->bits.Z=(REG(%#p)==0);\n", &(GETCPU.CPSR), REG(d.Rd));
				if (d.FlagsSet & FLAG_C)
					WRITE_CODE("((Status_Reg*)%#p)->bits.C=!BorrowFrom(shift_op, v);\n", &(GETCPU.CPSR));
				if (d.FlagsSet & FLAG_V)
					WRITE_CODE("((Status_Reg*)%#p)->bits.V=OverflowFromSUB(REG(%#p), shift_op, v);\n", &(GETCPU.CPSR), REG(d.Rd));
			}
		}

		if (d.R15Modified)
		{
			if (d.S)
			{
				DataProcessLoadCPSRGenerate(d, szCodeBuffer);
			}

			R15ModifiedGenerate(d, szCodeBuffer);
		}
	}

	OPCDECODER_DECL(IR_RSC)
	{
		u32 PROCNUM = d.ProcessID;

		if (d.I)
		{
			if (d.S && !d.R15Modified && ((d.FlagsSet & FLAG_C) || (d.FlagsSet & FLAG_V)))
				WRITE_CODE("u32 v=REG_R%s(%#p);\n", REG_R(d.Rn));
			WRITE_CODE("REG_W(%#p)=%u-REG_R%s(%#p)-!((Status_Reg*)%#p)->bits.C;\n", REG_W(d.Rd), d.Immediate, REG_R(d.Rn), &(GETCPU.CPSR));
			if (d.S && !d.R15Modified)
			{
				if (d.FlagsSet & FLAG_N)
					WRITE_CODE("((Status_Reg*)%#p)->bits.N=BIT31(REG(%#p));\n", &(GETCPU.CPSR), REG(d.Rd));
				if (d.FlagsSet & FLAG_Z)
					WRITE_CODE("((Status_Reg*)%#p)->bits.Z=(REG(%#p)==0);\n", &(GETCPU.CPSR), REG(d.Rd));
				if (d.FlagsSet & FLAG_V)
					WRITE_CODE("((Status_Reg*)%#p)->bits.V=BIT31((%u^v) & (%u^REG(%#p)));\n", &(GETCPU.CPSR), d.Immediate, d.Immediate, REG(d.Rd));
				if (d.FlagsSet & FLAG_C)
				{
					WRITE_CODE("if(((Status_Reg*)%#p)->bits.C)\n", &(GETCPU.CPSR));
					WRITE_CODE("((Status_Reg*)%#p)->bits.C=%u>=v;\n", &(GETCPU.CPSR), d.Immediate);
					WRITE_CODE("else\n");
					WRITE_CODE("((Status_Reg*)%#p)->bits.C=%u>v;\n", &(GETCPU.CPSR), d.Immediate);
				}
			}
		}
		else
		{
			IRShiftOpGenerate(d, szCodeBuffer, false);

			if (d.S && !d.R15Modified && ((d.FlagsSet & FLAG_C) || (d.FlagsSet & FLAG_V)))
				WRITE_CODE("u32 v=REG_R%s(%#p);\n", REG_R(d.Rn));
			WRITE_CODE("REG_W(%#p)=shift_op-REG_R%s(%#p)-!((Status_Reg*)%#p)->bits.C;\n", REG_W(d.Rd), REG_R(d.Rn), &(GETCPU.CPSR));
			if (d.S && !d.R15Modified)
			{
				if (d.FlagsSet & FLAG_N)
					WRITE_CODE("((Status_Reg*)%#p)->bits.N=BIT31(REG(%#p));\n", &(GETCPU.CPSR), REG(d.Rd));
				if (d.FlagsSet & FLAG_Z)
					WRITE_CODE("((Status_Reg*)%#p)->bits.Z=(REG(%#p)==0);\n", &(GETCPU.CPSR), REG(d.Rd));
				if (d.FlagsSet & FLAG_V)
					WRITE_CODE("((Status_Reg*)%#p)->bits.V=BIT31((v^shift_op) & (shift_op^REG(%#p)));\n", &(GETCPU.CPSR), REG(d.Rd));
				if (d.FlagsSet & FLAG_C)
				{
					WRITE_CODE("if(((Status_Reg*)%#p)->bits.C)\n", &(GETCPU.CPSR));
					WRITE_CODE("((Status_Reg*)%#p)->bits.C=shift_op>=v;\n", &(GETCPU.CPSR));
					WRITE_CODE("else\n");
					WRITE_CODE("((Status_Reg*)%#p)->bits.C=shift_op>v;\n", &(GETCPU.CPSR));
				}
			}
		}

		if (d.R15Modified)
		{
			if (d.S)
			{
				DataProcessLoadCPSRGenerate(d, szCodeBuffer);
			}

			R15ModifiedGenerate(d, szCodeBuffer);
		}
	}

	OPCDECODER_DECL(IR_CMP)
	{
		u32 PROCNUM = d.ProcessID;

		if (d.I)
		{
			WRITE_CODE("u32 tmp=REG_R%s(%#p)-%u;\n", REG_R(d.Rn), d.Immediate);

			{
				if (d.FlagsSet & FLAG_N)
					WRITE_CODE("((Status_Reg*)%#p)->bits.N=BIT31(tmp);\n", &(GETCPU.CPSR));
				if (d.FlagsSet & FLAG_Z)
					WRITE_CODE("((Status_Reg*)%#p)->bits.Z=(tmp==0);\n", &(GETCPU.CPSR));
				if (d.FlagsSet & FLAG_C)
					WRITE_CODE("((Status_Reg*)%#p)->bits.C=!BorrowFrom(REG_R%s(%#p), %u);\n", &(GETCPU.CPSR), REG_R(d.Rn), d.Immediate);
				if (d.FlagsSet & FLAG_V)
					WRITE_CODE("((Status_Reg*)%#p)->bits.V=OverflowFromSUB(tmp, REG_R%s(%#p), %u);\n", &(GETCPU.CPSR), REG_R(d.Rn), d.Immediate);
			}
		}
		else
		{
			IRShiftOpGenerate(d, szCodeBuffer, false);

			WRITE_CODE("u32 tmp=REG_R%s(%#p)-shift_op;\n", REG_R(d.Rn));

			{
				if (d.FlagsSet & FLAG_N)
					WRITE_CODE("((Status_Reg*)%#p)->bits.N=BIT31(tmp);\n", &(GETCPU.CPSR));
				if (d.FlagsSet & FLAG_Z)
					WRITE_CODE("((Status_Reg*)%#p)->bits.Z=(tmp==0);\n", &(GETCPU.CPSR));
				if (d.FlagsSet & FLAG_C)
					WRITE_CODE("((Status_Reg*)%#p)->bits.C=!BorrowFrom(REG_R%s(%#p), shift_op);\n", &(GETCPU.CPSR), REG_R(d.Rn));
				if (d.FlagsSet & FLAG_V)
					WRITE_CODE("((Status_Reg*)%#p)->bits.V=OverflowFromSUB(tmp, REG_R%s(%#p), shift_op);\n", &(GETCPU.CPSR), REG_R(d.Rn));
			}
		}
	}

	OPCDECODER_DECL(IR_CMN)
	{
		u32 PROCNUM = d.ProcessID;

		if (d.I)
		{
			WRITE_CODE("u32 tmp=REG_R%s(%#p)+%u;\n", REG_R(d.Rn), d.Immediate);

			{
				if (d.FlagsSet & FLAG_N)
					WRITE_CODE("((Status_Reg*)%#p)->bits.N=BIT31(tmp);\n", &(GETCPU.CPSR));
				if (d.FlagsSet & FLAG_Z)
					WRITE_CODE("((Status_Reg*)%#p)->bits.Z=(tmp==0);\n", &(GETCPU.CPSR));
				if (d.FlagsSet & FLAG_C)
					WRITE_CODE("((Status_Reg*)%#p)->bits.C=CarryFrom(REG_R%s(%#p), %u);\n", &(GETCPU.CPSR), REG_R(d.Rn), d.Immediate);
				if (d.FlagsSet & FLAG_V)
					WRITE_CODE("((Status_Reg*)%#p)->bits.V=OverflowFromADD(tmp, REG_R%s(%#p), %u);\n", &(GETCPU.CPSR), REG_R(d.Rn), d.Immediate);
			}
		}
		else
		{
			IRShiftOpGenerate(d, szCodeBuffer, false);

			WRITE_CODE("u32 tmp=REG_R%s(%#p)+shift_op;\n", REG_R(d.Rn));

			{
				if (d.FlagsSet & FLAG_N)
					WRITE_CODE("((Status_Reg*)%#p)->bits.N=BIT31(tmp);\n", &(GETCPU.CPSR));
				if (d.FlagsSet & FLAG_Z)
					WRITE_CODE("((Status_Reg*)%#p)->bits.Z=(tmp==0);\n", &(GETCPU.CPSR));
				if (d.FlagsSet & FLAG_C)
					WRITE_CODE("((Status_Reg*)%#p)->bits.C=CarryFrom(REG_R%s(%#p), shift_op);\n", &(GETCPU.CPSR), REG_R(d.Rn));
				if (d.FlagsSet & FLAG_V)
					WRITE_CODE("((Status_Reg*)%#p)->bits.V=OverflowFromADD(tmp, REG_R%s(%#p), shift_op);\n", &(GETCPU.CPSR), REG_R(d.Rn));
			}
		}
	}

	OPCDECODER_DECL(IR_MUL)
	{
		u32 PROCNUM = d.ProcessID;

		WRITE_CODE("u32 v=REG_R%s(%#p);\n", REG_R(d.Rs));
		WRITE_CODE("REG_W(%#p)=REG_R%s(%#p)*v;\n", REG_W(d.Rd), REG_R(d.Rm));
		if (d.S)
		{
			if (d.FlagsSet & FLAG_N)
				WRITE_CODE("((Status_Reg*)%#p)->bits.N=BIT31(REG(%#p));\n", &(GETCPU.CPSR), REG(d.Rd));
			if (d.FlagsSet & FLAG_Z)
				WRITE_CODE("((Status_Reg*)%#p)->bits.Z=(REG(%#p)==0);\n", &(GETCPU.CPSR), REG(d.Rd));
		}

		WRITE_CODE("v >>= 8;\n");
		WRITE_CODE("if((v==0)||(v==0xFFFFFF)){\n");
		WRITE_CODE("ExecuteCycles+=1+1;\n");
		WRITE_CODE("}else{\n");
		WRITE_CODE("v >>= 8;\n");
		WRITE_CODE("if((v==0)||(v==0xFFFF)){\n");
		WRITE_CODE("ExecuteCycles+=1+2;\n");
		WRITE_CODE("}else{\n");
		WRITE_CODE("v >>= 8;\n");
		WRITE_CODE("if((v==0)||(v==0xFF)){\n");
		WRITE_CODE("ExecuteCycles+=1+3;\n");
		WRITE_CODE("}else{\n");
		WRITE_CODE("ExecuteCycles+=1+4;\n");
		WRITE_CODE("}}}\n");
	}

	OPCDECODER_DECL(IR_MLA)
	{
		u32 PROCNUM = d.ProcessID;

		WRITE_CODE("u32 v=REG_R%s(%#p);\n", REG_R(d.Rs));
		WRITE_CODE("REG_W(%#p)=REG_R%s(%#p)*v+REG_R%s(%#p);\n", REG_W(d.Rd), REG_R(d.Rm), REG_R(d.Rn));
		if (d.S)
		{
			if (d.FlagsSet & FLAG_N)
				WRITE_CODE("((Status_Reg*)%#p)->bits.N=BIT31(REG(%#p));\n", &(GETCPU.CPSR), REG(d.Rd));
			if (d.FlagsSet & FLAG_Z)
				WRITE_CODE("((Status_Reg*)%#p)->bits.Z=(REG(%#p)==0);\n", &(GETCPU.CPSR), REG(d.Rd));
		}

		WRITE_CODE("v >>= 8;\n");
		WRITE_CODE("if((v==0)||(v==0xFFFFFF)){\n");
		WRITE_CODE("ExecuteCycles+=2+1;\n");
		WRITE_CODE("}else{\n");
		WRITE_CODE("v >>= 8;\n");
		WRITE_CODE("if((v==0)||(v==0xFFFF)){\n");
		WRITE_CODE("ExecuteCycles+=2+2;\n");
		WRITE_CODE("}else{\n");
		WRITE_CODE("v >>= 8;\n");
		WRITE_CODE("if((v==0)||(v==0xFF)){\n");
		WRITE_CODE("ExecuteCycles+=2+3;\n");
		WRITE_CODE("}else{\n");
		WRITE_CODE("ExecuteCycles+=2+4;\n");
		WRITE_CODE("}}}\n");
	}

	OPCDECODER_DECL(IR_UMULL)
	{
		u32 PROCNUM = d.ProcessID;

		WRITE_CODE("u32 v=REG_R%s(%#p);\n", REG_R(d.Rs));
		WRITE_CODE("u64 res=(u64)REG_R%s(%#p)*v;\n", REG_R(d.Rm));
		WRITE_CODE("REG_W(%#p)=(u32)res;\n", REG_W(d.Rn));
		WRITE_CODE("REG_W(%#p)=(u32)(res>>32);\n", REG_W(d.Rd));
		if (d.S)
		{
			if (d.FlagsSet & FLAG_N)
				WRITE_CODE("((Status_Reg*)%#p)->bits.N=BIT31(REG(%#p));\n", &(GETCPU.CPSR), REG(d.Rd));
			if (d.FlagsSet & FLAG_Z)
				WRITE_CODE("((Status_Reg*)%#p)->bits.Z=(REG(%#p)==0)&&(REG(%#p)==0);\n", &(GETCPU.CPSR), REG(d.Rd), REG(d.Rn));
		}

		WRITE_CODE("v >>= 8;\n");
		WRITE_CODE("if(v==0){\n");
		WRITE_CODE("ExecuteCycles+=2+1;\n");
		WRITE_CODE("}else{\n");
		WRITE_CODE("v >>= 8;\n");
		WRITE_CODE("if(v==0){\n");
		WRITE_CODE("ExecuteCycles+=2+2;\n");
		WRITE_CODE("}else{\n");
		WRITE_CODE("v >>= 8;\n");
		WRITE_CODE("if(v==0){\n");
		WRITE_CODE("ExecuteCycles+=2+3;\n");
		WRITE_CODE("}else{\n");
		WRITE_CODE("ExecuteCycles+=2+4;\n");
		WRITE_CODE("}}}\n");
	}

	OPCDECODER_DECL(IR_UMLAL)
	{
		u32 PROCNUM = d.ProcessID;

		WRITE_CODE("u32 v=REG_R%s(%#p);\n", REG_R(d.Rs));
		WRITE_CODE("u64 res=(u64)REG_R%s(%#p)*v;\n", REG_R(d.Rm));
		WRITE_CODE("u32 tmp=(u32)res;\n");
		WRITE_CODE("REG_W(%#p)=(u32)(res>>32)+REG_R%s(%#p)+CarryFrom(tmp,REG_R%s(%#p));\n", REG_W(d.Rd), REG_R(d.Rd), REG_R(d.Rn));
		WRITE_CODE("REG_W(%#p)=REG_R%s(%#p)+tmp;\n", REG_W(d.Rn), REG_R(d.Rn));
		if (d.S)
		{
			if (d.FlagsSet & FLAG_N)
				WRITE_CODE("((Status_Reg*)%#p)->bits.N=BIT31(REG(%#p));\n", &(GETCPU.CPSR), REG(d.Rd));
			if (d.FlagsSet & FLAG_Z)
				WRITE_CODE("((Status_Reg*)%#p)->bits.Z=(REG(%#p)==0)&&(REG(%#p)==0);\n", &(GETCPU.CPSR), REG(d.Rd), REG(d.Rn));
		}

		WRITE_CODE("v >>= 8;\n");
		WRITE_CODE("if(v==0){\n");
		WRITE_CODE("ExecuteCycles+=3+1;\n");
		WRITE_CODE("}else{\n");
		WRITE_CODE("v >>= 8;\n");
		WRITE_CODE("if(v==0){\n");
		WRITE_CODE("ExecuteCycles+=3+2;\n");
		WRITE_CODE("}else{\n");
		WRITE_CODE("v >>= 8;\n");
		WRITE_CODE("if(v==0){\n");
		WRITE_CODE("ExecuteCycles+=3+3;\n");
		WRITE_CODE("}else{\n");
		WRITE_CODE("ExecuteCycles+=3+4;\n");
		WRITE_CODE("}}}\n");
	}

	OPCDECODER_DECL(IR_SMULL)
	{
		u32 PROCNUM = d.ProcessID;

		WRITE_CODE("s64 v=REG_SR%s(%#p);\n", REG_R(d.Rs));
		WRITE_CODE("s64 res=(s64)REG_SR%s(%#p)*v;\n", REG_R(d.Rm));
		WRITE_CODE("REG_W(%#p)=(u32)res;\n", REG_W(d.Rn));
		WRITE_CODE("REG_W(%#p)=(u32)(res>>32);\n", REG_W(d.Rd));
		if (d.S)
		{
			if (d.FlagsSet & FLAG_N)
				WRITE_CODE("((Status_Reg*)%#p)->bits.N=BIT31(REG(%#p));\n", &(GETCPU.CPSR), REG(d.Rd));
			if (d.FlagsSet & FLAG_Z)
				WRITE_CODE("((Status_Reg*)%#p)->bits.Z=(REG(%#p)==0)&&(REG(%#p)==0);\n", &(GETCPU.CPSR), REG(d.Rd), REG(d.Rn));
		}

		WRITE_CODE("u32 v2 = v&0xFFFFFFFF;\n");
		WRITE_CODE("v2 >>= 8;\n");
		WRITE_CODE("if((v2==0)||(v2==0xFFFFFF)){\n");
		WRITE_CODE("ExecuteCycles+=2+1;\n");
		WRITE_CODE("}else{\n");
		WRITE_CODE("v2 >>= 8;\n");
		WRITE_CODE("if((v2==0)||(v2==0xFFFF)){\n");
		WRITE_CODE("ExecuteCycles+=2+2;\n");
		WRITE_CODE("}else{\n");
		WRITE_CODE("v2 >>= 8;\n");
		WRITE_CODE("if((v2==0)||(v2==0xFF)){\n");
		WRITE_CODE("ExecuteCycles+=2+3;\n");
		WRITE_CODE("}else{\n");
		WRITE_CODE("ExecuteCycles+=2+4;\n");
		WRITE_CODE("}}}\n");
	}

	OPCDECODER_DECL(IR_SMLAL)
	{
		u32 PROCNUM = d.ProcessID;

		WRITE_CODE("s64 v=REG_SR%s(%#p);\n", REG_R(d.Rs));
		WRITE_CODE("s64 res=(s64)REG_SR%s(%#p)*v;\n", REG_R(d.Rm));
		WRITE_CODE("u32 tmp=(u32)res;\n");
		WRITE_CODE("REG_W(%#p)=(u32)(res>>32)+REG_R%s(%#p)+CarryFrom(tmp,REG_R%s(%#p));\n", REG_W(d.Rd), REG_R(d.Rd), REG_R(d.Rn));
		WRITE_CODE("REG_W(%#p)=REG_R%s(%#p)+tmp;\n", REG_W(d.Rn), REG_R(d.Rn));
		if (d.S)
		{
			if (d.FlagsSet & FLAG_N)
				WRITE_CODE("((Status_Reg*)%#p)->bits.N=BIT31(REG(%#p));\n", &(GETCPU.CPSR), REG(d.Rd));
			if (d.FlagsSet & FLAG_Z)
				WRITE_CODE("((Status_Reg*)%#p)->bits.Z=(REG(%#p)==0)&&(REG(%#p)==0);\n", &(GETCPU.CPSR), REG(d.Rd), REG(d.Rn));
		}

		WRITE_CODE("u32 v2 = v&0xFFFFFFFF;\n");
		WRITE_CODE("v2 >>= 8;\n");
		WRITE_CODE("if((v2==0)||(v2==0xFFFFFF)){\n");
		WRITE_CODE("ExecuteCycles+=3+1;\n");
		WRITE_CODE("}else{\n");
		WRITE_CODE("v2 >>= 8;\n");
		WRITE_CODE("if((v2==0)||(v2==0xFFFF)){\n");
		WRITE_CODE("ExecuteCycles+=3+2;\n");
		WRITE_CODE("}else{\n");
		WRITE_CODE("v2 >>= 8;\n");
		WRITE_CODE("if((v2==0)||(v2==0xFF)){\n");
		WRITE_CODE("ExecuteCycles+=3+3;\n");
		WRITE_CODE("}else{\n");
		WRITE_CODE("ExecuteCycles+=3+4;\n");
		WRITE_CODE("}}}\n");
	}

	OPCDECODER_DECL(IR_SMULxy)
	{
		u32 PROCNUM = d.ProcessID;

		WRITE_CODE("REG_W(%#p)=(u32)(", REG_W(d.Rd));
		if (d.X)
			WRITE_CODE("HWORD(");
		else
			WRITE_CODE("LWORD(");
		WRITE_CODE("REG_R%s(%#p))*", REG_R(d.Rm));
		if (d.Y)
			WRITE_CODE("HWORD(");
		else
			WRITE_CODE("LWORD(");
		WRITE_CODE("REG_R%s(%#p)));\n", REG_R(d.Rs));
	}

	OPCDECODER_DECL(IR_SMLAxy)
	{
		u32 PROCNUM = d.ProcessID;

		if (!d.X && !d.Y)
		{
			WRITE_CODE("u32 tmp=(u32)((s16)REG_R%s(%#p) * (s16)REG_R%s(%#p));\n", REG_R(d.Rm), REG_R(d.Rs));
			WRITE_CODE("REG_W(%#p) = tmp + REG_R%s(%#p);\n", REG_W(d.Rd), REG_R(d.Rn));
			WRITE_CODE("if (OverflowFromADD(REG(%#p), tmp, REG_R%s(%#p)))\n", REG(d.Rd), REG_R(d.Rn));
			WRITE_CODE("((Status_Reg*)%#p)->bits.Q=1;\n", &(GETCPU.CPSR));
		}
		else
		{
			WRITE_CODE("u32 tmp=(u32)(");
			if (d.X)
				WRITE_CODE("HWORD(");
			else
				WRITE_CODE("LWORD(");
			WRITE_CODE("REG_R%s(%#p))*", REG_R(d.Rm));
			if (d.Y)
				WRITE_CODE("HWORD(");
			else
				WRITE_CODE("LWORD(");
			WRITE_CODE("REG_R%s(%#p)));\n", REG_R(d.Rs));
			WRITE_CODE("u32 a = REG_R%s(%#p);\n", REG_R(d.Rn));
			WRITE_CODE("REG_W(%#p) = tmp + a;\n", REG_W(d.Rd));
			WRITE_CODE("if (SIGNED_OVERFLOW(tmp, a, REG(%#p)))\n", REG(d.Rd));
			WRITE_CODE("((Status_Reg*)%#p)->bits.Q=1;\n", &(GETCPU.CPSR));
		}
	}

	OPCDECODER_DECL(IR_SMULWy)
	{
		u32 PROCNUM = d.ProcessID;

		WRITE_CODE("s64 tmp = (s64)");
		if (d.Y)
			WRITE_CODE("HWORD(");
		else
			WRITE_CODE("LWORD(");
		WRITE_CODE("REG_R%s(%#p)) * (s64)((s32)REG_R%s(%#p));\n", REG_R(d.Rs), REG_R(d.Rm));
		WRITE_CODE("REG_W(%#p) = ((tmp>>16)&0xFFFFFFFF);\n", REG_W(d.Rd));
	}

	OPCDECODER_DECL(IR_SMLAWy)
	{
		u32 PROCNUM = d.ProcessID;

		WRITE_CODE("s64 tmp = (s64)");
		if (d.Y)
			WRITE_CODE("HWORD(");
		else
			WRITE_CODE("LWORD(");
		WRITE_CODE("REG_R%s(%#p)) * (s64)((s32)REG_R%s(%#p));\n", REG_R(d.Rs), REG_R(d.Rm));
		WRITE_CODE("u32 a = REG_R%s(%#p);\n", REG_R(d.Rn));
		WRITE_CODE("tmp = ((tmp>>16)&0xFFFFFFFF);\n");
		WRITE_CODE("REG_W(%#p) = tmp + a;\n", REG_W(d.Rd));
		WRITE_CODE("if (SIGNED_OVERFLOW((u32)tmp, a, REG(%#p)))\n", REG(d.Rd));
		WRITE_CODE("((Status_Reg*)%#p)->bits.Q=1;\n", &(GETCPU.CPSR));
	}

	OPCDECODER_DECL(IR_SMLALxy)
	{
		u32 PROCNUM = d.ProcessID;

		WRITE_CODE("s64 tmp=(s64)(");
		if (d.X)
			WRITE_CODE("HWORD(");
		else
			WRITE_CODE("LWORD(");
		WRITE_CODE("REG_R%s(%#p))*", REG_R(d.Rm));
		if (d.Y)
			WRITE_CODE("HWORD(");
		else
			WRITE_CODE("LWORD(");
		WRITE_CODE("REG_R%s(%#p)));\n", REG_R(d.Rs));
		WRITE_CODE("u64 res = (u64)tmp + REG_R%s(%#p);\n", REG_R(d.Rn));
		WRITE_CODE("REG_W(%#p) = (u32)res;\n", REG_W(d.Rn));
		WRITE_CODE("REG_W(%#p) = REG_R%s(%#p) + (res + ((tmp<0)*0xFFFFFFFF));\n", REG_W(d.Rd), REG_R(d.Rd));
	}

	template<u32 PROCNUM, u32 memtype, u32 cycle>
	static u32 FASTCALL MEMOP_LDR(u32 adr, u32 *dstreg)
	{
		u32 data = READ32(GETCPU.mem_if->data, adr);
		if(adr&3)
			data = ROR(data, 8*(adr&3));
		*dstreg = data;
		return MMU_aluMemAccessCycles<PROCNUM,32,MMU_AD_READ>(cycle,adr);
	}

	template<u32 PROCNUM, u32 memtype, u32 cycle>
	static u32 FASTCALL MEMOP_LDRB(u32 adr, u32 *dstreg)
	{
		*dstreg = READ8(GETCPU.mem_if->data, adr);
		return MMU_aluMemAccessCycles<PROCNUM,8,MMU_AD_READ>(cycle,adr);
	}

	static const MemOp1 LDR_Tab[2][MEMTYPE_COUNT] = 
	{
		{
			MEMOP_LDR<0,MEMTYPE_GENERIC,3>,
			MEMOP_LDR<0,MEMTYPE_MAIN,3>,
			MEMOP_LDR<0,MEMTYPE_DTCM_ARM9,3>,
			MEMOP_LDR<0,MEMTYPE_GENERIC,3>,//MEMOP_LDR<0,MEMTYPE_ERAM_ARM7,3>,
			MEMOP_LDR<0,MEMTYPE_SWIRAM,3>,
		},
		{
			MEMOP_LDR<1,MEMTYPE_GENERIC,3>,
			MEMOP_LDR<1,MEMTYPE_MAIN,3>,
			MEMOP_LDR<1,MEMTYPE_GENERIC,3>,//MEMOP_LDR<1,MEMTYPE_DTCM_ARM9,3>,
			MEMOP_LDR<1,MEMTYPE_ERAM_ARM7,3>,
			MEMOP_LDR<1,MEMTYPE_SWIRAM,3>,
		}
	};

	static const MemOp1 LDR_R15_Tab[2][MEMTYPE_COUNT] = 
	{
		{
			MEMOP_LDR<0,MEMTYPE_GENERIC,5>,
			MEMOP_LDR<0,MEMTYPE_MAIN,5>,
			MEMOP_LDR<0,MEMTYPE_DTCM_ARM9,5>,
			MEMOP_LDR<0,MEMTYPE_GENERIC,5>,//MEMOP_LDR<0,MEMTYPE_ERAM_ARM7,5>,
			MEMOP_LDR<0,MEMTYPE_SWIRAM,5>,
		},
		{
			MEMOP_LDR<1,MEMTYPE_GENERIC,5>,
			MEMOP_LDR<1,MEMTYPE_MAIN,5>,
			MEMOP_LDR<1,MEMTYPE_GENERIC,5>,//MEMOP_LDR<1,MEMTYPE_DTCM_ARM9,5>,
			MEMOP_LDR<1,MEMTYPE_ERAM_ARM7,5>,
			MEMOP_LDR<1,MEMTYPE_SWIRAM,5>,
		}
	};

	static const MemOp1 LDRB_Tab[2][MEMTYPE_COUNT] = 
	{
		{
			MEMOP_LDRB<0,MEMTYPE_GENERIC,3>,
			MEMOP_LDRB<0,MEMTYPE_MAIN,3>,
			MEMOP_LDRB<0,MEMTYPE_DTCM_ARM9,3>,
			MEMOP_LDRB<0,MEMTYPE_GENERIC,3>,//MEMOP_LDRB<0,MEMTYPE_ERAM_ARM7,3>,
			MEMOP_LDRB<0,MEMTYPE_SWIRAM,3>,
		},
		{
			MEMOP_LDRB<1,MEMTYPE_GENERIC,3>,
			MEMOP_LDRB<1,MEMTYPE_MAIN,3>,
			MEMOP_LDRB<1,MEMTYPE_GENERIC,3>,//MEMOP_LDRB<1,MEMTYPE_DTCM_ARM9,3>,
			MEMOP_LDRB<1,MEMTYPE_ERAM_ARM7,3>,
			MEMOP_LDRB<1,MEMTYPE_SWIRAM,3>,
		}
	};

	OPCDECODER_DECL(IR_LDR)
	{
		u32 PROCNUM = d.ProcessID;
		u32 adr_guess = 0;

		if (d.P)
		{
			if (d.I)
			{
				WRITE_CODE("u32 adr = REG_R%s(%#p) %c %u;\n", REG_R(d.Rn), d.U?'+':'-', d.Immediate);

				adr_guess = READREG(d.Rn) + (d.Immediate * (d.U?1:-1));
			}
			else
			{
				IRShiftOpGenerate(d, szCodeBuffer, false);

				WRITE_CODE("u32 adr = REG_R%s(%#p) %c shift_op;\n", REG_R(d.Rn), d.U?'+':'-');

				adr_guess = READREG(d.Rn) + (CalcShiftOp(d) * (d.U?1:-1));
			}

			if (d.W)
				WRITE_CODE("REG_W(%#p) = adr;\n", REG_W(d.Rn));
		}
		else
		{
			WRITE_CODE("u32 adr = REG_R%s(%#p);\n", REG_R(d.Rn));
			if (d.I)
				WRITE_CODE("REG_W(%#p) = adr %c %u;\n", REG_W(d.Rn), d.U?'+':'-', d.Immediate);
			else
			{
				IRShiftOpGenerate(d, szCodeBuffer, false);

				WRITE_CODE("REG_W(%#p) = adr %c shift_op;\n", REG_W(d.Rn), d.U?'+':'-');
			}

			adr_guess = READREG(d.Rn);
		}

		if (d.B)
			WRITE_CODE("ExecuteCycles+=((u32 (FASTCALL *)(u32, u32*))%#p)(adr,REGPTR(%#p));\n", LDRB_Tab[PROCNUM][GuessAddressArea(PROCNUM,adr_guess)], REGPTR(d.Rd));
		else
		{
			if (d.R15Modified)
			{
				WRITE_CODE("ExecuteCycles+=((u32 (FASTCALL *)(u32, u32*))%#p)(adr,REGPTR(%#p));\n", LDR_R15_Tab[PROCNUM][GuessAddressArea(PROCNUM,adr_guess)], REGPTR(d.Rd));

				if (PROCNUM == 0)
				{
					WRITE_CODE("((Status_Reg*)%#p)->bits.T=BIT0(REG(%#p));\n", &(GETCPU.CPSR), REG(15));
					WRITE_CODE("REG(%#p) &= 0xFFFFFFFE;\n", REG(15));
				}
				else
					WRITE_CODE("REG(%#p) &= 0xFFFFFFFC;\n", REG(15));

				R15ModifiedGenerate(d, szCodeBuffer);
			}
			else
				WRITE_CODE("ExecuteCycles+=((u32 (FASTCALL *)(u32, u32*))%#p)(adr,REGPTR(%#p));\n", LDR_Tab[PROCNUM][GuessAddressArea(PROCNUM,adr_guess)], REGPTR(d.Rd));
		}
	}

	template<u32 PROCNUM, u32 memtype, u32 cycle>
	static u32 FASTCALL MEMOP_STR(u32 adr, u32 data)
	{
		WRITE32(GETCPU.mem_if->data, adr, data);
		return MMU_aluMemAccessCycles<PROCNUM,32,MMU_AD_WRITE>(cycle,adr);
	}

	template<u32 PROCNUM, u32 memtype, u32 cycle>
	static u32 FASTCALL MEMOP_STRB(u32 adr, u32 data)
	{
		WRITE8(GETCPU.mem_if->data, adr, data);
		return MMU_aluMemAccessCycles<PROCNUM,8,MMU_AD_WRITE>(cycle,adr);
	}

	static const MemOp2 STR_Tab[2][MEMTYPE_COUNT] = 
	{
		{
			MEMOP_STR<0,MEMTYPE_GENERIC,2>,
			MEMOP_STR<0,MEMTYPE_MAIN,2>,
			MEMOP_STR<0,MEMTYPE_DTCM_ARM9,2>,
			MEMOP_STR<0,MEMTYPE_GENERIC,2>,//MEMOP_STR<0,MEMTYPE_ERAM_ARM7,2>,
			MEMOP_STR<0,MEMTYPE_SWIRAM,2>,
		},
		{
			MEMOP_STR<1,MEMTYPE_GENERIC,2>,
			MEMOP_STR<1,MEMTYPE_MAIN,2>,
			MEMOP_STR<1,MEMTYPE_GENERIC,2>,//MEMOP_STR<1,MEMTYPE_DTCM_ARM9,2>,
			MEMOP_STR<1,MEMTYPE_ERAM_ARM7,2>,
			MEMOP_STR<1,MEMTYPE_SWIRAM,2>,
		}
	};

	static const MemOp2 STRB_Tab[2][MEMTYPE_COUNT] = 
	{
		{
			MEMOP_STRB<0,MEMTYPE_GENERIC,2>,
			MEMOP_STRB<0,MEMTYPE_MAIN,2>,
			MEMOP_STRB<0,MEMTYPE_DTCM_ARM9,2>,
			MEMOP_STRB<0,MEMTYPE_GENERIC,2>,//MEMOP_STRB<0,MEMTYPE_ERAM_ARM7,2>,
			MEMOP_STRB<0,MEMTYPE_SWIRAM,2>,
		},
		{
			MEMOP_STRB<1,MEMTYPE_GENERIC,2>,
			MEMOP_STRB<1,MEMTYPE_MAIN,2>,
			MEMOP_STRB<1,MEMTYPE_GENERIC,2>,//MEMOP_STRB<1,MEMTYPE_DTCM_ARM9,2>,
			MEMOP_STRB<1,MEMTYPE_ERAM_ARM7,2>,
			MEMOP_STRB<1,MEMTYPE_SWIRAM,2>,
		}
	};

	OPCDECODER_DECL(IR_STR)
	{
		u32 PROCNUM = d.ProcessID;
		u32 adr_guess = 0;

		if (d.P)
		{
			if (d.I)
			{
				WRITE_CODE("u32 adr = REG_R%s(%#p) %c %u;\n", REG_R(d.Rn), d.U?'+':'-', d.Immediate);

				adr_guess = READREG(d.Rn) + (d.Immediate * (d.U?1:-1));
			}
			else
			{
				IRShiftOpGenerate(d, szCodeBuffer, false);

				WRITE_CODE("u32 adr = REG_R%s(%#p) %c shift_op;\n", REG_R(d.Rn), d.U?'+':'-');

				adr_guess = READREG(d.Rn) + (CalcShiftOp(d) * (d.U?1:-1));
			}

			if (d.W)
				WRITE_CODE("REG_W(%#p) = adr;\n", REG_W(d.Rn));
		}
		else
		{
			WRITE_CODE("u32 adr = REG_R%s(%#p);\n", REG_R(d.Rn));

			adr_guess = READREG(d.Rn);
		}

		if (d.B)
			WRITE_CODE("ExecuteCycles+=((u32 (FASTCALL *)(u32, u32))%#p)(adr,REG_R%s(%#p));\n", STRB_Tab[PROCNUM][GuessAddressArea(PROCNUM,adr_guess)], REG_R(d.Rd));
		else
			WRITE_CODE("ExecuteCycles+=((u32 (FASTCALL *)(u32, u32))%#p)(adr,REG_R%s(%#p));\n", STR_Tab[PROCNUM][GuessAddressArea(PROCNUM,adr_guess)], REG_R(d.Rd));

		if (!d.P)
		{
			if (d.I)
				WRITE_CODE("REG_W(%#p) = adr %c %u;\n", REG_W(d.Rn), d.U?'+':'-', d.Immediate);
			else
			{
				IRShiftOpGenerate(d, szCodeBuffer, false);

				WRITE_CODE("REG_W(%#p) = adr %c shift_op;\n", REG_W(d.Rn), d.U?'+':'-');
			}
		}
	}

	template<u32 PROCNUM, u32 memtype, u32 cycle>
	static u32 FASTCALL MEMOP_LDRH(u32 adr, u32 *dstreg)
	{
		*dstreg = READ16(GETCPU.mem_if->data, adr);
		return MMU_aluMemAccessCycles<PROCNUM,16,MMU_AD_READ>(cycle,adr);
	}

	template<u32 PROCNUM, u32 memtype, u32 cycle>
	static u32 FASTCALL MEMOP_LDRSH(u32 adr, u32 *dstreg)
	{
		*dstreg = (s16)READ16(GETCPU.mem_if->data, adr);
		return MMU_aluMemAccessCycles<PROCNUM,16,MMU_AD_READ>(cycle,adr);
	}

	template<u32 PROCNUM, u32 memtype, u32 cycle>
	static u32 FASTCALL MEMOP_LDRSB(u32 adr, u32 *dstreg)
	{
		*dstreg = (s8)READ8(GETCPU.mem_if->data, adr);
		return MMU_aluMemAccessCycles<PROCNUM,8,MMU_AD_READ>(cycle,adr);
	}

	static const MemOp1 LDRH_Tab[2][MEMTYPE_COUNT] = 
	{
		{
			MEMOP_LDRH<0,MEMTYPE_GENERIC,3>,
			MEMOP_LDRH<0,MEMTYPE_MAIN,3>,
			MEMOP_LDRH<0,MEMTYPE_DTCM_ARM9,3>,
			MEMOP_LDRH<0,MEMTYPE_GENERIC,3>,//MEMOP_LDRH<0,MEMTYPE_ERAM_ARM7,3>,
			MEMOP_LDRH<0,MEMTYPE_SWIRAM,3>,
		},
		{
			MEMOP_LDRH<1,MEMTYPE_GENERIC,3>,
			MEMOP_LDRH<1,MEMTYPE_MAIN,3>,
			MEMOP_LDRH<1,MEMTYPE_GENERIC,3>,//MEMOP_LDRH<1,MEMTYPE_DTCM_ARM9,3>,
			MEMOP_LDRH<1,MEMTYPE_ERAM_ARM7,3>,
			MEMOP_LDRH<1,MEMTYPE_SWIRAM,3>,
		}
	};

	static const MemOp1 LDRSH_Tab[2][MEMTYPE_COUNT] = 
	{
		{
			MEMOP_LDRSH<0,MEMTYPE_GENERIC,3>,
			MEMOP_LDRSH<0,MEMTYPE_MAIN,3>,
			MEMOP_LDRSH<0,MEMTYPE_DTCM_ARM9,3>,
			MEMOP_LDRSH<0,MEMTYPE_GENERIC,3>,//MEMOP_LDRSH<0,MEMTYPE_ERAM_ARM7,3>,
			MEMOP_LDRSH<0,MEMTYPE_SWIRAM,3>,
		},
		{
			MEMOP_LDRSH<1,MEMTYPE_GENERIC,3>,
			MEMOP_LDRSH<1,MEMTYPE_MAIN,3>,
			MEMOP_LDRSH<1,MEMTYPE_GENERIC,3>,//MEMOP_LDRSH<1,MEMTYPE_DTCM_ARM9,3>,
			MEMOP_LDRSH<1,MEMTYPE_ERAM_ARM7,3>,
			MEMOP_LDRSH<1,MEMTYPE_SWIRAM,3>,
		}
	};

	static const MemOp1 LDRSB_Tab[2][MEMTYPE_COUNT] = 
	{
		{
			MEMOP_LDRSB<0,MEMTYPE_GENERIC,3>,
			MEMOP_LDRSB<0,MEMTYPE_MAIN,3>,
			MEMOP_LDRSB<0,MEMTYPE_DTCM_ARM9,3>,
			MEMOP_LDRSB<0,MEMTYPE_GENERIC,3>,//MEMOP_LDRSB<0,MEMTYPE_ERAM_ARM7,3>,
			MEMOP_LDRSB<0,MEMTYPE_SWIRAM,3>,
		},
		{
			MEMOP_LDRSB<1,MEMTYPE_GENERIC,3>,
			MEMOP_LDRSB<1,MEMTYPE_MAIN,3>,
			MEMOP_LDRSB<1,MEMTYPE_GENERIC,3>,//MEMOP_LDRSB<1,MEMTYPE_DTCM_ARM9,3>,
			MEMOP_LDRSB<1,MEMTYPE_ERAM_ARM7,3>,
			MEMOP_LDRSB<1,MEMTYPE_SWIRAM,3>,
		}
	};

	OPCDECODER_DECL(IR_LDRx)
	{
		u32 PROCNUM = d.ProcessID;
		u32 adr_guess = 0;

		if (d.P)
		{
			if (d.I)
			{
				WRITE_CODE("u32 adr = REG_R%s(%#p) %c %u;\n", REG_R(d.Rn), d.U?'+':'-', d.Immediate);

				adr_guess = READREG(d.Rn) + (d.Immediate * (d.U?1:-1));
			}
			else
			{
				WRITE_CODE("u32 adr = REG_R%s(%#p) %c REG_R%s(%#p);\n", REG_R(d.Rn), d.U?'+':'-', REG_R(d.Rm));
				adr_guess = READREG(d.Rn) + (READREG(d.Rm) * (d.U?1:-1));
			}

			if (d.W)
				WRITE_CODE("REG_W(%#p) = adr;\n", REG_W(d.Rn));
		}
		else
		{
			WRITE_CODE("u32 adr = REG_R%s(%#p);\n", REG_R(d.Rn));

			adr_guess = READREG(d.Rn);

			if (d.I)
				WRITE_CODE("REG_W(%#p) = adr %c %u;\n", REG_W(d.Rn), d.U?'+':'-', d.Immediate);
			else
				WRITE_CODE("REG_W(%#p) = adr %c REG_R%s(%#p);\n", REG_W(d.Rn), d.U?'+':'-', REG_R(d.Rm));
		}

		if (d.H)
		{
			if (d.S)
				WRITE_CODE("ExecuteCycles+=((u32 (FASTCALL *)(u32, u32*))%#p)(adr,REGPTR(%#p));\n", LDRSH_Tab[PROCNUM][GuessAddressArea(PROCNUM,adr_guess)], REGPTR(d.Rd));
			else
				WRITE_CODE("ExecuteCycles+=((u32 (FASTCALL *)(u32, u32*))%#p)(adr,REGPTR(%#p));\n", LDRH_Tab[PROCNUM][GuessAddressArea(PROCNUM,adr_guess)], REGPTR(d.Rd));
		}
		else
			WRITE_CODE("ExecuteCycles+=((u32 (FASTCALL *)(u32, u32*))%#p)(adr,REGPTR(%#p));\n", LDRSB_Tab[PROCNUM][GuessAddressArea(PROCNUM,adr_guess)], REGPTR(d.Rd));
	}

	template<u32 PROCNUM, u32 memtype, u32 cycle>
	static u32 FASTCALL MEMOP_STRH(u32 adr, u32 data)
	{
		WRITE16(GETCPU.mem_if->data, adr, data);
		return MMU_aluMemAccessCycles<PROCNUM,16,MMU_AD_WRITE>(cycle,adr);
	}

	static const MemOp2 STRH_Tab[2][MEMTYPE_COUNT] = 
	{
		{
			MEMOP_STRH<0,MEMTYPE_GENERIC,2>,
			MEMOP_STRH<0,MEMTYPE_MAIN,2>,
			MEMOP_STRH<0,MEMTYPE_DTCM_ARM9,2>,
			MEMOP_STRH<0,MEMTYPE_GENERIC,2>,//MEMOP_STRH<0,MEMTYPE_ERAM_ARM7,2>,
			MEMOP_STRH<0,MEMTYPE_SWIRAM,2>,
		},
		{
			MEMOP_STRH<1,MEMTYPE_GENERIC,2>,
			MEMOP_STRH<1,MEMTYPE_MAIN,2>,
			MEMOP_STRH<1,MEMTYPE_GENERIC,2>,//MEMOP_STRH<1,MEMTYPE_DTCM_ARM9,2>,
			MEMOP_STRH<1,MEMTYPE_ERAM_ARM7,2>,
			MEMOP_STRH<1,MEMTYPE_SWIRAM,2>,
		}
	};

	OPCDECODER_DECL(IR_STRx)
	{
		u32 PROCNUM = d.ProcessID;
		u32 adr_guess = 0;

		if (d.P)
		{
			if (d.I)
			{
				WRITE_CODE("u32 adr = REG_R%s(%#p) %c %u;\n", REG_R(d.Rn), d.U?'+':'-', d.Immediate);

				adr_guess = READREG(d.Rn) + (d.Immediate * (d.U?1:-1));
			}
			else
			{
				WRITE_CODE("u32 adr = REG_R%s(%#p) %c REG_R%s(%#p);\n", REG_R(d.Rn), d.U?'+':'-', REG_R(d.Rm));

				adr_guess = READREG(d.Rn) + (READREG(d.Rm) * (d.U?1:-1));
			}

			if (d.W)
				WRITE_CODE("REG_W(%#p) = adr;\n", REG_W(d.Rn));
		}
		else
		{
			WRITE_CODE("u32 adr = REG_R%s(%#p);\n", REG_R(d.Rn));

			adr_guess = READREG(d.Rn);
		}

		WRITE_CODE("ExecuteCycles+=((u32 (FASTCALL *)(u32, u32))%#p)(adr,REG_R%s(%#p));\n", STRH_Tab[PROCNUM][GuessAddressArea(PROCNUM,adr_guess)], REG_R(d.Rd));

		if (!d.P)
		{
			if (d.I)
				WRITE_CODE("REG_W(%#p) = adr %c %u;\n", REG_W(d.Rn), d.U?'+':'-', d.Immediate);
			else
				WRITE_CODE("REG_W(%#p) = adr %c REG_R%s(%#p);\n", REG_W(d.Rn), d.U?'+':'-', REG_R(d.Rm));
		}
	}

	template<u32 PROCNUM, u32 memtype, u32 cycle>
	static u32 FASTCALL MEMOP_LDRD(u32 adr, u32 *dstreg)
	{
		*dstreg = READ32(GETCPU.mem_if->data, adr);
		*(dstreg+1) = READ32(GETCPU.mem_if->data, adr+4);
		return MMU_aluMemCycles<PROCNUM>(cycle, MMU_memAccessCycles<PROCNUM,32,MMU_AD_READ>(adr) + MMU_memAccessCycles<PROCNUM,32,MMU_AD_READ>(adr+4));
	}

	static const MemOp1 LDRD_Tab[2][MEMTYPE_COUNT] = 
	{
		{
			MEMOP_LDRD<0,MEMTYPE_GENERIC,3>,
			MEMOP_LDRD<0,MEMTYPE_MAIN,3>,
			MEMOP_LDRD<0,MEMTYPE_DTCM_ARM9,3>,
			MEMOP_LDRD<0,MEMTYPE_GENERIC,3>,//MEMOP_LDRD<0,MEMTYPE_ERAM_ARM7,3>,
			MEMOP_LDRD<0,MEMTYPE_SWIRAM,3>,
		},
		{
			MEMOP_LDRD<1,MEMTYPE_GENERIC,3>,
			MEMOP_LDRD<1,MEMTYPE_MAIN,3>,
			MEMOP_LDRD<1,MEMTYPE_GENERIC,3>,//MEMOP_LDRD<1,MEMTYPE_DTCM_ARM9,3>,
			MEMOP_LDRD<1,MEMTYPE_ERAM_ARM7,3>,
			MEMOP_LDRD<1,MEMTYPE_SWIRAM,3>,
		}
	};

	OPCDECODER_DECL(IR_LDRD)
	{
		u32 PROCNUM = d.ProcessID;
		u32 adr_guess = 0;

		if (d.P)
		{
			if (d.I)
			{
				WRITE_CODE("u32 adr = REG_R%s(%#p) %c %u;\n", REG_R(d.Rn), d.U?'+':'-', d.Immediate);

				adr_guess = READREG(d.Rn) + (d.Immediate * (d.U?1:-1));
			}
			else
			{
				WRITE_CODE("u32 adr = REG_R%s(%#p) %c REG_R%s(%#p);\n", REG_R(d.Rn), d.U?'+':'-', REG_R(d.Rm));

				adr_guess = READREG(d.Rn) + (READREG(d.Rm) * (d.U?1:-1));
			}

			if (d.W)
				WRITE_CODE("REG_W(%#p) = adr;\n", REG_W(d.Rn));
		}
		else
		{
			WRITE_CODE("u32 adr = REG_R%s(%#p);\n", REG_R(d.Rn));

			adr_guess = READREG(d.Rn);

			if (d.I)
				WRITE_CODE("REG_W(%#p) = adr %c %u;\n", REG_W(d.Rn), d.Immediate, d.U?'+':'-');
			else
				WRITE_CODE("REG_W(%#p) = adr %c REG_R%s(%#p);\n", REG_W(d.Rn), d.U?'+':'-', REG_R(d.Rm));
		}

		WRITE_CODE("ExecuteCycles+=((u32 (FASTCALL *)(u32, u32*))%#p)(adr,REGPTR(%#p));\n", LDRD_Tab[PROCNUM][GuessAddressArea(PROCNUM,adr_guess)], REGPTR(d.Rd));
	}

	template<u32 PROCNUM, u32 memtype, u32 cycle>
	static u32 FASTCALL MEMOP_STRD(u32 adr, u32 *srcreg)
	{
		WRITE32(GETCPU.mem_if->data, adr, *srcreg);
		WRITE32(GETCPU.mem_if->data, adr+4, *(srcreg+1));
		return MMU_aluMemCycles<PROCNUM>(cycle, MMU_memAccessCycles<PROCNUM,32,MMU_AD_WRITE>(adr) + MMU_memAccessCycles<PROCNUM,32,MMU_AD_WRITE>(adr+4));
	}

	static const MemOp1 STRD_Tab[2][MEMTYPE_COUNT] = 
	{
		{
			MEMOP_STRD<0,MEMTYPE_GENERIC,3>,
			MEMOP_STRD<0,MEMTYPE_MAIN,3>,
			MEMOP_STRD<0,MEMTYPE_DTCM_ARM9,3>,
			MEMOP_STRD<0,MEMTYPE_GENERIC,3>,//MEMOP_STRD<0,MEMTYPE_ERAM_ARM7,3>,
			MEMOP_STRD<0,MEMTYPE_SWIRAM,3>,
		},
		{
			MEMOP_STRD<1,MEMTYPE_GENERIC,3>,
			MEMOP_STRD<1,MEMTYPE_MAIN,3>,
			MEMOP_STRD<1,MEMTYPE_GENERIC,3>,//MEMOP_STRD<1,MEMTYPE_DTCM_ARM9,3>,
			MEMOP_STRD<1,MEMTYPE_ERAM_ARM7,3>,
			MEMOP_STRD<1,MEMTYPE_SWIRAM,3>,
		}
	};

	OPCDECODER_DECL(IR_STRD)
	{
		u32 PROCNUM = d.ProcessID;
		u32 adr_guess = 0;

		if (d.P)
		{
			if (d.I)
			{
				WRITE_CODE("u32 adr = REG_R%s(%#p) %c %u;\n", REG_R(d.Rn), d.U?'+':'-', d.Immediate);

				adr_guess = READREG(d.Rn) + (d.Immediate * (d.U?1:-1));
			}
			else
			{
				WRITE_CODE("u32 adr = REG_R%s(%#p) %c REG_R%s(%#p);\n", REG_R(d.Rn), d.U?'+':'-', REG_R(d.Rm));

				adr_guess = READREG(d.Rn) + (READREG(d.Rm) * (d.U?1:-1));
			}

			if (d.W)
				WRITE_CODE("REG_W(%#p) = adr;\n", REG_W(d.Rn));
		}
		else
		{
			WRITE_CODE("u32 adr = REG_R%s(%#p);\n", REG_R(d.Rn));

			adr_guess = READREG(d.Rn);

			if (d.I)
				WRITE_CODE("REG_W(%#p) = adr %c %u;\n", REG_W(d.Rn), d.Immediate, d.U?'+':'-');
			else
				WRITE_CODE("REG_W(%#p) = adr %c REG_R%s(%#p);\n", REG_W(d.Rn), d.U?'+':'-', REG_R(d.Rm));
		}

		if (d.Rd == 14)
			WRITE_CODE("REG_W(%#p) = %u;\n", REG_W(15), d.CalcR15(d));

		WRITE_CODE("ExecuteCycles+=((u32 (FASTCALL *)(u32, u32*))%#p)(adr,REGPTR(%#p));\n", STRD_Tab[PROCNUM][GuessAddressArea(PROCNUM,adr_guess)], REGPTR(d.Rd));
	}

	OPCDECODER_DECL(IR_LDREX)
	{
		u32 PROCNUM = d.ProcessID;

		WRITE_CODE("u32 adr = REG_R%s(%#p);\n", REG_R(d.Rn));

		u32 adr_guess = READREG(d.Rn);

		WRITE_CODE("ExecuteCycles+=((u32 (FASTCALL *)(u32, u32*))%#p)(adr,REGPTR(%#p));\n", LDR_Tab[PROCNUM][GuessAddressArea(PROCNUM,adr_guess)], REGPTR(d.Rd));
	}

	OPCDECODER_DECL(IR_STREX)
	{
		u32 PROCNUM = d.ProcessID;

		WRITE_CODE("u32 adr = REG_R%s(%#p);\n", REG_R(d.Rn));

		u32 adr_guess = READREG(d.Rn);

		WRITE_CODE("ExecuteCycles+=((u32 (FASTCALL *)(u32, u32))%#p)(adr,REG_R%s(%#p));\n", STR_Tab[PROCNUM][GuessAddressArea(PROCNUM,adr_guess)], REG_R(d.Rd));

		WRITE_CODE("REG_W(%#p) = 0;\n", REG_W(d.Rm));
	}

	template<u32 PROCNUM, u32 memtype, u32 cycle, bool up>
	static u32 MEMOP_LDM_SEQUENCE(u32 adr, u32 count, u32 *regs)
	{
		u32 c = 0;
		u8 *ptr = _MMU_read_getrawptr32<PROCNUM, MMU_AT_DATA>(adr, adr+(count-1)*4);
		if (ptr)
		{
#ifdef WORDS_BIGENDIAN
			if (up)
			{
				for (u32 i = 0; i < count; i++)
				{
					regs[i] = T1ReadLong_guaranteedAligned(ptr, i * sizeof(u32));
					c += MMU_memAccessCycles<PROCNUM,32,MMU_AD_READ>(adr);
					adr += 4;
				}
			}
			else
			{
				adr = adr+(count-1)*4;
				for (s32 i = (s32)count - 1; i >= 0; i--)
				{
					regs[i] = T1ReadLong_guaranteedAligned(ptr, i * sizeof(u32));
					c += MMU_memAccessCycles<PROCNUM,32,MMU_AD_READ>(adr);
					adr -= 4;
				}
			}
#else
			memcpy(regs, ptr, sizeof(u32) * count);
			if (up)
			{
				for (u32 i = 0; i < count; i++)
				{
					c += MMU_memAccessCycles<PROCNUM,32,MMU_AD_READ>(adr);
					adr += 4;
				}
			}
			else
			{
				adr = adr+(count-1)*4;
				for (s32 i = (s32)count - 1; i >= 0; i--)
				{
					c += MMU_memAccessCycles<PROCNUM,32,MMU_AD_READ>(adr);
					adr -= 4;
				}
			}
#endif
		}
		else
		{
			if (up)
			{
				for (u32 i = 0; i < count; i++)
				{
					regs[i] = READ32(GETCPU.mem_if->data, adr);
					c += MMU_memAccessCycles<PROCNUM,32,MMU_AD_READ>(adr);
					adr += 4;
				}
			}
			else
			{
				adr = adr+(count-1)*4;
				for (s32 i = (s32)count - 1; i >= 0; i--)
				{
					regs[i] = READ32(GETCPU.mem_if->data, adr);
					c += MMU_memAccessCycles<PROCNUM,32,MMU_AD_READ>(adr);
					adr -= 4;
				}
			}
		}

		return MMU_aluMemCycles<PROCNUM>(cycle, c);
	}

	template<u32 PROCNUM, u32 memtype, u32 cycle, bool up>
	static u32 MEMOP_LDM(u32 adr, u32 count, u32 *regs_ptr)
	{
		u32 c = 0;
		u32 **regs = (u32 **)regs_ptr;
		u8 *ptr = _MMU_read_getrawptr32<PROCNUM, MMU_AT_DATA>(adr, adr+(count-1)*4);
		if (ptr)
		{
			if (up)
			{
				for (u32 i = 0; i < count; i++)
				{
					*(regs[i]) = T1ReadLong_guaranteedAligned(ptr, i * sizeof(u32));
					c += MMU_memAccessCycles<PROCNUM,32,MMU_AD_READ>(adr);
					adr += 4;
				}
			}
			else
			{
				adr = adr+(count-1)*4;
				for (s32 i = (s32)count - 1; i >= 0; i--)
				{
					*(regs[i]) = T1ReadLong_guaranteedAligned(ptr, i * sizeof(u32));
					c += MMU_memAccessCycles<PROCNUM,32,MMU_AD_READ>(adr);
					adr -= 4;
				}
			}
		}
		else
		{
			if (up)
			{
				for (u32 i = 0; i < count; i++)
				{
					*(regs[i]) = READ32(GETCPU.mem_if->data, adr);
					c += MMU_memAccessCycles<PROCNUM,32,MMU_AD_READ>(adr);
					adr += 4;
				}
			}
			else
			{
				adr = adr+(count-1)*4;
				for (s32 i = (s32)count - 1; i >= 0; i--)
				{
					*(regs[i]) = READ32(GETCPU.mem_if->data, adr);
					c += MMU_memAccessCycles<PROCNUM,32,MMU_AD_READ>(adr);
					adr -= 4;
				}
			}
		}

		return MMU_aluMemCycles<PROCNUM>(cycle, c);
	}

	static const MemOp3 LDM_SEQUENCE_Up_Tab[2][MEMTYPE_COUNT] = 
	{
		{
			MEMOP_LDM_SEQUENCE<0,MEMTYPE_GENERIC,2,true>,
			MEMOP_LDM_SEQUENCE<0,MEMTYPE_MAIN,2,true>,
			MEMOP_LDM_SEQUENCE<0,MEMTYPE_DTCM_ARM9,2,true>,
			MEMOP_LDM_SEQUENCE<0,MEMTYPE_GENERIC,2,true>,//MEMOP_LDM_SEQUENCE<0,MEMTYPE_ERAM_ARM7,2,true>,
			MEMOP_LDM_SEQUENCE<0,MEMTYPE_SWIRAM,2,true>,
		},
		{
			MEMOP_LDM_SEQUENCE<1,MEMTYPE_GENERIC,2,true>,
			MEMOP_LDM_SEQUENCE<1,MEMTYPE_MAIN,2,true>,
			MEMOP_LDM_SEQUENCE<1,MEMTYPE_GENERIC,2,true>,//MEMOP_LDM_SEQUENCE<1,MEMTYPE_DTCM_ARM9,2,true>,
			MEMOP_LDM_SEQUENCE<1,MEMTYPE_ERAM_ARM7,2,true>,
			MEMOP_LDM_SEQUENCE<1,MEMTYPE_SWIRAM,2,true>,
		}
	};

	static const MemOp3 LDM_Up_Tab[2][MEMTYPE_COUNT] = 
	{
		{
			MEMOP_LDM<0,MEMTYPE_GENERIC,2,true>,
			MEMOP_LDM<0,MEMTYPE_MAIN,2,true>,
			MEMOP_LDM<0,MEMTYPE_DTCM_ARM9,2,true>,
			MEMOP_LDM<0,MEMTYPE_GENERIC,2,true>,//MEMOP_LDM<0,MEMTYPE_ERAM_ARM7,2,true>,
			MEMOP_LDM<0,MEMTYPE_SWIRAM,2,true>,
		},
		{
			MEMOP_LDM<1,MEMTYPE_GENERIC,2,true>,
			MEMOP_LDM<1,MEMTYPE_MAIN,2,true>,
			MEMOP_LDM<1,MEMTYPE_GENERIC,2,true>,//MEMOP_LDM<1,MEMTYPE_DTCM_ARM9,2,true>,
			MEMOP_LDM<1,MEMTYPE_ERAM_ARM7,2,true>,
			MEMOP_LDM<1,MEMTYPE_SWIRAM,2,true>,
		}
	};

	static const MemOp3 LDM_SEQUENCE_Up_R15_Tab[2][MEMTYPE_COUNT] = 
	{
		{
			MEMOP_LDM_SEQUENCE<0,MEMTYPE_GENERIC,4,true>,
			MEMOP_LDM_SEQUENCE<0,MEMTYPE_MAIN,4,true>,
			MEMOP_LDM_SEQUENCE<0,MEMTYPE_DTCM_ARM9,4,true>,
			MEMOP_LDM_SEQUENCE<0,MEMTYPE_GENERIC,4,true>,//MEMOP_LDM_SEQUENCE<0,MEMTYPE_ERAM_ARM7,4,true>,
			MEMOP_LDM_SEQUENCE<0,MEMTYPE_SWIRAM,4,true>,
		},
		{
			MEMOP_LDM_SEQUENCE<1,MEMTYPE_GENERIC,4,true>,
			MEMOP_LDM_SEQUENCE<1,MEMTYPE_MAIN,4,true>,
			MEMOP_LDM_SEQUENCE<1,MEMTYPE_GENERIC,4,true>,//MEMOP_LDM_SEQUENCE<1,MEMTYPE_DTCM_ARM9,4,true>,
			MEMOP_LDM_SEQUENCE<1,MEMTYPE_ERAM_ARM7,4,true>,
			MEMOP_LDM_SEQUENCE<1,MEMTYPE_SWIRAM,4,true>,
		}
	};

	static const MemOp3 LDM_Up_R15_Tab[2][MEMTYPE_COUNT] = 
	{
		{
			MEMOP_LDM<0,MEMTYPE_GENERIC,4,true>,
			MEMOP_LDM<0,MEMTYPE_MAIN,4,true>,
			MEMOP_LDM<0,MEMTYPE_DTCM_ARM9,4,true>,
			MEMOP_LDM<0,MEMTYPE_GENERIC,4,true>,//MEMOP_LDM<0,MEMTYPE_ERAM_ARM7,4,true>,
			MEMOP_LDM<0,MEMTYPE_SWIRAM,4,true>,
		},
		{
			MEMOP_LDM<1,MEMTYPE_GENERIC,4,true>,
			MEMOP_LDM<1,MEMTYPE_MAIN,4,true>,
			MEMOP_LDM<1,MEMTYPE_GENERIC,4,true>,//MEMOP_LDM<1,MEMTYPE_DTCM_ARM9,4,true>,
			MEMOP_LDM<1,MEMTYPE_ERAM_ARM7,4,true>,
			MEMOP_LDM<1,MEMTYPE_SWIRAM,4,true>,
		}
	};

	static const MemOp3 LDM_SEQUENCE_Down_Tab[2][MEMTYPE_COUNT] = 
	{
		{
			MEMOP_LDM_SEQUENCE<0,MEMTYPE_GENERIC,2,false>,
			MEMOP_LDM_SEQUENCE<0,MEMTYPE_MAIN,2,false>,
			MEMOP_LDM_SEQUENCE<0,MEMTYPE_DTCM_ARM9,2,false>,
			MEMOP_LDM_SEQUENCE<0,MEMTYPE_GENERIC,2,false>,//MEMOP_LDM_SEQUENCE<0,MEMTYPE_ERAM_ARM7,2,false>,
			MEMOP_LDM_SEQUENCE<0,MEMTYPE_SWIRAM,2,false>,
		},
		{
			MEMOP_LDM_SEQUENCE<1,MEMTYPE_GENERIC,2,false>,
			MEMOP_LDM_SEQUENCE<1,MEMTYPE_MAIN,2,false>,
			MEMOP_LDM_SEQUENCE<1,MEMTYPE_GENERIC,2,false>,//MEMOP_LDM_SEQUENCE<1,MEMTYPE_DTCM_ARM9,2,false>,
			MEMOP_LDM_SEQUENCE<1,MEMTYPE_ERAM_ARM7,2,false>,
			MEMOP_LDM_SEQUENCE<1,MEMTYPE_SWIRAM,2,false>,
		}
	};

	static const MemOp3 LDM_Down_Tab[2][MEMTYPE_COUNT] = 
	{
		{
			MEMOP_LDM<0,MEMTYPE_GENERIC,2,false>,
			MEMOP_LDM<0,MEMTYPE_MAIN,2,false>,
			MEMOP_LDM<0,MEMTYPE_DTCM_ARM9,2,false>,
			MEMOP_LDM<0,MEMTYPE_GENERIC,2,false>,//MEMOP_LDM<0,MEMTYPE_ERAM_ARM7,2,false>,
			MEMOP_LDM<0,MEMTYPE_SWIRAM,2,false>,
		},
		{
			MEMOP_LDM<1,MEMTYPE_GENERIC,2,false>,
			MEMOP_LDM<1,MEMTYPE_MAIN,2,false>,
			MEMOP_LDM<1,MEMTYPE_GENERIC,2,false>,//MEMOP_LDM<1,MEMTYPE_DTCM_ARM9,2,false>,
			MEMOP_LDM<1,MEMTYPE_ERAM_ARM7,2,false>,
			MEMOP_LDM<1,MEMTYPE_SWIRAM,2,false>,
		}
	};

	static const MemOp3 LDM_SEQUENCE_Down_R15_Tab[2][MEMTYPE_COUNT] = 
	{
		{
			MEMOP_LDM_SEQUENCE<0,MEMTYPE_GENERIC,4,false>,
			MEMOP_LDM_SEQUENCE<0,MEMTYPE_MAIN,4,false>,
			MEMOP_LDM_SEQUENCE<0,MEMTYPE_DTCM_ARM9,4,false>,
			MEMOP_LDM_SEQUENCE<0,MEMTYPE_GENERIC,4,false>,//MEMOP_LDM_SEQUENCE<0,MEMTYPE_ERAM_ARM7,4,false>,
			MEMOP_LDM_SEQUENCE<0,MEMTYPE_SWIRAM,4,false>,
		},
		{
			MEMOP_LDM_SEQUENCE<1,MEMTYPE_GENERIC,4,false>,
			MEMOP_LDM_SEQUENCE<1,MEMTYPE_MAIN,4,false>,
			MEMOP_LDM_SEQUENCE<1,MEMTYPE_GENERIC,4,false>,//MEMOP_LDM_SEQUENCE<1,MEMTYPE_DTCM_ARM9,4,false>,
			MEMOP_LDM_SEQUENCE<1,MEMTYPE_ERAM_ARM7,4,false>,
			MEMOP_LDM_SEQUENCE<1,MEMTYPE_SWIRAM,4,false>,
		}
	};

	static const MemOp3 LDM_Down_R15_Tab[2][MEMTYPE_COUNT] = 
	{
		{
			MEMOP_LDM<0,MEMTYPE_GENERIC,4,false>,
			MEMOP_LDM<0,MEMTYPE_MAIN,4,false>,
			MEMOP_LDM<0,MEMTYPE_DTCM_ARM9,4,false>,
			MEMOP_LDM<0,MEMTYPE_GENERIC,4,false>,//MEMOP_LDM<0,MEMTYPE_ERAM_ARM7,4,false>,
			MEMOP_LDM<0,MEMTYPE_SWIRAM,4,false>,
		},
		{
			MEMOP_LDM<1,MEMTYPE_GENERIC,4,false>,
			MEMOP_LDM<1,MEMTYPE_MAIN,4,false>,
			MEMOP_LDM<1,MEMTYPE_GENERIC,4,false>,//MEMOP_LDM<1,MEMTYPE_DTCM_ARM9,4,false>,
			MEMOP_LDM<1,MEMTYPE_ERAM_ARM7,4,false>,
			MEMOP_LDM<1,MEMTYPE_SWIRAM,4,false>,
		}
	};

	OPCDECODER_DECL(IR_LDM)
	{
		u32 PROCNUM = d.ProcessID;

		u32 SequenceFlag = 0;//0:no sequence start,1:one sequence start,2:one sequence end,3:more than one sequence start
		u32 Count = 0;
		u32* Regs[16];
		for(u32 RegisterList = d.RegisterList, n = 0; RegisterList; RegisterList >>= 1, n++)
		{
			if (RegisterList & 0x1)
			{
				Regs[Count] = &GETCPU.R[n];
				Count++;

				if (SequenceFlag == 0)
					SequenceFlag = 1;
				else if (SequenceFlag == 2)
					SequenceFlag = 3;
			}
			else
			{
				if (SequenceFlag == 1)
					SequenceFlag = 2;
			}
		}

		bool NeedWriteBack = false;
		if (d.W)
		{
			if (d.RegisterList & (1 << d.Rn))
			{
				u32 bitList = (~((2 << d.Rn)-1)) & 0xFFFF;
				if (/*!d.S && */(d.RegisterList & bitList))
					NeedWriteBack = true;
			}
			else
				NeedWriteBack = true;
		}

		bool IsOneSequence = (SequenceFlag == 1 || SequenceFlag == 2);

		if (NeedWriteBack)
			WRITE_CODE("u32 adr_old = REG_R%s(%#p);\n", REG_R(d.Rn));

		if (d.P)
			WRITE_CODE("u32 adr = (REG_R%s(%#p) %c 4) & 0xFFFFFFFC;\n", REG_R(d.Rn), d.U?'+':'-');
		else
			WRITE_CODE("u32 adr = REG_R%s(%#p) & 0xFFFFFFFC;\n", REG_R(d.Rn));

		if (d.S)
		{
			if (d.R15Modified)
			{
				//WRITE_CODE("((Status_Reg*)%#p)->val=((Status_Reg*)%#p)->val;\n", &(GETCPU.CPSR), &(GETCPU.SPSR));
			}
			else
				WRITE_CODE("u32 oldmode = ((u32 (*)(void*,u8))%#p)((void*)%#p,%u);\n", armcpu_switchMode, GETCPUPTR, SYS);
		}

		if (IsOneSequence)
		{
			if (d.U)
			{
				if (d.R15Modified)
					WRITE_CODE("ExecuteCycles+=((u32 (*)(u32, u32, u32*))%#p)(adr, %u,(u32*)%#p);\n", LDM_SEQUENCE_Up_R15_Tab[PROCNUM][0], Count, Regs[0]);
				else
					WRITE_CODE("ExecuteCycles+=((u32 (*)(u32, u32, u32*))%#p)(adr, %u,(u32*)%#p);\n", LDM_SEQUENCE_Up_Tab[PROCNUM][0], Count, Regs[0]);
			}
			else
			{
				if (d.R15Modified)
					WRITE_CODE("ExecuteCycles+=((u32 (*)(u32, u32, u32*))%#p)(adr, %u,(u32*)%#p);\n", LDM_SEQUENCE_Down_R15_Tab[PROCNUM][0], Count, Regs[0]);
				else
					WRITE_CODE("ExecuteCycles+=((u32 (*)(u32, u32, u32*))%#p)(adr, %u,(u32*)%#p);\n", LDM_SEQUENCE_Down_Tab[PROCNUM][0], Count, Regs[0]);
			}
		}
		else
		{
			WRITE_CODE("static const u32* Regs[]={");
			for (u32 i = 0; i < Count; i++)
			{
				WRITE_CODE("(u32*)%#p", Regs[i]);
				if (i != Count - 1)
					WRITE_CODE(",");
			}
			WRITE_CODE("};\n");

			if (d.U)
			{
				if (d.R15Modified)
					WRITE_CODE("ExecuteCycles+=((u32 (*)(u32, u32, u32*))%#p)(adr, %u,(u32*)&Regs[0]);\n", LDM_Up_R15_Tab[PROCNUM][0], Count);
				else
					WRITE_CODE("ExecuteCycles+=((u32 (*)(u32, u32, u32*))%#p)(adr, %u,(u32*)&Regs[0]);\n", LDM_Up_Tab[PROCNUM][0], Count);
			}
			else
			{
				if (d.R15Modified)
					WRITE_CODE("ExecuteCycles+=((u32 (*)(u32, u32, u32*))%#p)(adr, %u,(u32*)&Regs[0]);\n", LDM_Down_R15_Tab[PROCNUM][0], Count);
				else
					WRITE_CODE("ExecuteCycles+=((u32 (*)(u32, u32, u32*))%#p)(adr, %u,(u32*)&Regs[0]);\n", LDM_Down_Tab[PROCNUM][0], Count);
			}
		}

		if (d.S)
		{
			if (NeedWriteBack)
				WRITE_CODE("REG_W(%#p)=adr_old %c %u;\n", REG_W(d.Rn), d.U?'+':'-', Count*4);

			if (d.R15Modified)
			{
				LDM_S_LoadCPSRGenerate(d, szCodeBuffer);

				R15ModifiedGenerate(d, szCodeBuffer);
			}
			else
				WRITE_CODE("((u32 (*)(void*,u8))%#p)((void*)%#p,oldmode);\n", armcpu_switchMode, GETCPUPTR);
		}
		else
		{
			if (d.R15Modified)
			{
				if (PROCNUM == 0)
				{
					WRITE_CODE("((Status_Reg*)%#p)->bits.T=BIT0(REG(%#p));\n", &(GETCPU.CPSR), REG(15));
					WRITE_CODE("REG(%#p)&=0xFFFFFFFE;\n", REG(15));
				}
				else
					WRITE_CODE("REG(%#p)&=0xFFFFFFFC;\n", REG(15));
			}

			if (NeedWriteBack)
				WRITE_CODE("REG_W(%#p)=adr_old %c %u;\n", REG_W(d.Rn), d.U?'+':'-', Count*4);

			if (d.R15Modified)
				R15ModifiedGenerate(d, szCodeBuffer);
		}
	}

	template<u32 PROCNUM, u32 memtype, u32 cycle, bool up>
	static u32 MEMOP_STM_SEQUENCE(u32 adr, u32 count, u32 *regs)
	{
		u32 c = 0;
		u8 *ptr = _MMU_write_getrawptr32<PROCNUM, MMU_AT_DATA>(adr, adr+(count-1)*4);
		if (ptr)
		{
#ifdef WORDS_BIGENDIAN
			if (up)
			{
				for (u32 i = 0; i < count; i++)
				{
					T1WriteLong(ptr, i * sizeof(u32), regs[i]);
					c += MMU_memAccessCycles<PROCNUM,32,MMU_AD_WRITE>(adr);
					adr += 4;
				}
			}
			else
			{
				adr = adr+(count-1)*4;
				for (s32 i = (s32)count - 1; i >= 0; i--)
				{
					T1WriteLong(ptr, i * sizeof(u32), regs[i]);
					c += MMU_memAccessCycles<PROCNUM,32,MMU_AD_WRITE>(adr);
					adr -= 4;
				}
			}
#else
			memcpy(ptr, regs, sizeof(u32) * count);
			if (up)
			{
				for (u32 i = 0; i < count; i++)
				{
					c += MMU_memAccessCycles<PROCNUM,32,MMU_AD_WRITE>(adr);
					adr += 4;
				}
			}
			else
			{
				adr = adr+(count-1)*4;
				for (s32 i = (s32)count - 1; i >= 0; i--)
				{
					c += MMU_memAccessCycles<PROCNUM,32,MMU_AD_WRITE>(adr);
					adr -= 4;
				}
			}
#endif
		}
		else
		{
			if (up)
			{
				for (u32 i = 0; i < count; i++)
				{
					WRITE32(GETCPU.mem_if->data, adr, regs[i]);
					c += MMU_memAccessCycles<PROCNUM,32,MMU_AD_WRITE>(adr);
					adr += 4;
				}
			}
			else
			{
				adr = adr+(count-1)*4;
				for (s32 i = (s32)count - 1; i >= 0; i--)
				{
					WRITE32(GETCPU.mem_if->data, adr, regs[i]);
					c += MMU_memAccessCycles<PROCNUM,32,MMU_AD_WRITE>(adr);
					adr -= 4;
				}
			}
		}

		return MMU_aluMemCycles<PROCNUM>(cycle, c);
	}

	template<u32 PROCNUM, u32 memtype, u32 cycle, bool up>
	static u32 MEMOP_STM(u32 adr, u32 count, u32 *regs_ptr)
	{
		u32 c = 0;
		u32 **regs = (u32 **)regs_ptr;
		u8 *ptr = _MMU_write_getrawptr32<PROCNUM, MMU_AT_DATA>(adr, adr+(count-1)*4);
		if (ptr)
		{
			if (up)
			{
				for (u32 i = 0; i < count; i++)
				{
					T1WriteLong(ptr, i * sizeof(u32), *(regs[i]));
					c += MMU_memAccessCycles<PROCNUM,32,MMU_AD_WRITE>(adr);
					adr += 4;
				}
			}
			else
			{
				adr = adr+(count-1)*4;
				for (s32 i = (s32)count - 1; i >= 0; i--)
				{
					T1WriteLong(ptr, i * sizeof(u32), *(regs[i]));
					c += MMU_memAccessCycles<PROCNUM,32,MMU_AD_WRITE>(adr);
					adr -= 4;
				}
			}
		}
		else
		{
			if (up)
			{
				for (u32 i = 0; i < count; i++)
				{
					WRITE32(GETCPU.mem_if->data, adr, *(regs[i]));
					c += MMU_memAccessCycles<PROCNUM,32,MMU_AD_WRITE>(adr);
					adr += 4;
				}
			}
			else
			{
				adr = adr+(count-1)*4;
				for (s32 i = (s32)count - 1; i >= 0; i--)
				{
					WRITE32(GETCPU.mem_if->data, adr, *(regs[i]));
					c += MMU_memAccessCycles<PROCNUM,32,MMU_AD_WRITE>(adr);
					adr -= 4;
				}
			}
		}

		return MMU_aluMemCycles<PROCNUM>(cycle, c);
	}

	static const MemOp3 STM_SEQUENCE_Up_Tab[2][MEMTYPE_COUNT] = 
	{
		{
			MEMOP_STM_SEQUENCE<0,MEMTYPE_GENERIC,1,true>,
			MEMOP_STM_SEQUENCE<0,MEMTYPE_MAIN,1,true>,
			MEMOP_STM_SEQUENCE<0,MEMTYPE_DTCM_ARM9,1,true>,
			MEMOP_STM_SEQUENCE<0,MEMTYPE_GENERIC,1,true>,//MEMOP_STM_SEQUENCE<0,MEMTYPE_ERAM_ARM7,1,true>,
			MEMOP_STM_SEQUENCE<0,MEMTYPE_SWIRAM,1,true>,
		},
		{
			MEMOP_STM_SEQUENCE<1,MEMTYPE_GENERIC,1,true>,
			MEMOP_STM_SEQUENCE<1,MEMTYPE_MAIN,1,true>,
			MEMOP_STM_SEQUENCE<1,MEMTYPE_GENERIC,1,true>,//MEMOP_STM_SEQUENCE<1,MEMTYPE_DTCM_ARM9,1,true>,
			MEMOP_STM_SEQUENCE<1,MEMTYPE_ERAM_ARM7,1,true>,
			MEMOP_STM_SEQUENCE<1,MEMTYPE_SWIRAM,1,true>,
		}
	};

	static const MemOp3 STM_Up_Tab[2][MEMTYPE_COUNT] = 
	{
		{
			MEMOP_STM<0,MEMTYPE_GENERIC,1,true>,
			MEMOP_STM<0,MEMTYPE_MAIN,1,true>,
			MEMOP_STM<0,MEMTYPE_DTCM_ARM9,1,true>,
			MEMOP_STM<0,MEMTYPE_GENERIC,1,true>,//MEMOP_STM<0,MEMTYPE_ERAM_ARM7,1,true>,
			MEMOP_STM<0,MEMTYPE_SWIRAM,1,true>,
		},
		{
			MEMOP_STM<1,MEMTYPE_GENERIC,1,true>,
			MEMOP_STM<1,MEMTYPE_MAIN,1,true>,
			MEMOP_STM<1,MEMTYPE_GENERIC,1,true>,//MEMOP_STM<1,MEMTYPE_DTCM_ARM9,1,true>,
			MEMOP_STM<1,MEMTYPE_ERAM_ARM7,1,true>,
			MEMOP_STM<1,MEMTYPE_SWIRAM,1,true>,
		}
	};

	static const MemOp3 STM_SEQUENCE_Down_Tab[2][MEMTYPE_COUNT] = 
	{
		{
			MEMOP_STM_SEQUENCE<0,MEMTYPE_GENERIC,1,false>,
			MEMOP_STM_SEQUENCE<0,MEMTYPE_MAIN,1,false>,
			MEMOP_STM_SEQUENCE<0,MEMTYPE_DTCM_ARM9,1,false>,
			MEMOP_STM_SEQUENCE<0,MEMTYPE_GENERIC,1,false>,//MEMOP_STM_SEQUENCE<0,MEMTYPE_ERAM_ARM7,1,false>,
			MEMOP_STM_SEQUENCE<0,MEMTYPE_SWIRAM,1,false>,
		},
		{
			MEMOP_STM_SEQUENCE<1,MEMTYPE_GENERIC,1,false>,
			MEMOP_STM_SEQUENCE<1,MEMTYPE_MAIN,1,false>,
			MEMOP_STM_SEQUENCE<1,MEMTYPE_GENERIC,1,false>,//MEMOP_STM_SEQUENCE<1,MEMTYPE_DTCM_ARM9,1,false>,
			MEMOP_STM_SEQUENCE<1,MEMTYPE_ERAM_ARM7,1,false>,
			MEMOP_STM_SEQUENCE<1,MEMTYPE_SWIRAM,1,false>,
		}
	};

	static const MemOp3 STM_Down_Tab[2][MEMTYPE_COUNT] = 
	{
		{
			MEMOP_STM<0,MEMTYPE_GENERIC,1,false>,
			MEMOP_STM<0,MEMTYPE_MAIN,1,false>,
			MEMOP_STM<0,MEMTYPE_DTCM_ARM9,1,false>,
			MEMOP_STM<0,MEMTYPE_GENERIC,1,false>,//MEMOP_STM<0,MEMTYPE_ERAM_ARM7,1,false>,
			MEMOP_STM<0,MEMTYPE_SWIRAM,1,false>,
		},
		{
			MEMOP_STM<1,MEMTYPE_GENERIC,1,false>,
			MEMOP_STM<1,MEMTYPE_MAIN,1,false>,
			MEMOP_STM<1,MEMTYPE_GENERIC,1,false>,//MEMOP_STM<1,MEMTYPE_DTCM_ARM9,1,false>,
			MEMOP_STM<1,MEMTYPE_ERAM_ARM7,1,false>,
			MEMOP_STM<1,MEMTYPE_SWIRAM,1,false>,
		}
	};

	OPCDECODER_DECL(IR_STM)
	{
		u32 PROCNUM = d.ProcessID;

		bool StoreR15 = false;
		u32 SequenceFlag = 0;//0:no sequence start,1:one sequence start,2:one sequence end,3:more than one sequence start
		u32 Count = 0;
		u32* Regs[16];
		for(u32 RegisterList = d.RegisterList, n = 0; RegisterList; RegisterList >>= 1, n++)
		{
			if (RegisterList & 0x1)
			{
				Regs[Count] = &GETCPU.R[n];
				Count++;

				if (n == 15)
					StoreR15 = true;

				if (SequenceFlag == 0)
					SequenceFlag = 1;
				else if (SequenceFlag == 2)
					SequenceFlag = 3;
			}
			else
			{
				if (SequenceFlag == 1)
					SequenceFlag = 2;
			}
		}

		if (d.S)
			WRITE_CODE("if (((Status_Reg*)%#p)->bits.mode!=%u){\n", &(GETCPU.CPSR), USR);

		if (StoreR15)
			WRITE_CODE("REG_W(%#p) = %u;\n", REG_W(15), d.CalcR15(d));

		bool IsOneSequence = (SequenceFlag == 1 || SequenceFlag == 2);

		if (d.W)
			WRITE_CODE("u32 adr_old = REG_R%s(%#p);\n", REG_R(d.Rn));

		if (d.P)
			WRITE_CODE("u32 adr = (REG_R%s(%#p) %c 4) & 0xFFFFFFFC;\n", REG_R(d.Rn), d.U?'+':'-');
		else
			WRITE_CODE("u32 adr = REG_R%s(%#p) & 0xFFFFFFFC;\n", REG_R(d.Rn));

		if (d.S)
			WRITE_CODE("u32 oldmode = ((u32 (*)(void*,u8))%#p)((void*)%#p,%u);\n", armcpu_switchMode, GETCPUPTR, SYS);

		if (IsOneSequence)
		{
			if (d.U)
				WRITE_CODE("ExecuteCycles+=((u32 (*)(u32, u32, u32*))%#p)(adr, %u,(u32*)%#p);\n", STM_SEQUENCE_Up_Tab[PROCNUM][0], Count, Regs[0]);
			else
				WRITE_CODE("ExecuteCycles+=((u32 (*)(u32, u32, u32*))%#p)(adr, %u,(u32*)%#p);\n", STM_SEQUENCE_Down_Tab[PROCNUM][0], Count, Regs[0]);
		}
		else
		{
			WRITE_CODE("static const u32* Regs[]={");
			for (u32 i = 0; i < Count; i++)
			{
				WRITE_CODE("(u32*)%#p", Regs[i]);
				if (i != Count - 1)
					WRITE_CODE(",");
			}
			WRITE_CODE("};\n");

			if (d.U)
				WRITE_CODE("ExecuteCycles+=((u32 (*)(u32, u32, u32*))%#p)(adr, %u,(u32*)&Regs[0]);\n", STM_Up_Tab[PROCNUM][0], Count);
			else
				WRITE_CODE("ExecuteCycles+=((u32 (*)(u32, u32, u32*))%#p)(adr, %u,(u32*)&Regs[0]);\n", STM_Down_Tab[PROCNUM][0], Count);
		}

		if (d.S)
		{
			if (d.W)
				WRITE_CODE("REG_W(%#p)=adr_old %c %u;\n", REG_W(d.Rn), d.U?'+':'-', Count*4);

			WRITE_CODE("((u32 (*)(void*,u8))%#p)((void*)%#p,oldmode);\n", armcpu_switchMode, GETCPUPTR);

			WRITE_CODE("}else ExecuteCycles+=2;\n");
		}
		else
		{
			if (d.W)
				WRITE_CODE("REG_W(%#p)=adr_old %c %u;\n", REG_W(d.Rn), d.U?'+':'-', Count*4);
		}
	}

	template<u32 PROCNUM, u32 memtype, u32 cycle>
	static u32 MEMOP_SWP(u32 adr, u32 *Rd, u32 Rm)
	{
		u32 tmp = ROR(READ32(GETCPU.mem_if->data, adr), (adr & 3)<<3);
		WRITE32(GETCPU.mem_if->data, adr, Rm);
		*Rd = tmp;

		return MMU_aluMemCycles<PROCNUM>(cycle, MMU_memAccessCycles<PROCNUM,32,MMU_AD_READ>(adr) + MMU_memAccessCycles<PROCNUM,32,MMU_AD_WRITE>(adr));
	}

	template<u32 PROCNUM, u32 memtype, u32 cycle>
	static u32 MEMOP_SWPB(u32 adr, u32 *Rd, u32 Rm)
	{
		u32 tmp = READ8(GETCPU.mem_if->data, adr);
		WRITE8(GETCPU.mem_if->data, adr, Rm);
		*Rd = tmp;

		return MMU_aluMemCycles<PROCNUM>(cycle, MMU_memAccessCycles<PROCNUM,8,MMU_AD_READ>(adr) + MMU_memAccessCycles<PROCNUM,8,MMU_AD_WRITE>(adr));
	}

	static const MemOp4 SWP_Tab[2][MEMTYPE_COUNT] = 
	{
		{
			MEMOP_SWP<0,MEMTYPE_GENERIC,4>,
			MEMOP_SWP<0,MEMTYPE_MAIN,4>,
			MEMOP_SWP<0,MEMTYPE_DTCM_ARM9,4>,
			MEMOP_SWP<0,MEMTYPE_GENERIC,4>,//MEMOP_SWP<0,MEMTYPE_ERAM_ARM7,4>,
			MEMOP_SWP<0,MEMTYPE_SWIRAM,4>,
		},
		{
			MEMOP_SWP<1,MEMTYPE_GENERIC,4>,
			MEMOP_SWP<1,MEMTYPE_MAIN,4>,
			MEMOP_SWP<1,MEMTYPE_GENERIC,4>,//MEMOP_SWP<1,MEMTYPE_DTCM_ARM9,4>,
			MEMOP_SWP<1,MEMTYPE_ERAM_ARM7,4>,
			MEMOP_SWP<1,MEMTYPE_SWIRAM,4>,
		}
	};

	static const MemOp4 SWPB_Tab[2][MEMTYPE_COUNT] = 
	{
		{
			MEMOP_SWPB<0,MEMTYPE_GENERIC,4>,
			MEMOP_SWPB<0,MEMTYPE_MAIN,4>,
			MEMOP_SWPB<0,MEMTYPE_DTCM_ARM9,4>,
			MEMOP_SWPB<0,MEMTYPE_GENERIC,4>,//MEMOP_SWPB<0,MEMTYPE_ERAM_ARM7,4>,
			MEMOP_SWPB<0,MEMTYPE_SWIRAM,4>,
		},
		{
			MEMOP_SWPB<1,MEMTYPE_GENERIC,4>,
			MEMOP_SWPB<1,MEMTYPE_MAIN,4>,
			MEMOP_SWPB<1,MEMTYPE_GENERIC,4>,//MEMOP_SWPB<1,MEMTYPE_DTCM_ARM9,4>,
			MEMOP_SWPB<1,MEMTYPE_ERAM_ARM7,4>,
			MEMOP_SWPB<1,MEMTYPE_SWIRAM,4>,
		}
	};

	OPCDECODER_DECL(IR_SWP)
	{
		u32 PROCNUM = d.ProcessID;

		if (d.B)
			WRITE_CODE("ExecuteCycles+=((u32 (*)(u32, u32*, u32))%#p)(REG_R%s(%#p),REGPTR(%#p),REG_R%s(%#p));\n", SWPB_Tab[PROCNUM][0], REG_R(d.Rn), REGPTR(d.Rd), REG_R(d.Rm));
		else
			WRITE_CODE("ExecuteCycles+=((u32 (*)(u32, u32*, u32))%#p)(REG_R%s(%#p),REGPTR(%#p),REG_R%s(%#p));\n", SWP_Tab[PROCNUM][0], REG_R(d.Rn), REGPTR(d.Rd), REG_R(d.Rm));
	}

	OPCDECODER_DECL(IR_B)
	{
		u32 PROCNUM = d.ProcessID;

		WRITE_CODE("REG_W(%#p)=%u;\n", REG_W(15), d.Immediate);

		R15ModifiedGenerate(d, szCodeBuffer);
	}

	OPCDECODER_DECL(IR_BL)
	{
		u32 PROCNUM = d.ProcessID;

		WRITE_CODE("REG_W(%#p)=%u;\n", REG_W(14), d.CalcNextInstruction(d) | d.ThumbFlag);
		WRITE_CODE("REG_W(%#p)=%u;\n", REG_W(15), d.Immediate);

		R15ModifiedGenerate(d, szCodeBuffer);
	}

	OPCDECODER_DECL(IR_BX)
	{
		u32 PROCNUM = d.ProcessID;

		WRITE_CODE("u32 tmp = REG_R%s(%#p);\n", REG_R(d.Rn));

		WRITE_CODE("((Status_Reg*)%#p)->bits.T=BIT0(tmp);\n", &(GETCPU.CPSR));
		WRITE_CODE("REG_W(%#p)=tmp & (0xFFFFFFFC|(BIT0(tmp)<<1));\n", REG_W(15));

		R15ModifiedGenerate(d, szCodeBuffer);
	}

	OPCDECODER_DECL(IR_BLX)
	{
		u32 PROCNUM = d.ProcessID;

		WRITE_CODE("u32 tmp = REG_R%s(%#p);\n", REG_R(d.Rn));

		WRITE_CODE("REG_W(%#p)=%u;\n", REG_W(14), d.CalcNextInstruction(d) | d.ThumbFlag);
		WRITE_CODE("((Status_Reg*)%#p)->bits.T=BIT0(tmp);\n", &(GETCPU.CPSR));
		WRITE_CODE("REG_W(%#p)=tmp & (0xFFFFFFFC|(BIT0(tmp)<<1));\n", REG_W(15));

		R15ModifiedGenerate(d, szCodeBuffer);
	}

	OPCDECODER_DECL(IR_SWI)
	{
		u32 PROCNUM = d.ProcessID;

		if (GETCPU.swi_tab)
		{
			if (PROCNUM == 0)
				WRITE_CODE("if ((*(u32*)%#p) != 0x00000000){\n", &(GETCPU.intVector));
			else
				WRITE_CODE("if ((*(u32*)%#p) != 0xFFFF0000){\n", &(GETCPU.intVector));

			if (d.MayHalt)
			{
				if (d.ThumbFlag)
				{
					WRITE_CODE("(*(u32*)%#p) = %u;\n", &(GETCPU.instruct_adr), d.Address);
					//WRITE_CODE("(*(u32*)%#p) = %u;\n", &(GETCPU.next_instruction), d.CalcNextInstruction(d));
					// alway set r15 to next_instruction
					WRITE_CODE("REG_W(%#p) = %u;\n", REG_W(15), d.CalcNextInstruction(d));
				}
				else
				{
					WRITE_CODE("(*(u32*)%#p) = %u;\n", &(GETCPU.instruct_adr), d.Address);
					//WRITE_CODE("(*(u32*)%#p) = %u;\n", &(GETCPU.next_instruction), d.CalcNextInstruction(d));
					// alway set r15 to next_instruction
					WRITE_CODE("REG_W(%#p) = %u;\n", REG_W(15), d.CalcNextInstruction(d));
				}
			}

			WRITE_CODE("ExecuteCycles+=((u32 (*)())%#p)()+3;\n", GETCPU.swi_tab[d.Immediate]);

			if (d.MayHalt)
				R15ModifiedGenerate(d, szCodeBuffer);

			WRITE_CODE("}else{\n");
		}
		
		{
			WRITE_CODE("Status_Reg tmp;\n");
			WRITE_CODE("tmp.val = ((Status_Reg*)%#p)->val;\n", &(GETCPU.CPSR));
			WRITE_CODE("((u32 (*)(void*,u8))%#p)((void*)%#p,%u);\n", armcpu_switchMode, GETCPUPTR, SVC);
			WRITE_CODE("REG_W(%#p)=%u;\n", REG_W(14), d.CalcNextInstruction(d));
			WRITE_CODE("((Status_Reg*)%#p)->val = tmp.val;\n", &(GETCPU.SPSR));
			WRITE_CODE("((Status_Reg*)%#p)->bits.T=0;\n", &(GETCPU.CPSR));
			WRITE_CODE("((Status_Reg*)%#p)->bits.I=1;\n", &(GETCPU.CPSR));
			WRITE_CODE("((void (*)(void*))%#p)((void*)%#p);\n", armcpu_changeCPSR, GETCPUPTR);
			WRITE_CODE("REG_W(%#p)= (*(u32*)%#p) + 0x08;\n", REG_W(15), &(GETCPU.intVector));

			WRITE_CODE("ExecuteCycles+=3;\n");

			R15ModifiedGenerate(d, szCodeBuffer);
		}

		if (GETCPU.swi_tab)
			WRITE_CODE("}\n");
	}

	OPCDECODER_DECL(IR_MSR)
	{
		u32 PROCNUM = d.ProcessID;

		if (d.P)
		{
			u32 byte_mask = (BIT0(d.OpData)?0x000000FF:0x00000000) |
							(BIT1(d.OpData)?0x0000FF00:0x00000000) |
							(BIT2(d.OpData)?0x00FF0000:0x00000000) |
							(BIT3(d.OpData)?0xFF000000:0x00000000);

			WRITE_CODE("if(((Status_Reg*)%#p)->bits.mode!=%u&&((Status_Reg*)%#p)->bits.mode!=%u){\n",
					&(GETCPU.CPSR), USR, &(GETCPU.CPSR), SYS);
			if (d.I)
				WRITE_CODE("(*(u32*)%#p) = ((*(u32*)%#p) & %u) | %u);\n", 
						&(GETCPU.SPSR.val), &(GETCPU.SPSR.val), ~byte_mask, byte_mask & d.Immediate);
			else
				WRITE_CODE("(*(u32*)%#p) = ((*(u32*)%#p) & %u) | (REG_R%s(%#p) & %u);\n", 
						&(GETCPU.SPSR.val), &(GETCPU.SPSR.val), ~byte_mask, REG_R(d.Rm), byte_mask);
			WRITE_CODE("((void (*)(void*))%#p)((void*)%#p);\n", armcpu_changeCPSR, GETCPUPTR);
			WRITE_CODE("}\n");
		}
		else
		{
			u32 byte_mask_usr = (BIT3(d.OpData)?0xFF000000:0x00000000);
			u32 byte_mask_other = (BIT0(d.OpData)?0x000000FF:0x00000000) |
								(BIT1(d.OpData)?0x0000FF00:0x00000000) |
								(BIT2(d.OpData)?0x00FF0000:0x00000000) |
								(BIT3(d.OpData)?0xFF000000:0x00000000);

			WRITE_CODE("u32 byte_mask=(((Status_Reg*)%#p)->bits.mode==%u)?%u:%u;\n", 
					&(GETCPU.CPSR), USR, byte_mask_usr, byte_mask_other);

			if (BIT0(d.OpData))
			{
				WRITE_CODE("if(((Status_Reg*)%#p)->bits.mode!=%u){\n", &(GETCPU.CPSR), USR);
				if (d.I)
					WRITE_CODE("((u32 (*)(void*,u8))%#p)((void*)%#p,%u);\n", armcpu_switchMode, GETCPUPTR, d.Immediate & 0x1F);
				else
					WRITE_CODE("((u32 (*)(void*,u8))%#p)((void*)%#p,REG_R%s(%#p)&0x1F);\n", armcpu_switchMode, GETCPUPTR, REG_R(d.Rm));
				WRITE_CODE("}\n");
			}

			if (d.I)
				WRITE_CODE("(*(u32*)%#p) = ((*(u32*)%#p) & ~byte_mask) | (%u & byte_mask);\n", 
						&(GETCPU.CPSR.val), &(GETCPU.CPSR.val), d.Immediate);
			else
				WRITE_CODE("(*(u32*)%#p)=((*(u32*)%#p)&~byte_mask)|(REG_R%s(%#p)&byte_mask);\n", 
						&(GETCPU.CPSR.val), &(GETCPU.CPSR.val), REG_R(d.Rm));
			WRITE_CODE("((void (*)(void*))%#p)((void*)%#p);\n", armcpu_changeCPSR, GETCPUPTR);
		}
	}

	OPCDECODER_DECL(IR_MRS)
	{
		u32 PROCNUM = d.ProcessID;

		if (d.P)
			WRITE_CODE("REG_W(%#p)= (*(u32*)%#p);\n", REG_W(d.Rd), &(GETCPU.SPSR.val));
		else
			WRITE_CODE("REG_W(%#p)= (*(u32*)%#p);\n", REG_W(d.Rd), &(GETCPU.CPSR.val));
	}

	OPCDECODER_DECL(IR_MCR)
	{
		u32 PROCNUM = d.ProcessID;

		if (d.CPNum == 15)
		{
			WRITE_CODE("((BOOL (*)(u32,u8,u8,u8,u8))%#p)(REG_R%s(%#p),%u,%u,%u,%u);\n", 
					armcp15_moveARM2CP, REG_R(d.Rd), d.CRn, d.CRm, d.CPOpc, d.CP);
		}
		else
		{
			INFO("ARM%c: MCR P%i, 0, R%i, C%i, C%i, %i, %i (don't allocated coprocessor)\n", 
				PROCNUM?'7':'9', d.CPNum, d.Rd, d.CRn, d.CRm, d.CPOpc, d.CP);
		}
	}

	OPCDECODER_DECL(IR_MRC)
	{
		u32 PROCNUM = d.ProcessID;

		if (d.CPNum == 15)
		{
			if (d.Rd == 15)
			{
				WRITE_CODE("u32 data = 0;\n");
				WRITE_CODE("((BOOL (*)(u32*,u8,u8,u8,u8))%#p)(&data,%u,%u,%u,%u);\n", 
						armcp15_moveCP2ARM, d.CRn, d.CRm, d.CPOpc, d.CP);
				WRITE_CODE("((Status_Reg*)%#p)->bits.N=BIT31(data);\n", &(GETCPU.CPSR));
				WRITE_CODE("((Status_Reg*)%#p)->bits.Z=BIT30(data);\n", &(GETCPU.CPSR));
				WRITE_CODE("((Status_Reg*)%#p)->bits.C=BIT29(data);\n", &(GETCPU.CPSR));
				WRITE_CODE("((Status_Reg*)%#p)->bits.V=BIT28(data);\n", &(GETCPU.CPSR));
			}
			else
			{
				WRITE_CODE("((BOOL (*)(u32*,u8,u8,u8,u8))%#p)(REGPTR(%#p),%u,%u,%u,%u);\n", 
						armcp15_moveCP2ARM, REGPTR(d.Rd), d.CRn, d.CRm, d.CPOpc, d.CP);
			}
		}
		else
		{
			INFO("ARM%c: MRC P%i, 0, R%i, C%i, C%i, %i, %i (don't allocated coprocessor)\n", 
				PROCNUM?'7':'9', d.CPNum, d.Rd, d.CRn, d.CRm, d.CPOpc, d.CP);
		}
	}

	static const u8 CLZ_TAB[16]=
	{
		0,							// 0000
		1,							// 0001
		2, 2,						// 001X
		3, 3, 3, 3,					// 01XX
		4, 4, 4, 4, 4, 4, 4, 4		// 1XXX
	};

	OPCDECODER_DECL(IR_CLZ)
	{
		u32 PROCNUM = d.ProcessID;

		WRITE_CODE("u32 Rm = REG_R%s(%#p);\n", REG_R(d.Rm));
		WRITE_CODE("if(Rm==0){\n");
		WRITE_CODE("REG_W(%#p)=32;\n", REG_W(d.Rd));
		WRITE_CODE("}else{\n");
		WRITE_CODE("Rm |= (Rm >>1);\n");
		WRITE_CODE("Rm |= (Rm >>2);\n");
		WRITE_CODE("Rm |= (Rm >>4);\n");
		WRITE_CODE("Rm |= (Rm >>8);\n");
		WRITE_CODE("Rm |= (Rm >>16);\n");
		WRITE_CODE("static const u8* CLZ_TAB = (u8*)%#p;\n", CLZ_TAB);
		WRITE_CODE("u32 pos = CLZ_TAB[Rm&0xF] + \n");
		WRITE_CODE("			CLZ_TAB[(Rm>>4)&0xF] + \n");
		WRITE_CODE("			CLZ_TAB[(Rm>>8)&0xF] + \n");
		WRITE_CODE("			CLZ_TAB[(Rm>>12)&0xF] + \n");
		WRITE_CODE("			CLZ_TAB[(Rm>>16)&0xF] + \n");
		WRITE_CODE("			CLZ_TAB[(Rm>>20)&0xF] + \n");
		WRITE_CODE("			CLZ_TAB[(Rm>>24)&0xF] + \n");
		WRITE_CODE("			CLZ_TAB[(Rm>>28)&0xF];\n");
		WRITE_CODE("REG_W(%#p)=32-pos;}\n", REG_W(d.Rd));
	}

	OPCDECODER_DECL(IR_QADD)
	{
		u32 PROCNUM = d.ProcessID;

		WRITE_CODE("u32 res = REG_R%s(%#p) + REG_R%s(%#p);\n", REG_R(d.Rn), REG_R(d.Rm));
		WRITE_CODE("if(SIGNED_OVERFLOW(REG_R%s(%#p),REG_R%s(%#p),res)){\n", 
				REG_R(d.Rn), REG_R(d.Rm));
		WRITE_CODE("((Status_Reg*)%#p)->bits.Q=1;\n", &(GETCPU.CPSR));
		WRITE_CODE("REG_W(%#p)=0x80000000-BIT31(res);\n", REG_W(d.Rd));
		WRITE_CODE("}else{\n");
		if (d.R15Modified)
		{
			WRITE_CODE("REG_W(%#p)=res & 0xFFFFFFFC;\n", REG_W(d.Rd));
			R15ModifiedGenerate(d, szCodeBuffer);
		}
		else
			WRITE_CODE("REG_W(%#p)=res;\n", REG_W(d.Rd));
		WRITE_CODE("}\n");
	}

	OPCDECODER_DECL(IR_QSUB)
	{
		u32 PROCNUM = d.ProcessID;

		WRITE_CODE("u32 res = REG_R%s(%#p) - REG_R%s(%#p);\n", REG_R(d.Rm), REG_R(d.Rn));
		WRITE_CODE("if(SIGNED_UNDERFLOW(REG_R%s(%#p),REG_R%s(%#p),res)){\n", 
				REG_R(d.Rm), REG_R(d.Rn));
		WRITE_CODE("((Status_Reg*)%#p)->bits.Q=1;\n", &(GETCPU.CPSR));
		WRITE_CODE("REG_W(%#p)=0x80000000-BIT31(res);\n", REG_W(d.Rd));
		WRITE_CODE("}else{\n");
		if (d.R15Modified)
		{
			WRITE_CODE("REG_W(%#p)=res & 0xFFFFFFFC;\n", REG_W(d.Rd));
			R15ModifiedGenerate(d, szCodeBuffer);
		}
		else
			WRITE_CODE("REG_W(%#p)=res;\n", REG_W(d.Rd));
		WRITE_CODE("}\n");
	}

	OPCDECODER_DECL(IR_QDADD)
	{
		u32 PROCNUM = d.ProcessID;

		WRITE_CODE("u32 mul = REG_R%s(%#p)<<1;\n", REG_R(d.Rn));
		WRITE_CODE("if(BIT31(REG_R%s(%#p))!=BIT31(mul)){\n", REG_R(d.Rn));
		WRITE_CODE("((Status_Reg*)%#p)->bits.Q=1;\n", &(GETCPU.CPSR));
		WRITE_CODE("REG_W(%#p)=0x80000000-BIT31(res);\n", REG_W(d.Rd));
		WRITE_CODE("}\n");
		WRITE_CODE("u32 res = mul + REG_R%s(%#p);\n", REG_R(d.Rm));
		WRITE_CODE("if(SIGNED_OVERFLOW(REG_R%s(%#p),mul, res)){\n", REG_R(d.Rm));
		WRITE_CODE("((Status_Reg*)%#p)->bits.Q=1;\n", &(GETCPU.CPSR));
		WRITE_CODE("REG_W(%#p)=0x80000000-BIT31(res);\n", REG_W(d.Rd));
		WRITE_CODE("}else{\n");
		if (d.R15Modified)
		{
			WRITE_CODE("REG_W(%#p)=res & 0xFFFFFFFC;\n", REG_W(d.Rd));
			R15ModifiedGenerate(d, szCodeBuffer);
		}
		else
			WRITE_CODE("REG_W(%#p)=res;\n", REG_W(d.Rd));
		WRITE_CODE("}\n");
	}

	OPCDECODER_DECL(IR_QDSUB)
	{
		u32 PROCNUM = d.ProcessID;

		WRITE_CODE("u32 mul = REG_R%s(%#p)<<1;\n", REG_R(d.Rn));
		WRITE_CODE("if(BIT31(REG_R%s(%#p))!=BIT31(mul)){\n", REG_R(d.Rn));
		WRITE_CODE("((Status_Reg*)%#p)->bits.Q=1;\n", &(GETCPU.CPSR));
		WRITE_CODE("REG_W(%#p)=0x80000000-BIT31(res);\n", REG_W(d.Rd));
		WRITE_CODE("}\n");
		WRITE_CODE("u32 res = REG_R%s(%#p) - mul;\n", REG_R(d.Rm));
		WRITE_CODE("if(SIGNED_UNDERFLOW(REG_R%s(%#p),mul, res)){\n", REG_R(d.Rm));
		WRITE_CODE("((Status_Reg*)%#p)->bits.Q=1;\n", &(GETCPU.CPSR));
		WRITE_CODE("REG_W(%#p)=0x80000000-BIT31(res);\n", REG_W(d.Rd));
		WRITE_CODE("}else{\n");
		if (d.R15Modified)
		{
			WRITE_CODE("REG_W(%#p)=res & 0xFFFFFFFC;\n", REG_W(d.Rd));
			R15ModifiedGenerate(d, szCodeBuffer);
		}
		else
			WRITE_CODE("REG_W(%#p)=res;\n", REG_W(d.Rd));
		WRITE_CODE("}\n");
	}

	OPCDECODER_DECL(IR_BLX_IMM)
	{
		u32 PROCNUM = d.ProcessID;

		if(d.ThumbFlag)
			WRITE_CODE("((Status_Reg*)%#p)->bits.T=0;\n", &(GETCPU.CPSR));
		else
			WRITE_CODE("((Status_Reg*)%#p)->bits.T=1;\n", &(GETCPU.CPSR));

		WRITE_CODE("REG_W(%#p)=%u;\n", REG_W(14), d.CalcNextInstruction(d) | d.ThumbFlag);
		WRITE_CODE("REG_W(%#p)=%u;\n", REG_W(15), d.Immediate);

		R15ModifiedGenerate(d, szCodeBuffer);
	}

	OPCDECODER_DECL(IR_BKPT)
	{
		u32 PROCNUM = d.ProcessID;

		INFO("ARM%c: Unimplemented opcode BKPT\n", PROCNUM?'7':'9');
	}
};

static const IROpCDecoder iropcdecoder_set[IR_MAXNUM] = {
#define TABDECL(x) ArmCJit::x##_CDecoder
#include "ArmAnalyze_tabdef.inc"
#undef TABDECL
};

////////////////////////////////////////////////////////////////////
template<u32 PROCNUM, bool thumb>
static u32 FASTCALL RunInterpreter(const Decoded &d)
{
	u32 cycles;

	GETCPU.next_instruction = d.CalcNextInstruction(d);
	GETCPU.R[15] = d.CalcR15(d);
	if (thumb)
	{
		u32 opcode = d.Instruction.ThumbOp;
		cycles = thumb_instructions_set[PROCNUM][opcode>>6](opcode);
	}
	else
	{
		u32 opcode = d.Instruction.ArmOp;
		if(CONDITION(opcode) == 0xE || TEST_COND(CONDITION(opcode), CODE(opcode), GETCPU.CPSR))
			cycles = arm_instructions_set[PROCNUM][INSTRUCTION_INDEX(opcode)](opcode);
		else
			cycles = 1;
	}
	GETCPU.instruct_adr = GETCPU.next_instruction;

	return cycles;
}

static const Interpreter s_OpDecode[2][2] = {RunInterpreter<0,false>, RunInterpreter<0,true>, RunInterpreter<1,false>, RunInterpreter<1,true>};

////////////////////////////////////////////////////////////////////
static void FASTCALL InterpreterFallback(const Decoded &d, char *&szCodeBuffer)
{
	u32 PROCNUM = d.ProcessID;

	WRITE_CODE("(*(u32*)%#p) = %u;\n", &(GETCPU.next_instruction), d.CalcNextInstruction(d));
	WRITE_CODE("REG_W(%#p) = %u;\n", REG_W(15), d.CalcR15(d));
	if (d.ThumbFlag)
		WRITE_CODE("ExecuteCycles+=((u32 (FASTCALL *)(u32))%#p)(%u);\n", thumb_instructions_set[PROCNUM][d.Instruction.ThumbOp>>6], d.Instruction.ThumbOp);
	else
		WRITE_CODE("ExecuteCycles+=((u32 (FASTCALL *)(u32))%#p)(%u);\n", arm_instructions_set[PROCNUM][INSTRUCTION_INDEX(d.Instruction.ArmOp)], d.Instruction.ArmOp);
	WRITE_CODE("(*(u32*)%#p) = (*(u32*)%#p);\n", &(GETCPU.instruct_adr), &(GETCPU.next_instruction));

	if (d.R15Modified)
		WRITE_CODE("return ExecuteCycles;\n");
}
////////////////////////////////////////////////////////////////////
static const u32 s_CacheReserveMin = 4 * 1024 * 1024;
static u32 s_CacheReserve = 16 * 1024 * 1024;
static MemBuffer* s_CodeBuffer = NULL;

static void ReleaseCodeBuffer()
{
	delete s_CodeBuffer;
	s_CodeBuffer = NULL;
}

static void InitializeCodeBuffer()
{
	ReleaseCodeBuffer();

	s_CodeBuffer = new MemBuffer(MemBuffer::kRead|MemBuffer::kWrite|MemBuffer::kExec,s_CacheReserveMin);
	s_CodeBuffer->Reserve(s_CacheReserve);
	s_CacheReserve = s_CodeBuffer->GetReservedSize();

	INFO("CodeBuffer : start=%#p, size1=%u, size2=%u\n", 
		s_CodeBuffer->GetBasePtr(), s_CodeBuffer->GetCommittedSize(), s_CacheReserve);
}

static void ResetCodeBuffer()
{
	u8* base = s_CodeBuffer->GetBasePtr();
	u32 size = s_CodeBuffer->GetUsedSize();

	PROGINFO("CodeBuffer : used=%u\n", size);

	s_CodeBuffer->Reset();

	FlushIcacheSection(base, base + size);
}

static u8* AllocCodeBuffer(size_t size)
{
	//return s_CodeBuffer->Alloc(size);
	static const u32 align = 4 - 1;

	u32 size_new = size + align;

	uintptr_t ptr = (uintptr_t)s_CodeBuffer->Alloc(size_new);
	if (ptr == 0)
		return NULL;

	uintptr_t retptr = (ptr + align) & ~align;

	return (u8*)retptr;
}

////////////////////////////////////////////////////////////////////
static MemBuffer* s_CMemBuffer = NULL;
static char* s_CBufferBase = NULL;
static char* s_CBuffer = NULL;
static char* s_CBufferCur = NULL;

static void ReleaseCBuffer()
{
	delete s_CMemBuffer;

	s_CMemBuffer = NULL;
	s_CBufferBase = NULL;
	s_CBuffer = NULL;
	s_CBufferCur = NULL;
}

static void InitializeCBuffer()
{
	ReleaseCBuffer();

	static const int Size = 1024 * 1024;

	s_CMemBuffer = new MemBuffer(MemBuffer::kRead|MemBuffer::kWrite,Size);
	s_CMemBuffer->Reserve();
	s_CBufferBase = (char*)s_CMemBuffer->Alloc(Size);

	{
		char* szCodeBuffer = s_CBufferBase;

		WRITE_CODE("typedef unsigned char u8;\n");
		WRITE_CODE("typedef unsigned short u16;\n");
		WRITE_CODE("typedef unsigned int u32;\n");
		WRITE_CODE("typedef unsigned long long u64;\n");
		WRITE_CODE("typedef signed char s8;\n");
		WRITE_CODE("typedef signed short s16;\n");
		WRITE_CODE("typedef signed int s32;\n");
		WRITE_CODE("typedef signed long long s64;\n");
		WRITE_CODE("typedef int BOOL;\n");

#ifdef __MINGW32__ 
		WRITE_CODE("#define FASTCALL __attribute__((fastcall))\n");
#elif defined (__i386__) && !defined(__clang__)
		WRITE_CODE("#define FASTCALL __attribute__((regparm(3)))\n");
#elif defined(_MSC_VER) || defined(__INTEL_COMPILER)
		//WRITE_CODE("#define FASTCALL __fastcall\n");
		WRITE_CODE("#define FASTCALL __attribute__((fastcall))\n");
#else
		WRITE_CODE("#define FASTCALL\n");
#endif

		WRITE_CODE("#define BIT(n)  (1<<(n))\n");
		WRITE_CODE("#define BIT_N(i,n)  (((i)>>(n))&1)\n");
		WRITE_CODE("#define BIT0(i)     ((i)&1)\n");
		//WRITE_CODE("#define BIT1(i)     BIT_N((i),1)\n");
		//WRITE_CODE("#define BIT2(i)     BIT_N((i),2)\n");
		//WRITE_CODE("#define BIT3(i)     BIT_N((i),3)\n");
		//WRITE_CODE("#define BIT4(i)     BIT_N((i),4)\n");
		//WRITE_CODE("#define BIT5(i)     BIT_N((i),5)\n");
		//WRITE_CODE("#define BIT6(i)     BIT_N((i),6)\n");
		//WRITE_CODE("#define BIT7(i)     BIT_N((i),7)\n");
		//WRITE_CODE("#define BIT8(i)     BIT_N((i),8)\n");
		//WRITE_CODE("#define BIT9(i)     BIT_N((i),9)\n");
		//WRITE_CODE("#define BIT10(i)     BIT_N((i),10)\n");
		//WRITE_CODE("#define BIT11(i)     BIT_N((i),11)\n");
		//WRITE_CODE("#define BIT12(i)     BIT_N((i),12)\n");
		//WRITE_CODE("#define BIT13(i)     BIT_N((i),13)\n");
		//WRITE_CODE("#define BIT14(i)     BIT_N((i),14)\n");
		//WRITE_CODE("#define BIT15(i)     BIT_N((i),15)\n");
		//WRITE_CODE("#define BIT16(i)     BIT_N((i),16)\n");
		//WRITE_CODE("#define BIT17(i)     BIT_N((i),17)\n");
		//WRITE_CODE("#define BIT18(i)     BIT_N((i),18)\n");
		//WRITE_CODE("#define BIT19(i)     BIT_N((i),19)\n");
		//WRITE_CODE("#define BIT20(i)     BIT_N((i),20)\n");
		//WRITE_CODE("#define BIT21(i)     BIT_N((i),21)\n");
		//WRITE_CODE("#define BIT22(i)     BIT_N((i),22)\n");
		//WRITE_CODE("#define BIT23(i)     BIT_N((i),23)\n");
		//WRITE_CODE("#define BIT24(i)     BIT_N((i),24)\n");
		//WRITE_CODE("#define BIT25(i)     BIT_N((i),25)\n");
		//WRITE_CODE("#define BIT26(i)     BIT_N((i),26)\n");
		//WRITE_CODE("#define BIT27(i)     BIT_N((i),27)\n");
		WRITE_CODE("#define BIT28(i)     BIT_N((i),28)\n");
		WRITE_CODE("#define BIT29(i)     BIT_N((i),29)\n");
		WRITE_CODE("#define BIT30(i)     BIT_N((i),30)\n");
		WRITE_CODE("#define BIT31(i)    ((i)>>31)\n");

		WRITE_CODE("#define HWORD(i)   ((s32)(((s32)(i))>>16))\n");
		WRITE_CODE("#define LWORD(i)   (s32)(((s32)((i)<<16))>>16)\n");

		WRITE_CODE("#define NEG(i) ((i)>>31)\n");
		WRITE_CODE("#define POS(i) ((~(i))>>31)\n");

#ifdef WORDS_BIGENDIAN
		WRITE_CODE("typedef union{\n");
		WRITE_CODE("	struct{\n");
		WRITE_CODE("		u32 N : 1,\n");
		WRITE_CODE("		Z : 1,\n");
		WRITE_CODE("		C : 1,\n");
		WRITE_CODE("		V : 1,\n");
		WRITE_CODE("		Q : 1,\n");
		WRITE_CODE("		RAZ : 19,\n");
		WRITE_CODE("		I : 1,\n");
		WRITE_CODE("		F : 1,\n");
		WRITE_CODE("		T : 1,\n");
		WRITE_CODE("		mode : 5;\n");
		WRITE_CODE("	} bits;\n");
		WRITE_CODE("	u32 val;\n");
		WRITE_CODE("} Status_Reg;\n");
#else
		WRITE_CODE("typedef union{\n");
		WRITE_CODE("	struct{\n");
		WRITE_CODE("		u32 mode : 5,\n");
		WRITE_CODE("		T : 1,\n");
		WRITE_CODE("		F : 1,\n");
		WRITE_CODE("		I : 1,\n");
		WRITE_CODE("		RAZ : 19,\n");
		WRITE_CODE("		Q : 1,\n");
		WRITE_CODE("		V : 1,\n");
		WRITE_CODE("		C : 1,\n");
		WRITE_CODE("		Z : 1,\n");
		WRITE_CODE("		N : 1;\n");
		WRITE_CODE("	} bits;\n");
		WRITE_CODE("	u32 val;\n");
		WRITE_CODE("} Status_Reg;\n");
#endif

		WRITE_CODE("#define REG_R(p)	(*(u32*)(p))\n");
		WRITE_CODE("#define REG_SR(p)	(*(s32*)(p))\n");
		WRITE_CODE("#define REG_R_C(p)	((u32)(p))\n");
		WRITE_CODE("#define REG_SR_C(p)	((s32)(p))\n");
		WRITE_CODE("#define REG_W(p)	(*(u32*)(p))\n");
		WRITE_CODE("#define REG(p)		(*(u32*)(p))\n");
		WRITE_CODE("#define REGPTR(p)	((u32*)(p))\n");

		WRITE_CODE("static const u8* arm_cond_table = (const u8*)%#p;\n", arm_cond_table);

		WRITE_CODE("#define TEST_COND(cond,inst,CPSR) ((arm_cond_table[((CPSR>>24)&0xf0)|(cond)]) & (1<<(inst)))\n");

#if 0
		WRITE_CODE("inline u32 ROR(u32 i, u32 j)\n");
		WRITE_CODE("{return ((((u32)(i))>>(j)) | (((u32)(i))<<(32-(j))));}\n");

		//WRITE_CODE("inline u32 UNSIGNED_OVERFLOW(u32 a,u32 b,u32 c)\n");
		//WRITE_CODE("{return BIT31(((a)&(b)) | (((a)|(b))&(~c)));}\n");

		//WRITE_CODE("inline u32 UNSIGNED_UNDERFLOW(u32 a,u32 b,u32 c)\n");
		//WRITE_CODE("{return BIT31(((~a)&(b)) | (((~a)|(b))&(c)));}\n");

		WRITE_CODE("inline u32 SIGNED_OVERFLOW(u32 a,u32 b,u32 c)\n");
		WRITE_CODE("{return BIT31(((a)&(b)&(~c)) | ((~a)&(~(b))&(c)));}\n");

		WRITE_CODE("inline u32 SIGNED_UNDERFLOW(u32 a,u32 b,u32 c)\n");
		WRITE_CODE("{return BIT31(((a)&(~(b))&(~c)) | ((~a)&(b)&(c)));}\n");

		WRITE_CODE("inline BOOL CarryFrom(s32 left, s32 right)\n");
		WRITE_CODE("{u32 res  = (0xFFFFFFFFU - (u32)left);\n");
		WRITE_CODE("return ((u32)right > res);}\n");

		WRITE_CODE("inline BOOL BorrowFrom(s32 left, s32 right)\n");
		WRITE_CODE("{return ((u32)right > (u32)left);}\n");

		WRITE_CODE("inline BOOL OverflowFromADD(s32 alu_out, s32 left, s32 right)\n");
		WRITE_CODE("{return ((left >= 0 && right >= 0) || (left < 0 && right < 0))\n");
		WRITE_CODE("&& ((left < 0 && alu_out >= 0) || (left >= 0 && alu_out < 0));}\n");

		WRITE_CODE("inline BOOL OverflowFromSUB(s32 alu_out, s32 left, s32 right)\n");
		WRITE_CODE("{return ((left < 0 && right >= 0) || (left >= 0 && right < 0))\n");
		WRITE_CODE("&& ((left < 0 && alu_out >= 0) || (left >= 0 && alu_out < 0));}\n");
#else
		WRITE_CODE("#define ROR(i, j) ((((u32)(i))>>(j)) | (((u32)(i))<<(32-(j))))\n");

		//WRITE_CODE("#define UNSIGNED_OVERFLOW(a,b,c) BIT31(((a)&(b)) | (((a)|(b))&(~c)))\n");

		//WRITE_CODE("#define UNSIGNED_UNDERFLOW(a,b,c) BIT31(((~a)&(b)) | (((~a)|(b))&(c)))\n");

		WRITE_CODE("#define SIGNED_OVERFLOW(a,b,c) BIT31(((a)&(b)&(~c)) | ((~a)&(~(b))&(c)))\n");

		WRITE_CODE("#define SIGNED_UNDERFLOW(a,b,c) BIT31(((a)&(~(b))&(~c)) | ((~a)&(b)&(c)))\n");

		WRITE_CODE("#define CarryFrom(left, right) ((u32)(right) > (0xFFFFFFFFU - (u32)(left)))\n");

		WRITE_CODE("#define BorrowFrom(left, right) ((u32)(right) > (u32)(left))\n");

		//WRITE_CODE("#define OverflowFromADD(alu_out, left, right) ");
		//WRITE_CODE("(((s32)(left) >= 0 && (s32)(right) >= 0) || ((s32)(left) < 0 && (s32)(right) < 0))");
		//WRITE_CODE("&& (((s32)(left) < 0 && (s32)(alu_out) >= 0) || ((s32)(left) >= 0 && (s32)(alu_out) < 0))\n");

		//WRITE_CODE("#define OverflowFromSUB(alu_out, left, right) ");
		//WRITE_CODE("(((s32)(left) < 0 && (s32)(right) >= 0) || ((s32)(left) >= 0 && (s32)(right) < 0))");
		//WRITE_CODE("&& (((s32)(left) < 0 && (s32)(alu_out) >= 0) || ((s32)(left) >= 0 && (s32)(alu_out) < 0))\n");

		WRITE_CODE("#define OverflowFromADD(alu_out, left, right) ((NEG(left) & NEG(right) & POS(alu_out)) | (POS(left) & POS(right) & NEG(alu_out)))\n");

		WRITE_CODE("#define OverflowFromSUB(alu_out, left, right) ((NEG(left) & POS(right) & POS(alu_out)) | (POS(left) & NEG(right) & NEG(alu_out)))\n");
#endif

		s_CBuffer = szCodeBuffer;
	}

	s_CBufferCur = s_CBuffer;
}

static void ResetCBuffer()
{
	s_CBufferCur = s_CBuffer;
}

////////////////////////////////////////////////////////////////////
struct CompiledAddress
{
	u32 Address;
	u32 ProcessID;
};

static const u32 s_MaxCompiledAddress = 16;
static CompiledAddress s_CompiledAddress[s_MaxCompiledAddress] = {0};
static u32 s_CurCompiledAddress = 0;

static void TccErrOutput(void *opaque, const char *msg)
{
	INFO("%s\n", msg);
}

static bool TccGenerateNativeCode()
{
	bool ret = false;
	int size;
	u8* ptr;
	char szFunName[64];

	TCCState *s = tcc_new();

	//tcc_set_output_type(s, TCC_OUTPUT_MEMORY);
	//tcc_set_options(s, "-Werror");
	tcc_set_error_func(s, NULL, TccErrOutput);
	tcc_set_options(s, "-nostdlib");

	if (tcc_compile_string(s, s_CBufferBase) == -1)
	{
		INFO("%s\n", s_CBufferBase);
		goto cleanup;
	}

	size = tcc_relocate(s, NULL);
	if (size == -1)
		goto cleanup;

	ptr = AllocCodeBuffer(size);
	if (!ptr)
	{
		INFO("JIT: cache full, reset cpu.\n");

		arm_cjit.Reset();

		ptr = AllocCodeBuffer(size);
		if (!ptr)
		{
			INFO("JIT: alloc code buffer failed, size : %u.\n", size);
			goto cleanup;
		}
	}
		
	if (tcc_relocate(s, ptr) == -1)
		goto cleanup;

	ret = true;

	FlushIcacheSection(ptr, ptr + size);

	for (u32 i = 0; i < s_CurCompiledAddress; i++)
	{
		sprintf(szFunName, "ArmOp_%u_%u", s_CompiledAddress[i].Address, s_CompiledAddress[i].ProcessID);
		uintptr_t opfun = (uintptr_t)tcc_get_symbol(s, szFunName);
		JITLUT_HANDLE(s_CompiledAddress[i].Address, s_CompiledAddress[i].ProcessID) = opfun;
	}

cleanup:
	tcc_delete(s);

	memset(s_CompiledAddress, 0, sizeof(s_CompiledAddress));
	s_CurCompiledAddress = 0;

	ResetCBuffer();

	return ret;
}

static bool IsAddressCompiled(u32 Address, u32 ProcessID)
{
	//is address already compiled
	for (u32 i = 0; i < s_CurCompiledAddress; i++)
	{
		if (s_CompiledAddress[i].Address == Address && 
			s_CompiledAddress[i].ProcessID == ProcessID)
			return true;
	}

	return false;
}

static bool TccCompileCCode(const BlockInfo &blockinfo)
{
	s_CompiledAddress[s_CurCompiledAddress].Address = blockinfo.Instructions[0].Address;
	s_CompiledAddress[s_CurCompiledAddress].ProcessID = blockinfo.Instructions[0].ProcessID;
	s_CurCompiledAddress++;

	if (s_CurCompiledAddress >= s_MaxCompiledAddress)
		return TccGenerateNativeCode();

	return true;
}

static void ResetTcc()
{
	ResetCBuffer();

	memset(s_CompiledAddress, 0, sizeof(s_CompiledAddress));
	s_CurCompiledAddress = 0;
}

////////////////////////////////////////////////////////////////////
static ArmAnalyze *s_pArmAnalyze = NULL;

TEMPLATE static u32 armcpu_compileblock(BlockInfo &blockinfo, bool runblock)
{
	u32 Cycles = 0;

	Decoded *Instructions = blockinfo.Instructions;
	s32 InstructionsNum = blockinfo.InstructionsNum;

	u32 adr = Instructions[0].Address;

	//find a better way
	if (IsAddressCompiled(adr, PROCNUM))
	{
		TccGenerateNativeCode();
		return 0;
	}

	char* szCodeBuffer = s_CBufferCur;

	WRITE_CODE("u32 ArmOp_%u_%u(){\n", adr, PROCNUM);
	WRITE_CODE("u32 ExecuteCycles=0;\n");
	
	u32 CurSubBlock = INVALID_SUBBLOCK;
	u32 CurInstructions = 0;
	u32 ConstCycles = 0;
	bool IsSubBlockStart = false;

	for (s32 i = 0; i < InstructionsNum; i++)
	{
		Decoded &Inst = Instructions[i];

		if (CurSubBlock != Inst.SubBlock)
		{
			if (ConstCycles > 0)
			{
				WRITE_CODE("ExecuteCycles+=%u;\n", ConstCycles);
				ConstCycles = 0;
			}

			if (IsSubBlockStart)
			{
				WRITE_CODE("}\n");
				WRITE_CODE("else ExecuteCycles+=%u;\n", CurInstructions);
				IsSubBlockStart = false;
			}

			if (Inst.Cond != 0xE && Inst.Cond != 0xF)
			{
				WRITE_CODE("if(TEST_COND(%u,0,(*(u32*)%#p))){\n", Inst.Cond, &GETCPU.CPSR.val);
				IsSubBlockStart = true;
			}

			CurInstructions = 0;

			CurSubBlock = Inst.SubBlock;
		}

		CurInstructions++;

		if (!Inst.VariableCycles)
			ConstCycles += Inst.ExecuteCycles;

		WRITE_CODE("{\n");
		if ((Inst.R15Modified || Inst.TbitModified) && (ConstCycles > 0))
		{
			WRITE_CODE("ExecuteCycles+=%u;\n", ConstCycles);
			ConstCycles = 0;
		}
		if (Inst.ThumbFlag)
		{
			if ((Inst.IROp >= IR_LDM && Inst.IROp <= IR_STM))
				InterpreterFallback(Inst, szCodeBuffer);
			else
				iropcdecoder_set[Inst.IROp](Inst, szCodeBuffer);
		}
		else
		{
			if ((Inst.IROp >= IR_LDM && Inst.IROp <= IR_STM))
				InterpreterFallback(Inst, szCodeBuffer);
			else
				iropcdecoder_set[Inst.IROp](Inst, szCodeBuffer);
		}
		WRITE_CODE("}\n");

		if (runblock)
			Cycles += s_OpDecode[PROCNUM][Inst.ThumbFlag](Inst);
	}

	if (ConstCycles > 0)
	{
		WRITE_CODE("ExecuteCycles+=%u;\n", ConstCycles);
		ConstCycles = 0;
	}

	Decoded &LastIns = Instructions[InstructionsNum - 1];
	if (IsSubBlockStart)
	{
		WRITE_CODE("}\n");
		IsSubBlockStart = false;
	}
	WRITE_CODE("(*(u32*)%#p) = %u;\n", &(GETCPU.instruct_adr), LastIns.Address + (LastIns.ThumbFlag ? 2 : 4));
	WRITE_CODE("return ExecuteCycles;}\n");

	s_CBufferCur = szCodeBuffer;

	TccCompileCCode(blockinfo);

	return Cycles;
}

TEMPLATE u32 armcpu_compileCJIT()
{
	u32 adr = ARMPROC.instruct_adr;
	u32 Cycles = 0;

	if (!JITLUT_MAPPED(adr & 0x0FFFFFFF, PROCNUM))
	{
		INFO("JIT: use unmapped memory address %08X\n", adr);
		execute = false;
		return 1;
	}

	if (!s_pArmAnalyze->Decode(GETCPUPTR) || !s_pArmAnalyze->CreateBlocks())
	{
		INFO("JIT: unknow error cpu[%d].\n", PROCNUM);
		return 1;
	}

	BlockInfo *BlockInfos;
	s32 BlockInfoNum;

	s_pArmAnalyze->GetBlocks(BlockInfos, BlockInfoNum);
	for (s32 BlockNum = 0; BlockNum < BlockInfoNum; BlockNum++)
	{
		Cycles += armcpu_compileblock<PROCNUM>(BlockInfos[BlockNum], BlockNum == 0);
	}

	return Cycles;
}

static void cpuReserve()
{
	InitializeCBuffer();
	InitializeCodeBuffer();

	s_pArmAnalyze = new ArmAnalyze(CommonSettings.jit_max_block_size);

	s_pArmAnalyze->m_MergeSubBlocks = true;
	s_pArmAnalyze->m_OptimizeFlag = true;
	s_pArmAnalyze->m_JumpEndDecode = true;
}

static void cpuShutdown()
{
	ReleaseCBuffer();
	ReleaseCodeBuffer();

	JitLutReset();

	delete s_pArmAnalyze;
	s_pArmAnalyze = NULL;
}

static void cpuReset()
{
	ResetCodeBuffer();
	ResetTcc();

	JitLutReset();
}

static void cpuSync()
{
	armcpu_sync();
}

TEMPLATE static void cpuClear(u32 Addr, u32 Size)
{
	JITLUT_HANDLE(Addr, PROCNUM) = (uintptr_t)NULL;
}

static u32 cpuGetCacheReserve()
{
	return s_CacheReserve / 1024 /1024;
}

static void cpuSetCacheReserve(u32 reserveInMegs)
{
	s_CacheReserve = reserveInMegs * 1024 * 1024;
}

static const char* cpuDescription()
{
	return "Arm CJit";
}

CpuBase arm_cjit =
{
	cpuReserve,

	cpuShutdown,

	cpuReset,

	cpuSync,

	cpuClear<0>, cpuClear<1>,

	cpuExecuteCJIT<0>, cpuExecuteCJIT<1>,

	cpuGetCacheReserve,
	cpuSetCacheReserve,

	cpuDescription
};

template u32 armcpu_compileCJIT<0>();
template u32 armcpu_compileCJIT<1>();

#endif
