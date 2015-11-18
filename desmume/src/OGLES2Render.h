/*
	Copyright (C) 2006 yopyop
	Copyright (C) 2006-2007 shash
	Copyright (C) 2008-2013 DeSmuME team

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

#ifndef OGLES2RENDER_H
#define OGLES2RENDER_H

#include <queue>
#include <set>
#include <string>
#include "render3D.h"
#include "types.h"

#include <GLES2/gl2.h>
#include <GLES2/gl2ext.h>
#include <EGL/egl.h>

#define OGLEXT(procPtr, func)		procPtr func = NULL;
#define INITOGLEXT(procPtr, func)	func = (procPtr)eglGetProcAddress(#func);
#define EXTERNOGLEXT(procPtr, func)	extern procPtr func;

// VAO
EXTERNOGLEXT(PFNGLGENVERTEXARRAYSOESPROC, glGenVertexArraysOES)
EXTERNOGLEXT(PFNGLDELETEVERTEXARRAYSOESPROC, glDeleteVertexArraysOES)
EXTERNOGLEXT(PFNGLBINDVERTEXARRAYOESPROC, glBindVertexArrayOES)

// VBO
EXTERNOGLEXT(PFNGLMAPBUFFEROESPROC, glMapBufferOES)
EXTERNOGLEXT(PFNGLUNMAPBUFFEROESPROC, glUnmapBufferOES)

// Define the minimum required OpenGL version for the driver to support
#define OGLRENDER_MINIMUM_DRIVER_VERSION_REQUIRED_MAJOR			2
#define OGLRENDER_MINIMUM_DRIVER_VERSION_REQUIRED_MINOR			0

#define OGLRENDER_MAX_MULTISAMPLES			16
#define OGLRENDER_VERT_INDEX_BUFFER_COUNT	131072

enum OGLVertexAttributeID
{
	OGLVertexAttributeID_Position	= 0,
	OGLVertexAttributeID_TexCoord0	= 8,
	OGLVertexAttributeID_Color		= 3,
};

enum OGLTextureUnitID
{
	// Main textures will always be on texture unit 0.
	OGLTextureUnitID_ToonTable = 1,
	OGLTextureUnitID_ClearImage
};

enum OGLErrorCode
{
	OGLERROR_NOERR = RENDER3DERROR_NOERR,
	
	OGLERROR_FEATURE_UNSUPPORTED,
	OGLERROR_VBO_UNSUPPORTED,
	OGLERROR_PBO_UNSUPPORTED,
	OGLERROR_SHADER_UNSUPPORTED,
	OGLERROR_VAO_UNSUPPORTED,
	OGLERROR_FBO_UNSUPPORTED,
	OGLERROR_MULTISAMPLED_FBO_UNSUPPORTED,
	
	OGLERROR_VERTEX_SHADER_PROGRAM_LOAD_ERROR,
	OGLERROR_FRAGMENT_SHADER_PROGRAM_LOAD_ERROR,
	OGLERROR_SHADER_CREATE_ERROR,
	
	OGLERROR_FBO_CREATE_ERROR
};

struct OGLESRenderRef
{	
	// OpenGL Feature Support
	GLint stateTexMirroredRepeat;
	
	// VBO
	GLuint vboVertexID;
	GLuint iboIndexID;
	
	// FBO
	GLuint texClearImageColorID;
	GLuint texClearImageDepthStencilID;
	GLuint fboClearImageID;
	
	GLuint rboFinalOutputColorID;
	GLuint rboFinalOutputDepthStencilID;
	GLuint fboFinalOutputID;
	
	GLuint selectedRenderingFBO;
	
	// Shader states
	GLuint vertexShaderID;
	GLuint fragmentShaderID;
	GLuint shaderProgram;
	
	GLint uniformPolyID;
	GLint uniformPolyAlpha;
	GLint uniformTexScale;
	GLint uniformHasTexture;
	GLint uniformPolygonMode;
	GLint uniformToonShadingMode;
	GLint uniformWBuffer;
	GLint uniformEnableAlphaTest;
	GLint uniformAlphaTestRef;
	
	GLuint texToonTableID;
	
	// VAO
	GLuint vaoMainStatesID;
	
	// Textures
	std::queue<GLuint> freeTextureIDs;
	
	// Client-side Buffers
	DS_ALIGN(16) GLushort vertIndexBuffer[OGLRENDER_VERT_INDEX_BUFFER_COUNT];
};

struct GFX3D_State;
struct VERTLIST;
struct POLYLIST;
struct INDEXLIST;
struct POLY;
class TexCacheItem;
class OpenGLESRenderer;

extern GPU3DInterface gpu3Dgles2;

//This is called by OGLRender whenever it initializes.
//Platforms, please be sure to set this up.
//return true if you successfully init.
extern bool (*oglrender_init)();

//This is called by OGLRender before it uses opengl.
//return true if youre OK with using opengl
extern bool (*oglrender_beginOpenGL)();

//This is called by OGLRender after it is done using opengl.
extern void (*oglrender_endOpenGL)();

// Lookup Tables
extern CACHE_ALIGN GLuint dsDepthToD24S8_LUT[32768];
extern const GLfloat divide5bitBy31_LUT[32];

//FORCEINLINE u32 BGRA8888_32_To_RGBA6665_32(const u32 srcPix);
//FORCEINLINE u32 BGRA8888_32Rev_To_RGBA6665_32Rev(const u32 srcPix);
FORCEINLINE u32 RGBA8888_32_To_RGBA6665_32Rev(const u32 srcPix);
FORCEINLINE u32 RGBA8888_32Rev_To_RGBA6665_32Rev(const u32 srcPix);
bool IsVersionSupported(unsigned int checkVersionMajor, unsigned int checkVersionMinor);

class OpenGLESRenderer : public Render3D
{
private:
	// Driver's OpenGL Version
	unsigned int versionMajor;
	unsigned int versionMinor;
	
protected:
	// OpenGL-specific References
	OGLESRenderRef *ref;
	
	// OpenGL Feature Support
	bool isFBOSupported;
	bool isVAOSupported;
	
	// Textures
	TexCacheItem *currTexture;
	
	u32 currentToonTable32[32];
	bool toonTableNeedsUpdate;
	
	DS_ALIGN(16) u32 GPU_screen3D[2][256 * 192 * sizeof(u32)];
	bool gpuScreen3DHasNewData[2];
	unsigned int doubleBufferIndex;
	u8 clearImageStencilValue;
	
	// OpenGL-specific methods
	virtual Render3DError CreateVBOs() = 0;
	virtual void DestroyVBOs() = 0;
	virtual Render3DError CreateFBOs() = 0;
	virtual void DestroyFBOs() = 0;
	virtual Render3DError CreateShaders(const std::string *vertexShaderProgram, const std::string *fragmentShaderProgram) = 0;
	virtual void DestroyShaders() = 0;
	virtual Render3DError CreateVAOs() = 0;
	virtual void DestroyVAOs() = 0;
	virtual Render3DError InitTextures() = 0;
	virtual Render3DError InitFinalRenderStates(const std::set<std::string> *oglExtensionSet) = 0;
	virtual Render3DError InitTables() = 0;
	
	virtual Render3DError LoadShaderPrograms(std::string *outVertexShaderProgram, std::string *outFragmentShaderProgram) = 0;
	virtual Render3DError SetupShaderIO() = 0;
	virtual Render3DError CreateToonTable() = 0;
	virtual Render3DError DestroyToonTable() = 0;
	virtual Render3DError UploadToonTable(const GLuint *toonTableBuffer) = 0;
	virtual Render3DError CreateClearImage() = 0;
	virtual Render3DError DestroyClearImage() = 0;
	virtual Render3DError UploadClearImage(const GLushort *clearImageColorBuffer, const GLint *clearImageDepthBuffer) = 0;
	
	virtual void GetExtensionSet(std::set<std::string> *oglExtensionSet) = 0;
	virtual Render3DError ExpandFreeTextures() = 0;
	virtual Render3DError SetupVertices(const VERTLIST *vertList, const POLYLIST *polyList, const INDEXLIST *indexList, GLushort *outIndexBuffer, unsigned int *outIndexCount) = 0;
	virtual Render3DError EnableVertexAttributes(const VERTLIST *vertList, const GLushort *indexBuffer, const unsigned int vertIndexCount) = 0;
	virtual Render3DError DisableVertexAttributes() = 0;
	virtual Render3DError SelectRenderingFramebuffer() = 0;
	virtual Render3DError ReadBackPixels() = 0;
	
	// Base rendering methods
	virtual Render3DError BeginRender(const GFX3D_State *renderState) = 0;
	virtual Render3DError PreRender(const GFX3D_State *renderState, const VERTLIST *vertList, const POLYLIST *polyList, const INDEXLIST *indexList) = 0;
	virtual Render3DError DoRender(const GFX3D_State *renderState, const VERTLIST *vertList, const POLYLIST *polyList, const INDEXLIST *indexList) = 0;
	virtual Render3DError PostRender() = 0;
	virtual Render3DError EndRender(const u64 frameCount) = 0;
	
	virtual Render3DError UpdateClearImage(const u16 *__restrict colorBuffer, const u16 *__restrict depthBuffer, const u8 clearStencil, const u8 xScroll, const u8 yScroll) = 0;
	virtual Render3DError UpdateToonTable(const u16 *toonTableBuffer) = 0;
	
	virtual Render3DError ClearUsingImage() const = 0;
	virtual Render3DError ClearUsingValues(const u8 r, const u8 g, const u8 b, const u8 a, const u32 clearDepth, const u8 clearStencil) const = 0;
	
	virtual Render3DError SetupPolygon(const POLY *thePoly) = 0;
	virtual Render3DError SetupTexture(const POLY *thePoly, bool enableTexturing) = 0;
	virtual Render3DError SetupViewport(const POLY *thePoly) = 0;
	
public:
	OpenGLESRenderer();
	virtual ~OpenGLESRenderer() {};
	
	virtual Render3DError InitExtensions() = 0;
	virtual Render3DError Reset() = 0;
	virtual Render3DError RenderFinish() = 0;
	
	virtual Render3DError DeleteTexture(const TexCacheItem *item) = 0;
	
	bool IsExtensionPresent(const std::set<std::string> *oglExtensionSet, const std::string extensionName) const;
	bool ValidateShaderCompile(GLuint theShader) const;
	bool ValidateShaderProgramLink(GLuint theProgram) const;
	void GetVersion(unsigned int *major, unsigned int *minor) const;
	void SetVersion(unsigned int major, unsigned int minor);
	void ConvertFramebuffer(const u32 *__restrict srcBuffer, u32 *dstBuffer);
};

class OpenGLES2Renderer : public OpenGLESRenderer
{
protected:
	// OpenGL-specific methods
	virtual Render3DError CreateVBOs();
	virtual void DestroyVBOs();
	virtual Render3DError CreateFBOs();
	virtual void DestroyFBOs();
	virtual Render3DError CreateShaders(const std::string *vertexShaderProgram, const std::string *fragmentShaderProgram);
	virtual void DestroyShaders();
	virtual Render3DError CreateVAOs();
	virtual void DestroyVAOs();
	virtual Render3DError InitTextures();
	virtual Render3DError InitFinalRenderStates(const std::set<std::string> *oglExtensionSet);
	virtual Render3DError InitTables();
	
	virtual Render3DError LoadShaderPrograms(std::string *outVertexShaderProgram, std::string *outFragmentShaderProgram);
	virtual Render3DError SetupShaderIO();
	virtual Render3DError CreateToonTable();
	virtual Render3DError DestroyToonTable();
	virtual Render3DError UploadToonTable(const GLuint *toonTableBuffer);
	virtual Render3DError CreateClearImage();
	virtual Render3DError DestroyClearImage();
	virtual Render3DError UploadClearImage(const GLushort *clearImageColorBuffer, const GLint *clearImageDepthBuffer);
	
	virtual void GetExtensionSet(std::set<std::string> *oglExtensionSet);
	virtual Render3DError ExpandFreeTextures();
	virtual Render3DError SetupVertices(const VERTLIST *vertList, const POLYLIST *polyList, const INDEXLIST *indexList, GLushort *outIndexBuffer, unsigned int *outIndexCount);
	virtual Render3DError EnableVertexAttributes(const VERTLIST *vertList, const GLushort *indexBuffer, const unsigned int vertIndexCount);
	virtual Render3DError DisableVertexAttributes();
	virtual Render3DError SelectRenderingFramebuffer();
	virtual Render3DError ReadBackPixels();
	
	// Base rendering methods
	virtual Render3DError BeginRender(const GFX3D_State *renderState);
	virtual Render3DError PreRender(const GFX3D_State *renderState, const VERTLIST *vertList, const POLYLIST *polyList, const INDEXLIST *indexList);
	virtual Render3DError DoRender(const GFX3D_State *renderState, const VERTLIST *vertList, const POLYLIST *polyList, const INDEXLIST *indexList);
	virtual Render3DError PostRender();
	virtual Render3DError EndRender(const u64 frameCount);
	
	virtual Render3DError UpdateClearImage(const u16 *__restrict colorBuffer, const u16 *__restrict depthBuffer, const u8 clearStencil, const u8 xScroll, const u8 yScroll);
	virtual Render3DError UpdateToonTable(const u16 *toonTableBuffer);
	
	virtual Render3DError ClearUsingImage() const;
	virtual Render3DError ClearUsingValues(const u8 r, const u8 g, const u8 b, const u8 a, const u32 clearDepth, const u8 clearStencil) const;
	
	virtual Render3DError SetupPolygon(const POLY *thePoly);
	virtual Render3DError SetupTexture(const POLY *thePoly, bool enableTexturing);
	virtual Render3DError SetupViewport(const POLY *thePoly);
	
public:
	OpenGLES2Renderer();
	~OpenGLES2Renderer();
	
	virtual Render3DError InitExtensions();
	virtual Render3DError Reset();
	virtual Render3DError RenderFinish();
	
	virtual Render3DError DeleteTexture(const TexCacheItem *item);
};

#endif
