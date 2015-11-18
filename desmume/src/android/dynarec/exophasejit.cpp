#include "exophasejit.h"
#include "cpu.h"

#include "armcpu.h"
#include "JitCommon.h"
#include "debug.h"

#define TEMPLATE template<u32 PROCNUM>

extern u32* spsr_pointer;
extern u32 dynarec_proc;
extern armcpu_t* dynarec_cpu;

u32 total_cycle_arm7 = 0;
u32 total_cycle_arm9 = 0;

FORCEINLINE void switch_arm7()
{
	dynarec_proc = 1;
	dynarec_cpu = &NDS_ARM7;
	spsr_pointer = &(NDS_ARM7.SPSR.val);
}

FORCEINLINE void switch_arm9()
{
	dynarec_proc = 0;
	dynarec_cpu = &NDS_ARM9;
	spsr_pointer = &(NDS_ARM9.SPSR.val);
}

extern "C" u32 dynarec_exec();

static void cpuReserve()
{
}

static void cpuShutdown()
{
	JitLutReset();
}

static void cpuReset()
{
	JitLutReset();
	
	dynarec_cpu = &NDS_ARM9;
	dynarec_cpu->R[CHANGED_PC_STATUS] =1;
	spsr_pointer = &(dynarec_cpu->SPSR.val);

	dynarec_cpu = &NDS_ARM7;
	dynarec_cpu->R[CHANGED_PC_STATUS] =1;
	spsr_pointer = &(dynarec_cpu->SPSR.val);

	flush_translation_cache_rom();
	flush_translation_cache_ram();
	flush_translation_cache_bios();
}

static void cpuSync()
{
	NDS_ARM7.instruct_adr = NDS_ARM7.R[15];
	NDS_ARM9.instruct_adr = NDS_ARM9.R[15];
}

TEMPLATE static void cpuClear(u32 Addr, u32 Size)
{
}

TEMPLATE static u32 cpuExecute()
{
	if (PROCNUM==ARMCPU_ARM9)
		switch_arm9();
	else
		switch_arm7();

//	if (PROCNUM==0)
//		LOGE("dynarec_cpu(%d)->PC : %x %x %x\n", PROCNUM, dynarec_cpu->R[15], dynarec_cpu->R[23], dynarec_cpu->R[31]);

	u32 c = dynarec_exec();

//	if (PROCNUM==ARMCPU_ARM9)
//		total_cycle_arm9 += c;
//	else
//		total_cycle_arm7 += c << 1;

//	INFO("%d\n",c);

	return c;
}

static u32 cpuGetCacheReserve()
{
	return 64 * 1024;
}

static void cpuSetCacheReserve(u32 reserveInMegs)
{
}

static const char* cpuDescription()
{
	return "Arm Exophase Jit";
}

CpuBase arm_exophasejit =
{
	cpuReserve,

	cpuShutdown,

	cpuReset,

	cpuSync,

	cpuClear<0>, cpuClear<1>,

	cpuExecute<0>, cpuExecute<1>,

	cpuGetCacheReserve,
	cpuSetCacheReserve,

	cpuDescription
};
