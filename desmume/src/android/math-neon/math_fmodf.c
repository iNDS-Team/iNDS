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
Assumes the floating point value |x / y| < 2,147,483,648
*/

#include "math_neon.h"

float fmodf_c(float x, float y)
{
	int n;
	union {
		float f;
		int   i;
	} yinv;
	float a;
	
	//fast reciporical approximation (4x Newton)
	yinv.f = y;
	n = 0x3F800000 - (yinv.i & 0x7F800000);
	yinv.i = yinv.i + n;
	yinv.f = 1.41176471f - 0.47058824f * yinv.f;
	yinv.i = yinv.i + n;
	a = 2.0 - yinv.f * y;
	yinv.f = yinv.f * a;	
	a = 2.0 - yinv.f * y;
	yinv.f = yinv.f * a;
	a = 2.0 - yinv.f * y;
	yinv.f = yinv.f * a;
	a = 2.0 - yinv.f * y;
	yinv.f = yinv.f * a;
	
	n = (int)(x * yinv.f);
	x = x - ((float)n) * y;
	return x;
}


float fmodf_neon_hfp(float x, float y)
{
#ifdef __MATH_NEON
	asm volatile (
	"vdup.f32 		d1, d0[1]					\n\t"	//d1[0] = y
	"vdup.f32 		d0, d0[0]					\n\t"	//d1[0] = y
	
	//fast reciporical approximation
	"vrecpe.f32 	d2, d1					\n\t"	//d2 = ~1.0 / d1
	"vrecps.f32		d3, d2, d1				\n\t"	//d3 = 2.0 - d2 * d1; 
	"vmul.f32		d2, d2, d3				\n\t"	//d2 = d2 * d3; 
	"vrecps.f32		d3, d2, d1				\n\t"	//d3 = 2.0 - d2 * d1; 
	"vmul.f32		d2, d2, d3				\n\t"	//d2 = d2 * d3; 
	"vrecps.f32		d3, d2, d1				\n\t"	//d3 = 2.0 - d2 * d1; 
	"vmul.f32		d2, d2, d3				\n\t"	//d2 = d2 * d3; 
	"vrecps.f32		d3, d2, d1				\n\t"	//d3 = 2.0 - d2 * d1; 
	"vmul.f32		d2, d2, d3				\n\t"	//d2 = d2 * d3; 

	"vmul.f32		d2, d2, d0				\n\t"	//d2 = d2 * d0; 
	"vcvt.s32.f32	d2, d2					\n\t"	//d2 = (int) d2; 
	"vcvt.f32.s32	d2, d2					\n\t"	//d2 = (float) d2; 
	"vmls.f32		d0, d1, d2				\n\t"	//d0 = d0 - d1 * d2; 

	::: "d0", "d1", "d2", "d3"
	);
#endif
}


float fmodf_neon_sfp(float x, float y)
{
#ifdef __MATH_NEON
	asm volatile ("vmov.f32 s0, r0 		\n\t");
	asm volatile ("vmov.f32 s1, r1 		\n\t");
	fmodf_neon_hfp(x, y);
	asm volatile ("vmov.f32 r0, s0 		\n\t");
#else
	return fmodf_c(x,y);
#endif
};
