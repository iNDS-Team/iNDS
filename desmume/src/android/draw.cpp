#include "main.h"
#include "../NDSSystem.h"
#include "video.h"
#include <android/bitmap.h>

extern VideoInfo video;

#define DOBITMAPIMPL 	if(pixelFormat == ANDROID_BITMAP_FORMAT_RGBA_8888) \
	{ \
		u32* src = (u32*)pixels; \
		src += (verticalOffset * (rotate ? height : width)); \
		if(video.currentfilter == VideoInfo::NONE) \
		{ \
			if(rotate) \
			{ \
				for(int y = 0 ; y < height ; ++y) \
				{ \
					u32* destline = (u32*)dest; \
					u32* srccol = src + (height - y - 1); \
					for(int x = 0 ; x < width ; ++x)  \
					{ \
						*destline++ = *srccol; \
						srccol += height; \
					} \
					dest += stride; \
				} \
			} \
			else \
			{ \
				if(stride == width * sizeof(u32)) \
					memcpy(dest, src, width * height * sizeof(u32)); \
				else \
				{ \
					for(int y = 0 ; y < height ; ++y) \
					{ \
						memcpy(dest, &src[y * width], width * sizeof(u32)); \
						dest += stride; \
					} \
				} \
			} \
		} \
		else \
		{ \
			if(rotate) \
			{ \
				for(int y = 0 ; y < height ; ++y) \
				{ \
					u32* destline = (u32*)dest; \
					u32* srccol = src + (height - y - 1); \
					for(int x = 0 ; x < width ; ++x)  \
					{ \
						*destline++ = 0xFF000000 | *srccol; \
						srccol += height; \
					} \
					dest += stride; \
				} \
			} \
			else \
			{ \
				for(int y = 0 ; y < height ; ++y) \
				{ \
					u32* destline = (u32*)dest; \
					for(int x = 0 ; x < width ; ++x) \
						*destline++ = 0xFF000000 | *src++; \
					dest += stride; \
				} \
			} \
		} \
	} \
	else \
	{ \
		u16* src = (u16*)pixels; \
		src += (verticalOffset * (rotate ? height : width)); \
		if(rotate) \
		{ \
			for(int y = 0 ; y < height ; ++y) \
			{ \
				u16* destline = (u16*)dest; \
				u16* srccol = src + (height - y - 1); \
				for(int x = 0 ; x < width ; ++x)  \
				{ \
					*destline++ = *srccol; \
					srccol += height; \
				} \
				dest += stride; \
			} \
		} \
		else \
		{ \
			if(stride == width * sizeof(u16)) \
				memcpy(dest, src, width * height * sizeof(u16)); \
			else \
			{ \
				for(int y = 0 ; y < height ; ++y) \
				{ \
					memcpy(dest, &src[y * width], width * sizeof(u16)); \
					dest += stride; \
				} \
			} \
		} \
	}

//Since the loops will have constant numbers of iterations at compile times and the pointers are restricted, gcc should be able to use the NEON instruction set nicely here
template<int width, int height> void doBitmapDrawTemplate(u8* __restrict__ pixels, u8* __restrict__ dest, int stride, int pixelFormat, int verticalOffset, bool rotate)
{
	DOBITMAPIMPL
}

void doBitmapDrawStandard(u8* __restrict__ pixels, u8* __restrict__ dest, int width, int height, int stride, int pixelFormat, int verticalOffset, bool rotate)
{
	DOBITMAPIMPL
}
	
void doBitmapDraw(u8* pixels, u8* dest, int width, int height, int stride, int pixelFormat, int verticalOffset, bool rotate)
{
	if(width == 256 && height == 192)
		doBitmapDrawTemplate<256,192>(pixels,dest,stride,pixelFormat,verticalOffset,rotate);
	else
		doBitmapDrawStandard(pixels,dest,width,height,stride,pixelFormat,verticalOffset,rotate);
}