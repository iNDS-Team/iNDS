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

/*
Assumes the floating point value |x| < 2147483648
*/

#include "math.h"
#include "math_neon.h"

float floorf_c(float x)
{
	int n;
	float r;	
	n = (int) x;
	r = (float) n;
	r = r - (r > x);
	return r;
}

float floorf_neon_hfp(float x)
{
#ifdef __MATH_NEON
	asm volatile (
	"vcvt.s32.f32 	d1, d0					\n\t"	//d1 = (int) d0;
	"vcvt.f32.s32 	d1, d1					\n\t"	//d1 = (float) d1;
	"vcgt.f32 		d0, d1, d0				\n\t"	//d0 = (d1 > d0);
	"vshr.u32 		d0, #31					\n\t"	//d0 = d0 >> 31;
	"vcvt.f32.u32 	d0, d0					\n\t"	//d0 = (float) d0;
	"vsub.f32 		d0, d1, d0				\n\t"	//d0 = d1 - d0;
	::: "d0", "d1"
	);
#endif
}

float floorf_neon_sfp(float x)
{
#ifdef __MATH_NEON
	asm volatile ("vmov.f32 s0, r0 		\n\t");
	floorf_neon_hfp(x);
	asm volatile ("vmov.f32 r0, s0 		\n\t");
#else
	return floorf_c(x);
#endif
};
