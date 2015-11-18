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

#include "math.h"
#include "math_neon.h"
 
/* 
TanH = (e^x - e^-x) / (e^x + e^-x)
TanH = (e^x - e^-x)(e^x) / (e^x + e^-x)(e^x)
TanH = (e^2x - 1) / (e^2x + 1)

*/
 
float tanhf_c(float x)
{
	float a, b, c;
	int m;
	union{
		float 	f;
		int 	i;
	} xx;
	
	x = 2.0f * x;
	a = expf_c(x);
	c = a + 1.0f;
		
	//reciporical approx.
	xx.f = c;
	m = 0x3F800000 - (xx.i & 0x7F800000);
	xx.i = xx.i + m;
	xx.f = 1.41176471f - 0.47058824f * xx.f;
	xx.i = xx.i + m;
	b = 2.0 - xx.f * c;
	xx.f = xx.f * b;	
	b = 2.0 - xx.f * c;
	xx.f = xx.f * b;
	c = a - 1.0;
	xx.f *= c;
	return xx.f;
}


float tanhf_neon_hfp(float x)
{
#ifdef __MATH_NEON
	asm volatile ("vadd.f32 d0, d0, d0 		\n\t");
	expf_neon_hfp(x);
	asm volatile (
	"vmov.f32 		d2, #1.0 				\n\t"
	"vsub.f32 		d3, d0, d2 				\n\t"
	"vadd.f32 		d0, d0, d2 				\n\t"

	"vrecpe.f32		d1, d0					\n\t"	//d1 = ~ 1 / d0; 
	"vrecps.f32		d2, d1, d0				\n\t"	//d2 = 2.0 - d1 * d0; 
	"vmul.f32		d1, d1, d2				\n\t"	//d1 = d1 * d2; 
	"vrecps.f32		d2, d1, d0				\n\t"	//d2 = 2.0 - d1 * d0; 
	"vmul.f32		d0, d1, d2				\n\t"	//d0 = d1 * d2; 
	"vmul.f32		d0, d0, d3				\n\t"	//d0 = d0 * d3; 	
	::: "d0", "d1", "d2", "d3"
	);	
#endif
}

float tanhf_neon_sfp(float x)
{
#ifdef __MATH_NEON
	asm volatile ("vmov.f32 s0, r0 		\n\t");
	tanhf_neon_hfp(x);
	asm volatile ("vmov.f32 r0, s0 		\n\t");
#else
	return tanhf_c(x);
#endif
};

