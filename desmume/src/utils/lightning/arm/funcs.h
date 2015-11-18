/******************************** -*- C -*- ****************************
 *
 *	Run-time assembler for the arm
 *
 ***********************************************************************/

/***********************************************************************
 *
 * Copyright 2011 Free Software Foundation
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
 *	Paulo Cesar Pereira de Andrade
 ***********************************************************************/

#ifndef __lightning_funcs_h
#define __lightning_funcs_h

#if defined(__linux__)
#  include <stdio.h>
#  include <string.h>
#endif

#include <stdlib.h>
#include <unistd.h>
#include <sys/mman.h>

// extern void __clear_cache(void*, void*);

// static void
// jit_flush_code(void *start, void *end)
// {
    // mprotect(start, (char *)end - (char *)start,
	     // PROT_READ | PROT_WRITE | PROT_EXEC);
    // __clear_cache(start, end);
// }

#define jit_get_cpu			jit_get_cpu
__jit_constructor static void
jit_get_cpu(void)
{
#if defined(__linux__) || defined(ANDROID)
    FILE	*fp;
    char	*ptr;
    char	 buf[128];
    static int	 initialized;

    if (initialized)
		return;
    initialized = 1;
    if ((fp = fopen("/proc/cpuinfo", "r")) == NULL)
		return;

    while (fgets(buf, sizeof(buf), fp)) {
	if (strncmp(buf, "CPU architecture:", 17) == 0) {
	    jit_cpu.version = strtol(buf + 17, &ptr, 10);
	    while (*ptr) {
		if (*ptr == 'T' || *ptr == 't') {
		    ++ptr;
		    jit_cpu.thumb = 1;
		}
		else if (*ptr == 'E' || *ptr == 'e') {
		    jit_cpu.extend = 1;
		    ++ptr;
		}
		else
		    ++ptr;
	    }
	}
	else if (strncmp(buf, "Features\t:", 10) == 0) {
	    if ((ptr = strstr(buf + 10, "vfpv")))
		jit_cpu.vfp = strtol(ptr + 4, NULL, 0);
	    if ((ptr = strstr(buf + 10, "neon")))
		jit_cpu.neon = 1;
	    if ((ptr = strstr(buf + 10, "thumb")))
		jit_cpu.thumb = 1;
	}
    }
    fclose(fp);
#endif
#if defined(__ARM_PCS_VFP)
    if (!jit_cpu.vfp)
		jit_cpu.vfp = 3;
    if (!jit_cpu.version)
		jit_cpu.version = 7;
    jit_cpu.abi = 1;
#endif
    /* armv6t2 todo (software float and thumb2) */
    if (!jit_cpu.vfp && jit_cpu.thumb)
		jit_cpu.thumb = 0;
}

#endif /* __lightning_funcs_h */
