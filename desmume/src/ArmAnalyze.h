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

#ifndef ARM_ANALYZE
#define ARM_ANALYZE

#include "common.h"

#define FLAG_N     (1<<3)          // sign (neg)
#define FLAG_Z     (1<<2)          // zero
#define FLAG_C     (1<<1)          // carry
#define FLAG_V     (1<<0)          // overflow
#define ALL_FLAGS  (FLAG_N|FLAG_Z|FLAG_C|FLAG_V)
#define NO_FLAGS 0

enum IROpCode
{
	IR_UND,

	IR_NOP,
	IR_DUMMY,

	IR_T32P1,		//Part1 of THUMB2
	IR_T32P2,		//Part2 of THUMB2

	//Logical Operations
	IR_MOV,			//I=0?REG:IMM, R=0?ShiftIsImm:ShiftIsReg, Typ=ShiftOp
	IR_MVN,			//I=0?REG:IMM, R=0?ShiftIsImm:ShiftIsReg, Typ=ShiftOp
	IR_AND,			//I=0?REG:IMM, R=0?ShiftIsImm:ShiftIsReg, Typ=ShiftOp
	IR_TST,			//I=0?REG:IMM, R=0?ShiftIsImm:ShiftIsReg, Typ=ShiftOp
	IR_EOR,			//I=0?REG:IMM, R=0?ShiftIsImm:ShiftIsReg, Typ=ShiftOp
	IR_TEQ,			//I=0?REG:IMM, R=0?ShiftIsImm:ShiftIsReg, Typ=ShiftOp
	IR_ORR,			//I=0?REG:IMM, R=0?ShiftIsImm:ShiftIsReg, Typ=ShiftOp
	IR_BIC,			//I=0?REG:IMM, R=0?ShiftIsImm:ShiftIsReg, Typ=ShiftOp

	//Arithmetic Operations
	IR_ADD,			//I=0?REG:IMM, R=0?ShiftIsImm:ShiftIsReg, Typ=ShiftOp
	IR_ADC,			//I=0?REG:IMM, R=0?ShiftIsImm:ShiftIsReg, Typ=ShiftOp
	IR_SUB,			//I=0?REG:IMM, R=0?ShiftIsImm:ShiftIsReg, Typ=ShiftOp
	IR_SBC,			//I=0?REG:IMM, R=0?ShiftIsImm:ShiftIsReg, Typ=ShiftOp
	IR_RSB,			//I=0?REG:IMM, R=0?ShiftIsImm:ShiftIsReg, Typ=ShiftOp
	IR_RSC,			//I=0?REG:IMM, R=0?ShiftIsImm:ShiftIsReg, Typ=ShiftOp
	IR_CMP,			//I=0?REG:IMM, R=0?ShiftIsImm:ShiftIsReg, Typ=ShiftOp
	IR_CMN,			//I=0?REG:IMM, R=0?ShiftIsImm:ShiftIsReg, Typ=ShiftOp

	//Multiply
	IR_MUL,
	IR_MLA,
	IR_UMULL,		//RdHi=Rd,RdLo=Rn
	IR_UMLAL,		//RdHi=Rd,RdLo=Rn
	IR_SMULL,		//RdHi=Rd,RdLo=Rn
	IR_SMLAL,		//RdHi=Rd,RdLo=Rn
	IR_SMULxy,		//Rm:X=0?Lower_16bit:Upper_16bit, Rs:Y=0?Lower_16bit:Upper_16bit
	IR_SMLAxy,		//Rm:X=0?Lower_16bit:Upper_16bit, Rs:Y=0?Lower_16bit:Upper_16bit
	IR_SMULWy,		//Rs:Y=0?Lower_16bit:Upper_16bit
	IR_SMLAWy,		//Rs:Y=0?Lower_16bit:Upper_16bit
	IR_SMLALxy,		//Rm:X=0?Lower_16bit:Upper_16bit, Rs:Y=0?Lower_16bit:Upper_16bit;RdHi=Rd,RdLo=Rn

	//Memory Load/Store
	IR_LDR,			//P=0?Post:Pre, U=0?Down:Up, W=0?no_WB:WB, B=0?Word:Byte, //I=0?REG:IMM
	IR_STR,			//P=0?Post:Pre, U=0?Down:Up, W=0?no_WB:WB, B=0?Word:Byte, //I=0?REG:IMM
	IR_LDRx,		//P=0?Post:Pre, U=0?Down:Up, W=0?no_WB:WB, S=0?unsigned:signed, H=0?byte:halfword, I=0?REG:IMM
	IR_STRx,		//P=0?Post:Pre, U=0?Down:Up, W=0?no_WB:WB, S=0?unsigned:signed, H=0?byte:halfword, I=0?REG:IMM
	IR_LDRD,		//I=0?REG:IMM, P=0?Post:Pre, U=0?Down:Up, W=0?no_WB:WB
	IR_STRD,		//I=0?REG:IMM, P=0?Post:Pre, U=0?Down:Up, W=0?no_WB:WB
	IR_LDREX,
	IR_STREX,
	IR_LDM,			//P=0?Post:Pre, U=0?Down:Up, S=0?No:load_PSR_or_force_user_mode, W=0?no_WB:WB
	IR_STM,			//P=0?Post:Pre, U=0?Down:Up, S=0?No:load_PSR_or_force_user_mode, W=0?no_WB:WB
	IR_SWP,

	//Jumps, Calls
	IR_B,			//IMM
	IR_BL,			//IMM
	IR_BX,			//REG
	IR_BLX,			//REG
	IR_SWI,

	//CPSR Mode
	IR_MSR,			//P=0?CPSR:SPSR
	IR_MRS,			//P=0?CPSR:SPSR

	//Coprocessor Functions
	//IR_CDP,
	//IR_STC,
	//IR_LDC,
	IR_MCR,
	IR_MRC,
	//IR_MCRR,
	//IR_MRRC,

	//Others
	IR_CLZ,
	IR_QADD,
	IR_QSUB,
	IR_QDADD,
	IR_QDSUB,

	//UnconditionalExtension
	//IR_PLD,
	IR_BLX_IMM,		//IMM
	//IR_CDP2,
	//IR_LDC2,
	//IR_STC2,
	//IR_MCR2,
	//IR_MRC2,

	//Debug
	IR_BKPT,

	//IR Num
	IR_MAXNUM
};

enum IRShiftOpCode
{
	IRSHIFT_UND,
	IRSHIFT_LSL,
	IRSHIFT_LSR,
	IRSHIFT_ASR,
	IRSHIFT_ROR
};

typedef union _OPCODE {
	u32 ArmOp;
	u16 ThumbOp;
} OPCODE;

#define INVALID_SUBBLOCK 0

typedef struct _Decoded
{
	u16 Block;
	u16 SubBlock;

	u32 ProcessID;
	u32 Address;
	OPCODE Instruction;

	u32 ExecuteCycles;
	u32 VariableCycles:1;

	u32 Cond:4;
	u32 ThumbFlag:1;
	u32 R15Used:1;
	u32 R15Modified:1;
	u32 TbitModified:1;
	u32 Reschedule:2;//1:always reschedule, 2:may reschedule
	u32 InvalidICache:2;//1:clean ICache, 2:clean jit cache
	u32 MayHalt:1;

	u8 FlagsNeeded:4;
	u8 FlagsSet:4;

	IROpCode IROp;

	u32 ReadPCMask;

	u32 OpData;//used in IR_MSR
	u32 Immediate;

	// Registers
	u32 Rd:4;
	u32 Rn:4;
	u32 Rm:4;
	u32 Rs:4;
	u16 RegisterList;

	// Coprocs
	u32 CP:3;
	u32 CRd:4;
	u32 CRm:4;
	u32 CRn:4;
	u32 CPNum:4;
	u32 CPOpc:4;

	u32 I:1;
	u32 S:1;
	//u32 L:1;
	u32 P:1;
	//u32 A:1;
	u32 U:1;
	u32 X:1;
	u32 Y:1;
	u32 B:1;
	u32 W:1;
	u32 H:1;
	//u32 N:1;
	u32 R:1;
	u32 Typ:3;

	// CalcR15
	static u32 CalcR15(const _Decoded &Instruction);
	// 
	static u32 CalcNextInstruction(const _Decoded &Instruction);
}Decoded;

typedef struct _BlockInfo
{
	Decoded *Instructions;
	s32 InstructionsNum;
	s32 R15Num;
	s32 SubBlocks;
}BlockInfo;

struct ArmAnalyze
{
public:
	static std::string DumpInstruction(Decoded *Instructions, s32 InstructionsNum);

public:
	ArmAnalyze(s32 MaxInstructionsNum, s32 MaxBlocksNum = 0);

	~ArmAnalyze();

	bool Decode(struct armcpu_t *armcpu);

	bool CreateBlocks();

	void GetInstructions(Decoded *&Instructions, s32 &InstructionsNum);
	
	void GetBlocks(BlockInfo *&BlockInfos, s32 &BlocksNum);

protected:
	s32 Optimize(Decoded *Instructions, s32 InstructionsNum);

	u32 OptimizeFlag(Decoded *Instructions, s32 InstructionsNum);

	s32 CreateSubBlocks(Decoded *Instructions, s32 InstructionsNum);

public:
	bool m_Optimize;
	bool m_OptimizeFlag;
	bool m_MergeSubBlocks;
	bool m_JumpEndDecode;

protected:
	Decoded *m_Instructions;
	s32 m_MaxInstructionsNum;
	s32 m_InstructionsNum;

	BlockInfo *m_BlockInfos;
	s32 m_MaxBlocksNum;
	s32 m_BlocksNum;
};

#endif