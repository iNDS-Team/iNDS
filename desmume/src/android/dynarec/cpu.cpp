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
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
 */

// Important todo:
// - stm reglist writeback when base is in the list needs adjustment
// - block memory needs psr swapping and user mode reg swapping

#include "dynarec/cpu.h"
#include "dynarec_linker.h"
#include "sys/mman.h"
#include "errno.h"
#include "android/log.h"

using namespace Dynarec;

extern "C"
{

u32 memory_region_access_read_u8[16];
u32 memory_region_access_read_s8[16];
u32 memory_region_access_read_u16[16];
u32 memory_region_access_read_s16[16];
u32 memory_region_access_read_u32[16];
u32 memory_region_access_write_u8[16];
u32 memory_region_access_write_u16[16];
u32 memory_region_access_write_u32[16];
u32 memory_reads_u8;
u32 memory_reads_s8;
u32 memory_reads_u16;
u32 memory_reads_s16;
u32 memory_reads_u32;
u32 memory_writes_u8;
u32 memory_writes_u16;
u32 memory_writes_u32;

const u8 bit_count[256] =
{
  0, 1, 1, 2, 1, 2, 2, 3, 1, 2, 2, 3, 2, 3, 3, 4, 1, 2, 2, 3, 2, 3, 3,
  4, 2, 3, 3, 4, 3, 4, 4, 5, 1, 2, 2, 3, 2, 3, 3, 4, 2, 3, 3, 4, 3, 4,
  4, 5, 2, 3, 3, 4, 3, 4, 4, 5, 3, 4, 4, 5, 4, 5, 5, 6, 1, 2, 2, 3, 2,
  3, 3, 4, 2, 3, 3, 4, 3, 4, 4, 5, 2, 3, 3, 4, 3, 4, 4, 5, 3, 4, 4, 5,
  4, 5, 5, 6, 2, 3, 3, 4, 3, 4, 4, 5, 3, 4, 4, 5, 4, 5, 5, 6, 3, 4, 4,
  5, 4, 5, 5, 6, 4, 5, 5, 6, 5, 6, 6, 7, 1, 2, 2, 3, 2, 3, 3, 4, 2, 3,
  3, 4, 3, 4, 4, 5, 2, 3, 3, 4, 3, 4, 4, 5, 3, 4, 4, 5, 4, 5, 5, 6, 2,
  3, 3, 4, 3, 4, 4, 5, 3, 4, 4, 5, 4, 5, 5, 6, 3, 4, 4, 5, 4, 5, 5, 6,
  4, 5, 5, 6, 5, 6, 6, 7, 2, 3, 3, 4, 3, 4, 4, 5, 3, 4, 4, 5, 4, 5, 5,
  6, 3, 4, 4, 5, 4, 5, 5, 6, 4, 5, 5, 6, 5, 6, 6, 7, 3, 4, 4, 5, 4, 5,
  5, 6, 4, 5, 5, 6, 5, 6, 6, 7, 4, 5, 5, 6, 5, 6, 6, 7, 5, 6, 6, 7, 6,
  7, 7, 8
};


#ifdef REGISTER_USAGE_ANALYZE

u64 instructions_total = 0;

u64 arm_reg_freq[16];
u64 arm_reg_access_total = 0;
u64 arm_instructions_total = 0;

u64 thumb_reg_freq[16];
u64 thumb_reg_access_total = 0;
u64 thumb_instructions_total = 0;

// mla/long mla's addition operand are not counted yet.

#define using_register(instruction_set, register, type)                       \
  instruction_set##_reg_freq[register]++;                                     \
  instruction_set##_reg_access_total++                                        \

#define using_register_list(instruction_set, rlist, count)                    \
{                                                                             \
  u32 i;                                                                      \
  for(i = 0; i < count; i++)                                                  \
  {                                                                           \
    if((reg_list >> i) & 0x01)                                                \
    {                                                                         \
      using_register(instruction_set, i, memory_target);                      \
    }                                                                         \
  }                                                                           \
}                                                                             \

#define using_instruction(instruction_set)                                    \
  instruction_set##_instructions_total++;                                     \
  instructions_total++                                                        \

int sort_tagged_element(const void *_a, const void *_b)
{
  const u64 *a = _a;
  const u64 *b = _b;

  return (int)(b[1] - a[1]);
}

void print_register_usage()
{
  u32 i;
  u64 arm_reg_freq_tagged[32];
  u64 thumb_reg_freq_tagged[32];
  double percent;
  double percent_total = 0.0;

  for(i = 0; i < 16; i++)
  {
    arm_reg_freq_tagged[i * 2] = i;
    arm_reg_freq_tagged[(i * 2) + 1] = arm_reg_freq[i];
    thumb_reg_freq_tagged[i * 2] = i;
    thumb_reg_freq_tagged[(i * 2) + 1] = thumb_reg_freq[i];
  }

  qsort(arm_reg_freq_tagged, 16, sizeof(u64) * 2, sort_tagged_element);
  qsort(thumb_reg_freq_tagged, 16, sizeof(u64) * 2, sort_tagged_element);

  printf("ARM register usage (%lf%% ARM instructions):\n",
   (arm_instructions_total * 100.0) / instructions_total);
  for(i = 0; i < 16; i++)
  {
    percent = (arm_reg_freq_tagged[(i * 2) + 1] * 100.0) /
     arm_reg_access_total;
    percent_total += percent;
    printf("r%02d: %lf%% (-- %lf%%)\n",
     (u32)arm_reg_freq_tagged[(i * 2)], percent, percent_total);
  }

  percent_total = 0.0;

  printf("\nThumb register usage (%lf%% Thumb instructions):\n",
   (thumb_instructions_total * 100.0) / instructions_total);
  for(i = 0; i < 16; i++)
  {
    percent = (thumb_reg_freq_tagged[(i * 2) + 1] * 100.0) /
     thumb_reg_access_total;
    percent_total += percent;
    printf("r%02d: %lf%% (-- %lf%%)\n",
     (u32)thumb_reg_freq_tagged[(i * 2)], percent, percent_total);
  }

  memset(arm_reg_freq, 0, sizeof(u64) * 16);
  memset(thumb_reg_freq, 0, sizeof(u64) * 16);
  arm_reg_access_total = 0;
  thumb_reg_access_total = 0;
}

#else

#define using_register(instruction_set, register, type)                       \

#define using_register_list(instruction_set, rlist, count)                    \

#define using_instruction(instruction_set)                                    \

#endif


#define arm_decode_data_proc_reg()                                            \
  u32 rn = (opcode >> 16) & 0x0F;                                             \
  u32 rd = (opcode >> 12) & 0x0F;                                             \
  u32 rm = opcode & 0x0F;                                                     \
  using_register(arm, rd, op_dest);                                           \
  using_register(arm, rn, op_src);                                            \
  using_register(arm, rm, op_src)                                             \

#define arm_decode_data_proc_imm()                                            \
  u32 rn = (opcode >> 16) & 0x0F;                                             \
  u32 rd = (opcode >> 12) & 0x0F;                                             \
  u32 imm;                                                                    \
  ror(imm, opcode & 0xFF, ((opcode >> 8) & 0x0F) * 2);                        \
  using_register(arm, rd, op_dest);                                           \
  using_register(arm, rn, op_src)                                             \

#define arm_decode_psr_reg()                                                  \
  u32 psr_field = (opcode >> 16) & 0x0F;                                      \
  u32 rd = (opcode >> 12) & 0x0F;                                             \
  u32 rm = opcode & 0x0F;                                                     \
  using_register(arm, rd, op_dest);                                           \
  using_register(arm, rm, op_src)                                             \

#define arm_decode_psr_imm()                                                  \
  u32 psr_field = (opcode >> 16) & 0x0F;                                      \
  u32 rd = (opcode >> 12) & 0x0F;                                             \
  u32 imm;                                                                    \
  ror(imm, opcode & 0xFF, ((opcode >> 8) & 0x0F) * 2);                        \
  using_register(arm, rd, op_dest)                                            \

#define arm_decode_branchx()                                                  \
  u32 rn = opcode & 0x0F;                                                     \
  using_register(arm, rn, branch_target)                                      \

#define arm_decode_multiply()                                                 \
  u32 rd = (opcode >> 16) & 0x0F;                                             \
  u32 rn = (opcode >> 12) & 0x0F;                                             \
  u32 rs = (opcode >> 8) & 0x0F;                                              \
  u32 rm = opcode & 0x0F;                                                     \
  using_register(arm, rd, op_dest);                                           \
  using_register(arm, rn, op_src);                                            \
  using_register(arm, rm, op_src)                                             \

#define arm_decode_multiply_long()                                            \
  u32 rdhi = (opcode >> 16) & 0x0F;                                           \
  u32 rdlo = (opcode >> 12) & 0x0F;                                           \
  u32 rn = (opcode >> 8) & 0x0F;                                              \
  u32 rm = opcode & 0x0F;                                                     \
  using_register(arm, rdhi, op_dest);                                         \
  using_register(arm, rdlo, op_dest);                                         \
  using_register(arm, rn, op_src);                                            \
  using_register(arm, rm, op_src)                                             \

#define arm_decode_swap()                                                     \
  u32 rn = (opcode >> 16) & 0x0F;                                             \
  u32 rd = (opcode >> 12) & 0x0F;                                             \
  u32 rm = opcode & 0x0F;                                                     \
  using_register(arm, rd, memory_target);                                     \
  using_register(arm, rn, memory_base);                                       \
  using_register(arm, rm, memory_target)                                      \

#define arm_decode_half_trans_r()                                             \
  u32 rn = (opcode >> 16) & 0x0F;                                             \
  u32 rd = (opcode >> 12) & 0x0F;                                             \
  u32 rm = opcode & 0x0F;                                                     \
  using_register(arm, rd, memory_target);                                     \
  using_register(arm, rn, memory_base);                                       \
  using_register(arm, rm, memory_offset)                                      \

#define arm_decode_half_trans_of()                                            \
  u32 rn = (opcode >> 16) & 0x0F;                                             \
  u32 rd = (opcode >> 12) & 0x0F;                                             \
  u32 offset = ((opcode >> 4) & 0xF0) | (opcode & 0x0F);                      \
  using_register(arm, rd, memory_target);                                     \
  using_register(arm, rn, memory_base)                                        \

#define arm_decode_data_trans_imm()                                           \
  u32 rn = (opcode >> 16) & 0x0F;                                             \
  u32 rd = (opcode >> 12) & 0x0F;                                             \
  u32 offset = opcode & 0x0FFF;                                               \
  using_register(arm, rd, memory_target);                                     \
  using_register(arm, rn, memory_base)                                        \

#define arm_decode_data_trans_reg()                                           \
  u32 rn = (opcode >> 16) & 0x0F;                                             \
  u32 rd = (opcode >> 12) & 0x0F;                                             \
  u32 rm = opcode & 0x0F;                                                     \
  using_register(arm, rd, memory_target);                                     \
  using_register(arm, rn, memory_base);                                       \
  using_register(arm, rm, memory_offset)                                      \

#define arm_decode_block_trans()                                              \
  u32 rn = (opcode >> 16) & 0x0F;                                             \
  u32 reg_list = opcode & 0xFFFF;                                             \
  using_register(arm, rn, memory_base);                                       \
  using_register_list(arm, reg_list, 16)                                      \

#define arm_decode_branch()                                                   \
  s32 offset = ((s32)(opcode & 0xFFFFFF) << 8) >> 6                           \


#define thumb_decode_shift()                                                  \
  u32 imm = (opcode >> 6) & 0x1F;                                             \
  u32 rs = (opcode >> 3) & 0x07;                                              \
  u32 rd = opcode & 0x07;                                                     \
  using_register(thumb, rd, op_dest);                                         \
  using_register(thumb, rs, op_shift)                                         \

#define thumb_decode_add_sub()                                                \
  u32 rn = (opcode >> 6) & 0x07;                                              \
  u32 rs = (opcode >> 3) & 0x07;                                              \
  u32 rd = opcode & 0x07;                                                     \
  using_register(thumb, rd, op_dest);                                         \
  using_register(thumb, rn, op_src);                                          \
  using_register(thumb, rn, op_src)                                           \

#define thumb_decode_add_sub_imm()                                            \
  u32 imm = (opcode >> 6) & 0x07;                                             \
  u32 rs = (opcode >> 3) & 0x07;                                              \
  u32 rd = opcode & 0x07;                                                     \
  using_register(thumb, rd, op_src_dest);                                     \
  using_register(thumb, rs, op_src)                                           \

#define thumb_decode_imm()                                                    \
  u32 imm = opcode & 0xFF;                                                    \
  using_register(thumb, ((opcode >> 8) & 0x07), op_dest)                      \

#define thumb_decode_alu_op()                                                 \
  u32 rs = (opcode >> 3) & 0x07;                                              \
  u32 rd = opcode & 0x07;                                                     \
  using_register(thumb, rd, op_src_dest);                                     \
  using_register(thumb, rs, op_src)                                           \

#define thumb_decode_hireg_op()                                               \
  u32 rs = (opcode >> 3) & 0x0F;                                              \
  u32 rd = ((opcode >> 4) & 0x08) | (opcode & 0x07);                          \
  using_register(thumb, rd, op_src_dest);                                     \
  using_register(thumb, rs, op_src)                                           \


#define thumb_decode_mem_reg()                                                \
  u32 ro = (opcode >> 6) & 0x07;                                              \
  u32 rb = (opcode >> 3) & 0x07;                                              \
  u32 rd = opcode & 0x07;                                                     \
  using_register(thumb, rd, memory_target);                                   \
  using_register(thumb, rb, memory_base);                                     \
  using_register(thumb, ro, memory_offset)                                    \


#define thumb_decode_mem_imm()                                                \
  u32 imm = (opcode >> 6) & 0x1F;                                             \
  u32 rb = (opcode >> 3) & 0x07;                                              \
  u32 rd = opcode & 0x07;                                                     \
  using_register(thumb, rd, memory_target);                                   \
  using_register(thumb, rb, memory_base)                                      \


#define thumb_decode_add_sp()                                                 \
  u32 imm = opcode & 0x7F;                                                    \
  using_register(thumb, REG_SP, op_dest)                                      \

#define thumb_decode_rlist()                                                  \
  u32 reg_list = opcode & 0xFF;                                               \
  using_register_list(thumb, rlist, 8)                                        \

#define thumb_decode_branch_cond()                                            \
  s32 offset = (s8)(opcode & 0xFF)                                            \

#define thumb_decode_swi()                                                    \
  u32 comment = opcode & 0xFF                                                 \

#define thumb_decode_branch()                                                 \
  u32 offset = opcode & 0x07FF                                                \


#define get_shift_register(dest)                                              \
  u32 shift = reg[(opcode >> 8) & 0x0F];                                      \
  using_register(arm, ((opcode >> 8) & 0x0F), op_shift);                      \
  dest = reg[rm];                                                             \
  if(rm == 15)                                                                \
    dest += 4                                                                 \


#define calculate_z_flag(dest)                                                \
  z_flag = (dest == 0)                                                        \

#define calculate_n_flag(dest)                                                \
  n_flag = ((signed)dest < 0)                                                 \

#define calculate_c_flag_sub(dest, src_a, src_b)                              \
  c_flag = ((unsigned)src_b <= (unsigned)src_a)                               \

#define calculate_v_flag_sub(dest, src_a, src_b)                              \
  v_flag = ((signed)src_b > (signed)src_a) != ((signed)dest < 0)              \

#define calculate_c_flag_add(dest, src_a, src_b)                              \
  c_flag = ((unsigned)dest < (unsigned)src_a)                                 \

#define calculate_v_flag_add(dest, src_a, src_b)                              \
  v_flag = ((signed)dest < (signed)src_a) != ((signed)src_b < 0)              \


#define calculate_reg_sh()                                                    \
  u32 reg_sh = 0;                                                             \
  switch((opcode >> 4) & 0x07)                                                \
  {                                                                           \
    /* LSL imm */                                                             \
    case 0x0:                                                                 \
    {                                                                         \
      reg_sh = reg[rm] << ((opcode >> 7) & 0x1F);                             \
      break;                                                                  \
    }                                                                         \
                                                                              \
    /* LSL reg */                                                             \
    case 0x1:                                                                 \
    {                                                                         \
      get_shift_register(reg_sh);                                             \
      if(shift <= 31)                                                         \
        reg_sh = reg_sh << shift;                                             \
      else                                                                    \
        reg_sh = 0;                                                           \
      break;                                                                  \
    }                                                                         \
                                                                              \
    /* LSR imm */                                                             \
    case 0x2:                                                                 \
    {                                                                         \
      u32 imm = (opcode >> 7) & 0x1F;                                         \
      if(imm == 0)                                                            \
        reg_sh = 0;                                                           \
      else                                                                    \
        reg_sh = reg[rm] >> imm;                                              \
      break;                                                                  \
    }                                                                         \
                                                                              \
    /* LSR reg */                                                             \
    case 0x3:                                                                 \
    {                                                                         \
      get_shift_register(reg_sh);                                             \
      if(shift <= 31)                                                         \
        reg_sh = reg_sh >> shift;                                             \
      else                                                                    \
        reg_sh = 0;                                                           \
      break;                                                                  \
    }                                                                         \
                                                                              \
    /* ASR imm */                                                             \
    case 0x4:                                                                 \
    {                                                                         \
      u32 imm = (opcode >> 7) & 0x1F;                                         \
      reg_sh = reg[rm];                                                       \
                                                                              \
      if(imm == 0)                                                            \
        reg_sh = (s32)reg_sh >> 31;                                           \
      else                                                                    \
        reg_sh = (s32)reg_sh >> imm;                                          \
      break;                                                                  \
    }                                                                         \
                                                                              \
    /* ASR reg */                                                             \
    case 0x5:                                                                 \
    {                                                                         \
      get_shift_register(reg_sh);                                             \
      if(shift <= 31)                                                         \
        reg_sh = (s32)reg_sh >> shift;                                        \
      else                                                                    \
        reg_sh = (s32)reg_sh >> 31;                                           \
      break;                                                                  \
    }                                                                         \
                                                                              \
    /* ROR imm */                                                             \
    case 0x6:                                                                 \
    {                                                                         \
      u32 imm = (opcode >> 7) & 0x1F;                                         \
                                                                              \
      if(imm == 0)                                                            \
        reg_sh = (reg[rm] >> 1) | (c_flag << 31);                             \
      else                                                                    \
        ror(reg_sh, reg[rm], imm);                                            \
      break;                                                                  \
    }                                                                         \
                                                                              \
    /* ROR reg */                                                             \
    case 0x7:                                                                 \
    {                                                                         \
      get_shift_register(reg_sh);                                             \
      ror(reg_sh, reg_sh, shift);                                             \
      break;                                                                  \
    }                                                                         \
  }                                                                           \

#define calculate_reg_sh_flags()                                              \
  u32 reg_sh = 0;                                                             \
  switch((opcode >> 4) & 0x07)                                                \
  {                                                                           \
    /* LSL imm */                                                             \
    case 0x0:                                                                 \
    {                                                                         \
      u32 imm = (opcode >> 7) & 0x1F;                                         \
      reg_sh = reg[rm];                                                       \
                                                                              \
      if(imm != 0)                                                            \
      {                                                                       \
        c_flag = (reg_sh >> (32 - imm)) & 0x01;                               \
        reg_sh <<= imm;                                                       \
      }                                                                       \
                                                                              \
      break;                                                                  \
    }                                                                         \
                                                                              \
    /* LSL reg */                                                             \
    case 0x1:                                                                 \
    {                                                                         \
      get_shift_register(reg_sh);                                             \
      if(shift != 0)                                                          \
      {                                                                       \
        if(shift > 31)                                                        \
        {                                                                     \
          if(shift == 32)                                                     \
            c_flag = reg_sh & 0x01;                                           \
          else                                                                \
            c_flag = 0;                                                       \
          reg_sh = 0;                                                         \
        }                                                                     \
        else                                                                  \
        {                                                                     \
          c_flag = (reg_sh >> (32 - shift)) & 0x01;                           \
          reg_sh <<= shift;                                                   \
        }                                                                     \
      }                                                                       \
      break;                                                                  \
    }                                                                         \
                                                                              \
    /* LSR imm */                                                             \
    case 0x2:                                                                 \
    {                                                                         \
      u32 imm = (opcode >> 7) & 0x1F;                                         \
      reg_sh = reg[rm];                                                       \
      if(imm == 0)                                                            \
      {                                                                       \
        c_flag = reg_sh >> 31;                                                \
        reg_sh = 0;                                                           \
      }                                                                       \
      else                                                                    \
      {                                                                       \
        c_flag = (reg_sh >> (imm - 1)) & 0x01;                                \
        reg_sh >>= imm;                                                       \
      }                                                                       \
      break;                                                                  \
    }                                                                         \
                                                                              \
    /* LSR reg */                                                             \
    case 0x3:                                                                 \
    {                                                                         \
      get_shift_register(reg_sh);                                             \
      if(shift != 0)                                                          \
      {                                                                       \
        if(shift > 31)                                                        \
        {                                                                     \
          if(shift == 32)                                                     \
            c_flag = (reg_sh >> 31) & 0x01;                                   \
          else                                                                \
            c_flag = 0;                                                       \
          reg_sh = 0;                                                         \
        }                                                                     \
        else                                                                  \
        {                                                                     \
          c_flag = (reg_sh >> (shift - 1)) & 0x01;                            \
          reg_sh >>= shift;                                                   \
        }                                                                     \
      }                                                                       \
      break;                                                                  \
    }                                                                         \
                                                                              \
    /* ASR imm */                                                             \
    case 0x4:                                                                 \
    {                                                                         \
      u32 imm = (opcode >> 7) & 0x1F;                                         \
      reg_sh = reg[rm];                                                       \
      if(imm == 0)                                                            \
      {                                                                       \
        reg_sh = (s32)reg_sh >> 31;                                           \
        c_flag = reg_sh & 0x01;                                               \
      }                                                                       \
      else                                                                    \
      {                                                                       \
        c_flag = (reg_sh >> (imm - 1)) & 0x01;                                \
        reg_sh = (s32)reg_sh >> imm;                                          \
      }                                                                       \
      break;                                                                  \
    }                                                                         \
                                                                              \
    /* ASR reg */                                                             \
    case 0x5:                                                                 \
    {                                                                         \
      get_shift_register(reg_sh);                                             \
      if(shift != 0)                                                          \
      {                                                                       \
        if(shift > 31)                                                        \
        {                                                                     \
          reg_sh = (s32)reg_sh >> 31;                                         \
          c_flag = reg_sh & 0x01;                                             \
        }                                                                     \
        else                                                                  \
        {                                                                     \
          c_flag = (reg_sh >> (shift - 1)) & 0x01;                            \
          reg_sh = (s32)reg_sh >> shift;                                      \
        }                                                                     \
      }                                                                       \
      break;                                                                  \
    }                                                                         \
                                                                              \
    /* ROR imm */                                                             \
    case 0x6:                                                                 \
    {                                                                         \
      u32 imm = (opcode >> 7) & 0x1F;                                         \
      reg_sh = reg[rm];                                                       \
      if(imm == 0)                                                            \
      {                                                                       \
        u32 old_c_flag = c_flag;                                              \
        c_flag = reg_sh & 0x01;                                               \
        reg_sh = (reg_sh >> 1) | (old_c_flag << 31);                          \
      }                                                                       \
      else                                                                    \
      {                                                                       \
        c_flag = (reg_sh >> (imm - 1)) & 0x01;                                \
        ror(reg_sh, reg_sh, imm);                                             \
      }                                                                       \
      break;                                                                  \
    }                                                                         \
                                                                              \
    /* ROR reg */                                                             \
    case 0x7:                                                                 \
    {                                                                         \
      get_shift_register(reg_sh);                                             \
      if(shift != 0)                                                          \
      {                                                                       \
        c_flag = (reg_sh >> (shift - 1)) & 0x01;                              \
        ror(reg_sh, reg_sh, shift);                                           \
      }                                                                       \
      break;                                                                  \
    }                                                                         \
  }                                                                           \

#define calculate_reg_offset()                                                \
  u32 reg_offset = 0;                                                         \
  switch((opcode >> 5) & 0x03)                                                \
  {                                                                           \
    /* LSL imm */                                                             \
    case 0x0:                                                                 \
    {                                                                         \
      reg_offset = reg[rm] << ((opcode >> 7) & 0x1F);                         \
      break;                                                                  \
    }                                                                         \
                                                                              \
    /* LSR imm */                                                             \
    case 0x1:                                                                 \
    {                                                                         \
      u32 imm = (opcode >> 7) & 0x1F;                                         \
      if(imm == 0)                                                            \
        reg_offset = 0;                                                       \
      else                                                                    \
        reg_offset = reg[rm] >> imm;                                          \
      break;                                                                  \
    }                                                                         \
                                                                              \
    /* ASR imm */                                                             \
    case 0x2:                                                                 \
    {                                                                         \
      u32 imm = (opcode >> 7) & 0x1F;                                         \
      if(imm == 0)                                                            \
        reg_offset = (s32)reg[rm] >> 31;                                      \
      else                                                                    \
        reg_offset = (s32)reg[rm] >> imm;                                     \
      break;                                                                  \
    }                                                                         \
                                                                              \
    /* ROR imm */                                                             \
    case 0x3:                                                                 \
    {                                                                         \
      u32 imm = (opcode >> 7) & 0x1F;                                         \
      if(imm == 0)                                                            \
        reg_offset = (reg[rm] >> 1) | (c_flag << 31);                         \
      else                                                                    \
        ror(reg_offset, reg[rm], imm);                                        \
      break;                                                                  \
    }                                                                         \
  }                                                                           \

#define calculate_flags_add(dest, src_a, src_b)                               \
  calculate_z_flag(dest);                                                     \
  calculate_n_flag(dest);                                                     \
  calculate_c_flag_add(dest, src_a, src_b);                                   \
  calculate_v_flag_add(dest, src_a, src_b)                                    \

#define calculate_flags_sub(dest, src_a, src_b)                               \
  calculate_z_flag(dest);                                                     \
  calculate_n_flag(dest);                                                     \
  calculate_c_flag_sub(dest, src_a, src_b);                                   \
  calculate_v_flag_sub(dest, src_a, src_b)                                    \

#define calculate_flags_logic(dest)                                           \
  calculate_z_flag(dest);                                                     \
  calculate_n_flag(dest)                                                      \

#define extract_flags()                                                       \
  n_flag = reg[REG_CPSR] >> 31;                                               \
  z_flag = (reg[REG_CPSR] >> 30) & 0x01;                                      \
  c_flag = (reg[REG_CPSR] >> 29) & 0x01;                                      \
  v_flag = (reg[REG_CPSR] >> 28) & 0x01;                                      \

#define collapse_flags()                                                      \
  reg[REG_CPSR] = (n_flag << 31) | (z_flag << 30) | (c_flag << 29) |          \
   (v_flag << 28) | (reg[REG_CPSR] & 0xFF)                                    \

#define memory_region(r_dest, l_dest, address)                                \
  r_dest = memory_regions[address >> 24];                                     \
  l_dest = memory_limits[address >> 24]                                       \


#define pc_region()                                                           \
  memory_region(pc_region, pc_limit, pc)                                      \

#define check_pc_region()                                                     \
  new_pc_region = (pc >> 15);                                                 \
  if(new_pc_region != pc_region)                                              \
  {                                                                           \
    pc_region = new_pc_region;                                                \
    pc_address_block = memory_map_read[new_pc_region];                        \
                                                                              \
    if(pc_address_block == NULL)                                              \
      pc_address_block = load_gamepak_page(pc_region & 0x3FF);                \
  }                                                                           \

u32 branch_targets = 0;
u32 high_frequency_branch_targets = 0;

#define BRANCH_ACTIVITY_THRESHOLD 50

#define arm_update_pc()                                                       \
  pc = reg[REG_PC]                                                            \

#define arm_pc_offset(val)                                                    \
  pc += val;                                                                  \
  reg[REG_PC] = pc                                                            \

#define arm_pc_offset_update(val)                                             \
  pc += val;                                                                  \
  reg[REG_PC] = pc                                                            \

#define arm_pc_offset_update_direct(val)                                      \
  pc = val;                                                                   \
  reg[REG_PC] = pc                                                            \


// It should be okay to still generate result flags, spsr will overwrite them.
// This is pretty infrequent (returning from interrupt handlers, et al) so
// probably not worth optimizing for.

#define check_for_interrupts()                                                \
  if((io_registers[REG_IE] & io_registers[REG_IF]) &&                         \
   io_registers[REG_IME] && ((reg[REG_CPSR] & 0x80) == 0))                    \
  {                                                                           \
    reg_mode[MODE_IRQ][6] = reg[REG_PC] + 4;                                  \
    spsr[MODE_IRQ] = reg[REG_CPSR];                                           \
    reg[REG_CPSR] = 0xD2;                                                     \
    reg[REG_PC] = armcpu->intVector + 0x18;                                                 \
    arm_update_pc();                                                          \
    set_cpu_mode(MODE_IRQ);                                                   \
    goto arm_loop;                                                            \
  }                                                                           \

#define arm_spsr_restore()                                                    \
  if(rd == 15)                                                                \
  {                                                                           \
    if(reg[CPU_MODE] != MODE_USER)                                            \
    {                                                                         \
      reg[REG_CPSR] = spsr[reg[CPU_MODE]];                                    \
      extract_flags();                                                        \
      set_cpu_mode(cpu_modes[reg[REG_CPSR] & 0x1F]);                          \
      check_for_interrupts();                                                 \
    }                                                                         \
    arm_update_pc();                                                          \
                                                                              \
    if(reg[REG_CPSR] & 0x20)                                                  \
      goto thumb_loop;                                                        \
  }                                                                           \

#define arm_data_proc_flags_reg()                                             \
  arm_decode_data_proc_reg();                                                 \
  calculate_reg_sh_flags()                                                    \

#define arm_data_proc_reg()                                                   \
  arm_decode_data_proc_reg();                                                 \
  calculate_reg_sh()                                                          \

#define arm_data_proc_flags_imm()                                             \
  arm_decode_data_proc_imm()                                                  \

#define arm_data_proc_imm()                                                   \
  arm_decode_data_proc_imm()                                                  \

#define arm_data_proc(expr, type)                                             \
{                                                                             \
  u32 dest;                                                                   \
  arm_pc_offset(8);                                                           \
  arm_data_proc_##type();                                                     \
  dest = expr;                                                                \
  arm_pc_offset(-4);                                                          \
  reg[rd] = dest;                                                             \
                                                                              \
  if(rd == 15)                                                                \
  {                                                                           \
    arm_update_pc();                                                          \
  }                                                                           \
}                                                                             \

#define flags_vars(src_a, src_b)                                              \
  u32 dest;                                                                   \
  const u32 _sa = src_a;                                                      \
  const u32 _sb = src_b                                                       \

#define arm_data_proc_logic_flags(expr, type)                                 \
{                                                                             \
  arm_pc_offset(8);                                                           \
  arm_data_proc_flags_##type();                                               \
  u32 dest = expr;                                                            \
  calculate_flags_logic(dest);                                                \
  arm_pc_offset(-4);                                                          \
  reg[rd] = dest;                                                             \
  arm_spsr_restore();                                                         \
}                                                                             \

#define arm_data_proc_add_flags(src_a, src_b, type)                           \
{                                                                             \
  arm_pc_offset(8);                                                           \
  arm_data_proc_##type();                                                     \
  flags_vars(src_a, src_b);                                                   \
  dest = _sa + _sb;                                                           \
  calculate_flags_add(dest, _sa, _sb);                                        \
  arm_pc_offset(-4);                                                          \
  reg[rd] = dest;                                                             \
  arm_spsr_restore();                                                         \
}

#define arm_data_proc_sub_flags(src_a, src_b, type)                           \
{                                                                             \
  arm_pc_offset(8);                                                           \
  arm_data_proc_##type();                                                     \
  flags_vars(src_a, src_b);                                                   \
  dest = _sa - _sb;                                                           \
  calculate_flags_sub(dest, _sa, _sb);                                        \
  arm_pc_offset(-4);                                                          \
  reg[rd] = dest;                                                             \
  arm_spsr_restore();                                                         \
}                                                                             \

#define arm_data_proc_test_logic(expr, type)                                  \
{                                                                             \
  arm_pc_offset(8);                                                           \
  arm_data_proc_flags_##type();                                               \
  u32 dest = expr;                                                            \
  calculate_flags_logic(dest);                                                \
  arm_pc_offset(-4);                                                          \
}                                                                             \

#define arm_data_proc_test_add(src_a, src_b, type)                            \
{                                                                             \
  arm_pc_offset(8);                                                           \
  arm_data_proc_##type();                                                     \
  flags_vars(src_a, src_b);                                                   \
  dest = _sa + _sb;                                                           \
  calculate_flags_add(dest, _sa, _sb);                                        \
  arm_pc_offset(-4);                                                          \
}                                                                             \

#define arm_data_proc_test_sub(src_a, src_b, type)                            \
{                                                                             \
  arm_pc_offset(8);                                                           \
  arm_data_proc_##type();                                                     \
  flags_vars(src_a, src_b);                                                   \
  dest = _sa - _sb;                                                           \
  calculate_flags_sub(dest, _sa, _sb);                                        \
  arm_pc_offset(-4);                                                          \
}                                                                             \

#define arm_multiply_flags_yes(_dest)                                         \
  calculate_z_flag(_dest);                                                    \
  calculate_n_flag(_dest);                                                    \

#define arm_multiply_flags_no(_dest)                                          \

#define arm_multiply_long_flags_yes(_dest_lo, _dest_hi)                       \
  z_flag = (_dest_lo == 0) & (_dest_hi == 0);                                 \
  calculate_n_flag(_dest_hi)                                                  \

#define arm_multiply_long_flags_no(_dest_lo, _dest_hi)                        \

#define arm_multiply(add_op, flags)                                           \
{                                                                             \
  u32 dest;                                                                   \
  arm_decode_multiply();                                                      \
  dest = (reg[rm] * reg[rs]) add_op;                                          \
  arm_multiply_flags_##flags(dest);                                           \
  reg[rd] = dest;                                                             \
  arm_pc_offset(4);                                                           \
}                                                                             \

#define arm_multiply_long_addop(type)                                         \
  + ((type##64)((((type##64)reg[rdhi]) << 32) | reg[rdlo]));                  \

#define arm_multiply_long(add_op, flags, type)                                \
{                                                                             \
  type##64 dest;                                                              \
  u32 dest_lo;                                                                \
  u32 dest_hi;                                                                \
  arm_decode_multiply_long();                                                 \
  dest = ((type##64)((type##32)reg[rm]) *                                     \
   (type##64)((type##32)reg[rn])) add_op;                                     \
  dest_lo = (u32)dest;                                                        \
  dest_hi = (u32)(dest >> 32);                                                \
  arm_multiply_long_flags_##flags(dest_lo, dest_hi);                          \
  reg[rdlo] = dest_lo;                                                        \
  reg[rdhi] = dest_hi;                                                        \
  arm_pc_offset(4);                                                           \
}                                                                             \

const u32 psr_masks[16] =
{
  0x00000000, 0x000000FF, 0x0000FF00, 0x0000FFFF, 0x00FF0000,
  0x00FF00FF, 0x00FFFF00, 0x00FFFFFF, 0xFF000000, 0xFF0000FF,
  0xFF00FF00, 0xFF00FFFF, 0xFFFF0000, 0xFFFF00FF, 0xFFFFFF00,
  0xFFFFFFFF
};

#define arm_psr_read(dummy, psr_reg)                                          \
  collapse_flags();                                                           \
  reg[rd] = psr_reg                                                           \

#define arm_psr_store_cpsr(source)                                            \
  reg[REG_CPSR] = (source & store_mask) | (reg[REG_CPSR] & (~store_mask));    \
  extract_flags();                                                            \
  if(store_mask & 0xFF)                                                       \
  {                                                                           \
    set_cpu_mode(cpu_modes[reg[REG_CPSR] & 0x1F]);                            \
    check_for_interrupts();                                                   \
  }                                                                           \

#define arm_psr_store_spsr(source)                                            \
  u32 _psr = spsr[reg[CPU_MODE]];                                             \
  spsr[reg[CPU_MODE]] = (source & store_mask) | (_psr & (~store_mask))        \

#define arm_psr_store(source, psr_reg)                                        \
  const u32 store_mask = psr_masks[psr_field];                                \
  arm_psr_store_##psr_reg(source)                                             \

#define arm_psr_src_reg reg[rm]

#define arm_psr_src_imm imm

#define arm_psr(op_type, transfer_type, psr_reg)                              \
{                                                                             \
  arm_decode_psr_##op_type();                                                 \
  arm_pc_offset(4);                                                           \
  arm_psr_##transfer_type(arm_psr_src_##op_type, psr_reg);                    \
}                                                                             \

#define arm_data_trans_reg()                                                  \
  arm_decode_data_trans_reg();                                                \
  calculate_reg_offset()                                                      \

#define arm_data_trans_imm()                                                  \
  arm_decode_data_trans_imm()                                                 \

#define arm_data_trans_half_reg()                                             \
  arm_decode_half_trans_r()                                                   \

#define arm_data_trans_half_imm()                                             \
  arm_decode_half_trans_of()                                                  \

#define aligned_address_mask8  0xF0000000
#define aligned_address_mask16 0xF0000001
#define aligned_address_mask32 0xF0000003

#define fast_read_memory(size, type, address, dest)                           \
{                                                                             \
  u8 *map;                                                                    \
  u32 _address = address;                                                     \
                                                                              \
  if(_address < 0x10000000)                                                   \
  {                                                                           \
    memory_region_access_read_##type[_address >> 24]++;                       \
    memory_reads_##type++;                                                    \
  }                                                                           \
  if(((_address >> 24) == 0) && (pc >= 0x4000))                               \
  {                                                                           \
    dest = *((type *)((u8 *)&bios_read_protect + (_address & 0x03)));         \
  }                                                                           \
  else                                                                        \
                                                                              \
  if(((_address & aligned_address_mask##size) == 0) &&                        \
   (map = memory_map_read[_address >> 15]))                                   \
  {                                                                           \
    dest = *((type *)((u8 *)map + (_address & 0x7FFF)));                      \
  }                                                                           \
  else                                                                        \
  {                                                                           \
    dest = (type)read_memory##size(_address);                                 \
  }                                                                           \
}                                                                             \

#define fast_read_memory_s16(address, dest)                                   \
{                                                                             \
  u8 *map;                                                                    \
  u32 _address = address;                                                     \
  if(_address < 0x10000000)                                                   \
  {                                                                           \
    memory_region_access_read_s16[_address >> 24]++;                          \
    memory_reads_s16++;                                                       \
  }                                                                           \
  if(((_address & aligned_address_mask16) == 0) &&                            \
   (map = memory_map_read[_address >> 15]))                                   \
  {                                                                           \
    dest = *((s16 *)((u8 *)map + (_address & 0x7FFF)));                       \
  }                                                                           \
  else                                                                        \
  {                                                                           \
    dest = (s16)read_memory16_signed(_address);                               \
  }                                                                           \
}                                                                             \


#define fast_write_memory(size, type, address, value)                         \
{                                                                             \
  u8 *map;                                                                    \
  u32 _address = (address) & ~(aligned_address_mask##size & 0x03);            \
  if(_address < 0x10000000)                                                   \
  {                                                                           \
    memory_region_access_write_##type[_address >> 24]++;                      \
    memory_writes_##type++;                                                   \
  }                                                                           \
                                                                              \
  if(((_address & aligned_address_mask##size) == 0) &&                        \
   (map = memory_map_write[_address >> 15]))                                  \
  {                                                                           \
    *((type *)((u8 *)map + (_address & 0x7FFF))) = value;                     \
  }                                                                           \
  else                                                                        \
  {                                                                           \
    cpu_alert = write_memory##size(_address, value);                          \
    if(cpu_alert)                                                             \
      goto alert;                                                             \
  }                                                                           \
}                                                                             \

#define load_aligned32(address, dest)                                         \
{                                                                             \
  u32 _address = address;                                                     \
  u8 *map = memory_map_read[_address >> 15];                                  \
  if(_address < 0x10000000)                                                   \
  {                                                                           \
    memory_region_access_read_u32[_address >> 24]++;                          \
    memory_reads_u32++;                                                       \
  }                                                                           \
  if(map)                                                                     \
  {                                                                           \
    dest = address32(map, _address & 0x7FFF);                                 \
  }                                                                           \
  else                                                                        \
  {                                                                           \
    dest = read_memory32(_address);                                           \
  }                                                                           \
}                                                                             \

#define store_aligned32(address, value)                                       \
{                                                                             \
  u32 _address = address;                                                     \
  u8 *map = memory_map_write[_address >> 15];                                 \
  if(_address < 0x10000000)                                                   \
  {                                                                           \
    memory_region_access_write_u32[_address >> 24]++;                         \
    memory_writes_u32++;                                                      \
  }                                                                           \
  if(map)                                                                     \
  {                                                                           \
    address32(map, _address & 0x7FFF) = value;                                \
  }                                                                           \
  else                                                                        \
  {                                                                           \
    cpu_alert = write_memory32(_address, value);                              \
    if(cpu_alert)                                                             \
      goto alert;                                                             \
  }                                                                           \
}                                                                             \

#define load_memory_u8(address, dest)                                         \
  fast_read_memory(8, u8, address, dest)                                      \

#define load_memory_u16(address, dest)                                        \
  fast_read_memory(16, u16, address, dest)                                    \

#define load_memory_u32(address, dest)                                        \
  fast_read_memory(32, u32, address, dest)                                    \

#define load_memory_s8(address, dest)                                         \
  fast_read_memory(8, s8, address, dest)                                      \

#define load_memory_s16(address, dest)                                        \
  fast_read_memory_s16(address, dest)                                         \

#define store_memory_u8(address, value)                                       \
  fast_write_memory(8, u8, address, value)                                    \

#define store_memory_u16(address, value)                                      \
  fast_write_memory(16, u16, address, value)                                  \

#define store_memory_u32(address, value)                                      \
  fast_write_memory(32, u32, address, value)                                  \

#define no_op                                                                 \

#define arm_access_memory_writeback_yes(off_op)                               \
  reg[rn] = address off_op                                                    \

#define arm_access_memory_writeback_no(off_op)                                \

#define arm_access_memory_pc_preadjust_load()                                 \

#define arm_access_memory_pc_preadjust_store()                                \
  u32 reg_op = reg[rd];                                                       \
  if(rd == 15)                                                                \
    reg_op += 4                                                               \

#define arm_access_memory_pc_postadjust_load()                                \
  arm_update_pc()                                                             \

#define arm_access_memory_pc_postadjust_store()                               \

#define load_reg_op reg[rd]                                                   \

#define store_reg_op reg_op                                                   \

#define arm_access_memory(access_type, off_op, off_type, mem_type,            \
 wb, wb_off_op)                                                               \
{                                                                             \
  arm_pc_offset(8);                                                           \
  arm_data_trans_##off_type();                                                \
  u32 address = reg[rn] off_op;                                               \
  arm_access_memory_pc_preadjust_##access_type();                             \
                                                                              \
  arm_pc_offset(-4);                                                          \
  arm_access_memory_writeback_##wb(wb_off_op);                                \
  access_type##_memory_##mem_type(address, access_type##_reg_op);             \
  arm_access_memory_pc_postadjust_##access_type();                            \
}                                                                             \

#define word_bit_count(word)                                                  \
  (bit_count[word >> 8] + bit_count[word & 0xFF])                             \

#define sprint_no(access_type, offset_type, writeback_type)                   \

#define sprint_yes(access_type, offset_type, writeback_type)                  \
  printf("sbit on %s %s %s\n", #access_type, #offset_type, #writeback_type)   \

#define arm_block_writeback_load()                                            \
  if(!((reg_list >> rn) & 0x01))                                              \
  {                                                                           \
    reg[rn] = address;                                                        \
  }                                                                           \

#define arm_block_writeback_store()                                           \
  reg[rn] = address                                                           \

#define arm_block_writeback_yes(access_type)                                  \
  arm_block_writeback_##access_type()                                         \

#define arm_block_writeback_no(access_type)                                   \

#define load_block_memory(address, dest)                                      \
  dest = address32(address_region, (address + offset) & 0x7FFF)               \

#define store_block_memory(address, dest)                                     \
  address32(address_region, (address + offset) & 0x7FFF) = dest               \

#define arm_block_memory_offset_down_a()                                      \
  (base - (word_bit_count(reg_list) * 4) + 4)                                 \

#define arm_block_memory_offset_down_b()                                      \
  (base - (word_bit_count(reg_list) * 4))                                     \

#define arm_block_memory_offset_no()                                          \
  (base)                                                                      \

#define arm_block_memory_offset_up()                                          \
  (base + 4)                                                                  \

#define arm_block_memory_writeback_down()                                     \
  reg[rn] = base - (word_bit_count(reg_list) * 4)                             \

#define arm_block_memory_writeback_up()                                       \
  reg[rn] = base + (word_bit_count(reg_list) * 4)                             \

#define arm_block_memory_writeback_no()                                       \

#define arm_block_memory_load_pc()                                            \
  load_aligned32(address, pc);                                                \
  reg[REG_PC] = pc                                                            \

#define arm_block_memory_store_pc()                                           \
  store_aligned32(address, pc + 4)                                            \

#define arm_block_memory(access_type, offset_type, writeback_type, s_bit)     \
{                                                                             \
  arm_decode_block_trans();                                                   \
  u32 base = reg[rn];                                                         \
  u32 address = arm_block_memory_offset_##offset_type() & 0xFFFFFFFC;         \
  u32 i;                                                                      \
                                                                              \
  arm_block_memory_writeback_##writeback_type();                              \
                                                                              \
  for(i = 0; i < 15; i++)                                                     \
  {                                                                           \
    if((reg_list >> i) & 0x01)                                                \
    {                                                                         \
      access_type##_aligned32(address, reg[i]);                               \
      address += 4;                                                           \
    }                                                                         \
  }                                                                           \
                                                                              \
  arm_pc_offset(4);                                                           \
  if(reg_list & 0x8000)                                                       \
  {                                                                           \
    arm_block_memory_##access_type##_pc();                                    \
  }                                                                           \
}                                                                             \

#define arm_swap(type)                                                        \
{                                                                             \
  arm_decode_swap();                                                          \
  u32 temp;                                                                   \
  load_memory_##type(reg[rn], temp);                                          \
  store_memory_##type(reg[rn], reg[rm]);                                      \
  reg[rd] = temp;                                                             \
  arm_pc_offset(4);                                                           \
}                                                                             \

#define arm_next_instruction()                                                \
{                                                                             \
  arm_pc_offset(4);                                                           \
  goto skip_instruction;                                                      \
}                                                                             \

#define thumb_update_pc()                                                     \
  pc = reg[REG_PC]                                                            \

#define thumb_pc_offset(val)                                                  \
  pc += val;                                                                  \
  reg[REG_PC] = pc                                                            \

#define thumb_pc_offset_update(val)                                           \
  pc += val;                                                                  \
  reg[REG_PC] = pc                                                            \

#define thumb_pc_offset_update_direct(val)                                    \
  pc = val;                                                                   \
  reg[REG_PC] = pc                                                            \

// Types: add_sub, add_sub_imm, alu_op, imm
// Affects N/Z/C/V flags

#define thumb_add(type, dest_reg, src_a, src_b)                               \
{                                                                             \
  thumb_decode_##type();                                                      \
  const u32 _sa = src_a;                                                      \
  const u32 _sb = src_b;                                                      \
  u32 dest = _sa + _sb;                                                       \
  calculate_flags_add(dest, _sa, _sb);                                        \
  reg[dest_reg] = dest;                                                       \
  thumb_pc_offset(2);                                                         \
}                                                                             \

#define thumb_add_noflags(type, dest_reg, src_a, src_b)                       \
{                                                                             \
  thumb_decode_##type();                                                      \
  u32 dest = (src_a) + (src_b);                                               \
  reg[dest_reg] = dest;                                                       \
  thumb_pc_offset(2);                                                         \
}                                                                             \

#define thumb_sub(type, dest_reg, src_a, src_b)                               \
{                                                                             \
  thumb_decode_##type();                                                      \
  const u32 _sa = src_a;                                                      \
  const u32 _sb = src_b;                                                      \
  u32 dest = _sa - _sb;                                                       \
  calculate_flags_sub(dest, _sa, _sb);                                        \
  reg[dest_reg] = dest;                                                       \
  thumb_pc_offset(2);                                                         \
}                                                                             \

// Affects N/Z flags

#define thumb_logic(type, dest_reg, expr)                                     \
{                                                                             \
  thumb_decode_##type();                                                      \
  u32 dest = expr;                                                            \
  calculate_flags_logic(dest);                                                \
  reg[dest_reg] = dest;                                                       \
  thumb_pc_offset(2);                                                         \
}                                                                             \

// Decode types: shift, alu_op
// Operation types: lsl, lsr, asr, ror
// Affects N/Z/C flags

#define thumb_shift_lsl_reg()                                                 \
  u32 shift = reg[rs];                                                        \
  u32 dest = reg[rd];                                                         \
  if(shift != 0)                                                              \
  {                                                                           \
    if(shift > 31)                                                            \
    {                                                                         \
      if(shift == 32)                                                         \
        c_flag = dest & 0x01;                                                 \
      else                                                                    \
        c_flag = 0;                                                           \
      dest = 0;                                                               \
    }                                                                         \
    else                                                                      \
    {                                                                         \
      c_flag = (dest >> (32 - shift)) & 0x01;                                 \
      dest <<= shift;                                                         \
    }                                                                         \
  }                                                                           \

#define thumb_shift_lsr_reg()                                                 \
  u32 shift = reg[rs];                                                        \
  u32 dest = reg[rd];                                                         \
  if(shift != 0)                                                              \
  {                                                                           \
    if(shift > 31)                                                            \
    {                                                                         \
      if(shift == 32)                                                         \
        c_flag = dest >> 31;                                                  \
      else                                                                    \
        c_flag = 0;                                                           \
      dest = 0;                                                               \
    }                                                                         \
    else                                                                      \
    {                                                                         \
      c_flag = (dest >> (shift - 1)) & 0x01;                                  \
      dest >>= shift;                                                         \
    }                                                                         \
  }                                                                           \

#define thumb_shift_asr_reg()                                                 \
  u32 shift = reg[rs];                                                        \
  u32 dest = reg[rd];                                                         \
  if(shift != 0)                                                              \
  {                                                                           \
    if(shift > 31)                                                            \
    {                                                                         \
      dest = (s32)dest >> 31;                                                 \
      c_flag = dest & 0x01;                                                   \
    }                                                                         \
    else                                                                      \
    {                                                                         \
      c_flag = (dest >> (shift - 1)) & 0x01;                                  \
      dest = (s32)dest >> shift;                                              \
    }                                                                         \
  }                                                                           \

#define thumb_shift_ror_reg()                                                 \
  u32 shift = reg[rs];                                                        \
  u32 dest = reg[rd];                                                         \
  if(shift != 0)                                                              \
  {                                                                           \
    c_flag = (dest >> (shift - 1)) & 0x01;                                    \
    ror(dest, dest, shift);                                                   \
  }                                                                           \

#define thumb_shift_lsl_imm()                                                 \
  u32 dest = reg[rs];                                                         \
  if(imm != 0)                                                                \
  {                                                                           \
    c_flag = (dest >> (32 - imm)) & 0x01;                                     \
    dest <<= imm;                                                             \
  }                                                                           \

#define thumb_shift_lsr_imm()                                                 \
  u32 dest;                                                                   \
  if(imm == 0)                                                                \
  {                                                                           \
    dest = 0;                                                                 \
    c_flag = reg[rs] >> 31;                                                   \
  }                                                                           \
  else                                                                        \
  {                                                                           \
    dest = reg[rs];                                                           \
    c_flag = (dest >> (imm - 1)) & 0x01;                                      \
    dest >>= imm;                                                             \
  }                                                                           \

#define thumb_shift_asr_imm()                                                 \
  u32 dest;                                                                   \
  if(imm == 0)                                                                \
  {                                                                           \
    dest = (s32)reg[rs] >> 31;                                                \
    c_flag = dest & 0x01;                                                     \
  }                                                                           \
  else                                                                        \
  {                                                                           \
    dest = reg[rs];                                                           \
    c_flag = (dest >> (imm - 1)) & 0x01;                                      \
    dest = (s32)dest >> imm;                                                  \
  }                                                                           \

#define thumb_shift_ror_imm()                                                 \
  u32 dest = reg[rs];                                                         \
  if(imm == 0)                                                                \
  {                                                                           \
    u32 old_c_flag = c_flag;                                                  \
    c_flag = dest & 0x01;                                                     \
    dest = (dest >> 1) | (old_c_flag << 31);                                  \
  }                                                                           \
  else                                                                        \
  {                                                                           \
    c_flag = (dest >> (imm - 1)) & 0x01;                                      \
    ror(dest, dest, imm);                                                     \
  }                                                                           \

#define thumb_shift(decode_type, op_type, value_type)                         \
{                                                                             \
  thumb_decode_##decode_type();                                               \
  thumb_shift_##op_type##_##value_type();                                     \
  calculate_flags_logic(dest);                                                \
  reg[rd] = dest;                                                             \
  thumb_pc_offset(2);                                                         \
}                                                                             \

#define thumb_test_add(type, src_a, src_b)                                    \
{                                                                             \
  thumb_decode_##type();                                                      \
  const u32 _sa = src_a;                                                      \
  const u32 _sb = src_b;                                                      \
  u32 dest = _sa + _sb;                                                       \
  calculate_flags_add(dest, src_a, src_b);                                    \
  thumb_pc_offset(2);                                                         \
}                                                                             \

#define thumb_test_sub(type, src_a, src_b)                                    \
{                                                                             \
  thumb_decode_##type();                                                      \
  const u32 _sa = src_a;                                                      \
  const u32 _sb = src_b;                                                      \
  u32 dest = _sa - _sb;                                                       \
  calculate_flags_sub(dest, src_a, src_b);                                    \
  thumb_pc_offset(2);                                                         \
}                                                                             \

#define thumb_test_logic(type, expr)                                          \
{                                                                             \
  thumb_decode_##type();                                                      \
  u32 dest = expr;                                                            \
  calculate_flags_logic(dest);                                                \
  thumb_pc_offset(2);                                                         \
}

#define thumb_hireg_op(expr)                                                  \
{                                                                             \
  thumb_pc_offset(4);                                                         \
  thumb_decode_hireg_op();                                                    \
  u32 dest = expr;                                                            \
  thumb_pc_offset(-2);                                                        \
  if(rd == 15)                                                                \
  {                                                                           \
    reg[REG_PC] = dest & ~0x01;                                               \
    thumb_update_pc();                                                        \
  }                                                                           \
  else                                                                        \
  {                                                                           \
    reg[rd] = dest;                                                           \
  }                                                                           \
}                                                                             \

// Operation types: imm, mem_reg, mem_imm

#define thumb_access_memory(access_type, op_type, address, reg_op,            \
 mem_type)                                                                    \
{                                                                             \
  thumb_decode_##op_type();                                                   \
  access_type##_memory_##mem_type(address, reg_op);                           \
  thumb_pc_offset(2);                                                         \
}                                                                             \

#define thumb_block_address_preadjust_no_op()                                 \

#define thumb_block_address_preadjust_up()                                    \
  address += bit_count[reg_list] * 4                                          \

#define thumb_block_address_preadjust_down()                                  \
  address -= bit_count[reg_list] * 4                                          \

#define thumb_block_address_preadjust_push_lr()                               \
  address -= (bit_count[reg_list] + 1) * 4                                    \

#define thumb_block_address_postadjust_no_op()                                \

#define thumb_block_address_postadjust_up()                                   \
  address += offset                                                           \

#define thumb_block_address_postadjust_down()                                 \
  address -= offset                                                           \

#define thumb_block_address_postadjust_pop_pc()                               \
  load_memory_u32(address + offset, pc);                                      \
  pc &= ~0x01;                                                                \
  reg[REG_PC] = pc;                                                           \
  address += offset + 4                                                       \

#define thumb_block_address_postadjust_push_lr()                              \
  store_memory_u32(address + offset, reg[REG_LR]);                            \

#define thumb_block_memory_wb_load(base_reg)                                  \
  if(!((reg_list >> base_reg) & 0x01))                                        \
  {                                                                           \
    reg[base_reg] = address;                                                  \
  }                                                                           \

#define thumb_block_memory_wb_store(base_reg)                                 \
  reg[base_reg] = address                                                     \

#define thumb_block_memory(access_type, pre_op, post_op, base_reg)            \
{                                                                             \
  u32 i;                                                                      \
  u32 offset = 0;                                                             \
  thumb_decode_rlist();                                                       \
  using_register(thumb, base_reg, memory_base);                               \
  u32 address = reg[base_reg] & ~0x03;                                        \
  thumb_block_address_preadjust_##pre_op();                                   \
                                                                              \
  for(i = 0; i < 8; i++)                                                      \
  {                                                                           \
    if((reg_list >> i) & 1)                                                   \
    {                                                                         \
      access_type##_aligned32(address + offset, reg[i]);                      \
      offset += 4;                                                            \
    }                                                                         \
  }                                                                           \
                                                                              \
  thumb_pc_offset(2);                                                         \
                                                                              \
  thumb_block_address_postadjust_##post_op();                                 \
  thumb_block_memory_wb_##access_type(base_reg);                              \
}                                                                             \

#define thumb_conditional_branch(condition)                                   \
{                                                                             \
  thumb_decode_branch_cond();                                                 \
  if(condition)                                                               \
  {                                                                           \
    thumb_pc_offset((offset * 2) + 4);                                        \
  }                                                                           \
  else                                                                        \
  {                                                                           \
    thumb_pc_offset(2);                                                       \
  }                                                                           \
}                                                                             \

// When a mode change occurs from non-FIQ to non-FIQ retire the current
// reg[13] and reg[14] into reg_mode[cpu_mode][5] and reg_mode[cpu_mode][6]
// respectively and load into reg[13] and reg[14] reg_mode[new_mode][5] and
// reg_mode[new_mode][6]. When swapping to/from FIQ retire/load reg[8]
// through reg[14] to/from reg_mode[MODE_FIQ][0] through reg_mode[MODE_FIQ][6].
u32* spsr_pointer;
u32 reg_mode[7][7];

cpu_mode_type cpu_modes[32] =
{
  MODE_INVALID, MODE_INVALID, MODE_INVALID, MODE_INVALID, MODE_INVALID,
  MODE_INVALID, MODE_INVALID, MODE_INVALID, MODE_INVALID, MODE_INVALID,
  MODE_INVALID, MODE_INVALID, MODE_INVALID, MODE_INVALID, MODE_INVALID,
  MODE_INVALID, MODE_USER, MODE_FIQ, MODE_IRQ, MODE_SUPERVISOR, MODE_INVALID,
  MODE_INVALID, MODE_INVALID, MODE_ABORT, MODE_INVALID, MODE_INVALID,
  MODE_INVALID, MODE_INVALID, MODE_UNDEFINED, MODE_INVALID, MODE_INVALID,
  MODE_USER
};

u32 cpu_modes_cpsr[7] = { 0x10, 0x11, 0x12, 0x13, 0x17, 0x1B, 0x1F };

// When switching modes set spsr[new_mode] to cpsr. Modifying PC as the
// target of a data proc instruction will set cpsr to spsr[cpu_mode].
// ARM/Thumb mode is stored in the flags directly, this is simpler than
// shadowing it since it has a constant 1bit represenation.

u32 last_mode;

void switch_user_arm()
{
	last_mode = armcpu_switchMode(dynarec_cpu, USR);
}

void switch_back_arm()
{
	armcpu_switchMode(dynarec_cpu, last_mode);
}

/*void dynarec_irq_interrupt()
{
    // Interrupt handler in BIOS
    reg_mode[MODE_IRQ][6] = reg[REG_PC] + 4;
    spsr[MODE_IRQ] = reg[REG_CPSR];
    //reg[REG_CPSR] = 0xD2;
    reg[REG_PC] = dynarec_cpu->intVector + 0x18;


    set_cpu_mode(MODE_IRQ);
    reg[CPU_HALT_STATE] = CPU_ACTIVE;
    reg[CHANGED_PC_STATUS] = 1;
  }*/

void cache_reg(u32 *old_reg)
{
	memcpy(dynarec_cpu->reg, old_reg, 32*4);
	dynarec_cpu->R= &dynarec_cpu->reg[0];

//	for (int i = 0; i < 16; i++)
//		LOGE("cache_reg[%d] : %x", i, dynarec_cpu->R[i]);

//	LOGE("cache_reg %x", old_reg);
}

void load_reg(u32 *new_reg)
{
	memcpy(new_reg, dynarec_cpu->R, 32*4);
	dynarec_cpu->R = new_reg;

//	LOGE("load_reg %x", new_reg);
}



void move_reg(u32 *new_reg)
{
	//load_reg(new_reg);
	dynarec_cpu = &NDS_ARM9;
	dynarec_cpu->R[CHANGED_PC_STATUS] =1;
	dynarec_cpu->R[15]-=8;
	spsr_pointer = &(dynarec_cpu->SPSR.val);

	dynarec_cpu = &NDS_ARM7;
	dynarec_cpu->R[CHANGED_PC_STATUS] =1;
	dynarec_cpu->R[15]-=8;
	spsr_pointer = &(dynarec_cpu->SPSR.val);

  //rom_translation_ptr = last_rom_translation_ptr = rom_translation_cache;
  u32 offset = reinterpret_cast<u32>(rom_translation_ptr) & 0x00000FFF;
  u8* page = rom_translation_ptr-offset;

    int fail = mprotect(page, ROM_TRANSLATION_CACHE_SIZE+4096, PROT_READ | PROT_WRITE | PROT_EXEC);

  if(fail)
  {
	  LOGE("rom protect change fail %s", strerror(errno));
//	  exit(1);
  }

  /*u32 ram_offset= reinterpret_cast<u32>(ram_translation_ptr) & 0x00000FFF;
  u8* ram_page= ram_translation_ptr-ram_offset;

  int ram_protect_fail = mprotect(ram_page, RAM_TRANSLATION_CACHE_SIZE+4096, PROT_READ | PROT_WRITE | PROT_EXEC);

  if(ram_protect_fail)
    {
  	  LOGE("ram protect change fail %s", strerror(errno));
  //	  exit(1);
    }*/

  u32 bios_offset= reinterpret_cast<u32>(bios_translation_ptr) & 0x00000FFF;
  u8* bios_page= bios_translation_ptr-bios_offset;


  int bios_protect_fail = mprotect(bios_page, BIOS_TRANSLATION_CACHE_SIZE+4096, PROT_READ | PROT_WRITE | PROT_EXEC);

  if(bios_protect_fail)
    {
  	  LOGE("bios protect change fail %s", strerror(errno));
  //	  exit(1);
    }

  flush_translation_cache_rom();
  flush_translation_cache_ram();
  flush_translation_cache_bios();

}
#include "MMU.h"
int start=0;
bool yummy=true;
bool log_out_enable =false;
extern u32 total_cycle_arm9;
extern u32 total_cycle_arm7;
int test=0;
u32 preserv[17];
#define PROCNUM ARMCPU_ARM9
u32 step_debug(u32 pc, u32 cycles)
{
	/*if(dynarec_proc == ARMCPU_ARM9)
		if(dynarec_cpu->R[Dynarec::REG_CPSR] & 0x20)
			LOGE("pc %x instruct %x", pc, _MMU_read16<ARMCPU_ARM9, MMU_AT_CODE>(pc));
			else
			{
			LOGE("pc %x instruct %x", pc, _MMU_read32<ARMCPU_ARM9, MMU_AT_CODE>(pc));
			}*/
	/*if((0-cycles)>1000000 || enable)
	{LOGE("cycles %x", 0-cycles);
	enable=true;
	}*/

	if(dynarec_proc == DEBUG_PROC && (pc <0 || start>0 /* || total_cycle_arm9 > 8893000*/) && yummy)
	{
		for(int i=0;i<15;i++)
			LOGE("r%u %x", i, dynarec_cpu->R[i]);

		if(dynarec_cpu->R[Dynarec::REG_CPSR] & 0x20)
			{
				LOGE("t pc %x instruct %x r12 %x cycle_count %u", pc, _MMU_read16(dynarec_proc, MMU_AT_CODE,pc),dynarec_cpu->R[12], (0-cycles-1)*2);

			}
				else
				{

					LOGE("a pc %x instruct %x  r12 %x cycle_count %u", pc, _MMU_read32(dynarec_proc, MMU_AT_CODE,pc), dynarec_cpu->R[12], (0-cycles-1)*2);
				}

		LOGE("cpsr %x", dynarec_cpu->R[Dynarec::REG_CPSR]);
		//start++;
		yummy=false;
		log_out_enable=true;
		/*if(start>0)
		{
			sleep(10);
			exit(0);
		}*/
	}

	bool cond_sat = pc == 0x20cdae8  ;//(_MMU_read16(0, MMU_AT_DATA, 0x4000180) == 0x606 && NDS_ARM9.R[0] == 0x6);// _MMU_read32(dynarec_proc, MMU_AT_DATA,0x2328880) ==0x2320828;//dynarec_cpu->R[14] == 0x20159f0 && dynarec_cpu->R[0] == 0x2320828;

	if(dynarec_proc == DEBUG_PROC && (cond_sat|| start>0 /* || total_cycle_arm9 > 8893000*/) && log_out_enable)
		{
			//for(int i=0;i<15;i++)
			//			LOGE("r%u %x", i, dynarec_cpu->R[i]);

			u32 arm7_instruct;

			if(NDS_ARM7.R[Dynarec::REG_CPSR] & 0x20)
			{
				arm7_instruct = _MMU_read16(dynarec_proc, MMU_AT_CODE,NDS_ARM7.R[15]);
			}
			else
			{
				arm7_instruct = _MMU_read32(dynarec_proc, MMU_AT_CODE,NDS_ARM7.R[15]);
			}

			if(dynarec_cpu->R[Dynarec::REG_CPSR] & 0x20)
				{
					LOGE("t pc %x instruct %x cpsr %x sp %x lr %x", pc, _MMU_read16(dynarec_proc, MMU_AT_CODE,pc), dynarec_cpu->R[16], dynarec_cpu->R[13], dynarec_cpu->R[14]);

				}
					else
					{

						LOGE("a pc %x instruct %x cpsr %x sp %x lr %x", pc, _MMU_read32(dynarec_proc, MMU_AT_CODE,pc),dynarec_cpu->R[16], dynarec_cpu->R[13], dynarec_cpu->R[14]);
					}

			//LOGE("cpsr %x", dynarec_cpu->R[Dynarec::REG_CPSR]);
			start++;

			//for(int i=0;i<15;i++)
			//						LOGE("old r%u %x", i, preserv[i]);

			/*			if(preserv[Dynarec::REG_CPSR] & 0x20)
							{
								LOGE("old pc %x instruct %x r0 %x r4 %x r8 %x cycle_count %u", preserv[15], _MMU_read16(dynarec_proc, MMU_AT_CODE,preserv[15]), preserv[0], preserv[4], preserv[8], (0-cycles-1)*2);

							}
								else
								{

									LOGE("old pc %x instruct %x r0 %x r4 %x r8 %x cycle_count %u", preserv[15], _MMU_read32(dynarec_proc, MMU_AT_CODE,preserv[15]),preserv[0], preserv[4],  preserv[8],(0-cycles-1)*2);
								}*/

			//			LOGE("old cpsr %x", preserv[Dynarec::REG_CPSR]);

			if(pc == 0x20cc6e4)
			{
				LOGE("hit");
			}
		}

	if(dynarec_proc == DEBUG_PROC)
	{
		for(int i=0;i<=16;i++)
			preserv[i]=dynarec_cpu->R[i];

		preserv[15]=pc;
	}


	/*if(dynarec_cpu->R[Dynarec::REG_CPSR] & 0x20)
		{
			if(_MMU_read16(dynarec_proc, MMU_AT_CODE,pc) == 0)
			{
				LOGE("thumb zero instruct %x pc %x", dynarec_proc, pc);
				sleep(10);
				exit(0);

			}
		}
	else if( _MMU_read32(dynarec_proc, MMU_AT_CODE,pc) == 0)
	{
		LOGE("arm zero instruct %x pc %x", dynarec_proc, pc);
		sleep(10);
		exit(0);

	}*/


	//LOGE("cycle counter %x", 0-cycles);

	/*if(((pc == 0x200c224)|| start>0) && yummy && dynarec_proc ==0)
	{
		//LOGE("pc %x instruct %x cycles %u", pc,_MMU_read32<ARMCPU_ARM9, MMU_AT_CODE>(pc), 0-cycles-1);
			for(int i=0;i<15;i++)
					LOGE("r%u %x", i, dynarec_cpu->R[i]);

	if(dynarec_cpu->R[Dynarec::REG_CPSR] & 0x20)
	{
		LOGE("pc %x instruct %x cycle %u", pc, _MMU_read16(dynarec_proc, MMU_AT_CODE,pc), total_cycle_arm9);

	}
		else
		{

			LOGE("pc %x instruct %x cycle %u", pc, _MMU_read32(dynarec_proc, MMU_AT_CODE,pc), total_cycle_arm9);
		}
		LOGE("cpsr %x", dynarec_cpu->R[Dynarec::REG_CPSR]);
		//LOGE("spsr %x spsr irq %x", dynarec_cpu->SPSR.val, dynarec_cpu->SPSR_irq.val);
		//LOGE("cycles %x", 0-cycles);
		//exit(1);
		//yummy=false
		//if(pc == 0x2036c50)
		//sleep(10);
		//exit(0);
		//enable=true;
		//yummy=false;
		start++;
	}*/

			//LOGE("cpsr %x", reg[Dynarec::REG_CPSR]);

	/*for(int i=0;i<15;i++)
	if(reg[i] == 0x027e4280)
	{
		for(int i=0;i<15;i++)
							LOGE("r%u %x", i, reg[i]);

			if(reg[Dynarec::REG_CPSR] & 0x20)
				LOGE("pc %x instruct %x", pc, _MMU_read16<ARMCPU_ARM9, MMU_AT_CODE>(pc));
				else
				{
					LOGE("pc %x instruct %x", pc, _MMU_read32<ARMCPU_ARM9, MMU_AT_CODE>(pc));
				}
				LOGE("cpsr %x", reg[Dynarec::REG_CPSR]);
				exit(0);
	}*/



	/*if(((pc ==  0x200c224 )|| start>0) && enable && dynarec_proc ==0)
	{
		for(int i=0;i<15;i++)
					LOGE("r%u %x", i, dynarec_cpu->R[i]);
		if(dynarec_cpu->R[16] & 0x20)
				LOGE("pc %x instruct %x r8 %x r13 %x cycle count %u val %x", pc, _MMU_read16(dynarec_proc, MMU_AT_CODE,pc), dynarec_cpu->R[8], dynarec_cpu->R[13], total_cycle_arm9,
						_MMU_read32(dynarec_proc, MMU_AT_DATA,0x380ff28));
				else
				{
					LOGE("pc %x instruct %x r8 %x r13 %x cycle count %u val %x", pc, _MMU_read32(dynarec_proc, MMU_AT_CODE,pc), dynarec_cpu->R[8], dynarec_cpu->R[13], total_cycle_arm9,
							_MMU_read32(dynarec_proc, MMU_AT_DATA,0x380ff30));
				}
		LOGE("cpsr %x", dynarec_cpu->R[16]);
		//LOGE("spsr %x spsr irq %x", dynarec_cpu->SPSR.val, dynarec_cpu->SPSR_irq.val);
		start++;
	}*/

	/*if(joker() && yummy)
	{
		for(int i=0;i<15;i++)
							LOGE("r%u %x", i, reg[i]);
				LOGE("pc %x instruct %x", pc, _MMU_read32<ARMCPU_ARM9, MMU_AT_CODE>(pc));
				LOGE("cpsr %x", reg[Dynarec::REG_CPSR]);
				yummy=false;
	}*/

	/*if(reg[Dynarec::REG_CPSR] & 0x20)
					LOGE("pc %x instruct %x", pc, _MMU_read16<ARMCPU_ARM9, MMU_AT_CODE>(pc));
					else
					{
						LOGE("pc %x instruct %x", pc, _MMU_read32<ARMCPU_ARM9, MMU_AT_CODE>(pc));
					}*/

	//if(start>100){

		//log_out_enable=false;}

	/*if((test != dynarec_cpu->SPSR_irq.val) && dynarec_proc ==1 )
		{

			test = dynarec_cpu->SPSR_irq.val;
			LOGE("uh oh %x pc %x cpsr %x spsr %x cycle count %u", test, pc, dynarec_cpu->R[16], dynarec_cpu->SPSR.val, total_cycle_arm9);
		}*/

	/*if((test != _MMU_read32<ARMCPU_ARM9, MMU_AT_DATA>(0x027e3a40)))
	{

		test = _MMU_read32<ARMCPU_ARM9, MMU_AT_DATA>(0x027e3a40);
		LOGE("uh oh %x pc %x", test, pc);
	}*/
//if(yummy){LOGE("pc %x instruct %x", pc, _MMU_read32<ARMCPU_ARM9, MMU_AT_CODE>(pc));}
	//if(yummy){LOGE("r0 %x", reg[0]);}
	//if(yummy)LOGE("cpsr %x instruct %x", reg[Dynarec::REG_CPSR]);

	//if(pc == 0)yummy=false;

				/*if(_MMU_read32<ARMCPU_ARM9, MMU_AT_CODE>(pc) >>28 == 0xF && enable)
				{
					LOGE("yo %x pc %x", _MMU_read32<ARMCPU_ARM9, MMU_AT_CODE>(pc), pc);
					//exit(0);
				}*/
				//LOGE("CPSR %x", reg[Dynarec::REG_CPSR]);
				//start++;
	//}
//if(start==50)
//exit(0);
	//LOGE("cycle counter %x", cycles);

	return 0;
}

u32 instruction_count = 0;

u32 output_field = 0;
const u32 num_output_fields = 2;

u32 last_instruction = 0;

u32 in_interrupt = 0;

}

