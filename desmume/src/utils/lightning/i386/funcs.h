/******************************** -*- C -*- ****************************
 *
 *	Platform-independent layer inline functions (i386)
 *
 ***********************************************************************/


/***********************************************************************
 *
 * Copyright 2000,2001,2002,2006,2010 Free Software Foundation, Inc.
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
 *	Paolo Bonzini
 *	Paulo Cesar Pereira de Andrade
 ***********************************************************************/



#ifndef __lightning_funcs_h
#define __lightning_funcs_h

#ifdef __linux__
#include <unistd.h>
#include <sys/mman.h>
#endif

#ifdef _MSC_VER
#include <intrin.h>
#endif

// static void
// jit_flush_code(void *dest, void *end)
// {
  // /* On the x86, the PROT_EXEC bits are not handled by the MMU.
     // However, the kernel can emulate this by setting the code
     // segment's limit to the end address of the highest page
     // whose PROT_EXEC bit is set.

     // Linux kernels that do so and that disable by default the
     // execution of the data and stack segment are becoming more
     // and more common (Fedora, for example), so we implement our
     // jit_flush_code as an mprotect.  */
// #ifdef __linux__
  // static long prev_page = 0, prev_length = 0;
  // long page, length;
// #ifdef PAGESIZE
  // const int page_size = PAGESIZE;
// #else
  // static int page_size = -1;
  // if (page_size == -1)
    // page_size = sysconf (_SC_PAGESIZE);
// #endif

  // page = (long) dest & ~(page_size - 1);
  // length = ((char *) end - (char *) page + page_size - 1) & ~(page_size - 1);

  // /* Simple-minded attempt at optimizing the common case where a single
     // chunk of memory is used to compile multiple functions.  */
  // if (page >= prev_page && page + length <= prev_page + prev_length)
    // return;

  // mprotect ((void *) page, length, PROT_READ | PROT_WRITE | PROT_EXEC);

  // /* See if we can extend the previously mprotect'ed memory area towards
     // higher addresses: the starting address remains the same as before.  */
  // if (page >= prev_page && page <= prev_page + prev_length)
    // prev_length = page + length - prev_page;

  // /* See if we can extend the previously mprotect'ed memory area towards
     // lower addresses: the highest address remains the same as before.  */
  // else if (page < prev_page && page + length >= prev_page
          // && page + length <= prev_page + prev_length)
    // prev_length += prev_page - page, prev_page = page;

  // /* Nothing to do, replace the area.  */
  // else
    // prev_page = page, prev_length = length;
// #endif
// }

#define jit_get_cpu			jit_get_cpu
__jit_constructor static void
jit_get_cpu(void)
{
    union {
	struct {
	    _ui sse3:		1;
	    _ui pclmulqdq:	1;
	    _ui dtes64:		1;	/* amd reserved */
	    _ui monitor:	1;
	    _ui ds_cpl:		1;	/* amd reserved */
	    _ui vmx:		1;	/* amd reserved */
	    _ui smx:		1;	/* amd reserved */
	    _ui est:		1;	/* amd reserved */
	    _ui tm2:		1;	/* amd reserved */
	    _ui ssse3:		1;
	    _ui cntx_id:	1;	/* amd reserved */
	    _ui __reserved0:	1;
	    _ui fma:		1;
	    _ui cmpxchg16b:	1;
	    _ui xtpr:		1;	/* amd reserved */
	    _ui pdcm:		1;	/* amd reserved */
	    _ui __reserved1:	1;
	    _ui pcid:		1;	/* amd reserved */
	    _ui dca:		1;	/* amd reserved */
	    _ui sse4_1:		1;
	    _ui sse4_2:		1;
	    _ui x2apic:		1;	/* amd reserved */
	    _ui movbe:		1;	/* amd reserved */
	    _ui popcnt:		1;
	    _ui tsc:		1;	/* amd reserved */
	    _ui aes:		1;
	    _ui xsave:		1;
	    _ui osxsave:	1;
	    _ui avx:		1;
	    _ui __reserved2:	1;	/* amd F16C */
	    _ui __reserved3:	1;
	    _ui __alwayszero:	1;	/* amd RAZ */
	} bits;
	_ui	cpuid;
    } ecx;
    union {
	struct {
	    _ui fpu:		1;
	    _ui vme:		1;
	    _ui de:		1;
	    _ui pse:		1;
	    _ui tsc:		1;
	    _ui msr:		1;
	    _ui pae:		1;
	    _ui mce:		1;
	    _ui cmpxchg8b:	1;
	    _ui apic:		1;
	    _ui __reserved0:	1;
	    _ui sep:		1;
	    _ui mtrr:		1;
	    _ui pge:		1;
	    _ui mca:		1;
	    _ui cmov:		1;
	    _ui pat:		1;
	    _ui pse36:		1;
	    _ui psn:		1;	/* amd reserved */
	    _ui clfsh:		1;
	    _ui __reserved1:	1;
	    _ui ds:		1;	/* amd reserved */
	    _ui acpi:		1;	/* amd reserved */
	    _ui mmx:		1;
	    _ui fxsr:		1;
	    _ui sse:		1;
	    _ui sse2:		1;
	    _ui ss:		1;	/* amd reserved */
	    _ui htt:		1;
	    _ui tm:		1;	/* amd reserved */
	    _ui __reserved2:	1;
	    _ui pbe:		1;	/* amd reserved */
	} bits;
	_ui	cpuid;
    } edx;
#ifndef __LP64__
    int		ac, flags;
#endif
    _ui		eax, ebx;
    static int	initialized;

    /* may need to be called explicitly if not using gcc */
    if (initialized)
		return;
    initialized = 1;

#ifndef __LP64__
#ifdef _MSC_VER
    _asm 
    {
        ; check if 'cpuid' instructions is available by toggling eflags bit 21
        ;
        pushfd                      ; save eflags to stack
        pop     eax                 ; load eax from stack (with eflags)
        mov     ecx, eax            ; save the original eflags values to ecx
        xor     eax, 0x00200000     ; toggle bit 21
        push    eax                 ; store toggled eflags to stack
        popfd                       ; load eflags from stack
        pushfd                      ; save updated eflags to stack
        pop     eax                 ; load from stack
		xor     eax, ecx            ; 
		push    ecx                 ;
		popfd                       ;
		mov     ac, eax
	}
#else
    /* adapted from glibc __sysconf */
    __asm__ volatile ("pushfl;\n\t"
		      "popl %0;\n\t"
		      "movl $0x240000, %1;\n\t"
		      "xorl %0, %1;\n\t"
		      "pushl %1;\n\t"
		      "popfl;\n\t"
		      "pushfl;\n\t"
		      "popl %1;\n\t"
		      "xorl %0, %1;\n\t"
		      "pushl %0;\n\t"
		      "popfl"
		      : "=r" (flags), "=r" (ac));
#endif
    /* i386 or i486 without cpuid */
    if ((ac & (1 << 21)) == 0)
	/* probably without x87 as well */
		return;
#endif

    /* query %eax = 1 function */
#ifdef _MSC_VER
	//int ecx_cpuid;
	//int edx_cpuid;
	//_asm 
	//{
	//	mov     eax, 1
	//	cpuid
	//	mov     ecx_cpuid, ecx
	//	mov     edx_cpuid, edx
	//}
	//ecx.cpuid = (_ui)ecx_cpuid;
	//edx.cpuid = (_ui)edx_cpuid;

	int id_data[4];

	__cpuid(id_data, 1);

	ecx.cpuid = (_ui)id_data[2];
	edx.cpuid = (_ui)id_data[3];

#else
    __asm__ volatile ("xchgl %%ebx, %1; cpuid; xchgl %%ebx, %1"
		      : "=a" (eax), "=r" (ebx),
		      "=c" (ecx.cpuid), "=d" (edx.cpuid)
		      : "0" (1));
#endif

    /* check only what is useful for lightning and/or
     * what is the same for AMD and Intel processors */
    jit_cpu.fpu		= edx.bits.fpu;
    jit_cpu.cmpxchg8b	= edx.bits.cmpxchg8b;
    jit_cpu.cmov	= edx.bits.cmov;
    jit_cpu.mmx		= edx.bits.mmx;
    jit_cpu.sse		= edx.bits.sse;
    jit_cpu.sse2	= edx.bits.sse2;
    jit_cpu.sse3	= ecx.bits.sse3;
    jit_cpu.pclmulqdq	= ecx.bits.pclmulqdq;
    jit_cpu.ssse3	= ecx.bits.ssse3;
    jit_cpu.fma		= ecx.bits.fma;
    jit_cpu.cmpxchg16b	= ecx.bits.cmpxchg16b;
    jit_cpu.sse4_1	= ecx.bits.sse4_1;
    jit_cpu.sse4_2	= ecx.bits.sse4_2;
    jit_cpu.movbe	= ecx.bits.movbe;
    jit_cpu.popcnt	= ecx.bits.popcnt;
    jit_cpu.aes		= ecx.bits.aes;
    jit_cpu.avx		= ecx.bits.avx;

#ifdef __LP64__
#ifdef _MSC_VER
	//_asm 
	//{
	//	mov     eax, 0x80000001
	//	cpuid
	//	mov     ecx_cpuid, ecx
	//	mov     edx_cpuid, edx
	//}
	//ecx.cpuid = (_ui)ecx_cpuid;
	//edx.cpuid = (_ui)edx_cpuid;

	__cpuid(id_data, 0x80000001);

	ecx.cpuid = (_ui)id_data[2];
	edx.cpuid = (_ui)id_data[3];
#else
    /* query %eax = 0x80000001 function */
    __asm__ volatile ("xchgl %%ebx, %1; cpuid; xchgl %%ebx, %1"
		      : "=a" (eax), "=r" (ebx),
		      "=c" (ecx.cpuid), "=d" (edx.cpuid)
		      : "0" (0x80000001));
    jit_cpu.lahf	= ecx.cpuid & 1;
#endif
#endif

    /* default to round to nearest */
    jit_flags.rnd_near	= 1;

    /* default to use push/pop for arguments and float conversion;
     * this generates several redundant stack adjustments, but is
     * not dependent on patching stack adjustment in jit_prolog,
     * and this way, works with any possible code that jumps from/to
     * code after different jit_prolog */
    jit_flags.push_pop	= 1;
}

#endif /* __lightning_funcs_h */
