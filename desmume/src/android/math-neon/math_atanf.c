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

const float __atanf_lut[4] = {
	-0.0443265554792128,	//p7
	-0.3258083974640975,	//p3
	+0.1555786518463281,	//p5
	+0.9997878412794807  	//p1
}; 
 
const float __atanf_pi_2 = M_PI_2;
    
float atanf_c(float x)
{

	float a, b, r, xx;
	int m;
	
	union {
		float f;
		int i;
	} xinv, ax;

	ax.f = fabs(x);
	
	//fast inverse approximation (2x newton)
	xinv.f = ax.f;
	m = 0x3F800000 - (xinv.i & 0x7F800000);
	xinv.i = xinv.i + m;
	xinv.f = 1.41176471f - 0.47058824f * xinv.f;
	xinv.i = xinv.i + m;
	b = 2.0 - xinv.f * ax.f;
	xinv.f = xinv.f * b;	
	b = 2.0 - xinv.f * ax.f;
	xinv.f = xinv.f * b;
	
	//if |x| > 1.0 -> ax = -1/ax, r = pi/2
	xinv.f = xinv.f + ax.f;
	a = (ax.f > 1.0f);
	ax.f = ax.f - a * xinv.f;
	r = a * __atanf_pi_2;
	
	//polynomial evaluation
	xx = ax.f * ax.f;	
	a = (__atanf_lut[0] * ax.f) * xx + (__atanf_lut[2] * ax.f);
	b = (__atanf_lut[1] * ax.f) * xx + (__atanf_lut[3] * ax.f);
	xx = xx * xx;
	b = b + a * xx; 
	r = r + b;

	//if x < 0 -> r = -r
	a = 2 * r;
	b = (x < 0.0f);
	r = r - a * b;

	return r;
}


float atanf_neon_hfp(float x)
{
#ifdef __MATH_NEON
	asm volatile (

	"vdup.f32	 	d0, d0[0]				\n\t"	//d0 = {x, x};

	"vdup.f32	 	d4, %1					\n\t"	//d4 = {pi/2, pi/2};
	"vmov.f32	 	d6, d0					\n\t"	//d6 = d0;
	"vabs.f32	 	d0, d0					\n\t"	//d0 = fabs(d0) ;

	//fast reciporical approximation
	"vrecpe.f32		d1, d0					\n\t"	//d1 = ~ 1 / d0; 
	"vrecps.f32		d2, d1, d0				\n\t"	//d2 = 2.0 - d1 * d0; 
	"vmul.f32		d1, d1, d2				\n\t"	//d1 = d1 * d2; 
	"vrecps.f32		d2, d1, d0				\n\t"	//d2 = 2.0 - d1 * d0; 
	"vmul.f32		d1, d1, d2				\n\t"	//d1 = d1 * d2; 

		
	//if |x| > 1.0 -> ax = -1/ax, r = pi/2
	"vadd.f32		d1, d1, d0				\n\t"	//d1 = d1 + d0; 
	"vmov.f32	 	d2, #1.0				\n\t"	//d2 = 1.0;
	"vcgt.f32	 	d3, d0, d2				\n\t"	//d3 = (d0 > d2);
	"vshr.u32	 	d3, #31					\n\t"	//d3 = (d0 > d2);
	"vcvt.f32.u32	d3, d3					\n\t"	//d5 = (float) d3;	
	"vmls.f32		d0, d1, d3[0]			\n\t"	//d0 = d0 - d1 * d3[0]; 	
	"vmul.f32		d7, d4, d3[0] 			\n\t"	//d7 = d5 * d4; 	
	
	//polynomial:
	"vmul.f32 		d2, d0, d0				\n\t"	//d2 = d0*d0 = {ax^2, ax^2}	
	"vld1.32 		{d4, d5}, [%0]			\n\t"	//d4 = {p7, p3}, d5 = {p5, p1}
	"vmul.f32 		d3, d2, d2				\n\t"	//d3 = d2*d2 = {x^4, x^4}		
	"vmul.f32 		q0, q2, d0[0]			\n\t"	//q0 = q2 * d0[0] = {p7x, p3x, p5x, p1x}
	"vmla.f32 		d1, d0, d2[0]			\n\t"	//d1 = d1 + d0*d2[0] = {p5x + p7x^3, p1x + p3x^3}		
	"vmla.f32 		d1, d3, d1[0]			\n\t"	//d1 = d1 + d3*d1[0] = {..., p1x + p3x^3 + p5x^5 + p7x^7}		
	"vadd.f32 		d1, d1, d7				\n\t"	//d1 = d1 + d7		

	"vadd.f32 		d2, d1, d1				\n\t"	//d2 = d1 + d1		
	"vclt.f32	 	d3, d6, #0				\n\t"	//d3 = (d6 < 0)	
	"vshr.u32	 	d3, #31					\n\t"	//d3 = (d0 > d2);
	"vcvt.f32.u32	d3, d3					\n\t"	//d3 = (float) d3	
	"vmls.f32 		d1, d3, d2				\n\t"	//d1 = d1 - d2 * d3;

	"vmov.f32 		s0, s3					\n\t"	//s0 = s3

	:: "r"(__atanf_lut),  "r"(__atanf_pi_2) 
    : "d0", "d1", "d2", "d3", "d4", "d5", "d6", "d7"
	);

#endif
}


float atanf_neon_sfp(float x)
{
#ifdef __MATH_NEON
	asm volatile ("vdup.f32 d0, r0 		\n\t");
	atanf_neon_hfp(x);
	asm volatile ("vmov.f32 r0, s0 		\n\t");
#else
	return atanf_c(x);
#endif
};



