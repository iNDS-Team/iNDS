/* gameplaySP
 *
 * Copyright (C) 2006 Exophase <exophase@gmail.com>
 * Copyright (C) 2012 Qingping He
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
 */

// Not-so-important todo:
// - stm reglist writeback when base is in the list needs adjustment
// - block memory needs psr swapping and user mode reg swapping
#include "dynarec/cpu.h"
#include "warm.h"
#include <android/log.h>

#define PC_BUILD

using namespace Dynarec;

extern "C" {

#ifdef USE_PROFILER
#pragma GCC optimize ("O2", "no-stack-protector", "unroll-loops", "unswitch-loops", "no-profile-generate")
#else
#pragma GCC optimize ("O2", "no-stack-protector", "omit-frame-pointer", "unroll-loops", "unswitch-loops", "no-profile-generate")
#endif


#define address8(base, offset)                                                \
  *((u8 *)((u8 *)base + (offset)))                                            \

#define address16(base, offset)                                               \
  *((u16 *)((u8 *)base + (offset)))                                           \

#define address32(base, offset)                                               \
  *((u32 *)((u8 *)base + (offset)))                                           \

u8 rom_translation_cache[ROM_TRANSLATION_CACHE_SIZE];
u8 *rom_translation_ptr = rom_translation_cache;

//u8 ram_translation_cache[RAM_TRANSLATION_CACHE_SIZE];
//u8 *ram_translation_ptr = ram_translation_cache;
u32 iwram_code_min = 0xFFFFFFFF;
u32 iwram_code_max = 0xFFFFFFFF;
u32 ewram_code_min = 0xFFFFFFFF;
u32 ewram_code_max = 0xFFFFFFFF;

u8 bios_translation_cache[BIOS_TRANSLATION_CACHE_SIZE];
u8 *bios_translation_ptr = bios_translation_cache;
u8 bios_rom[1024 * 32];

u32 *rom_branch_hash[ROM_BRANCH_HASH_SIZE];

// Default
u32 idle_loop_target_pc = 0xFFFFFFFF;
u32 force_pc_update_target = 0xFFFFFFFF;
u32 translation_gate_target_pc[MAX_TRANSLATION_GATES];
u32 translation_gate_targets = 0;
u32 iwram_stack_optimize = 1;
u32 allow_smc_ram_u8 = 1;
u32 allow_smc_ram_u16 = 1;
u32 allow_smc_ram_u32 = 1;

typedef struct {
	u8 *block_offset;
	u16 flag_data;
	u8 condition;
	u8 update_cycles;
} block_data_type;

typedef struct {
	u32 branch_target;
	u8 *branch_source;
} block_exit_type;

const u8 bit_count[256] = { 0, 1, 1, 2, 1, 2, 2, 3, 1, 2, 2, 3, 2, 3, 3, 4, 1,
		2, 2, 3, 2, 3, 3, 4, 2, 3, 3, 4, 3, 4, 4, 5, 1, 2, 2, 3, 2, 3, 3, 4, 2,
		3, 3, 4, 3, 4, 4, 5, 2, 3, 3, 4, 3, 4, 4, 5, 3, 4, 4, 5, 4, 5, 5, 6, 1,
		2, 2, 3, 2, 3, 3, 4, 2, 3, 3, 4, 3, 4, 4, 5, 2, 3, 3, 4, 3, 4, 4, 5, 3,
		4, 4, 5, 4, 5, 5, 6, 2, 3, 3, 4, 3, 4, 4, 5, 3, 4, 4, 5, 4, 5, 5, 6, 3,
		4, 4, 5, 4, 5, 5, 6, 4, 5, 5, 6, 5, 6, 6, 7, 1, 2, 2, 3, 2, 3, 3, 4, 2,
		3, 3, 4, 3, 4, 4, 5, 2, 3, 3, 4, 3, 4, 4, 5, 3, 4, 4, 5, 4, 5, 5, 6, 2,
		3, 3, 4, 3, 4, 4, 5, 3, 4, 4, 5, 4, 5, 5, 6, 3, 4, 4, 5, 4, 5, 5, 6, 4,
		5, 5, 6, 5, 6, 6, 7, 2, 3, 3, 4, 3, 4, 4, 5, 3, 4, 4, 5, 4, 5, 5, 6, 3,
		4, 4, 5, 4, 5, 5, 6, 4, 5, 5, 6, 5, 6, 6, 7, 3, 4, 4, 5, 4, 5, 5, 6, 4,
		5, 5, 6, 5, 6, 6, 7, 4, 5, 5, 6, 5, 6, 6, 7, 5, 6, 6, 7, 6, 7, 7, 8 };

#define arm_decode_data_proc_reg()                                            \
  u32 rn = (opcode >> 16) & 0x0F;                                             \
  u32 rd = (opcode >> 12) & 0x0F;                                             \
  u32 rm = opcode & 0x0F                                                      \

#define arm_decode_data_proc_imm()                                            \
  u32 rn = (opcode >> 16) & 0x0F;                                             \
  u32 rd = (opcode >> 12) & 0x0F;                                             \
  u32 imm = opcode & 0xFF;                                                    \
  u32 imm_ror = ((opcode >> 8) & 0x0F) * 2                                    \

#define arm_decode_psr_reg()                                                  \
  u32 psr_field = (opcode >> 16) & 0x0F;                                      \
  u32 rd = (opcode >> 12) & 0x0F;                                             \
  u32 rm = opcode & 0x0F                                                      \

#define arm_decode_psr_imm()                                                  \
  u32 psr_field = (opcode >> 16) & 0x0F;                                      \
  u32 rd = (opcode >> 12) & 0x0F;                                             \
  u32 imm = opcode & 0xFF;                                                    \
  u32 imm_ror = ((opcode >> 8) & 0x0F) * 2                                    \

#define arm_decode_branchx()                                                  \
  u32 rn = opcode & 0x0F                                                      \

#define arm_decode_multiply()                                                 \
  u32 rd = (opcode >> 16) & 0x0F;                                             \
  u32 rn = (opcode >> 12) & 0x0F;                                             \
  u32 rs = (opcode >> 8) & 0x0F;                                              \
  u32 rm = opcode & 0x0F                                                      \

#define arm_decode_q_mla() \
		  u32 rd = (opcode >> 16) & 0x0F;                                             \
		  u32 rn = (opcode >> 12) & 0x0F;                                             \
		  u32 rs = (opcode >> 8) & 0x0F;                                              \
		  u32 rm = opcode & 0x0F;                                                      \
		  u32 clean_op = opcode & 0x0FF000F0\

#define arm_decode_q_mul() \
		  u32 rd = (opcode >> 16) & 0x0F;                                             \
		  u32 rs = (opcode >> 8) & 0x0F;                                              \
		  u32 rm = opcode & 0x0F;                                                      \
		  u32 clean_op = opcode & 0x0FF000F0\

#define arm_decode_q_op()  \
		  u32 rn = (opcode >> 16) & 0x0F;                                             \
		  u32 rd = (opcode >> 12) & 0x0F;                                             \
		  u32 rm = opcode & 0x0F                                                      \

#define arm_decode_clz()\
  u32 rm = opcode & 0x0F;\
  u32 rd = (opcode >> 12) & 0x0F\

#define arm_decode_multiply_long()                                            \
  u32 rdhi = (opcode >> 16) & 0x0F;                                           \
  u32 rdlo = (opcode >> 12) & 0x0F;                                           \
  u32 rs = (opcode >> 8) & 0x0F;                                              \
  u32 rm = opcode & 0x0F                                                      \

#define arm_decode_swap()                                                     \
  u32 rn = (opcode >> 16) & 0x0F;                                             \
  u32 rd = (opcode >> 12) & 0x0F;                                             \
  u32 rm = opcode & 0x0F                                                      \

#define arm_decode_half_trans_r()                                             \
  u32 rn = (opcode >> 16) & 0x0F;                                             \
  u32 rd = (opcode >> 12) & 0x0F;                                             \
  u32 rm = opcode & 0x0F                                                      \

#define arm_decode_half_trans_of()                                            \
  u32 rn = (opcode >> 16) & 0x0F;                                             \
  u32 rd = (opcode >> 12) & 0x0F;                                             \
  u32 offset = ((opcode >> 4) & 0xF0) | (opcode & 0x0F)                       \

#define arm_decode_data_trans_imm()                                           \
  u32 rn = (opcode >> 16) & 0x0F;                                             \
  u32 rd = (opcode >> 12) & 0x0F;                                             \
  u32 offset = opcode & 0x0FFF                                                \

#define arm_decode_data_trans_reg()                                           \
  u32 rn = (opcode >> 16) & 0x0F;                                             \
  u32 rd = (opcode >> 12) & 0x0F;                                             \
  u32 rm = opcode & 0x0F                                                      \

#define arm_decode_block_trans()                                              \
  u32 rn = (opcode >> 16) & 0x0F;                                             \
  u32 reg_list = opcode & 0xFFFF                                              \

#define arm_decode_branch()                                                   \
  s32 offset = ((s32)(opcode & 0xFFFFFF) << 8) >> 6                           \

#define thumb_decode_shift()                                                  \
  u32 imm = (opcode >> 6) & 0x1F;                                             \
  u32 rs = (opcode >> 3) & 0x07;                                              \
  u32 rd = opcode & 0x07                                                      \

#define thumb_decode_add_sub()                                                \
  u32 rn = (opcode >> 6) & 0x07;                                              \
  u32 rs = (opcode >> 3) & 0x07;                                              \
  u32 rd = opcode & 0x07                                                      \

#define thumb_decode_add_sub_imm()                                            \
  u32 imm = (opcode >> 6) & 0x07;                                             \
  u32 rs = (opcode >> 3) & 0x07;                                              \
  u32 rd = opcode & 0x07                                                      \

#define thumb_decode_imm()                                                    \
  u32 imm = opcode & 0xFF                                                     \

#define thumb_decode_alu_op()                                                 \
  u32 rs = (opcode >> 3) & 0x07;                                              \
  u32 rd = opcode & 0x07                                                      \

#define thumb_decode_hireg_op()                                               \
  u32 rs = (opcode >> 3) & 0x0F;                                              \
  u32 rd = ((opcode >> 4) & 0x08) | (opcode & 0x07)                           \

#define thumb_decode_mem_reg()                                                \
  u32 ro = (opcode >> 6) & 0x07;                                              \
  u32 rb = (opcode >> 3) & 0x07;                                              \
  u32 rd = opcode & 0x07                                                      \

#define thumb_decode_mem_imm()                                                \
  u32 imm = (opcode >> 6) & 0x1F;                                             \
  u32 rb = (opcode >> 3) & 0x07;                                              \
  u32 rd = opcode & 0x07                                                      \

#define thumb_decode_add_sp()                                                 \
  u32 imm = opcode & 0x7F                                                     \

#define thumb_decode_rlist()                                                  \
  u32 reg_list = opcode & 0xFF                                                \

#define thumb_decode_branch_cond()                                            \
  s32 offset = (s8)(opcode & 0xFF)                                            \

#define thumb_decode_swi()                                                    \
  u32 comment = opcode & 0xFF                                                 \

#define thumb_decode_branch()                                                 \
  u32 offset = opcode & 0x07FF                                                \

#include "arm_emit.h"

#define check_pc_region(pc)                                                   \

#define translate_arm_instruction()                                           \
  check_pc_region(pc);                                                        \
	opcode = _MMU_read32(dynarec_proc, MMU_AT_CODE, pc);                        \
	/*if(opcode & 0x0E1000F0 == 0x000000D0)*/\
	/*{*/\
	/*if(block_print)LOGE("instruction before trans %x pc %x", opcode, pc);*/\
	\
  condition = block_data[block_data_position].condition;                      \
  \
  if((condition != last_condition) || (condition >= 0x20))                    \
    {                                                                           \
      if((last_condition & 0x0F) != 0x0E && (last_condition & 0x0F) != 0x0F)                                       \
      {                                                                         \
        generate_branch_patch_conditional_arm(backpatch_address, translation_ptr);  \
      }                                                                         \
                                                                                \
      last_condition = condition;                                               \
                                                                                \
      condition &= 0x0F;                                                        \
                                                                                \
      if(condition != 0x0E && condition != 0x0F)                                                     \
      {                                                                         \
        arm_conditional_block_header();                                         \
        cond_pc = pc;   \
      }                                                                         \
    }                                                                           \
    \
/*generate_step_debug(); */\
                                                                              \
  switch((opcode >> 20) & 0xFF)                                               \
  {                                                                           \
    case 0x00:                                                                \
      if((opcode & 0x90) == 0x90)                                             \
      {                                                                       \
        if(opcode & 0x20)                                                     \
        {                                                                     \
          /* STRH rd, [rn], -rm */                                            \
          switch((opcode & 0xF0))                                          \
		  {                                                                     \
		    case 0xD0:                                                             \
			  /* LDRD rd, [rn + imm] */                                         \
			  arm_access_memory(loadd, down, post, u16, half_reg);              \
			  break;                                                            \
																			  \
		    case 0xF0:                                                             \
			  /* STRD rd, [rn + imm] */                                        \
			  arm_access_memory(stored, down, post, u16, half_reg);                  \
			  break;                                                            \
																			  \
		    case 0xB0:                                                             \
			  /* STRH rd, [rn + imm] */                                        \
			  arm_access_memory(store, down, post, u16, half_reg);                \
			  break;                                                            \
		  }                                                                     \
        }                                                                     \
        else                                                                  \
        {                                                                     \
          /* MUL rd, rm, rs */                                                \
          arm_multiply(no, no);                                               \
        }                                                                     \
      }                                                                       \
      else                                                                    \
      {                                                                       \
        /* AND rd, rn, reg_op */                                              \
        arm_data_proc(and, reg, no_flags);                                    \
      }                                                                       \
      break;                                                                  \
                                                                              \
    case 0x01:                                                                \
      if((opcode & 0x90) == 0x90)                                             \
      {                                                                       \
        switch((opcode >> 5) & 0x03)                                          \
        {                                                                     \
          case 0:                                                             \
            /* MULS rd, rm, rs */                                             \
            arm_multiply(no, yes);                                            \
            break;                                                            \
                                                                              \
          case 1:                                                             \
            /* LDRH rd, [rn], -rm */                                          \
            arm_access_memory(load, down, post, u16, half_reg);               \
            break;                                                            \
                                                                              \
          case 2:                                                             \
            /* LDRSB rd, [rn], -rm */                                         \
            arm_access_memory(load, down, post, s8, half_reg);                \
            break;                                                            \
                                                                              \
          case 3:                                                             \
            /* LDRSH rd, [rn], -rm */                                         \
            arm_access_memory(load, down, post, s16, half_reg);               \
            break;                                                            \
        }                                                                     \
      }                                                                       \
      else                                                                    \
      {                                                                       \
        /* ANDS rd, rn, reg_op */                                             \
        arm_data_proc(ands, reg_flags, flags);                                \
      }                                                                       \
      break;                                                                  \
                                                                              \
    case 0x02:                                                                \
      if((opcode & 0x90) == 0x90)                                             \
      {                                                                       \
        if(opcode & 0x20)                                                     \
        {                                                                     \
          /* STRH rd, [rn], -rm */                                            \
		switch((opcode & 0xF0))                                          \
		  {                                                                     \
			case 0xD0:                                                             \
			  /* LDRD rd, [rn + imm] */                                         \
			  arm_access_memory(loadd, down, post, u16, half_reg);              \
			  break;                                                            \
																			  \
			case 0xF0:                                                             \
			  /* STRD rd, [rn + imm] */                                        \
			  arm_access_memory(stored, down, post, u16, half_reg);                  \
			  break;                                                            \
																			  \
			case 0xB0:                                                             \
			  /* STRH rd, [rn + imm] */                                        \
			  arm_access_memory(store, down, post, u16, half_reg);                \
			  break;                                                            \
		  }                                                                     \
        }                                                                     \
        else                                                                  \
        {                                                                     \
          /* MLA rd, rm, rs, rn */                                            \
          arm_multiply(yes, no);                                              \
        }                                                                     \
      }                                                                       \
      else                                                                    \
      {                                                                       \
        /* EOR rd, rn, reg_op */                                              \
        arm_data_proc(eor, reg, no_flags);                                    \
      }                                                                       \
      break;                                                                  \
                                                                              \
    case 0x03:                                                                \
      if((opcode & 0x90) == 0x90)                                             \
      {                                                                       \
        switch((opcode >> 5) & 0x03)                                          \
        {                                                                     \
          case 0:                                                             \
            /* MLAS rd, rm, rs, rn */                                         \
            arm_multiply(yes, yes);                                           \
            break;                                                            \
                                                                              \
          case 1:                                                             \
            /* LDRH rd, [rn], -rm */                                          \
            arm_access_memory(load, down, post, u16, half_reg);               \
            break;                                                            \
                                                                              \
          case 2:                                                             \
            /* LDRSB rd, [rn], -rm */                                         \
            arm_access_memory(load, down, post, s8, half_reg);                \
            break;                                                            \
                                                                              \
          case 3:                                                             \
            /* LDRSH rd, [rn], -rm */                                         \
            arm_access_memory(load, down, post, s16, half_reg);               \
            break;                                                            \
        }                                                                     \
      }                                                                       \
      else                                                                    \
      {                                                                       \
        /* EORS rd, rn, reg_op */                                             \
        arm_data_proc(eors, reg_flags, flags);                                \
      }                                                                       \
      break;                                                                  \
                                                                              \
    case 0x04:                                                                \
      if((opcode & 0x90) == 0x90)                                             \
      {                                                                       \
        /* STRH rd, [rn], -imm */                                             \
        switch((opcode & 0xF0))                                          \
		  {                                                                     \
			case 0xD0:                                                             \
			  /* LDRD rd, [rn + imm] */                                         \
			  arm_access_memory(loadd, down, post, u16, half_imm);              \
			  break;                                                            \
																			  \
			case 0xF0:                                                             \
			  /* STRD rd, [rn + imm] */                                        \
			  arm_access_memory(stored, down, post, u16, half_imm);                  \
			  break;                                                            \
																			  \
			case 0xB0:                                                             \
			  /* STRH rd, [rn + imm] */                                        \
			  arm_access_memory(store, down, post, u16,  half_imm);                \
			  break;                                                            \
		  }                                                                     \
      }                                                                       \
      else                                                                    \
      {                                                                       \
        /* SUB rd, rn, reg_op */                                              \
        arm_data_proc(sub, reg, no_flags);                                    \
      }                                                                       \
      break;                                                                  \
                                                                              \
    case 0x05:                                                                \
      if((opcode & 0x90) == 0x90)                                             \
      {                                                                       \
        switch((opcode >> 5) & 0x03)                                          \
        {                                                                     \
          case 1:                                                             \
            /* LDRH rd, [rn], -imm */                                         \
            arm_access_memory(load, down, post, u16, half_imm);               \
            break;                                                            \
                                                                              \
          case 2:                                                             \
            /* LDRSB rd, [rn], -imm */                                        \
            arm_access_memory(load, down, post, s8, half_imm);                \
            break;                                                            \
                                                                              \
          case 3:                                                             \
            /* LDRSH rd, [rn], -imm */                                        \
            arm_access_memory(load, down, post, s16, half_imm);               \
            break;                                                            \
        }                                                                     \
      }                                                                       \
      else                                                                    \
      {                                                                       \
        /* SUBS rd, rn, reg_op */                                             \
        arm_data_proc(subs, reg, flags);                                      \
      }                                                                       \
      break;                                                                  \
                                                                              \
    case 0x06:                                                                \
      if((opcode & 0x90) == 0x90)                                             \
      {                                                                       \
        /* STRH rd, [rn], -imm */                                             \
		switch((opcode & 0xF0))                                          \
		  {                                                                     \
			case 0xD0:                                                             \
			  /* LDRD rd, [rn + imm] */                                         \
			  arm_access_memory(loadd, down, post, u16, half_imm);              \
			  break;                                                            \
																			  \
			case 0xF0:                                                             \
			  /* STRD rd, [rn + imm] */                                        \
			  arm_access_memory(stored, down, post, u16, half_imm);                  \
			  break;                                                            \
																			  \
			case 0xB0:                                                             \
			  /* STRH rd, [rn + imm] */                                        \
			  arm_access_memory(store, down, post, u16,  half_imm);                \
			  break;                                                            \
		  }                                                                     \
      }                                                                       \
      else                                                                    \
      {                                                                       \
        /* RSB rd, rn, reg_op */                                              \
        arm_data_proc(rsb, reg, no_flags);                                    \
      }                                                                       \
      break;                                                                  \
                                                                              \
    case 0x07:                                                                \
      if((opcode & 0x90) == 0x90)                                             \
      {                                                                       \
        switch((opcode >> 5) & 0x03)                                          \
        {                                                                     \
          case 1:                                                             \
            /* LDRH rd, [rn], -imm */                                         \
            arm_access_memory(load, down, post, u16, half_imm);               \
            break;                                                            \
                                                                              \
          case 2:                                                             \
            /* LDRSB rd, [rn], -imm */                                        \
            arm_access_memory(load, down, post, s8, half_imm);                \
            break;                                                            \
                                                                              \
          case 3:                                                             \
            /* LDRSH rd, [rn], -imm */                                        \
            arm_access_memory(load, down, post, s16, half_imm);               \
            break;                                                            \
        }                                                                     \
      }                                                                       \
      else                                                                    \
      {                                                                       \
        /* RSBS rd, rn, reg_op */                                             \
        arm_data_proc(rsbs, reg, flags);                                      \
      }                                                                       \
      break;                                                                  \
                                                                              \
    case 0x08:                                                                \
      if((opcode & 0x90) == 0x90)                                             \
      {                                                                       \
        if(opcode & 0x20)                                                     \
        {                                                                     \
          /* STRH rd, [rn], +rm */                                            \
          switch((opcode & 0xF0))                                          \
		  {                                                                     \
			case 0xD0:                                                             \
			  /* LDRD rd, [rn + imm] */                                         \
			  arm_access_memory(loadd, up, post, u16, half_reg);            \
			  break;                                                            \
																			  \
			case 0xF0:                                                             \
			  /* STRD rd, [rn + imm] */                                        \
			  arm_access_memory(stored, up, post, u16, half_reg);                 \
			  break;                                                            \
																			  \
			case 0xB0:                                                             \
			  /* STRH rd, [rn + imm] */                                        \
			  arm_access_memory(store,up, post, u16, half_reg);                \
			  break;                                                            \
		  }                                                                     \
        }                                                                     \
        else                                                                  \
        {                                                                     \
          /* UMULL rd, rm, rs */                                              \
          arm_multiply_long(u64, no, no);                                     \
        }                                                                     \
      }                                                                       \
      else                                                                    \
      {                                                                       \
        /* ADD rd, rn, reg_op */                                              \
        arm_data_proc(add, reg, no_flags);                                    \
      }                                                                       \
      break;                                                                  \
                                                                              \
    case 0x09:                                                                \
      if((opcode & 0x90) == 0x90)                                             \
      {                                                                       \
        switch((opcode >> 5) & 0x03)                                          \
        {                                                                     \
          case 0:                                                             \
            /* UMULLS rdlo, rdhi, rm, rs */                                   \
            arm_multiply_long(u64, no, yes);                                  \
            break;                                                            \
                                                                              \
          case 1:                                                             \
            /* LDRH rd, [rn], +rm */                                          \
            arm_access_memory(load, up, post, u16, half_reg);                 \
            break;                                                            \
                                                                              \
          case 2:                                                             \
            /* LDRSB rd, [rn], +rm */                                         \
            arm_access_memory(load, up, post, s8, half_reg);                  \
            break;                                                            \
                                                                              \
          case 3:                                                             \
            /* LDRSH rd, [rn], +rm */                                         \
            arm_access_memory(load, up, post, s16, half_reg);                 \
            break;                                                            \
        }                                                                     \
      }                                                                       \
      else                                                                    \
      {                                                                       \
        /* ADDS rd, rn, reg_op */                                             \
        arm_data_proc(adds, reg, flags);                                      \
      }                                                                       \
      break;                                                                  \
                                                                              \
    case 0x0A:                                                                \
      if((opcode & 0x90) == 0x90)                                             \
      {                                                                       \
        if(opcode & 0x20)                                                     \
        {                                                                     \
          /* STRH rd, [rn], +rm */                                            \
          switch((opcode & 0xF0))                                          \
		  {                                                                     \
			case 0xD0:                                                             \
			  /* LDRD rd, [rn + imm] */                                         \
			  arm_access_memory(loadd, up, post, u16, half_reg);            \
			  break;                                                            \
																			  \
			case 0xF0:                                                             \
			  /* STRD rd, [rn + imm] */                                        \
			  arm_access_memory(stored, up, post, u16, half_reg);                 \
			  break;                                                            \
																			  \
			case 0xB0:                                                             \
			  /* STRH rd, [rn + imm] */                                        \
			  arm_access_memory(store,up, post, u16, half_reg);                \
			  break;                                                            \
		  }                                                                     \
        }                                                                     \
        else                                                                  \
        {                                                                     \
          /* UMLAL rd, rm, rs */                                              \
          arm_multiply_long(u64_add, yes, no);                                \
        }                                                                     \
      }                                                                       \
      else                                                                    \
      {                                                                       \
        /* ADC rd, rn, reg_op */                                              \
        arm_data_proc(adc, reg, no_flags);                                    \
      }                                                                       \
      break;                                                                  \
                                                                              \
    case 0x0B:                                                                \
      if((opcode & 0x90) == 0x90)                                             \
      {                                                                       \
        switch((opcode >> 5) & 0x03)                                          \
        {                                                                     \
          case 0:                                                             \
            /* UMLALS rdlo, rdhi, rm, rs */                                   \
            arm_multiply_long(u64_add, yes, yes);                             \
            break;                                                            \
                                                                              \
          case 1:                                                             \
            /* LDRH rd, [rn], +rm */                                          \
            arm_access_memory(load, up, post, u16, half_reg);                 \
            break;                                                            \
                                                                              \
          case 2:                                                             \
            /* LDRSB rd, [rn], +rm */                                         \
            arm_access_memory(load, up, post, s8, half_reg);                  \
            break;                                                            \
                                                                              \
          case 3:                                                             \
            /* LDRSH rd, [rn], +rm */                                         \
            arm_access_memory(load, up, post, s16, half_reg);                 \
            break;                                                            \
        }                                                                     \
      }                                                                       \
      else                                                                    \
      {                                                                       \
        /* ADCS rd, rn, reg_op */                                             \
        arm_data_proc(adcs, reg, flags);                                      \
      }                                                                       \
      break;                                                                  \
                                                                              \
    case 0x0C:                                                                \
      if((opcode & 0x90) == 0x90)                                             \
      {                                                                       \
        if(opcode & 0x20)                                                     \
        {                                                                     \
          /* STRH rd, [rn], +imm */                                           \
          switch((opcode & 0xF0))                                          \
		  {                                                                     \
			case 0xD0:                                                             \
			  /* LDRD rd, [rn + imm] */                                         \
			  arm_access_memory(loadd, up, post, u16, half_imm);            \
			  break;                                                            \
																			  \
			case 0xF0:                                                             \
			  /* STRD rd, [rn + imm] */                                        \
			  arm_access_memory(stored, up, post, u16, half_imm);                 \
			  break;                                                            \
																			  \
			case 0xB0:                                                             \
			  /* STRH rd, [rn + imm] */                                        \
			  arm_access_memory(store,up, post, u16, half_imm);                \
			  break;                                                            \
		  }                                                                     \
        }                                                                     \
        else                                                                  \
        {                                                                     \
          /* SMULL rd, rm, rs */                                              \
          arm_multiply_long(s64, no, no);                                     \
        }                                                                     \
      }                                                                       \
      else                                                                    \
      {                                                                       \
        /* SBC rd, rn, reg_op */                                              \
        arm_data_proc(sbc, reg, no_flags);                                    \
      }                                                                       \
      break;                                                                  \
                                                                              \
    case 0x0D:                                                                \
      if((opcode & 0x90) == 0x90)                                             \
      {                                                                       \
        switch((opcode >> 5) & 0x03)                                          \
        {                                                                     \
          case 0:                                                             \
            /* SMULLS rdlo, rdhi, rm, rs */                                   \
            arm_multiply_long(s64, no, yes);                                  \
            break;                                                            \
                                                                              \
          case 1:                                                             \
            /* LDRH rd, [rn], +imm */                                         \
            arm_access_memory(load, up, post, u16, half_imm);                 \
            break;                                                            \
                                                                              \
          case 2:                                                             \
            /* LDRSB rd, [rn], +imm */                                        \
            arm_access_memory(load, up, post, s8, half_imm);                  \
            break;                                                            \
                                                                              \
          case 3:                                                             \
            /* LDRSH rd, [rn], +imm */                                        \
            arm_access_memory(load, up, post, s16, half_imm);                 \
            break;                                                            \
        }                                                                     \
      }                                                                       \
      else                                                                    \
      {                                                                       \
        /* SBCS rd, rn, reg_op */                                             \
        arm_data_proc(sbcs, reg, flags);                                      \
      }                                                                       \
      break;                                                                  \
                                                                              \
    case 0x0E:                                                                \
      if((opcode & 0x90) == 0x90)                                             \
      {                                                                       \
        if(opcode & 0x20)                                                     \
        {                                                                     \
          /* STRH rd, [rn], +imm */                                           \
          switch((opcode & 0xF0))                                          \
		  {                                                                     \
			case 0xD0:                                                             \
			  /* LDRD rd, [rn + imm] */                                         \
			  arm_access_memory(loadd, up, post, u16, half_imm);            \
			  break;                                                            \
																			  \
			case 0xF0:                                                             \
			  /* STRD rd, [rn + imm] */                                        \
			  arm_access_memory(stored, up, post, u16, half_imm);                 \
			  break;                                                            \
																			  \
			case 0xB0:                                                             \
			  /* STRH rd, [rn + imm] */                                        \
			  arm_access_memory(store,up, post, u16, half_imm);                \
			  break;                                                            \
		  }                                                                     \
        }                                                                     \
        else                                                                  \
        {                                                                     \
          /* SMLAL rd, rm, rs */                                              \
          arm_multiply_long(s64_add, yes, no);                                \
        }                                                                     \
      }                                                                       \
      else                                                                    \
      {                                                                       \
        /* RSC rd, rn, reg_op */                                              \
        arm_data_proc(rsc, reg, no_flags);                                    \
      }                                                                       \
      break;                                                                  \
                                                                              \
    case 0x0F:                                                                \
      if((opcode & 0x90) == 0x90)                                             \
      {                                                                       \
        switch((opcode >> 5) & 0x03)                                          \
        {                                                                     \
          case 0:                                                             \
            /* SMLALS rdlo, rdhi, rm, rs */                                   \
            arm_multiply_long(s64_add, yes, yes);                             \
            break;                                                            \
                                                                              \
          case 1:                                                             \
            /* LDRH rd, [rn], +imm */                                         \
            arm_access_memory(load, up, post, u16, half_imm);                 \
            break;                                                            \
                                                                              \
          case 2:                                                             \
            /* LDRSB rd, [rn], +imm */                                        \
            arm_access_memory(load, up, post, s8, half_imm);                  \
            break;                                                            \
                                                                              \
          case 3:                                                             \
            /* LDRSH rd, [rn], +imm */                                        \
            arm_access_memory(load, up, post, s16, half_imm);                 \
            break;                                                            \
        }                                                                     \
      }                                                                       \
      else                                                                    \
      {                                                                       \
        /* RSCS rd, rn, reg_op */                                             \
        arm_data_proc(rscs, reg, flags);                                      \
      }                                                                       \
      break;                                                                  \
                                                                              \
    case 0x10:                                                                \
      if((opcode & 0x90) == 0x90)                                             \
      {                                                                       \
                                                                              \
        if(opcode & 0x20)                                                     \
        {                                                                     \
          /* STRH rd, [rn - rm] */                                            \
          switch((opcode & 0xF0))                                          \
		  {                                                                     \
			case 0xD0:                                                             \
			  /* LDRD rd, [rn + imm] */                                         \
			  arm_access_memory(loadd, down, pre, u16, half_reg);            \
			  break;                                                            \
																			  \
			case 0xF0:                                                             \
			  /* STRD rd, [rn + imm] */                                        \
			  arm_access_memory(stored, down, pre, u16, half_reg);                 \
			  break;                                                            \
																			  \
			case 0xB0:                                                             \
			  /* STRH rd, [rn + imm] */                                        \
			  arm_access_memory(store, down, pre, u16, half_reg);               \
			  break;                                                            \
		  }                                                                     \
        }                                                                     \
        else                                                                  \
        {                                                                     \
          /* SWP rd, rm, [rn] */                                              \
          arm_swap(u32);                                                      \
        }                                                                     \
      }                                                                       \
      else                                                                    \
      {                                                                       \
    	  switch((opcode & 0xF0))                                          \
		  {                                                                     \
			case 0x50:                                                             \
			  arm_q_op(add);                                            \
			  break;                                                            \
																			  \
			case 0x00:                                                             \
			  arm_psr(reg, read, cpsr);                                             \
			  break;                                                            \
			  	  	  	  	  	  	  	  	  	  	  	  	  	  	  	  	  	  \
			default: \
				arm_q_mla();\
				break; \
		  }                                                                     \
      }                                                                       \
      break;                                                                  \
                                                                              \
    case 0x11:                                                                \
      if((opcode & 0x90) == 0x90)                                             \
      {                                                                       \
        switch((opcode >> 5) & 0x03)                                          \
        {                                                                     \
          case 1:                                                             \
            /* LDRH rd, [rn - rm] */                                          \
            arm_access_memory(load, down, pre, u16, half_reg);                \
            break;                                                            \
                                                                              \
          case 2:                                                             \
            /* LDRSB rd, [rn - rm] */                                         \
            arm_access_memory(load, down, pre, s8, half_reg);                 \
            break;                                                            \
                                                                              \
          case 3:                                                             \
            /* LDRSH rd, [rn - rm] */                                         \
            arm_access_memory(load, down, pre, s16, half_reg);                \
            break;                                                            \
        }                                                                     \
      }                                                                       \
      else                                                                    \
      {                                                                       \
        /* TST rd, rn, reg_op */                                              \
        arm_data_proc_test(tst, reg_flags);                                   \
      }                                                                       \
      break;                                                                  \
                                                                              \
    case 0x12:                                                                \
      if((opcode & 0x90) == 0x90)                                             \
      {                                                                       \
        /* STRH rd, [rn - rm]! */                                             \
        switch((opcode & 0xF0))                                          \
		  {                                                                     \
			case 0xD0:                                                             \
			  /* LDRD rd, [rn + imm] */                                         \
			  arm_access_memory(loadd, down, pre_wb, u16, half_reg);               \
			  break;                                                            \
																			  \
			case 0xF0:                                                             \
			  /* STRD rd, [rn + imm] */                                        \
			  arm_access_memory(stored,down, pre_wb, u16, half_reg);                    \
			  break;                                                            \
																			  \
			case 0xB0:                                                             \
			  /* STRH rd, [rn + imm] */                                        \
			  arm_access_memory(store, down, pre_wb, u16, half_reg);                 \
			  break;                                                            \
		  }                                                                     \
      }                                                                       \
      else                                                                    \
      {                                                                       \
        if(opcode & 0x10)                                                     \
        {                                                                     \
        	if((opcode & 0xF0) == 0x30)                                         \
        	{                                                                 \
        		arm_blx_reg();                                                \
        	}                                                                 \
        	else if((opcode & 0xF0) == 0x50)                                           \
	      	{                                                                     \
	      		arm_q_op(sub);                                                    \
	      	}                                                                     \
    	    else   if((opcode & 0xb0) == 0x80) \
    	    {  \
    	      	arm_q_mla();  \
    	    }  \
            else if((opcode & 0xb0) == 0xA0)\
            {  \
            	arm_q_mul();  \
            }  \
    	    else  \
        	{                                                                 \
        		/* BX rn */                                                   \
				arm_bx();                                                     \
        	}                                                                 \
        }                                                                     \
        else                                                                  \
        {                                                                     \
          /* MSR cpsr, rm */                                                  \
          arm_psr(reg, store, cpsr);                                          \
        }                                                                     \
      }                                                                       \
      break;                                                                  \
                                                                              \
    case 0x13:                                                                \
      if((opcode & 0x90) == 0x90)                                             \
      {                                                                       \
        switch((opcode >> 5) & 0x03)                                          \
        {                                                                     \
          case 1:                                                             \
            /* LDRH rd, [rn - rm]! */                                         \
            arm_access_memory(load, down, pre_wb, u16, half_reg);             \
            break;                                                            \
                                                                              \
          case 2:                                                             \
            /* LDRSB rd, [rn - rm]! */                                        \
            arm_access_memory(load, down, pre_wb, s8, half_reg);              \
            break;                                                            \
                                                                              \
          case 3:                                                             \
            /* LDRSH rd, [rn - rm]! */                                        \
            arm_access_memory(load, down, pre_wb, s16, half_reg);             \
            break;                                                            \
        }                                                                     \
      }                                                                       \
      else                                                                    \
      {                                                                       \
        /* TEQ rd, rn, reg_op */                                              \
        arm_data_proc_test(teq, reg_flags);                                   \
      }                                                                       \
      break;                                                                  \
                                                                              \
    case 0x14:                                                                \
      if((opcode & 0x90) == 0x90)                                             \
      {                                                                       \
        if(opcode & 0x20)                                                     \
        {                                                                     \
          /* STRH rd, [rn - imm] */                                           \
          switch((opcode & 0xF0))                                          \
		  {                                                                     \
			case 0xD0:                                                             \
			  /* LDRD rd, [rn + imm] */                                         \
			  arm_access_memory(loadd, down, pre, u16, half_imm);               \
			  break;                                                            \
																			  \
			case 0xF0:                                                             \
			  /* STRD rd, [rn + imm] */                                        \
			  arm_access_memory(stored,down, pre, u16, half_imm);                     \
			  break;                                                            \
																			  \
			case 0xB0:                                                             \
			  /* STRH rd, [rn + imm] */                                        \
			  arm_access_memory(store, down, pre, u16, half_imm);                 \
			  break;                                                            \
		  }                                                                     \
        }                                                                     \
        else                                                                  \
        {                                                                     \
          /* SWPB rd, rm, [rn] */                                             \
          arm_swap(u8);                                                       \
        }                                                                     \
      }                                                                       \
      else                                                                    \
      {                                                                       \
    	  if((opcode & 0xF0) == 0x50)                                           \
    	  {                                                                     \
    	  	arm_q_op(dadd);                                                    \
    	  }                                                                     \
    	  else   if((opcode & 0xF0) == 0x00) \
    	  {  \
    		  /* MRS rd, spsr */                                                    \
        	arm_psr(reg, read, spsr);                                             \
    	  }    \
    	  else  \
    	  {  \
    	     arm_q_mla();  \
    	  }   \
      }                                                                       \
      break;                                                                  \
                                                                              \
    case 0x15:                                                                \
      if((opcode & 0x90) == 0x90)                                             \
      {                                                                       \
        switch((opcode >> 5) & 0x03)                                          \
        {                                                                     \
          case 1:                                                             \
            /* LDRH rd, [rn - imm] */                                         \
            arm_access_memory(load, down, pre, u16, half_imm);                \
            break;                                                            \
                                                                              \
          case 2:                                                             \
            /* LDRSB rd, [rn - imm] */                                        \
            arm_access_memory(load, down, pre, s8, half_imm);                 \
            break;                                                            \
                                                                              \
          case 3:                                                             \
            /* LDRSH rd, [rn - imm] */                                        \
            arm_access_memory(load, down, pre, s16, half_imm);                \
            break;                                                            \
        }                                                                     \
      }                                                                       \
      else                                                                    \
      {                                                                       \
        /* CMP rn, reg_op */                                                  \
        arm_data_proc_test(cmp, reg);                                         \
      }                                                                       \
      break;                                                                  \
                                                                              \
    case 0x16:                                                                \
      if((opcode & 0x90) == 0x90)                                             \
      {                                                                       \
        /* STRH rd, [rn - imm]! */                                            \
        switch((opcode & 0xF0))                                          \
		  {                                                                     \
			case 0xD0:                                                             \
			  /* LDRD rd, [rn + imm] */                                         \
			  arm_access_memory(loadd, down, pre_wb, u16, half_imm);               \
			  break;                                                            \
																			  \
			case 0xF0:                                                             \
			  /* STRD rd, [rn + imm] */                                        \
			  arm_access_memory(stored,down, pre_wb, u16, half_imm);                     \
			  break;                                                            \
																			  \
			case 0xB0:                                                             \
			  /* STRH rd, [rn + imm] */                                        \
			  arm_access_memory(store, down, pre_wb, u16, half_imm);                 \
			  break;                                                            \
		  }                                                                     \
      }                                                                       \
      else                                                                    \
      {                                                                       \
        /* MSR spsr, rm */                                                    \
		switch((opcode & 0xF0))                                          \
		  {                                                                     \
			case 0x50:                                                             \
			  arm_q_op(dsub);                                                    \
			  break;                                                            \
																			  \
			case 0x10:                                                             \
			  arm_clz();  \
			  break;                                                            \
																			  \
			case 0x00:                                                             \
			  arm_psr(reg, store, spsr);                                            \
			  break;                                                            \
																				  \
			default:                                                             \
			  arm_q_mul();\
			  break;                                                            \
																			  \
		  }                                                                     \
      }                                                                       \
      break;                                                                  \
                                                                              \
    case 0x17:                                                                \
      if((opcode & 0x90) == 0x90)                                             \
      {                                                                       \
        switch((opcode >> 5) & 0x03)                                          \
        {                                                                     \
          case 1:                                                             \
            /* LDRH rd, [rn - imm]! */                                        \
            arm_access_memory(load, down, pre_wb, u16, half_imm);             \
            break;                                                            \
                                                                              \
          case 2:                                                             \
            /* LDRSB rd, [rn - imm]! */                                       \
            arm_access_memory(load, down, pre_wb, s8, half_imm);              \
            break;                                                            \
                                                                              \
          case 3:                                                             \
            /* LDRSH rd, [rn - imm]! */                                       \
            arm_access_memory(load, down, pre_wb, s16, half_imm);             \
            break;                                                            \
        }                                                                     \
      }                                                                       \
      else                                                                    \
      {                                                                       \
        /* CMN rd, rn, reg_op */                                              \
        arm_data_proc_test(cmn, reg);                                         \
      }                                                                       \
      break;                                                                  \
                                                                              \
    case 0x18:                                                                \
      if((opcode & 0x90) == 0x90)                                             \
      {                                                                       \
        /* STRH rd, [rn + rm] */                                              \
        switch((opcode & 0xF0))                                          \
		  {                                                                     \
			case 0xD0:                                                             \
			  /* LDRD rd, [rn + imm] */                                         \
			  arm_access_memory(loadd, up, pre, u16, half_reg);               \
			  break;                                                            \
																			  \
			case 0xF0:                                                             \
			  /* STRD rd, [rn + imm] */                                        \
			  arm_access_memory(stored,up, pre, u16, half_reg);                     \
			  break;                                                            \
																			  \
			case 0xB0:                                                             \
			  /* STRH rd, [rn + imm] */                                        \
			  arm_access_memory(store,up, pre, u16, half_reg);                  \
			  break;                                                            \
		  }                                                                     \
      }                                                                       \
      else                                                                    \
      {                                                                       \
        /* ORR rd, rn, reg_op */                                              \
        arm_data_proc(orr, reg, no_flags);                                    \
      }                                                                       \
      break;                                                                  \
                                                                              \
    case 0x19:                                                                \
      if((opcode & 0x90) == 0x90)                                             \
      {                                                                       \
        switch((opcode >> 5) & 0x03)                                          \
        {                                                                     \
          case 1:                                                             \
            /* LDRH rd, [rn + rm] */                                          \
            arm_access_memory(load, up, pre, u16, half_reg);                  \
            break;                                                            \
                                                                              \
          case 2:                                                             \
            /* LDRSB rd, [rn + rm] */                                         \
            arm_access_memory(load, up, pre, s8, half_reg);                   \
            break;                                                            \
                                                                              \
          case 3:                                                             \
            /* LDRSH rd, [rn + rm] */                                         \
            arm_access_memory(load, up, pre, s16, half_reg);                  \
            break;                                                            \
        }                                                                     \
      }                                                                       \
      else                                                                    \
      {                                                                       \
        /* ORRS rd, rn, reg_op */                                             \
        arm_data_proc(orrs, reg_flags, flags);                                \
      }                                                                       \
      break;                                                                  \
                                                                              \
    case 0x1A:                                                                \
      if((opcode & 0x90) == 0x90)                                             \
      {                                                                       \
        /* STRH rd, [rn + rm]! */                                             \
        switch((opcode & 0xF0))                                          \
		  {                                                                     \
			case 0xD0:                                                             \
			  /* LDRD rd, [rn + imm] */                                         \
			  arm_access_memory(loadd, up, pre_wb, u16, half_reg);               \
			  break;                                                            \
																			  \
			case 0xF0:                                                             \
			  /* STRD rd, [rn + imm] */                                        \
			  arm_access_memory(stored,up, pre_wb, u16, half_reg);                     \
			  break;                                                            \
																			  \
			case 0xB0:                                                             \
			  /* STRH rd, [rn + imm] */                                        \
			  arm_access_memory(store,up, pre_wb, u16, half_reg);                  \
			  break;                                                            \
		  }                                                                     \
      }                                                                       \
      else                                                                    \
      {                                                                       \
        /* MOV rd, reg_op */                                                  \
        arm_data_proc_unary(mov, reg, no_flags);                              \
      }                                                                       \
      break;                                                                  \
                                                                              \
    case 0x1B:                                                                \
      if((opcode & 0x90) == 0x90)                                             \
      {                                                                       \
        switch((opcode >> 5) & 0x03)                                          \
        {                                                                     \
          case 1:                                                             \
            /* LDRH rd, [rn + rm]! */                                         \
            arm_access_memory(load, up, pre_wb, u16, half_reg);               \
            break;                                                            \
                                                                              \
          case 2:                                                             \
            /* LDRSB rd, [rn + rm]! */                                        \
            arm_access_memory(load, up, pre_wb, s8, half_reg);                \
            break;                                                            \
                                                                              \
          case 3:                                                             \
            /* LDRSH rd, [rn + rm]! */                                        \
            arm_access_memory(load, up, pre_wb, s16, half_reg);               \
            break;                                                            \
        }                                                                     \
      }                                                                       \
      else                                                                    \
      {                                                                       \
        /* MOVS rd, reg_op */                                                 \
        arm_data_proc_unary(movs, reg_flags, flags);                          \
      }                                                                       \
      break;                                                                  \
                                                                              \
    case 0x1C:                                                                \
      if((opcode & 0x90) == 0x90)                                             \
      {                                                                       \
        /* STRH rd, [rn + imm] */                                             \
        switch((opcode & 0xF0))                                          \
		  {                                                                     \
			case 0xD0:                                                             \
			  /* LDRD rd, [rn + imm] */                                         \
			  arm_access_memory(loadd, up, pre, u16, half_imm);               \
			  break;                                                            \
																			  \
			case 0xF0:                                                             \
			  /* STRD rd, [rn + imm] */                                        \
			  arm_access_memory(stored,up, pre, u16, half_imm);                     \
			  break;                                                            \
																			  \
			case 0xB0:                                                             \
			  /* STRH rd, [rn + imm] */                                        \
			  arm_access_memory(store,up, pre, u16, half_imm);                  \
			  break;                                                            \
		  }                                                                     \
      }                                                                       \
      else                                                                    \
      {                                                                       \
        /* BIC rd, rn, reg_op */                                              \
        arm_data_proc(bic, reg, no_flags);                                    \
      }                                                                       \
      break;                                                                  \
                                                                              \
    case 0x1D:                                                                \
      if((opcode & 0x90) == 0x90)                                             \
      {                                                                       \
        switch((opcode >> 5) & 0x03)                                          \
        {                                                                     \
          case 1:                                                             \
            /* LDRH rd, [rn + imm] */                                         \
            arm_access_memory(load, up, pre, u16, half_imm);                  \
            break;                                                            \
                                                                              \
          case 2:                                                             \
            /* LDRSB rd, [rn + imm] */                                        \
            arm_access_memory(load, up, pre, s8, half_imm);                   \
            break;                                                            \
                                                                              \
          case 3:                                                             \
            /* LDRSH rd, [rn + imm] */                                        \
            arm_access_memory(load, up, pre, s16, half_imm);                  \
            break;                                                            \
        }                                                                     \
      }                                                                       \
      else                                                                    \
      {                                                                       \
        /* BICS rd, rn, reg_op */                                             \
        arm_data_proc(bics, reg_flags, flags);                                \
      }                                                                       \
      break;                                                                  \
                                                                              \
    case 0x1E:                                                                \
      if((opcode & 0x90) == 0x90)                                             \
      {                                                                       \
        /* STRH rd, [rn + imm]! */                                            \
		switch((opcode >> 4) & 0x0F)                                          \
		{                                                                     \
		  case 0x0D:                                                             \
			/* LDRD rd, [rn + imm] */                                         \
			arm_access_memory(loadd, up, pre_wb, u16, half_imm);                \
			break;                                                            \
																			  \
		  case 0x0F:                                                             \
			/* STRD rd, [rn + imm] */                                        \
			arm_access_memory(stored, up, pre_wb, u16, half_imm);                  \
			break;                                                            \
																			  \
		  case 0x0B:                                                             \
			/* STRH rd, [rn + imm] */                                        \
			arm_access_memory(store, up, pre_wb, u16, half_imm);                \
			break;                                                            \
		}                                                                     \
      }                                                                       \
      else                                                                    \
      {                                                                       \
        /* MVN rd, reg_op */                                                  \
        arm_data_proc_unary(mvn, reg, no_flags);                              \
      }                                                                       \
      break;                                                                  \
                                                                              \
    case 0x1F:                                                                \
      if((opcode & 0x90) == 0x90)                                             \
      {                                                                       \
        switch((opcode >> 5) & 0x03)                                          \
        {                                                                     \
          case 1:                                                             \
            /* LDRH rd, [rn + imm]! */                                        \
            arm_access_memory(load, up, pre_wb, u16, half_imm);               \
            break;                                                            \
                                                                              \
          case 2:                                                             \
            /* LDRSB rd, [rn + imm]! */                                       \
            arm_access_memory(load, up, pre_wb, s8, half_imm);                \
            break;                                                            \
                                                                              \
          case 3:                                                             \
            /* LDRSH rd, [rn + imm]! */                                       \
            arm_access_memory(load, up, pre_wb, s16, half_imm);               \
            break;                                                            \
        }                                                                     \
      }                                                                       \
      else                                                                    \
      {                                                                       \
        /* MVNS rd, rn, reg_op */                                             \
        arm_data_proc_unary(mvns, reg_flags, flags);                          \
      }                                                                       \
      break;                                                                  \
                                                                              \
    case 0x20:                                                                \
      /* AND rd, rn, imm */                                                   \
      arm_data_proc(and, imm, no_flags);                                      \
      break;                                                                  \
                                                                              \
    case 0x21:                                                                \
      /* ANDS rd, rn, imm */                                                  \
      arm_data_proc(ands, imm_flags, flags);                                  \
      break;                                                                  \
                                                                              \
    case 0x22:                                                                \
      /* EOR rd, rn, imm */                                                   \
      arm_data_proc(eor, imm, no_flags);                                      \
      break;                                                                  \
                                                                              \
    case 0x23:                                                                \
      /* EORS rd, rn, imm */                                                  \
      arm_data_proc(eors, imm_flags, flags);                                  \
      break;                                                                  \
                                                                              \
    case 0x24:                                                                \
      /* SUB rd, rn, imm */                                                   \
      arm_data_proc(sub, imm, no_flags);                                      \
      break;                                                                  \
                                                                              \
    case 0x25:                                                                \
      /* SUBS rd, rn, imm */                                                  \
      arm_data_proc(subs, imm, flags);                                        \
      break;                                                                  \
                                                                              \
    case 0x26:                                                                \
      /* RSB rd, rn, imm */                                                   \
      arm_data_proc(rsb, imm, no_flags);                                      \
      break;                                                                  \
                                                                              \
    case 0x27:                                                                \
      /* RSBS rd, rn, imm */                                                  \
      arm_data_proc(rsbs, imm, flags);                                        \
      break;                                                                  \
                                                                              \
    case 0x28:                                                                \
      /* ADD rd, rn, imm */                                                   \
      arm_data_proc(add, imm, no_flags);                                      \
      break;                                                                  \
                                                                              \
    case 0x29:                                                                \
      /* ADDS rd, rn, imm */                                                  \
      arm_data_proc(adds, imm, flags);                                        \
      break;                                                                  \
                                                                              \
    case 0x2A:                                                                \
      /* ADC rd, rn, imm */                                                   \
      arm_data_proc(adc, imm, no_flags);                                      \
      break;                                                                  \
                                                                              \
    case 0x2B:                                                                \
      /* ADCS rd, rn, imm */                                                  \
      arm_data_proc(adcs, imm, flags);                                        \
      break;                                                                  \
                                                                              \
    case 0x2C:                                                                \
      /* SBC rd, rn, imm */                                                   \
      arm_data_proc(sbc, imm, no_flags);                                      \
      break;                                                                  \
                                                                              \
    case 0x2D:                                                                \
      /* SBCS rd, rn, imm */                                                  \
      arm_data_proc(sbcs, imm, flags);                                        \
      break;                                                                  \
                                                                              \
    case 0x2E:                                                                \
      /* RSC rd, rn, imm */                                                   \
      arm_data_proc(rsc, imm, no_flags);                                      \
      break;                                                                  \
                                                                              \
    case 0x2F:                                                                \
      /* RSCS rd, rn, imm */                                                  \
      arm_data_proc(rscs, imm, flags);                                        \
      break;                                                                  \
                                                                              \
    case 0x30 ... 0x31:                                                       \
      /* TST rn, imm */                                                       \
      arm_data_proc_test(tst, imm);                                           \
      break;                                                                  \
                                                                              \
    case 0x32:                                                                \
      /* MSR cpsr, imm */                                                     \
      arm_psr(imm, store, cpsr);                                              \
      break;                                                                  \
                                                                              \
    case 0x33:                                                                \
      /* TEQ rn, imm */                                                       \
      arm_data_proc_test(teq, imm);                                           \
      break;                                                                  \
                                                                              \
    case 0x34 ... 0x35:                                                       \
      /* CMP rn, imm */                                                       \
      arm_data_proc_test(cmp, imm);                                           \
      break;                                                                  \
                                                                              \
    case 0x36:                                                                \
      /* MSR spsr, imm */                                                     \
      arm_psr(imm, store, spsr);                                              \
      break;                                                                  \
                                                                              \
    case 0x37:                                                                \
      /* CMN rn, imm */                                                       \
      arm_data_proc_test(cmn, imm);                                           \
      break;                                                                  \
                                                                              \
    case 0x38:                                                                \
      /* ORR rd, rn, imm */                                                   \
      arm_data_proc(orr, imm, no_flags);                                      \
      break;                                                                  \
                                                                              \
    case 0x39:                                                                \
      /* ORRS rd, rn, imm */                                                  \
      arm_data_proc(orrs, imm_flags, flags);                                  \
      break;                                                                  \
                                                                              \
    case 0x3A:                                                                \
      /* MOV rd, imm */                                                       \
      arm_data_proc_unary(mov, imm, no_flags);                                \
      break;                                                                  \
                                                                              \
    case 0x3B:                                                                \
      /* MOVS rd, imm */                                                      \
      arm_data_proc_unary(movs, imm_flags, flags);                            \
      break;                                                                  \
                                                                              \
    case 0x3C:                                                                \
      /* BIC rd, rn, imm */                                                   \
      arm_data_proc(bic, imm, no_flags);                                      \
      break;                                                                  \
                                                                              \
    case 0x3D:                                                                \
      /* BICS rd, rn, imm */                                                  \
      arm_data_proc(bics, imm_flags, flags);                                  \
      break;                                                                  \
                                                                              \
    case 0x3E:                                                                \
      /* MVN rd, imm */                                                       \
      arm_data_proc_unary(mvn, imm, no_flags);                                \
      break;                                                                  \
                                                                              \
    case 0x3F:                                                                \
      /* MVNS rd, imm */                                                      \
      arm_data_proc_unary(mvns, imm_flags, flags);                            \
      break;                                                                  \
                                                                              \
    case 0x40:                                                                \
      /* STR rd, [rn], -imm */                                                \
      arm_access_memory(store, down, post, u32, imm);                         \
      break;                                                                  \
                                                                              \
    case 0x41:                                                                \
      /* LDR rd, [rn], -imm */                                                \
      arm_access_memory(load, down, post, u32, imm);                          \
      break;                                                                  \
                                                                              \
    case 0x42:                                                                \
      /* STRT rd, [rn], -imm */                                               \
      arm_access_memory(store, down, post, u32, imm);                         \
      break;                                                                  \
                                                                              \
    case 0x43:                                                                \
      /* LDRT rd, [rn], -imm */                                               \
      arm_access_memory(load, down, post, u32, imm);                          \
      break;                                                                  \
                                                                              \
    case 0x44:                                                                \
      /* STRB rd, [rn], -imm */                                               \
      arm_access_memory(store, down, post, u8, imm);                          \
      break;                                                                  \
                                                                              \
    case 0x45:                                                                \
      /* LDRB rd, [rn], -imm */                                               \
      arm_access_memory(load, down, post, u8, imm);                           \
      break;                                                                  \
                                                                              \
    case 0x46:                                                                \
      /* STRBT rd, [rn], -imm */                                              \
      arm_access_memory(store, down, post, u8, imm);                          \
      break;                                                                  \
                                                                              \
    case 0x47:                                                                \
      /* LDRBT rd, [rn], -imm */                                              \
      arm_access_memory(load, down, post, u8, imm);                           \
      break;                                                                  \
                                                                              \
    case 0x48:                                                                \
      /* STR rd, [rn], +imm */                                                \
      arm_access_memory(store, up, post, u32, imm);                           \
      break;                                                                  \
                                                                              \
    case 0x49:                                                                \
      /* LDR rd, [rn], +imm */                                                \
      arm_access_memory(load, up, post, u32, imm);                            \
      break;                                                                  \
                                                                              \
    case 0x4A:                                                                \
      /* STRT rd, [rn], +imm */                                               \
      arm_access_memory(store, up, post, u32, imm);                           \
      break;                                                                  \
                                                                              \
    case 0x4B:                                                                \
      /* LDRT rd, [rn], +imm */                                               \
      arm_access_memory(load, up, post, u32, imm);                            \
      break;                                                                  \
                                                                              \
    case 0x4C:                                                                \
      /* STRB rd, [rn], +imm */                                               \
      arm_access_memory(store, up, post, u8, imm);                            \
      break;                                                                  \
                                                                              \
    case 0x4D:                                                                \
      /* LDRB rd, [rn], +imm */                                               \
      arm_access_memory(load, up, post, u8, imm);                             \
      break;                                                                  \
                                                                              \
    case 0x4E:                                                                \
      /* STRBT rd, [rn], +imm */                                              \
      arm_access_memory(store, up, post, u8, imm);                            \
      break;                                                                  \
                                                                              \
    case 0x4F:                                                                \
      /* LDRBT rd, [rn], +imm */                                              \
      arm_access_memory(load, up, post, u8, imm);                             \
      break;                                                                  \
                                                                              \
    case 0x50:                                                                \
      /* STR rd, [rn - imm] */                                                \
      arm_access_memory(store, down, pre, u32, imm);                          \
      break;                                                                  \
                                                                              \
    case 0x51:                                                                \
      /* LDR rd, [rn - imm] */                                                \
      arm_access_memory(load, down, pre, u32, imm);                           \
      break;                                                                  \
                                                                              \
    case 0x52:                                                                \
      /* STR rd, [rn - imm]! */                                               \
      arm_access_memory(store, down, pre_wb, u32, imm);                       \
      break;                                                                  \
                                                                              \
    case 0x53:                                                                \
      /* LDR rd, [rn - imm]! */                                               \
      arm_access_memory(load, down, pre_wb, u32, imm);                        \
      break;                                                                  \
                                                                              \
    case 0x54:                                                                \
      /* STRB rd, [rn - imm] */                                               \
      arm_access_memory(store, down, pre, u8, imm);                           \
      break;                                                                  \
                                                                              \
    case 0x55:                                                                \
      /* LDRB rd, [rn - imm] */                                               \
      arm_access_memory(load, down, pre, u8, imm);                            \
      break;                                                                  \
                                                                              \
    case 0x56:                                                                \
      /* STRB rd, [rn - imm]! */                                              \
      arm_access_memory(store, down, pre_wb, u8, imm);                        \
      break;                                                                  \
                                                                              \
    case 0x57:                                                                \
      /* LDRB rd, [rn - imm]! */                                              \
      arm_access_memory(load, down, pre_wb, u8, imm);                         \
      break;                                                                  \
                                                                              \
    case 0x58:                                                                \
      /* STR rd, [rn + imm] */                                                \
      arm_access_memory(store, up, pre, u32, imm);                            \
      break;                                                                  \
                                                                              \
    case 0x59:                                                                \
      /* LDR rd, [rn + imm] */                                                \
      arm_access_memory(load, up, pre, u32, imm);                             \
      break;                                                                  \
                                                                              \
    case 0x5A:                                                                \
      /* STR rd, [rn + imm]! */                                               \
      arm_access_memory(store, up, pre_wb, u32, imm);                         \
      break;                                                                  \
                                                                              \
    case 0x5B:                                                                \
      /* LDR rd, [rn + imm]! */                                               \
      arm_access_memory(load, up, pre_wb, u32, imm);                          \
      break;                                                                  \
                                                                              \
    case 0x5C:                                                                \
      /* STRB rd, [rn + imm] */                                               \
      arm_access_memory(store, up, pre, u8, imm);                             \
      break;                                                                  \
                                                                              \
    case 0x5D:                                                                \
      /* LDRB rd, [rn + imm] */                                               \
      arm_access_memory(load, up, pre, u8, imm);                              \
      break;                                                                  \
                                                                              \
    case 0x5E:                                                                \
      /* STRB rd, [rn + imm]! */                                              \
      arm_access_memory(store, up, pre_wb, u8, imm);                          \
      break;                                                                  \
                                                                              \
    case 0x5F:                                                                \
      /* LDRBT rd, [rn + imm]! */                                             \
      arm_access_memory(load, up, pre_wb, u8, imm);                           \
      break;                                                                  \
                                                                              \
    case 0x60:                                                                \
      /* STR rd, [rn], -rm */                                                 \
      arm_access_memory(store, down, post, u32, reg);                         \
      break;                                                                  \
                                                                              \
    case 0x61:                                                                \
      /* LDR rd, [rn], -rm */                                                 \
      arm_access_memory(load, down, post, u32, reg);                          \
      break;                                                                  \
                                                                              \
    case 0x62:                                                                \
      /* STRT rd, [rn], -rm */                                                \
      arm_access_memory(store, down, post, u32, reg);                         \
      break;                                                                  \
                                                                              \
    case 0x63:                                                                \
      /* LDRT rd, [rn], -rm */                                                \
      arm_access_memory(load, down, post, u32, reg);                          \
      break;                                                                  \
                                                                              \
    case 0x64:                                                                \
      /* STRB rd, [rn], -rm */                                                \
      arm_access_memory(store, down, post, u8, reg);                          \
      break;                                                                  \
                                                                              \
    case 0x65:                                                                \
      /* LDRB rd, [rn], -rm */                                                \
      arm_access_memory(load, down, post, u8, reg);                           \
      break;                                                                  \
                                                                              \
    case 0x66:                                                                \
      /* STRBT rd, [rn], -rm */                                               \
      arm_access_memory(store, down, post, u8, reg);                          \
      break;                                                                  \
                                                                              \
    case 0x67:                                                                \
      /* LDRBT rd, [rn], -rm */                                               \
      arm_access_memory(load, down, post, u8, reg);                           \
      break;                                                                  \
                                                                              \
    case 0x68:                                                                \
      /* STR rd, [rn], +rm */                                                 \
      arm_access_memory(store, up, post, u32, reg);                           \
      break;                                                                  \
                                                                              \
    case 0x69:                                                                \
      /* LDR rd, [rn], +rm */                                                 \
      arm_access_memory(load, up, post, u32, reg);                            \
      break;                                                                  \
                                                                              \
    case 0x6A:                                                                \
      /* STRT rd, [rn], +rm */                                                \
      arm_access_memory(store, up, post, u32, reg);                           \
      break;                                                                  \
                                                                              \
    case 0x6B:                                                                \
      /* LDRT rd, [rn], +rm */                                                \
      arm_access_memory(load, up, post, u32, reg);                            \
      break;                                                                  \
                                                                              \
    case 0x6C:                                                                \
      /* STRB rd, [rn], +rm */                                                \
      arm_access_memory(store, up, post, u8, reg);                            \
      break;                                                                  \
                                                                              \
    case 0x6D:                                                                \
      /* LDRB rd, [rn], +rm */                                                \
      arm_access_memory(load, up, post, u8, reg);                             \
      break;                                                                  \
                                                                              \
    case 0x6E:                                                                \
      /* STRBT rd, [rn], +rm */                                               \
      arm_access_memory(store, up, post, u8, reg);                            \
      break;                                                                  \
                                                                              \
    case 0x6F:                                                                \
      /* LDRBT rd, [rn], +rm */                                               \
      arm_access_memory(load, up, post, u8, reg);                             \
      break;                                                                  \
                                                                              \
    case 0x70:                                                                \
      /* STR rd, [rn - rm] */                                                 \
      arm_access_memory(store, down, pre, u32, reg);                          \
      break;                                                                  \
                                                                              \
    case 0x71:                                                                \
      /* LDR rd, [rn - rm] */                                                 \
      arm_access_memory(load, down, pre, u32, reg);                           \
      break;                                                                  \
                                                                              \
    case 0x72:                                                                \
      /* STR rd, [rn - rm]! */                                                \
      arm_access_memory(store, down, pre_wb, u32, reg);                       \
      break;                                                                  \
                                                                              \
    case 0x73:                                                                \
      /* LDR rd, [rn - rm]! */                                                \
      arm_access_memory(load, down, pre_wb, u32, reg);                        \
      break;                                                                  \
                                                                              \
    case 0x74:                                                                \
      /* STRB rd, [rn - rm] */                                                \
      arm_access_memory(store, down, pre, u8, reg);                           \
      break;                                                                  \
                                                                              \
    case 0x75:                                                                \
      /* LDRB rd, [rn - rm] */                                                \
      arm_access_memory(load, down, pre, u8, reg);                            \
      break;                                                                  \
                                                                              \
    case 0x76:                                                                \
      /* STRB rd, [rn - rm]! */                                               \
      arm_access_memory(store, down, pre_wb, u8, reg);                        \
      break;                                                                  \
                                                                              \
    case 0x77:                                                                \
      /* LDRB rd, [rn - rm]! */                                               \
      arm_access_memory(load, down, pre_wb, u8, reg);                         \
      break;                                                                  \
                                                                              \
    case 0x78:                                                                \
      /* STR rd, [rn + rm] */                                                 \
      arm_access_memory(store, up, pre, u32, reg);                            \
      break;                                                                  \
                                                                              \
    case 0x79:                                                                \
      /* LDR rd, [rn + rm] */                                                 \
      arm_access_memory(load, up, pre, u32, reg);                             \
      break;                                                                  \
                                                                              \
    case 0x7A:                                                                \
      /* STR rd, [rn + rm]! */                                                \
      arm_access_memory(store, up, pre_wb, u32, reg);                         \
      break;                                                                  \
                                                                              \
    case 0x7B:                                                                \
      /* LDR rd, [rn + rm]! */                                                \
      arm_access_memory(load, up, pre_wb, u32, reg);                          \
      break;                                                                  \
                                                                              \
    case 0x7C:                                                                \
      /* STRB rd, [rn + rm] */                                                \
      arm_access_memory(store, up, pre, u8, reg);                             \
      break;                                                                  \
                                                                              \
    case 0x7D:                                                                \
      /* LDRB rd, [rn + rm] */                                                \
      arm_access_memory(load, up, pre, u8, reg);                              \
      break;                                                                  \
                                                                              \
    case 0x7E:                                                                \
      /* STRB rd, [rn + rm]! */                                               \
      arm_access_memory(store, up, pre_wb, u8, reg);                          \
      break;                                                                  \
                                                                              \
    case 0x7F:                                                                \
      /* LDRBT rd, [rn + rm]! */                                              \
      arm_access_memory(load, up, pre_wb, u8, reg);                           \
      break;                                                                  \
                                                                              \
    case 0x80:                                                                \
      /* STMDA rn, rlist */                                                   \
      arm_block_memory(store, down_a, no, no);                                \
      break;                                                                  \
                                                                              \
    case 0x81:                                                                \
      /* LDMDA rn, rlist */                                                   \
      arm_block_memory(load, down_a, no, no);                                 \
      break;                                                                  \
                                                                              \
    case 0x82:                                                                \
      /* STMDA rn!, rlist */                                                  \
      arm_block_memory(store, down_a, down, no);                              \
      break;                                                                  \
                                                                              \
    case 0x83:                                                                \
      /* LDMDA rn!, rlist */                                                  \
      arm_block_memory(load, down_a, down, no);                               \
      break;                                                                  \
                                                                              \
    case 0x84:                                                                \
      /* STMDA rn, rlist^ */                                                  \
      arm_block_memory(store, down_a, no, yes);                               \
      break;                                                                  \
                                                                              \
    case 0x85:                                                                \
      /* LDMDA rn, rlist^ */                                                  \
      arm_block_memory(load, down_a, no, yes);                                \
      break;                                                                  \
                                                                              \
    case 0x86:                                                                \
      /* STMDA rn!, rlist^ */                                                 \
      arm_block_memory(store, down_a, down, yes);                             \
      break;                                                                  \
                                                                              \
    case 0x87:                                                                \
      /* LDMDA rn!, rlist^ */                                                 \
      arm_block_memory(load, down_a, down, yes);                              \
      break;                                                                  \
                                                                              \
    case 0x88:                                                                \
      /* STMIA rn, rlist */                                                   \
      arm_block_memory(store, no, no, no);                                    \
      break;                                                                  \
                                                                              \
    case 0x89:                                                                \
      /* LDMIA rn, rlist */                                                   \
      arm_block_memory(load, no, no, no);                                     \
      break;                                                                  \
                                                                              \
    case 0x8A:                                                                \
      /* STMIA rn!, rlist */                                                  \
      arm_block_memory(store, no, up, no);                                    \
      break;                                                                  \
                                                                              \
    case 0x8B:                                                                \
      /* LDMIA rn!, rlist */                                                  \
      arm_block_memory(load, no, up, no);                                     \
      break;                                                                  \
                                                                              \
    case 0x8C:                                                                \
      /* STMIA rn, rlist^ */                                                  \
      arm_block_memory(store, no, no, yes);                                   \
      break;                                                                  \
                                                                              \
    case 0x8D:                                                                \
      /* LDMIA rn, rlist^ */                                                  \
      arm_block_memory(load, no, no, yes);                                    \
      break;                                                                  \
                                                                              \
    case 0x8E:                                                                \
      /* STMIA rn!, rlist^ */                                                 \
      arm_block_memory(store, no, up, yes);                                   \
      break;                                                                  \
                                                                              \
    case 0x8F:                                                                \
      /* LDMIA rn!, rlist^ */                                                 \
      arm_block_memory(load, no, up, yes);                                    \
      break;                                                                  \
                                                                              \
    case 0x90:                                                                \
      /* STMDB rn, rlist */                                                   \
      arm_block_memory(store, down_b, no, no);                                \
      break;                                                                  \
                                                                              \
    case 0x91:                                                                \
      /* LDMDB rn, rlist */                                                   \
      arm_block_memory(load, down_b, no, no);                                 \
      break;                                                                  \
                                                                              \
    case 0x92:                                                                \
      /* STMDB rn!, rlist */                                                  \
      arm_block_memory(store, down_b, down, no);                              \
      break;                                                                  \
                                                                              \
    case 0x93:                                                                \
      /* LDMDB rn!, rlist */                                                  \
      arm_block_memory(load, down_b, down, no);                               \
      break;                                                                  \
                                                                              \
    case 0x94:                                                                \
      /* STMDB rn, rlist^ */                                                  \
      arm_block_memory(store, down_b, no, yes);                               \
      break;                                                                  \
                                                                              \
    case 0x95:                                                                \
      /* LDMDB rn, rlist^ */                                                  \
      arm_block_memory(load, down_b, no, yes);                                \
      break;                                                                  \
                                                                              \
    case 0x96:                                                                \
      /* STMDB rn!, rlist^ */                                                 \
      arm_block_memory(store, down_b, down, yes);                             \
      break;                                                                  \
                                                                              \
    case 0x97:                                                                \
      /* LDMDB rn!, rlist^ */                                                 \
      arm_block_memory(load, down_b, down, yes);                              \
      break;                                                                  \
                                                                              \
    case 0x98:                                                                \
      /* STMIB rn, rlist */                                                   \
      arm_block_memory(store, up, no, no);                                    \
      break;                                                                  \
                                                                              \
    case 0x99:                                                                \
      /* LDMIB rn, rlist */                                                   \
      arm_block_memory(load, up, no, no);                                     \
      break;                                                                  \
                                                                              \
    case 0x9A:                                                                \
      /* STMIB rn!, rlist */                                                  \
      arm_block_memory(store, up, up, no);                                    \
      break;                                                                  \
                                                                              \
    case 0x9B:                                                                \
      /* LDMIB rn!, rlist */                                                  \
      arm_block_memory(load, up, up, no);                                     \
      break;                                                                  \
                                                                              \
    case 0x9C:                                                                \
      /* STMIB rn, rlist^ */                                                  \
      arm_block_memory(store, up, no, yes);                                   \
      break;                                                                  \
                                                                              \
    case 0x9D:                                                                \
      /* LDMIB rn, rlist^ */                                                  \
      arm_block_memory(load, up, no, yes);                                    \
      break;                                                                  \
                                                                              \
    case 0x9E:                                                                \
      /* STMIB rn!, rlist^ */                                                 \
      arm_block_memory(store, up, up, yes);                                   \
      break;                                                                  \
                                                                              \
    case 0x9F:                                                                \
      /* LDMIB rn!, rlist^ */                                                 \
      arm_block_memory(load, up, up, yes);                                    \
      break;                                                                  \
                                                                              \
    case 0xA0 ... 0xAF:                                                       \
    {                                                                         \
      /* B offset */                                                          \
		if(condition == 0x0F)    \
		{\
arm_blx_imm();   \
}\
		      else    \
		      {\
      arm_b();                                                                \
}\
      break;                                                                  \
    }                                                                         \
                                                                              \
    case 0xB0 ... 0xBF:                                                       \
    {                                                                         \
      /* BL offset */                                                         \
      if(condition == 0x0F)   \
      {\
    	  arm_blx_imm();    \
      }\
      else     \
      {\
      arm_bl();                                                               \
      }\
      break;                                                                  \
    }                                                                         \
                                                                              \
    case 0xC0 ... 0xEF:                                                       \
    {\
      /* coprocessor instructions, reserved on GBA */                         \
	  if((opcode >> 20) & 0x01)\
	  {\
		  arm_mrc_emit();\
	  }\
	  else\
	  {\
		  arm_mcr_emit();\
	  }\
\
      break;                                                                  \
    }\
                                                                              \
    case 0xF0 ... 0xFF:                                                       \
    {                                                                         \
      /* SWI comment */                                                       \
      arm_swi();                                                              \
      break;                                                                  \
    }                                                                         \
  }                                                                           \
                                                                              \
  pc += 4                                                                     \

#define arm_flag_status()                                                     \

bool print_instruct=false;

#define translate_thumb_instruction()                                         \
  flag_status = block_data[block_data_position].flag_data;                    \
  last_opcode = opcode;                                                       \
  opcode = _MMU_read16(dynarec_proc, MMU_AT_CODE, pc);                        \
  /*if(block_print)LOGE("instruction before trans %x pc %x", opcode, pc);*/\
                                                                              \
  switch((opcode >> 8) & 0xFF)                                                \
  {                                                                           \
    case 0x00 ... 0x07:                                                       \
      /* LSL rd, rs, imm */                                                   \
      thumb_shift(shift, lsl, imm);                                           \
      break;                                                                  \
                                                                              \
    case 0x08 ... 0x0F:                                                       \
      /* LSR rd, rs, imm */                                                   \
      thumb_shift(shift, lsr, imm);                                           \
      break;                                                                  \
                                                                              \
    case 0x10 ... 0x17:                                                       \
      /* ASR rd, rs, imm */                                                   \
      thumb_shift(shift, asr, imm);                                           \
      break;                                                                  \
                                                                              \
    case 0x18 ... 0x19:                                                       \
      /* ADD rd, rs, rn */                                                    \
      thumb_data_proc(add_sub, adds, reg, rd, rs, rn);                        \
      break;                                                                  \
                                                                              \
    case 0x1A ... 0x1B:                                                       \
      /* SUB rd, rs, rn */                                                    \
      thumb_data_proc(add_sub, subs, reg, rd, rs, rn);                        \
      break;                                                                  \
                                                                              \
    case 0x1C ... 0x1D:                                                       \
      /* ADD rd, rs, imm */                                                   \
      thumb_data_proc(add_sub_imm, adds, imm, rd, rs, imm);                   \
      break;                                                                  \
                                                                              \
    case 0x1E ... 0x1F:                                                       \
      /* SUB rd, rs, imm */                                                   \
      thumb_data_proc(add_sub_imm, subs, imm, rd, rs, imm);                   \
      break;                                                                  \
                                                                              \
    case 0x20:                                                                \
      /* MOV r0, imm */                                                       \
      thumb_data_proc_unary(imm, movs, imm, 0, imm);                          \
      break;                                                                  \
                                                                              \
    case 0x21:                                                                \
      /* MOV r1, imm */                                                       \
      thumb_data_proc_unary(imm, movs, imm, 1, imm);                          \
      break;                                                                  \
                                                                              \
    case 0x22:                                                                \
      /* MOV r2, imm */                                                       \
      thumb_data_proc_unary(imm, movs, imm, 2, imm);                          \
      break;                                                                  \
                                                                              \
    case 0x23:                                                                \
      /* MOV r3, imm */                                                       \
      thumb_data_proc_unary(imm, movs, imm, 3, imm);                          \
      break;                                                                  \
                                                                              \
    case 0x24:                                                                \
      /* MOV r4, imm */                                                       \
      thumb_data_proc_unary(imm, movs, imm, 4, imm);                          \
      break;                                                                  \
                                                                              \
    case 0x25:                                                                \
      /* MOV r5, imm */                                                       \
      thumb_data_proc_unary(imm, movs, imm, 5, imm);                          \
      break;                                                                  \
                                                                              \
    case 0x26:                                                                \
      /* MOV r6, imm */                                                       \
      thumb_data_proc_unary(imm, movs, imm, 6, imm);                          \
      break;                                                                  \
                                                                              \
    case 0x27:                                                                \
      /* MOV r7, imm */                                                       \
      thumb_data_proc_unary(imm, movs, imm, 7, imm);                          \
      break;                                                                  \
                                                                              \
    case 0x28:                                                                \
      /* CMP r0, imm */                                                       \
      thumb_data_proc_test(imm, cmp, imm, 0, imm);                            \
      break;                                                                  \
                                                                              \
    case 0x29:                                                                \
      /* CMP r1, imm */                                                       \
      thumb_data_proc_test(imm, cmp, imm, 1, imm);                            \
      break;                                                                  \
                                                                              \
    case 0x2A:                                                                \
      /* CMP r2, imm */                                                       \
      thumb_data_proc_test(imm, cmp, imm, 2, imm);                            \
      break;                                                                  \
                                                                              \
    case 0x2B:                                                                \
      /* CMP r3, imm */                                                       \
      thumb_data_proc_test(imm, cmp, imm, 3, imm);                            \
      break;                                                                  \
                                                                              \
    case 0x2C:                                                                \
      /* CMP r4, imm */                                                       \
      thumb_data_proc_test(imm, cmp, imm, 4, imm);                            \
      break;                                                                  \
                                                                              \
    case 0x2D:                                                                \
      /* CMP r5, imm */                                                       \
      thumb_data_proc_test(imm, cmp, imm, 5, imm);                            \
      break;                                                                  \
                                                                              \
    case 0x2E:                                                                \
      /* CMP r6, imm */                                                       \
      thumb_data_proc_test(imm, cmp, imm, 6, imm);                            \
      break;                                                                  \
                                                                              \
    case 0x2F:                                                                \
      /* CMP r7, imm */                                                       \
      thumb_data_proc_test(imm, cmp, imm, 7, imm);                            \
      break;                                                                  \
                                                                              \
    case 0x30:                                                                \
      /* ADD r0, imm */                                                       \
      thumb_data_proc(imm, adds, imm, 0, 0, imm);                             \
      break;                                                                  \
                                                                              \
    case 0x31:                                                                \
      /* ADD r1, imm */                                                       \
      thumb_data_proc(imm, adds, imm, 1, 1, imm);                             \
      break;                                                                  \
                                                                              \
    case 0x32:                                                                \
      /* ADD r2, imm */                                                       \
      thumb_data_proc(imm, adds, imm, 2, 2, imm);                             \
      break;                                                                  \
                                                                              \
    case 0x33:                                                                \
      /* ADD r3, imm */                                                       \
      thumb_data_proc(imm, adds, imm, 3, 3, imm);                             \
      break;                                                                  \
                                                                              \
    case 0x34:                                                                \
      /* ADD r4, imm */                                                       \
      thumb_data_proc(imm, adds, imm, 4, 4, imm);                             \
      break;                                                                  \
                                                                              \
    case 0x35:                                                                \
      /* ADD r5, imm */                                                       \
      thumb_data_proc(imm, adds, imm, 5, 5, imm);                             \
      break;                                                                  \
                                                                              \
    case 0x36:                                                                \
      /* ADD r6, imm */                                                       \
      thumb_data_proc(imm, adds, imm, 6, 6, imm);                             \
      break;                                                                  \
                                                                              \
    case 0x37:                                                                \
      /* ADD r7, imm */                                                       \
      thumb_data_proc(imm, adds, imm, 7, 7, imm);                             \
      break;                                                                  \
                                                                              \
    case 0x38:                                                                \
      /* SUB r0, imm */                                                       \
      thumb_data_proc(imm, subs, imm, 0, 0, imm);                             \
      break;                                                                  \
                                                                              \
    case 0x39:                                                                \
      /* SUB r1, imm */                                                       \
      thumb_data_proc(imm, subs, imm, 1, 1, imm);                             \
      break;                                                                  \
                                                                              \
    case 0x3A:                                                                \
      /* SUB r2, imm */                                                       \
      thumb_data_proc(imm, subs, imm, 2, 2, imm);                             \
      break;                                                                  \
                                                                              \
    case 0x3B:                                                                \
      /* SUB r3, imm */                                                       \
      thumb_data_proc(imm, subs, imm, 3, 3, imm);                             \
      break;                                                                  \
                                                                              \
    case 0x3C:                                                                \
      /* SUB r4, imm */                                                       \
      thumb_data_proc(imm, subs, imm, 4, 4, imm);                             \
      break;                                                                  \
                                                                              \
    case 0x3D:                                                                \
      /* SUB r5, imm */                                                       \
      thumb_data_proc(imm, subs, imm, 5, 5, imm);                             \
      break;                                                                  \
                                                                              \
    case 0x3E:                                                                \
      /* SUB r6, imm */                                                       \
      thumb_data_proc(imm, subs, imm, 6, 6, imm);                             \
      break;                                                                  \
                                                                              \
    case 0x3F:                                                                \
      /* SUB r7, imm */                                                       \
      thumb_data_proc(imm, subs, imm, 7, 7, imm);                             \
      break;                                                                  \
                                                                              \
    case 0x40:                                                                \
      switch((opcode >> 6) & 0x03)                                            \
      {                                                                       \
        case 0x00:                                                            \
          /* AND rd, rs */                                                    \
          thumb_data_proc(alu_op, ands, reg, rd, rd, rs);                     \
          break;                                                              \
                                                                              \
        case 0x01:                                                            \
          /* EOR rd, rs */                                                    \
          thumb_data_proc(alu_op, eors, reg, rd, rd, rs);                     \
          break;                                                              \
                                                                              \
        case 0x02:                                                            \
          /* LSL rd, rs */                                                    \
          thumb_shift(alu_op, lsl, reg);                                      \
          break;                                                              \
                                                                              \
        case 0x03:                                                            \
          /* LSR rd, rs */                                                    \
          thumb_shift(alu_op, lsr, reg);                                      \
          break;                                                              \
      }                                                                       \
      break;                                                                  \
                                                                              \
    case 0x41:                                                                \
      switch((opcode >> 6) & 0x03)                                            \
      {                                                                       \
        case 0x00:                                                            \
          /* ASR rd, rs */                                                    \
          thumb_shift(alu_op, asr, reg);                                      \
          break;                                                              \
                                                                              \
        case 0x01:                                                            \
          /* ADC rd, rs */                                                    \
          thumb_data_proc(alu_op, adcs, reg, rd, rd, rs);                     \
          break;                                                              \
                                                                              \
        case 0x02:                                                            \
          /* SBC rd, rs */                                                    \
          thumb_data_proc(alu_op, sbcs, reg, rd, rd, rs);                     \
          break;                                                              \
                                                                              \
        case 0x03:                                                            \
          /* ROR rd, rs */                                                    \
          thumb_shift(alu_op, ror, reg);                                      \
          break;                                                              \
      }                                                                       \
      break;                                                                  \
                                                                              \
    case 0x42:                                                                \
      switch((opcode >> 6) & 0x03)                                            \
      {                                                                       \
        case 0x00:                                                            \
          /* TST rd, rs */                                                    \
          thumb_data_proc_test(alu_op, tst, reg, rd, rs);                     \
          break;                                                              \
                                                                              \
        case 0x01:                                                            \
          /* NEG rd, rs */                                                    \
          thumb_data_proc_unary(alu_op, neg, reg, rd, rs);                    \
          break;                                                              \
                                                                              \
        case 0x02:                                                            \
          /* CMP rd, rs */                                                    \
          thumb_data_proc_test(alu_op, cmp, reg, rd, rs);                     \
          break;                                                              \
                                                                              \
        case 0x03:                                                            \
          /* CMN rd, rs */                                                    \
          thumb_data_proc_test(alu_op, cmn, reg, rd, rs);                     \
          break;                                                              \
      }                                                                       \
      break;                                                                  \
                                                                              \
    case 0x43:                                                                \
      switch((opcode >> 6) & 0x03)                                            \
      {                                                                       \
        case 0x00:                                                            \
          /* ORR rd, rs */                                                    \
          thumb_data_proc(alu_op, orrs, reg, rd, rd, rs);                     \
          break;                                                              \
                                                                              \
        case 0x01:                                                            \
          /* MUL rd, rs */                                                    \
          thumb_data_proc(alu_op, muls, reg, rd, rd, rs);                     \
          break;                                                              \
                                                                              \
        case 0x02:                                                            \
          /* BIC rd, rs */                                                    \
          thumb_data_proc(alu_op, bics, reg, rd, rd, rs);                     \
          break;                                                              \
                                                                              \
        case 0x03:                                                            \
          /* MVN rd, rs */                                                    \
          thumb_data_proc_unary(alu_op, mvns, reg, rd, rs);                   \
          break;                                                              \
      }                                                                       \
      break;                                                                  \
                                                                              \
    case 0x44:                                                                \
      /* ADD rd, rs */                                                        \
      thumb_data_proc_hi(add);                                                \
      break;                                                                  \
                                                                              \
    case 0x45:                                                                \
      /* CMP rd, rs */                                                        \
      thumb_data_proc_test_hi(cmp);                                           \
      break;                                                                  \
                                                                              \
    case 0x46:                                                                \
      /* MOV rd, rs */                                                        \
      thumb_data_proc_mov_hi();                                               \
      break;                                                                  \
                                                                              \
    case 0x47:                                                                \
      /* BX rs */                                                             \
    	if(((opcode >> 7) & 0x01) == 0x01)\
    	{\
    		thumb_blx_reg();\
    	}\
    	else\
    	{\
      thumb_bx();                                                             \
    	}\
      break;                                                                  \
                                                                              \
    case 0x48:                                                                \
      /* LDR r0, [pc + imm] */                                                \
      thumb_access_memory(load, imm, 0, 0, 0, pc_relative,                    \
       (pc & ~2) + (imm * 4) + 4, u32);                                       \
      break;                                                                  \
                                                                              \
    case 0x49:                                                                \
      /* LDR r1, [pc + imm] */                                                \
      thumb_access_memory(load, imm, 1, 0, 0, pc_relative,                    \
       (pc & ~2) + (imm * 4) + 4, u32);                                       \
      break;                                                                  \
                                                                              \
    case 0x4A:                                                                \
      /* LDR r2, [pc + imm] */                                                \
      thumb_access_memory(load, imm, 2, 0, 0, pc_relative,                    \
       (pc & ~2) + (imm * 4) + 4, u32);                                       \
      break;                                                                  \
                                                                              \
    case 0x4B:                                                                \
      /* LDR r3, [pc + imm] */                                                \
      thumb_access_memory(load, imm, 3, 0, 0, pc_relative,                    \
       (pc & ~2) + (imm * 4) + 4, u32);                                       \
      break;                                                                  \
                                                                              \
    case 0x4C:                                                                \
      /* LDR r4, [pc + imm] */                                                \
      thumb_access_memory(load, imm, 4, 0, 0, pc_relative,                    \
       (pc & ~2) + (imm * 4) + 4, u32);                                       \
      break;                                                                  \
                                                                              \
    case 0x4D:                                                                \
      /* LDR r5, [pc + imm] */                                                \
      thumb_access_memory(load, imm, 5, 0, 0, pc_relative,                    \
       (pc & ~2) + (imm * 4) + 4, u32);                                       \
      break;                                                                  \
                                                                              \
    case 0x4E:                                                                \
      /* LDR r6, [pc + imm] */                                                \
      thumb_access_memory(load, imm, 6, 0, 0, pc_relative,                    \
       (pc & ~2) + (imm * 4) + 4, u32);                                       \
      break;                                                                  \
                                                                              \
    case 0x4F:                                                                \
      /* LDR r7, [pc + imm] */                                                \
      thumb_access_memory(load, imm, 7, 0, 0, pc_relative,                    \
       (pc & ~2) + (imm * 4) + 4, u32);                                       \
      break;                                                                  \
                                                                              \
    case 0x50 ... 0x51:                                                       \
      /* STR rd, [rb + ro] */                                                 \
      thumb_access_memory(store, mem_reg, rd, rb, ro, reg_reg, 0, u32);       \
      break;                                                                  \
                                                                              \
    case 0x52 ... 0x53:                                                       \
      /* STRH rd, [rb + ro] */                                                \
      thumb_access_memory(store, mem_reg, rd, rb, ro, reg_reg, 0, u16);       \
      break;                                                                  \
                                                                              \
    case 0x54 ... 0x55:                                                       \
      /* STRB rd, [rb + ro] */                                                \
      thumb_access_memory(store, mem_reg, rd, rb, ro, reg_reg, 0, u8);        \
      break;                                                                  \
                                                                              \
    case 0x56 ... 0x57:                                                       \
      /* LDSB rd, [rb + ro] */                                                \
      thumb_access_memory(load, mem_reg, rd, rb, ro, reg_reg, 0, s8);         \
      break;                                                                  \
                                                                              \
    case 0x58 ... 0x59:                                                       \
      /* LDR rd, [rb + ro] */                                                 \
      thumb_access_memory(load, mem_reg, rd, rb, ro, reg_reg, 0, u32);        \
      break;                                                                  \
                                                                              \
    case 0x5A ... 0x5B:                                                       \
      /* LDRH rd, [rb + ro] */                                                \
      thumb_access_memory(load, mem_reg, rd, rb, ro, reg_reg, 0, u16);        \
      break;                                                                  \
                                                                              \
    case 0x5C ... 0x5D:                                                       \
      /* LDRB rd, [rb + ro] */                                                \
      thumb_access_memory(load, mem_reg, rd, rb, ro, reg_reg, 0, u8);         \
      break;                                                                  \
                                                                              \
    case 0x5E ... 0x5F:                                                       \
      /* LDSH rd, [rb + ro] */                                                \
      thumb_access_memory(load, mem_reg, rd, rb, ro, reg_reg, 0, s16);        \
      break;                                                                  \
                                                                              \
    case 0x60 ... 0x67:                                                       \
      /* STR rd, [rb + imm] */                                                \
      thumb_access_memory(store, mem_imm, rd, rb, 0, reg_imm, (imm * 4),      \
       u32);                                                                  \
      break;                                                                  \
                                                                              \
    case 0x68 ... 0x6F:                                                       \
      /* LDR rd, [rb + imm] */                                                \
      thumb_access_memory(load, mem_imm, rd, rb, 0, reg_imm, (imm * 4), u32); \
      break;                                                                  \
                                                                              \
    case 0x70 ... 0x77:                                                       \
      /* STRB rd, [rb + imm] */                                               \
      thumb_access_memory(store, mem_imm, rd, rb, 0, reg_imm, imm, u8);       \
      break;                                                                  \
                                                                              \
    case 0x78 ... 0x7F:                                                       \
      /* LDRB rd, [rb + imm] */                                               \
      thumb_access_memory(load, mem_imm, rd, rb, 0, reg_imm, imm, u8);        \
      break;                                                                  \
                                                                              \
    case 0x80 ... 0x87:                                                       \
      /* STRH rd, [rb + imm] */                                               \
      thumb_access_memory(store, mem_imm, rd, rb, 0, reg_imm,                 \
       (imm * 2), u16);                                                       \
      break;                                                                  \
                                                                              \
    case 0x88 ... 0x8F:                                                       \
      /* LDRH rd, [rb + imm] */                                               \
      thumb_access_memory(load, mem_imm, rd, rb, 0, reg_imm, (imm * 2), u16); \
      break;                                                                  \
                                                                              \
    case 0x90:                                                                \
      /* STR r0, [sp + imm] */                                                \
      thumb_access_memory(store, imm, 0, 13, 0, reg_imm_sp, imm, u32);        \
      break;                                                                  \
                                                                              \
    case 0x91:                                                                \
      /* STR r1, [sp + imm] */                                                \
      thumb_access_memory(store, imm, 1, 13, 0, reg_imm_sp, imm, u32);        \
      break;                                                                  \
                                                                              \
    case 0x92:                                                                \
      /* STR r2, [sp + imm] */                                                \
      thumb_access_memory(store, imm, 2, 13, 0, reg_imm_sp, imm, u32);        \
      break;                                                                  \
                                                                              \
    case 0x93:                                                                \
      /* STR r3, [sp + imm] */                                                \
      thumb_access_memory(store, imm, 3, 13, 0, reg_imm_sp, imm, u32);        \
      break;                                                                  \
                                                                              \
    case 0x94:                                                                \
      /* STR r4, [sp + imm] */                                                \
      thumb_access_memory(store, imm, 4, 13, 0, reg_imm_sp, imm, u32);        \
      break;                                                                  \
                                                                              \
    case 0x95:                                                                \
      /* STR r5, [sp + imm] */                                                \
      thumb_access_memory(store, imm, 5, 13, 0, reg_imm_sp, imm, u32);        \
      break;                                                                  \
                                                                              \
    case 0x96:                                                                \
      /* STR r6, [sp + imm] */                                                \
      thumb_access_memory(store, imm, 6, 13, 0, reg_imm_sp, imm, u32);        \
      break;                                                                  \
                                                                              \
    case 0x97:                                                                \
      /* STR r7, [sp + imm] */                                                \
      thumb_access_memory(store, imm, 7, 13, 0, reg_imm_sp, imm, u32);        \
      break;                                                                  \
                                                                              \
    case 0x98:                                                                \
      /* LDR r0, [sp + imm] */                                                \
      thumb_access_memory(load, imm, 0, 13, 0, reg_imm_sp, imm, u32);         \
      break;                                                                  \
                                                                              \
    case 0x99:                                                                \
      /* LDR r1, [sp + imm] */                                                \
      thumb_access_memory(load, imm, 1, 13, 0, reg_imm_sp, imm, u32);         \
      break;                                                                  \
                                                                              \
    case 0x9A:                                                                \
      /* LDR r2, [sp + imm] */                                                \
      thumb_access_memory(load, imm, 2, 13, 0, reg_imm_sp, imm, u32);         \
      break;                                                                  \
                                                                              \
    case 0x9B:                                                                \
      /* LDR r3, [sp + imm] */                                                \
      thumb_access_memory(load, imm, 3, 13, 0, reg_imm_sp, imm, u32);         \
      break;                                                                  \
                                                                              \
    case 0x9C:                                                                \
      /* LDR r4, [sp + imm] */                                                \
      thumb_access_memory(load, imm, 4, 13, 0, reg_imm_sp, imm, u32);         \
      break;                                                                  \
                                                                              \
    case 0x9D:                                                                \
      /* LDR r5, [sp + imm] */                                                \
      thumb_access_memory(load, imm, 5, 13, 0, reg_imm_sp, imm, u32);         \
      break;                                                                  \
                                                                              \
    case 0x9E:                                                                \
      /* LDR r6, [sp + imm] */                                                \
      thumb_access_memory(load, imm, 6, 13, 0, reg_imm_sp, imm, u32);         \
      break;                                                                  \
                                                                              \
    case 0x9F:                                                                \
      /* LDR r7, [sp + imm] */                                                \
      thumb_access_memory(load, imm, 7, 13, 0, reg_imm_sp, imm, u32);         \
      break;                                                                  \
                                                                              \
    case 0xA0:                                                                \
      /* ADD r0, pc, +imm */                                                  \
      thumb_load_pc(0);                                                       \
      break;                                                                  \
                                                                              \
    case 0xA1:                                                                \
      /* ADD r1, pc, +imm */                                                  \
      thumb_load_pc(1);                                                       \
      break;                                                                  \
                                                                              \
    case 0xA2:                                                                \
      /* ADD r2, pc, +imm */                                                  \
      thumb_load_pc(2);                                                       \
      break;                                                                  \
                                                                              \
    case 0xA3:                                                                \
      /* ADD r3, pc, +imm */                                                  \
      thumb_load_pc(3);                                                       \
      break;                                                                  \
                                                                              \
    case 0xA4:                                                                \
      /* ADD r4, pc, +imm */                                                  \
      thumb_load_pc(4);                                                       \
      break;                                                                  \
                                                                              \
    case 0xA5:                                                                \
      /* ADD r5, pc, +imm */                                                  \
      thumb_load_pc(5);                                                       \
      break;                                                                  \
                                                                              \
    case 0xA6:                                                                \
      /* ADD r6, pc, +imm */                                                  \
      thumb_load_pc(6);                                                       \
      break;                                                                  \
                                                                              \
    case 0xA7:                                                                \
      /* ADD r7, pc, +imm */                                                  \
      thumb_load_pc(7);                                                       \
      break;                                                                  \
                                                                              \
    case 0xA8:                                                                \
      /* ADD r0, sp, +imm */                                                  \
      thumb_load_sp(0);                                                       \
      break;                                                                  \
                                                                              \
    case 0xA9:                                                                \
      /* ADD r1, sp, +imm */                                                  \
      thumb_load_sp(1);                                                       \
      break;                                                                  \
                                                                              \
    case 0xAA:                                                                \
      /* ADD r2, sp, +imm */                                                  \
      thumb_load_sp(2);                                                       \
      break;                                                                  \
                                                                              \
    case 0xAB:                                                                \
      /* ADD r3, sp, +imm */                                                  \
      thumb_load_sp(3);                                                       \
      break;                                                                  \
                                                                              \
    case 0xAC:                                                                \
      /* ADD r4, sp, +imm */                                                  \
      thumb_load_sp(4);                                                       \
      break;                                                                  \
                                                                              \
    case 0xAD:                                                                \
      /* ADD r5, sp, +imm */                                                  \
      thumb_load_sp(5);                                                       \
      break;                                                                  \
                                                                              \
    case 0xAE:                                                                \
      /* ADD r6, sp, +imm */                                                  \
      thumb_load_sp(6);                                                       \
      break;                                                                  \
                                                                              \
    case 0xAF:                                                                \
      /* ADD r7, sp, +imm */                                                  \
      thumb_load_sp(7);                                                       \
      break;                                                                  \
                                                                              \
    case 0xB0 ... 0xB3:                                                       \
      if((opcode >> 7) & 0x01)                                                \
      {                                                                       \
        /* ADD sp, -imm */                                                    \
        thumb_adjust_sp(down);                                                \
      }                                                                       \
      else                                                                    \
      {                                                                       \
        /* ADD sp, +imm */                                                    \
        thumb_adjust_sp(up);                                                  \
      }                                                                       \
      break;                                                                  \
                                                                              \
    case 0xB4:                                                                \
      /* PUSH rlist */                                                        \
      thumb_block_memory(store, down, no, 13);                                \
      break;                                                                  \
                                                                              \
    case 0xB5:                                                                \
      /* PUSH rlist, lr */                                                    \
      thumb_block_memory(store, push_lr, push_lr, 13);                        \
      break;                                                                  \
                                                                              \
    case 0xBC:                                                                \
      /* POP rlist */                                                         \
      thumb_block_memory(load, no, up, 13);                                   \
      break;                                                                  \
                                                                              \
    case 0xBD:                                                                \
      /* POP rlist, pc */                                                     \
      thumb_block_memory(load, no, pop_pc, 13);                               \
      break;                                                                  \
                                                                              \
    case 0xC0:                                                                \
      /* STMIA r0!, rlist */                                                  \
      thumb_block_memory(store, no, up, 0);                                   \
      break;                                                                  \
                                                                              \
    case 0xC1:                                                                \
      /* STMIA r1!, rlist */                                                  \
      thumb_block_memory(store, no, up, 1);                                   \
      break;                                                                  \
                                                                              \
    case 0xC2:                                                                \
      /* STMIA r2!, rlist */                                                  \
      thumb_block_memory(store, no, up, 2);                                   \
      break;                                                                  \
                                                                              \
    case 0xC3:                                                                \
      /* STMIA r3!, rlist */                                                  \
      thumb_block_memory(store, no, up, 3);                                   \
      break;                                                                  \
                                                                              \
    case 0xC4:                                                                \
      /* STMIA r4!, rlist */                                                  \
      thumb_block_memory(store, no, up, 4);                                   \
      break;                                                                  \
                                                                              \
    case 0xC5:                                                                \
      /* STMIA r5!, rlist */                                                  \
      thumb_block_memory(store, no, up, 5);                                   \
      break;                                                                  \
                                                                              \
    case 0xC6:                                                                \
      /* STMIA r6!, rlist */                                                  \
      thumb_block_memory(store, no, up, 6);                                   \
      break;                                                                  \
                                                                              \
    case 0xC7:                                                                \
      /* STMIA r7!, rlist */                                                  \
      thumb_block_memory(store, no, up, 7);                                   \
      break;                                                                  \
                                                                              \
    case 0xC8:                                                                \
      /* LDMIA r0!, rlist */                                                  \
      thumb_block_memory(load, no, up, 0);                                    \
      break;                                                                  \
                                                                              \
    case 0xC9:                                                                \
      /* LDMIA r1!, rlist */                                                  \
      thumb_block_memory(load, no, up, 1);                                    \
      break;                                                                  \
                                                                              \
    case 0xCA:                                                                \
      /* LDMIA r2!, rlist */                                                  \
      thumb_block_memory(load, no, up, 2);                                    \
      break;                                                                  \
                                                                              \
    case 0xCB:                                                                \
      /* LDMIA r3!, rlist */                                                  \
      thumb_block_memory(load, no, up, 3);                                    \
      break;                                                                  \
                                                                              \
    case 0xCC:                                                                \
      /* LDMIA r4!, rlist */                                                  \
      thumb_block_memory(load, no, up, 4);                                    \
      break;                                                                  \
                                                                              \
    case 0xCD:                                                                \
      /* LDMIA r5!, rlist */                                                  \
      thumb_block_memory(load, no, up, 5);                                    \
      break;                                                                  \
                                                                              \
    case 0xCE:                                                                \
      /* LDMIA r6!, rlist */                                                  \
      thumb_block_memory(load, no, up, 6);                                    \
      break;                                                                  \
                                                                              \
    case 0xCF:                                                                \
      /* LDMIA r7!, rlist */                                                  \
      thumb_block_memory(load, no, up, 7);                                    \
      break;                                                                  \
                                                                              \
    case 0xD0:                                                                \
      /* BEQ label */                                                         \
      thumb_conditional_branch(eq);                                           \
      break;                                                                  \
                                                                              \
    case 0xD1:                                                                \
      /* BNE label */                                                         \
      thumb_conditional_branch(ne);                                           \
      break;                                                                  \
                                                                              \
    case 0xD2:                                                                \
      /* BCS label */                                                         \
      thumb_conditional_branch(cs);                                           \
      break;                                                                  \
                                                                              \
    case 0xD3:                                                                \
      /* BCC label */                                                         \
      thumb_conditional_branch(cc);                                           \
      break;                                                                  \
                                                                              \
    case 0xD4:                                                                \
      /* BMI label */                                                         \
      thumb_conditional_branch(mi);                                           \
      break;                                                                  \
                                                                              \
    case 0xD5:                                                                \
      /* BPL label */                                                         \
      thumb_conditional_branch(pl);                                           \
      break;                                                                  \
                                                                              \
    case 0xD6:                                                                \
      /* BVS label */                                                         \
      thumb_conditional_branch(vs);                                           \
      break;                                                                  \
                                                                              \
    case 0xD7:                                                                \
      /* BVC label */                                                         \
      thumb_conditional_branch(vc);                                           \
      break;                                                                  \
                                                                              \
    case 0xD8:                                                                \
      /* BHI label */                                                         \
      thumb_conditional_branch(hi);                                           \
      break;                                                                  \
                                                                              \
    case 0xD9:                                                                \
      /* BLS label */                                                         \
      thumb_conditional_branch(ls);                                           \
      break;                                                                  \
                                                                              \
    case 0xDA:                                                                \
      /* BGE label */                                                         \
      thumb_conditional_branch(ge);                                           \
      break;                                                                  \
                                                                              \
    case 0xDB:                                                                \
      /* BLT label */                                                         \
      thumb_conditional_branch(lt);                                           \
      break;                                                                  \
                                                                              \
    case 0xDC:                                                                \
      /* BGT label */                                                         \
      thumb_conditional_branch(gt);                                           \
      break;                                                                  \
                                                                              \
    case 0xDD:                                                                \
      /* BLE label */                                                         \
      thumb_conditional_branch(le);                                           \
      break;                                                                  \
                                                                              \
    case 0xDE:                                                                \
      /* B label */                                                         \
      thumb_b();                                           \
      break;                                                                  \
                                                                              \
    case 0xDF:                                                                \
    {                                                                         \
      /* SWI comment */                                                       \
      thumb_swi();                                                            \
      break;                                                                  \
    }                                                                         \
                                                                              \
    case 0xE0 ... 0xE7:                                                       \
    {                                                                         \
      /* B label */                                                           \
      thumb_b();                                                              \
      break;                                                                  \
    }                                                                         \
                                                                              \
    case 0xE8 ... 0xEF:                                                       \
    {                                                                         \
    	if((last_opcode >= 0xF000) && (last_opcode < 0xF800))                   \
    	      {                                                                       \
    	        thumb_blx_imm();                                                          \
    	      }                                                                       \
else\
{\
	LOGE("ruh roh pc %x opcode %x (%x)", pc, opcode, dynarec_proc); \
	/*exit(0);*/\
	thumb_blh();\
}\
    }                                                                         \
                                                                              \
    case 0xF0 ... 0xF7:                                                       \
    {                                                                         \
      /* (low word) BL label */                                               \
      /* This should possibly generate code if not in conjunction with a BLH  \
         next, but I don't think anyone will do that. */                      \
      break;                                                                  \
    }                                                                         \
                                                                              \
    case 0xF8 ... 0xFF:                                                       \
    {                                                                         \
      /* (high word) BL label */                                              \
      /* This might not be preceeding a BL low word (Golden Sun 2), if so     \
         it must be handled like an indirect branch. */                       \
      if((last_opcode >= 0xF000) && (last_opcode < 0xF800))                   \
      {                                                                       \
        thumb_bl();                                                           \
      }                                                                       \
      else                                                                    \
      {                                                                       \
        thumb_blh();                                                          \
      }                                                                       \
      break;                                                                  \
    }                                                                         \
  }                                                                           \
                                                                              \
  pc += 2                                                                     \

#define thumb_flag_modifies_all()                                             \
  flag_status |= 0xFF                                                         \

#define thumb_flag_modifies_zn()                                              \
  flag_status |= 0xCC                                                         \

#define thumb_flag_modifies_znc()                                             \
  flag_status |= 0xEE                                                         \

#define thumb_flag_modifies_zn_maybe_c()                                      \
  flag_status |= 0xCE                                                         \

#define thumb_flag_modifies_c()                                               \
  flag_status |= 0x22                                                         \

#define thumb_flag_requires_c()                                               \
  flag_status |= 0x200                                                        \

#define thumb_flag_requires_all()                                             \
  flag_status |= 0xF00                                                        \

#define thumb_flag_status()                                                   \
{                                                                             \
  u16 flag_status = 0;                                                        \
  switch((opcode >> 8) & 0xFF)                                                \
  {                                                                           \
    /* left shift by imm */                                                   \
    case 0x00 ... 0x07:                                                       \
      thumb_flag_modifies_zn();                                               \
      if(((opcode >> 6) & 0x1F) != 0)                                         \
      {                                                                       \
        thumb_flag_modifies_c();                                              \
      }                                                                       \
      break;                                                                  \
                                                                              \
    /* right shift by imm */                                                  \
    case 0x08 ... 0x17:                                                       \
      thumb_flag_modifies_znc();                                              \
      break;                                                                  \
                                                                              \
    /* add, subtract */                                                       \
    case 0x18 ... 0x1F:                                                       \
      thumb_flag_modifies_all();                                              \
      break;                                                                  \
                                                                              \
    /* mov reg, imm */                                                        \
    case 0x20 ... 0x27:                                                       \
      thumb_flag_modifies_zn();                                               \
      break;                                                                  \
                                                                              \
    /* cmp reg, imm; add, subtract */                                         \
    case 0x28 ... 0x3F:                                                       \
      thumb_flag_modifies_all();                                              \
      break;                                                                  \
                                                                              \
    case 0x40:                                                                \
      switch((opcode >> 6) & 0x03)                                            \
      {                                                                       \
        case 0x00:                                                            \
          /* AND rd, rs */                                                    \
          thumb_flag_modifies_zn();                                           \
          break;                                                              \
                                                                              \
        case 0x01:                                                            \
          /* EOR rd, rs */                                                    \
          thumb_flag_modifies_zn();                                           \
          break;                                                              \
                                                                              \
        case 0x02:                                                            \
          /* LSL rd, rs */                                                    \
          thumb_flag_modifies_zn_maybe_c();                                   \
          break;                                                              \
                                                                              \
        case 0x03:                                                            \
          /* LSR rd, rs */                                                    \
          thumb_flag_modifies_zn_maybe_c();                                   \
          break;                                                              \
      }                                                                       \
      break;                                                                  \
                                                                              \
    case 0x41:                                                                \
      switch((opcode >> 6) & 0x03)                                            \
      {                                                                       \
        case 0x00:                                                            \
          /* ASR rd, rs */                                                    \
          thumb_flag_modifies_zn_maybe_c();                                   \
          break;                                                              \
                                                                              \
        case 0x01:                                                            \
          /* ADC rd, rs */                                                    \
          thumb_flag_modifies_all();                                          \
          thumb_flag_requires_c();                                            \
          break;                                                              \
                                                                              \
        case 0x02:                                                            \
          /* SBC rd, rs */                                                    \
          thumb_flag_modifies_all();                                          \
          thumb_flag_requires_c();                                            \
          break;                                                              \
                                                                              \
        case 0x03:                                                            \
          /* ROR rd, rs */                                                    \
          thumb_flag_modifies_zn_maybe_c();                                   \
          break;                                                              \
      }                                                                       \
      break;                                                                  \
                                                                              \
    /* TST, NEG, CMP, CMN */                                                  \
    case 0x42:                                                                \
      thumb_flag_modifies_all();                                              \
      break;                                                                  \
                                                                              \
    /* ORR, MUL, BIC, MVN */                                                  \
    case 0x43:                                                                \
      thumb_flag_modifies_zn();                                               \
      break;                                                                  \
                                                                              \
    case 0x45:                                                                \
      /* CMP rd, rs */                                                        \
      thumb_flag_modifies_all();                                              \
      break;                                                                  \
                                                                              \
    /* mov might change PC (fall through if so) */                            \
    case 0x46:                                                                \
      if((opcode & 0xFF87) != 0x4687)                                         \
        break;                                                                \
                                                                              \
    /* branches (can change PC) */                                            \
    case 0x47:                                                                \
    case 0xBD:                                                                \
    case 0xD0 ... 0xE7:                                                       \
    case 0xF0 ... 0xFF:                                                       \
      thumb_flag_requires_all();                                              \
      break;                                                                  \
  }                                                                           \
  block_data[block_data_position].flag_data = flag_status;                    \
}                                                                             \

u8 *ram_block_ptrs[1024 * 64];
u32 ram_block_tag_top = 0x0101;

u8 *bios_block_ptrs[1024 * 8];
u32 bios_block_tag_top = 0x0101;

//begin page table stuff

u32** dynarec_page_table[DYNAREC_PAGE_TABLE_SIZE];

void write_smc_check(u32 adr)
{
//	LOGE("write_smc_check(%x)", adr);

	u32** page = dynarec_page_table[(adr & 0x0FFFFFFF)>>12];
	if(!page)
		return;

	u32* trans_adr = page[(adr & 0xFFF)>>1];
	if(!trans_adr)
		return;

	LOGE("self modifying code detected");
	dynarec_cpu->R[CHANGED_PC_STATUS]=1;
	//hack but I dont think a block will go a whole page
	/*free((void*)(dynarec_page_table[idx]));
	dynarec_page_table[idx] = 0;

	if(idx > 0 && !dynarec_page_table[idx-1])
	{
		free((void*)(dynarec_page_table[idx-1]));
		dynarec_page_table[idx-1]=0;
	}

	if(idx < DYNAREC_PAGE_TABLE_SIZE-1 && !dynarec_page_table[idx+1])
	{
		free((void*)(dynarec_page_table[idx+1]));
		dynarec_page_table[idx+1]=0;
	}*/
	flush_translation_cache_rom();
}
//end page table stuff

// This function will return a pointer to a translated block of code. If it
// doesn't exist it will translate it, if it does it will pass it back.

// type should be "arm", "thumb", or "dual." For arm or thumb the PC should
// be a real PC, for dual the least significant bit will determine if it's
// ARM or Thumb mode.

#define block_lookup_address_pc_arm()                                         \
  pc &= ~0x03

#define block_lookup_address_pc_thumb()                                       \
  pc &= ~0x01                                                                 \

#define block_lookup_address_pc_dual()                                        \
  u32 thumb = pc & 0x01;                                                      \
                                                                              \
  if(thumb)                                                                   \
  {                                                                           \
    pc--;                                                                     \
    dynarec_cpu->R[REG_CPSR] |= 0x20;                                                    \
  }                                                                           \
  else                                                                        \
  {                                                                           \
    pc = (pc + 2) & ~0x03;                                                    \
    dynarec_cpu->R[REG_CPSR] &= ~0x20;                                                   \
  }                                                                           \

#define ram_translation_region  TRANSLATION_REGION_RAM
#define rom_translation_region  TRANSLATION_REGION_ROM
#define bios_translation_region TRANSLATION_REGION_BIOS

#define block_lookup_translate_arm(mem_type, smc_enable)                      \
  translation_result = translate_block_arm(pc, mem_type##_translation_region, \
   smc_enable)                                                                \

#define block_lookup_translate_thumb(mem_type, smc_enable)                    \
  translation_result = translate_block_thumb(pc,                              \
   mem_type##_translation_region, smc_enable)                                 \

#define block_lookup_translate_dual(mem_type, smc_enable)                     \
  if(thumb)                                                                   \
  {                                                                           \
    translation_result = translate_block_thumb(pc,                            \
     mem_type##_translation_region, smc_enable);                              \
  }                                                                           \
  else                                                                        \
  {                                                                           \
    translation_result = translate_block_arm(pc,                              \
     mem_type##_translation_region, smc_enable);                              \
  }                                                                           \

// 0x0101 is the smallest tag that can be used. 0xFFFF is marked
// in the middle of blocks and used for write guarding, it doesn't
// indicate a valid block either (it's okay to compile a new block
// that overlaps the earlier one, although this should be relatively
// uncommon)

#define fill_tag_arm(mem_type)                                                \
  location[0] = mem_type##_block_tag_top;                                     \
  location[1] = 0xFFFF                                                        \

#define fill_tag_thumb(mem_type)                                              \
  *location = mem_type##_block_tag_top                                        \

#define fill_tag_dual(mem_type)                                               \
  if(thumb)                                                                   \
    fill_tag_thumb(mem_type);                                                 \
  else                                                                        \
    fill_tag_arm(mem_type)                                                    \

#define block_lookup_translate(instruction_type, mem_type, smc_enable)        \
  block_tag = *location;                                                      \
  if((block_tag < 0x0101) || (block_tag == 0xFFFF))                           \
  {                                                                           \
    __label__ redo;                                                           \
    s32 translation_result;                                                   \
                                                                              \
    redo:                                                                     \
                                                                              \
    translation_recursion_level++;                                            \
    block_address = mem_type##_translation_ptr + block_prologue_size;         \
    mem_type##_block_ptrs[mem_type##_block_tag_top] = block_address;          \
    fill_tag_##instruction_type(mem_type);                                    \
    mem_type##_block_tag_top++;                                               \
                                                                              \
    block_lookup_translate_##instruction_type(mem_type, smc_enable);          \
    translation_recursion_level--;                                            \
                                                                              \
    /* If the translation failed then pass that failure on if we're in        \
       a recursive level, or try again if we've hit the bottom. */            \
    if(translation_result == -1)                                              \
    {                                                                         \
      if(translation_recursion_level)                                         \
        return NULL;                                                          \
                                                                              \
      goto redo;                                                              \
    }                                                                         \
                                                                              \
    if(translation_recursion_level == 0)                                      \
      translate_invalidate_dcache();                                          \
  }                                                                           \
  else                                                                        \
  {                                                                           \
    block_address = mem_type##_block_ptrs[block_tag];                         \
  }                                                                           \
     /* u32 hash_target = ((pc * 2654435761U) >> 16) &                          \
	          (ROM_BRANCH_HASH_SIZE - 1);                                            \
	     u32 *block_ptr = rom_branch_hash[hash_target];                          \
	     u32 **block_ptr_address = rom_branch_hash + hash_target;                \
	                                                                                 \
	          while(block_ptr)                                                        \
	           {                                                                       \
	             if(block_ptr[0] == pc)                                                \
	           {                                                                     \
	                block_address = (u8 *)(block_ptr + 2) + block_prologue_size;        \
	                 break;                                                              \
	                 }                                                                     \
	                                                                                     \
	                   block_ptr_address = (u32 **)(block_ptr + 1);                          \
	                    block_ptr = (u32 *)block_ptr[1];                                      \
	                  }                                                                       \*/
u32 translation_recursion_level = 0;
u32 translation_flush_count = 0;

#define block_lookup_address_builder(type)                                    \
__attribute__((noinline)) u8 function_cc *block_lookup_address_##type(u32 pc)                           \
{                                                                             \
  u16 *location;                                                              \
  u32 block_tag;                                                              \
  u8 *block_address;                                                          \
                                                                              \
  /* Starting at the beginning, we allow for one translation cache flush. */  \
  if(translation_recursion_level == 0)                                        \
    translation_flush_count = 0;                                              \
  block_lookup_address_pc_##type();                                           \
                                                                              \
  switch(pc >> 24)                                                            \
  {                                                                           \
    case 0x0 ... 0xE:                                                         \
    {                                                                         \
                                                                              \
      	        u32 *block_ptr = retrieve_adr(pc);                          \
	        u32** block_ptr_address = &block_ptr;               \
	           block_address = (u8 *)(block_ptr + 2) + block_prologue_size;        \
	           \
      if(block_ptr == NULL|| block_ptr == reinterpret_cast<u32*>(0xFFFFFFFF) || *block_ptr != pc)                                                   \
      {                                                                       \
        __label__ redo;                                                       \
        s32 translation_result;                                               \
                                                                              \
        redo:                                                                 \
                                                                              \
        translation_recursion_level++;                                        \
        ((u32 *)rom_translation_ptr)[0] = pc;                                 \
        ((u32 **)rom_translation_ptr)[1] = NULL;                              \
        *block_ptr_address = (u32 *)rom_translation_ptr;                      \
        rom_translation_ptr += 8;                                             \
        block_address = rom_translation_ptr + block_prologue_size;            \
        block_lookup_translate_##type(rom, 0);                                \
        translation_recursion_level--;                                        \
                                                                              \
        /* If the translation failed then pass that failure on if we're in    \
         a recursive level, or try again if we've hit the bottom. */          \
        if(translation_result == -1)                                          \
        {                                                                     \
          if(translation_recursion_level)                                     \
            return NULL;                                                      \
                                                                              \
          goto redo;                                                          \
        }                                                                     \
                                                                              \
        if(translation_recursion_level == 0)                                  \
          translate_invalidate_dcache();                                      \
      }                                                                       \
      break;                                                                  \
    }                                                                         \
\
    case 0xFF:\
    {\
	        u32 *block_ptr = retrieve_adr(pc);                          \
    u32** block_ptr_address = &block_ptr;               \
       block_address = (u8 *)(block_ptr + 2) + block_prologue_size;        \
       \
		if(block_ptr == NULL|| block_ptr == reinterpret_cast<u32*>(0xFFFFFFFF) || *block_ptr != pc)                                                   \
		{                                                                       \
		__label__ redo;                                                       \
		s32 translation_result;                                               \
																			  \
		redo:                                                                 \
																			  \
		translation_recursion_level++;                                        \
		((u32 *)bios_translation_ptr)[0] = pc;                                 \
		((u32 **)bios_translation_ptr)[1] = NULL;                              \
		*block_ptr_address = (u32 *)bios_translation_ptr;                      \
		bios_translation_ptr += 8;                                             \
		block_address = bios_translation_ptr + block_prologue_size;            \
		block_lookup_translate_##type(bios, 0);                                \
		translation_recursion_level--;                                        \
																			  \
		/* If the translation failed then pass that failure on if we're in    \
		 a recursive level, or try again if we've hit the bottom. */          \
		if(translation_result == -1)                                          \
		{                                                                     \
		  if(translation_recursion_level)                                     \
			return NULL;                                                      \
																			  \
		  goto redo;                                                          \
		}                                                                     \
																			  \
		if(translation_recursion_level == 0)                                  \
		  translate_invalidate_dcache();                                      \
		}                                                                       \
		break;                                                                  \
    }\
                                                                              \
    default:                                                                  \
      /* If we're at the bottom, it means we're actually trying to jump to an \
         address that we can't handle. Otherwise, it means that code scanned  \
         has reached an address that can't be handled, which means that we    \
         have most likely hit an area that doesn't contain code yet (for      \
         instance, in RAM). If such a thing happens, return -1 and the        \
         block translater will naively link it (it'll be okay, since it       \
         should never be hit) */                                              \
      if(translation_recursion_level == 0)                                    \
      {                                                                       \
        LOGE("bad jump %x (%x) (%x)", pc, dynarec_cpu->R[REG_PC],           \
         last_instruction);                                                   \
        sleep(10);\
        exit(1);                                                               \
      }                                                                       \
      block_address = (u8 *)(-1);                                             \
      break;                                                                  \
  }                                                                           \
  return block_address;                                                       \
}                                                                             \


__attribute__((noinline)) u8 function_cc *block_lookup_address_arm(u32 pc)
{
  u16 *location;
  u32 block_tag;
  u8 *block_address;

  /* Starting at the beginning, we allow for one translation cache flush. */
  if(translation_recursion_level == 0)
    translation_flush_count = 0;
  block_lookup_address_pc_arm();

  switch(pc >> 24)
  {
    case 0x0 ... 0xE:
    {

      	        u32 *block_ptr = retrieve_adr(pc);
	        u32** block_ptr_address = &block_ptr;
	           block_address = (u8 *)(block_ptr + 2) + block_prologue_size;

      if(block_ptr == NULL|| block_ptr == reinterpret_cast<u32*>(0xFFFFFFFF) || *block_ptr != pc)
      {
	  
        __label__ redo;
        s32 translation_result;
		
		/*if(block_ptr == NULL)
		{
			__android_log_print(ANDROID_LOG_INFO,"nds4droid","new arm rom block, pc %x", (unsigned long)pc);
		}
		else if(*block_ptr != pc)
		{
			__android_log_print(ANDROID_LOG_INFO,"nds4droid","arm rom block collision, pc %x has entry %x", (unsigned long)pc, (unsigned long)*block_ptr);
		}*/

        redo:

        translation_recursion_level++;
        ((u32 *)rom_translation_ptr)[0] = pc;
        ((u32 **)rom_translation_ptr)[1] = NULL;
        *block_ptr_address = (u32 *)rom_translation_ptr;
        rom_translation_ptr += 8;
        block_address = rom_translation_ptr + block_prologue_size;
        block_lookup_translate_arm(rom, 0);
        translation_recursion_level--;

        /* If the translation failed then pass that failure on if we're in
         a recursive level, or try again if we've hit the bottom. */
        if(translation_result == -1)
        {
          if(translation_recursion_level)
            return NULL;

          goto redo;
        }

        if(translation_recursion_level == 0)
          translate_invalidate_dcache();
      }
	  /*else
	  {
			__android_log_print(ANDROID_LOG_INFO,"nds4droid","existing arm rom block found, pc %x", (unsigned long)pc);
	  }*/
      break;
    }

    case 0xFF:
    {
    	 u32 *block_ptr = retrieve_adr(pc);
    	    u32** block_ptr_address = &block_ptr;
    	       block_address = (u8 *)(block_ptr + 2) + block_prologue_size;

    			if(block_ptr == NULL|| block_ptr == reinterpret_cast<u32*>(0xFFFFFFFF) || *block_ptr != pc)
    			{
    			__label__ redo;
    			s32 translation_result;
				
				/*if(block_ptr == NULL)
				{
					__android_log_print(ANDROID_LOG_INFO,"nds4droid","new arm bios block, pc %x", (unsigned long)pc);
				}
				else if(*block_ptr != pc)
				{
					__android_log_print(ANDROID_LOG_INFO,"nds4droid","arm bios block collision, pc %x has entry %x at %x", (unsigned long)pc, (unsigned long)*block_ptr, (unsigned long)block_ptr);
				}*/

    			redo:

    			translation_recursion_level++;
    			((u32 *)bios_translation_ptr)[0] = pc;
    			((u32 **)bios_translation_ptr)[1] = NULL;
    			*block_ptr_address = (u32 *)bios_translation_ptr;
    			bios_translation_ptr += 8;
    			block_address = bios_translation_ptr + block_prologue_size;
    			block_lookup_translate_arm(bios, 0);
    			translation_recursion_level--;

    			/* If the translation failed then pass that failure on if we're in
    			 a recursive level, or try again if we've hit the bottom. */
    			if(translation_result == -1)
    			{
    			  if(translation_recursion_level)
    				return NULL;

    			  goto redo;
    			}

    			if(translation_recursion_level == 0)
    			  translate_invalidate_dcache();
    			}
				/*else
				{
						__android_log_print(ANDROID_LOG_INFO,"nds4droid","existing arm bios block found, pc %x", (unsigned long)pc);
				}*/
    			break;
    }

    default:
      /* If we're at the bottom, it means we're actually trying to jump to an
         address that we can't handle. Otherwise, it means that code scanned
         has reached an address that can't be handled, which means that we
         have most likely hit an area that doesn't contain code yet (for
         instance, in RAM). If such a thing happens, return -1 and the
         block translater will naively link it (it'll be okay, since it
         should never be hit) */
      if(translation_recursion_level == 0)
      {
    	  LOGE("bad jump %x (%x) (%x)", pc, dynarec_cpu->R[REG_PC],
    	           last_instruction);
        sleep(10);
        exit(1);
      }
      block_address = (u8 *)(-1);
      break;
  }
  return block_address;
}

;
block_lookup_address_builder(thumb)
;
block_lookup_address_builder(dual)
;

// Potential exit point: If the rd field is pc for instructions is 0x0F,
// the instruction is b/bl/bx, or the instruction is ldm with PC in the
// register list.
// All instructions with upper 3 bits less than 100b have an rd field
// except bx, where the bits must be 0xF there anyway, multiplies,
// which cannot have 0xF in the corresponding fields, and msr, which
// has 0x0F there but doesn't end things (therefore must be special
// checked against). Because MSR and BX overlap both are checked for.

#define arm_exit_point                                                        \
 (((opcode < 0x8000000) && ((opcode & 0x000F000) == 0x000F000) &&             \
  ((opcode & 0xDB0F000) != 0x120F000)) ||                                     \
  ((opcode & 0x12FFF10) == 0x12FFF10) ||                                      \
  ((opcode & 0x8108000) == 0x8108000) ||                                      \
  ((opcode >= 0xA000000) && (opcode < 0xC000000)) ||\
  ((opcode >= 0xC000000) && (opcode < 0xEF00000) && (((opcode >> 12) & 0x0F) == 0x0F)) ||\
	((opcode & 0xF0000000) == 0xF0000000))\

#define arm_opcode_branch                                                     \
  ((opcode & 0xE000000) == 0xA000000)                                         \

#define arm_opcode_swi                                                        \
  ((opcode & 0xF000000) == 0xF000000)                                         \

#define arm_opcode_unconditional_branch                                       \
  (condition == 0x0E || condition == 0x0F)                                                         \

#define arm_load_opcode()                                                     \
		opcode = _MMU_read32(dynarec_proc, MMU_AT_CODE, block_end_pc);\
		        condition = opcode >> 28;\
		        opcode &= 0xFFFFFFF;\
		        block_end_pc += 4;\

#define arm_branch_target()                                                   \
	if(condition == 0x0F)  \
	branch_target = ((block_end_pc + 4 + (((s32)(opcode & 0xFFFFFF) << 8) >> 6)) | 0x00000001 ) | (((opcode >> 24) & 0x01)<<1);   \
else    \
  branch_target = (block_end_pc + 4 + (((s32)(opcode & 0xFFFFFF) << 8) >> 6)); \

// Contiguous conditional block flags modification - it will set 0x20 in the
// condition's bits if this instruction modifies flags. Taken from the CPU
// switch so it'd better be right this time.

#define arm_set_condition(_condition)                                         \
  block_data[block_data_position].condition = _condition;                     \
  switch((opcode >> 20) & 0xFF)                                               \
  {                                                                           \
    case 0x01:                                                                \
    case 0x03:                                                                \
    case 0x09:                                                                \
    case 0x0B:                                                                \
    case 0x0D:                                                                \
    case 0x0F:                                                                \
      if((((opcode >> 5) & 0x03) == 0) || ((opcode & 0x90) != 0x90))          \
        block_data[block_data_position].condition |= 0x20;                    \
      break;                                                                  \
                                                                              \
    case 0x05:                                                                \
    case 0x07:                                                                \
    case 0x11:                                                                \
    case 0x13:                                                                \
    case 0x15 ... 0x17:                                                       \
    case 0x19:                                                                \
    case 0x1B:                                                                \
    case 0x1D:                                                                \
    case 0x1F:                                                                \
      if((opcode & 0x90) != 0x90)                                             \
        block_data[block_data_position].condition |= 0x20;                    \
      break;                                                                  \
                                                                              \
    case 0x12:                                                                \
      if(((opcode & 0x90) != 0x90) && !(opcode & 0x10))                       \
        block_data[block_data_position].condition |= 0x20;                    \
      break;                                                                  \
                                                                              \
    case 0x21:                                                                \
    case 0x23:                                                                \
    case 0x25:                                                                \
    case 0x27:                                                                \
    case 0x29:                                                                \
    case 0x2B:                                                                \
    case 0x2D:                                                                \
    case 0x2F ... 0x37:                                                       \
    case 0x39:                                                                \
    case 0x3B:                                                                \
    case 0x3D:                                                                \
    case 0x3F:                                                                \
      block_data[block_data_position].condition |= 0x20;                      \
    break;                                                                    \
  }                                                                           \

#define arm_link_block()                                                      \
{	if((branch_target & 0x1) != 0x1)\
  {translation_target = block_lookup_address_arm(branch_target);     }           \
else \
{translation_target = block_lookup_address_thumb((branch_target & 0xFFFFFFFE));}}\

#define arm_instruction_width 4

#define arm_base_cycles()                                                     \

// For now this just sets a variable that says flags should always be
// computed.

#define arm_dead_flag_eliminate()                                             \
  flag_status = 0xF                                                           \

// The following Thumb instructions can exit:
// b, bl, bx, swi, pop {... pc}, and mov pc, ..., the latter being a hireg
// op only. Rather simpler to identify than the ARM set.

#define thumb_exit_point                                                      \
  (((opcode >= 0xD000) && (opcode < 0xDF00)) ||                                      \
   ((opcode >= 0xE000) && (opcode < 0xF000)) ||                               \
   ((opcode & 0xFF00) == 0x4700) ||                                           \
   ((opcode & 0xFF00) == 0xBD00) ||                                           \
   ((opcode & 0xFF87) == 0x4687) ||                                           \
   (opcode >= 0xF800))                                                      \

#define thumb_opcode_branch                                                   \
  (((opcode >= 0xD000) && (opcode < 0xDF00)) ||                               \
   ((opcode >= 0xE000) && (opcode < 0xF000)) ||                               \
   (opcode >= 0xF800))                                                        \

#define thumb_opcode_swi                                                      \
  ((opcode & 0xFF00) == 0xDF00)                                               \

#define thumb_opcode_unconditional_branch                                     \
  ((opcode < 0xD000) || (opcode >= 0xDF00))                                   \

#define thumb_load_opcode()                                                   \
  last_opcode = opcode;                                                       \
	opcode = _MMU_read16(dynarec_proc, MMU_AT_CODE, block_end_pc);\
  block_end_pc += 2                                                           \

#define thumb_branch_target()                                                 \
  if(opcode < 0xE000)                                                         \
  {                                                                           \
    branch_target = block_end_pc + 2 + ((s8)(opcode & 0xFF) * 2);             \
  }                                                                           \
  else                                                                        \
                                                                              \
  if((opcode < 0xF800 && opcode >= 0xF000)|| opcode < 0xE800)                                                         \
  {                                                                           \
    branch_target = block_end_pc + 2 + ((s32)((opcode & 0x7FF) << 21) >> 20); \
  }                                                                           \
  else                                                                        \
  {                                                                           \
    if((last_opcode >= 0xF000) && (last_opcode < 0xF800))                     \
    {                                                                         \
      if(opcode >= 0xE800 && opcode < 0xF000)\
      {\
	branch_target =                                                         \
       ((block_end_pc + ((s32)((last_opcode & 0x07FF) << 21) >> 9) +           \
       ((opcode & 0x07FF) * 2))& 0xFFFFFFFC) | 0x1; /*sets last bit as blx flag*/ \
      }\
      else\
      {\
    	  branch_target =                                                         \
    	         (block_end_pc + ((s32)((last_opcode & 0x07FF) << 21) >> 9) +           \
    	         ((opcode & 0x07FF) * 2));                                              \
      }\
    }                                                                         \
    else                                                                      \
    {                                                                         \
      goto no_direct_branch;                                                  \
    }                                                                         \
  }                                                                           \

#define thumb_set_condition(_condition)                                       \

#define thumb_link_block()                                                    \
  if(branch_target != 0x00000008 && ((branch_target & 0x1) != 0x1)) /*check last bit to see if blx flag set*/                                            \
    translation_target = block_lookup_address_thumb(branch_target);           \
  else                                                                        \
    translation_target = block_lookup_address_arm((branch_target & 0xFFFFFFFE))              \

#define thumb_instruction_width 2

#define thumb_base_cycles()                                                   \
  //if(!(block_data_position & 0x02))   \
  //	  cycle_count += MMU_codeFetchCycles<proc,32>(pc);  \

// Here's how this works: each instruction has three different sets of flag
// attributes, each consisiting of a 4bit mask describing how that instruction
// interacts with the 4 main flags (N/Z/C/V).
// The first set, in bits 0:3, is the set of flags the instruction may
// modify. After this pass this is changed to the set of flags the instruction
// should modify - if the bit for the corresponding flag is not set then code
// does not have to be generated to calculate the flag for that instruction.

// The second set, in bits 7:4, is the set of flags that the instruction must
// modify (ie, for shifts by the register values the instruction may not
// always modify the C flag, and thus the C bit won't be set here).

// The third set, in bits 11:8, is the set of flags that the instruction uses
// in its computation, or the set of flags that will be needed after the
// instruction is done. For any instructions that change the PC all of the
// bits should be set because it is (for now) unknown what flags will be
// needed after it arrives at its destination. Instructions that use the
// carry flag as input will have it set as well.

// The algorithm is a simple liveness analysis procedure: It starts at the
// bottom of the instruction stream and sets a "currently needed" mask to
// the flags needed mask of the current instruction. Then it moves down
// an instruction, ANDs that instructions "should generate" mask by the
// "currently needed" mask, then ANDs the "currently needed" mask by
// the 1's complement of the instruction's "must generate" mask, and ORs
// the "currently needed" mask by the instruction's "flags needed" mask.

#define thumb_dead_flag_eliminate()                                           \
{                                                                             \
  u32 needed_mask;                                                            \
  needed_mask = block_data[block_data_position].flag_data >> 8;               \
                                                                              \
  block_data_position--;                                                      \
  while(block_data_position >= 0)                                             \
  {                                                                           \
    flag_status = block_data[block_data_position].flag_data;                  \
    block_data[block_data_position].flag_data =                               \
     (flag_status & needed_mask);                                             \
    needed_mask &= ~((flag_status >> 4) & 0x0F);                              \
    needed_mask |= flag_status >> 8;                                          \
    block_data_position--;                                                    \
  }                                                                           \
}                                                                             \

#define MAX_BLOCK_SIZE 8192
#define MAX_EXITS      256

block_data_type block_data[8192];
block_exit_type block_exits[MAX_EXITS];

#define smc_write_arm_yes()                                                   \
  if((ds_read32(block_end_pc) - 0x8000) == 0x0000) \
  {                                                                           \
	  ds_write32(block_end_pc - 0x8000, 0xFFFFFFFF);                             \
  }                                                                           \

#define smc_write_thumb_yes()                                                 \
  if((ds_read16(block_end_pc) - 0x8000) == 0x0000) \
  {                                                                           \
    ds_write16(block_end_pc- 0x8000, 0xFFFF);   \
  }                                                                           \

#define smc_write_arm_no()                                                    \

#define smc_write_thumb_no()                                                  \

#define scan_block(type, smc_write_op)                                        \
{                                                                             \
  __label__ block_end;                                                        \
  /* Find the end of the block */                                             \
  do                                                                          \
  {                                                                           \
    check_pc_region(block_end_pc);                                            \
    smc_write_##type##_##smc_write_op();                                      \
    type##_load_opcode();                                                     \
    type##_flag_status();                                                     \
                                                                              \
    if(type##_exit_point)                                                     \
    {                                                                         \
      /* Branch/branch with link */                                           \
      if(type##_opcode_branch)                                                \
      {                                                                       \
        __label__ no_direct_branch;                                           \
        type##_branch_target();                                               \
        block_exits[block_exit_position].branch_target = branch_target;       \
        block_exit_position++;                                                \
                                                                              \
        /* Give the branch target macro somewhere to bail if it turns out to  \
           be an indirect branch (ala malformed Thumb bl) */                  \
        no_direct_branch:;                                                    \
      }                                                                       \
                                                                              \
      /* SWI branches to the BIOS, this will likely change when               \
         some HLE BIOS is implemented.CHANGED :D */                                     \
                                                                              \
      type##_set_condition(condition | 0x10);                                 \
                                                                              \
      /* Only unconditional branches can end the block. */                    \
      if(type##_opcode_unconditional_branch)                                  \
      {                                                                       \
        /* Check to see if any prior block exits branch after here,           \
           if so don't end the block. Starts from the top and works           \
           down because the most recent branch is most likely to              \
           join after the end (if/then form) */                               \
        for(i = block_exit_position - 2; i >= 0; i--)                         \
        {                                                                     \
          if(block_exits[i].branch_target == block_end_pc)                    \
            break;                                                            \
        }                                                                     \
                                                                              \
        if(i < 0)                                                             \
          break;                                                              \
      }                                                                       \
      if(block_exit_position == MAX_EXITS)                                    \
        break;                                                                \
    }                                                                         \
    else                                                                      \
    {                                                                         \
      type##_set_condition(condition);                                        \
    }                                                                         \
                                                                              \
    for(i = 0; i < translation_gate_targets; i++)                             \
    {                                                                         \
      if(block_end_pc == translation_gate_target_pc[i])                       \
        goto block_end;                                                       \
    }                                                                         \
                                                                              \
    block_data[block_data_position].update_cycles = 0;                        \
    block_data_position++;                                                    \
    if((block_data_position == MAX_BLOCK_SIZE) ||                             \
     (block_end_pc == 0x3007FF0) || (block_end_pc == 0x203FFFF0))			  \
    {                                                                         \
      break;                                                                  \
    }                                                                         \
  } while(1);                                                                 \
                                                                              \
  block_end:;                                                                 \
}                                                                             \

#define arm_fix_pc()                                                          \
  pc &= ~0x03                                                                 \

#define thumb_fix_pc()                                                        \
  pc &= ~0x01                                                                 \

bool enable_instruct_debug=false;

u32 cycle_counter_arm9=0;
bool block_print =false;

__attribute__((noinline)) s32 translate_block_arm(u32 pc, translation_region_type translation_region,
		u32 smc_enable) {
#define generate_indirect_branch_arm()                                         \
  ({                                                                           \
    if(condition == 0x0E)                                                     \
    {                                                                         \
      generate_cycle_update();                                                \
    }                                                                         \
    generate_indirect_branch_no_cycle_update(arm);                            \
  })                                                                           \

#define generate_indirect_branch_dual()                                        \
  ({                                                                           \
    if(condition == 0x0E)                                                     \
    {                                                                         \
      generate_cycle_update();                                                \
    }                                                                         \
    generate_indirect_branch_no_cycle_update(dual_arm);                       \
  })                                                                           \
                                                                              \

#define prepare_load_reg(scratch_reg, reg_index)                        \
  ({                                                                           \
	  u32 out;\
    u32 reg_use = arm_register_allocation[reg_index];                         \
    if(reg_use == mem_reg)                                                    \
    {                                                                         \
      ARM_LDR_IMM(0, scratch_reg, reg_base,                                   \
       (reg_base_offset + (reg_index * 4)));                                  \
      out=scratch_reg;                                                     \
    }                                                                         \
    else                                                                          \
    {\
    	out= reg_use;                                                           \
    }\
    out;\
  })                                                                           \
                                                                              \

#define prepare_load_reg_pc(scratch_reg, reg_index, pc_offset)      \
  ({                                                                           \
	u32 out;\
    if(reg_index == 15)                                                       \
    {                                                                         \
      generate_load_pc(scratch_reg, pc + pc_offset);                          \
      out= scratch_reg;                                                     \
    }                                                                         \
    else\
    {\
    	out=prepare_load_reg(scratch_reg, reg_index);                          \
    }\
    out;\
  })                                                                           \
                                                                              \

#define prepare_store_reg(scratch_reg, reg_index)                       \
  ({                                                                           \
    u32 reg_use = arm_register_allocation[reg_index];                         \
    reg_use==mem_reg ? scratch_reg : reg_use; \
  })                                                                           \
                                                                              \

#define complete_store_reg(scratch_reg, reg_index)                     \
  ({                                                                           \
    if(arm_register_allocation[reg_index] == mem_reg)                         \
    {                                                                         \
      ARM_STR_IMM(0, scratch_reg, reg_base,                                   \
       (reg_base_offset + (reg_index * 4)));                                  \
    }                                                                         \
  })                                                                           \

#define complete_store_reg_pc_no_flags(scratch_reg, reg_index)         \
  ({                                                                           \
    if(reg_index == 15)                                                       \
    {                                                                         \
      generate_indirect_branch_arm();                                         \
    }                                                                         \
    else                                                                      \
    {                                                                         \
      complete_store_reg(scratch_reg, reg_index);                             \
    }                                                                         \
  })                                                                           \
                                                                              \

#define complete_store_reg_pc_flags(scratch_reg, reg_index)            \
  ({                                                                           \
    if(reg_index == 15)                                                       \
    {                                                                         \
      if(condition == 0x0E)                                                   \
      {                                                                       \
        generate_cycle_update();                                              \
      }                                                                       \
      generate_function_call(execute_spsr_restore);                           \
    }                                                                         \
    else                                                                      \
    {                                                                         \
      complete_store_reg(scratch_reg, reg_index);                             \
    }                                                                         \
  })                                                                           \
                                                                              \

#define generate_load_reg(ireg, reg_index)                             \
  ({                                                                           \
    s32 load_src = arm_register_allocation[reg_index];                        \
    if(load_src != mem_reg)                                                   \
    {                                                                         \
      ARM_MOV_REG_REG(0, ireg, load_src);                                     \
    }                                                                         \
    else                                                                      \
    {                                                                         \
      ARM_LDR_IMM(0, ireg, reg_base, (reg_base_offset + (reg_index * 4)));    \
    }                                                                         \
  })                                                                           \
                                                                              \

#define generate_store_reg(ireg, reg_index)                            \
  ({                                                                           \
    s32 store_dest = arm_register_allocation[reg_index];                      \
    if(store_dest != mem_reg)                                                 \
    {                                                                         \
      ARM_MOV_REG_REG(0, store_dest, ireg);                                   \
    }                                                                         \
    else                                                                      \
    {                                                                         \
      ARM_STR_IMM(0, ireg, reg_base, (reg_base_offset + (reg_index * 4)));    \
    }                                                                         \
  })                                                                           \


	u32 opcode = 0;
	u32 last_opcode;
	u32 condition;
	u32 last_condition;
	u32 block_start_pc = pc;
	u32 block_end_pc = pc;
	u32 block_exit_position = 0;
	s32 block_data_position = 0;
	u32 external_block_exit_position = 0;
	u32 branch_target;
	u32 cycle_count = 0;
	u8 *translation_target;
	u8 *backpatch_address = NULL;
	u8 *translation_ptr = NULL;
	u8 *translation_cache_limit = NULL;
	s32 i;
	u32 flag_status;
	u32 exec_cyc;
	u32 cond_pc;
	block_exit_type external_block_exits[MAX_EXITS];
	arm_fix_pc();

	if(pc<0xFFFF0000)
	{
	translation_ptr = rom_translation_ptr;
	translation_cache_limit = rom_translation_cache + ROM_TRANSLATION_CACHE_SIZE
			- TRANSLATION_CACHE_LIMIT_THRESHOLD;
	}
	else
	{

		translation_ptr = bios_translation_ptr;
		translation_cache_limit = bios_translation_cache +
		BIOS_TRANSLATION_CACHE_SIZE;
	}

	generate_block_prologue();

	/* This is a function because it's used a lot more than it might seem (all
	 of the data processing functions can access it), and its expansion was
	 massacreing the compiler. */

	if (smc_enable) {
		scan_block(arm, yes);
	} else {
		scan_block(arm, no);
	}

	for (i = 0; i < block_exit_position; i++) {
		branch_target = block_exits[i].branch_target;

		if ((branch_target > block_start_pc)
				&& (branch_target < block_end_pc)) {
			block_data[(branch_target - block_start_pc) / arm_instruction_width].update_cycles =
					1;
		}
	}

	arm_dead_flag_eliminate();

	block_exit_position = 0;
	block_data_position = 0;

	last_condition = 0x0E;

	/*if((pc & 0xFFFF0000)== 0xFFFF0000)
		block_print=false;
	else*/
	/*if(pc == 0x2005a50)
		block_print=true;
	//if(pc == 0x021c7914)
	//{
	if(dynarec_proc == DEBUG_PROC  && block_print)
		LOGE("pc begin %x recur level %u", pc, translation_recursion_level);*/
	//	//enable_instruct_debug=true;
	//}

	add_adr(pc, (u32*)(translation_ptr - 8 - block_prologue_size));
	while (pc != block_end_pc) {
		//cycle_counter_arm9++;
		/*if(cycle_counter_arm9 <1000)
			LOGE("pc %x cycle %u", pc, cycle_count);
		else
			exit(0);*/
//		LOGE("arm pc %d\n", pc);

		exec_cyc=0;
		block_data[block_data_position].block_offset = translation_ptr;


		arm_base_cycles();
		//generate_step_debug();


		translate_arm_instruction();

		//if(pc != block_end_pc)
	//						add_adr(pc, reinterpret_cast<u32*>(0xFFFFFFFF));


		cycle_count+=exec_cyc;

		block_data_position++;

		/* If it went too far the cache needs to be flushed and the process
		 restarted. Because we might already be nested several stages in
		 a simple recursive call here won't work, it has to pedal out to
		 the beginning. */

		if (translation_ptr > translation_cache_limit) {
			translation_flush_count++;

			if(pc<0xFFFF0000) {

				flush_translation_cache_rom();}
				else
				{
				flush_translation_cache_bios();
			}

			return -1;
		}
		/* If the next instruction is a block entry point update the
		 cycle counter and update */
		if (block_data[block_data_position].update_cycles == 1) {
			generate_cycle_update();
		}
	}
	//if(dynarec_proc ==DEBUG_PROC && block_print)
	//LOGE("arm pc end %d", pc);

	for (i = 0; i < translation_gate_targets; i++) {
		if (pc == translation_gate_target_pc[i]) {
			generate_translation_gate(arm);
			break;
		}
	}

	for (i = 0; i < block_exit_position; i++) {
		branch_target = block_exits[i].branch_target;

		if ((branch_target >= block_start_pc)
				&& (branch_target < block_end_pc)) {
			/* Internal branch, patch to recorded address */
			translation_target = block_data[(branch_target - block_start_pc)
					/ arm_instruction_width].block_offset;

			generate_branch_patch_unconditional(block_exits[i].branch_source,
					translation_target);
		} else {
			/* External branch, save for later */
			external_block_exits[external_block_exit_position].branch_target =
					branch_target;
			external_block_exits[external_block_exit_position].branch_source =
					block_exits[i].branch_source;
			external_block_exit_position++;
		}
	}

	if(pc<0xFFFF0000) {

		rom_translation_ptr = translation_ptr;}
					else
					{
						 bios_translation_ptr = translation_ptr;
				}

	for (i = 0; i < external_block_exit_position; i++) {
		branch_target = external_block_exits[i].branch_target;
		arm_link_block();
		if (translation_target == NULL)
			return -1;
		generate_branch_patch_unconditional(
				external_block_exits[i].branch_source, translation_target);
	}

	return 0;

#undef generate_indirect_branch_arm
#undef generate_indirect_branch_dual
#undef prepare_load_reg
#undef prepare_load_reg_pc
#undef prepare_store_reg
#undef complete_store_reg
#undef complete_store_reg_pc_no_flags
#undef complete_store_reg_pc_flags
#undef generate_load_reg
#undef generate_store_reg
}

__attribute__((noinline)) s32 translate_block_thumb(u32 pc, translation_region_type translation_region,
		u32 smc_enable) {
#define prepare_load_reg(scratch_reg, reg_index)                        \
  ({                                                                           \
	u32 out;																  \
    u32 reg_use = thumb_register_allocation[reg_index];                       \
    if(reg_use == mem_reg)                                                    \
    {                                                                         \
      ARM_LDR_IMM(0, scratch_reg, reg_base,                                   \
       (reg_base_offset + (reg_index * 4)));                                  \
      out=scratch_reg;                                                     \
    }                                                                         \
    else\
    {\
    	out=reg_use;                                                           \
    }\
    out;\
  })                                                                           \
                                                                              \

#define prepare_load_reg_pc(scratch_reg, reg_index, pc_offset)      \
  ({                                                                           \
	  u32 out;\
    if(reg_index == 15)                                                       \
    {                                                                         \
      generate_load_pc(scratch_reg, pc + pc_offset);                          \
      out=scratch_reg;                                                     \
    }\
	else\
	{\
		out=prepare_load_reg(scratch_reg, reg_index);                          \
	}\
    out;\
  })                                                                           \
                                                                              \

#define prepare_store_reg(scratch_reg, reg_index)                       \
  ({                                                                           \
    u32 reg_use = thumb_register_allocation[reg_index];                       \
    reg_use==mem_reg ? scratch_reg : reg_use;\
  })                                                                           \
                                                                              \

#define complete_store_reg(scratch_reg, reg_index)                     \
  ({                                                                           \
    if(thumb_register_allocation[reg_index] == mem_reg)                       \
    {                                                                         \
      ARM_STR_IMM(0, scratch_reg, reg_base,                                   \
       (reg_base_offset + (reg_index * 4)));                                  \
    }                                                                         \
  })                                                                           \
                                                                              \

#define generate_load_reg(ireg, reg_index)                             \
  ({                                                                           \
    s32 load_src = thumb_register_allocation[reg_index];                      \
    if(load_src != mem_reg)                                                   \
    {                                                                         \
      ARM_MOV_REG_REG(0, ireg, load_src);                                     \
    }                                                                         \
    else                                                                      \
    {                                                                         \
      ARM_LDR_IMM(0, ireg, reg_base, (reg_base_offset + (reg_index * 4)));    \
    }                                                                         \
  })                                                                           \
                                                                              \

#define generate_store_reg(ireg, reg_index)                            \
  ({                                                                           \
    s32 store_dest = thumb_register_allocation[reg_index];                    \
    if(store_dest != mem_reg)                                                 \
    {                                                                         \
      ARM_MOV_REG_REG(0, store_dest, ireg);                                   \
    }                                                                         \
    else                                                                      \
    {                                                                         \
      ARM_STR_IMM(0, ireg, reg_base, (reg_base_offset + (reg_index * 4)));    \
    }                                                                         \
  })                                                                           \


	u32 opcode = 0;
	u32 last_opcode;
	u32 condition;
	u32 last_condition;
	u32 block_start_pc = pc;
	u32 block_end_pc = pc;
	u32 block_exit_position = 0;
	s32 block_data_position = 0;
	u32 external_block_exit_position = 0;
	u32 branch_target;
	u32 cycle_count = 0;
	u8 *translation_target;
	u8 *backpatch_address = NULL;
	u8 *translation_ptr = NULL;
	u8 *translation_cache_limit = NULL;
	s32 i;
	u32 flag_status;
	u32 exec_cyc=0;
	block_exit_type external_block_exits[MAX_EXITS];
	thumb_fix_pc();

	if(pc<0xFFFF0000)
		{
		translation_ptr = rom_translation_ptr;
		translation_cache_limit = rom_translation_cache + ROM_TRANSLATION_CACHE_SIZE
				- TRANSLATION_CACHE_LIMIT_THRESHOLD;
		}
		else
		{
			translation_ptr = bios_translation_ptr;
			translation_cache_limit = bios_translation_cache +
			BIOS_TRANSLATION_CACHE_SIZE;
		}

	generate_block_prologue();

	/* This is a function because it's used a lot more than it might seem (all
	 of the data processing functions can access it), and its expansion was
	 massacreing the compiler. */

	if (smc_enable) {
		scan_block(thumb, yes);
	} else {
		scan_block(thumb, no);
	}

	for (i = 0; i < block_exit_position; i++) {
		branch_target = block_exits[i].branch_target;

		if ((branch_target > block_start_pc)
				&& (branch_target < block_end_pc)) {
			block_data[(branch_target - block_start_pc)
					/ thumb_instruction_width].update_cycles = 1;
		}
	}

	thumb_dead_flag_eliminate();

	block_exit_position = 0;
	block_data_position = 0;

	last_condition = 0x0E;
	//if(dynarec_proc == DEBUG_PROC && block_print)
	//LOGE("thumb  pc begin %x recur level %u", pc, translation_recursion_level);

	add_adr(pc, (u32*)(translation_ptr - 8 - block_prologue_size));
	while (pc != block_end_pc) {
//		LOGE("thumb pc %d\n", pc);
		exec_cyc=0;
	
		block_data[block_data_position].block_offset = translation_ptr;

		thumb_base_cycles();
		//generate_step_debug();

		translate_thumb_instruction();

		//if(pc != block_end_pc)
		//			add_adr(pc, reinterpret_cast<u32*>(0xFFFFFFFF));

		block_data_position++;

		/* If it went too far the cache needs to be flushed and the process
		 restarted. Because we might already be nested several stages in
		 a simple recursive call here won't work, it has to pedal out to
		 the beginning. */
		cycle_count+=exec_cyc;

		if (translation_ptr > translation_cache_limit) {
			translation_flush_count++;

			if(pc<0xFFFF0000) {

							flush_translation_cache_rom();}
							else
							{
							flush_translation_cache_bios();
						}

			return -1;
		}

		/* If the next instruction is a block entry point update the
		 cycle counter and update */
		if (block_data[block_data_position].update_cycles == 1) {
			generate_cycle_update();
		}
	}
	//if(dynarec_proc ==DEBUG_PROC && block_print)
	//LOGE("thumb pc end %d", pc);

	for (i = 0; i < translation_gate_targets; i++) {
		if (pc == translation_gate_target_pc[i]) {
			generate_translation_gate(thumb);
			break;
		}
	}

	for (i = 0; i < block_exit_position; i++) {
		branch_target = block_exits[i].branch_target;

		if ((branch_target >= block_start_pc)
				&& (branch_target < block_end_pc)) {
			/* Internal branch, patch to recorded address */
			translation_target = block_data[(branch_target - block_start_pc)
					/ thumb_instruction_width].block_offset;

			generate_branch_patch_unconditional(block_exits[i].branch_source,
					translation_target);
		} else {
			/* External branch, save for later */
			external_block_exits[external_block_exit_position].branch_target =
					branch_target;
			external_block_exits[external_block_exit_position].branch_source =
					block_exits[i].branch_source;
			external_block_exit_position++;
		}
	}

	if(pc<0xFFFF0000) {

			rom_translation_ptr = translation_ptr;}
						else
						{
							 bios_translation_ptr = translation_ptr; ;
					}

	for (i = 0; i < external_block_exit_position; i++) {
		branch_target = external_block_exits[i].branch_target;
		thumb_link_block();
		if (translation_target == NULL)
			return -1;
		generate_branch_patch_unconditional(
				external_block_exits[i].branch_source, translation_target);
	}

	return 0;

#undef prepare_load_reg
#undef prepare_load_reg_pc
#undef prepare_store_reg
#undef complete_store_reg
#undef generate_load_reg
#undef generate_store_reg
}

void flush_translation_cache_ram() {
	//flush_ram_count++;
	/*  printf("ram flush %d (pc %x), %x to %x, %x to %x\n",
	 flush_ram_count, reg[REG_PC], iwram_code_min, iwram_code_max,
	 ewram_code_min, ewram_code_max); */

#ifndef PC_BUILD
	invalidate_icache_region(ram_translation_cache,
			(ram_translation_ptr - ram_translation_cache) + 0x100);
#endif
	//ram_translation_ptr = ram_translation_cache;
	//last_ram_translation_ptr = ram_translation_cache;
	ram_block_tag_top = 0x0101;
	if (iwram_code_min != 0xFFFFFFFF) {
		iwram_code_min &= 0x7FFF;
		iwram_code_max &= 0x7FFF;
		//memset(iwram + iwram_code_min, 0, iwram_code_max - iwram_code_min);
	}

	if (ewram_code_min != 0xFFFFFFFF) {
		u32 ewram_code_min_page;
		u32 ewram_code_max_page;
		u32 ewram_code_min_offset;
		u32 ewram_code_max_offset;
		u32 i;

		ewram_code_min &= 0x3FFFF;
		ewram_code_max &= 0x3FFFF;

		ewram_code_min_page = ewram_code_min >> 15;
		ewram_code_max_page = ewram_code_max >> 15;
		ewram_code_min_offset = ewram_code_min & 0x7FFF;
		ewram_code_max_offset = ewram_code_max & 0x7FFF;

		if (ewram_code_min_page == ewram_code_max_page) {
			//memset(ewram + (ewram_code_min_page * 0x10000) +
			//ewram_code_min_offset, 0,
			// ewram_code_max_offset - ewram_code_min_offset);
		} else {
			for (i = ewram_code_min_page + 1; i < ewram_code_max_page; i++) {
				//memset(ewram + (i * 0x10000), 0, 0x8000);
			}

			//memset(ewram, 0, ewram_code_max_offset);
		}
	}

	iwram_code_min = 0xFFFFFFFF;
	iwram_code_max = 0xFFFFFFFF;
	ewram_code_min = 0xFFFFFFFF;
	ewram_code_max = 0xFFFFFFFF;
}

void flush_translation_cache_rom() {
#ifndef PC_BUILD
	invalidate_icache_region(rom_translation_cache,
			rom_translation_ptr - rom_translation_cache + 0x100);
#endif

	rom_translation_ptr = rom_translation_cache;
	last_rom_translation_ptr = rom_translation_cache;
	memset(rom_branch_hash, 0, sizeof(rom_branch_hash));
	flush_page_table();
}

void flush_translation_cache_bios() {
#ifndef PC_BUILD
	invalidate_icache_region(bios_translation_cache,
			bios_translation_ptr - bios_translation_cache + 0x100);
#endif

	bios_block_tag_top = 0x0101;
	bios_translation_ptr = bios_translation_cache;
	last_bios_translation_ptr = bios_translation_cache;
	//LOGE("flush called");
	memset(bios_rom, 0, 0x8000);
	wipe_bios();
}

#ifdef GP2X_BUILD
#define cache_dump_prefix "/mnt/dump/"
#else
#define cache_dump_prefix ""
#endif

#define stdio_file_open_read  "rb"
#define stdio_file_open_write "wb"

#define file_close(filename_tag)                                            \
   fclose(filename_tag)                                                      \

#define file_write(filename_tag, buffer, size)                              \
	    fwrite(buffer, size, 1, filename_tag)                                     \

#define file_open(filename_tag, filename, mode)                             \
  FILE *filename_tag = fopen(filename, stdio_file_open_##mode)              \

__attribute__((noinline)) void dump_translation_cache() {
	/*file_open(ram_cache, cache_dump_prefix "ram_cache.bin", write);
	file_write(ram_cache, ram_translation_cache,
			ram_translation_ptr - ram_translation_cache);
	file_close(ram_cache);*/

	file_open(rom_cache, cache_dump_prefix "rom_cache.bin", write);
	file_write(rom_cache, rom_translation_cache,
			rom_translation_ptr - rom_translation_cache);
	file_close(rom_cache);

	file_open(bios_cache, cache_dump_prefix "bios_cache.bin", write);
	file_write(bios_cache, bios_translation_cache,
			bios_translation_ptr - bios_translation_cache);
	file_close(bios_cache);
}

}

