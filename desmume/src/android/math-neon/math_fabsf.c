/*
Math-NEON:  Neon Optimised Math Library based on cmath
Contact:    lachlan.ts@gmail.com
Copyright (C) 2009  Lachlan Tychsen - Smith aka Adventus

This library is free software; you can redistribute it and/or
modify it under the terms of the GNU Lesser General Public
License as published by the Free Software Foundation; either
version 3 of the License, or (at your option) any later version.

This library is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
Lesser General Public License for more details.

You should have received a copy of the GNU Lesser General Public
License along with this library; if not, write to the Free
Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/

#include "math_neon.h"

	
float fabsf_c(float x)
{
	union {
		int i;
		float f;
	} xx;

	xx.f = x;
	xx.i = xx.i & 0x7FFFFFFF;
	return xx.f;
}

float fabsf_neon_hfp(float x)
{
#ifdef __MATH_NEON
	asm volatile (
	"fabss	 		s0, s0					\n\t"	//s0 = fabs(s0)
	);
#endif
}

float fabsf_neon_sfp(float x)
{
#ifdef __MATH_NEON
	asm volatile (
	"bic	 		r0, r0, #0x80000000		\n\t"	//r0 = r0 & ~(1 << 31)
	);
#else
	return fabsf_c(x);
#endif
}
