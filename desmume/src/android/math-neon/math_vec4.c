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

#include "arm_neon.h" 

//vec4 scalar product
float 
dot4_c(float v0[4], float v1[4])
{
	float r;
	r = v0[0]*v1[0];
	r += v0[1]*v1[1];
	r += v0[2]*v1[2]; 
	r += v0[3]*v1[3];
	return r;
}

void 
normalize4_c(float v[4], float d[4])
{
	float b, c, x;
	union {
		float 	f;
		int 	i;
	} a;
	
	x = v[0]*v[0];
	x += v[1]*v[1];
	x += v[2]*v[2];
	x += v[3]*v[3];

	//fast invsqrt approx
	a.f = x;
	a.i = 0x5F3759DF - (a.i >> 1);		//VRSQRTE
	c = x * a.f;
	b = (3.0f - c * a.f) * 0.5;		//VRSQRTS
	a.f = a.f * b;		
	c = x * a.f;
	b = (3.0f - c * a.f) * 0.5;
    a.f = a.f * b;	

	d[0] = v[0]*a.f;
	d[1] = v[1]*a.f;
	d[2] = v[2]*a.f;
	d[3] = v[3]*a.f;
}

void 
normalize4_neon(float v[4], float d[4])
{
#ifdef __MATH_NEON
	asm volatile (
	"vld1.32 		{d4, d5}, [%0]			\n\t"	//d2={x0,y0}, d3={z0, w0}
	"vmul.f32 		d0, d4, d4				\n\t"	//d0= d4*d4
	"vmla.f32 		d0, d5, d5				\n\t"	//d0 = d0 + d5*d5 
	"vpadd.f32 		d0, d0					\n\t"	//d0 = d[0] + d[1]
	
	"vmov.f32 		d1, d0					\n\t"	//d1 = d0
	"vrsqrte.f32 	d0, d0					\n\t"	//d0 = ~ 1.0 / sqrt(d0)
	"vmul.f32 		d2, d0, d1				\n\t"	//d2 = d0 * d1
	"vrsqrts.f32 	d3, d2, d0				\n\t"	//d3 = (3 - d0 * d2) / 2 	
	"vmul.f32 		d0, d0, d3				\n\t"	//d0 = d0 * d3
	"vmul.f32 		d2, d0, d1				\n\t"	//d2 = d0 * d1	
	"vrsqrts.f32 	d3, d2, d0				\n\t"	//d4 = (3 - d0 * d3) / 2	
	"vmul.f32 		d0, d0, d3				\n\t"	//d0 = d0 * d4	

	"vmul.f32 		q2, q2, d0[0]			\n\t"	//d0= d2*d4
	"vst1.32 		{d4, d5}, [%1]			\n\t"	//d2={x0,y0}, d3={z0, w0}
	
	:: "r"(v), "r"(d) 
    : "d0", "d1", "d2", "d3", "d4", "d5", "memory"
	);	
#else
	normalize4_c(v, d);
#endif

}


float 
dot4_neon_hfp(float v0[4], float v1[4])
{
#ifdef __MATH_NEON
	asm volatile (
	"vld1.32 		{d2, d3}, [%0]			\n\t"	//d2={x0,y0}, d3={z0, w0}
	"vld1.32 		{d4, d5}, [%1]			\n\t"	//d4={x1,y1}, d5={z1, w1}
	"vmul.f32 		d0, d2, d4				\n\t"	//d0= d2*d4
	"vmla.f32 		d0, d3, d5				\n\t"	//d0 = d0 + d3*d5 
	"vpadd.f32 		d0, d0					\n\t"	//d0 = d[0] + d[1]
	:: "r"(v0), "r"(v1) : 
	);	
#endif
}

#if 0 //funky ifdef problem... figure out later

float32_t 
dot4_neon(float32x4_t v0, float32x4_t v1)
{	
	float32x2_t a, b, c, d, r;
	a = vget_high_f32(v0);
	b = vget_low_f32(v0);
	c = vget_high_f32(v1);
	d = vget_low_f32(v1);
	
	r = vmul_f32(a, c);
	r = vmla_f32(r, b, d);
	r = vpadd_f32(r, r);
	return vget_lane_f32(r, 0);
}

float 
dot4_neon_sfp(float v0[4], float v1[4])
{
#ifdef __MATH_NEON
	dot4_neon_hfp(v0, v1);
	asm volatile ("vmov.f32 r0, s0 		\n\t");
#else
	return dot4_c(v0, v1);
#endif
};

#endif

