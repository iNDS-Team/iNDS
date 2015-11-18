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

const float __sinfv_rng[2] = {
	2.0 / M_PI,
	M_PI / 2.0, 
};

const float __sinfv_lut[4] = {
	-0.00018365f,	//p7
	-0.16664831f,	//p3
	+0.00830636f,	//p5
	+0.99999661f,	//p1
};

void sinfv_c(float *x, int n, float *r)
{
	union {
		float 	f;
		int 	i;
	} ax, bx;
	
	float aa, ab, ba, bb, axx, bxx;
	int am, bm, an, bn;

	if (n & 0x1) {
		*r++ = sinf_c(*x++);
		n--;
	}

	float rng0 = __sinfv_rng[0];
	float rng1 = __sinfv_rng[1];

	while(n > 0){
		
		float x0 = *x++;
		float x1 = *x++;
		
		ax.f = fabsf(x0);
		bx.f = fabsf(x1);

		//Range Reduction:
		am = (int) (ax.f * rng0);	
		bm = (int) (bx.f * rng0);	
		
		ax.f = ax.f - (((float)am) * rng1);
		bx.f = bx.f - (((float)bm) * rng1);

		//Test Quadrant
		an = am & 1;
		bn = bm & 1;
		ax.f = ax.f - an * rng1;
		bx.f = bx.f - bn * rng1;
		am = (am & 2) >> 1;
		bm = (bm & 2) >> 1;
		ax.i = ax.i ^ ((an ^ am ^ (x0 < 0)) << 31);
		bx.i = bx.i ^ ((bn ^ bm ^ (x1 < 0)) << 31);
			
		//Taylor Polynomial (Estrins)
		axx = ax.f * ax.f;	
		bxx = bx.f * bx.f;	
		aa = (__sinfv_lut[0] * ax.f) * axx + (__sinfv_lut[2] * ax.f);
		ba = (__sinfv_lut[0] * bx.f) * bxx + (__sinfv_lut[2] * bx.f);
		ab = (__sinfv_lut[1] * ax.f) * axx + (__sinfv_lut[3] * ax.f);
		bb = (__sinfv_lut[1] * bx.f) * bxx + (__sinfv_lut[3] * bx.f);
		axx = axx * axx;
		bxx = bxx * bxx;
		*r++ = ab + aa * axx;
		*r++ = bb + ba * bxx;
		n -= 2;
	}
	
	
}

void sinfv_neon(float *x, int n, float *r)
{
#ifdef __MATH_NEON
	asm volatile (""
	:
	:"r"(x), "r"(n)
	);
#else
	sinfv_c(x, n, r);
#endif
}
