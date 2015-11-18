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
#include <stddef.h>
#import <string>

#include "ArmLJit.h"
#include "ArmAnalyze.h"
#include "instructions.h"
#include "Disassembler.h"

#include "utils/lightning/lightning.h"
#if defined(__arm__) || defined(__arm64__)
#import "utils/lightning/arm/funcs.h"
#import "utils/lightning/arm/core.h"
#else
#import "utils/lightning/i386/funcs.h"
#import "utils/lightning/i386/core.h"
#endif

#include "armcpu.h"
#include "MMU.h"
#include "MMU_timing.h"
#include "JitCommon.h"
#include "utils/MemBuffer.h"

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <sys/mman.h>

#ifdef _MSC_VER
#include <intrin.h>
#endif

#ifdef HAVE_JIT

#define GETCPUPTR (&ARMPROC)
#define GETCPU (ARMPROC)

#define TEMPLATE template<u32 PROCNUM> 
#define OPDECODER_DECL(name) void FASTCALL name##_Decoder(const Decoded &d, RegisterMap &regMap)

typedef void (FASTCALL* IROpDecoder)(const Decoded &d, RegisterMap &regMap);
typedef u32 (* ArmOpCompiled)();

typedef u32 (* MemOp1)(u32, u32*);
typedef u32 (* MemOp2)(u32, u32);
typedef u32 (* MemOp3)(u32, u32, u32*);
typedef u32 (* MemOp4)(u32, u32*, u32);

#define HWORD(i)   ((s32)(((s32)(i))>>16))
#define LWORD(i)   (s32)(((s32)((i)<<16))>>16)

#define DEBUG_BKPT	_O(0xcc);

namespace ArmLJit
{
//------------------------------------------------------------
//                         RegisterMap Imp
//------------------------------------------------------------
	enum LocalRegType
	{
		LRT_R,
		LRT_V,
	};

	static jit_gpr_t LocalMap[JIT_V_NUM + JIT_R_NUM];

	FORCEINLINE u32 LOCALREG_INIT()
	{
		for (u32 i = 0; i < JIT_V_NUM; i++)
			LocalMap[i] = JIT_V(i);

		for (u32 i = 0; i < JIT_R_NUM; i++)
			LocalMap[i + JIT_V_NUM] = JIT_R(i);

		return JIT_V_NUM + JIT_R_NUM;
	}

	FORCEINLINE jit_gpr_t LOCALREG(u32 i)
	{
		return LocalMap[i];
	}

	FORCEINLINE LocalRegType LOCALREGTYPE(u32 i)
	{
		if (i < JIT_V_NUM)
			return LRT_V;

		return LRT_R;
	}

	FORCEINLINE RegisterMap::GuestRegId REGID(u32 i)
	{
		return (RegisterMap::GuestRegId)(i);
	}

	class RegisterMapImp : public RegisterMap
	{
		public:
			RegisterMapImp(u32 HostRegCount);

			void CallABI(void* funptr, 
						const std::vector<ABIOp> &args, 
						const std::vector<GuestRegId> &flushs, 
						u32 hostreg_ret, 
						ImmData::Type type_ret);

		protected:
			void StartBlock();
			void EndBlock();

			void StoreGuestReg(u32 hostreg, GuestRegId guestreg);
			void LoadGuestReg(u32 hostreg, GuestRegId guestreg);

			void StoreImm(GuestRegId guestreg, const ImmData &data);
			void LoadImm(u32 hostreg, const ImmData &data);

			bool IsPerdureHostReg(u32 hostreg);

		private:
			int m_StackExecyc;
			int m_StackTemp;
	};

	RegisterMapImp::RegisterMapImp(u32 HostRegCount)
		: RegisterMap(HostRegCount)
	{
	}

	void RegisterMapImp::CallABI(void* funptr, 
								const std::vector<ABIOp> &args, 
								const std::vector<GuestRegId> &flushs, 
								u32 hostreg_ret, 
								ImmData::Type type_ret)
	{
		jit_prepare(args.size());

		for (std::vector<ABIOp>::const_reverse_iterator itr = args.rbegin(); 
			itr != args.rend(); itr++)
		{
			const ABIOp &Op = *itr;

			switch (Op.type)
			{
			case ABIOp::IMM:
				{
					u32 tmp = AllocTempReg();

					switch (Op.immdata.type)
					{
					case ImmData::IMM8:
						jit_movi_ui(LOCALREG(tmp), Op.immdata.imm8);
						jit_pusharg_uc(LOCALREG(tmp));
						break;
					case ImmData::IMM16:
						jit_movi_ui(LOCALREG(tmp), Op.immdata.imm16);
						jit_pusharg_us(LOCALREG(tmp));
						break;
					case ImmData::IMM32:
						jit_movi_ui(LOCALREG(tmp), Op.immdata.imm32);
						jit_pusharg_ui(LOCALREG(tmp));
						break;
					case ImmData::IMMPTR:
						jit_movi_p(LOCALREG(tmp), Op.immdata.immptr);
						jit_pusharg_p(LOCALREG(tmp));
						break;

					default:
						break;
					}

					ReleaseTempReg(tmp);
				}
				break;

			case ABIOp::GUSETREG:
				{
					u32 reg = MapReg(REGID(Op.regdata));
					Lock(reg);

					jit_pusharg_ui(LOCALREG(reg));

					Unlock(reg);
				}
				break;

			case ABIOp::HOSTREG:
				jit_pusharg_ui(LOCALREG(Op.regdata));
				break;

			case ABIOp::TEMPREG:
				{
					u32 tmp = Op.regdata;

					jit_pusharg_ui(LOCALREG(tmp));
					ReleaseTempReg(tmp);
				}
				break;

			case ABIOp::GUSETREGPTR:
				{
					u32 cpuptr = GetCpuPtrReg();
					u32 tmp = AllocTempReg();

					if (REGID(Op.regdata) >= R0 && REGID(Op.regdata) <= R15)
						jit_addi_p(LOCALREG(tmp), LOCALREG(cpuptr), jit_field(armcpu_t, R[Op.regdata]));
					else if (REGID(Op.regdata) == CPSR)
						jit_addi_p(LOCALREG(tmp), LOCALREG(cpuptr), jit_field(armcpu_t, CPSR));
					else if (REGID(Op.regdata) == SPSR)
						jit_addi_p(LOCALREG(tmp), LOCALREG(cpuptr), jit_field(armcpu_t, SPSR));
					jit_pusharg_p(LOCALREG(tmp));

					ReleaseTempReg(tmp);
				}
				break;

			default:
				break;
			}
		}

		for (size_t i = 0; i < flushs.size(); i++)
		{
			FlushGuestReg(flushs[i]);
		}

		for (u32 i = 0; i < m_HostRegCount; i++)
		{
			if (m_State.HostRegs[i].alloced && 
				m_State.HostRegs[i].guestreg != INVALID_REG_ID && 
				!IsPerdureHostReg(i))
				FlushHostReg(i);
		}

		jit_finish(funptr);

		if (hostreg_ret != INVALID_REG_ID)
		{
			switch (type_ret)
			{
			case ImmData::IMM8:
				jit_retval_uc(LOCALREG(hostreg_ret));
				break;
			case ImmData::IMM16:
				jit_retval_us(LOCALREG(hostreg_ret));
				break;
			case ImmData::IMM32:
				jit_retval_ui(LOCALREG(hostreg_ret));
				break;
			case ImmData::IMMPTR:
				jit_retval_p(LOCALREG(hostreg_ret));
				break;

			default:
				break;
			}
		}

		m_Profile.CallABICount++;
	}

	void RegisterMapImp::StartBlock()
	{
		jit_prolog(0);

		m_StackExecyc = jit_allocai(sizeof(u32));
		m_StackTemp = jit_allocai(sizeof(u32));

		SetImm32(EXECUTECYCLES, 0);

		jit_movi_p(LOCALREG(GetCpuPtrReg()), m_Cpu);
	}

	void RegisterMapImp::EndBlock()
	{
		if (IsImm(EXECUTECYCLES))
		{
			jit_movi_ui(JIT_RET, GetImm32(EXECUTECYCLES));
			jit_ret();
		}
		else
		{
			u32 execyc = MapReg(EXECUTECYCLES);
			Lock(execyc);

			jit_movr_ui(JIT_RET, LOCALREG(execyc));
			jit_ret();

			Unlock(execyc);
		}
	}

	void RegisterMapImp::StoreGuestReg(u32 hostreg, GuestRegId guestreg)
	{
		if (guestreg >= R0 && guestreg <= SPSR)
		{
			u32 cpuptr = GetCpuPtrReg();

			if (guestreg >= R0 && guestreg <= R15)
				jit_stxi_ui(jit_field(armcpu_t, R[guestreg]), LOCALREG(cpuptr), LOCALREG(hostreg));
			else if (guestreg == CPSR)
				jit_stxi_ui(jit_field(armcpu_t, CPSR), LOCALREG(cpuptr), LOCALREG(hostreg));
			else if (guestreg == SPSR)
				jit_stxi_ui(jit_field(armcpu_t, SPSR), LOCALREG(cpuptr), LOCALREG(hostreg));
		}
		else if (guestreg == EXECUTECYCLES)
		{
			jit_stxi_ui(m_StackExecyc, JIT_FP, LOCALREG(hostreg));
		}

		m_Profile.StoreRegCount++;
	}

	void RegisterMapImp::LoadGuestReg(u32 hostreg, GuestRegId guestreg)
	{
		if (guestreg >= R0 && guestreg <= SPSR)
		{
			Lock(hostreg);

			u32 cpuptr = GetCpuPtrReg();

			if (guestreg >= R0 && guestreg <= R15)
				jit_ldxi_ui(LOCALREG(hostreg), LOCALREG(cpuptr), jit_field(armcpu_t, R[guestreg]));
			else if (guestreg == CPSR)
				jit_ldxi_ui(LOCALREG(hostreg), LOCALREG(cpuptr), jit_field(armcpu_t, CPSR));
			else if (guestreg == SPSR)
				jit_ldxi_ui(LOCALREG(hostreg), LOCALREG(cpuptr), jit_field(armcpu_t, SPSR));

			Unlock(hostreg);
		}
		else if (guestreg == EXECUTECYCLES)
		{
			jit_ldxi_ui(LOCALREG(hostreg), JIT_FP, m_StackExecyc);
		}

		m_Profile.LoadRegCount++;
	}

	void RegisterMapImp::StoreImm(GuestRegId guestreg, const ImmData &data)
	{
		u32 regmode = 0; //0:freereg 1:tempreg 2:backupreg
		u32 tmp = FindFreeHostReg();

		if (tmp == INVALID_REG_ID)
		{
			if (!m_IsInMerge)
			{
				tmp = AllocTempReg();

				regmode = 1;
			}
			else
			{
				tmp = FindFirstHostReg();

				jit_stxi_ui(m_StackTemp, JIT_FP, LOCALREG(tmp));

				regmode = 2;

				m_Profile.BackupRegCount++;
			}
		}

		switch (data.type)
		{
		case ImmData::IMM8:
			jit_movi_ui(LOCALREG(tmp), data.imm8);
			break;
		case ImmData::IMM16:
			jit_movi_ui(LOCALREG(tmp), data.imm16);
			break;
		case ImmData::IMM32:
			jit_movi_ui(LOCALREG(tmp), data.imm32);
			break;
		case ImmData::IMMPTR:
			jit_movi_p(LOCALREG(tmp), data.immptr);
			break;

		default:
			break;
		}
		StoreGuestReg(tmp, guestreg);

		if (regmode == 1)
			ReleaseTempReg(tmp);
		else if (regmode == 2)
			jit_ldxi_ui(LOCALREG(tmp), JIT_FP, m_StackTemp);
	}

	void RegisterMapImp::LoadImm(u32 hostreg, const ImmData &data)
	{
		switch (data.type)
		{
		case ImmData::IMM8:
			jit_movi_ui(LOCALREG(hostreg), data.imm8);
			break;
		case ImmData::IMM16:
			jit_movi_ui(LOCALREG(hostreg), data.imm16);
			break;
		case ImmData::IMM32:
			jit_movi_ui(LOCALREG(hostreg), data.imm32);
			break;
		case ImmData::IMMPTR:
			jit_movi_p(LOCALREG(hostreg), data.immptr);
			break;

		default:
			break;
		}
	}

	bool RegisterMapImp::IsPerdureHostReg(u32 hostreg)
	{
		return LOCALREGTYPE(hostreg) == LRT_V;
	}

//------------------------------------------------------------
//                         Memory function
//------------------------------------------------------------
	enum {
		MEMTYPE_GENERIC = 0,	// no assumptions
		MEMTYPE_MAIN = 1,		// arm9:r/w arm7:r/w
		MEMTYPE_DTCM_ARM9 = 2,	// arm9:r/w
		MEMTYPE_ERAM_ARM7 = 3,	// arm7:r/w
		MEMTYPE_SWIRAM = 4,		// arm9:r/w arm7:r/w

		MEMTYPE_COUNT,
	};

	template<u32 PROCNUM, u32 memtype>
	static u32 MEMOP_LDR(u32 adr, u32 *dstreg)
	{
		u32 data = READ32(cpu->mem_if->data, adr);
		if(adr&3)
			data = ROR(data, 8*(adr&3));
		*dstreg = data;
		return MMU_aluMemAccessCycles<PROCNUM,32,MMU_AD_READ>(3,adr);
	}

	static const MemOp1 LDR_Tab[2][MEMTYPE_COUNT] = 
	{
		{
			MEMOP_LDR<0,MEMTYPE_GENERIC>,
			MEMOP_LDR<0,MEMTYPE_MAIN>,
			MEMOP_LDR<0,MEMTYPE_DTCM_ARM9>,
			MEMOP_LDR<0,MEMTYPE_GENERIC>,//MEMOP_LDR<0,MEMTYPE_ERAM_ARM7>,
			MEMOP_LDR<0,MEMTYPE_SWIRAM>,
		},
		{
			MEMOP_LDR<1,MEMTYPE_GENERIC>,
			MEMOP_LDR<1,MEMTYPE_MAIN>,
			MEMOP_LDR<1,MEMTYPE_GENERIC>,//MEMOP_LDR<1,MEMTYPE_DTCM_ARM9>,
			MEMOP_LDR<1,MEMTYPE_ERAM_ARM7>,
			MEMOP_LDR<1,MEMTYPE_SWIRAM>,
		}
	};

	template<u32 PROCNUM, u32 memtype>
	static u32 MEMOP_LDRB(u32 adr, u32 *dstreg)
	{
		*dstreg = READ8(cpu->mem_if->data, adr);
		return MMU_aluMemAccessCycles<PROCNUM,8,MMU_AD_READ>(3,adr);
	}

	static const MemOp1 LDRB_Tab[2][MEMTYPE_COUNT] = 
	{
		{
			MEMOP_LDRB<0,MEMTYPE_GENERIC>,
			MEMOP_LDRB<0,MEMTYPE_MAIN>,
			MEMOP_LDRB<0,MEMTYPE_DTCM_ARM9>,
			MEMOP_LDRB<0,MEMTYPE_GENERIC>,//MEMOP_LDRB<0,MEMTYPE_ERAM_ARM7>,
			MEMOP_LDRB<0,MEMTYPE_SWIRAM>,
		},
		{
			MEMOP_LDRB<1,MEMTYPE_GENERIC>,
			MEMOP_LDRB<1,MEMTYPE_MAIN>,
			MEMOP_LDRB<1,MEMTYPE_GENERIC>,//MEMOP_LDRB<1,MEMTYPE_DTCM_ARM9>,
			MEMOP_LDRB<1,MEMTYPE_ERAM_ARM7>,
			MEMOP_LDRB<1,MEMTYPE_SWIRAM>,
		}
	};

	template<u32 PROCNUM, u32 memtype>
	static u32 MEMOP_LDRH(u32 adr, u32 *dstreg)
	{
		*dstreg = READ16(cpu->mem_if->data, adr);
		return MMU_aluMemAccessCycles<PROCNUM,16,MMU_AD_READ>(3,adr);
	}

	static const MemOp1 LDRH_Tab[2][MEMTYPE_COUNT] = 
	{
		{
			MEMOP_LDRH<0,MEMTYPE_GENERIC>,
			MEMOP_LDRH<0,MEMTYPE_MAIN>,
			MEMOP_LDRH<0,MEMTYPE_DTCM_ARM9>,
			MEMOP_LDRH<0,MEMTYPE_GENERIC>,//MEMOP_LDRH<0,MEMTYPE_ERAM_ARM7>,
			MEMOP_LDRH<0,MEMTYPE_SWIRAM>,
		},
		{
			MEMOP_LDRH<1,MEMTYPE_GENERIC>,
			MEMOP_LDRH<1,MEMTYPE_MAIN>,
			MEMOP_LDRH<1,MEMTYPE_GENERIC>,//MEMOP_LDRH<1,MEMTYPE_DTCM_ARM9>,
			MEMOP_LDRH<1,MEMTYPE_ERAM_ARM7>,
			MEMOP_LDRH<1,MEMTYPE_SWIRAM>,
		}
	};

	template<u32 PROCNUM, u32 memtype>
	static u32 MEMOP_LDRSH(u32 adr, u32 *dstreg)
	{
		*dstreg = (s16)READ16(cpu->mem_if->data, adr);
		return MMU_aluMemAccessCycles<PROCNUM,16,MMU_AD_READ>(3,adr);
	}

	static const MemOp1 LDRSH_Tab[2][MEMTYPE_COUNT] = 
	{
		{
			MEMOP_LDRSH<0,MEMTYPE_GENERIC>,
			MEMOP_LDRSH<0,MEMTYPE_MAIN>,
			MEMOP_LDRSH<0,MEMTYPE_DTCM_ARM9>,
			MEMOP_LDRSH<0,MEMTYPE_GENERIC>,//MEMOP_LDRSH<0,MEMTYPE_ERAM_ARM7>,
			MEMOP_LDRSH<0,MEMTYPE_SWIRAM>,
		},
		{
			MEMOP_LDRSH<1,MEMTYPE_GENERIC>,
			MEMOP_LDRSH<1,MEMTYPE_MAIN>,
			MEMOP_LDRSH<1,MEMTYPE_GENERIC>,//MEMOP_LDRSH<1,MEMTYPE_DTCM_ARM9>,
			MEMOP_LDRSH<1,MEMTYPE_ERAM_ARM7>,
			MEMOP_LDRSH<1,MEMTYPE_SWIRAM>,
		}
	};

	template<u32 PROCNUM, u32 memtype>
	static u32 MEMOP_LDRSB(u32 adr, u32 *dstreg)
	{
		*dstreg = (s8)READ8(cpu->mem_if->data, adr);
		return MMU_aluMemAccessCycles<PROCNUM,8,MMU_AD_READ>(3,adr);
	}

	static const MemOp1 LDRSB_Tab[2][MEMTYPE_COUNT] = 
	{
		{
			MEMOP_LDRSB<0,MEMTYPE_GENERIC>,
			MEMOP_LDRSB<0,MEMTYPE_MAIN>,
			MEMOP_LDRSB<0,MEMTYPE_DTCM_ARM9>,
			MEMOP_LDRSB<0,MEMTYPE_GENERIC>,//MEMOP_LDRSB<0,MEMTYPE_ERAM_ARM7>,
			MEMOP_LDRSB<0,MEMTYPE_SWIRAM>,
		},
		{
			MEMOP_LDRSB<1,MEMTYPE_GENERIC>,
			MEMOP_LDRSB<1,MEMTYPE_MAIN>,
			MEMOP_LDRSB<1,MEMTYPE_GENERIC>,//MEMOP_LDRSB<1,MEMTYPE_DTCM_ARM9>,
			MEMOP_LDRSB<1,MEMTYPE_ERAM_ARM7>,
			MEMOP_LDRSB<1,MEMTYPE_SWIRAM>,
		}
	};

	template<u32 PROCNUM, u32 memtype>
	static u32 MEMOP_STRB(u32 adr, u32 data)
	{
		WRITE8(cpu->mem_if->data, adr, data);
		return MMU_aluMemAccessCycles<PROCNUM,8,MMU_AD_WRITE>(2,adr);
	}

	static const MemOp2 STRB_Tab[2][MEMTYPE_COUNT] = 
	{
		{
			MEMOP_STRB<0,MEMTYPE_GENERIC>,
			MEMOP_STRB<0,MEMTYPE_MAIN>,
			MEMOP_STRB<0,MEMTYPE_DTCM_ARM9>,
			MEMOP_STRB<0,MEMTYPE_GENERIC>,//MEMOP_STRB<0,MEMTYPE_ERAM_ARM7>,
			MEMOP_STRB<0,MEMTYPE_SWIRAM>,
		},
		{
			MEMOP_STRB<1,MEMTYPE_GENERIC>,
			MEMOP_STRB<1,MEMTYPE_MAIN>,
			MEMOP_STRB<1,MEMTYPE_GENERIC>,//MEMOP_STRB<1,MEMTYPE_DTCM_ARM9>,
			MEMOP_STRB<1,MEMTYPE_ERAM_ARM7>,
			MEMOP_STRB<1,MEMTYPE_SWIRAM>,
		}
	};

	template<u32 PROCNUM, u32 memtype>
	static u32 MEMOP_STR(u32 adr, u32 data)
	{
		WRITE32(cpu->mem_if->data, adr, data);
		return MMU_aluMemAccessCycles<PROCNUM,32,MMU_AD_WRITE>(2,adr);
	}

	static const MemOp2 STR_Tab[2][MEMTYPE_COUNT] = 
	{
		{
			MEMOP_STR<0,MEMTYPE_GENERIC>,
			MEMOP_STR<0,MEMTYPE_MAIN>,
			MEMOP_STR<0,MEMTYPE_DTCM_ARM9>,
			MEMOP_STR<0,MEMTYPE_GENERIC>,//MEMOP_STR<0,MEMTYPE_ERAM_ARM7>,
			MEMOP_STR<0,MEMTYPE_SWIRAM>,
		},
		{
			MEMOP_STR<1,MEMTYPE_GENERIC>,
			MEMOP_STR<1,MEMTYPE_MAIN>,
			MEMOP_STR<1,MEMTYPE_GENERIC>,//MEMOP_STR<1,MEMTYPE_DTCM_ARM9>,
			MEMOP_STR<1,MEMTYPE_ERAM_ARM7>,
			MEMOP_STR<1,MEMTYPE_SWIRAM>,
		}
	};

	template<u32 PROCNUM, u32 memtype>
	static u32 MEMOP_STRH(u32 adr, u32 data)
	{
		WRITE16(cpu->mem_if->data, adr, data);
		return MMU_aluMemAccessCycles<PROCNUM,16,MMU_AD_WRITE>(2,adr);
	}

	static const MemOp2 STRH_Tab[2][MEMTYPE_COUNT] = 
	{
		{
			MEMOP_STRH<0,MEMTYPE_GENERIC>,
			MEMOP_STRH<0,MEMTYPE_MAIN>,
			MEMOP_STRH<0,MEMTYPE_DTCM_ARM9>,
			MEMOP_STRH<0,MEMTYPE_GENERIC>,//MEMOP_STRH<0,MEMTYPE_ERAM_ARM7>,
			MEMOP_STRH<0,MEMTYPE_SWIRAM>,
		},
		{
			MEMOP_STRH<1,MEMTYPE_GENERIC>,
			MEMOP_STRH<1,MEMTYPE_MAIN>,
			MEMOP_STRH<1,MEMTYPE_GENERIC>,//MEMOP_STRH<1,MEMTYPE_DTCM_ARM9>,
			MEMOP_STRH<1,MEMTYPE_ERAM_ARM7>,
			MEMOP_STRH<1,MEMTYPE_SWIRAM>,
		}
	};
//------------------------------------------------------------
//                         Help function
//------------------------------------------------------------
#ifdef WORDS_BIGENDIAN
	static const u32 PSR_MODE_BITSHIFT = 27;
	static const u32 PSR_MODE_BITMASK = 0xF8000000;
	static const u32 PSR_T_BITSHIFT = 26;
	static const u32 PSR_T_BITMASK = 1 << PSR_T_BITSHIFT;
	static const u32 PSR_F_BITSHIFT = 25;
	static const u32 PSR_F_BITMASK = 1 << PSR_F_BITSHIFT;
	static const u32 PSR_I_BITSHIFT = 24;
	static const u32 PSR_I_BITMASK = 1 << PSR_I_BITSHIFT;
	static const u32 PSR_Q_BITSHIFT = 4;
	static const u32 PSR_Q_BITMASK = 1 << PSR_Q_BITSHIFT;
	static const u32 PSR_V_BITSHIFT = 3;
	static const u32 PSR_V_BITMASK = 1 << PSR_V_BITSHIFT;
	static const u32 PSR_C_BITSHIFT = 2;
	static const u32 PSR_C_BITMASK = 1 << PSR_C_BITSHIFT;
	static const u32 PSR_Z_BITSHIFT = 1;
	static const u32 PSR_Z_BITMASK = 1 << PSR_Z_BITSHIFT;
	static const u32 PSR_N_BITSHIFT = 0;
	static const u32 PSR_N_BITMASK = 1 << PSR_N_BITSHIFT;
#else
	static const u32 PSR_MODE_BITSHIFT = 0;
	static const u32 PSR_MODE_BITMASK = 31;
	static const u32 PSR_T_BITSHIFT = 5;
	static const u32 PSR_T_BITMASK = 1 << PSR_T_BITSHIFT;
	static const u32 PSR_F_BITSHIFT = 6;
	static const u32 PSR_F_BITMASK = 1 << PSR_F_BITSHIFT;
	static const u32 PSR_I_BITSHIFT = 7;
	static const u32 PSR_I_BITMASK = 1 << PSR_I_BITSHIFT;
	static const u32 PSR_Q_BITSHIFT = 27;
	static const u32 PSR_Q_BITMASK = 1 << PSR_Q_BITSHIFT;
	static const u32 PSR_V_BITSHIFT = 28;
	static const u32 PSR_V_BITMASK = 1 << PSR_V_BITSHIFT;
	static const u32 PSR_C_BITSHIFT = 29;
	static const u32 PSR_C_BITMASK = 1 << PSR_C_BITSHIFT;
	static const u32 PSR_Z_BITSHIFT = 30;
	static const u32 PSR_Z_BITMASK = 1 << PSR_Z_BITSHIFT;
	static const u32 PSR_N_BITSHIFT = 31;
	static const u32 PSR_N_BITMASK = 1 << PSR_N_BITSHIFT;
#endif

	enum ARMCPU_PSR
	{
		PSR_MODE,
		PSR_T,
		PSR_F,
		PSR_I,
		PSR_Q,
		PSR_V,
		PSR_C,
		PSR_Z,
		PSR_N,
	};

	void FASTCALL UnpackPSR(ARMCPU_PSR flg, u32 in, u32 out)
	{
		u32 shift, mask;

		switch (flg)
		{
		case PSR_MODE:
			shift = PSR_MODE_BITSHIFT;
			mask = PSR_MODE_BITMASK;
			break;
		case PSR_T:
			shift = PSR_T_BITSHIFT;
			mask = PSR_T_BITMASK;
			break;
		case PSR_F:
			shift = PSR_F_BITSHIFT;
			mask = PSR_F_BITMASK;
			break;
		case PSR_I:
			shift = PSR_I_BITSHIFT;
			mask = PSR_I_BITMASK;
			break;
		case PSR_Q:
			shift = PSR_Q_BITSHIFT;
			mask = PSR_Q_BITMASK;
			break;
		case PSR_V:
			shift = PSR_V_BITSHIFT;
			mask = PSR_V_BITMASK;
			break;
		case PSR_C:
			shift = PSR_C_BITSHIFT;
			mask = PSR_C_BITMASK;
			break;
		case PSR_Z:
			shift = PSR_Z_BITSHIFT;
			mask = PSR_Z_BITMASK;
			break;
		case PSR_N:
			shift = PSR_N_BITSHIFT;
			mask = PSR_N_BITMASK;
			break;

		default:
			shift = 0;
			mask = 0;
			break;
		}

		if (mask == (1 << 31) && shift == 31)
			jit_rshi_ui(LOCALREG(out), LOCALREG(in), shift);
		else
		{
			jit_andi_ui(LOCALREG(out), LOCALREG(in), mask);
			if (shift != 0)
				jit_rshi_ui(LOCALREG(out), LOCALREG(out), shift);
		}
	}

	void FASTCALL UnpackCPSR(RegisterMap &regMap, ARMCPU_PSR flg, u32 out)
	{
		u32 cpsr = regMap.MapReg(RegisterMap::CPSR);
		regMap.Lock(cpsr);

		UnpackPSR(flg, cpsr, out);

		regMap.Unlock(cpsr);
	}

	void FASTCALL PackCPSR(RegisterMap &regMap, ARMCPU_PSR flg, u32 in)
	{
		u32 shift, mask;

		switch (flg)
		{
		case PSR_MODE:
			shift = PSR_MODE_BITSHIFT;
			mask = PSR_MODE_BITMASK;
			break;
		case PSR_T:
			shift = PSR_T_BITSHIFT;
			mask = PSR_T_BITMASK;
			break;
		case PSR_F:
			shift = PSR_F_BITSHIFT;
			mask = PSR_F_BITMASK;
			break;
		case PSR_I:
			shift = PSR_I_BITSHIFT;
			mask = PSR_I_BITMASK;
			break;
		case PSR_Q:
			shift = PSR_Q_BITSHIFT;
			mask = PSR_Q_BITMASK;
			break;
		case PSR_V:
			shift = PSR_V_BITSHIFT;
			mask = PSR_V_BITMASK;
			break;
		case PSR_C:
			shift = PSR_C_BITSHIFT;
			mask = PSR_C_BITMASK;
			break;
		case PSR_Z:
			shift = PSR_Z_BITSHIFT;
			mask = PSR_Z_BITMASK;
			break;
		case PSR_N:
			shift = PSR_N_BITSHIFT;
			mask = PSR_N_BITMASK;
			break;

		default:
			shift = 0;
			mask = 0;
			break;
		}

		u32 cpsr = regMap.MapReg(RegisterMap::CPSR, RegisterMap::MAP_DIRTY);
		regMap.Lock(cpsr);

		jit_andi_ui(LOCALREG(cpsr), LOCALREG(cpsr), ~mask);
		if (shift == 0)
			jit_orr_ui(LOCALREG(cpsr), LOCALREG(cpsr), LOCALREG(in));
		else
		{
			jit_lshi_ui(LOCALREG(in), LOCALREG(in), shift);
			jit_orr_ui(LOCALREG(cpsr), LOCALREG(cpsr), LOCALREG(in));
		}

		regMap.Unlock(cpsr);
	}

	void FASTCALL PackCPSRImm(RegisterMap &regMap, ARMCPU_PSR flg, u32 in)
	{
		u32 shift, mask;

		switch (flg)
		{
		case PSR_MODE:
			shift = PSR_MODE_BITSHIFT;
			mask = PSR_MODE_BITMASK;
			break;
		case PSR_T:
			shift = PSR_T_BITSHIFT;
			mask = PSR_T_BITMASK;
			break;
		case PSR_F:
			shift = PSR_F_BITSHIFT;
			mask = PSR_F_BITMASK;
			break;
		case PSR_I:
			shift = PSR_I_BITSHIFT;
			mask = PSR_I_BITMASK;
			break;
		case PSR_Q:
			shift = PSR_Q_BITSHIFT;
			mask = PSR_Q_BITMASK;
			break;
		case PSR_V:
			shift = PSR_V_BITSHIFT;
			mask = PSR_V_BITMASK;
			break;
		case PSR_C:
			shift = PSR_C_BITSHIFT;
			mask = PSR_C_BITMASK;
			break;
		case PSR_Z:
			shift = PSR_Z_BITSHIFT;
			mask = PSR_Z_BITMASK;
			break;
		case PSR_N:
			shift = PSR_N_BITSHIFT;
			mask = PSR_N_BITMASK;
			break;

		default:
			shift = 0;
			mask = 0;
			break;
		}

		if (flg != PSR_MODE)
			in = !!in;

		u32 cpsr = regMap.MapReg(RegisterMap::CPSR, RegisterMap::MAP_DIRTY);
		regMap.Lock(cpsr);

		if (in)
			jit_ori_ui(LOCALREG(cpsr), LOCALREG(cpsr), in << shift);
		else
			jit_andi_ui(LOCALREG(cpsr), LOCALREG(cpsr), ~mask);

		regMap.Unlock(cpsr);
	}

	struct ShiftOut
	{
		u32 shiftop;
		u32 cflg;
		bool shiftopimm;
		bool cflgimm;

		ShiftOut()
			: shiftop(INVALID_REG_ID)
			, cflg(INVALID_REG_ID)
			, shiftopimm(false)
			, cflgimm(false)
		{
		}

		void CleanShiftOp(RegisterMap &regMap)
		{
			if (!shiftopimm && shiftop != INVALID_REG_ID)
				regMap.ReleaseTempReg(shiftop);
		}

		void CleanCflg(RegisterMap &regMap)
		{
			if (!cflgimm && cflg != INVALID_REG_ID)
				regMap.ReleaseTempReg(cflg);
		}

		void Clean(RegisterMap &regMap)
		{
			CleanShiftOp(regMap);
			CleanCflg(regMap);
		}
	};

	ShiftOut FASTCALL IRShiftOpGenerate(const Decoded &d, RegisterMap &regMap, bool clacCarry)
	{
		u32 PROCNUM = d.ProcessID;

		ShiftOut Out;

		switch (d.Typ)
		{
		case IRSHIFT_LSL:
			if (!d.R)
			{
				if (clacCarry)
				{
					if (d.Immediate == 0)
					{
						Out.cflg = regMap.AllocTempReg();

						UnpackCPSR(regMap, PSR_C, Out.cflg);
					}
					else
					{
						if (regMap.IsImm(REGID(d.Rm)))
						{
							Out.cflg = BIT_N(regMap.GetImm32(REGID(d.Rm)), (32-d.Immediate));
							Out.cflgimm = true;
						}
						else
						{
							Out.cflg = regMap.AllocTempReg();

							u32 rm = regMap.MapReg(REGID(d.Rm));
							regMap.Lock(rm);

							jit_rshi_ui(LOCALREG(Out.cflg), LOCALREG(rm), 32-d.Immediate);
							jit_andi_ui(LOCALREG(Out.cflg), LOCALREG(Out.cflg), 1);

							regMap.Unlock(rm);
						}
					}
				}

				if (d.Immediate == 0)
				{
					if (regMap.IsImm(REGID(d.Rm)))
					{
						Out.shiftop = regMap.GetImm32(REGID(d.Rm));
						Out.shiftopimm = true;
					}
					else
					{
						Out.shiftop = regMap.AllocTempReg();

						u32 rm = regMap.MapReg(REGID(d.Rm));
						regMap.Lock(rm);

						jit_movr_ui(LOCALREG(Out.shiftop), LOCALREG(rm));

						regMap.Unlock(rm);
					}
				}
				else
				{
					if (regMap.IsImm(REGID(d.Rm)))
					{
						Out.shiftop = regMap.GetImm32(REGID(d.Rm))<<d.Immediate;
						Out.shiftopimm = true;
					}
					else
					{
						Out.shiftop = regMap.AllocTempReg();

						u32 rm = regMap.MapReg(REGID(d.Rm));
						regMap.Lock(rm);

						jit_lshi_ui(LOCALREG(Out.shiftop), LOCALREG(rm), d.Immediate);

						regMap.Unlock(rm);
					}
				}
			}
			else
			{
				if (clacCarry)
				{
					{
						Out.cflg = regMap.AllocTempReg();

						UnpackCPSR(regMap, PSR_C, Out.cflg);
					}

					{
						u32 rs = regMap.MapReg(REGID(d.Rs));
						regMap.Lock(rs);

						Out.shiftop = regMap.AllocTempReg();

						jit_andi_ui(LOCALREG(Out.shiftop), LOCALREG(rs), 0xFF);

						regMap.Unlock(rs);
					}

					{
						u32 rm = regMap.MapReg(REGID(d.Rm));
						regMap.Lock(rm);
					
						jit_insn *eq0 = jit_beqi_ui(jit_forward(), LOCALREG(Out.shiftop), 0);
						jit_insn *lt32 = jit_blti_ui(jit_forward(), LOCALREG(Out.shiftop), 32);
						jit_insn *eq32 = jit_beqi_ui(jit_forward(), LOCALREG(Out.shiftop), 32);
						jit_movi_ui(LOCALREG(Out.shiftop), 0);
						jit_movi_ui(LOCALREG(Out.cflg), 0);
						jit_insn *done1 = jit_jmpi(jit_forward());
						jit_patch(eq32);
						jit_movi_ui(LOCALREG(Out.shiftop), 0);
						jit_andi_ui(LOCALREG(Out.cflg), LOCALREG(rm), 1);
						jit_insn *done2 = jit_jmpi(jit_forward());
						jit_patch(eq0);
						jit_movr_ui(LOCALREG(Out.shiftop), LOCALREG(rm));
						jit_insn *done3 = jit_jmpi(jit_forward());
						jit_patch(lt32);
						jit_rsbi_ui(LOCALREG(Out.cflg), LOCALREG(Out.shiftop), 32);
						jit_rshr_ui(LOCALREG(Out.cflg), LOCALREG(rm), LOCALREG(Out.cflg));
						jit_andi_ui(LOCALREG(Out.cflg), LOCALREG(Out.cflg), 1);
						jit_lshr_ui(LOCALREG(Out.shiftop), LOCALREG(rm), LOCALREG(Out.shiftop));
						jit_patch(done1);
						jit_patch(done2);
						jit_patch(done3);

						regMap.Unlock(rm);
					}
				}
				else
				{
					Out.shiftop = regMap.AllocTempReg();

					{
						u32 rs = regMap.MapReg(REGID(d.Rs));
						regMap.Lock(rs);

						jit_andi_ui(LOCALREG(Out.shiftop), LOCALREG(rs), 0xFF);

						regMap.Unlock(rs);
					}

					{
						u32 rm = regMap.MapReg(REGID(d.Rm));
						regMap.Lock(rm);

						jit_insn *lt32 = jit_blti_ui(jit_forward(), LOCALREG(Out.shiftop), 32);
						jit_movi_ui(LOCALREG(Out.shiftop), 0);
						jit_insn *done = jit_jmpi(jit_forward());
						jit_patch(lt32);
						jit_lshr_ui(LOCALREG(Out.shiftop), LOCALREG(rm), LOCALREG(Out.shiftop));
						jit_patch(done);

						regMap.Unlock(rm);
					}
				}
			}
			break;
		case IRSHIFT_LSR:
			if (!d.R)
			{
				if (clacCarry)
				{
					if (d.Immediate == 0)
					{
						if (regMap.IsImm(REGID(d.Rm)))
						{
							Out.cflg = BIT31(regMap.GetImm32(REGID(d.Rm)));
							Out.cflgimm = true;
						}
						else
						{
							Out.cflg = regMap.AllocTempReg();

							u32 rm = regMap.MapReg(REGID(d.Rm));
							regMap.Lock(rm);

							jit_rshi_ui(LOCALREG(Out.cflg), LOCALREG(rm), 31);
							//jit_andi_ui(LOCALREG(Out.cflg), LOCALREG(Out.cflg), 1);

							regMap.Unlock(rm);
						}
					}
					else
					{
						if (regMap.IsImm(REGID(d.Rm)))
						{
							Out.cflg = BIT_N(regMap.GetImm32(REGID(d.Rm)), d.Immediate-1);
							Out.cflgimm = true;
						}
						else
						{
							Out.cflg = regMap.AllocTempReg();

							u32 rm = regMap.MapReg(REGID(d.Rm));
							regMap.Lock(rm);

							jit_rshi_ui(LOCALREG(Out.cflg), LOCALREG(rm), d.Immediate-1);
							jit_andi_ui(LOCALREG(Out.cflg), LOCALREG(Out.cflg), 1);

							regMap.Unlock(rm);
						}
					}
				}

				if (d.Immediate == 0)
				{
					Out.shiftop = 0;
					Out.shiftopimm = true;
				}
				else
				{
					if (regMap.IsImm(REGID(d.Rm)))
					{
						Out.shiftop = regMap.GetImm32(REGID(d.Rm))>>d.Immediate;
						Out.shiftopimm = true;
					}
					else
					{
						Out.shiftop = regMap.AllocTempReg();

						u32 rm = regMap.MapReg(REGID(d.Rm));
						regMap.Lock(rm);

						jit_rshi_ui(LOCALREG(Out.shiftop), LOCALREG(rm), d.Immediate);

						regMap.Unlock(rm);
					}
				}
			}
			else
			{
				if (clacCarry)
				{
					{
						Out.cflg = regMap.AllocTempReg();

						UnpackCPSR(regMap, PSR_C, Out.cflg);
					}

					{
						u32 rs = regMap.MapReg(REGID(d.Rs));
						regMap.Lock(rs);

						Out.shiftop = regMap.AllocTempReg();

						jit_andi_ui(LOCALREG(Out.shiftop), LOCALREG(rs), 0xFF);

						regMap.Unlock(rs);
					}

					{
						u32 rm = regMap.MapReg(REGID(d.Rm));
						regMap.Lock(rm);

						jit_insn *eq0 = jit_beqi_ui(jit_forward(), LOCALREG(Out.shiftop), 0);
						jit_insn *lt32 = jit_blti_ui(jit_forward(), LOCALREG(Out.shiftop), 32);
						jit_insn *eq32 = jit_beqi_ui(jit_forward(), LOCALREG(Out.shiftop), 32);
						jit_movi_ui(LOCALREG(Out.shiftop), 0);
						jit_movi_ui(LOCALREG(Out.cflg), 0);
						jit_insn *done1 = jit_jmpi(jit_forward());
						jit_patch(eq32);
						jit_movi_ui(LOCALREG(Out.shiftop), 0);
						jit_rshi_ui(LOCALREG(Out.cflg), LOCALREG(rm), 31);
						//jit_andi_ui(LOCALREG(Out.cflg), LOCALREG(Out.cflg), 1);
						jit_insn *done2 = jit_jmpi(jit_forward());
						jit_patch(eq0);
						jit_movr_ui(LOCALREG(Out.shiftop), LOCALREG(rm));
						jit_insn *done3 = jit_jmpi(jit_forward());
						jit_patch(lt32);
						jit_subi_ui(LOCALREG(Out.cflg), LOCALREG(Out.shiftop), 1);
						jit_rshr_ui(LOCALREG(Out.cflg), LOCALREG(rm), LOCALREG(Out.cflg));
						jit_andi_ui(LOCALREG(Out.cflg), LOCALREG(Out.cflg), 1);
						jit_rshr_ui(LOCALREG(Out.shiftop), LOCALREG(rm), LOCALREG(Out.shiftop));
						jit_patch(done1);
						jit_patch(done2);
						jit_patch(done3);

						regMap.Unlock(rm);
					}
					
				}
				else
				{
					Out.shiftop = regMap.AllocTempReg();

					{
						u32 rs = regMap.MapReg(REGID(d.Rs));
						regMap.Lock(rs);

						jit_andi_ui(LOCALREG(Out.shiftop), LOCALREG(rs), 0xFF);

						regMap.Unlock(rs);
					}

					{
						u32 rm = regMap.MapReg(REGID(d.Rm));
						regMap.Lock(rm);
					
						jit_insn *lt32 = jit_blti_ui(jit_forward(), LOCALREG(Out.shiftop), 32);
						jit_movi_ui(LOCALREG(Out.shiftop), 0);
						jit_insn *done = jit_jmpi(jit_forward());
						jit_patch(lt32);
						jit_rshr_ui(LOCALREG(Out.shiftop), LOCALREG(rm), LOCALREG(Out.shiftop));
						jit_patch(done);

						regMap.Unlock(rm);
					}
				}
			}
			break;
		case IRSHIFT_ASR:
			if (!d.R)
			{
				if (clacCarry)
				{
					if (d.Immediate == 0)
					{
						if (regMap.IsImm(REGID(d.Rm)))
						{
							Out.cflg = BIT31(regMap.GetImm32(REGID(d.Rm)));
							Out.cflgimm = true;
						}
						else
						{
							Out.cflg = regMap.AllocTempReg();

							u32 rm = regMap.MapReg(REGID(d.Rm));
							regMap.Lock(rm);

							jit_rshi_ui(LOCALREG(Out.cflg), LOCALREG(rm), 31);
							//jit_andi_ui(LOCALREG(Out.cflg), LOCALREG(Out.cflg), 1);

							regMap.Unlock(rm);
						}
					}
					else
					{
						if (regMap.IsImm(REGID(d.Rm)))
						{
							Out.cflg = BIT_N(regMap.GetImm32(REGID(d.Rm)), d.Immediate-1);
							Out.cflgimm = true;
						}
						else
						{
							Out.cflg = regMap.AllocTempReg();

							u32 rm = regMap.MapReg(REGID(d.Rm));
							regMap.Lock(rm);

							jit_rshi_ui(LOCALREG(Out.cflg), LOCALREG(rm), d.Immediate-1);
							jit_andi_ui(LOCALREG(Out.cflg), LOCALREG(Out.cflg), 1);

							regMap.Unlock(rm);
						}
					}
				}

				if (d.Immediate == 0)
				{
					if (regMap.IsImm(REGID(d.Rm)))
					{
						Out.shiftop = BIT31(regMap.GetImm32(REGID(d.Rm))) * 0xFFFFFFFF;
						Out.shiftopimm = true;
					}
					else
					{
						Out.shiftop = regMap.AllocTempReg();

						u32 rm = regMap.MapReg(REGID(d.Rm));
						regMap.Lock(rm);

						//jit_rshi_ui(LOCALREG(Out.shiftop), LOCALREG(rm), 31);
						//jit_muli_ui(LOCALREG(Out.shiftop), LOCALREG(Out.shiftop), 0xFFFFFFFF);
						jit_rshi_i(LOCALREG(Out.shiftop), LOCALREG(rm), 31);

						regMap.Unlock(rm);
					}
				}
				else
				{
					if (regMap.IsImm(REGID(d.Rm)))
					{
						Out.shiftop = (u32)((s32)regMap.GetImm32(REGID(d.Rm)) >> d.Immediate);
						Out.shiftopimm = true;
					}
					else
					{
						Out.shiftop = regMap.AllocTempReg();

						u32 rm = regMap.MapReg(REGID(d.Rm));
						regMap.Lock(rm);

						jit_rshi_i(LOCALREG(Out.shiftop), LOCALREG(rm), d.Immediate);

						regMap.Unlock(rm);
					}
				}
			}
			else
			{
				if (clacCarry)
				{
					{
						Out.cflg = regMap.AllocTempReg();

						UnpackCPSR(regMap, PSR_C, Out.cflg);
					}

					{
						u32 rs = regMap.MapReg(REGID(d.Rs));
						regMap.Lock(rs);

						Out.shiftop = regMap.AllocTempReg();

						jit_andi_ui(LOCALREG(Out.shiftop), LOCALREG(rs), 0xFF);

						regMap.Unlock(rs);
					}

					{
						u32 rm = regMap.MapReg(REGID(d.Rm));
						regMap.Lock(rm);

						jit_insn *eq0 = jit_beqi_ui(jit_forward(), LOCALREG(Out.shiftop), 0);
						jit_insn *lt32 = jit_blti_ui(jit_forward(), LOCALREG(Out.shiftop), 32);
						//jit_rshi_ui(LOCALREG(Out.shiftop), LOCALREG(rm), 31);
						//jit_muli_ui(LOCALREG(Out.shiftop), LOCALREG(Out.shiftop), 0xFFFFFFFF);
						jit_rshi_i(LOCALREG(Out.shiftop), LOCALREG(rm), 31);
						jit_rshi_ui(LOCALREG(Out.cflg), LOCALREG(rm), 31);
						//jit_andi_ui(LOCALREG(Out.cflg), LOCALREG(Out.cflg), 1);
						jit_insn *done1 = jit_jmpi(jit_forward());
						jit_patch(eq0);
						jit_movr_ui(LOCALREG(Out.shiftop), LOCALREG(rm));
						jit_insn *done2 = jit_jmpi(jit_forward());
						jit_patch(lt32);
						jit_subi_ui(LOCALREG(Out.cflg), LOCALREG(Out.shiftop), 1);
						jit_rshr_ui(LOCALREG(Out.cflg), LOCALREG(rm), LOCALREG(Out.cflg));
						jit_andi_ui(LOCALREG(Out.cflg), LOCALREG(Out.cflg), 1);
						jit_rshr_i(LOCALREG(Out.shiftop), LOCALREG(rm), LOCALREG(Out.shiftop));
						jit_patch(done1);
						jit_patch(done2);

						regMap.Unlock(rm);
					}
				}
				else
				{
					Out.shiftop = regMap.AllocTempReg();

					{
						u32 rs = regMap.MapReg(REGID(d.Rs));
						regMap.Lock(rs);

						jit_andi_ui(LOCALREG(Out.shiftop), LOCALREG(rs), 0xFF);

						regMap.Unlock(rs);
					}

					{
						u32 rm = regMap.MapReg(REGID(d.Rm));
						regMap.Lock(rm);
					
						jit_insn *eq0 = jit_beqi_ui(jit_forward(), LOCALREG(Out.shiftop), 0);
						jit_insn *lt32 = jit_blti_ui(jit_forward(), LOCALREG(Out.shiftop), 32);
						//jit_rshi_ui(LOCALREG(Out.shiftop), LOCALREG(rm), 31);
						//jit_muli_ui(LOCALREG(Out.shiftop), LOCALREG(Out.shiftop), 0xFFFFFFFF);
						jit_rshi_i(LOCALREG(Out.shiftop), LOCALREG(rm), 31);
						jit_insn *done1 = jit_jmpi(jit_forward());
						jit_patch(eq0);
						jit_movr_ui(LOCALREG(Out.shiftop), LOCALREG(rm));
						jit_insn *done2 = jit_jmpi(jit_forward());
						jit_patch(lt32);
						jit_rshr_i(LOCALREG(Out.shiftop), LOCALREG(rm), LOCALREG(Out.shiftop));
						jit_patch(done1);
						jit_patch(done2);

						regMap.Unlock(rm);
					}
				}
			}
			break;
		case IRSHIFT_ROR:
			if (!d.R)
			{
				if (clacCarry)
				{
					if (d.Immediate == 0)
					{
						if (regMap.IsImm(REGID(d.Rm)))
						{
							Out.cflg = BIT0(regMap.GetImm32(REGID(d.Rm)));
							Out.cflgimm = true;
						}
						else
						{
							Out.cflg = regMap.AllocTempReg();

							u32 rm = regMap.MapReg(REGID(d.Rm));
							regMap.Lock(rm);

							jit_andi_ui(LOCALREG(Out.cflg), LOCALREG(rm), 1);

							regMap.Unlock(rm);
						}
					}
					else
					{
						if (regMap.IsImm(REGID(d.Rm)))
						{
							Out.cflg = BIT_N(regMap.GetImm32(REGID(d.Rm)), d.Immediate-1);
							Out.cflgimm = true;
						}
						else
						{
							Out.cflg = regMap.AllocTempReg();

							u32 rm = regMap.MapReg(REGID(d.Rm));
							regMap.Lock(rm);

							jit_rshi_ui(LOCALREG(Out.cflg), LOCALREG(rm), d.Immediate-1);
							jit_andi_ui(LOCALREG(Out.cflg), LOCALREG(Out.cflg), 1);

							regMap.Unlock(rm);
						}
					}
				}

				if (d.Immediate == 0)
				{
					if (regMap.IsImm(REGID(d.Rm)))
					{
						Out.shiftop = regMap.AllocTempReg();

						u32 tmp = regMap.GetImm32(REGID(d.Rm)) >> 1;

						UnpackCPSR(regMap, PSR_C, Out.shiftop);
						jit_lshi_ui(LOCALREG(Out.shiftop), LOCALREG(Out.shiftop), 31);
						jit_ori_ui(LOCALREG(Out.shiftop), LOCALREG(Out.shiftop), tmp);
					}
					else
					{
						Out.shiftop = regMap.AllocTempReg();

						UnpackCPSR(regMap, PSR_C, Out.shiftop);

						u32 rm = regMap.MapReg(REGID(d.Rm));
						regMap.Lock(rm);

						u32 tmp = regMap.AllocTempReg();

						jit_lshi_ui(LOCALREG(Out.shiftop), LOCALREG(Out.shiftop), 31);
						jit_rshi_ui(LOCALREG(tmp), LOCALREG(rm), 1);
						jit_orr_ui(LOCALREG(Out.shiftop), LOCALREG(Out.shiftop), LOCALREG(tmp));

						regMap.ReleaseTempReg(tmp);
						regMap.Unlock(rm);
					}
				}
				else
				{
					if (regMap.IsImm(REGID(d.Rm)))
					{
						Out.shiftop = ROR(regMap.GetImm32(REGID(d.Rm)), d.Immediate);
						Out.shiftopimm = true;
					}
					else
					{
						Out.shiftop = regMap.AllocTempReg();

						u32 rm = regMap.MapReg(REGID(d.Rm));
						regMap.Lock(rm);
						u32 tmp = regMap.AllocTempReg();

						jit_rshi_ui(LOCALREG(tmp), LOCALREG(rm), d.Immediate);
						jit_lshi_ui(LOCALREG(Out.shiftop), LOCALREG(rm), 32-d.Immediate);
						jit_orr_ui(LOCALREG(Out.shiftop), LOCALREG(Out.shiftop), LOCALREG(tmp));

						regMap.ReleaseTempReg(tmp);
						regMap.Unlock(rm);
					}
				}
			}
			else
			{
				if (clacCarry)
				{
					{
						Out.cflg = regMap.AllocTempReg();

						UnpackCPSR(regMap, PSR_C, Out.cflg);
					}

					u32 rs = regMap.MapReg(REGID(d.Rs));
					regMap.Lock(rs);
					u32 rm = regMap.MapReg(REGID(d.Rm));
					regMap.Lock(rm);

					Out.shiftop = regMap.AllocTempReg();

					u32 tmp = regMap.AllocTempReg();

					jit_andi_ui(LOCALREG(Out.shiftop), LOCALREG(rs), 0xFF);
					jit_insn *eq0 = jit_beqi_ui(jit_forward(), LOCALREG(Out.shiftop), 0);
					jit_andi_ui(LOCALREG(Out.shiftop), LOCALREG(rs), 0x1F);
					jit_insn *eq0_1F = jit_beqi_ui(jit_forward(), LOCALREG(Out.shiftop), 0);
					jit_subi_ui(LOCALREG(tmp), LOCALREG(Out.shiftop), 1);
					jit_rshr_ui(LOCALREG(Out.cflg), LOCALREG(rm), LOCALREG(tmp));
					jit_andi_ui(LOCALREG(Out.cflg), LOCALREG(Out.cflg), 1);
					jit_rsbi_ui(LOCALREG(tmp), LOCALREG(Out.shiftop), 32);
					jit_rshr_ui(LOCALREG(Out.shiftop), LOCALREG(rm), LOCALREG(Out.shiftop));
					jit_lshr_ui(LOCALREG(tmp), LOCALREG(rm), LOCALREG(tmp));
					jit_orr_ui(LOCALREG(Out.shiftop), LOCALREG(Out.shiftop), LOCALREG(tmp));
					jit_insn *done1 = jit_jmpi(jit_forward());
					jit_patch(eq0_1F);
					jit_rshi_ui(LOCALREG(Out.cflg), LOCALREG(rm), 31);
					jit_movr_ui(LOCALREG(Out.shiftop), LOCALREG(rm));
					jit_insn *done2 = jit_jmpi(jit_forward());
					jit_patch(eq0);
					jit_movr_ui(LOCALREG(Out.shiftop), LOCALREG(rm));
					jit_patch(done1);
					jit_patch(done2);

					regMap.ReleaseTempReg(tmp);
					regMap.Unlock(rm);
					regMap.Unlock(rs);
				}
				else
				{
					Out.shiftop = regMap.AllocTempReg();

					{
						u32 rs = regMap.MapReg(REGID(d.Rs));
						regMap.Lock(rs);

						jit_andi_ui(LOCALREG(Out.shiftop), LOCALREG(rs), 0x1F);

						regMap.Unlock(rs);
					}

					{
						u32 rm = regMap.MapReg(REGID(d.Rm));
						regMap.Lock(rm);
						u32 tmp = regMap.AllocTempReg();

						jit_insn *nei0 = jit_bnei_ui(jit_forward(), LOCALREG(Out.shiftop), 0);
						jit_movr_ui(LOCALREG(Out.shiftop), LOCALREG(rm));
						jit_insn *done = jit_jmpi(jit_forward());
						jit_patch(nei0);
						jit_rsbi_ui(LOCALREG(tmp), LOCALREG(Out.shiftop), 32);
						jit_rshr_ui(LOCALREG(Out.shiftop), LOCALREG(rm), LOCALREG(Out.shiftop));
						jit_lshr_ui(LOCALREG(tmp), LOCALREG(rm), LOCALREG(tmp));
						jit_orr_ui(LOCALREG(Out.shiftop), LOCALREG(Out.shiftop), LOCALREG(tmp));
						jit_patch(done);

						regMap.ReleaseTempReg(tmp);
						regMap.Unlock(rm);
					}
				}
			}
			break;
		default:
			INFO("Unknow Shift Op : %u.\n", d.Typ);
			if (clacCarry)
			{
				Out.cflg = 0;
				Out.cflgimm = true;
			}
			Out.shiftop = 0;
			Out.shiftopimm = true;
			break;
		}

		return Out;
	}

	void FASTCALL DataProcessLoadCPSRGenerate(const Decoded &d, RegisterMap &regMap)
	{
		u32 PROCNUM = d.ProcessID;

		std::vector<ABIOp> args;
		std::vector<RegisterMap::GuestRegId> flushs;

		u32 tmp = regMap.AllocTempReg(true);

		{
			args.clear();
			flushs.clear();

			u32 tmp2 = regMap.AllocTempReg();
			u32 spsr = regMap.MapReg(RegisterMap::SPSR);
			regMap.Lock(spsr);
			jit_movr_ui(LOCALREG(tmp), LOCALREG(spsr));
			regMap.Unlock(spsr);
			UnpackPSR(PSR_MODE, tmp, tmp2);

			for (u32 i = RegisterMap::R8; i <= RegisterMap::R14; i++)
				flushs.push_back((RegisterMap::GuestRegId)i);
			flushs.push_back(RegisterMap::CPSR);
			flushs.push_back(RegisterMap::SPSR);

			ABIOp op;

			op.type = ABIOp::HOSTREG;
			op.regdata = regMap.GetCpuPtrReg();
			args.push_back(op);

			op.type = ABIOp::TEMPREG;
			op.regdata = tmp2;
			args.push_back(op);

			regMap.CallABI((void*)&armcpu_switchMode, args, flushs);
		}

		u32 cpsr = regMap.MapReg(RegisterMap::CPSR, RegisterMap::MAP_DIRTY | RegisterMap::MAP_NOTINIT);
		regMap.Lock(cpsr);
		jit_movr_ui(LOCALREG(cpsr), LOCALREG(tmp));
		regMap.Unlock(cpsr);

		regMap.ReleaseTempReg(tmp);

		{
			args.clear();
			flushs.clear();

			ABIOp op;

			op.type = ABIOp::HOSTREG;
			op.regdata = regMap.GetCpuPtrReg();
			args.push_back(op);

			regMap.CallABI((void*)&armcpu_changeCPSR, args, flushs);
		}

		tmp = regMap.AllocTempReg();

		UnpackCPSR(regMap, PSR_T, tmp);
		jit_lshi_ui(LOCALREG(tmp), LOCALREG(tmp), 1);
		jit_ori_ui(LOCALREG(tmp), LOCALREG(tmp), 0xFFFFFFFC);

		u32 r15 = regMap.MapReg(RegisterMap::R15, RegisterMap::MAP_DIRTY);
		regMap.Lock(r15);

		jit_andr_ui(LOCALREG(r15), LOCALREG(r15), LOCALREG(tmp));

		regMap.Unlock(r15);

		regMap.ReleaseTempReg(tmp);
	}

	void FASTCALL LDM_S_LoadCPSRGenerate(const Decoded &d, RegisterMap &regMap)
	{
		u32 PROCNUM = d.ProcessID;

		DataProcessLoadCPSRGenerate(d, regMap);
	}

	void FASTCALL R15ModifiedGenerate(const Decoded &d, RegisterMap &regMap)
	{
		u32 PROCNUM = d.ProcessID;

		u32 cpuptr = regMap.GetCpuPtrReg();
		u32 r15 = regMap.MapReg(RegisterMap::R15);
		regMap.Lock(r15);

		jit_stxi_ui(jit_field(armcpu_t, instruct_adr), LOCALREG(cpuptr), LOCALREG(r15));

		regMap.Unlock(r15);
	}

	void FASTCALL MUL_Mxx_END(const Decoded &d, RegisterMap &regMap, u32 base, u32 v)
	{
		u32 execyc = regMap.MapReg(RegisterMap::EXECUTECYCLES, RegisterMap::MAP_DIRTY);
		regMap.Lock(execyc);
		u32 tmp = regMap.AllocTempReg();

		jit_addi_ui(LOCALREG(execyc), LOCALREG(execyc), base);
		jit_andi_ui(LOCALREG(tmp), LOCALREG(v), 0xFFFFFF00);
		jit_insn *eq_l1 = jit_beqi_ui(jit_forward(), LOCALREG(tmp), 0);
		jit_andi_ui(LOCALREG(tmp), LOCALREG(v), 0xFFFF0000);
		jit_insn *eq_l2 = jit_beqi_ui(jit_forward(), LOCALREG(tmp), 0);
		jit_andi_ui(LOCALREG(tmp), LOCALREG(v), 0xFF000000);
		jit_insn *eq_l3 = jit_beqi_ui(jit_forward(), LOCALREG(tmp), 0);
		jit_addi_ui(LOCALREG(execyc), LOCALREG(execyc), 4);
		jit_insn *done4 = jit_jmpi(jit_forward());
		jit_patch(eq_l1);
		jit_addi_ui(LOCALREG(execyc), LOCALREG(execyc), 1);
		jit_insn *done1 = jit_jmpi(jit_forward());
		jit_patch(eq_l2);
		jit_addi_ui(LOCALREG(execyc), LOCALREG(execyc), 2);
		jit_insn *done2 = jit_jmpi(jit_forward());
		jit_patch(eq_l3);
		jit_addi_ui(LOCALREG(execyc), LOCALREG(execyc), 3);
		jit_patch(done1);
		jit_patch(done2);
		jit_patch(done4);

		regMap.Unlock(execyc);
		regMap.ReleaseTempReg(tmp);
	}

	void FASTCALL MUL_Mxx_END_Imm(const Decoded &d, RegisterMap &regMap, u32 base, u32 v)
	{
		if ((v & 0xFFFFFF00) == 0)
			base += 1;
		else if ((v & 0xFFFF0000) == 0)
			base += 2;
		else if ((v & 0xFF000000) == 0)
			base += 3;
		else 
			base += 4;

		u32 execyc = regMap.MapReg(RegisterMap::EXECUTECYCLES, RegisterMap::MAP_DIRTY);
		regMap.Lock(execyc);
		
		jit_addi_ui(LOCALREG(execyc), LOCALREG(execyc), base);

		regMap.Unlock(execyc);
	}

	jit_insn* FASTCALL PrepareSLZone()
	{
		static const u32 ALIGN_SIZE = 4 - 1;
		static const u32 SLZONE_SIZE = 32*4;

		jit_insn* bk_ptr = jit_get_label();

		uintptr_t new_ptr = (uintptr_t)jit_get_ip().ptr + (SLZONE_SIZE + ALIGN_SIZE);
		jit_insn* new_ptr_align = (jit_insn*)(new_ptr & ~ALIGN_SIZE);

		jit_set_ip(new_ptr_align);

		memset(bk_ptr, 0xCC, new_ptr_align - bk_ptr);

		//PROGINFO("PrepareSLZone(), bk : %#p, new : %#p, size : %u\n", bk_ptr, new_ptr_align, new_ptr_align - bk_ptr);

		return bk_ptr;
	}

	void FASTCALL Fallback2Interpreter(const Decoded &d, RegisterMap &regMap)
	{
#ifdef HAVE_FASTCALL
		struct Interpreter
		{
			static u32 Method(OpFunc func, u32 opcode)
			{
				return func(opcode);
			}
		};
#endif
		u32 PROCNUM = d.ProcessID;

		std::vector<ABIOp> args;
		std::vector<RegisterMap::GuestRegId> flushs;

		u32 opcode;
		OpFunc func;

		if (d.ThumbFlag)
		{
			opcode = d.Instruction.ThumbOp;
			func = thumb_instructions_set[PROCNUM][opcode>>6];
		}
		else
		{
			opcode = d.Instruction.ArmOp;
			func = arm_instructions_set[PROCNUM][INSTRUCTION_INDEX(opcode)];
		}

		args.clear();
		flushs.clear();

		u32 tmp = regMap.AllocTempReg();

		ABIOp op;
#ifdef HAVE_FASTCALL
		op.type = ABIOp::IMM;
		op.immdata.type = ImmData::IMMPTR;
		op.immdata.immptr = reinterpret_cast<void*>(func);
		args.push_back(op);
		
		op.type = ABIOp::IMM;
		op.immdata.type = ImmData::IMM32;
		op.immdata.imm32 = opcode;
		args.push_back(op);

		regMap.CallABI((void*)&Interpreter::Method, args, flushs, tmp);
#else
		op.type = ABIOp::IMM;
		op.immdata.type = ImmData::IMM32;
		op.immdata.imm32 = opcode;
		args.push_back(op);

		regMap.CallABI((void*)func, args, flushs, tmp);
#endif
		u32 execyc = regMap.MapReg(RegisterMap::EXECUTECYCLES, RegisterMap::MAP_DIRTY);
		regMap.Lock(execyc);

		jit_addr_ui(LOCALREG(execyc), LOCALREG(execyc), LOCALREG(tmp));

		regMap.Unlock(execyc);

		regMap.ReleaseTempReg(tmp);
	}

	void FASTCALL CheckReschedule(const Decoded &d, RegisterMap &regMap)
	{
	}

//------------------------------------------------------------
//                         IROp decoder
//------------------------------------------------------------
	OPDECODER_DECL(IR_UND)
	{
		u32 PROCNUM = d.ProcessID;

		INFO("IR_UND\n");

		u32 cpuptr = regMap.GetCpuPtrReg();
		u32 tmp = regMap.AllocTempReg();

		if (d.ThumbFlag)
			jit_movi_ui(LOCALREG(tmp), d.Instruction.ThumbOp);
		else
			jit_movi_ui(LOCALREG(tmp), d.Instruction.ArmOp);
		jit_stxi_ui(jit_field(armcpu_t, instruction), LOCALREG(cpuptr), LOCALREG(tmp));

		jit_movi_ui(LOCALREG(tmp), d.Address);
		jit_stxi_ui(jit_field(armcpu_t, instruct_adr), LOCALREG(cpuptr), LOCALREG(tmp));

		regMap.ReleaseTempReg(tmp);

		{
			std::vector<ABIOp> args;
			std::vector<RegisterMap::GuestRegId> flushs;

			ABIOp op;
			op.type = ABIOp::HOSTREG;
			op.regdata = regMap.GetCpuPtrReg();
			args.push_back(op);

			regMap.CallABI((void*)&TRAPUNDEF, args, flushs);
		}
	}

	OPDECODER_DECL(IR_NOP)
	{
	}

	OPDECODER_DECL(IR_DUMMY)
	{
	}

	OPDECODER_DECL(IR_T32P1)
	{
		u32 PROCNUM = d.ProcessID;

		INFO("IR_T32P1\n");

		u32 cpuptr = regMap.GetCpuPtrReg();
		u32 tmp = regMap.AllocTempReg();

		if (d.ThumbFlag)
			jit_movi_ui(LOCALREG(tmp), d.Instruction.ThumbOp);
		else
			jit_movi_ui(LOCALREG(tmp), d.Instruction.ArmOp);
		jit_stxi_ui(jit_field(armcpu_t, instruction), LOCALREG(cpuptr), LOCALREG(tmp));

		jit_movi_ui(LOCALREG(tmp), d.Address);
		jit_stxi_ui(jit_field(armcpu_t, instruct_adr), LOCALREG(cpuptr), LOCALREG(tmp));

		regMap.ReleaseTempReg(tmp);

		{
			std::vector<ABIOp> args;
			std::vector<RegisterMap::GuestRegId> flushs;

			ABIOp op;
			op.type = ABIOp::HOSTREG;
			op.regdata = regMap.GetCpuPtrReg();
			args.push_back(op);

			regMap.CallABI((void*)&TRAPUNDEF, args, flushs);
		}
	}

	OPDECODER_DECL(IR_T32P2)
	{
		u32 PROCNUM = d.ProcessID;

		INFO("IR_T32P2\n");

		u32 cpuptr = regMap.GetCpuPtrReg();
		u32 tmp = regMap.AllocTempReg();

		if (d.ThumbFlag)
			jit_movi_ui(LOCALREG(tmp), d.Instruction.ThumbOp);
		else
			jit_movi_ui(LOCALREG(tmp), d.Instruction.ArmOp);
		jit_stxi_ui(jit_field(armcpu_t, instruction), LOCALREG(cpuptr), LOCALREG(tmp));

		jit_movi_ui(LOCALREG(tmp), d.Address);
		jit_stxi_ui(jit_field(armcpu_t, instruct_adr), LOCALREG(cpuptr), LOCALREG(tmp));

		regMap.ReleaseTempReg(tmp);

		{
			std::vector<ABIOp> args;
			std::vector<RegisterMap::GuestRegId> flushs;

			ABIOp op;
			op.type = ABIOp::HOSTREG;
			op.regdata = regMap.GetCpuPtrReg();
			args.push_back(op);

			regMap.CallABI((void*)&TRAPUNDEF, args, flushs);
		}
	}

	OPDECODER_DECL(IR_MOV)
	{
		u32 PROCNUM = d.ProcessID;

		if (d.I)
		{
			regMap.SetImm32(REGID(d.Rd), d.Immediate);

			if (d.S && !d.R15Modified)
			{
				if (d.FlagsSet & FLAG_C)
					PackCPSRImm(regMap, PSR_C, BIT31(d.Immediate));
				if (d.FlagsSet & FLAG_N)
					PackCPSRImm(regMap, PSR_N, BIT31(d.Immediate));
				if (d.FlagsSet & FLAG_Z)
					PackCPSRImm(regMap, PSR_Z, d.Immediate==0 ? 1 : 0);
			}
		}
		else
		{
			const bool clacCarry = d.S && !d.R15Modified && (d.FlagsSet & FLAG_C);
			ShiftOut shift_out = IRShiftOpGenerate(d, regMap, clacCarry);

			u32 rd = INVALID_REG_ID;

			if (shift_out.shiftopimm)
				regMap.SetImm32(REGID(d.Rd), shift_out.shiftop);
			else
			{
				rd = regMap.MapReg(REGID(d.Rd), RegisterMap::MAP_DIRTY | RegisterMap::MAP_NOTINIT);
				regMap.Lock(rd);

				jit_movr_ui(LOCALREG(rd), LOCALREG(shift_out.shiftop));
			}
			
			if (d.S && !d.R15Modified)
			{
				if (d.FlagsSet & FLAG_C)
				{
					if (shift_out.cflgimm)
						PackCPSRImm(regMap, PSR_C, shift_out.cflg);
					else
					{
						PackCPSR(regMap, PSR_C, shift_out.cflg);
						shift_out.CleanCflg(regMap);
					}
				}
				if (d.FlagsSet & FLAG_N)
				{
					if (regMap.IsImm(REGID(d.Rd)))
						PackCPSRImm(regMap, PSR_N, BIT31(regMap.GetImm32(REGID(d.Rd))));
					else
					{
						jit_rshi_ui(LOCALREG(shift_out.shiftop), LOCALREG(rd), 31);
						PackCPSR(regMap, PSR_N, shift_out.shiftop);
					}
				}
				if (d.FlagsSet & FLAG_Z)
				{
					if (regMap.IsImm(REGID(d.Rd)))
						PackCPSRImm(regMap, PSR_Z, regMap.GetImm32(REGID(d.Rd))==0);
					else
					{
						jit_eqi_ui(LOCALREG(shift_out.shiftop), LOCALREG(rd), 0);
						PackCPSR(regMap, PSR_Z, shift_out.shiftop);
					}
				}
			}

			if (rd != INVALID_REG_ID)
				regMap.Unlock(rd);

			shift_out.Clean(regMap);
		}

		if (d.R15Modified)
		{
			if (d.S)
			{
				DataProcessLoadCPSRGenerate(d, regMap);
			}

			R15ModifiedGenerate(d, regMap);
		}
	}

	OPDECODER_DECL(IR_MVN)
	{
		u32 PROCNUM = d.ProcessID;

		if (d.I)
		{
			regMap.SetImm32(REGID(d.Rd), ~d.Immediate);

			if (d.S && !d.R15Modified)
			{
				if (d.FlagsSet & FLAG_C)
					PackCPSRImm(regMap, PSR_C, BIT31(d.Immediate));
				if (d.FlagsSet & FLAG_N)
					PackCPSRImm(regMap, PSR_N, BIT31(~d.Immediate));
				if (d.FlagsSet & FLAG_Z)
					PackCPSRImm(regMap, PSR_Z, ~d.Immediate==0 ? 1 : 0);
			}
		}
		else
		{
			const bool clacCarry = d.S && !d.R15Modified && (d.FlagsSet & FLAG_C);
			ShiftOut shift_out = IRShiftOpGenerate(d, regMap, clacCarry);

			u32 rd = INVALID_REG_ID;

			if (shift_out.shiftopimm)
				regMap.SetImm32(REGID(d.Rd), ~shift_out.shiftop);
			else
			{
				rd = regMap.MapReg(REGID(d.Rd), RegisterMap::MAP_DIRTY | RegisterMap::MAP_NOTINIT);
				regMap.Lock(rd);

				jit_notr_ui(LOCALREG(rd), LOCALREG(shift_out.shiftop));
			}
			
			if (d.S && !d.R15Modified)
			{
				if (d.FlagsSet & FLAG_C)
				{
					if (shift_out.cflgimm)
						PackCPSRImm(regMap, PSR_C, shift_out.cflg);
					else
					{
						PackCPSR(regMap, PSR_C, shift_out.cflg);
						shift_out.CleanCflg(regMap);
					}
				}
				if (d.FlagsSet & FLAG_N)
				{
					if (regMap.IsImm(REGID(d.Rd)))
						PackCPSRImm(regMap, PSR_N, BIT31(regMap.GetImm32(REGID(d.Rd))));
					else
					{
						jit_rshi_ui(LOCALREG(shift_out.shiftop), LOCALREG(rd), 31);
						PackCPSR(regMap, PSR_N, shift_out.shiftop);
					}
				}
				if (d.FlagsSet & FLAG_Z)
				{
					if (regMap.IsImm(REGID(d.Rd)))
						PackCPSRImm(regMap, PSR_Z, regMap.GetImm32(REGID(d.Rd))==0);
					else
					{
						jit_eqi_ui(LOCALREG(shift_out.shiftop), LOCALREG(rd), 0);
						PackCPSR(regMap, PSR_Z, shift_out.shiftop);
					}
				}
			}

			if (rd != INVALID_REG_ID)
				regMap.Unlock(rd);

			shift_out.Clean(regMap);
		}

		if (d.R15Modified)
		{
			if (d.S)
			{
				DataProcessLoadCPSRGenerate(d, regMap);
			}

			R15ModifiedGenerate(d, regMap);
		}
	}

	OPDECODER_DECL(IR_AND)
	{
		u32 PROCNUM = d.ProcessID;

		u32 rd = INVALID_REG_ID;

		if (d.I)
		{
			if (regMap.IsImm(REGID(d.Rn)))
				regMap.SetImm32(REGID(d.Rd), regMap.GetImm32(REGID(d.Rn)) & d.Immediate);
			else
			{
				u32 rn = regMap.MapReg(REGID(d.Rn));
				regMap.Lock(rn);

				rd = regMap.MapReg(REGID(d.Rd), RegisterMap::MAP_DIRTY | RegisterMap::MAP_NOTINIT);
				regMap.Lock(rd);

				jit_andi_ui(LOCALREG(rd), LOCALREG(rn), d.Immediate);

				regMap.Unlock(rn);
			}

			if (d.S && !d.R15Modified)
			{
				if (d.FlagsSet & FLAG_C)
					PackCPSRImm(regMap, PSR_C, BIT31(d.Immediate));
			}
		}
		else
		{
			const bool clacCarry = d.S && !d.R15Modified && (d.FlagsSet & FLAG_C);
			ShiftOut shift_out = IRShiftOpGenerate(d, regMap, clacCarry);

			if (regMap.IsImm(REGID(d.Rn)) && shift_out.shiftopimm)
				regMap.SetImm32(REGID(d.Rd), regMap.GetImm32(REGID(d.Rn)) & shift_out.shiftop);
			else
			{
				u32 rn = regMap.MapReg(REGID(d.Rn));
				regMap.Lock(rn);

				rd = regMap.MapReg(REGID(d.Rd), RegisterMap::MAP_DIRTY | RegisterMap::MAP_NOTINIT);
				regMap.Lock(rd);

				if (shift_out.shiftopimm)
					jit_andi_ui(LOCALREG(rd), LOCALREG(rn), shift_out.shiftop);
				else
					jit_andr_ui(LOCALREG(rd), LOCALREG(rn), LOCALREG(shift_out.shiftop));

				regMap.Unlock(rn);
			}

			if (d.S && !d.R15Modified)
			{
				if (d.FlagsSet & FLAG_C)
				{
					if (shift_out.cflgimm)
						PackCPSRImm(regMap, PSR_C, shift_out.cflg);
					else
						PackCPSR(regMap, PSR_C, shift_out.cflg);
				}
			}

			shift_out.Clean(regMap);
		}

		if (d.S && !d.R15Modified)
		{
			if (d.FlagsSet & FLAG_N)
			{
				if (regMap.IsImm(REGID(d.Rd)))
					PackCPSRImm(regMap, PSR_N, BIT31(regMap.GetImm32(REGID(d.Rd))));
				else
				{
					u32 tmp = regMap.AllocTempReg();

					jit_rshi_ui(LOCALREG(tmp), LOCALREG(rd), 31);
					PackCPSR(regMap, PSR_N, tmp);
						
					regMap.ReleaseTempReg(tmp);
				}
			}
			if (d.FlagsSet & FLAG_Z)
			{
				if (regMap.IsImm(REGID(d.Rd)))
					PackCPSRImm(regMap, PSR_Z, regMap.GetImm32(REGID(d.Rd))==0);
				else
				{
					u32 tmp = regMap.AllocTempReg();

					jit_eqi_ui(LOCALREG(tmp), LOCALREG(rd), 0);
					PackCPSR(regMap, PSR_Z, tmp);
						
					regMap.ReleaseTempReg(tmp);
				}
			}
		}

		if (rd != INVALID_REG_ID)
			regMap.Unlock(rd);

		if (d.R15Modified)
		{
			if (d.S)
			{
				DataProcessLoadCPSRGenerate(d, regMap);
			}

			R15ModifiedGenerate(d, regMap);
		}
	}

	OPDECODER_DECL(IR_TST)
	{
		u32 PROCNUM = d.ProcessID;

		u32 dst = INVALID_REG_ID;
		bool dstimm = false;

		if (d.I)
		{
			if (regMap.IsImm(REGID(d.Rn)))
			{
				dst = regMap.GetImm32(REGID(d.Rn)) & d.Immediate;
				dstimm = true;
			}
			else
			{
				u32 rn = regMap.MapReg(REGID(d.Rn));
				regMap.Lock(rn);

				dst = regMap.AllocTempReg();

				jit_andi_ui(LOCALREG(dst), LOCALREG(rn), d.Immediate);

				regMap.Unlock(rn);
			}

			{
				if (d.FlagsSet & FLAG_C)
					PackCPSRImm(regMap, PSR_C, BIT31(d.Immediate));
			}
		}
		else
		{
			const bool clacCarry = (d.FlagsSet & FLAG_C);
			ShiftOut shift_out = IRShiftOpGenerate(d, regMap, clacCarry);

			if (regMap.IsImm(REGID(d.Rn)) && shift_out.shiftopimm)
			{
				dst = regMap.GetImm32(REGID(d.Rn)) & shift_out.shiftop;
				dstimm = true;
			}
			else
			{
				u32 rn = regMap.MapReg(REGID(d.Rn));
				regMap.Lock(rn);

				dst = regMap.AllocTempReg();

				if (shift_out.shiftopimm)
					jit_andi_ui(LOCALREG(dst), LOCALREG(rn), shift_out.shiftop);
				else
					jit_andr_ui(LOCALREG(dst), LOCALREG(rn), LOCALREG(shift_out.shiftop));

				regMap.Unlock(rn);
			}
			
			{
				if (d.FlagsSet & FLAG_C)
				{
					if (shift_out.cflgimm)
						PackCPSRImm(regMap, PSR_C, shift_out.cflg);
					else
						PackCPSR(regMap, PSR_C, shift_out.cflg);
				}

				shift_out.Clean(regMap);
			}
		}

		{
			if (d.FlagsSet & FLAG_N)
			{
				if (dstimm)
					PackCPSRImm(regMap, PSR_N, BIT31(dst));
				else
				{
					u32 tmp = regMap.AllocTempReg();

					jit_rshi_ui(LOCALREG(tmp), LOCALREG(dst), 31);
					PackCPSR(regMap, PSR_N, tmp);
						
					regMap.ReleaseTempReg(tmp);
				}
			}
			if (d.FlagsSet & FLAG_Z)
			{
				if (dstimm)
					PackCPSRImm(regMap, PSR_Z, dst==0);
				else
				{
					u32 tmp = regMap.AllocTempReg();

					jit_eqi_ui(LOCALREG(tmp), LOCALREG(dst), 0);
					PackCPSR(regMap, PSR_Z, tmp);
						
					regMap.ReleaseTempReg(tmp);
				}
			}
		}

		if (!dstimm)
			regMap.ReleaseTempReg(dst);
	}

	OPDECODER_DECL(IR_EOR)
	{
		u32 PROCNUM = d.ProcessID;

		u32 rd = INVALID_REG_ID;

		if (d.I)
		{
			if (regMap.IsImm(REGID(d.Rn)))
				regMap.SetImm32(REGID(d.Rd), regMap.GetImm32(REGID(d.Rn)) ^ d.Immediate);
			else
			{
				u32 rn = regMap.MapReg(REGID(d.Rn));
				regMap.Lock(rn);

				rd = regMap.MapReg(REGID(d.Rd), RegisterMap::MAP_DIRTY | RegisterMap::MAP_NOTINIT);
				regMap.Lock(rd);

				jit_xori_ui(LOCALREG(rd), LOCALREG(rn), d.Immediate);

				regMap.Unlock(rn);
			}

			if (d.S && !d.R15Modified)
			{
				if (d.FlagsSet & FLAG_C)
					PackCPSRImm(regMap, PSR_C, BIT31(d.Immediate));
			}
		}
		else
		{
			const bool clacCarry = d.S && !d.R15Modified && (d.FlagsSet & FLAG_C);
			ShiftOut shift_out = IRShiftOpGenerate(d, regMap, clacCarry);

			if (regMap.IsImm(REGID(d.Rn)) && shift_out.shiftopimm)
				regMap.SetImm32(REGID(d.Rd), regMap.GetImm32(REGID(d.Rn)) ^ shift_out.shiftop);
			else
			{
				u32 rn = regMap.MapReg(REGID(d.Rn));
				regMap.Lock(rn);

				rd = regMap.MapReg(REGID(d.Rd), RegisterMap::MAP_DIRTY | RegisterMap::MAP_NOTINIT);
				regMap.Lock(rd);

				if (shift_out.shiftopimm)
					jit_xori_ui(LOCALREG(rd), LOCALREG(rn), shift_out.shiftop);
				else
					jit_xorr_ui(LOCALREG(rd), LOCALREG(rn), LOCALREG(shift_out.shiftop));

				regMap.Unlock(rn);
			}

			if (d.S && !d.R15Modified)
			{
				if (d.FlagsSet & FLAG_C)
				{
					if (shift_out.cflgimm)
						PackCPSRImm(regMap, PSR_C, shift_out.cflg);
					else
						PackCPSR(regMap, PSR_C, shift_out.cflg);
				}
			}

			shift_out.Clean(regMap);
		}

		if (d.S && !d.R15Modified)
		{
			if (d.FlagsSet & FLAG_N)
			{
				if (regMap.IsImm(REGID(d.Rd)))
					PackCPSRImm(regMap, PSR_N, BIT31(regMap.GetImm32(REGID(d.Rd))));
				else
				{
					u32 tmp = regMap.AllocTempReg();

					jit_rshi_ui(LOCALREG(tmp), LOCALREG(rd), 31);
					PackCPSR(regMap, PSR_N, tmp);
						
					regMap.ReleaseTempReg(tmp);
				}
			}
			if (d.FlagsSet & FLAG_Z)
			{
				if (regMap.IsImm(REGID(d.Rd)))
					PackCPSRImm(regMap, PSR_Z, regMap.GetImm32(REGID(d.Rd))==0);
				else
				{
					u32 tmp = regMap.AllocTempReg();

					jit_eqi_ui(LOCALREG(tmp), LOCALREG(rd), 0);
					PackCPSR(regMap, PSR_Z, tmp);
						
					regMap.ReleaseTempReg(tmp);
				}
			}
		}

		if (rd != INVALID_REG_ID)
			regMap.Unlock(rd);

		if (d.R15Modified)
		{
			if (d.S)
			{
				DataProcessLoadCPSRGenerate(d, regMap);
			}

			R15ModifiedGenerate(d, regMap);
		}
	}

	OPDECODER_DECL(IR_TEQ)
	{
		u32 PROCNUM = d.ProcessID;

		u32 dst = INVALID_REG_ID;
		bool dstimm = false;

		if (d.I)
		{
			if (regMap.IsImm(REGID(d.Rn)))
			{
				dst = regMap.GetImm32(REGID(d.Rn)) ^ d.Immediate;
				dstimm = true;
			}
			else
			{
				u32 rn = regMap.MapReg(REGID(d.Rn));
				regMap.Lock(rn);

				dst = regMap.AllocTempReg();

				jit_xori_ui(LOCALREG(dst), LOCALREG(rn), d.Immediate);

				regMap.Unlock(rn);
			}

			{
				if (d.FlagsSet & FLAG_C)
					PackCPSRImm(regMap, PSR_C, BIT31(d.Immediate));
			}
		}
		else
		{
			const bool clacCarry = (d.FlagsSet & FLAG_C);
			ShiftOut shift_out = IRShiftOpGenerate(d, regMap, clacCarry);

			if (regMap.IsImm(REGID(d.Rn)) && shift_out.shiftopimm)
			{
				dst = regMap.GetImm32(REGID(d.Rn)) ^ shift_out.shiftop;
				dstimm = true;
			}
			else
			{
				u32 rn = regMap.MapReg(REGID(d.Rn));
				regMap.Lock(rn);

				dst = regMap.AllocTempReg();

				if (shift_out.shiftopimm)
					jit_xori_ui(LOCALREG(dst), LOCALREG(rn), shift_out.shiftop);
				else
					jit_xorr_ui(LOCALREG(dst), LOCALREG(rn), LOCALREG(shift_out.shiftop));

				regMap.Unlock(rn);
			}
			
			{
				if (d.FlagsSet & FLAG_C)
				{
					if (shift_out.cflgimm)
						PackCPSRImm(regMap, PSR_C, shift_out.cflg);
					else
						PackCPSR(regMap, PSR_C, shift_out.cflg);
				}

				shift_out.Clean(regMap);
			}
		}

		{
			if (d.FlagsSet & FLAG_N)
			{
				if (dstimm)
					PackCPSRImm(regMap, PSR_N, BIT31(dst));
				else
				{
					u32 tmp = regMap.AllocTempReg();

					jit_rshi_ui(LOCALREG(tmp), LOCALREG(dst), 31);
					PackCPSR(regMap, PSR_N, tmp);
						
					regMap.ReleaseTempReg(tmp);
				}
			}
			if (d.FlagsSet & FLAG_Z)
			{
				if (dstimm)
					PackCPSRImm(regMap, PSR_Z, dst==0);
				else
				{
					u32 tmp = regMap.AllocTempReg();

					jit_eqi_ui(LOCALREG(tmp), LOCALREG(dst), 0);
					PackCPSR(regMap, PSR_Z, tmp);
						
					regMap.ReleaseTempReg(tmp);
				}
			}
		}

		if (!dstimm)
			regMap.ReleaseTempReg(dst);
	}

	OPDECODER_DECL(IR_ORR)
	{
		u32 PROCNUM = d.ProcessID;

		u32 rd = INVALID_REG_ID;

		if (d.I)
		{
			if (regMap.IsImm(REGID(d.Rn)))
				regMap.SetImm32(REGID(d.Rd), regMap.GetImm32(REGID(d.Rn)) | d.Immediate);
			else
			{
				u32 rn = regMap.MapReg(REGID(d.Rn));
				regMap.Lock(rn);

				rd = regMap.MapReg(REGID(d.Rd), RegisterMap::MAP_DIRTY | RegisterMap::MAP_NOTINIT);
				regMap.Lock(rd);

				jit_ori_ui(LOCALREG(rd), LOCALREG(rn), d.Immediate);

				regMap.Unlock(rn);
			}

			if (d.S && !d.R15Modified)
			{
				if (d.FlagsSet & FLAG_C)
					PackCPSRImm(regMap, PSR_C, BIT31(d.Immediate));
			}
		}
		else
		{
			const bool clacCarry = d.S && !d.R15Modified && (d.FlagsSet & FLAG_C);
			ShiftOut shift_out = IRShiftOpGenerate(d, regMap, clacCarry);

			if (regMap.IsImm(REGID(d.Rn)) && shift_out.shiftopimm)
				regMap.SetImm32(REGID(d.Rd), regMap.GetImm32(REGID(d.Rn)) | shift_out.shiftop);
			else
			{
				u32 rn = regMap.MapReg(REGID(d.Rn));
				regMap.Lock(rn);

				rd = regMap.MapReg(REGID(d.Rd), RegisterMap::MAP_DIRTY | RegisterMap::MAP_NOTINIT);
				regMap.Lock(rd);

				if (shift_out.shiftopimm)
					jit_ori_ui(LOCALREG(rd), LOCALREG(rn), shift_out.shiftop);
				else
					jit_orr_ui(LOCALREG(rd), LOCALREG(rn), LOCALREG(shift_out.shiftop));

				regMap.Unlock(rn);
			}

			if (d.S && !d.R15Modified)
			{
				if (d.FlagsSet & FLAG_C)
				{
					if (shift_out.cflgimm)
						PackCPSRImm(regMap, PSR_C, shift_out.cflg);
					else
						PackCPSR(regMap, PSR_C, shift_out.cflg);
				}
			}

			shift_out.Clean(regMap);
		}

		if (d.S && !d.R15Modified)
		{
			if (d.FlagsSet & FLAG_N)
			{
				if (regMap.IsImm(REGID(d.Rd)))
					PackCPSRImm(regMap, PSR_N, BIT31(regMap.GetImm32(REGID(d.Rd))));
				else
				{
					u32 tmp = regMap.AllocTempReg();

					jit_rshi_ui(LOCALREG(tmp), LOCALREG(rd), 31);
					PackCPSR(regMap, PSR_N, tmp);
						
					regMap.ReleaseTempReg(tmp);
				}
			}
			if (d.FlagsSet & FLAG_Z)
			{
				if (regMap.IsImm(REGID(d.Rd)))
					PackCPSRImm(regMap, PSR_Z, regMap.GetImm32(REGID(d.Rd))==0);
				else
				{
					u32 tmp = regMap.AllocTempReg();

					jit_eqi_ui(LOCALREG(tmp), LOCALREG(rd), 0);
					PackCPSR(regMap, PSR_Z, tmp);
						
					regMap.ReleaseTempReg(tmp);
				}
			}
		}

		if (rd != INVALID_REG_ID)
			regMap.Unlock(rd);

		if (d.R15Modified)
		{
			if (d.S)
			{
				DataProcessLoadCPSRGenerate(d, regMap);
			}

			R15ModifiedGenerate(d, regMap);
		}
	}

	OPDECODER_DECL(IR_BIC)
	{
		u32 PROCNUM = d.ProcessID;

		u32 rd = INVALID_REG_ID;

		if (d.I)
		{
			if (regMap.IsImm(REGID(d.Rn)))
				regMap.SetImm32(REGID(d.Rd), regMap.GetImm32(REGID(d.Rn)) & (~d.Immediate));
			else
			{
				u32 rn = regMap.MapReg(REGID(d.Rn));
				regMap.Lock(rn);

				rd = regMap.MapReg(REGID(d.Rd), RegisterMap::MAP_DIRTY | RegisterMap::MAP_NOTINIT);
				regMap.Lock(rd);

				jit_andi_ui(LOCALREG(rd), LOCALREG(rn), ~d.Immediate);

				regMap.Unlock(rn);
			}

			if (d.S && !d.R15Modified)
			{
				if (d.FlagsSet & FLAG_C)
					PackCPSRImm(regMap, PSR_C, BIT31(d.Immediate));
			}
		}
		else
		{
			const bool clacCarry = d.S && !d.R15Modified && (d.FlagsSet & FLAG_C);
			ShiftOut shift_out = IRShiftOpGenerate(d, regMap, clacCarry);

			if (regMap.IsImm(REGID(d.Rn)) && shift_out.shiftopimm)
				regMap.SetImm32(REGID(d.Rd), regMap.GetImm32(REGID(d.Rn)) & (~shift_out.shiftop));
			else
			{
				u32 rn = regMap.MapReg(REGID(d.Rn));
				regMap.Lock(rn);

				rd = regMap.MapReg(REGID(d.Rd), RegisterMap::MAP_DIRTY | RegisterMap::MAP_NOTINIT);
				regMap.Lock(rd);

				if (shift_out.shiftopimm)
					jit_andi_ui(LOCALREG(rd), LOCALREG(rn), ~shift_out.shiftop);
				else
				{
					jit_notr_ui(LOCALREG(shift_out.shiftop), LOCALREG(shift_out.shiftop));
					jit_andr_ui(LOCALREG(rd), LOCALREG(rn), LOCALREG(shift_out.shiftop));
				}

				regMap.Unlock(rn);
			}

			if (d.S && !d.R15Modified)
			{
				if (d.FlagsSet & FLAG_C)
				{
					if (shift_out.cflgimm)
						PackCPSRImm(regMap, PSR_C, shift_out.cflg);
					else
						PackCPSR(regMap, PSR_C, shift_out.cflg);
				}
			}

			shift_out.Clean(regMap);
		}

		if (d.S && !d.R15Modified)
		{
			if (d.FlagsSet & FLAG_N)
			{
				if (regMap.IsImm(REGID(d.Rd)))
					PackCPSRImm(regMap, PSR_N, BIT31(regMap.GetImm32(REGID(d.Rd))));
				else
				{
					u32 tmp = regMap.AllocTempReg();

					jit_rshi_ui(LOCALREG(tmp), LOCALREG(rd), 31);
					PackCPSR(regMap, PSR_N, tmp);
						
					regMap.ReleaseTempReg(tmp);
				}
			}
			if (d.FlagsSet & FLAG_Z)
			{
				if (regMap.IsImm(REGID(d.Rd)))
					PackCPSRImm(regMap, PSR_Z, regMap.GetImm32(REGID(d.Rd))==0);
				else
				{
					u32 tmp = regMap.AllocTempReg();

					jit_eqi_ui(LOCALREG(tmp), LOCALREG(rd), 0);
					PackCPSR(regMap, PSR_Z, tmp);
						
					regMap.ReleaseTempReg(tmp);
				}
			}
		}

		if (rd != INVALID_REG_ID)
			regMap.Unlock(rd);

		if (d.R15Modified)
		{
			if (d.S)
			{
				DataProcessLoadCPSRGenerate(d, regMap);
			}

			R15ModifiedGenerate(d, regMap);
		}
	}

	OPDECODER_DECL(IR_ADD)
	{
		u32 PROCNUM = d.ProcessID;

		u32 rd = INVALID_REG_ID;

		if (d.I)
		{
			if (regMap.IsImm(REGID(d.Rn)))
			{
				u32 v = regMap.GetImm32(REGID(d.Rn));

				regMap.SetImm32(REGID(d.Rd), regMap.GetImm32(REGID(d.Rn)) + d.Immediate);

				if (d.S && !d.R15Modified)
				{
					if (d.FlagsSet & FLAG_C)
						PackCPSRImm(regMap, PSR_C, CarryFrom(v, d.Immediate));
					if (d.FlagsSet & FLAG_V)
						PackCPSRImm(regMap, PSR_V, OverflowFromADD(regMap.GetImm32(REGID(d.Rd)), v, d.Immediate));
				}
			}
			else
			{
				u32 rn = regMap.MapReg(REGID(d.Rn));
				regMap.Lock(rn);

				rd = regMap.MapReg(REGID(d.Rd), RegisterMap::MAP_DIRTY | RegisterMap::MAP_NOTINIT);
				regMap.Lock(rd);

				u32 v_tmp = INVALID_REG_ID;
				u32 c_tmp = INVALID_REG_ID;

				if (d.S && !d.R15Modified && ((d.FlagsSet & FLAG_V)))
				{
					v_tmp = regMap.AllocTempReg();

					jit_movr_ui(LOCALREG(v_tmp), LOCALREG(rn));
				}

				if (d.S && !d.R15Modified && ((d.FlagsSet & FLAG_C)))
				{
					c_tmp = regMap.AllocTempReg();

					jit_movi_ui(LOCALREG(c_tmp), 0);
					jit_addci_ui(LOCALREG(rd), LOCALREG(rn), d.Immediate);
					jit_addxi_ui(LOCALREG(c_tmp), LOCALREG(c_tmp), 0);
				}
				else
					jit_addi_ui(LOCALREG(rd), LOCALREG(rn), d.Immediate);

				regMap.Unlock(rn);

				if (d.S && !d.R15Modified)
				{
					if (d.FlagsSet & FLAG_C)
					{
						PackCPSR(regMap, PSR_C, c_tmp);

						regMap.ReleaseTempReg(c_tmp);
					}
					if (d.FlagsSet & FLAG_V)
					{
						u32 tmp = regMap.AllocTempReg();

						jit_xori_ui(LOCALREG(tmp), LOCALREG(v_tmp), d.Immediate);
						jit_notr_ui(LOCALREG(tmp), LOCALREG(tmp));
						jit_xori_ui(LOCALREG(v_tmp), LOCALREG(rd), d.Immediate);
						jit_andr_ui(LOCALREG(v_tmp), LOCALREG(tmp), LOCALREG(v_tmp));
						jit_rshi_ui(LOCALREG(v_tmp), LOCALREG(v_tmp), 31);

						regMap.ReleaseTempReg(tmp);

						PackCPSR(regMap, PSR_V, v_tmp);

						regMap.ReleaseTempReg(v_tmp);
					}
				}
			}
		}
		else
		{
			ShiftOut shift_out = IRShiftOpGenerate(d, regMap, false);

			if (regMap.IsImm(REGID(d.Rn)) && shift_out.shiftopimm)
			{
				u32 v = regMap.GetImm32(REGID(d.Rn));

				regMap.SetImm32(REGID(d.Rd), regMap.GetImm32(REGID(d.Rn)) + shift_out.shiftop);

				if (d.S && !d.R15Modified)
				{
					if (d.FlagsSet & FLAG_C)
						PackCPSRImm(regMap, PSR_C, CarryFrom(v, shift_out.shiftop));
					if (d.FlagsSet & FLAG_V)
						PackCPSRImm(regMap, PSR_V, OverflowFromADD(regMap.GetImm32(REGID(d.Rd)), v, shift_out.shiftop));
				}
			}
			else
			{
				u32 rn = regMap.MapReg(REGID(d.Rn));
				regMap.Lock(rn);

				rd = regMap.MapReg(REGID(d.Rd), RegisterMap::MAP_DIRTY | RegisterMap::MAP_NOTINIT);
				regMap.Lock(rd);

				u32 v_tmp = INVALID_REG_ID;
				u32 c_tmp = INVALID_REG_ID;

				if (d.S && !d.R15Modified && ((d.FlagsSet & FLAG_V)))
				{
					v_tmp = regMap.AllocTempReg();

					jit_movr_ui(LOCALREG(v_tmp), LOCALREG(rn));
				}

				if (d.S && !d.R15Modified && ((d.FlagsSet & FLAG_C)))
				{
					c_tmp = regMap.AllocTempReg();

					jit_movi_ui(LOCALREG(c_tmp), 0);
					if (shift_out.shiftopimm)
						jit_addci_ui(LOCALREG(rd), LOCALREG(rn), shift_out.shiftop);
					else
						jit_addcr_ui(LOCALREG(rd), LOCALREG(rn), LOCALREG(shift_out.shiftop));
					jit_addxi_ui(LOCALREG(c_tmp), LOCALREG(c_tmp), 0);
				}
				else
				{
					if (shift_out.shiftopimm)
						jit_addi_ui(LOCALREG(rd), LOCALREG(rn), shift_out.shiftop);
					else
						jit_addr_ui(LOCALREG(rd), LOCALREG(rn), LOCALREG(shift_out.shiftop));
				}

				regMap.Unlock(rn);

				if (d.S && !d.R15Modified)
				{
					if (d.FlagsSet & FLAG_C)
					{
						PackCPSR(regMap, PSR_C, c_tmp);

						regMap.ReleaseTempReg(c_tmp);
					}
					if (d.FlagsSet & FLAG_V)
					{
						u32 tmp = regMap.AllocTempReg();

						if (shift_out.shiftopimm)
						{
							jit_xori_ui(LOCALREG(tmp), LOCALREG(v_tmp), shift_out.shiftop);
							jit_notr_ui(LOCALREG(tmp), LOCALREG(tmp));
							jit_xori_ui(LOCALREG(v_tmp), LOCALREG(rd), shift_out.shiftop);
							jit_andr_ui(LOCALREG(v_tmp), LOCALREG(tmp), LOCALREG(v_tmp));
							jit_rshi_ui(LOCALREG(v_tmp), LOCALREG(v_tmp), 31);
						}
						else
						{
							jit_xorr_ui(LOCALREG(tmp), LOCALREG(v_tmp), LOCALREG(shift_out.shiftop));
							jit_notr_ui(LOCALREG(tmp), LOCALREG(tmp));
							jit_xorr_ui(LOCALREG(v_tmp), LOCALREG(rd), LOCALREG(shift_out.shiftop));
							jit_andr_ui(LOCALREG(v_tmp), LOCALREG(tmp), LOCALREG(v_tmp));
							jit_rshi_ui(LOCALREG(v_tmp), LOCALREG(v_tmp), 31);
						}

						regMap.ReleaseTempReg(tmp);

						PackCPSR(regMap, PSR_V, v_tmp);

						regMap.ReleaseTempReg(v_tmp);
					}
				}
			}

			shift_out.Clean(regMap);
		}

		if (d.S && !d.R15Modified)
		{
			if (d.FlagsSet & FLAG_N)
			{
				if (regMap.IsImm(REGID(d.Rd)))
					PackCPSRImm(regMap, PSR_N, BIT31(regMap.GetImm32(REGID(d.Rd))));
				else
				{
					u32 tmp = regMap.AllocTempReg();

					jit_rshi_ui(LOCALREG(tmp), LOCALREG(rd), 31);
					PackCPSR(regMap, PSR_N, tmp);
						
					regMap.ReleaseTempReg(tmp);
				}
			}
			if (d.FlagsSet & FLAG_Z)
			{
				if (regMap.IsImm(REGID(d.Rd)))
					PackCPSRImm(regMap, PSR_Z, regMap.GetImm32(REGID(d.Rd))==0);
				else
				{
					u32 tmp = regMap.AllocTempReg();

					jit_eqi_ui(LOCALREG(tmp), LOCALREG(rd), 0);
					PackCPSR(regMap, PSR_Z, tmp);
						
					regMap.ReleaseTempReg(tmp);
				}
			}
		}

		if (rd != INVALID_REG_ID)
			regMap.Unlock(rd);

		if (d.R15Modified)
		{
			if (d.S)
			{
				DataProcessLoadCPSRGenerate(d, regMap);
			}

			R15ModifiedGenerate(d, regMap);
		}
	}

	OPDECODER_DECL(IR_ADC)
	{
		u32 PROCNUM = d.ProcessID;

		u32 rd = INVALID_REG_ID;

		if (d.I)
		{
			{
				u32 rn = regMap.MapReg(REGID(d.Rn));
				regMap.Lock(rn);

				u32 rn_tmp = regMap.AllocTempReg();

				jit_movr_ui(LOCALREG(rn_tmp), LOCALREG(rn));

				regMap.Unlock(rn);

				u32 cflg = regMap.AllocTempReg();
				UnpackCPSR(regMap, PSR_C, cflg);

				rd = regMap.MapReg(REGID(d.Rd), RegisterMap::MAP_DIRTY | RegisterMap::MAP_NOTINIT);
				regMap.Lock(rd);
				
				u32 c_tmp = INVALID_REG_ID;

				if (d.S && !d.R15Modified && ((d.FlagsSet & FLAG_C)))
				{
					c_tmp = regMap.AllocTempReg();

					jit_movi_ui(LOCALREG(c_tmp), 0);
					jit_addci_ui(LOCALREG(rd), LOCALREG(rn_tmp), d.Immediate);
					jit_addxi_ui(LOCALREG(c_tmp), LOCALREG(c_tmp), 0);
					jit_addcr_ui(LOCALREG(rd), LOCALREG(rd), LOCALREG(cflg));
					jit_addxi_ui(LOCALREG(c_tmp), LOCALREG(c_tmp), 0);
					jit_nei_ui(LOCALREG(c_tmp), LOCALREG(c_tmp), 0);
				}
				else
				{
					jit_addi_ui(LOCALREG(rd), LOCALREG(rn_tmp), d.Immediate);
					jit_addr_ui(LOCALREG(rd), LOCALREG(rd), LOCALREG(cflg));
				}

				regMap.ReleaseTempReg(cflg);

				if (d.S && !d.R15Modified)
				{
					if (d.FlagsSet & FLAG_C)
					{
						PackCPSR(regMap, PSR_C, c_tmp);

						regMap.ReleaseTempReg(c_tmp);
					}
					if (d.FlagsSet & FLAG_V)
					{
						u32 tmp = regMap.AllocTempReg();

						jit_xori_ui(LOCALREG(tmp), LOCALREG(rn_tmp), d.Immediate);
						jit_notr_ui(LOCALREG(tmp), LOCALREG(tmp));
						jit_xori_ui(LOCALREG(rn_tmp), LOCALREG(rd), d.Immediate);
						jit_andr_ui(LOCALREG(rn_tmp), LOCALREG(tmp), LOCALREG(rn_tmp));
						jit_rshi_ui(LOCALREG(rn_tmp), LOCALREG(rn_tmp), 31);

						regMap.ReleaseTempReg(tmp);

						PackCPSR(regMap, PSR_V, rn_tmp);
					}
				}

				regMap.ReleaseTempReg(rn_tmp);
			}
		}
		else
		{
			ShiftOut shift_out = IRShiftOpGenerate(d, regMap, false);

			{
				u32 rn = regMap.MapReg(REGID(d.Rn));
				regMap.Lock(rn);

				u32 rn_tmp = regMap.AllocTempReg();

				jit_movr_ui(LOCALREG(rn_tmp), LOCALREG(rn));

				regMap.Unlock(rn);

				u32 cflg = regMap.AllocTempReg();
				UnpackCPSR(regMap, PSR_C, cflg);

				rd = regMap.MapReg(REGID(d.Rd), RegisterMap::MAP_DIRTY | RegisterMap::MAP_NOTINIT);
				regMap.Lock(rd);
				
				u32 c_tmp = INVALID_REG_ID;

				if (d.S && !d.R15Modified && ((d.FlagsSet & FLAG_C)))
				{
					c_tmp = regMap.AllocTempReg();

					jit_movi_ui(LOCALREG(c_tmp), 0);
					if (shift_out.shiftopimm)
						jit_addci_ui(LOCALREG(rd), LOCALREG(rn_tmp), shift_out.shiftop);
					else
						jit_addcr_ui(LOCALREG(rd), LOCALREG(rn_tmp), LOCALREG(shift_out.shiftop));
					jit_addxi_ui(LOCALREG(c_tmp), LOCALREG(c_tmp), 0);
					jit_addcr_ui(LOCALREG(rd), LOCALREG(rd), LOCALREG(cflg));
					jit_addxi_ui(LOCALREG(c_tmp), LOCALREG(c_tmp), 0);
					jit_nei_ui(LOCALREG(c_tmp), LOCALREG(c_tmp), 0);
				}
				else
				{
					if (shift_out.shiftopimm)
						jit_addi_ui(LOCALREG(rd), LOCALREG(rn_tmp), shift_out.shiftop);
					else
						jit_addr_ui(LOCALREG(rd), LOCALREG(rn_tmp), LOCALREG(shift_out.shiftop));
					jit_addr_ui(LOCALREG(rd), LOCALREG(rd), LOCALREG(cflg));
				}

				regMap.ReleaseTempReg(cflg);

				if (d.S && !d.R15Modified)
				{
					if (d.FlagsSet & FLAG_C)
					{
						PackCPSR(regMap, PSR_C, c_tmp);

						regMap.ReleaseTempReg(c_tmp);
					}
					if (d.FlagsSet & FLAG_V)
					{
						u32 tmp = regMap.AllocTempReg();

						if (shift_out.shiftopimm)
						{
							jit_xori_ui(LOCALREG(tmp), LOCALREG(rn_tmp), shift_out.shiftop);
							jit_notr_ui(LOCALREG(tmp), LOCALREG(tmp));
							jit_xori_ui(LOCALREG(rn_tmp), LOCALREG(rd), shift_out.shiftop);
							jit_andr_ui(LOCALREG(rn_tmp), LOCALREG(tmp), LOCALREG(rn_tmp));
							jit_rshi_ui(LOCALREG(rn_tmp), LOCALREG(rn_tmp), 31);
						}
						else
						{
							jit_xorr_ui(LOCALREG(tmp), LOCALREG(rn_tmp), LOCALREG(shift_out.shiftop));
							jit_notr_ui(LOCALREG(tmp), LOCALREG(tmp));
							jit_xorr_ui(LOCALREG(rn_tmp), LOCALREG(rd), LOCALREG(shift_out.shiftop));
							jit_andr_ui(LOCALREG(rn_tmp), LOCALREG(tmp), LOCALREG(rn_tmp));
							jit_rshi_ui(LOCALREG(rn_tmp), LOCALREG(rn_tmp), 31);
						}

						regMap.ReleaseTempReg(tmp);

						PackCPSR(regMap, PSR_V, rn_tmp);
					}
				}

				regMap.ReleaseTempReg(rn_tmp);
			}

			shift_out.Clean(regMap);
		}

		if (d.S && !d.R15Modified)
		{
			if (d.FlagsSet & FLAG_N)
			{
				if (regMap.IsImm(REGID(d.Rd)))
					PackCPSRImm(regMap, PSR_N, BIT31(regMap.GetImm32(REGID(d.Rd))));
				else
				{
					u32 tmp = regMap.AllocTempReg();

					jit_rshi_ui(LOCALREG(tmp), LOCALREG(rd), 31);
					PackCPSR(regMap, PSR_N, tmp);
						
					regMap.ReleaseTempReg(tmp);
				}
			}
			if (d.FlagsSet & FLAG_Z)
			{
				if (regMap.IsImm(REGID(d.Rd)))
					PackCPSRImm(regMap, PSR_Z, regMap.GetImm32(REGID(d.Rd))==0);
				else
				{
					u32 tmp = regMap.AllocTempReg();

					jit_eqi_ui(LOCALREG(tmp), LOCALREG(rd), 0);
					PackCPSR(regMap, PSR_Z, tmp);
						
					regMap.ReleaseTempReg(tmp);
				}
			}
		}

		if (rd != INVALID_REG_ID)
			regMap.Unlock(rd);

		if (d.R15Modified)
		{
			if (d.S)
			{
				DataProcessLoadCPSRGenerate(d, regMap);
			}

			R15ModifiedGenerate(d, regMap);
		}
	}

	OPDECODER_DECL(IR_SUB)
	{
		u32 PROCNUM = d.ProcessID;

		u32 rd = INVALID_REG_ID;

		if (d.I)
		{
			if (regMap.IsImm(REGID(d.Rn)))
			{
				u32 v = regMap.GetImm32(REGID(d.Rn));

				regMap.SetImm32(REGID(d.Rd), regMap.GetImm32(REGID(d.Rn)) - d.Immediate);

				if (d.S && !d.R15Modified)
				{
					if (d.FlagsSet & FLAG_C)
						PackCPSRImm(regMap, PSR_C, !BorrowFrom(v, d.Immediate));
					if (d.FlagsSet & FLAG_V)
						PackCPSRImm(regMap, PSR_V, OverflowFromSUB(regMap.GetImm32(REGID(d.Rd)), v, d.Immediate));
				}
			}
			else
			{
				u32 rn = regMap.MapReg(REGID(d.Rn));
				regMap.Lock(rn);

				rd = regMap.MapReg(REGID(d.Rd), RegisterMap::MAP_DIRTY | RegisterMap::MAP_NOTINIT);
				regMap.Lock(rd);

				u32 v_tmp = INVALID_REG_ID;
				u32 c_tmp = INVALID_REG_ID;

				if (d.S && !d.R15Modified && ((d.FlagsSet & FLAG_V)))
				{
					v_tmp = regMap.AllocTempReg();

					jit_movr_ui(LOCALREG(v_tmp), LOCALREG(rn));
				}

				if (d.S && !d.R15Modified && ((d.FlagsSet & FLAG_C)))
				{
					c_tmp = regMap.AllocTempReg();

					jit_gei_ui(LOCALREG(c_tmp), LOCALREG(rn), d.Immediate);
					jit_subi_ui(LOCALREG(rd), LOCALREG(rn), d.Immediate);
				}
				else
					jit_subi_ui(LOCALREG(rd), LOCALREG(rn), d.Immediate);

				regMap.Unlock(rn);

				if (d.S && !d.R15Modified)
				{
					if (d.FlagsSet & FLAG_C)
					{
						PackCPSR(regMap, PSR_C, c_tmp);

						regMap.ReleaseTempReg(c_tmp);
					}
					if (d.FlagsSet & FLAG_V)
					{
						u32 tmp = regMap.AllocTempReg();

						jit_xori_ui(LOCALREG(tmp), LOCALREG(v_tmp), d.Immediate);
						jit_xorr_ui(LOCALREG(v_tmp), LOCALREG(rd), LOCALREG(v_tmp));
						jit_andr_ui(LOCALREG(v_tmp), LOCALREG(tmp), LOCALREG(v_tmp));
						jit_rshi_ui(LOCALREG(v_tmp), LOCALREG(v_tmp), 31);

						regMap.ReleaseTempReg(tmp);

						PackCPSR(regMap, PSR_V, v_tmp);

						regMap.ReleaseTempReg(v_tmp);
					}
				}
			}
		}
		else
		{
			ShiftOut shift_out = IRShiftOpGenerate(d, regMap, false);

			if (regMap.IsImm(REGID(d.Rn)) && shift_out.shiftopimm)
			{
				u32 v = regMap.GetImm32(REGID(d.Rn));

				regMap.SetImm32(REGID(d.Rd), regMap.GetImm32(REGID(d.Rn)) - shift_out.shiftop);

				if (d.S && !d.R15Modified)
				{
					if (d.FlagsSet & FLAG_C)
						PackCPSRImm(regMap, PSR_C, !BorrowFrom(v, shift_out.shiftop));
					if (d.FlagsSet & FLAG_V)
						PackCPSRImm(regMap, PSR_V, OverflowFromSUB(regMap.GetImm32(REGID(d.Rd)), v, shift_out.shiftop));
				}
			}
			else
			{
				u32 rn = regMap.MapReg(REGID(d.Rn));
				regMap.Lock(rn);

				rd = regMap.MapReg(REGID(d.Rd), RegisterMap::MAP_DIRTY | RegisterMap::MAP_NOTINIT);
				regMap.Lock(rd);

				u32 v_tmp = INVALID_REG_ID;
				u32 c_tmp = INVALID_REG_ID;

				if (d.S && !d.R15Modified && ((d.FlagsSet & FLAG_V)))
				{
					v_tmp = regMap.AllocTempReg();

					jit_movr_ui(LOCALREG(v_tmp), LOCALREG(rn));
				}

				if (d.S && !d.R15Modified && ((d.FlagsSet & FLAG_C)))
				{
					c_tmp = regMap.AllocTempReg();

					if (shift_out.shiftopimm)
					{
						jit_gei_ui(LOCALREG(c_tmp), LOCALREG(rn), shift_out.shiftop);
						jit_subi_ui(LOCALREG(rd), LOCALREG(rn), shift_out.shiftop);
					}
					else
					{
						jit_ger_ui(LOCALREG(c_tmp), LOCALREG(rn), LOCALREG(shift_out.shiftop));
						jit_subr_ui(LOCALREG(rd), LOCALREG(rn), LOCALREG(shift_out.shiftop));
					}
				}
				else
				{
					if (shift_out.shiftopimm)
						jit_subi_ui(LOCALREG(rd), LOCALREG(rn), shift_out.shiftop);
					else
						jit_subr_ui(LOCALREG(rd), LOCALREG(rn), LOCALREG(shift_out.shiftop));
				}

				regMap.Unlock(rn);

				if (d.S && !d.R15Modified)
				{
					if (d.FlagsSet & FLAG_C)
					{
						PackCPSR(regMap, PSR_C, c_tmp);

						regMap.ReleaseTempReg(c_tmp);
					}
					if (d.FlagsSet & FLAG_V)
					{
						u32 tmp = regMap.AllocTempReg();

						if (shift_out.shiftopimm)
						{
							jit_xori_ui(LOCALREG(tmp), LOCALREG(v_tmp), shift_out.shiftop);
							jit_xorr_ui(LOCALREG(v_tmp), LOCALREG(rd), LOCALREG(v_tmp));
							jit_andr_ui(LOCALREG(v_tmp), LOCALREG(tmp), LOCALREG(v_tmp));
							jit_rshi_ui(LOCALREG(v_tmp), LOCALREG(v_tmp), 31);
						}
						else
						{
							jit_xorr_ui(LOCALREG(tmp), LOCALREG(v_tmp), LOCALREG(shift_out.shiftop));
							jit_xorr_ui(LOCALREG(v_tmp), LOCALREG(rd), LOCALREG(v_tmp));
							jit_andr_ui(LOCALREG(v_tmp), LOCALREG(tmp), LOCALREG(v_tmp));
							jit_rshi_ui(LOCALREG(v_tmp), LOCALREG(v_tmp), 31);
						}

						regMap.ReleaseTempReg(tmp);

						PackCPSR(regMap, PSR_V, v_tmp);

						regMap.ReleaseTempReg(v_tmp);
					}
				}
			}

			shift_out.Clean(regMap);
		}

		if (d.S && !d.R15Modified)
		{
			if (d.FlagsSet & FLAG_N)
			{
				if (regMap.IsImm(REGID(d.Rd)))
					PackCPSRImm(regMap, PSR_N, BIT31(regMap.GetImm32(REGID(d.Rd))));
				else
				{
					u32 tmp = regMap.AllocTempReg();

					jit_rshi_ui(LOCALREG(tmp), LOCALREG(rd), 31);
					PackCPSR(regMap, PSR_N, tmp);
						
					regMap.ReleaseTempReg(tmp);
				}
			}
			if (d.FlagsSet & FLAG_Z)
			{
				if (regMap.IsImm(REGID(d.Rd)))
					PackCPSRImm(regMap, PSR_Z, regMap.GetImm32(REGID(d.Rd))==0);
				else
				{
					u32 tmp = regMap.AllocTempReg();

					jit_eqi_ui(LOCALREG(tmp), LOCALREG(rd), 0);
					PackCPSR(regMap, PSR_Z, tmp);
						
					regMap.ReleaseTempReg(tmp);
				}
			}
		}

		if (rd != INVALID_REG_ID)
			regMap.Unlock(rd);

		if (d.R15Modified)
		{
			if (d.S)
			{
				DataProcessLoadCPSRGenerate(d, regMap);
			}

			R15ModifiedGenerate(d, regMap);
		}
	}

	OPDECODER_DECL(IR_SBC)
	{
		u32 PROCNUM = d.ProcessID;

		u32 rd = INVALID_REG_ID;

		if (d.I)
		{
			{
				u32 rn = regMap.MapReg(REGID(d.Rn));
				regMap.Lock(rn);

				u32 rn_tmp = regMap.AllocTempReg();
				jit_movr_ui(LOCALREG(rn_tmp), LOCALREG(rn));

				regMap.Unlock(rn);

				u32 cflg = regMap.AllocTempReg();
				UnpackCPSR(regMap, PSR_C, cflg);
				jit_xori_ui(LOCALREG(cflg), LOCALREG(cflg), 1);

				rd = regMap.MapReg(REGID(d.Rd), RegisterMap::MAP_DIRTY | RegisterMap::MAP_NOTINIT);
				regMap.Lock(rd);

				jit_subi_ui(LOCALREG(rd), LOCALREG(rn_tmp), d.Immediate);
				jit_subr_ui(LOCALREG(rd), LOCALREG(rd), LOCALREG(cflg));

				if (d.S && !d.R15Modified)
				{
					if (d.FlagsSet & FLAG_C)
					{
						jit_insn *eq0 = jit_beqi_ui(jit_forward(), LOCALREG(cflg), 1);
						jit_gei_ui(LOCALREG(cflg), LOCALREG(rn_tmp), d.Immediate);
						jit_insn *done = jit_jmpi(jit_forward());
						jit_patch(eq0);
						jit_gti_ui(LOCALREG(cflg), LOCALREG(rn_tmp), d.Immediate);
						jit_patch(done);

						PackCPSR(regMap, PSR_C, cflg);
					}
					if (d.FlagsSet & FLAG_V)
					{
						u32 tmp = cflg;

						jit_xori_ui(LOCALREG(tmp), LOCALREG(rn_tmp), d.Immediate);
						jit_xorr_ui(LOCALREG(rn_tmp), LOCALREG(rd), LOCALREG(rn_tmp));
						jit_andr_ui(LOCALREG(rn_tmp), LOCALREG(tmp), LOCALREG(rn_tmp));
						jit_rshi_ui(LOCALREG(rn_tmp), LOCALREG(rn_tmp), 31);

						PackCPSR(regMap, PSR_V, rn_tmp);
					}
				}

				regMap.ReleaseTempReg(cflg);
				regMap.ReleaseTempReg(rn_tmp);
			}
		}
		else
		{
			ShiftOut shift_out = IRShiftOpGenerate(d, regMap, false);

			{
				u32 rn = regMap.MapReg(REGID(d.Rn));
				regMap.Lock(rn);

				u32 rn_tmp = regMap.AllocTempReg();

				jit_movr_ui(LOCALREG(rn_tmp), LOCALREG(rn));

				regMap.Unlock(rn);

				u32 cflg = regMap.AllocTempReg();
				UnpackCPSR(regMap, PSR_C, cflg);
				jit_xori_ui(LOCALREG(cflg), LOCALREG(cflg), 1);

				rd = regMap.MapReg(REGID(d.Rd), RegisterMap::MAP_DIRTY | RegisterMap::MAP_NOTINIT);
				regMap.Lock(rd);

				if (shift_out.shiftopimm)
					jit_subi_ui(LOCALREG(rd), LOCALREG(rn_tmp), shift_out.shiftop);
				else
					jit_subr_ui(LOCALREG(rd), LOCALREG(rn_tmp), LOCALREG(shift_out.shiftop));
				jit_subr_ui(LOCALREG(rd), LOCALREG(rd), LOCALREG(cflg));

				if (d.S && !d.R15Modified)
				{
					if (d.FlagsSet & FLAG_C)
					{
						if (shift_out.shiftopimm)
						{
							jit_insn *eq0 = jit_beqi_ui(jit_forward(), LOCALREG(cflg), 1);
							jit_gei_ui(LOCALREG(cflg), LOCALREG(rn_tmp), shift_out.shiftop);
							jit_insn *done = jit_jmpi(jit_forward());
							jit_patch(eq0);
							jit_gti_ui(LOCALREG(cflg), LOCALREG(rn_tmp), shift_out.shiftop);
							jit_patch(done);
						}
						else
						{
							jit_insn *eq0 = jit_beqi_ui(jit_forward(), LOCALREG(cflg), 1);
							jit_ger_ui(LOCALREG(cflg), LOCALREG(rn_tmp), LOCALREG(shift_out.shiftop));
							jit_insn *done = jit_jmpi(jit_forward());
							jit_patch(eq0);
							jit_gtr_ui(LOCALREG(cflg), LOCALREG(rn_tmp), LOCALREG(shift_out.shiftop));
							jit_patch(done);
						}

						PackCPSR(regMap, PSR_C, cflg);
					}
					if (d.FlagsSet & FLAG_V)
					{
						u32 tmp = cflg;

						if (shift_out.shiftopimm)
						{
							jit_xori_ui(LOCALREG(tmp), LOCALREG(rn_tmp), shift_out.shiftop);
							jit_xorr_ui(LOCALREG(rn_tmp), LOCALREG(rd), LOCALREG(rn_tmp));
							jit_andr_ui(LOCALREG(rn_tmp), LOCALREG(tmp), LOCALREG(rn_tmp));
							jit_rshi_ui(LOCALREG(rn_tmp), LOCALREG(rn_tmp), 31);
						}
						else
						{
							jit_xorr_ui(LOCALREG(tmp), LOCALREG(rn_tmp), LOCALREG(shift_out.shiftop));
							jit_xorr_ui(LOCALREG(rn_tmp), LOCALREG(rd), LOCALREG(rn_tmp));
							jit_andr_ui(LOCALREG(rn_tmp), LOCALREG(tmp), LOCALREG(rn_tmp));
							jit_rshi_ui(LOCALREG(rn_tmp), LOCALREG(rn_tmp), 31);
						}

						PackCPSR(regMap, PSR_V, rn_tmp);
					}
				}

				regMap.ReleaseTempReg(cflg);
				regMap.ReleaseTempReg(rn_tmp);
			}

			shift_out.Clean(regMap);
		}

		if (d.S && !d.R15Modified)
		{
			if (d.FlagsSet & FLAG_N)
			{
				if (regMap.IsImm(REGID(d.Rd)))
					PackCPSRImm(regMap, PSR_N, BIT31(regMap.GetImm32(REGID(d.Rd))));
				else
				{
					u32 tmp = regMap.AllocTempReg();

					jit_rshi_ui(LOCALREG(tmp), LOCALREG(rd), 31);
					PackCPSR(regMap, PSR_N, tmp);
						
					regMap.ReleaseTempReg(tmp);
				}
			}
			if (d.FlagsSet & FLAG_Z)
			{
				if (regMap.IsImm(REGID(d.Rd)))
					PackCPSRImm(regMap, PSR_Z, regMap.GetImm32(REGID(d.Rd))==0);
				else
				{
					u32 tmp = regMap.AllocTempReg();

					jit_eqi_ui(LOCALREG(tmp), LOCALREG(rd), 0);
					PackCPSR(regMap, PSR_Z, tmp);
						
					regMap.ReleaseTempReg(tmp);
				}
			}
		}

		if (rd != INVALID_REG_ID)
			regMap.Unlock(rd);

		if (d.R15Modified)
		{
			if (d.S)
			{
				DataProcessLoadCPSRGenerate(d, regMap);
			}

			R15ModifiedGenerate(d, regMap);
		}
	}

	OPDECODER_DECL(IR_RSB)
	{
		u32 PROCNUM = d.ProcessID;

		u32 rd = INVALID_REG_ID;

		if (d.I)
		{
			if (regMap.IsImm(REGID(d.Rn)))
			{
				u32 v = regMap.GetImm32(REGID(d.Rn));

				regMap.SetImm32(REGID(d.Rd), d.Immediate - v);

				if (d.S && !d.R15Modified)
				{
					if (d.FlagsSet & FLAG_C)
						PackCPSRImm(regMap, PSR_C, !BorrowFrom(d.Immediate, v));
					if (d.FlagsSet & FLAG_V)
						PackCPSRImm(regMap, PSR_V, OverflowFromSUB(regMap.GetImm32(REGID(d.Rd)), d.Immediate, v));
				}
			}
			else
			{
				u32 rn = regMap.MapReg(REGID(d.Rn));
				regMap.Lock(rn);

				rd = regMap.MapReg(REGID(d.Rd), RegisterMap::MAP_DIRTY | RegisterMap::MAP_NOTINIT);
				regMap.Lock(rd);

				u32 v_tmp = INVALID_REG_ID;

				if (d.S && !d.R15Modified && ((d.FlagsSet & FLAG_C) || (d.FlagsSet & FLAG_V)))
				{
					v_tmp = regMap.AllocTempReg();

					jit_movr_ui(LOCALREG(v_tmp), LOCALREG(rn));
				}

				jit_rsbi_ui(LOCALREG(rd), LOCALREG(rn), d.Immediate);

				regMap.Unlock(rn);

				if (d.S && !d.R15Modified)
				{
					if (d.FlagsSet & FLAG_C)
					{
						u32 tmp = regMap.AllocTempReg();

						jit_lei_ui(LOCALREG(tmp), LOCALREG(v_tmp), d.Immediate);
						PackCPSR(regMap, PSR_C, tmp);

						regMap.ReleaseTempReg(tmp);
					}
					if (d.FlagsSet & FLAG_V)
					{
						u32 tmp = regMap.AllocTempReg();

						jit_xori_ui(LOCALREG(tmp), LOCALREG(v_tmp), d.Immediate);
						jit_xori_ui(LOCALREG(v_tmp), LOCALREG(rd), d.Immediate);
						jit_andr_ui(LOCALREG(v_tmp), LOCALREG(tmp), LOCALREG(v_tmp));
						jit_rshi_ui(LOCALREG(v_tmp), LOCALREG(v_tmp), 31);

						regMap.ReleaseTempReg(tmp);

						PackCPSR(regMap, PSR_V, v_tmp);
					}
				}

				if (v_tmp != INVALID_REG_ID)
					regMap.ReleaseTempReg(v_tmp);
			}
		}
		else
		{
			ShiftOut shift_out = IRShiftOpGenerate(d, regMap, false);

			if (regMap.IsImm(REGID(d.Rn)) && shift_out.shiftopimm)
			{
				u32 v = regMap.GetImm32(REGID(d.Rn));

				regMap.SetImm32(REGID(d.Rd), shift_out.shiftop - v);

				if (d.S && !d.R15Modified)
				{
					if (d.FlagsSet & FLAG_C)
						PackCPSRImm(regMap, PSR_C, !BorrowFrom(shift_out.shiftop, v));
					if (d.FlagsSet & FLAG_V)
						PackCPSRImm(regMap, PSR_V, OverflowFromSUB(regMap.GetImm32(REGID(d.Rd)), shift_out.shiftop, v));
				}
			}
			else
			{
				u32 rn = regMap.MapReg(REGID(d.Rn));
				regMap.Lock(rn);

				rd = regMap.MapReg(REGID(d.Rd), RegisterMap::MAP_DIRTY | RegisterMap::MAP_NOTINIT);
				regMap.Lock(rd);

				u32 v_tmp = INVALID_REG_ID;

				if (d.S && !d.R15Modified && ((d.FlagsSet & FLAG_C) || (d.FlagsSet & FLAG_V)))
				{
					v_tmp = regMap.AllocTempReg();

					jit_movr_ui(LOCALREG(v_tmp), LOCALREG(rn));
				}

				if (shift_out.shiftopimm)
					jit_rsbi_ui(LOCALREG(rd), LOCALREG(rn), shift_out.shiftop);
				else
					jit_rsbr_ui(LOCALREG(rd), LOCALREG(rn), LOCALREG(shift_out.shiftop));

				regMap.Unlock(rn);

				if (d.S && !d.R15Modified)
				{
					if (d.FlagsSet & FLAG_C)
					{
						u32 tmp = regMap.AllocTempReg();

						if (shift_out.shiftopimm)
							jit_lei_ui(LOCALREG(tmp), LOCALREG(v_tmp), shift_out.shiftop);
						else
							jit_ler_ui(LOCALREG(tmp), LOCALREG(v_tmp), LOCALREG(shift_out.shiftop));
						PackCPSR(regMap, PSR_C, tmp);

						regMap.ReleaseTempReg(tmp);
					}
					if (d.FlagsSet & FLAG_V)
					{
						u32 tmp = regMap.AllocTempReg();

						if (shift_out.shiftopimm)
						{
							jit_xori_ui(LOCALREG(tmp), LOCALREG(v_tmp), shift_out.shiftop);
							jit_xori_ui(LOCALREG(v_tmp), LOCALREG(rd), shift_out.shiftop);
							jit_andr_ui(LOCALREG(v_tmp), LOCALREG(tmp), LOCALREG(v_tmp));
							jit_rshi_ui(LOCALREG(v_tmp), LOCALREG(v_tmp), 31);
						}
						else
						{
							jit_xorr_ui(LOCALREG(tmp), LOCALREG(v_tmp), LOCALREG(shift_out.shiftop));
							jit_xorr_ui(LOCALREG(v_tmp), LOCALREG(rd), LOCALREG(shift_out.shiftop));
							jit_andr_ui(LOCALREG(v_tmp), LOCALREG(tmp), LOCALREG(v_tmp));
							jit_rshi_ui(LOCALREG(v_tmp), LOCALREG(v_tmp), 31);
						}

						regMap.ReleaseTempReg(tmp);

						PackCPSR(regMap, PSR_V, v_tmp);
					}
				}

				if (v_tmp != INVALID_REG_ID)
					regMap.ReleaseTempReg(v_tmp);
			}

			shift_out.Clean(regMap);
		}

		if (d.S && !d.R15Modified)
		{
			if (d.FlagsSet & FLAG_N)
			{
				if (regMap.IsImm(REGID(d.Rd)))
					PackCPSRImm(regMap, PSR_N, BIT31(regMap.GetImm32(REGID(d.Rd))));
				else
				{
					u32 tmp = regMap.AllocTempReg();

					jit_rshi_ui(LOCALREG(tmp), LOCALREG(rd), 31);
					PackCPSR(regMap, PSR_N, tmp);
						
					regMap.ReleaseTempReg(tmp);
				}
			}
			if (d.FlagsSet & FLAG_Z)
			{
				if (regMap.IsImm(REGID(d.Rd)))
					PackCPSRImm(regMap, PSR_Z, regMap.GetImm32(REGID(d.Rd))==0);
				else
				{
					u32 tmp = regMap.AllocTempReg();

					jit_eqi_ui(LOCALREG(tmp), LOCALREG(rd), 0);
					PackCPSR(regMap, PSR_Z, tmp);
						
					regMap.ReleaseTempReg(tmp);
				}
			}
		}

		if (rd != INVALID_REG_ID)
			regMap.Unlock(rd);

		if (d.R15Modified)
		{
			if (d.S)
			{
				DataProcessLoadCPSRGenerate(d, regMap);
			}

			R15ModifiedGenerate(d, regMap);
		}
	}

	OPDECODER_DECL(IR_RSC)
	{
		u32 PROCNUM = d.ProcessID;

		u32 rd = INVALID_REG_ID;

		if (d.I)
		{
			{
				u32 rn = regMap.MapReg(REGID(d.Rn));
				regMap.Lock(rn);

				u32 rn_tmp = regMap.AllocTempReg();

				jit_movr_ui(LOCALREG(rn_tmp), LOCALREG(rn));

				regMap.Unlock(rn);

				u32 cflg = regMap.AllocTempReg();
				UnpackCPSR(regMap, PSR_C, cflg);
				jit_xori_ui(LOCALREG(cflg), LOCALREG(cflg), 1);

				rd = regMap.MapReg(REGID(d.Rd), RegisterMap::MAP_DIRTY | RegisterMap::MAP_NOTINIT);
				regMap.Lock(rd);

				jit_rsbi_ui(LOCALREG(rd), LOCALREG(rn_tmp), d.Immediate);
				jit_subr_ui(LOCALREG(rd), LOCALREG(rd), LOCALREG(cflg));

				if (d.S && !d.R15Modified)
				{
					if (d.FlagsSet & FLAG_C)
					{
						jit_insn *eq0 = jit_beqi_ui(jit_forward(), LOCALREG(cflg), 1);
						jit_lei_ui(LOCALREG(cflg), LOCALREG(rn_tmp), d.Immediate);
						jit_insn *done = jit_jmpi(jit_forward());
						jit_patch(eq0);
						jit_lti_ui(LOCALREG(cflg), LOCALREG(rn_tmp), d.Immediate);
						jit_patch(done);

						PackCPSR(regMap, PSR_C, cflg);
					}
					if (d.FlagsSet & FLAG_V)
					{
						u32 tmp = cflg;

						jit_xori_ui(LOCALREG(tmp), LOCALREG(rn_tmp), d.Immediate);
						jit_xori_ui(LOCALREG(rn_tmp), LOCALREG(rd), d.Immediate);
						jit_andr_ui(LOCALREG(rn_tmp), LOCALREG(tmp), LOCALREG(rn_tmp));
						jit_rshi_ui(LOCALREG(rn_tmp), LOCALREG(rn_tmp), 31);

						PackCPSR(regMap, PSR_V, rn_tmp);
					}
				}

				regMap.ReleaseTempReg(cflg);
				regMap.ReleaseTempReg(rn_tmp);
			}
		}
		else
		{
			ShiftOut shift_out = IRShiftOpGenerate(d, regMap, false);

			{
				u32 rn = regMap.MapReg(REGID(d.Rn));
				regMap.Lock(rn);

				u32 rn_tmp = regMap.AllocTempReg();

				jit_movr_ui(LOCALREG(rn_tmp), LOCALREG(rn));

				regMap.Unlock(rn);

				u32 cflg = regMap.AllocTempReg();
				UnpackCPSR(regMap, PSR_C, cflg);
				jit_xori_ui(LOCALREG(cflg), LOCALREG(cflg), 1);

				rd = regMap.MapReg(REGID(d.Rd), RegisterMap::MAP_DIRTY | RegisterMap::MAP_NOTINIT);
				regMap.Lock(rd);

				if (shift_out.shiftopimm)
					jit_rsbi_ui(LOCALREG(rd), LOCALREG(rn_tmp), shift_out.shiftop);
				else
					jit_rsbr_ui(LOCALREG(rd), LOCALREG(rn_tmp), LOCALREG(shift_out.shiftop));
				jit_subr_ui(LOCALREG(rd), LOCALREG(rd), LOCALREG(cflg));

				if (d.S && !d.R15Modified)
				{
					if (d.FlagsSet & FLAG_C)
					{
						if (shift_out.shiftopimm)
						{
							jit_insn *eq0 = jit_beqi_ui(jit_forward(), LOCALREG(cflg), 1);
							jit_lei_ui(LOCALREG(cflg), LOCALREG(rn_tmp), shift_out.shiftop);
							jit_insn *done = jit_jmpi(jit_forward());
							jit_patch(eq0);
							jit_lti_ui(LOCALREG(cflg), LOCALREG(rn_tmp), shift_out.shiftop);
							jit_patch(done);
						}
						else
						{
							jit_insn *eq0 = jit_beqi_ui(jit_forward(), LOCALREG(cflg), 1);
							jit_ler_ui(LOCALREG(cflg), LOCALREG(rn_tmp), LOCALREG(shift_out.shiftop));
							jit_insn *done = jit_jmpi(jit_forward());
							jit_patch(eq0);
							jit_ltr_ui(LOCALREG(cflg), LOCALREG(rn_tmp), LOCALREG(shift_out.shiftop));
							jit_patch(done);
						}

						PackCPSR(regMap, PSR_C, cflg);
					}
					if (d.FlagsSet & FLAG_V)
					{
						u32 tmp = cflg;

						if (shift_out.shiftopimm)
						{
							jit_xori_ui(LOCALREG(tmp), LOCALREG(rn_tmp), shift_out.shiftop);
							jit_xori_ui(LOCALREG(rn_tmp), LOCALREG(rd), shift_out.shiftop);
							jit_andr_ui(LOCALREG(rn_tmp), LOCALREG(tmp), LOCALREG(rn_tmp));
							jit_rshi_ui(LOCALREG(rn_tmp), LOCALREG(rn_tmp), 31);
						}
						else
						{
							jit_xorr_ui(LOCALREG(tmp), LOCALREG(rn_tmp), LOCALREG(shift_out.shiftop));
							jit_xorr_ui(LOCALREG(rn_tmp), LOCALREG(rd), LOCALREG(shift_out.shiftop));
							jit_andr_ui(LOCALREG(rn_tmp), LOCALREG(tmp), LOCALREG(rn_tmp));
							jit_rshi_ui(LOCALREG(rn_tmp), LOCALREG(rn_tmp), 31);
						}

						PackCPSR(regMap, PSR_V, rn_tmp);
					}
				}

				regMap.ReleaseTempReg(cflg);
				regMap.ReleaseTempReg(rn_tmp);
			}

			shift_out.Clean(regMap);
		}

		if (d.S && !d.R15Modified)
		{
			if (d.FlagsSet & FLAG_N)
			{
				if (regMap.IsImm(REGID(d.Rd)))
					PackCPSRImm(regMap, PSR_N, BIT31(regMap.GetImm32(REGID(d.Rd))));
				else
				{
					u32 tmp = regMap.AllocTempReg();

					jit_rshi_ui(LOCALREG(tmp), LOCALREG(rd), 31);
					PackCPSR(regMap, PSR_N, tmp);
						
					regMap.ReleaseTempReg(tmp);
				}
			}
			if (d.FlagsSet & FLAG_Z)
			{
				if (regMap.IsImm(REGID(d.Rd)))
					PackCPSRImm(regMap, PSR_Z, regMap.GetImm32(REGID(d.Rd))==0);
				else
				{
					u32 tmp = regMap.AllocTempReg();

					jit_eqi_ui(LOCALREG(tmp), LOCALREG(rd), 0);
					PackCPSR(regMap, PSR_Z, tmp);
						
					regMap.ReleaseTempReg(tmp);
				}
			}
		}

		if (rd != INVALID_REG_ID)
			regMap.Unlock(rd);

		if (d.R15Modified)
		{
			if (d.S)
			{
				DataProcessLoadCPSRGenerate(d, regMap);
			}

			R15ModifiedGenerate(d, regMap);
		}
	}

	OPDECODER_DECL(IR_CMP)
	{
		u32 PROCNUM = d.ProcessID;

		u32 dst = INVALID_REG_ID;
		bool dstimm = false;

		if (d.I)
		{
			if (regMap.IsImm(REGID(d.Rn)))
			{
				u32 v = regMap.GetImm32(REGID(d.Rn));

				dst = regMap.GetImm32(REGID(d.Rn)) - d.Immediate;
				dstimm = true;

				{
					if (d.FlagsSet & FLAG_C)
						PackCPSRImm(regMap, PSR_C, !BorrowFrom(v, d.Immediate));
					if (d.FlagsSet & FLAG_V)
						PackCPSRImm(regMap, PSR_V, OverflowFromSUB(dst, v, d.Immediate));
				}
			}
			else
			{
				u32 rn = regMap.MapReg(REGID(d.Rn));
				regMap.Lock(rn);

				dst = regMap.AllocTempReg();

				jit_subi_ui(LOCALREG(dst), LOCALREG(rn), d.Immediate);

				{
					if (d.FlagsSet & FLAG_C)
					{
						u32 c_tmp = regMap.AllocTempReg();

						jit_gei_ui(LOCALREG(c_tmp), LOCALREG(rn), d.Immediate);

						PackCPSR(regMap, PSR_C, c_tmp);
						
						regMap.ReleaseTempReg(c_tmp);
					}
					if (d.FlagsSet & FLAG_V)
					{
						u32 flg = regMap.AllocTempReg();
						u32 flg_tmp = regMap.AllocTempReg();

						jit_xori_ui(LOCALREG(flg), LOCALREG(rn), d.Immediate);
						jit_xorr_ui(LOCALREG(flg_tmp), LOCALREG(dst), LOCALREG(rn));
						jit_andr_ui(LOCALREG(flg), LOCALREG(flg), LOCALREG(flg_tmp));
						jit_rshi_ui(LOCALREG(flg), LOCALREG(flg), 31);

						regMap.ReleaseTempReg(flg_tmp);

						PackCPSR(regMap, PSR_V, flg);

						regMap.ReleaseTempReg(flg);
					}
				}

				regMap.Unlock(rn);
			}
		}
		else
		{
			ShiftOut shift_out = IRShiftOpGenerate(d, regMap, false);

			if (regMap.IsImm(REGID(d.Rn)) && shift_out.shiftopimm)
			{
				u32 v = regMap.GetImm32(REGID(d.Rn));

				dst = regMap.GetImm32(REGID(d.Rn)) - shift_out.shiftop;
				dstimm = true;

				if (d.S && !d.R15Modified)
				{
					if (d.FlagsSet & FLAG_C)
						PackCPSRImm(regMap, PSR_C, !BorrowFrom(v, shift_out.shiftop));
					if (d.FlagsSet & FLAG_V)
						PackCPSRImm(regMap, PSR_V, OverflowFromSUB(dst, v, shift_out.shiftop));
				}
			}
			else
			{
				u32 rn = regMap.MapReg(REGID(d.Rn));
				regMap.Lock(rn);

				dst = regMap.AllocTempReg();

				if (shift_out.shiftopimm)
					jit_subi_ui(LOCALREG(dst), LOCALREG(rn), shift_out.shiftop);
				else
					jit_subr_ui(LOCALREG(dst), LOCALREG(rn), LOCALREG(shift_out.shiftop));

				{
					if (d.FlagsSet & FLAG_C)
					{
						u32 c_tmp = regMap.AllocTempReg();

						if (shift_out.shiftopimm)
							jit_gei_ui(LOCALREG(c_tmp), LOCALREG(rn), shift_out.shiftop);
						else
							jit_ger_ui(LOCALREG(c_tmp), LOCALREG(rn), LOCALREG(shift_out.shiftop));

						PackCPSR(regMap, PSR_C, c_tmp);

						regMap.ReleaseTempReg(c_tmp);
					}
					if (d.FlagsSet & FLAG_V)
					{
						u32 flg = regMap.AllocTempReg();
						u32 flg_tmp = regMap.AllocTempReg();

						if (shift_out.shiftopimm)
						{
							jit_xori_ui(LOCALREG(flg), LOCALREG(rn), shift_out.shiftop);
							jit_xorr_ui(LOCALREG(flg_tmp), LOCALREG(dst), LOCALREG(rn));
							jit_andr_ui(LOCALREG(flg), LOCALREG(flg), LOCALREG(flg_tmp));
							jit_rshi_ui(LOCALREG(flg), LOCALREG(flg), 31);
						}
						else
						{
							jit_xorr_ui(LOCALREG(flg), LOCALREG(rn), LOCALREG(shift_out.shiftop));
							jit_xorr_ui(LOCALREG(flg_tmp), LOCALREG(dst), LOCALREG(rn));
							jit_andr_ui(LOCALREG(flg), LOCALREG(flg), LOCALREG(flg_tmp));
							jit_rshi_ui(LOCALREG(flg), LOCALREG(flg), 31);
						}

						regMap.ReleaseTempReg(flg_tmp);

						PackCPSR(regMap, PSR_V, flg);

						regMap.ReleaseTempReg(flg);
					}
				}

				regMap.Unlock(rn);
			}

			shift_out.Clean(regMap);
		}

		{
			if (d.FlagsSet & FLAG_N)
			{
				if (dstimm)
					PackCPSRImm(regMap, PSR_N, BIT31(dst));
				else
				{
					u32 tmp = regMap.AllocTempReg();

					jit_rshi_ui(LOCALREG(tmp), LOCALREG(dst), 31);
					PackCPSR(regMap, PSR_N, tmp);
						
					regMap.ReleaseTempReg(tmp);
				}
			}
			if (d.FlagsSet & FLAG_Z)
			{
				if (dstimm)
					PackCPSRImm(regMap, PSR_Z, dst==0);
				else
				{
					u32 tmp = regMap.AllocTempReg();

					jit_eqi_ui(LOCALREG(tmp), LOCALREG(dst), 0);
					PackCPSR(regMap, PSR_Z, tmp);
						
					regMap.ReleaseTempReg(tmp);
				}
			}
		}

		if (!dstimm)
			regMap.ReleaseTempReg(dst);
	}

	OPDECODER_DECL(IR_CMN)
	{
		u32 PROCNUM = d.ProcessID;

		u32 dst = INVALID_REG_ID;
		bool dstimm = false;

		if (d.I)
		{
			if (regMap.IsImm(REGID(d.Rn)))
			{
				u32 v = regMap.GetImm32(REGID(d.Rn));

				dst = regMap.GetImm32(REGID(d.Rn)) + d.Immediate;
				dstimm = true;

				{
					if (d.FlagsSet & FLAG_C)
						PackCPSRImm(regMap, PSR_C, CarryFrom(v, d.Immediate));
					if (d.FlagsSet & FLAG_V)
						PackCPSRImm(regMap, PSR_V, OverflowFromADD(dst, v, d.Immediate));
				}
			}
			else
			{
				u32 rn = regMap.MapReg(REGID(d.Rn));
				regMap.Lock(rn);

				dst = regMap.AllocTempReg();

				u32 c_tmp = INVALID_REG_ID;

				if (d.FlagsSet & FLAG_C)
				{
					c_tmp = regMap.AllocTempReg();

					jit_movi_ui(LOCALREG(c_tmp), 0);
					jit_addci_ui(LOCALREG(dst), LOCALREG(rn), d.Immediate);
					jit_addxi_ui(LOCALREG(c_tmp), LOCALREG(c_tmp), 0);
				}
				else
					jit_addi_ui(LOCALREG(dst), LOCALREG(rn), d.Immediate);

				{
					if (d.FlagsSet & FLAG_C)
					{
						PackCPSR(regMap, PSR_C, c_tmp);

						regMap.ReleaseTempReg(c_tmp);
					}
					if (d.FlagsSet & FLAG_V)
					{
						u32 flg = regMap.AllocTempReg();
						u32 flg_tmp = regMap.AllocTempReg();

						jit_xori_ui(LOCALREG(flg), LOCALREG(rn), d.Immediate);
						jit_notr_ui(LOCALREG(flg), LOCALREG(flg));
						jit_xori_ui(LOCALREG(flg_tmp), LOCALREG(dst), d.Immediate);
						jit_andr_ui(LOCALREG(flg), LOCALREG(flg), LOCALREG(flg_tmp));
						jit_rshi_ui(LOCALREG(flg), LOCALREG(flg), 31);

						regMap.ReleaseTempReg(flg_tmp);

						PackCPSR(regMap, PSR_V, flg);

						regMap.ReleaseTempReg(flg);
					}
				}

				regMap.Unlock(rn);
			}
		}
		else
		{
			ShiftOut shift_out = IRShiftOpGenerate(d, regMap, false);

			if (regMap.IsImm(REGID(d.Rn)) && shift_out.shiftopimm)
			{
				u32 v = regMap.GetImm32(REGID(d.Rn));

				dst = regMap.GetImm32(REGID(d.Rn)) + shift_out.shiftop;
				dstimm = true;

				{
					if (d.FlagsSet & FLAG_C)
						PackCPSRImm(regMap, PSR_C, CarryFrom(v, shift_out.shiftop));
					if (d.FlagsSet & FLAG_V)
						PackCPSRImm(regMap, PSR_V, OverflowFromADD(dst, v, shift_out.shiftop));
				}
			}
			else
			{
				u32 rn = regMap.MapReg(REGID(d.Rn));
				regMap.Lock(rn);

				dst = regMap.AllocTempReg();

				u32 c_tmp = INVALID_REG_ID;

				if (d.FlagsSet & FLAG_C)
				{
					c_tmp = regMap.AllocTempReg();

					jit_movi_ui(LOCALREG(c_tmp), 0);
					if (shift_out.shiftopimm)
						jit_addci_ui(LOCALREG(dst), LOCALREG(rn), shift_out.shiftop);
					else
						jit_addcr_ui(LOCALREG(dst), LOCALREG(rn), LOCALREG(shift_out.shiftop));
					jit_addxi_ui(LOCALREG(c_tmp), LOCALREG(c_tmp), 0);
				}
				else
				{
					if (shift_out.shiftopimm)
						jit_addi_ui(LOCALREG(dst), LOCALREG(rn), shift_out.shiftop);
					else
						jit_addr_ui(LOCALREG(dst), LOCALREG(rn), LOCALREG(shift_out.shiftop));
				}

				{
					if (d.FlagsSet & FLAG_C)
					{
						PackCPSR(regMap, PSR_C, c_tmp);

						regMap.ReleaseTempReg(c_tmp);
					}
					if (d.FlagsSet & FLAG_V)
					{
						u32 flg = regMap.AllocTempReg();
						u32 flg_tmp = regMap.AllocTempReg();

						if (shift_out.shiftopimm)
						{
							jit_xori_ui(LOCALREG(flg), LOCALREG(rn), shift_out.shiftop);
							jit_notr_ui(LOCALREG(flg), LOCALREG(flg));
							jit_xori_ui(LOCALREG(flg_tmp), LOCALREG(dst), shift_out.shiftop);
							jit_andr_ui(LOCALREG(flg), LOCALREG(flg), LOCALREG(flg_tmp));
							jit_rshi_ui(LOCALREG(flg), LOCALREG(flg), 31);
						}
						else
						{
							jit_xorr_ui(LOCALREG(flg), LOCALREG(rn), LOCALREG(shift_out.shiftop));
							jit_notr_ui(LOCALREG(flg), LOCALREG(flg));
							jit_xorr_ui(LOCALREG(flg_tmp), LOCALREG(dst), LOCALREG(shift_out.shiftop));
							jit_andr_ui(LOCALREG(flg), LOCALREG(flg), LOCALREG(flg_tmp));
							jit_rshi_ui(LOCALREG(flg), LOCALREG(flg), 31);
						}

						regMap.ReleaseTempReg(flg_tmp);

						PackCPSR(regMap, PSR_V, flg);

						regMap.ReleaseTempReg(flg);
					}
				}

				regMap.Unlock(rn);
			}

			shift_out.Clean(regMap);
		}

		{
			if (d.FlagsSet & FLAG_N)
			{
				if (dstimm)
					PackCPSRImm(regMap, PSR_N, BIT31(dst));
				else
				{
					u32 tmp = regMap.AllocTempReg();

					jit_rshi_ui(LOCALREG(tmp), LOCALREG(dst), 31);
					PackCPSR(regMap, PSR_N, tmp);
						
					regMap.ReleaseTempReg(tmp);
				}
			}
			if (d.FlagsSet & FLAG_Z)
			{
				if (dstimm)
					PackCPSRImm(regMap, PSR_Z, dst==0);
				else
				{
					u32 tmp = regMap.AllocTempReg();

					jit_eqi_ui(LOCALREG(tmp), LOCALREG(dst), 0);
					PackCPSR(regMap, PSR_Z, tmp);
						
					regMap.ReleaseTempReg(tmp);
				}
			}
		}

		if (!dstimm)
			regMap.ReleaseTempReg(dst);
	}

	OPDECODER_DECL(IR_MUL)
	{
		u32 PROCNUM = d.ProcessID;

		if (regMap.IsImm(REGID(d.Rs)) && regMap.IsImm(REGID(d.Rm)))
		{
			u32 v = regMap.GetImm32(REGID(d.Rs));
			if ((s32)v < 0)
				v = ~v;

			MUL_Mxx_END_Imm(d, regMap, 1, v);

			regMap.SetImm32(REGID(d.Rd), regMap.GetImm32(REGID(d.Rs)) * regMap.GetImm32(REGID(d.Rm)));

			if (d.S)
			{
				if (d.FlagsSet & FLAG_N)
					PackCPSRImm(regMap, PSR_N, BIT31(regMap.GetImm32(REGID(d.Rd))));
				if (d.FlagsSet & FLAG_Z)
					PackCPSRImm(regMap, PSR_Z, regMap.GetImm32(REGID(d.Rd))==0);
			}
		}
		else
		{
			u32 v = INVALID_REG_ID;
			bool vimm = false;

			if (regMap.IsImm(REGID(d.Rs)))
			{
				v = regMap.GetImm32(REGID(d.Rs));
				vimm = true;

				if ((s32)v < 0)
					v = ~v;
			}
			else
				v = regMap.AllocTempReg();

			u32 rs = regMap.MapReg(REGID(d.Rs));
			regMap.Lock(rs);

			if (!vimm)
			{
				jit_movr_ui(LOCALREG(v), LOCALREG(rs));
				jit_rshi_i(LOCALREG(v), LOCALREG(v), 31);
				jit_xorr_ui(LOCALREG(v), LOCALREG(v), LOCALREG(rs));

				MUL_Mxx_END(d, regMap, 1, v);

				regMap.ReleaseTempReg(v);
			}
			else
				MUL_Mxx_END_Imm(d, regMap, 1, v);

			u32 rm = regMap.MapReg(REGID(d.Rm));
			regMap.Lock(rm);

			u32 rd = regMap.MapReg(REGID(d.Rd), RegisterMap::MAP_DIRTY | RegisterMap::MAP_NOTINIT);
			regMap.Lock(rd);

			jit_mulr_ui(LOCALREG(rd), LOCALREG(rs), LOCALREG(rm));

			regMap.Unlock(rm);
			regMap.Unlock(rs);

			if (d.S)
			{
				if (d.FlagsSet & FLAG_N)
				{
					u32 tmp = regMap.AllocTempReg();

					jit_rshi_ui(LOCALREG(tmp), LOCALREG(rd), 31);
					PackCPSR(regMap, PSR_N, tmp);

					regMap.ReleaseTempReg(tmp);
				}
				if (d.FlagsSet & FLAG_Z)
				{
					u32 tmp = regMap.AllocTempReg();

					jit_eqi_ui(LOCALREG(tmp), LOCALREG(rd), 0);
					PackCPSR(regMap, PSR_Z, tmp);

					regMap.ReleaseTempReg(tmp);
				}
			}

			regMap.Unlock(rd);
		}
	}

	OPDECODER_DECL(IR_MLA)
	{
		u32 PROCNUM = d.ProcessID;

		if (regMap.IsImm(REGID(d.Rs)) && regMap.IsImm(REGID(d.Rm)) && regMap.IsImm(REGID(d.Rn)))
		{
			u32 v = regMap.GetImm32(REGID(d.Rs));

			if ((s32)v < 0)
				v = ~v;

			MUL_Mxx_END_Imm(d, regMap, 2, v);

			regMap.SetImm32(REGID(d.Rd), regMap.GetImm32(REGID(d.Rs)) * regMap.GetImm32(REGID(d.Rm)) + regMap.GetImm32(REGID(d.Rn)));

			if (d.S)
			{
				if (d.FlagsSet & FLAG_N)
					PackCPSRImm(regMap, PSR_N, BIT31(regMap.GetImm32(REGID(d.Rd))));
				if (d.FlagsSet & FLAG_Z)
					PackCPSRImm(regMap, PSR_Z, regMap.GetImm32(REGID(d.Rd))==0);
			}
		}
		else
		{
			u32 v = INVALID_REG_ID;
			bool vimm = false;

			if (regMap.IsImm(REGID(d.Rs)))
			{
				v = regMap.GetImm32(REGID(d.Rs));
				vimm = true;

				if ((s32)v < 0)
					v = ~v;
			}
			else
				v = regMap.AllocTempReg();

			u32 rs = regMap.MapReg(REGID(d.Rs));
			regMap.Lock(rs);

			if (!vimm)
			{
				jit_movr_ui(LOCALREG(v), LOCALREG(rs));
				jit_rshi_i(LOCALREG(v), LOCALREG(v), 31);
				jit_xorr_ui(LOCALREG(v), LOCALREG(v), LOCALREG(rs));

				MUL_Mxx_END(d, regMap, 2, v);

				regMap.ReleaseTempReg(v);
			}
			else
				MUL_Mxx_END_Imm(d, regMap, 2, v);

			u32 rm = regMap.MapReg(REGID(d.Rm));
			regMap.Lock(rm);

			u32 rn = regMap.MapReg(REGID(d.Rn));
			regMap.Lock(rn);

			u32 rd = regMap.MapReg(REGID(d.Rd), RegisterMap::MAP_DIRTY | RegisterMap::MAP_NOTINIT);
			regMap.Lock(rd);

			if (rd != rn)
			{
				jit_mulr_ui(LOCALREG(rd), LOCALREG(rs), LOCALREG(rm));
				jit_addr_ui(LOCALREG(rd), LOCALREG(rd), LOCALREG(rn));
			}
			else
			{
				u32 tmp = regMap.AllocTempReg();

				jit_mulr_ui(LOCALREG(tmp), LOCALREG(rs), LOCALREG(rm));
				jit_addr_ui(LOCALREG(rd), LOCALREG(tmp), LOCALREG(rn));

				regMap.ReleaseTempReg(tmp);
			}

			regMap.Unlock(rn);
			regMap.Unlock(rm);
			regMap.Unlock(rs);

			if (d.S)
			{
				if (d.FlagsSet & FLAG_N)
				{
					u32 tmp = regMap.AllocTempReg();

					jit_rshi_ui(LOCALREG(tmp), LOCALREG(rd), 31);
					PackCPSR(regMap, PSR_N, tmp);

					regMap.ReleaseTempReg(tmp);
				}
				if (d.FlagsSet & FLAG_Z)
				{
					u32 tmp = regMap.AllocTempReg();

					jit_eqi_ui(LOCALREG(tmp), LOCALREG(rd), 0);
					PackCPSR(regMap, PSR_Z, tmp);

					regMap.ReleaseTempReg(tmp);
				}
			}

			regMap.Unlock(rd);
		}
	}

	OPDECODER_DECL(IR_UMULL)
	{
		u32 PROCNUM = d.ProcessID;

		if (regMap.IsImm(REGID(d.Rs)) && regMap.IsImm(REGID(d.Rm)))
		{
			u32 v = regMap.GetImm32(REGID(d.Rs));

			MUL_Mxx_END_Imm(d, regMap, 2, v);

			u64 res = (u64)regMap.GetImm32(REGID(d.Rs)) * regMap.GetImm32(REGID(d.Rm));

			regMap.SetImm32(REGID(d.Rn), (u32)res);
			regMap.SetImm32(REGID(d.Rd), (u32)(res>>32));

			if (d.S)
			{
				if (d.FlagsSet & FLAG_N)
					PackCPSRImm(regMap, PSR_N, BIT31(regMap.GetImm32(REGID(d.Rd))));
				if (d.FlagsSet & FLAG_Z)
					PackCPSRImm(regMap, PSR_Z, res==0);
			}
		}
		else
		{
			u32 v = INVALID_REG_ID;
			bool vimm = false;

			if (regMap.IsImm(REGID(d.Rs)))
			{
				v = regMap.GetImm32(REGID(d.Rs));
				vimm = true;
			}

			u32 rs = regMap.MapReg(REGID(d.Rs));
			regMap.Lock(rs);

			if (!vimm)
				MUL_Mxx_END(d, regMap, 2, rs);
			else
				MUL_Mxx_END_Imm(d, regMap, 2, v);

			u32 rm = regMap.MapReg(REGID(d.Rm));
			regMap.Lock(rm);

			u32 rn = regMap.MapReg(REGID(d.Rn), RegisterMap::MAP_DIRTY | RegisterMap::MAP_NOTINIT);
			regMap.Lock(rn);

			u32 rd = regMap.MapReg(REGID(d.Rd), RegisterMap::MAP_DIRTY | RegisterMap::MAP_NOTINIT);
			regMap.Lock(rd);

			if (rn != rs && rn != rm)
			{
				jit_mulr_ui(LOCALREG(rn), LOCALREG(rs), LOCALREG(rm));
				jit_hmulr_ui(LOCALREG(rd), LOCALREG(rs), LOCALREG(rm));
			}
			else
			{
				u32 tmp = regMap.AllocTempReg();

				jit_mulr_ui(LOCALREG(tmp), LOCALREG(rs), LOCALREG(rm));
				jit_hmulr_ui(LOCALREG(rd), LOCALREG(rs), LOCALREG(rm));
				jit_movr_ui(LOCALREG(rn), LOCALREG(tmp));

				regMap.ReleaseTempReg(tmp);
			}

			regMap.Unlock(rm);
			regMap.Unlock(rs);

			if (d.S)
			{
				if (d.FlagsSet & FLAG_N)
				{
					u32 tmp = regMap.AllocTempReg();

					jit_rshi_ui(LOCALREG(tmp), LOCALREG(rd), 31);
					PackCPSR(regMap, PSR_N, tmp);

					regMap.ReleaseTempReg(tmp);
				}
				if (d.FlagsSet & FLAG_Z)
				{
					u32 tmp = regMap.AllocTempReg();

					jit_andr_ui(LOCALREG(tmp), LOCALREG(rn), LOCALREG(rd));
					jit_eqi_ui(LOCALREG(tmp), LOCALREG(tmp), 0);
					PackCPSR(regMap, PSR_Z, tmp);

					regMap.ReleaseTempReg(tmp);
				}
			}

			regMap.Unlock(rn);
			regMap.Unlock(rd);
		}
	}

	OPDECODER_DECL(IR_UMLAL)
	{
		u32 PROCNUM = d.ProcessID;

		u32 v = INVALID_REG_ID;
		bool vimm = false;

		if (regMap.IsImm(REGID(d.Rs)))
		{
			v = regMap.GetImm32(REGID(d.Rs));
			vimm = true;
		}

		u32 rs = regMap.MapReg(REGID(d.Rs));
		regMap.Lock(rs);

		if (!vimm)
			MUL_Mxx_END(d, regMap, 3, rs);
		else
			MUL_Mxx_END_Imm(d, regMap, 3, v);

		u32 rm = regMap.MapReg(REGID(d.Rm));
		regMap.Lock(rm);

		u32 hi = regMap.AllocTempReg();
		u32 lo = regMap.AllocTempReg();

		jit_mulr_ui(LOCALREG(lo), LOCALREG(rs), LOCALREG(rm));
		jit_hmulr_ui(LOCALREG(hi), LOCALREG(rs), LOCALREG(rm));

		regMap.Unlock(rm);
		regMap.Unlock(rs);

		u32 rn = regMap.MapReg(REGID(d.Rn), RegisterMap::MAP_DIRTY);
		regMap.Lock(rn);

		u32 rd = regMap.MapReg(REGID(d.Rd), RegisterMap::MAP_DIRTY);
		regMap.Lock(rd);

		jit_addcr_ui(LOCALREG(rn), LOCALREG(rn), LOCALREG(lo));
		jit_addxr_ui(LOCALREG(rd), LOCALREG(rd), LOCALREG(hi));

		regMap.ReleaseTempReg(lo);
		regMap.ReleaseTempReg(hi);

		if (d.S)
		{
			if (d.FlagsSet & FLAG_N)
			{
				u32 tmp = regMap.AllocTempReg();

				jit_rshi_ui(LOCALREG(tmp), LOCALREG(rd), 31);
				PackCPSR(regMap, PSR_N, tmp);

				regMap.ReleaseTempReg(tmp);
			}
			if (d.FlagsSet & FLAG_Z)
			{
				u32 tmp = regMap.AllocTempReg();

				jit_andr_ui(LOCALREG(tmp), LOCALREG(rn), LOCALREG(rd));
				jit_eqi_ui(LOCALREG(tmp), LOCALREG(tmp), 0);
				PackCPSR(regMap, PSR_Z, tmp);

				regMap.ReleaseTempReg(tmp);
			}
		}

		regMap.Unlock(rn);
		regMap.Unlock(rd);
	}

	OPDECODER_DECL(IR_SMULL)
	{
		u32 PROCNUM = d.ProcessID;

		if (regMap.IsImm(REGID(d.Rs)) && regMap.IsImm(REGID(d.Rm)))
		{
			u32 v = regMap.GetImm32(REGID(d.Rs));
			if ((s32)v < 0)
				v = ~v;

			MUL_Mxx_END_Imm(d, regMap, 2, v);

			u64 res = (s64)regMap.GetImm32(REGID(d.Rs)) * regMap.GetImm32(REGID(d.Rm));

			regMap.SetImm32(REGID(d.Rn), (u32)res);
			regMap.SetImm32(REGID(d.Rd), (u32)(res>>32));

			if (d.S)
			{
				if (d.FlagsSet & FLAG_N)
					PackCPSRImm(regMap, PSR_N, BIT31(regMap.GetImm32(REGID(d.Rd))));
				if (d.FlagsSet & FLAG_Z)
					PackCPSRImm(regMap, PSR_Z, res==0);
			}
		}
		else
		{
			u32 v = INVALID_REG_ID;
			bool vimm = false;

			if (regMap.IsImm(REGID(d.Rs)))
			{
				v = regMap.GetImm32(REGID(d.Rs));
				vimm = true;

				if ((s32)v < 0)
					v = ~v;
			}
			else
				v = regMap.AllocTempReg();

			u32 rs = regMap.MapReg(REGID(d.Rs));
			regMap.Lock(rs);

			if (!vimm)
			{
				jit_movr_ui(LOCALREG(v), LOCALREG(rs));
				jit_rshi_i(LOCALREG(v), LOCALREG(v), 31);
				jit_xorr_ui(LOCALREG(v), LOCALREG(v), LOCALREG(rs));

				MUL_Mxx_END(d, regMap, 2, v);

				regMap.ReleaseTempReg(v);
			}
			else
				MUL_Mxx_END_Imm(d, regMap, 2, v);

			u32 rm = regMap.MapReg(REGID(d.Rm));
			regMap.Lock(rm);

			u32 rn = regMap.MapReg(REGID(d.Rn), RegisterMap::MAP_DIRTY | RegisterMap::MAP_NOTINIT);
			regMap.Lock(rn);

			u32 rd = regMap.MapReg(REGID(d.Rd), RegisterMap::MAP_DIRTY | RegisterMap::MAP_NOTINIT);
			regMap.Lock(rd);

			if (rn != rs && rn != rm)
			{
				jit_mulr_i(LOCALREG(rn), LOCALREG(rs), LOCALREG(rm));
				jit_hmulr_i(LOCALREG(rd), LOCALREG(rs), LOCALREG(rm));
			}
			else
			{
				u32 tmp = regMap.AllocTempReg();

				jit_mulr_i(LOCALREG(tmp), LOCALREG(rs), LOCALREG(rm));
				jit_hmulr_i(LOCALREG(rd), LOCALREG(rs), LOCALREG(rm));
				jit_movr_i(LOCALREG(rn), LOCALREG(tmp));

				regMap.ReleaseTempReg(tmp);
			}

			regMap.Unlock(rm);
			regMap.Unlock(rs);

			if (d.S)
			{
				if (d.FlagsSet & FLAG_N)
				{
					u32 tmp = regMap.AllocTempReg();

					jit_rshi_ui(LOCALREG(tmp), LOCALREG(rd), 31);
					PackCPSR(regMap, PSR_N, tmp);

					regMap.ReleaseTempReg(tmp);
				}
				if (d.FlagsSet & FLAG_Z)
				{
					u32 tmp = regMap.AllocTempReg();

					jit_andr_ui(LOCALREG(tmp), LOCALREG(rn), LOCALREG(rd));
					jit_eqi_ui(LOCALREG(tmp), LOCALREG(tmp), 0);
					PackCPSR(regMap, PSR_Z, tmp);

					regMap.ReleaseTempReg(tmp);
				}
			}

			regMap.Unlock(rn);
			regMap.Unlock(rd);
		}
	}

	OPDECODER_DECL(IR_SMLAL)
	{
		u32 PROCNUM = d.ProcessID;

		u32 v = INVALID_REG_ID;
		bool vimm = false;

		if (regMap.IsImm(REGID(d.Rs)))
		{
			v = regMap.GetImm32(REGID(d.Rs));
			vimm = true;

			if ((s32)v < 0)
				v = ~v;
		}
		else
			v = regMap.AllocTempReg();

		u32 rs = regMap.MapReg(REGID(d.Rs));
		regMap.Lock(rs);

		if (!vimm)
		{
			jit_movr_ui(LOCALREG(v), LOCALREG(rs));
			jit_rshi_i(LOCALREG(v), LOCALREG(v), 31);
			jit_xorr_ui(LOCALREG(v), LOCALREG(v), LOCALREG(rs));

			MUL_Mxx_END(d, regMap, 3, v);

			regMap.ReleaseTempReg(v);
		}
		else
			MUL_Mxx_END_Imm(d, regMap, 3, v);

		u32 rm = regMap.MapReg(REGID(d.Rm));
		regMap.Lock(rm);

		u32 hi = regMap.AllocTempReg();
		u32 lo = regMap.AllocTempReg();

		jit_mulr_i(LOCALREG(lo), LOCALREG(rs), LOCALREG(rm));
		jit_hmulr_i(LOCALREG(hi), LOCALREG(rs), LOCALREG(rm));

		regMap.Unlock(rm);
		regMap.Unlock(rs);

		u32 rn = regMap.MapReg(REGID(d.Rn), RegisterMap::MAP_DIRTY);
		regMap.Lock(rn);

		u32 rd = regMap.MapReg(REGID(d.Rd), RegisterMap::MAP_DIRTY);
		regMap.Lock(rd);

		jit_addcr_ui(LOCALREG(rn), LOCALREG(rn), LOCALREG(lo));
		jit_addxr_ui(LOCALREG(rd), LOCALREG(rd), LOCALREG(hi));

		regMap.ReleaseTempReg(lo);
		regMap.ReleaseTempReg(hi);

		if (d.S)
		{
			if (d.FlagsSet & FLAG_N)
			{
				u32 tmp = regMap.AllocTempReg();

				jit_rshi_ui(LOCALREG(tmp), LOCALREG(rd), 31);
				PackCPSR(regMap, PSR_N, tmp);

				regMap.ReleaseTempReg(tmp);
			}
			if (d.FlagsSet & FLAG_Z)
			{
				u32 tmp = regMap.AllocTempReg();

				jit_andr_ui(LOCALREG(tmp), LOCALREG(rn), LOCALREG(rd));
				jit_eqi_ui(LOCALREG(tmp), LOCALREG(tmp), 0);
				PackCPSR(regMap, PSR_Z, tmp);

				regMap.ReleaseTempReg(tmp);
			}
		}

		regMap.Unlock(rn);
		regMap.Unlock(rd);
	}

	OPDECODER_DECL(IR_SMULxy)
	{
		u32 PROCNUM = d.ProcessID;

		if (regMap.IsImm(REGID(d.Rs)) && regMap.IsImm(REGID(d.Rm)))
		{
			s32 tmp1, tmp2;

			if (d.X)
				tmp1 = HWORD(regMap.GetImm32(REGID(d.Rm)));
			else
				tmp1 = LWORD(regMap.GetImm32(REGID(d.Rm)));

			if (d.Y)
				tmp2 = HWORD(regMap.GetImm32(REGID(d.Rs)));
			else
				tmp2 = LWORD(regMap.GetImm32(REGID(d.Rs)));

			regMap.SetImm32(REGID(d.Rd), (u32)(tmp1 * tmp2));
		}
		else
		{
			u32 tmp1, tmp2;

			{
				u32 rm = regMap.MapReg(REGID(d.Rm));
				regMap.Lock(rm);

				tmp1 = regMap.AllocTempReg();

				if (d.X)
					jit_rshi_i(LOCALREG(tmp1), LOCALREG(rm), 16);
				else
				{
					jit_lshi_i(LOCALREG(tmp1), LOCALREG(rm), 16);
					jit_rshi_i(LOCALREG(tmp1), LOCALREG(tmp1), 16);
					//jit_extr_s_ui(LOCALREG(tmp1), LOCALREG(rm));
				}

				regMap.Unlock(rm);
			}

			{
				u32 rs = regMap.MapReg(REGID(d.Rs));
				regMap.Lock(rs);

				tmp2 = regMap.AllocTempReg();

				if (d.Y)
					jit_rshi_i(LOCALREG(tmp2), LOCALREG(rs), 16);
				else
				{
					jit_lshi_i(LOCALREG(tmp2), LOCALREG(rs), 16);
					jit_rshi_i(LOCALREG(tmp2), LOCALREG(tmp2), 16);
					//jit_extr_s_ui(LOCALREG(tmp2), LOCALREG(rs));
				}

				regMap.Unlock(rs);
			}

			u32 rd = regMap.MapReg(REGID(d.Rd), RegisterMap::MAP_DIRTY | RegisterMap::MAP_NOTINIT);
			regMap.Lock(rd);

			jit_mulr_i(LOCALREG(rd), LOCALREG(tmp1), LOCALREG(tmp2));

			regMap.Unlock(rd);

			regMap.ReleaseTempReg(tmp1);
			regMap.ReleaseTempReg(tmp2);
		}
	}

	OPDECODER_DECL(IR_SMLAxy)
	{
		u32 PROCNUM = d.ProcessID;

		if (regMap.IsImm(REGID(d.Rs)) && regMap.IsImm(REGID(d.Rm)) && regMap.IsImm(REGID(d.Rn)))
		{
			s32 tmp1, tmp2;

			if (d.X)
				tmp1 = HWORD(regMap.GetImm32(REGID(d.Rm)));
			else
				tmp1 = LWORD(regMap.GetImm32(REGID(d.Rm)));

			if (d.Y)
				tmp2 = HWORD(regMap.GetImm32(REGID(d.Rs)));
			else
				tmp2 = LWORD(regMap.GetImm32(REGID(d.Rs)));

			u32 mul = (u32)(tmp1 * tmp2);
			u32 mla = mul + regMap.GetImm32(REGID(d.Rn));

			if (OverflowFromADD(mla, mul, regMap.GetImm32(REGID(d.Rn))))
				PackCPSRImm(regMap, PSR_Q, 1);

			regMap.SetImm32(REGID(d.Rd), mla);
		}
		else
		{
			u32 tmp1, tmp2;

			{
				u32 rm = regMap.MapReg(REGID(d.Rm));
				regMap.Lock(rm);

				tmp1 = regMap.AllocTempReg();

				if (d.X)
					jit_rshi_i(LOCALREG(tmp1), LOCALREG(rm), 16);
				else
				{
					jit_lshi_i(LOCALREG(tmp1), LOCALREG(rm), 16);
					jit_rshi_i(LOCALREG(tmp1), LOCALREG(tmp1), 16);
					//jit_extr_s_ui(LOCALREG(tmp1), LOCALREG(rm));
				}

				regMap.Unlock(rm);
			}

			{
				u32 rs = regMap.MapReg(REGID(d.Rs));
				regMap.Lock(rs);

				tmp2 = regMap.AllocTempReg();

				if (d.Y)
					jit_rshi_i(LOCALREG(tmp2), LOCALREG(rs), 16);
				else
				{
					jit_lshi_i(LOCALREG(tmp2), LOCALREG(rs), 16);
					jit_rshi_i(LOCALREG(tmp2), LOCALREG(tmp2), 16);
					//jit_extr_s_ui(LOCALREG(tmp2), LOCALREG(rs));
				}

				regMap.Unlock(rs);
			}

			u32 rn = regMap.MapReg(REGID(d.Rn));
			regMap.Lock(rn);
			u32 rd = regMap.MapReg(REGID(d.Rd), RegisterMap::MAP_DIRTY | RegisterMap::MAP_NOTINIT);
			regMap.Lock(rd);

			jit_mulr_i(LOCALREG(tmp1), LOCALREG(tmp1), LOCALREG(tmp2));
			jit_movr_ui(LOCALREG(tmp2), LOCALREG(rn));
			jit_addr_ui(LOCALREG(rd), LOCALREG(tmp1), LOCALREG(rn));

			jit_xorr_ui(LOCALREG(tmp2), LOCALREG(tmp1), LOCALREG(tmp2));
			jit_notr_ui(LOCALREG(tmp2), LOCALREG(tmp2));
			jit_xorr_ui(LOCALREG(tmp1), LOCALREG(rd), LOCALREG(tmp1));
			jit_andr_ui(LOCALREG(tmp1), LOCALREG(tmp1), LOCALREG(tmp2));
			jit_rshi_ui(LOCALREG(tmp1), LOCALREG(tmp1), 31);

			regMap.Unlock(rd);
			regMap.Unlock(rn);

			regMap.ReleaseTempReg(tmp2);

			u32 cpsr = regMap.MapReg(RegisterMap::CPSR);
			regMap.Lock(cpsr);

			jit_lshi_ui(LOCALREG(tmp1), LOCALREG(tmp1), PSR_Q_BITSHIFT);
			jit_orr_ui(LOCALREG(cpsr), LOCALREG(cpsr), LOCALREG(tmp1));

			regMap.Unlock(cpsr);

			regMap.ReleaseTempReg(tmp1);
		}
	}

	OPDECODER_DECL(IR_SMULWy)
	{
		u32 PROCNUM = d.ProcessID;

		// fallback to interpreter
		regMap.FlushGuestReg(REGID(d.Rd));
		regMap.FlushGuestReg(REGID(d.Rs));
		regMap.FlushGuestReg(REGID(d.Rm));

		Fallback2Interpreter(d, regMap);
	}

	OPDECODER_DECL(IR_SMLAWy)
	{
		u32 PROCNUM = d.ProcessID;

		// fallback to interpreter
		regMap.FlushGuestReg(REGID(d.Rd));
		regMap.FlushGuestReg(REGID(d.Rn));
		regMap.FlushGuestReg(REGID(d.Rs));
		regMap.FlushGuestReg(REGID(d.Rm));
		regMap.FlushGuestReg(RegisterMap::CPSR);

		Fallback2Interpreter(d, regMap);
	}

	OPDECODER_DECL(IR_SMLALxy)
	{
		u32 PROCNUM = d.ProcessID;

		// fallback to interpreter
		regMap.FlushGuestReg(REGID(d.Rd));
		regMap.FlushGuestReg(REGID(d.Rn));
		regMap.FlushGuestReg(REGID(d.Rs));
		regMap.FlushGuestReg(REGID(d.Rm));

		Fallback2Interpreter(d, regMap);
	}

	OPDECODER_DECL(IR_LDR)
	{
		u32 PROCNUM = d.ProcessID;

		enum AddressRegType
		{
			TEMPREG,
			IMM
		};

		u32 adr = INVALID_REG_ID;
		AddressRegType adr_type = TEMPREG;

		if (d.P)
		{
			if (d.I)
			{
				if (regMap.IsImm(REGID(d.Rn)))
				{
					if (d.U)
						adr = regMap.GetImm32(REGID(d.Rn)) + d.Immediate;
					else
						adr = regMap.GetImm32(REGID(d.Rn)) - d.Immediate;

					adr_type = IMM;
				}
				else
				{
					u32 rn = regMap.MapReg(REGID(d.Rn));
					regMap.Lock(rn);

					adr = regMap.AllocTempReg();
					adr_type = TEMPREG;

					if (d.U)
						jit_addi_ui(LOCALREG(adr), LOCALREG(rn), d.Immediate);
					else
						jit_subi_ui(LOCALREG(adr), LOCALREG(rn), d.Immediate);

					regMap.Unlock(rn);
				}
			}
			else
			{
				ShiftOut shift_out = IRShiftOpGenerate(d, regMap, false);

				if (regMap.IsImm(REGID(d.Rn)) && shift_out.shiftopimm)
				{
					if (d.U)
						adr = regMap.GetImm32(REGID(d.Rn)) + shift_out.shiftop;
					else
						adr = regMap.GetImm32(REGID(d.Rn)) - shift_out.shiftop;

					adr_type = IMM;
				}
				else
				{
					u32 rn = regMap.MapReg(REGID(d.Rn));
					regMap.Lock(rn);

					adr = regMap.AllocTempReg();
					adr_type = TEMPREG;

					if (shift_out.shiftopimm)
					{
						if (d.U)
							jit_addi_ui(LOCALREG(adr), LOCALREG(rn), shift_out.shiftop);
						else
							jit_subi_ui(LOCALREG(adr), LOCALREG(rn), shift_out.shiftop);
					}
					else
					{
						if (d.U)
							jit_addr_ui(LOCALREG(adr), LOCALREG(rn), LOCALREG(shift_out.shiftop));
						else
							jit_subr_ui(LOCALREG(adr), LOCALREG(rn), LOCALREG(shift_out.shiftop));
					}

					regMap.Unlock(rn);
				}

				shift_out.Clean(regMap);
			}

			if (d.W)
			{
				if (adr_type == IMM)
					regMap.SetImm32(REGID(d.Rn), adr);
				else
				{
					u32 rn = regMap.MapReg(REGID(d.Rn), RegisterMap::MAP_DIRTY | RegisterMap::MAP_NOTINIT);
					regMap.Lock(rn);

					jit_movr_ui(LOCALREG(rn), LOCALREG(adr));

					regMap.Unlock(rn);
				}
			}
		}
		else
		{
			if (regMap.IsImm(REGID(d.Rn)))
			{
				adr = regMap.GetImm32(REGID(d.Rn));

				adr_type = IMM;
			}
			else
			{
				u32 rn = regMap.MapReg(REGID(d.Rn));
				regMap.Lock(rn);

				adr = regMap.AllocTempReg();
				adr_type = TEMPREG;

				jit_movr_ui(LOCALREG(adr), LOCALREG(rn));

				regMap.Unlock(rn);
			}

			if (d.I)
			{
				if (adr_type == IMM)
				{
					if (d.U)
						regMap.SetImm32(REGID(d.Rn), adr + d.Immediate);
					else
						regMap.SetImm32(REGID(d.Rn), adr - d.Immediate);
				}
				else
				{
					u32 rn = regMap.MapReg(REGID(d.Rn), RegisterMap::MAP_DIRTY | RegisterMap::MAP_NOTINIT);
					regMap.Lock(rn);

					if (d.U)
						jit_addi_ui(LOCALREG(rn), LOCALREG(adr), d.Immediate);
					else
						jit_subi_ui(LOCALREG(rn), LOCALREG(adr), d.Immediate);

					regMap.Unlock(rn);
				}
			}
			else
			{
				ShiftOut shift_out = IRShiftOpGenerate(d, regMap, false);

				if (shift_out.shiftopimm)
				{
					if (adr_type == IMM)
					{
						if (d.U)
							regMap.SetImm32(REGID(d.Rn), adr + shift_out.shiftop);
						else
							regMap.SetImm32(REGID(d.Rn), adr - shift_out.shiftop);
					}
					else
					{
						u32 rn = regMap.MapReg(REGID(d.Rn), RegisterMap::MAP_DIRTY | RegisterMap::MAP_NOTINIT);
						regMap.Lock(rn);

						if (d.U)
							jit_addi_ui(LOCALREG(rn), LOCALREG(adr), shift_out.shiftop);
						else
							jit_subi_ui(LOCALREG(rn), LOCALREG(adr), shift_out.shiftop);

						regMap.Unlock(rn);
					}
				}
				else
				{
					u32 rn = regMap.MapReg(REGID(d.Rn), RegisterMap::MAP_DIRTY | RegisterMap::MAP_NOTINIT);
					regMap.Lock(rn);

					if (adr_type == IMM)
					{
						if (d.U)
							jit_addi_ui(LOCALREG(rn), LOCALREG(shift_out.shiftop), adr);
						else
							jit_rsbi_ui(LOCALREG(rn), LOCALREG(shift_out.shiftop), adr);
					}
					else
					{
						if (d.U)
							jit_addr_ui(LOCALREG(rn), LOCALREG(adr), LOCALREG(shift_out.shiftop));
						else
							jit_subr_ui(LOCALREG(rn), LOCALREG(adr), LOCALREG(shift_out.shiftop));
					}

					regMap.Unlock(rn);
				}

				shift_out.Clean(regMap);
			}
		}

		regMap.DiscardReg(REGID(d.Rd), true);

		u32 tmp = regMap.AllocTempReg();

		std::vector<ABIOp> args;
		std::vector<RegisterMap::GuestRegId> flushs;

		args.clear();
		flushs.clear();

		ABIOp op;

		if (adr_type == TEMPREG)
		{
			op.type = ABIOp::TEMPREG;
			op.regdata = adr;
			args.push_back(op);
		}
		else if (adr_type == IMM)
		{
			op.type = ABIOp::IMM;
			op.immdata.type = ImmData::IMM32;
			op.immdata.imm32 = adr;
			args.push_back(op);
		}

		op.type = ABIOp::GUSETREGPTR;
		op.regdata = d.Rd;
		args.push_back(op);

		MemOp1 func = NULL;

		if (d.B)
			func = LDRB_Tab[PROCNUM][MEMTYPE_GENERIC];
		else
			func = LDR_Tab[PROCNUM][MEMTYPE_GENERIC];

		regMap.CallABI((void*)func, args, flushs, tmp);

		u32 execyc = regMap.MapReg(RegisterMap::EXECUTECYCLES, RegisterMap::MAP_DIRTY);
		regMap.Lock(execyc);

		jit_addr_ui(LOCALREG(execyc), LOCALREG(execyc), LOCALREG(tmp));

		regMap.Unlock(execyc);

		regMap.ReleaseTempReg(tmp);

		if (!d.B && d.R15Modified)
		{
			u32 r15 = regMap.MapReg(RegisterMap::R15, RegisterMap::MAP_DIRTY);
			regMap.Lock(r15);

			if (PROCNUM == 0)
			{
				u32 tmp = regMap.AllocTempReg();

				jit_andi_ui(LOCALREG(tmp), LOCALREG(r15), 1);
				jit_andi_ui(LOCALREG(r15), LOCALREG(r15), 0xFFFFFFFE);

				PackCPSR(regMap, PSR_T, tmp);

				regMap.ReleaseTempReg(tmp);
			}
			else
			{
				jit_andi_ui(LOCALREG(r15), LOCALREG(r15), 0xFFFFFFFC);
			}

			regMap.Unlock(r15);

			R15ModifiedGenerate(d, regMap);
		}
	}

	OPDECODER_DECL(IR_STR)
	{
		u32 PROCNUM = d.ProcessID;

		enum AddressRegType
		{
			TEMPREG,
			GUSETREG,
			IMM
		};

		u32 adr = INVALID_REG_ID;
		AddressRegType adr_type = TEMPREG;

		if (d.P)
		{
			if (d.I)
			{
				if (regMap.IsImm(REGID(d.Rn)))
				{
					if (d.U)
						adr = regMap.GetImm32(REGID(d.Rn)) + d.Immediate;
					else
						adr = regMap.GetImm32(REGID(d.Rn)) - d.Immediate;

					adr_type = IMM;
				}
				else
				{
					u32 rn = regMap.MapReg(REGID(d.Rn));
					regMap.Lock(rn);

					adr = regMap.AllocTempReg();
					adr_type = TEMPREG;

					if (d.U)
						jit_addi_ui(LOCALREG(adr), LOCALREG(rn), d.Immediate);
					else
						jit_subi_ui(LOCALREG(adr), LOCALREG(rn), d.Immediate);

					regMap.Unlock(rn);
				}
			}
			else
			{
				ShiftOut shift_out = IRShiftOpGenerate(d, regMap, false);

				if (regMap.IsImm(REGID(d.Rn)) && shift_out.shiftopimm)
				{
					if (d.U)
						adr = regMap.GetImm32(REGID(d.Rn)) + shift_out.shiftop;
					else
						adr = regMap.GetImm32(REGID(d.Rn)) - shift_out.shiftop;

					adr_type = IMM;
				}
				else
				{
					u32 rn = regMap.MapReg(REGID(d.Rn));
					regMap.Lock(rn);

					adr = regMap.AllocTempReg();
					adr_type = TEMPREG;

					if (shift_out.shiftopimm)
					{
						if (d.U)
							jit_addi_ui(LOCALREG(adr), LOCALREG(rn), shift_out.shiftop);
						else
							jit_subi_ui(LOCALREG(adr), LOCALREG(rn), shift_out.shiftop);
					}
					else
					{
						if (d.U)
							jit_addr_ui(LOCALREG(adr), LOCALREG(rn), LOCALREG(shift_out.shiftop));
						else
							jit_subr_ui(LOCALREG(adr), LOCALREG(rn), LOCALREG(shift_out.shiftop));
					}

					regMap.Unlock(rn);
				}

				shift_out.Clean(regMap);
			}

			if (d.W)
			{
				if (adr_type == IMM)
					regMap.SetImm32(REGID(d.Rn), adr);
				else
				{
					u32 rn = regMap.MapReg(REGID(d.Rn), RegisterMap::MAP_DIRTY | RegisterMap::MAP_NOTINIT);
					regMap.Lock(rn);

					jit_movr_ui(LOCALREG(rn), LOCALREG(adr));

					regMap.Unlock(rn);
				}
			}
		}
		else
		{
			adr = d.Rn;
			adr_type = GUSETREG;
		}

		u32 tmp = regMap.AllocTempReg();

		std::vector<ABIOp> args;
		std::vector<RegisterMap::GuestRegId> flushs;

		args.clear();
		flushs.clear();

		ABIOp op;

		if (adr_type == TEMPREG)
		{
			op.type = ABIOp::TEMPREG;
			op.regdata = adr;
			args.push_back(op);
		}
		else if (adr_type == GUSETREG)
		{
			op.type = ABIOp::GUSETREG;
			op.regdata = adr;
			args.push_back(op);
		}
		else if (adr_type == IMM)
		{
			op.type = ABIOp::IMM;
			op.immdata.type = ImmData::IMM32;
			op.immdata.imm32 = adr;
			args.push_back(op);
		}

		op.type = ABIOp::GUSETREG;
		op.regdata = d.Rd;
		args.push_back(op);

		MemOp2 func = NULL;

		if (d.B)
			func = STRB_Tab[PROCNUM][MEMTYPE_GENERIC];
		else
			func = STR_Tab[PROCNUM][MEMTYPE_GENERIC];

		regMap.CallABI((void*)func, args, flushs, tmp);

		u32 execyc = regMap.MapReg(RegisterMap::EXECUTECYCLES, RegisterMap::MAP_DIRTY);
		regMap.Lock(execyc);

		jit_addr_ui(LOCALREG(execyc), LOCALREG(execyc), LOCALREG(tmp));

		regMap.Unlock(execyc);

		regMap.ReleaseTempReg(tmp);

		if (!d.P)
		{
			if (d.I)
			{
				if (regMap.IsImm(REGID(d.Rn)))
				{
					if (d.U)
						regMap.SetImm32(REGID(d.Rn), regMap.GetImm32(REGID(d.Rn)) + d.Immediate);
					else
						regMap.SetImm32(REGID(d.Rn), regMap.GetImm32(REGID(d.Rn)) - d.Immediate);
				}
				else
				{
					u32 rn = regMap.MapReg(REGID(d.Rn), RegisterMap::MAP_DIRTY);
					regMap.Lock(rn);

					if (d.U)
						jit_addi_ui(LOCALREG(rn), LOCALREG(rn), d.Immediate);
					else
						jit_subi_ui(LOCALREG(rn), LOCALREG(rn), d.Immediate);

					regMap.Unlock(rn);
				}
			}
			else
			{
				ShiftOut shift_out = IRShiftOpGenerate(d, regMap, false);

				if (regMap.IsImm(REGID(d.Rn)) && shift_out.shiftopimm)
				{
					if (d.U)
						regMap.SetImm32(REGID(d.Rn), regMap.GetImm32(REGID(d.Rn)) + shift_out.shiftop);
					else
						regMap.SetImm32(REGID(d.Rn), regMap.GetImm32(REGID(d.Rn)) - shift_out.shiftop);
				}
				else
				{
					u32 rn = regMap.MapReg(REGID(d.Rn), RegisterMap::MAP_DIRTY);
					regMap.Lock(rn);

					if (shift_out.shiftopimm)
					{
						if (d.U)
							jit_addi_ui(LOCALREG(rn), LOCALREG(rn), shift_out.shiftop);
						else
							jit_subi_ui(LOCALREG(rn), LOCALREG(rn), shift_out.shiftop);
					}
					else
					{
						if (d.U)
							jit_addr_ui(LOCALREG(rn), LOCALREG(rn), LOCALREG(shift_out.shiftop));
						else
							jit_subr_ui(LOCALREG(rn), LOCALREG(rn), LOCALREG(shift_out.shiftop));
					}

					regMap.Unlock(rn);
				}

				shift_out.Clean(regMap);
			}
		}
	}

	OPDECODER_DECL(IR_LDRx)
	{
		u32 PROCNUM = d.ProcessID;

		enum AddressRegType
		{
			TEMPREG,
			IMM
		};

		u32 adr = INVALID_REG_ID;
		AddressRegType adr_type = TEMPREG;

		if (d.P)
		{
			if (d.I)
			{
				if (regMap.IsImm(REGID(d.Rn)))
				{
					if (d.U)
						adr = regMap.GetImm32(REGID(d.Rn)) + d.Immediate;
					else
						adr = regMap.GetImm32(REGID(d.Rn)) - d.Immediate;

					adr_type = IMM;
				}
				else
				{
					u32 rn = regMap.MapReg(REGID(d.Rn));
					regMap.Lock(rn);

					adr = regMap.AllocTempReg();
					adr_type = TEMPREG;

					if (d.U)
						jit_addi_ui(LOCALREG(adr), LOCALREG(rn), d.Immediate);
					else
						jit_subi_ui(LOCALREG(adr), LOCALREG(rn), d.Immediate);

					regMap.Unlock(rn);
				}
			}
			else
			{
				if (regMap.IsImm(REGID(d.Rn)) && regMap.IsImm(REGID(d.Rm)))
				{
					if (d.U)
						adr = regMap.GetImm32(REGID(d.Rn)) + regMap.GetImm32(REGID(d.Rm));
					else
						adr = regMap.GetImm32(REGID(d.Rn)) - regMap.GetImm32(REGID(d.Rm));

					adr_type = IMM;
				}
				else
				{
					u32 rn = regMap.MapReg(REGID(d.Rn));
					regMap.Lock(rn);
					u32 rm = regMap.MapReg(REGID(d.Rm));
					regMap.Lock(rm);

					adr = regMap.AllocTempReg();
					adr_type = TEMPREG;

					if (d.U)
						jit_addr_ui(LOCALREG(adr), LOCALREG(rn), LOCALREG(rm));
					else
						jit_subr_ui(LOCALREG(adr), LOCALREG(rn), LOCALREG(rm));

					regMap.Unlock(rm);
					regMap.Unlock(rn);
				}
			}

			if (d.W)
			{
				if (adr_type == IMM)
					regMap.SetImm32(REGID(d.Rn), adr);
				else
				{
					u32 rn = regMap.MapReg(REGID(d.Rn), RegisterMap::MAP_DIRTY | RegisterMap::MAP_NOTINIT);
					regMap.Lock(rn);

					jit_movr_ui(LOCALREG(rn), LOCALREG(adr));

					regMap.Unlock(rn);
				}
			}
		}
		else
		{
			if (regMap.IsImm(REGID(d.Rn)))
			{
				adr = regMap.GetImm32(REGID(d.Rn));

				adr_type = IMM;
			}
			else
			{
				u32 rn = regMap.MapReg(REGID(d.Rn));
				regMap.Lock(rn);

				adr = regMap.AllocTempReg();
				adr_type = TEMPREG;

				jit_movr_ui(LOCALREG(adr), LOCALREG(rn));

				regMap.Unlock(rn);
			}

			if (d.I)
			{
				if (adr_type == IMM)
				{
					if (d.U)
						regMap.SetImm32(REGID(d.Rn), adr + d.Immediate);
					else
						regMap.SetImm32(REGID(d.Rn), adr - d.Immediate);
				}
				else
				{
					u32 rn = regMap.MapReg(REGID(d.Rn), RegisterMap::MAP_DIRTY | RegisterMap::MAP_NOTINIT);
					regMap.Lock(rn);

					if (d.U)
						jit_addi_ui(LOCALREG(rn), LOCALREG(adr), d.Immediate);
					else
						jit_subi_ui(LOCALREG(rn), LOCALREG(adr), d.Immediate);

					regMap.Unlock(rn);
				}
			}
			else
			{
				if (regMap.IsImm(REGID(d.Rm)))
				{
					if (adr_type == IMM)
					{
						if (d.U)
							regMap.SetImm32(REGID(d.Rn), adr + regMap.GetImm32(REGID(d.Rm)));
						else
							regMap.SetImm32(REGID(d.Rn), adr - regMap.GetImm32(REGID(d.Rm)));
					}
					else
					{
						u32 rn = regMap.MapReg(REGID(d.Rn), RegisterMap::MAP_DIRTY | RegisterMap::MAP_NOTINIT);
						regMap.Lock(rn);

						if (d.U)
							jit_addi_ui(LOCALREG(rn), LOCALREG(adr), regMap.GetImm32(REGID(d.Rm)));
						else
							jit_subi_ui(LOCALREG(rn), LOCALREG(adr), regMap.GetImm32(REGID(d.Rm)));

						regMap.Unlock(rn);
					}
				}
				else
				{
					u32 rm = regMap.MapReg(REGID(d.Rm));
					regMap.Lock(rm);
					u32 rn = regMap.MapReg(REGID(d.Rn), RegisterMap::MAP_DIRTY | RegisterMap::MAP_NOTINIT);
					regMap.Lock(rn);

					if (adr_type == IMM)
					{
						if (d.U)
							jit_addi_ui(LOCALREG(rn), LOCALREG(rm), adr);
						else
							jit_rsbi_ui(LOCALREG(rn), LOCALREG(rm), adr);
					}
					else
					{
						if (d.U)
							jit_addr_ui(LOCALREG(rn), LOCALREG(adr), LOCALREG(rm));
						else
							jit_subr_ui(LOCALREG(rn), LOCALREG(adr), LOCALREG(rm));
					}

					regMap.Unlock(rn);
					regMap.Unlock(rm);
				}
			}
		}

		regMap.DiscardReg(REGID(d.Rd), true);

		u32 tmp = regMap.AllocTempReg();

		std::vector<ABIOp> args;
		std::vector<RegisterMap::GuestRegId> flushs;

		args.clear();
		flushs.clear();

		ABIOp op;

		if (adr_type == TEMPREG)
		{
			op.type = ABIOp::TEMPREG;
			op.regdata = adr;
			args.push_back(op);
		}
		else if (adr_type == IMM)
		{
			op.type = ABIOp::IMM;
			op.immdata.type = ImmData::IMM32;
			op.immdata.imm32 = adr;
			args.push_back(op);
		}

		op.type = ABIOp::GUSETREGPTR;
		op.regdata = d.Rd;
		args.push_back(op);

		MemOp1 func = NULL;

		if (d.H)
		{
			if (d.S)
				func = LDRSH_Tab[PROCNUM][MEMTYPE_GENERIC];
			else
				func = LDRH_Tab[PROCNUM][MEMTYPE_GENERIC];
		}
		else
			func = LDRSB_Tab[PROCNUM][MEMTYPE_GENERIC];

		regMap.CallABI((void*)func, args, flushs, tmp);

		u32 execyc = regMap.MapReg(RegisterMap::EXECUTECYCLES, RegisterMap::MAP_DIRTY);
		regMap.Lock(execyc);

		jit_addr_ui(LOCALREG(execyc), LOCALREG(execyc), LOCALREG(tmp));

		regMap.Unlock(execyc);

		regMap.ReleaseTempReg(tmp);
	}

	OPDECODER_DECL(IR_STRx)
	{
		u32 PROCNUM = d.ProcessID;

		enum AddressRegType
		{
			TEMPREG,
			GUSETREG,
			IMM
		};

		u32 adr = INVALID_REG_ID;
		AddressRegType adr_type = TEMPREG;

		if (d.P)
		{
			if (d.I)
			{
				if (regMap.IsImm(REGID(d.Rn)))
				{
					if (d.U)
						adr = regMap.GetImm32(REGID(d.Rn)) + d.Immediate;
					else
						adr = regMap.GetImm32(REGID(d.Rn)) - d.Immediate;

					adr_type = IMM;
				}
				else
				{
					u32 rn = regMap.MapReg(REGID(d.Rn));
					regMap.Lock(rn);

					adr = regMap.AllocTempReg();
					adr_type = TEMPREG;

					if (d.U)
						jit_addi_ui(LOCALREG(adr), LOCALREG(rn), d.Immediate);
					else
						jit_subi_ui(LOCALREG(adr), LOCALREG(rn), d.Immediate);

					regMap.Unlock(rn);
				}
			}
			else
			{
				if (regMap.IsImm(REGID(d.Rn)) && regMap.IsImm(REGID(d.Rm)))
				{
					if (d.U)
						adr = regMap.GetImm32(REGID(d.Rn)) + regMap.GetImm32(REGID(d.Rm));
					else
						adr = regMap.GetImm32(REGID(d.Rn)) - regMap.GetImm32(REGID(d.Rm));

					adr_type = IMM;
				}
				else
				{
					u32 rn = regMap.MapReg(REGID(d.Rn));
					regMap.Lock(rn);
					u32 rm = regMap.MapReg(REGID(d.Rm));
					regMap.Lock(rm);

					adr = regMap.AllocTempReg();
					adr_type = TEMPREG;

					if (d.U)
						jit_addr_ui(LOCALREG(adr), LOCALREG(rn), LOCALREG(rm));
					else
						jit_subr_ui(LOCALREG(adr), LOCALREG(rn), LOCALREG(rm));

					regMap.Unlock(rm);
					regMap.Unlock(rn);
				}
			}

			if (d.W)
			{
				if (adr_type == IMM)
					regMap.SetImm32(REGID(d.Rn), adr);
				else
				{
					u32 rn = regMap.MapReg(REGID(d.Rn), RegisterMap::MAP_DIRTY | RegisterMap::MAP_NOTINIT);
					regMap.Lock(rn);

					jit_movr_ui(LOCALREG(rn), LOCALREG(adr));

					regMap.Unlock(rn);
				}
			}
		}
		else
		{
			adr = d.Rn;
			adr_type = GUSETREG;
		}

		u32 tmp = regMap.AllocTempReg();

		std::vector<ABIOp> args;
		std::vector<RegisterMap::GuestRegId> flushs;

		args.clear();
		flushs.clear();

		ABIOp op;

		if (adr_type == TEMPREG)
		{
			op.type = ABIOp::TEMPREG;
			op.regdata = adr;
			args.push_back(op);
		}
		else if (adr_type == GUSETREG)
		{
			op.type = ABIOp::GUSETREG;
			op.regdata = adr;
			args.push_back(op);
		}
		else if (adr_type == IMM)
		{
			op.type = ABIOp::IMM;
			op.immdata.type = ImmData::IMM32;
			op.immdata.imm32 = adr;
			args.push_back(op);
		}

		op.type = ABIOp::GUSETREG;
		op.regdata = d.Rd;
		args.push_back(op);

		MemOp2 func = STRH_Tab[PROCNUM][MEMTYPE_GENERIC];

		regMap.CallABI((void*)func, args, flushs, tmp);

		u32 execyc = regMap.MapReg(RegisterMap::EXECUTECYCLES, RegisterMap::MAP_DIRTY);
		regMap.Lock(execyc);

		jit_addr_ui(LOCALREG(execyc), LOCALREG(execyc), LOCALREG(tmp));

		regMap.Unlock(execyc);

		regMap.ReleaseTempReg(tmp);

		if (!d.P)
		{
			if (d.I)
			{
				if (regMap.IsImm(REGID(d.Rn)))
				{
					if (d.U)
						regMap.SetImm32(REGID(d.Rn), regMap.GetImm32(REGID(d.Rn)) + d.Immediate);
					else
						regMap.SetImm32(REGID(d.Rn), regMap.GetImm32(REGID(d.Rn)) - d.Immediate);
				}
				else
				{
					u32 rn = regMap.MapReg(REGID(d.Rn), RegisterMap::MAP_DIRTY);
					regMap.Lock(rn);

					if (d.U)
						jit_addi_ui(LOCALREG(rn), LOCALREG(rn), d.Immediate);
					else
						jit_subi_ui(LOCALREG(rn), LOCALREG(rn), d.Immediate);

					regMap.Unlock(rn);
				}
			}
			else
			{
				if (regMap.IsImm(REGID(d.Rn)) && regMap.IsImm(REGID(d.Rm)))
				{
					if (d.U)
						regMap.SetImm32(REGID(d.Rn), regMap.GetImm32(REGID(d.Rn)) + regMap.GetImm32(REGID(d.Rm)));
					else
						regMap.SetImm32(REGID(d.Rn), regMap.GetImm32(REGID(d.Rn)) - regMap.GetImm32(REGID(d.Rm)));
				}
				else
				{
					u32 rn = regMap.MapReg(REGID(d.Rn), RegisterMap::MAP_DIRTY);
					regMap.Lock(rn);
					u32 rm = regMap.MapReg(REGID(d.Rm));
					regMap.Lock(rm);

					if (d.U)
						jit_addr_ui(LOCALREG(rn), LOCALREG(rn), LOCALREG(rm));
					else
						jit_subr_ui(LOCALREG(rn), LOCALREG(rn), LOCALREG(rm));

					regMap.Unlock(rm);
					regMap.Unlock(rn);
				}
			}
		}
	}

	OPDECODER_DECL(IR_LDRD)
	{
		u32 PROCNUM = d.ProcessID;

		// fallback to interpreter
		if(!d.I)
			regMap.FlushGuestReg(REGID(d.Rm));
		regMap.FlushGuestReg(REGID(d.Rn));
		regMap.FlushGuestReg(REGID(d.Rd));
		regMap.FlushGuestReg(REGID(d.Rd+1));

		Fallback2Interpreter(d, regMap);
	}

	OPDECODER_DECL(IR_STRD)
	{
		u32 PROCNUM = d.ProcessID;

		// fallback to interpreter
		if(!d.I)
			regMap.FlushGuestReg(REGID(d.Rm));
		regMap.FlushGuestReg(REGID(d.Rn));
		regMap.FlushGuestReg(REGID(d.Rd));
		regMap.FlushGuestReg(REGID(d.Rd+1));

		Fallback2Interpreter(d, regMap);
	}

	OPDECODER_DECL(IR_LDREX)
	{
		u32 PROCNUM = d.ProcessID;

		// fallback to interpreter
		regMap.FlushGuestReg(REGID(d.Rn));
		regMap.FlushGuestReg(REGID(d.Rd));

		Fallback2Interpreter(d, regMap);
	}

	OPDECODER_DECL(IR_STREX)
	{
		u32 PROCNUM = d.ProcessID;

		// fallback to interpreter
		regMap.FlushGuestReg(REGID(d.Rn));
		regMap.FlushGuestReg(REGID(d.Rd));

		Fallback2Interpreter(d, regMap);
	}

	OPDECODER_DECL(IR_LDM)
	{
		u32 PROCNUM = d.ProcessID;

		// fallback to interpreter
		regMap.FlushGuestReg(REGID(d.Rn));
		for (u32 RegisterList = d.RegisterList, n = 0; RegisterList; RegisterList >>= 1, n++)
		{
			if (RegisterList & 0x1)
				regMap.FlushGuestReg(REGID(n));
		}
		if (d.TbitModified)
			regMap.FlushGuestReg(RegisterMap::CPSR);
		if (d.S)
		{
			for (u32 i = RegisterMap::R8; i <= RegisterMap::R14; i++)
				regMap.FlushGuestReg((RegisterMap::GuestRegId)i);
			regMap.FlushGuestReg(RegisterMap::CPSR);
			regMap.FlushGuestReg(RegisterMap::SPSR);
		}

		Fallback2Interpreter(d, regMap);

		if (d.R15Modified)
			R15ModifiedGenerate(d, regMap);
	}

	OPDECODER_DECL(IR_STM)
	{
		u32 PROCNUM = d.ProcessID;

		// fallback to interpreter
		regMap.FlushGuestReg(REGID(d.Rn));
		for (u32 RegisterList = d.RegisterList, n = 0; RegisterList; RegisterList >>= 1, n++)
		{
			if (RegisterList & 0x1)
				regMap.FlushGuestReg(REGID(n));
		}
		if (d.S)
		{
			for (u32 i = RegisterMap::R8; i <= RegisterMap::R14; i++)
				regMap.FlushGuestReg((RegisterMap::GuestRegId)i);
			regMap.FlushGuestReg(RegisterMap::CPSR);
			regMap.FlushGuestReg(RegisterMap::SPSR);
		}

		Fallback2Interpreter(d, regMap);
	}

	OPDECODER_DECL(IR_SWP)
	{
		u32 PROCNUM = d.ProcessID;

		// fallback to interpreter
		regMap.FlushGuestReg(REGID(d.Rn));
		regMap.FlushGuestReg(REGID(d.Rd));
		regMap.FlushGuestReg(REGID(d.Rm));

		Fallback2Interpreter(d, regMap);
	}

	OPDECODER_DECL(IR_B)
	{
		u32 PROCNUM = d.ProcessID;

		regMap.SetImm32(RegisterMap::R15, d.Immediate);

		R15ModifiedGenerate(d, regMap);
	}

	OPDECODER_DECL(IR_BL)
	{
		u32 PROCNUM = d.ProcessID;

		regMap.SetImm32(RegisterMap::R14, d.CalcNextInstruction(d) | d.ThumbFlag);
		regMap.SetImm32(RegisterMap::R15, d.Immediate);

		R15ModifiedGenerate(d, regMap);
	}

	OPDECODER_DECL(IR_BX)
	{
		u32 PROCNUM = d.ProcessID;

		u32 rn = regMap.MapReg(REGID(d.Rn));
		regMap.Lock(rn);

		u32 tmp = regMap.AllocTempReg();

		jit_movr_ui(LOCALREG(tmp), LOCALREG(rn));

		regMap.Unlock(rn);

		u32 r15 = regMap.MapReg(RegisterMap::R15, RegisterMap::MAP_DIRTY | RegisterMap::MAP_NOTINIT);
		regMap.Lock(r15);

		jit_andi_ui(LOCALREG(r15), LOCALREG(tmp), 1);
		jit_lshi_ui(LOCALREG(r15), LOCALREG(r15), 1);
		jit_ori_ui(LOCALREG(r15), LOCALREG(r15), 0xFFFFFFFC);
		jit_andr_ui(LOCALREG(r15), LOCALREG(tmp), LOCALREG(r15));

		regMap.Unlock(r15);

		jit_andi_ui(LOCALREG(tmp), LOCALREG(tmp), 1);
		PackCPSR(regMap, PSR_T, tmp);

		regMap.ReleaseTempReg(tmp);

		R15ModifiedGenerate(d, regMap);
	}

	OPDECODER_DECL(IR_BLX)
	{
		u32 PROCNUM = d.ProcessID;

		u32 rn = regMap.MapReg(REGID(d.Rn));
		regMap.Lock(rn);

		u32 tmp = regMap.AllocTempReg();

		jit_movr_ui(LOCALREG(tmp), LOCALREG(rn));

		regMap.Unlock(rn);

		u32 r15 = regMap.MapReg(RegisterMap::R15, RegisterMap::MAP_DIRTY | RegisterMap::MAP_NOTINIT);
		regMap.Lock(r15);

		jit_andi_ui(LOCALREG(r15), LOCALREG(tmp), 1);
		jit_lshi_ui(LOCALREG(r15), LOCALREG(r15), 1);
		jit_ori_ui(LOCALREG(r15), LOCALREG(r15), 0xFFFFFFFC);
		jit_andr_ui(LOCALREG(r15), LOCALREG(tmp), LOCALREG(r15));

		regMap.Unlock(r15);

		jit_andi_ui(LOCALREG(tmp), LOCALREG(tmp), 1);
		PackCPSR(regMap, PSR_T, tmp);

		regMap.ReleaseTempReg(tmp);

		regMap.SetImm32(RegisterMap::R14, d.CalcNextInstruction(d) | d.ThumbFlag);

		R15ModifiedGenerate(d, regMap);
	}

	OPDECODER_DECL(IR_SWI)
	{
		u32 PROCNUM = d.ProcessID;

		std::vector<ABIOp> args;
		std::vector<RegisterMap::GuestRegId> flushs;

		bool bypassBuiltinSWI = 
			(GETCPU.intVector == 0x00000000 && PROCNUM==0)
			|| (GETCPU.intVector == 0xFFFF0000 && PROCNUM==1);

		if (GETCPU.swi_tab && !bypassBuiltinSWI)
		{
			if (d.MayHalt)
			{
				u32 cpuptr = regMap.GetCpuPtrReg();
				u32 tmp = regMap.AllocTempReg();

				jit_movi_ui(LOCALREG(tmp), d.Address);
				jit_stxi_ui(jit_field(armcpu_t, instruct_adr), LOCALREG(cpuptr), LOCALREG(tmp));

				jit_movi_ui(LOCALREG(tmp), d.CalcNextInstruction(d));
				jit_stxi_ui(jit_field(armcpu_t, next_instruction), LOCALREG(cpuptr), LOCALREG(tmp));

				//regMap.SetImm32(RegisterMap::R15, d.CalcNextInstruction(d));
				//regMap.FlushGuestReg(RegisterMap::R15);

				regMap.ReleaseTempReg(tmp);
			}

			u32 tmp = regMap.AllocTempReg();

			args.clear();
			flushs.clear();

			for (u32 i = RegisterMap::R0; i <= RegisterMap::R3; i++)
				flushs.push_back((RegisterMap::GuestRegId)i);

			regMap.CallABI((void*)GETCPU.swi_tab[d.Immediate], args, flushs, tmp);

			u32 execyc = regMap.MapReg(RegisterMap::EXECUTECYCLES, RegisterMap::MAP_DIRTY);
			regMap.Lock(execyc);

			jit_addr_ui(LOCALREG(execyc), LOCALREG(execyc), LOCALREG(tmp));
			jit_addi_ui(LOCALREG(execyc), LOCALREG(execyc), 3);

			regMap.Unlock(execyc);

			regMap.ReleaseTempReg(tmp);

			if (d.MayHalt)
			{
				u32 cpuptr = regMap.GetCpuPtrReg();
				u32 tmp = regMap.AllocTempReg();

				jit_ldxi_ui(LOCALREG(tmp), LOCALREG(cpuptr), jit_field(armcpu_t, next_instruction));
				jit_stxi_ui(jit_field(armcpu_t, instruct_adr), LOCALREG(cpuptr), LOCALREG(tmp));

				regMap.ReleaseTempReg(tmp);
			}
		}
		else
		{
			u32 tmp = regMap.AllocTempReg(true);

			u32 cpsr = regMap.MapReg(RegisterMap::CPSR);
			regMap.Lock(cpsr);

			jit_movr_ui(LOCALREG(tmp), LOCALREG(cpsr));

			regMap.Unlock(cpsr);

			{
				args.clear();
				flushs.clear();

				for (u32 i = RegisterMap::R8; i <= RegisterMap::R14; i++)
					flushs.push_back((RegisterMap::GuestRegId)i);
				flushs.push_back(RegisterMap::CPSR);
				flushs.push_back(RegisterMap::SPSR);

				ABIOp op;

				op.type = ABIOp::HOSTREG;
				op.regdata = regMap.GetCpuPtrReg();
				args.push_back(op);

				op.type = ABIOp::IMM;
				op.immdata.type = ImmData::IMM8;
				op.immdata.imm8 = SVC;
				args.push_back(op);

				regMap.CallABI((void*)&armcpu_switchMode, args, flushs);
			}

			regMap.SetImm32(RegisterMap::R14, d.CalcNextInstruction(d));

			u32 spsr = regMap.MapReg(RegisterMap::SPSR);

			jit_movr_ui(LOCALREG(spsr), LOCALREG(tmp));

			regMap.Unlock(spsr);

			regMap.ReleaseTempReg(tmp);

			PackCPSRImm(regMap, PSR_T, 0);
			PackCPSRImm(regMap, PSR_I, 1);

			{
				args.clear();
				flushs.clear();

				ABIOp op;

				op.type = ABIOp::HOSTREG;
				op.regdata = regMap.GetCpuPtrReg();
				args.push_back(op);

				regMap.CallABI((void*)&armcpu_changeCPSR, args, flushs);
			}

			u32 r15 = regMap.MapReg(RegisterMap::R15, RegisterMap::MAP_DIRTY | RegisterMap::MAP_NOTINIT);
			regMap.Lock(r15);
			u32 cpuptr = regMap.GetCpuPtrReg();

			jit_ldxi_ui(LOCALREG(r15), LOCALREG(cpuptr), jit_field(armcpu_t, intVector));
			jit_addi_ui(LOCALREG(r15), LOCALREG(r15), 0x08);

			regMap.Unlock(r15);

			u32 execyc = regMap.MapReg(RegisterMap::EXECUTECYCLES, RegisterMap::MAP_DIRTY);

			jit_addi_ui(LOCALREG(execyc), LOCALREG(execyc), 3);

			regMap.Unlock(execyc);

			R15ModifiedGenerate(d, regMap);
		}
	}

	OPDECODER_DECL(IR_MSR)
	{
		u32 PROCNUM = d.ProcessID;

		std::vector<ABIOp> args;
		std::vector<RegisterMap::GuestRegId> flushs;
		std::vector<u32> states;

		if (d.P)
		{
			u32 byte_mask = (BIT0(d.OpData)?0x000000FF:0x00000000) |
							(BIT1(d.OpData)?0x0000FF00:0x00000000) |
							(BIT2(d.OpData)?0x00FF0000:0x00000000) |
							(BIT3(d.OpData)?0xFF000000:0x00000000);

			u32 state_start = regMap.StoreState();

			u32 tmp = regMap.AllocTempReg();

			UnpackCPSR(regMap, PSR_MODE, tmp);
			jit_insn *done1 = jit_beqi_ui(jit_forward(), LOCALREG(tmp), USR);
			jit_insn *done2 = jit_beqi_ui(jit_forward(), LOCALREG(tmp), SYS);

			regMap.ReleaseTempReg(tmp);

			u32 spsr = regMap.MapReg(RegisterMap::SPSR, RegisterMap::MAP_DIRTY);
			regMap.Lock(spsr);

			if (d.I)
			{
				jit_andi_ui(LOCALREG(spsr), LOCALREG(spsr), ~byte_mask);
				jit_ori_ui(LOCALREG(spsr), LOCALREG(spsr), byte_mask & d.Immediate);
			}
			else
			{
				u32 rm = regMap.MapReg(REGID(d.Rm));
				regMap.Lock(rm);
				u32 tmp = regMap.AllocTempReg();

				jit_andi_ui(LOCALREG(spsr), LOCALREG(spsr), ~byte_mask);
				jit_andi_ui(LOCALREG(tmp), LOCALREG(rm), byte_mask);
				jit_orr_ui(LOCALREG(spsr), LOCALREG(spsr), LOCALREG(tmp));

				regMap.ReleaseTempReg(tmp);
				regMap.Unlock(rm);
			}

			regMap.Unlock(spsr);

			{
				args.clear();
				flushs.clear();

				ABIOp op;

				op.type = ABIOp::HOSTREG;
				op.regdata = regMap.GetCpuPtrReg();
				args.push_back(op);

				regMap.CallABI((void*)&armcpu_changeCPSR, args, flushs);
			}

			jit_insn* pt_end1 = PrepareSLZone();

			u32 state_end1 = regMap.StoreState();

			jit_patch(done1);
			jit_patch(done2);

			regMap.RestoreState(state_start);
			u32 state_end2 = regMap.StoreState();

			// merge states
			states.clear();
			states.push_back(state_end1);
			states.push_back(state_end2);

			u32 state_merge = regMap.CalcStates(state_start, states);

			// generate merge code
			regMap.RestoreState(state_end2);
			regMap.MergeToStates(state_merge);

			// backup pc
			jit_insn* lable_end = jit_get_label();

			// generate merge code
			regMap.RestoreState(state_end1);
			jit_set_ip(pt_end1);
			regMap.MergeToStates(state_merge);
			jit_jmpi(lable_end);

			// restore pc
			regMap.RestoreState(state_merge);
			jit_set_ip(lable_end);

			regMap.CleanState(state_start);
			regMap.CleanState(state_end1);
			regMap.CleanState(state_end2);
			regMap.CleanState(state_merge);
		}
		else
		{
			u32 byte_mask_usr = (BIT3(d.OpData)?0xFF000000:0x00000000);
			u32 byte_mask_other = (BIT0(d.OpData)?0x000000FF:0x00000000) |
								(BIT1(d.OpData)?0x0000FF00:0x00000000) |
								(BIT2(d.OpData)?0x00FF0000:0x00000000) |
								(BIT3(d.OpData)?0xFF000000:0x00000000);

			u32 byte_mask = regMap.AllocTempReg(true);
			u32 mode = regMap.AllocTempReg();

			UnpackCPSR(regMap, PSR_MODE, mode);
			jit_insn *eq_usr = jit_beqi_ui(jit_forward(), LOCALREG(mode), USR);
			jit_movi_ui(LOCALREG(byte_mask), byte_mask_other);
			jit_insn *eq_usr_done = jit_jmpi(jit_forward());
			jit_patch(eq_usr);
			jit_movi_ui(LOCALREG(byte_mask), byte_mask_usr);
			jit_patch(eq_usr_done);

			if (BIT0(d.OpData))
			{
				jit_insn *done = jit_beqi_ui(jit_forward(), LOCALREG(mode), USR);
				regMap.ReleaseTempReg(mode);

				u32 state_start = regMap.StoreState();

				{
					args.clear();
					flushs.clear();

					for (u32 i = RegisterMap::R8; i <= RegisterMap::R14; i++)
						flushs.push_back((RegisterMap::GuestRegId)i);
					flushs.push_back(RegisterMap::CPSR);
					flushs.push_back(RegisterMap::SPSR);

					ABIOp op;

					op.type = ABIOp::HOSTREG;
					op.regdata = regMap.GetCpuPtrReg();
					args.push_back(op);

					if (d.I)
					{
						op.type = ABIOp::IMM;
						op.immdata.type = ImmData::IMM8;
						op.immdata.imm8 = d.Immediate & 0x1F;
						args.push_back(op);
					}
					else
					{
						u32 rm = regMap.MapReg(REGID(d.Rm));
						regMap.Lock(rm);
						u32 tmp = regMap.AllocTempReg();

						jit_andi_ui(LOCALREG(tmp), LOCALREG(rm), 0x1F);

						regMap.Unlock(rm);

						op.type = ABIOp::TEMPREG;
						op.regdata = tmp;
						args.push_back(op);
					}

					regMap.CallABI((void*)&armcpu_switchMode, args, flushs);
				}

				jit_insn* pt_end1 = PrepareSLZone();

				u32 state_end1 = regMap.StoreState();

				jit_patch(done);

				regMap.RestoreState(state_start);
				u32 state_end2 = regMap.StoreState();

				// merge states
				states.clear();
				states.push_back(state_end1);
				states.push_back(state_end2);

				u32 state_merge = regMap.CalcStates(state_start, states);

				// generate merge code
				regMap.RestoreState(state_end2);
				regMap.MergeToStates(state_merge);

				// backup pc
				jit_insn* lable_end = jit_get_label();

				// generate merge code
				regMap.RestoreState(state_end1);
				jit_set_ip(pt_end1);
				regMap.MergeToStates(state_merge);
				jit_jmpi(lable_end);

				// restore pc
				regMap.RestoreState(state_merge);
				jit_set_ip(lable_end);

				regMap.CleanState(state_start);
				regMap.CleanState(state_end1);
				regMap.CleanState(state_end2);
				regMap.CleanState(state_merge);
			}
			else
				regMap.ReleaseTempReg(mode);

			u32 cpsr = regMap.MapReg(RegisterMap::CPSR, RegisterMap::MAP_DIRTY);
			regMap.Lock(cpsr);
			u32 tmp = regMap.AllocTempReg();

			jit_notr_ui(LOCALREG(tmp), LOCALREG(byte_mask));
			jit_andr_ui(LOCALREG(cpsr), LOCALREG(cpsr), LOCALREG(tmp));

			if (d.I)
				jit_andi_ui(LOCALREG(tmp), LOCALREG(byte_mask), d.Immediate);
			else
			{
				u32 rm = regMap.MapReg(REGID(d.Rm));
				regMap.Lock(rm);

				jit_andr_ui(LOCALREG(tmp), LOCALREG(byte_mask), LOCALREG(rm));

				regMap.Unlock(rm);
			}

			jit_orr_ui(LOCALREG(cpsr), LOCALREG(cpsr), LOCALREG(tmp));

			regMap.ReleaseTempReg(tmp);
			regMap.Unlock(cpsr);

			regMap.ReleaseTempReg(byte_mask);

			{
				args.clear();
				flushs.clear();

				ABIOp op;

				op.type = ABIOp::HOSTREG;
				op.regdata = regMap.GetCpuPtrReg();
				args.push_back(op);

				regMap.CallABI((void*)&armcpu_changeCPSR, args, flushs);
			}
		}
	}

	OPDECODER_DECL(IR_MRS)
	{
		u32 PROCNUM = d.ProcessID;

		u32 rd = regMap.MapReg(REGID(d.Rd), RegisterMap::MAP_DIRTY | RegisterMap::MAP_NOTINIT);
		regMap.Lock(rd);
		u32 psr = regMap.MapReg(d.P ? RegisterMap::SPSR : RegisterMap::CPSR);
		regMap.Lock(psr);

		jit_movr_ui(LOCALREG(rd), LOCALREG(psr));

		regMap.Unlock(psr);
		regMap.Unlock(rd);
	}

	OPDECODER_DECL(IR_MCR)
	{
		u32 PROCNUM = d.ProcessID;

		if (d.CPNum == 15)
		{
			std::vector<ABIOp> args;
			std::vector<RegisterMap::GuestRegId> flushs;

			{
				args.clear();
				flushs.clear();

				flushs.push_back(RegisterMap::CPSR);

				ABIOp op;

				op.type = ABIOp::GUSETREG;
				op.regdata = REGID(d.Rd);
				args.push_back(op);

				op.type = ABIOp::IMM;
				op.immdata.type = ImmData::IMM8;
				op.immdata.imm8 = d.CRn;
				args.push_back(op);

				op.type = ABIOp::IMM;
				op.immdata.type = ImmData::IMM8;
				op.immdata.imm8 = d.CRm;
				args.push_back(op);

				op.type = ABIOp::IMM;
				op.immdata.type = ImmData::IMM8;
				op.immdata.imm8 = d.CPOpc;
				args.push_back(op);

				op.type = ABIOp::IMM;
				op.immdata.type = ImmData::IMM8;
				op.immdata.imm8 = d.CP;
				args.push_back(op);

				regMap.CallABI((void*)&armcp15_moveARM2CP, args, flushs);
			}
		}
		else
		{
			INFO("ARM%c: MCR P%i, 0, R%i, C%i, C%i, %i, %i (don't allocated coprocessor)\n", 
				PROCNUM?'7':'9', d.CPNum, d.Rd, d.CRn, d.CRm, d.CPOpc, d.CP);
		}
	}

	OPDECODER_DECL(IR_MRC)
	{
		u32 PROCNUM = d.ProcessID;

		if (d.CPNum == 15)
		{
			std::vector<ABIOp> args;
			std::vector<RegisterMap::GuestRegId> flushs;

			if (d.Rd == 15)
			{
				// fallback to interpreter
				regMap.FlushGuestReg(RegisterMap::CPSR);

				Fallback2Interpreter(d, regMap);
			}
			else
			{
				args.clear();
				flushs.clear();

				flushs.push_back(RegisterMap::CPSR);
				flushs.push_back(REGID(d.Rd));

				ABIOp op;

				op.type = ABIOp::IMM;
				op.immdata.type = ImmData::IMMPTR;
				op.immdata.immptr = &GETCPU.R[d.Rd];
				args.push_back(op);

				op.type = ABIOp::IMM;
				op.immdata.type = ImmData::IMM8;
				op.immdata.imm8 = d.CRn;
				args.push_back(op);

				op.type = ABIOp::IMM;
				op.immdata.type = ImmData::IMM8;
				op.immdata.imm8 = d.CRm;
				args.push_back(op);

				op.type = ABIOp::IMM;
				op.immdata.type = ImmData::IMM8;
				op.immdata.imm8 = d.CPOpc;
				args.push_back(op);

				op.type = ABIOp::IMM;
				op.immdata.type = ImmData::IMM8;
				op.immdata.imm8 = d.CP;
				args.push_back(op);

				regMap.CallABI((void*)&armcp15_moveCP2ARM, args, flushs);
			}
		}
		else
		{
			INFO("ARM%c: MRC P%i, 0, R%i, C%i, C%i, %i, %i (don't allocated coprocessor)\n", 
				PROCNUM?'7':'9', d.CPNum, d.Rd, d.CRn, d.CRm, d.CPOpc, d.CP);
		}
	}

	OPDECODER_DECL(IR_CLZ)
	{
		struct CLZImp
		{
			static u32 Method(u32 in)
			{
				if (in == 0)
					return 32;

		#ifdef _MSC_VER
				unsigned long out;
				_BitScanReverse(&out, in);
				return out^0x1F;
		#else
				return __builtin_clz(in);
		#endif
			}
		};

		u32 PROCNUM = d.ProcessID;

		std::vector<ABIOp> args;
		std::vector<RegisterMap::GuestRegId> flushs;

		ABIOp op;
		op.type = ABIOp::GUSETREG;
		op.regdata = REGID(d.Rm);
		args.push_back(op);

		u32 tmp = regMap.AllocTempReg();

		regMap.CallABI((void*)&CLZImp::Method, args, flushs, tmp);

		u32 rd = regMap.MapReg(REGID(d.Rd), RegisterMap::MAP_DIRTY | RegisterMap::MAP_NOTINIT);
		regMap.Lock(rd);

		jit_movr_ui(LOCALREG(rd), LOCALREG(tmp));

		regMap.Unlock(rd);
		regMap.ReleaseTempReg(tmp);
	}

	OPDECODER_DECL(IR_QADD)
	{
		u32 PROCNUM = d.ProcessID;

		// fallback to interpreter
		regMap.FlushGuestReg(REGID(d.Rn));
		regMap.FlushGuestReg(REGID(d.Rm));
		regMap.FlushGuestReg(REGID(d.Rd));
		regMap.FlushGuestReg(RegisterMap::CPSR);

		Fallback2Interpreter(d, regMap);
		if (d.R15Modified)
		{
			u32 r15 = regMap.MapReg(REGID(d.Rd), RegisterMap::MAP_DIRTY);
			regMap.Lock(r15);

			jit_andi_ui(LOCALREG(r15), LOCALREG(r15), 0xFFFFFFFC);

			regMap.Unlock(r15);

			R15ModifiedGenerate(d, regMap);
		}
	}

	OPDECODER_DECL(IR_QSUB)
	{
		u32 PROCNUM = d.ProcessID;

		// fallback to interpreter
		regMap.FlushGuestReg(REGID(d.Rn));
		regMap.FlushGuestReg(REGID(d.Rm));
		regMap.FlushGuestReg(REGID(d.Rd));
		regMap.FlushGuestReg(RegisterMap::CPSR);

		Fallback2Interpreter(d, regMap);
		if (d.R15Modified)
		{
			u32 r15 = regMap.MapReg(REGID(d.Rd), RegisterMap::MAP_DIRTY);
			regMap.Lock(r15);

			jit_andi_ui(LOCALREG(r15), LOCALREG(r15), 0xFFFFFFFC);

			regMap.Unlock(r15);

			R15ModifiedGenerate(d, regMap);
		}
	}

	OPDECODER_DECL(IR_QDADD)
	{
		u32 PROCNUM = d.ProcessID;

		// fallback to interpreter
		regMap.FlushGuestReg(REGID(d.Rn));
		regMap.FlushGuestReg(REGID(d.Rm));
		regMap.FlushGuestReg(REGID(d.Rd));
		regMap.FlushGuestReg(RegisterMap::CPSR);

		Fallback2Interpreter(d, regMap);
		if (d.R15Modified)
		{
			u32 r15 = regMap.MapReg(REGID(d.Rd), RegisterMap::MAP_DIRTY);
			regMap.Lock(r15);

			jit_andi_ui(LOCALREG(r15), LOCALREG(r15), 0xFFFFFFFC);

			regMap.Unlock(r15);

			R15ModifiedGenerate(d, regMap);
		}
	}

	OPDECODER_DECL(IR_QDSUB)
	{
		u32 PROCNUM = d.ProcessID;

		// fallback to interpreter
		regMap.FlushGuestReg(REGID(d.Rn));
		regMap.FlushGuestReg(REGID(d.Rm));
		regMap.FlushGuestReg(REGID(d.Rd));
		regMap.FlushGuestReg(RegisterMap::CPSR);

		Fallback2Interpreter(d, regMap);
		if (d.R15Modified)
		{
			u32 r15 = regMap.MapReg(REGID(d.Rd), RegisterMap::MAP_DIRTY);
			regMap.Lock(r15);

			jit_andi_ui(LOCALREG(r15), LOCALREG(r15), 0xFFFFFFFC);

			regMap.Unlock(r15);

			R15ModifiedGenerate(d, regMap);
		}
	}

	OPDECODER_DECL(IR_BLX_IMM)
	{
		u32 PROCNUM = d.ProcessID;

		if(d.ThumbFlag)
			PackCPSRImm(regMap, PSR_T, 0);
		else
			PackCPSRImm(regMap, PSR_T, 1);

		regMap.SetImm32(RegisterMap::R14, d.CalcNextInstruction(d) | d.ThumbFlag);
		regMap.SetImm32(RegisterMap::R15, d.Immediate);

		R15ModifiedGenerate(d, regMap);
	}

	OPDECODER_DECL(IR_BKPT)
	{
		u32 PROCNUM = d.ProcessID;

		INFO("ARM%c: Unimplemented opcode BKPT\n", PROCNUM?'7':'9');
	}
};

static const IROpDecoder iropdecoder_set[IR_MAXNUM] = {
#define TABDECL(x) ArmLJit::x##_Decoder
#include "ArmAnalyze_tabdef.inc"
#undef TABDECL
};

//------------------------------------------------------------
//                         Code Buffer
//------------------------------------------------------------
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

	FlushIcacheSection(base, *(base + size));
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

static void FreeCodeBuffer(size_t size)
{
	s_CodeBuffer->Free(size);
}

//------------------------------------------------------------
//                         Cpu
//------------------------------------------------------------
using namespace ArmLJit;

static ArmAnalyze *s_pArmAnalyze = NULL;
static RegisterMap *s_pRegisterMap = NULL;

static void AddExecuteCycles(u32 ConstCycles)
{
	if (s_pRegisterMap->IsImm(RegisterMap::EXECUTECYCLES))
	{
		u32 execyc = s_pRegisterMap->GetImm32(RegisterMap::EXECUTECYCLES);
		s_pRegisterMap->SetImm32(RegisterMap::EXECUTECYCLES, execyc + ConstCycles);
	}
	else
	{
		u32 execyc = s_pRegisterMap->MapReg(RegisterMap::EXECUTECYCLES, RegisterMap::MAP_DIRTY);
		s_pRegisterMap->Lock(execyc);

		jit_addi_ui(LOCALREG(execyc), LOCALREG(execyc), ConstCycles);

		s_pRegisterMap->Unlock(execyc);
	}
}

static void EndSubBlock(u32 state_start, u32 state_end1, u32 state_end2, jit_insn* pt_end1)
{
	std::vector<u32> states;

	// merge states
	states.clear();
	states.push_back(state_end1);
	states.push_back(state_end2);

	u32 state_merge = s_pRegisterMap->CalcStates(state_start, states);

	// generate merge code
	s_pRegisterMap->RestoreState(state_end2);
	s_pRegisterMap->MergeToStates(state_merge);

	// backup pc
	jit_insn* lable_end = jit_get_label();

	// generate merge code
	s_pRegisterMap->RestoreState(state_end1);
	jit_set_ip(pt_end1);
	s_pRegisterMap->MergeToStates(state_merge);
	jit_jmpi(lable_end);

	// restore pc
	s_pRegisterMap->RestoreState(state_merge);
	jit_set_ip(lable_end);

	s_pRegisterMap->CleanState(state_merge);
}

TEMPLATE static void armcpu_compileblock(BlockInfo &blockinfo, bool runblock)
{
	struct PrintBlock
	{
		static void Method(u32 ProcessID, u32 adr)
		{
			INFO("%s %x\n", CPU_STR(ProcessID), adr);
		}
	};

	static const u32 CondSimple[] = {PSR_Z_BITMASK, PSR_Z_BITMASK, 
									PSR_C_BITMASK, PSR_C_BITMASK, 
									PSR_N_BITMASK, PSR_N_BITMASK, 
									PSR_V_BITMASK, PSR_V_BITMASK};

	static const u32 estimate_size = 64 * 1024;
	u8 *ptr = AllocCodeBuffer(estimate_size);
	if (!ptr)
	{
		INFO("JIT: cache full, reset cpu.\n");

		arm_ljit.Reset();

		ptr = AllocCodeBuffer(estimate_size);
		if (!ptr)
		{
			INFO("JIT: alloc code buffer failed, size : %u.\n", estimate_size);
			return;
		}
	}

	uintptr_t opfun = (uintptr_t)jit_set_ip(ptr).ptr;

	s_pRegisterMap->Start(NULL, GETCPUPTR);

	Decoded *Instructions = blockinfo.Instructions;
	s32 InstructionsNum = blockinfo.InstructionsNum;

	u32 Address = Instructions[0].Address;

	u32 CurSubBlock = INVALID_SUBBLOCK;
	u32 CurInstructions = 0;
	u32 ConstCycles = 0;
	bool IsSubBlockStart = false;

	u32 StateSubBlockStart = INVALID_STATE_ID;//before subblock

	jit_insn* PatchSubBlockEnd2 = NULL;//not exec subblock

#if 0
	{
		std::vector<ABIOp> args;
		std::vector<RegisterMap::GuestRegId> flushs;

		ABIOp op;
		
		op.type = ABIOp::IMM;
		op.immdata.type = ImmData::IMM32;
		op.immdata.imm32 = PROCNUM;
		args.push_back(op);

		op.type = ABIOp::IMM;
		op.immdata.type = ImmData::IMM32;
		op.immdata.imm32 = Address;
		args.push_back(op);

		s_pRegisterMap->CallABI((void*)&PrintBlock::Method, args, flushs);
	}
#endif

	for (s32 i = 0; i < InstructionsNum; i++)
	{
		Decoded &Inst = Instructions[i];

		if (CurSubBlock != Inst.SubBlock)
		{
			if (ConstCycles > 0)
			{
				AddExecuteCycles(ConstCycles);

				ConstCycles = 0;
			}

			if (IsSubBlockStart)
			{
				jit_insn* PatchSubBlockEnd1 = PrepareSLZone();//exec subblock

				u32 state_end1 = s_pRegisterMap->StoreState();//exec subblock

				s_pRegisterMap->RestoreState(StateSubBlockStart);

				jit_patch(PatchSubBlockEnd2);
				AddExecuteCycles(CurInstructions);

				u32 state_end2 = s_pRegisterMap->StoreState();//not exec subblock

				EndSubBlock(StateSubBlockStart, state_end1, state_end2, PatchSubBlockEnd1);

				s_pRegisterMap->CleanState(StateSubBlockStart);
				s_pRegisterMap->CleanState(state_end1);
				s_pRegisterMap->CleanState(state_end2);

				IsSubBlockStart = false;

				PatchSubBlockEnd2 = NULL;
			}

			if (Inst.Cond != 0xE && Inst.Cond != 0xF)
			{
				u32 cpsr = s_pRegisterMap->MapReg(RegisterMap::CPSR);
				s_pRegisterMap->Lock(cpsr);
				u32 tmp = s_pRegisterMap->AllocTempReg();
				
				if (Inst.Cond <= 0x07)
				{
					jit_andi_ui(LOCALREG(tmp), LOCALREG(cpsr), CondSimple[Inst.Cond]);
					if (Inst.Cond & 1)
						PatchSubBlockEnd2 = jit_bnei_ui(jit_forward(), LOCALREG(tmp), 0);//flag clear
					else
						PatchSubBlockEnd2 = jit_beqi_ui(jit_forward(), LOCALREG(tmp), 0);//flag set
				}
				else
				{
					u32 table = s_pRegisterMap->AllocTempReg();

					jit_movi_p(LOCALREG(table), (void*)arm_cond_table);
					jit_rshi_ui(LOCALREG(tmp), LOCALREG(cpsr), 24);
					jit_andi_ui(LOCALREG(tmp), LOCALREG(tmp), 0xF0);
					jit_ori_ui(LOCALREG(tmp), LOCALREG(tmp), Inst.Cond);
					jit_ldxr_ui(LOCALREG(tmp), LOCALREG(table), LOCALREG(tmp));
					jit_andi_ui(LOCALREG(tmp), LOCALREG(tmp), Inst.ThumbFlag ? (1 << 0) : (1 << (CODE(Inst.Instruction.ArmOp))));
					PatchSubBlockEnd2 = jit_beqi_ui(jit_forward(), LOCALREG(tmp), 0);

					s_pRegisterMap->ReleaseTempReg(table);
				}
				
				s_pRegisterMap->ReleaseTempReg(tmp);
				s_pRegisterMap->Unlock(cpsr);

				StateSubBlockStart = s_pRegisterMap->StoreState();

				IsSubBlockStart = true;
			}

			CurInstructions = 0;

			CurSubBlock = Inst.SubBlock;
		}

		CurInstructions++;

		s_pRegisterMap->SetImm32(RegisterMap::R15, Inst.CalcR15(Inst) & Inst.ReadPCMask);
		
		//if ((Inst.IROp >= IR_CMN && Inst.IROp <= IR_CMN))
		//{
		//	u32 cpuptr = s_pRegisterMap->GetCpuPtrReg();
		//	u32 tmp = s_pRegisterMap->AllocTempReg();

		//	jit_movi_ui(LOCALREG(tmp), Inst.Address);
		//	jit_stxi_ui(jit_field(armcpu_t, instruct_adr), LOCALREG(cpuptr), LOCALREG(tmp));

		//	jit_movi_ui(LOCALREG(tmp), Inst.CalcNextInstruction(Inst));
		//	jit_stxi_ui(jit_field(armcpu_t, next_instruction), LOCALREG(cpuptr), LOCALREG(tmp));

		//	s_pRegisterMap->ReleaseTempReg(tmp);

		//	s_pRegisterMap->FlushAll(true);
		//	Fallback2Interpreter(Inst, *s_pRegisterMap);

		//	if (Inst.IROp == IR_SWI && Inst.MayHalt)
		//	{
		//		u32 cpuptr = s_pRegisterMap->GetCpuPtrReg();
		//		u32 tmp = s_pRegisterMap->AllocTempReg();

		//		jit_ldxi_ui(LOCALREG(tmp), LOCALREG(cpuptr), jit_field(armcpu_t, next_instruction));
		//		jit_stxi_ui(jit_field(armcpu_t, instruct_adr), LOCALREG(cpuptr), LOCALREG(tmp));

		//		s_pRegisterMap->ReleaseTempReg(tmp);
		//	}

		//	if (Inst.R15Modified)
		//		R15ModifiedGenerate(Inst, *s_pRegisterMap);
		//}
		//else
		{
			//s_pRegisterMap->FlushAll(true);
			iropdecoder_set[Inst.IROp](Inst, *s_pRegisterMap);

			if (!Inst.VariableCycles)
				ConstCycles += Inst.ExecuteCycles;
		}

		if (Inst.R15Modified || (Inst.IROp == IR_SWI && Inst.MayHalt))
		{
			if (ConstCycles > 0)
			{
				AddExecuteCycles(ConstCycles);

				ConstCycles = 0;
			}

			s_pRegisterMap->End(false);
		}
	}

	if (ConstCycles > 0)
	{
		AddExecuteCycles(ConstCycles);

		ConstCycles = 0;
	}

	Decoded &LastIns = Instructions[InstructionsNum - 1];
	if (IsSubBlockStart)
	{
		jit_insn* PatchSubBlockEnd1 = PrepareSLZone();//exec subblock

		u32 state_end1 = s_pRegisterMap->StoreState();//exec subblock

		s_pRegisterMap->RestoreState(StateSubBlockStart);

		jit_patch(PatchSubBlockEnd2);
		AddExecuteCycles(CurInstructions);

		u32 state_end2 = s_pRegisterMap->StoreState();//not exec subblock

		EndSubBlock(StateSubBlockStart, state_end1, state_end2, PatchSubBlockEnd1);

		s_pRegisterMap->CleanState(StateSubBlockStart);
		s_pRegisterMap->CleanState(state_end1);
		s_pRegisterMap->CleanState(state_end2);

		IsSubBlockStart = false;
	}

	u32 cpuptr = s_pRegisterMap->GetCpuPtrReg();
	u32 tmp = s_pRegisterMap->AllocTempReg();

	jit_movi_ui(LOCALREG(tmp), LastIns.CalcNextInstruction(LastIns));
	jit_stxi_ui(jit_field(armcpu_t, instruct_adr), LOCALREG(cpuptr), LOCALREG(tmp));

	s_pRegisterMap->ReleaseTempReg(tmp);

	s_pRegisterMap->End(true);

	//{
	//	INFO("Block Address : 0x%x, InstructionsNum : %u\n", Address, InstructionsNum);
	//	s_pRegisterMap->PrintProfile();
	//	INFO("\n");
	//}

	JITLUT_HANDLE(Address, PROCNUM) = opfun;

	u8* ptr_end = (u8*)jit_get_ip().ptr;
	u32 used_size = (u8*)ptr_end - (u8*)ptr;

	if (used_size > estimate_size)
		INFO("JIT: estimate_size[%u] is too small, used_size[%u].\n", estimate_size, used_size);
	else
		FreeCodeBuffer(estimate_size - used_size);

	FlushIcacheSection((u8*)ptr, *(u8*)ptr_end);

	return;
}

TEMPLATE static ArmOpCompiled armcpu_compile()
{
	u32 adr = ARMPROC.instruct_adr;

	if (!JITLUT_MAPPED(adr & 0x0FFFFFFF, PROCNUM))
	{
		INFO("JIT: use unmapped memory address %08X\n", adr);
		execute = false;
		return NULL;
	}

	if (!s_pArmAnalyze->Decode(GETCPUPTR) || !s_pArmAnalyze->CreateBlocks())
	{
		INFO("JIT: unknow error cpu[%d].\n", PROCNUM);
		return NULL;
	}

	BlockInfo *BlockInfos;
	s32 BlockInfoNum;

	s_pArmAnalyze->GetBlocks(BlockInfos, BlockInfoNum);
	for (s32 BlockNum = 0; BlockNum < BlockInfoNum; BlockNum++)
	{
		armcpu_compileblock<PROCNUM>(BlockInfos[BlockNum], BlockNum == 0);
	}

	return (ArmOpCompiled)JITLUT_HANDLE(adr, PROCNUM);
}

static void cpuReserve()
{
	static bool is_inited = false;
	if (!is_inited)
	{
		is_inited = true;
		jit_get_cpu();
	}
	memset(&_jit,0,sizeof(jit_state_t));

	u32 HostRegCount = LOCALREG_INIT();
	InitializeCodeBuffer();

	s_pArmAnalyze = new ArmAnalyze(CommonSettings.jit_max_block_size);
	s_pRegisterMap = new RegisterMapImp(HostRegCount);

	s_pArmAnalyze->m_MergeSubBlocks = true;
	s_pArmAnalyze->m_OptimizeFlag = true;
	//s_pArmAnalyze->m_JumpEndDecode = true;
}

static void cpuShutdown()
{
	ReleaseCodeBuffer();

	JitLutReset();

	delete s_pArmAnalyze;
	s_pArmAnalyze = NULL;

	delete s_pRegisterMap;
	s_pRegisterMap = NULL;
}

static void cpuReset()
{
	ResetCodeBuffer();

	JitLutReset();
}

static void cpuSync()
{
	armcpu_sync();
}

TEMPLATE static void cpuClear(u32 Addr, u32 Size)
{
	if (Addr == 0 && Size == CPUBASE_FLUSHALL)
	{
		JitLutReset();
	}
	else
	{
		Size /= 2;
		for (u32 i = 0; i < Size; i++)
		{
			const u32 adr = Addr + i*2;

			if (JITLUT_MAPPED(adr, PROCNUM))
				JITLUT_HANDLE(adr, PROCNUM) = (uintptr_t)NULL;
		}
	}
}

#define PAGESIZE 4096

// Comment out the following line to test Riley's attempts at JIT.
#define USE_TEST_JIT

#ifdef USE_TEST_JIT

typedef int (*inc_t)(int temp);
inc_t _inc = NULL;
uint32_t *p;

TEMPLATE static u32 cpuExecuteLJIT()
{
	/*ArmOpCompiled opfun = (ArmOpCompiled)JITLUT_HANDLE(ARMPROC.instruct_adr, PROCNUM);
     if (!opfun)
     opfun = armcpu_compile<PROCNUM>();*/
    
    uint32_t code[] = {
		0xe2800001, // add	r0, r0, #1
		0xe12fff1e, // bx	lr
	};
    
    printf("Before Execution\n");
    
    if (!p)
    {
        p = (uint32_t *)malloc(1024+PAGESIZE-1);
    }
    else
    {
        if (mprotect(p, 1024, PROT_READ | PROT_WRITE)) {
            perror("Couldn't mprotect");
            exit(errno);
        }
    }
    if (!p) {
        perror("Couldn't malloc(1024)");
        exit(errno);
    }
    
    printf("Malloced\n");
    
    p = (uint32_t *)(((uintptr_t)p + PAGESIZE-1) & ~(PAGESIZE-1));
    
    printf("Before Compiling\n");
    
    // copy instructions to function
	p[0] = code[0];
	p[1] = code[1];
    
    printf("After Compiling\n");
    
    if (mprotect(p, 1024, PROT_READ | PROT_EXEC)) {
        perror("Couldn't mprotect");
        exit(errno);
    }
    
    printf("About to JIT\n");
    
    int a = 1;
    inc_t opfun = (inc_t)p;
        
	return opfun(a);
}

#else

typedef int (*inc_t)(int temp);
inc_t _inc = NULL;
uint32_t *p;

TEMPLATE static u32 cpuExecuteLJIT()
{
    
    printf("Before Execution\n");
    
    if (!p)
    {
        p = (uint32_t *)malloc(1024+PAGESIZE-1);
    }
    else
    {
        if (mprotect(p, 1024, PROT_READ | PROT_WRITE)) {
            perror("Couldn't mprotect");
            exit(errno);
        }
    }
    if (!p) {
        perror("Couldn't malloc(1024)");
        exit(errno);
    }
    
    printf("Malloced\n");
    
    p = (uint32_t *)(((uintptr_t)p + PAGESIZE-1) & ~(PAGESIZE-1));
    
    printf("Before Compiling\n");
    
	*p = (uint32_t)JITLUT_HANDLE(ARMPROC.instruct_adr, PROCNUM);
    if (!*p) {
        
        *p = (uint32_t)armcpu_compile<PROCNUM>();
    }
    
    printf("After Compiling\n");
    
    if (mprotect(p, 1024, PROT_READ | PROT_EXEC)) {
        perror("Couldn't mprotect");
        exit(errno);
    }
    
    printf("About to JIT\n");
    
    ArmOpCompiled opfun = (ArmOpCompiled)p;
    
	return opfun();
}

#endif

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
	return "Arm LJit";
}

CpuBase arm_ljit =
{
	cpuReserve,

	cpuShutdown,

	cpuReset,

	cpuSync,

	cpuClear<0>, cpuClear<1>,

	cpuExecuteLJIT<0>, cpuExecuteLJIT<1>,

	cpuGetCacheReserve,
	cpuSetCacheReserve,

	cpuDescription
};
#endif
