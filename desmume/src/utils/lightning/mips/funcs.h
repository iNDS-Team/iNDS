/******************************** -*- C -*- ****************************
 *
 *	Run-time assembler for the mips
 *
 ***********************************************************************/

/***********************************************************************
 *
 * Copyright 2010 Free Software Foundation
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

#include <unistd.h>
#include <sys/mman.h>

#if defined(__linux__)
#  include <sys/cachectl.h>
#endif

// static void
// jit_flush_code(void *start, void *end)
// {
    // mprotect(start, (char*)end - (char*)start,
	     // PROT_READ | PROT_WRITE | PROT_EXEC);
// #if defined(__linux__)
    // _flush_cache(start, (long)end - (long)start, ICACHE);
// #endif
// }

#define jit_get_cpu			jit_get_cpu
__jit_constructor static void
jit_get_cpu(void)
{
#if defined(__linux__)
    /* adapted from <gcc-base>/gcc/config/mips/driver-native.c */
    char	 buf[128];
    FILE	*fp;
    static int	 initialized;

    if (initialized)
	return;
    initialized = 1;
    if ((fp = fopen ("/proc/cpuinfo", "r")) == NULL)
	return;

    while (fgets(buf, sizeof (buf), fp)) {
	if (strncmp(buf, "cpu model", sizeof("cpu model") - 1) == 0) {
	    if (strstr(buf, "Godson2 V0.2") ||
		strstr(buf, "Loongson-2 V0.2")) {
		/* loongson2e */
		jit_cpu.mips64 = 1;
	    }
	    else if (strstr(buf, "Godson2 V0.3") ||
		     strstr(buf, "Loongson-2 V0.3")) {
		/* loongson2f */
		jit_cpu.mips64	= 1;
	    }
	    else if (strstr(buf, "SiByte SB1"))
		/* sb1 */;
	    else if(strstr (buf, "R5000"))
		/* r5000 */;
	    else if(strstr(buf, "Octeon"))
		/* octeon */;
	    break;
	}
    }
    fclose(fp);
#endif

#if __WORDSIZE == 64
    jit_cpu.algndbl = 1;
#endif
}

#endif /* __lightning_funcs_h */
