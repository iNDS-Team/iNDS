/*
	Copyright (C) 2012 Jeffrey Quesnelle

	This file is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 2 of the License, or
	(at your option) any later version.

	This file is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with the this software.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "neontest.h"
extern "C" {
#include "math-neon/math_neon.h"
}
#include "../matrix.h"
#include "main.h"
#include <stdio.h>
#include <string.h>

template<int WIDTH, int HEIGHT> void printmatrix(float* matrix)
{
	for(int y = 0 ; y < HEIGHT ; ++y )
	{
		char line[1024], work[1024];
		strcpy(line,"|");
		for(int x = 0 ; x < WIDTH ; ++x)
		{
			sprintf(work, "%.2f%s", matrix[(y * WIDTH) + x], x == WIDTH - 1 ? "|" : ", ");
			strcat(line, work);
		}
		LOGI("%s", line);
	}
}

void MatrixMultVec4x4Neon(float *matrix, float *vecPtr)
{
	matvec4_neon(matrix, vecPtr, vecPtr);
}

void MatrixMultVec3x3Neon(float *matrix, float *vecPtr)
{
	//fails... looks like it's still packed like a 4x4 in matrix.cpp
	matvec3_neon(matrix, vecPtr, vecPtr);
}

void MatrixMultiplyNeon(float * matrix, float * rightMatrix)
{
	float ret[16];
	matmul4_neon(matrix, rightMatrix, ret);
	memcpy(matrix, ret, sizeof(float)*16);
}

void MatrixMultVec4x4_M2Neon(float *matrix, float *vecPtr)
{
	matvec4_neon(matrix+16, vecPtr, vecPtr);
	matvec4_neon(matrix, vecPtr, vecPtr);
}

void testmultvec4x4()
{
	LOGI("MatrixMultVec4x4");
	float matrix1[] = {
		1.0f, 2.0f, 3.0f, 4.0f,
		5.0f, 6.0f, 7.0f, 8.0f,
		9.0f, 10.0f, 11.0f, 12.0f,
		13.0f, 14.0f, 15.0f, 16.0f
	};
	
	float vector[] = {
		M_PI,
		M_E,
		M_LN2,
		M_SQRT2
	};
	
	float vectorc[4], vectorneon[4];
	
	memcpy(vectorc, vector, sizeof(float)* 4 );
	memcpy(vectorneon, vector, sizeof(float)* 4 );
	
	printmatrix<4,4>(matrix1);
	LOGI("*");
	printmatrix<4,1>(vectorc);
	MatrixMultVec4x4(matrix1, vectorc);
	LOGI("=");
	printmatrix<4,1>(vectorc);	
	
	printmatrix<4,4>(matrix1);
	LOGI("*");
	printmatrix<4,1>(vectorneon);
	MatrixMultVec4x4Neon(matrix1, vectorneon);
	LOGI("=");
	printmatrix<4,1>(vectorneon);	
}
void testmultvec3x3()
{
	LOGI("MatrixMultVec3x3");
	float matrix1[] = {
		1.0f, 2.0f, 3.0f, 
		5.0f, 6.0f, 7.0f,
		9.0f, 10.0f, 11.0f, 
	};
	
	float vector[] = {
		M_PI,
		M_E,
		M_LN2,
	};
	
	float vectorc[3], vectorneon[3];
	
	memcpy(vectorc, vector, sizeof(float)* 3 );
	memcpy(vectorneon, vector, sizeof(float)* 3 );
	
	printmatrix<3,3>(matrix1);
	LOGI("*");
	printmatrix<3,1>(vectorc);
	MatrixMultVec3x3(matrix1, vectorc);
	LOGI("=");
	printmatrix<3,1>(vectorc);	
	
	printmatrix<3,3>(matrix1);
	LOGI("*");
	printmatrix<3,1>(vectorneon);
	MatrixMultVec3x3Neon(matrix1, vectorneon);
	LOGI("=");
	printmatrix<3,1>(vectorneon);	
}

void testmatrixmul()
{
	LOGI("MatrixMultiply");
	
	float matrix1[] = {
		1.0f, 2.0f, 3.0f, 4.0f,
		5.0f, 6.0f, 7.0f, 8.0f,
		9.0f, 10.0f, 11.0f, 12.0f,
		13.0f, 14.0f, 15.0f, 16.0f
	};
	
	float matrix2[] = {
		17.0f, 18.0f, 19.0f, 20.0f,
		21.0f, 22.0f, 23.0f, 24.0f,
		25.0f, 26.0f, 27.0f, 28.0f,
		29.0f, 30.0f, 31.0f, 32.0f
	};
	
	float matrixc[16], matrixneon[16];
	
	memcpy(matrixc, matrix1, sizeof(float)* 16 );
	memcpy(matrixneon, matrix1, sizeof(float)* 16 );
	
	printmatrix<4,4>(matrixc);
	LOGI("*");
	printmatrix<4,4>(matrix2);
	MatrixMultiply(matrixc, matrix2);
	LOGI("=");
	printmatrix<4,4>(matrixc);	
	
	printmatrix<4,4>(matrixneon);
	LOGI("*");
	printmatrix<4,4>(matrix2);
	MatrixMultiplyNeon(matrixneon, matrix2);
	LOGI("=");
	printmatrix<4,4>(matrixneon);	
}

void testmultvec4x4_m2()
{
	LOGI("MatrixMultVec4x4_M2");
	float matrix1[] = {
		1.0f, 2.0f, 3.0f, 4.0f,
		5.0f, 6.0f, 7.0f, 8.0f,
		9.0f, 10.0f, 11.0f, 12.0f,
		13.0f, 14.0f, 15.0f, 16.0f,
		17.0f, 18.0f, 19.0f, 20.0f,
		21.0f, 22.0f, 23.0f, 24.0f,
		25.0f, 26.0f, 27.0f, 28.0f,
		29.0f, 30.0f, 31.0f, 32.0f
	};
	
	float vector[] = {
		M_PI,
		M_E,
		M_LN2,
		M_SQRT2
	};
	
	float vectorc[4], vectorneon[4];
	
	memcpy(vectorc, vector, sizeof(float)* 4 );
	memcpy(vectorneon, vector, sizeof(float)* 4 );
	
	printmatrix<4,4>(matrix1);
	LOGI("*");
	printmatrix<4,4>(matrix1+16);
	LOGI("*");
	printmatrix<4,1>(vectorc);
	MatrixMultVec4x4_M2(matrix1, vectorc);
	LOGI("=");
	printmatrix<4,1>(vectorc);	
	
	printmatrix<4,4>(matrix1);
	LOGI("*");
	printmatrix<4,4>(matrix1+16);
	LOGI("*");
	printmatrix<4,1>(vectorneon);
	MatrixMultVec4x4_M2Neon(matrix1, vectorneon);
	LOGI("=");
	printmatrix<4,1>(vectorneon);	
}

void neontest()
{
	//testmultvec4x4();
	//testmatrixmul();
	//testmultvec4x4_m2();
	testmultvec3x3();
}