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

#include "OGLES2Render.h"

#include <stdlib.h>
#include <string.h>

#include "debug.h"
#include "gfx3d.h"
#include "NDSSystem.h"
#include "texcache.h"


typedef struct
{
	unsigned int major;
	unsigned int minor;
} OGLVersion;

static OGLVersion _OGLDriverVersion = {0, 0};
static OpenGLESRenderer *_OGLRenderer = NULL;

// Lookup Tables
CACHE_ALIGN GLuint dsDepthToD24S8_LUT[32768] = {0};
const GLfloat divide5bitBy31_LUT[32]	= {0.0, 0.03225806451613, 0.06451612903226, 0.09677419354839,
									   0.1290322580645, 0.1612903225806, 0.1935483870968, 0.2258064516129,
									   0.258064516129, 0.2903225806452, 0.3225806451613, 0.3548387096774,
									   0.3870967741935, 0.4193548387097, 0.4516129032258, 0.4838709677419,
									   0.5161290322581, 0.5483870967742, 0.5806451612903, 0.6129032258065,
									   0.6451612903226, 0.6774193548387, 0.7096774193548, 0.741935483871,
									   0.7741935483871, 0.8064516129032, 0.8387096774194, 0.8709677419355,
									   0.9032258064516, 0.9354838709677, 0.9677419354839, 1.0};

static bool BEGINGL()
{
	if(oglrender_beginOpenGL) 
		return oglrender_beginOpenGL();
	else return true;
}

static void ENDGL()
{
	if(oglrender_endOpenGL) 
		oglrender_endOpenGL();
}

// Function Pointers
bool (*oglrender_init)() = NULL;
bool (*oglrender_beginOpenGL)() = NULL;
void (*oglrender_endOpenGL)() = NULL;

//------------------------------------------------------------

// VAO
OGLEXT(PFNGLGENVERTEXARRAYSOESPROC, glGenVertexArraysOES)
OGLEXT(PFNGLDELETEVERTEXARRAYSOESPROC, glDeleteVertexArraysOES)
OGLEXT(PFNGLBINDVERTEXARRAYOESPROC, glBindVertexArrayOES)

// VBO
OGLEXT(PFNGLMAPBUFFEROESPROC, glMapBufferOES)
OGLEXT(PFNGLUNMAPBUFFEROESPROC, glUnmapBufferOES)

static void OGLES2LoadEntryPoints()
{
	// VAO
	INITOGLEXT(PFNGLGENVERTEXARRAYSOESPROC, glGenVertexArraysOES)
	INITOGLEXT(PFNGLDELETEVERTEXARRAYSOESPROC, glDeleteVertexArraysOES)
	INITOGLEXT(PFNGLBINDVERTEXARRAYOESPROC, glBindVertexArrayOES)

	// VBO
	INITOGLEXT(PFNGLMAPBUFFEROESPROC, glMapBufferOES)
	INITOGLEXT(PFNGLUNMAPBUFFEROESPROC, glUnmapBufferOES)
}

// Vertex Shader GLSL 1.00
static const char *vertexShader_100 = {"\
	attribute vec4 inPosition; \n\
	attribute vec2 inTexCoord0; \n\
	attribute vec3 inColor; \n\
	\n\
	uniform float polyAlpha; \n\
	uniform vec2 texScale; \n\
	\n\
	varying vec4 vtxPosition; \n\
	varying vec2 vtxTexCoord; \n\
	varying vec4 vtxColor; \n\
	\n\
	void main() \n\
	{ \n\
		mat2 texScaleMtx	= mat2(	vec2(texScale.x,        0.0), \n\
									vec2(       0.0, texScale.y)); \n\
		\n\
		vtxPosition = inPosition; \n\
		vtxTexCoord = texScaleMtx * inTexCoord0; \n\
		vtxColor = vec4(inColor * 4.0, polyAlpha); \n\
		\n\
		gl_Position = vtxPosition; \n\
	} \n\
"};

// Fragment Shader GLSL 1.00
static const char *fragmentShader_100 = {"\
	precision mediump float; \n\
	varying vec4 vtxPosition; \n\
	varying vec2 vtxTexCoord; \n\
	varying vec4 vtxColor; \n\
	\n\
	uniform sampler2D texMainRender; \n\
	uniform sampler2D texToonTable; \n\
	uniform int polyID; \n\
	uniform bool hasTexture; \n\
	uniform int polygonMode; \n\
	uniform int toonShadingMode; \n\
	uniform int oglWBuffer; \n\
	uniform bool enableAlphaTest; \n\
	uniform float alphaTestRef; \n\
	\n\
	void main() \n\
	{ \n\
		vec4 texColor = vec4(1.0, 1.0, 1.0, 1.0); \n\
		vec4 fragColor; \n\
		\n\
		if(hasTexture) \n\
		{ \n\
			texColor = texture2D(texMainRender, vtxTexCoord); \n\
		} \n\
		\n\
		fragColor = texColor; \n\
		\n\
		if(polygonMode == 0) \n\
		{ \n\
			fragColor = vtxColor * texColor; \n\
		} \n\
		else if(polygonMode == 1) \n\
		{ \n\
			if (texColor.a == 0.0 || !hasTexture) \n\
			{ \n\
				fragColor.rgb = vtxColor.rgb; \n\
			} \n\
			else if (texColor.a == 1.0) \n\
			{ \n\
				fragColor.rgb = texColor.rgb; \n\
			} \n\
			else \n\
			{ \n\
				fragColor.rgb = texColor.rgb * (1.0-texColor.a) + vtxColor.rgb * texColor.a;\n\
			} \n\
			\n\
			fragColor.a = vtxColor.a; \n\
		} \n\
		else if(polygonMode == 2) \n\
		{ \n\
			if (toonShadingMode == 0) \n\
			{ \n\
				vec3 toonColor = vec3(texture2D(texToonTable, vec2(vtxColor.r,0)).rgb); \n\
				fragColor.rgb = texColor.rgb * toonColor.rgb;\n\
				fragColor.a = texColor.a * vtxColor.a;\n\
			} \n\
			else \n\
			{ \n\
				vec3 toonColor = vec3(texture2D(texToonTable, vec2(vtxColor.r,0)).rgb); \n\
				fragColor.rgb = texColor.rgb * vtxColor.rgb + toonColor.rgb; \n\
				fragColor.a = texColor.a * vtxColor.a; \n\
			} \n\
		} \n\
		else if(polygonMode == 3) \n\
		{ \n\
			if (polyID != 0) \n\
			{ \n\
				fragColor = vtxColor; \n\
			} \n\
		} \n\
		\n\
		if (fragColor.a == 0.0 || (enableAlphaTest && fragColor.a < alphaTestRef)) \n\
		{ \n\
			discard; \n\
		} \n\
		\n\
		gl_FragColor = fragColor; \n\
	} \n\
"};

//FORCEINLINE u32 BGRA8888_32_To_RGBA6665_32(const u32 srcPix)
//{
//	const u32 dstPix = (srcPix >> 2) & 0x3F3F3F3F;
//	
//	return	 (dstPix & 0x0000FF00) << 16 |		// R
//			 (dstPix & 0x00FF0000)       |		// G
//			 (dstPix & 0xFF000000) >> 16 |		// B
//			((dstPix >> 1) & 0x000000FF);		// A
//}
//
//FORCEINLINE u32 BGRA8888_32Rev_To_RGBA6665_32Rev(const u32 srcPix)
//{
//	const u32 dstPix = (srcPix >> 2) & 0x3F3F3F3F;
//	
//	return	 (dstPix & 0x00FF0000) >> 16 |		// R
//			 (dstPix & 0x0000FF00)       |		// G
//			 (dstPix & 0x000000FF) << 16 |		// B
//			((dstPix >> 1) & 0xFF000000);		// A
//}

FORCEINLINE u32 RGBA8888_32_To_RGBA6665_32Rev(const u32 srcPix)
{
	const u32 dstPix = (srcPix >> 2) & 0x3F3F3F3F;
	
	return	 (dstPix & 0xFF000000) >> 24 |		// R
			 (dstPix & 0x00FF0000) >> 16 |		// G
			 (dstPix & 0x0000FF00) >> 8 |		// B
			((dstPix >> 1) & 0x000000FF) << 24;	// A
}

FORCEINLINE u32 RGBA8888_32Rev_To_RGBA6665_32Rev(const u32 srcPix)
{
	const u32 dstPix = (srcPix >> 2) & 0x3F3F3F3F;
	
	//return	 (dstPix & 0x00FF0000) |		// R
	//		 (dstPix & 0x0000FF00) |		// G
	//		 (dstPix & 0x000000FF) |		// B
	//		((dstPix >> 1) & 0xFF000000);	// A
	return ((dstPix >> 1) & 0xFF000000) | (dstPix & 0x00FFFFFF);
}

//opengl state caching:
//This is of dubious performance assistance, but it is easy to take out so I am leaving it for now.
//every function that is xgl* can be replaced with gl* if we decide to rip this out or if anyone else
//doesnt feel like sticking with it (or if it causes trouble)

struct GLCaps {
	u8 caps[0x100];
	GLCaps() {
		memset(caps,0xFF,sizeof(caps));
	}
};
static GLCaps glcaps;

static void _xglEnable(GLenum cap) {
	cap -= 0x0B00;
	if(glcaps.caps[cap] == 0xFF || glcaps.caps[cap] == 0) {
		glEnable(cap+0x0B00);
		glcaps.caps[cap] = 1;
	}
}

static void _xglDisable(GLenum cap) {
	cap -= 0x0B00;
	if(glcaps.caps[cap]) {
		glDisable(cap+0x0B00);
		glcaps.caps[cap] = 0;
	}
}

#define xglEnable(cap) { \
	CTASSERT((cap-0x0B00)<0x100); \
	_xglEnable(cap); }

#define xglDisable(cap) {\
	CTASSERT((cap-0x0B00)<0x100); \
	_xglDisable(cap); }

bool IsVersionSupported(unsigned int checkVersionMajor, unsigned int checkVersionMinor)
{
	bool result = false;
	
	if ( (_OGLDriverVersion.major > checkVersionMajor) ||
		 (_OGLDriverVersion.major >= checkVersionMajor && _OGLDriverVersion.minor >= checkVersionMinor))
	{
		result = true;
	}
	
	return result;
}

static void texDeleteCallback(TexCacheItem *item)
{
	_OGLRenderer->DeleteTexture(item);
}

static char OGLInit(void)
{
	char result = 0;
	Render3DError error = OGLERROR_NOERR;
	
	if(!oglrender_init)
		return result;
	if(!oglrender_init())
		return result;
	
	result = Default3D_Init();
	if (result == 0)
	{
		return result;
	}

	if(!BEGINGL())
	{
		INFO("OpenGLES2: Could not initialize -- BEGINGL() failed.\n");
		result = 0;
		return result;
	}
	
	// Get OpenGLES2 info
	const char *oglVersionString = (const char *)glGetString(GL_VERSION);
	const char *oglVendorString = (const char *)glGetString(GL_VENDOR);
	const char *oglRendererString = (const char *)glGetString(GL_RENDERER);
	
	// Check the driver's OpenGLES2 version
	_OGLDriverVersion.major = 2;
	_OGLDriverVersion.minor = 0;
	if (!IsVersionSupported(OGLRENDER_MINIMUM_DRIVER_VERSION_REQUIRED_MAJOR, OGLRENDER_MINIMUM_DRIVER_VERSION_REQUIRED_MINOR))
	{
		INFO("OpenGLES2: Driver does not support OpenGLES2 v%u.%u or later. Disabling 3D renderer.\n[ Driver Info -\n    Version: %s\n    Vendor: %s\n    Renderer: %s ]\n",
			 OGLRENDER_MINIMUM_DRIVER_VERSION_REQUIRED_MAJOR, OGLRENDER_MINIMUM_DRIVER_VERSION_REQUIRED_MINOR,
			 oglVersionString, oglVendorString, oglRendererString);
		
		result = 0;
		return result;
	}
	
	// If the renderer doesn't initialize with OpenGLES2 v3.2 or higher, fall back
	// to one of the lower versions.
	if (_OGLRenderer == NULL)
	{
		OGLES2LoadEntryPoints();
		
		if (IsVersionSupported(2, 0))
		{
			_OGLRenderer = new OpenGLES2Renderer;
			_OGLRenderer->SetVersion(2, 0);
		}
	}
	
	if (_OGLRenderer == NULL)
	{
		INFO("OpenGLES2: Renderer did not initialize. Disabling 3D renderer.\n[ Driver Info -\n    Version: %s\n    Vendor: %s\n    Renderer: %s ]\n",
			 oglVersionString, oglVendorString, oglRendererString);
		result = 0;
		return result;
	}
	
	// Initialize OpenGLES2 extensions
	error = _OGLRenderer->InitExtensions();
	if (error != OGLERROR_NOERR)
	{
		if ( IsVersionSupported(2, 0) &&
			(error == OGLERROR_SHADER_CREATE_ERROR ||
			 error == OGLERROR_VERTEX_SHADER_PROGRAM_LOAD_ERROR ||
			 error == OGLERROR_FRAGMENT_SHADER_PROGRAM_LOAD_ERROR) )
		{
			INFO("OpenGLES2: Shaders are not working, even though they should be. Disabling 3D renderer.\n");
			result = 0;
			return result;
		}
	}
	
	// Initialization finished -- reset the renderer
	_OGLRenderer->Reset();
	
	ENDGL();
	
	unsigned int major = 0;
	unsigned int minor = 0;
	_OGLRenderer->GetVersion(&major, &minor);
	
	INFO("OpenGLES2: Renderer initialized successfully (v%u.%u).\n[ Driver Info -\n    Version: %s\n    Vendor: %s\n    Renderer: %s ]\n",
		 major, minor, oglVersionString, oglVendorString, oglRendererString);
	
	return result;
}

static void OGLReset()
{
	if(!BEGINGL())
		return;
	
	_OGLRenderer->Reset();
	
	ENDGL();
}

static void OGLClose()
{
	if(!BEGINGL())
		return;
	
	delete _OGLRenderer;
	_OGLRenderer = NULL;
	
	ENDGL();
	
	Default3D_Close();
}

static void OGLRender()
{
	if(!BEGINGL())
		return;
	
	_OGLRenderer->Render(&gfx3d.renderState, gfx3d.vertlist, gfx3d.polylist, &gfx3d.indexlist, gfx3d.frameCtr);
	
	ENDGL();
}

static void OGLVramReconfigureSignal()
{
	if(!BEGINGL())
		return;
	
	_OGLRenderer->VramReconfigureSignal();
	
	ENDGL();
}

static void OGLRenderFinish()
{
	if(!BEGINGL())
		return;
	
	_OGLRenderer->RenderFinish();
	
	ENDGL();
}

GPU3DInterface gpu3Dgles2 = {
	"OpenGLES2",
	OGLInit,
	OGLReset,
	OGLClose,
	OGLRender,
	OGLRenderFinish,
	OGLVramReconfigureSignal
};

OpenGLESRenderer::OpenGLESRenderer()
{
	versionMajor = 0;
	versionMinor = 0;
}

bool OpenGLESRenderer::IsExtensionPresent(const std::set<std::string> *oglExtensionSet, const std::string extensionName) const
{
	if (oglExtensionSet == NULL || oglExtensionSet->size() == 0)
	{
		return false;
	}
	
	return (oglExtensionSet->find(extensionName) != oglExtensionSet->end());
}

bool OpenGLESRenderer::ValidateShaderCompile(GLuint theShader) const
{
	bool isCompileValid = false;
	GLint status = GL_FALSE;
	
	glGetShaderiv(theShader, GL_COMPILE_STATUS, &status);
	if(status == GL_TRUE)
	{
		isCompileValid = true;
	}
	else
	{
		GLint logSize;
		GLchar *log = NULL;
		
		glGetShaderiv(theShader, GL_INFO_LOG_LENGTH, &logSize);
		log = new GLchar[logSize];
		glGetShaderInfoLog(theShader, logSize, &logSize, log);
		
		INFO("OpenGLES2: SEVERE - FAILED TO COMPILE SHADER : %s\n", log);
		delete[] log;
	}
	
	return isCompileValid;
}

bool OpenGLESRenderer::ValidateShaderProgramLink(GLuint theProgram) const
{
	bool isLinkValid = false;
	GLint status = GL_FALSE;
	
	glGetProgramiv(theProgram, GL_LINK_STATUS, &status);
	if(status == GL_TRUE)
	{
		isLinkValid = true;
	}
	else
	{
		GLint logSize;
		GLchar *log = NULL;
		
		glGetProgramiv(theProgram, GL_INFO_LOG_LENGTH, &logSize);
		log = new GLchar[logSize];
		glGetProgramInfoLog(theProgram, logSize, &logSize, log);
		
		INFO("OpenGLES2: SEVERE - FAILED TO LINK SHADER PROGRAM : %s\n", log);
		delete[] log;
	}
	
	return isLinkValid;
}

void OpenGLESRenderer::GetVersion(unsigned int *major, unsigned int *minor) const
{
	*major = this->versionMajor;
	*minor = this->versionMinor;
}

void OpenGLESRenderer::SetVersion(unsigned int major, unsigned int minor)
{
	this->versionMajor = major;
	this->versionMinor = minor;
}

void OpenGLESRenderer::ConvertFramebuffer(const u32 *__restrict srcBuffer, u32 *dstBuffer)
{
	if (srcBuffer == NULL || dstBuffer == NULL)
	{
		return;
	}
	
	// Convert from 32-bit BGRA8888/RGBA8888 format to 32-bit RGBA6665 reversed format. OpenGLES2
	// stores pixels using a flipped Y-coordinate, so this needs to be flipped back
	// to the DS Y-coordinate.
	for(int i = 0, y = 191; y >= 0; y--)
	{
		u32 *__restrict dst = dstBuffer + (y << 8); // Same as dstBuffer + (y * 256)
		
		for(unsigned int x = 0; x < 256; x++, i++)
		{
			// Use the correct endian format since OpenGLES2 uses the native endian of
			// the architecture it is running on.
#ifdef WORDS_BIGENDIAN
			//*dst++ = BGRA8888_32_To_RGBA6665_32(srcBuffer[i]);
			*dst++ = RGBA8888_32_To_RGBA6665_32Rev(srcBuffer[i]);
#else
			//*dst++ = BGRA8888_32Rev_To_RGBA6665_32Rev(srcBuffer[i]);
			*dst++ = RGBA8888_32Rev_To_RGBA6665_32Rev(srcBuffer[i]);
#endif
		}
	}
}

OpenGLES2Renderer::OpenGLES2Renderer()
{
	isFBOSupported = false;
	isVAOSupported = false;
	
	// Init OpenGLES2 rendering states
	ref = new OGLESRenderRef;
}

OpenGLES2Renderer::~OpenGLES2Renderer()
{
	if (ref == NULL)
	{
		return;
	}
	
	glFinish();
	
	gpuScreen3DHasNewData[0] = false;
	gpuScreen3DHasNewData[1] = false;
	
	DestroyShaders();
	DestroyVAOs();
	DestroyVBOs();
	DestroyFBOs();
	
	//kill the tex cache to free all the texture ids
	TexCache_Reset();
	
	glBindTexture(GL_TEXTURE_2D, 0);
	
	while(!ref->freeTextureIDs.empty())
	{
		GLuint temp = ref->freeTextureIDs.front();
		ref->freeTextureIDs.pop();
		glDeleteTextures(1, &temp);
	}
	
	glFinish();
	
	// Destroy OpenGLES2 rendering states
	delete ref;
	ref = NULL;
}

Render3DError OpenGLES2Renderer::InitExtensions()
{
	Render3DError error = OGLERROR_NOERR;
	OGLESRenderRef &OGLRef = *this->ref;
	
	// Get OpenGLES2 extensions
	std::set<std::string> oglExtensionSet;
	this->GetExtensionSet(&oglExtensionSet);
	
	// Initialize OpenGLES2
	this->InitTables();
	
	std::string vertexShaderProgram;
	std::string fragmentShaderProgram;
	error = this->LoadShaderPrograms(&vertexShaderProgram, &fragmentShaderProgram);
	if (error != OGLERROR_NOERR)
	{
		return error;
	}
	
	error = this->CreateShaders(&vertexShaderProgram, &fragmentShaderProgram);
	if (error != OGLERROR_NOERR)
	{
		return error;
	}
	
	this->CreateToonTable();
	
	this->CreateVBOs();
	
	this->isVAOSupported = this->IsExtensionPresent(&oglExtensionSet, "GL_OES_vertex_array_object");
	if (this->isVAOSupported)
	{
		this->CreateVAOs();
	}
	
	//this->isFBOSupported	= this->IsExtensionPresent(&oglExtensionSet, "GL_OES_depth_texture") &&
	//						  this->IsExtensionPresent(&oglExtensionSet, "GL_OES_packed_depth_stencil");
	this->isFBOSupported = false;
	if (this->isFBOSupported)
	{
		error = this->CreateFBOs();
		if (error != OGLERROR_NOERR)
		{
			OGLRef.fboFinalOutputID = 0;
			this->isFBOSupported = false;
		}
	}
	else
	{
		OGLRef.fboFinalOutputID = 0;
		INFO("OpenGLES2: FBOs are unsupported. Some emulation features will be disabled.\n");
	}
	
	this->InitTextures();
	this->InitFinalRenderStates(&oglExtensionSet); // This must be done last
	
	return OGLERROR_NOERR;
}

Render3DError OpenGLES2Renderer::CreateVBOs()
{
	OGLESRenderRef &OGLRef = *this->ref;
	
	glGenBuffers(1, &OGLRef.vboVertexID);
	glBindBuffer(GL_ARRAY_BUFFER, OGLRef.vboVertexID);
	glBufferData(GL_ARRAY_BUFFER, VERTLIST_SIZE * sizeof(VERT), NULL, GL_STREAM_DRAW);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	
	glGenBuffers(1, &OGLRef.iboIndexID);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, OGLRef.iboIndexID);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, OGLRENDER_VERT_INDEX_BUFFER_COUNT * sizeof(GLushort), NULL, GL_STREAM_DRAW);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
	
	return OGLERROR_NOERR;
}

void OpenGLES2Renderer::DestroyVBOs()
{
	OGLESRenderRef &OGLRef = *this->ref;
	
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glDeleteBuffers(1, &OGLRef.vboVertexID);
	
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
	glDeleteBuffers(1, &OGLRef.iboIndexID);
}

Render3DError OpenGLES2Renderer::LoadShaderPrograms(std::string *outVertexShaderProgram, std::string *outFragmentShaderProgram)
{
	outVertexShaderProgram->clear();
	outFragmentShaderProgram->clear();
	
	*outVertexShaderProgram += std::string(vertexShader_100);
	*outFragmentShaderProgram += std::string(fragmentShader_100);
	
	return OGLERROR_NOERR;
}

Render3DError OpenGLES2Renderer::SetupShaderIO()
{
	OGLESRenderRef &OGLRef = *this->ref;
	
	glBindAttribLocation(OGLRef.shaderProgram, OGLVertexAttributeID_Position, "inPosition");
	glBindAttribLocation(OGLRef.shaderProgram, OGLVertexAttributeID_TexCoord0, "inTexCoord0");
	glBindAttribLocation(OGLRef.shaderProgram, OGLVertexAttributeID_Color, "inColor");
	
	return OGLERROR_NOERR;
}

Render3DError OpenGLES2Renderer::CreateShaders(const std::string *vertexShaderProgram, const std::string *fragmentShaderProgram)
{
	OGLESRenderRef &OGLRef = *this->ref;
	
	OGLRef.vertexShaderID = glCreateShader(GL_VERTEX_SHADER);
	if(!OGLRef.vertexShaderID)
	{
		INFO("OpenGLES2: Failed to create the vertex shader.\n");		
		return OGLERROR_SHADER_CREATE_ERROR;
	}
	
	const char *vertexShaderProgramChar = vertexShaderProgram->c_str();
	glShaderSource(OGLRef.vertexShaderID, 1, (const GLchar **)&vertexShaderProgramChar, NULL);
	glCompileShader(OGLRef.vertexShaderID);
	if (!this->ValidateShaderCompile(OGLRef.vertexShaderID))
	{
		glDeleteShader(OGLRef.vertexShaderID);
		INFO("OpenGLES2: Failed to compile the vertex shader.\n");
		return OGLERROR_SHADER_CREATE_ERROR;
	}
	
	OGLRef.fragmentShaderID = glCreateShader(GL_FRAGMENT_SHADER);
	if(!OGLRef.fragmentShaderID)
	{
		glDeleteShader(OGLRef.vertexShaderID);
		INFO("OpenGLES2: Failed to create the fragment shader.\n");
		return OGLERROR_SHADER_CREATE_ERROR;
	}
	
	const char *fragmentShaderProgramChar = fragmentShaderProgram->c_str();
	glShaderSource(OGLRef.fragmentShaderID, 1, (const GLchar **)&fragmentShaderProgramChar, NULL);
	glCompileShader(OGLRef.fragmentShaderID);
	if (!this->ValidateShaderCompile(OGLRef.fragmentShaderID))
	{
		glDeleteShader(OGLRef.vertexShaderID);
		glDeleteShader(OGLRef.fragmentShaderID);
		INFO("OpenGLES2: Failed to compile the fragment shader.\n");
		return OGLERROR_SHADER_CREATE_ERROR;
	}
	
	OGLRef.shaderProgram = glCreateProgram();
	if(!OGLRef.shaderProgram)
	{
		glDeleteShader(OGLRef.vertexShaderID);
		glDeleteShader(OGLRef.fragmentShaderID);
		INFO("OpenGLES2: Failed to create the shader program.\n");
		return OGLERROR_SHADER_CREATE_ERROR;
	}
	
	glAttachShader(OGLRef.shaderProgram, OGLRef.vertexShaderID);
	glAttachShader(OGLRef.shaderProgram, OGLRef.fragmentShaderID);
	
	this->SetupShaderIO();
	
	glLinkProgram(OGLRef.shaderProgram);
	if (!this->ValidateShaderProgramLink(OGLRef.shaderProgram))
	{
		glDetachShader(OGLRef.shaderProgram, OGLRef.vertexShaderID);
		glDetachShader(OGLRef.shaderProgram, OGLRef.fragmentShaderID);
		glDeleteProgram(OGLRef.shaderProgram);
		glDeleteShader(OGLRef.vertexShaderID);
		glDeleteShader(OGLRef.fragmentShaderID);
		INFO("OpenGLES2: Failed to link the shader program.\n");
		return OGLERROR_SHADER_CREATE_ERROR;
	}
	
	glValidateProgram(OGLRef.shaderProgram);
	glUseProgram(OGLRef.shaderProgram);
	
	// Set up shader uniforms
	GLint uniformTexSampler = glGetUniformLocation(OGLRef.shaderProgram, "texMainRender");
	glUniform1i(uniformTexSampler, 0);
	
	uniformTexSampler = glGetUniformLocation(OGLRef.shaderProgram, "texToonTable");
	glUniform1i(uniformTexSampler, OGLTextureUnitID_ToonTable);
	
	OGLRef.uniformPolyAlpha			= glGetUniformLocation(OGLRef.shaderProgram, "polyAlpha");
	OGLRef.uniformTexScale			= glGetUniformLocation(OGLRef.shaderProgram, "texScale");
	OGLRef.uniformPolyID			= glGetUniformLocation(OGLRef.shaderProgram, "polyID");
	OGLRef.uniformHasTexture		= glGetUniformLocation(OGLRef.shaderProgram, "hasTexture");
	OGLRef.uniformPolygonMode		= glGetUniformLocation(OGLRef.shaderProgram, "polygonMode");
	OGLRef.uniformToonShadingMode	= glGetUniformLocation(OGLRef.shaderProgram, "toonShadingMode");
	OGLRef.uniformWBuffer			= glGetUniformLocation(OGLRef.shaderProgram, "oglWBuffer");
	OGLRef.uniformEnableAlphaTest	= glGetUniformLocation(OGLRef.shaderProgram, "enableAlphaTest");
	OGLRef.uniformAlphaTestRef		= glGetUniformLocation(OGLRef.shaderProgram, "alphaTestRef");
	
	INFO("OpenGLES2: Successfully created shaders.\n");
	
	return OGLERROR_NOERR;
}

void OpenGLES2Renderer::DestroyShaders()
{
	OGLESRenderRef &OGLRef = *this->ref;
	
	glUseProgram(0);
	
	glDetachShader(OGLRef.shaderProgram, OGLRef.vertexShaderID);
	glDetachShader(OGLRef.shaderProgram, OGLRef.fragmentShaderID);
	
	glDeleteProgram(OGLRef.shaderProgram);
	glDeleteShader(OGLRef.vertexShaderID);
	glDeleteShader(OGLRef.fragmentShaderID);
	
	this->DestroyToonTable();
}

Render3DError OpenGLES2Renderer::CreateVAOs()
{
	OGLESRenderRef &OGLRef = *this->ref;
	
	glGenVertexArraysOES(1, &OGLRef.vaoMainStatesID);
	glBindVertexArrayOES(OGLRef.vaoMainStatesID);
	
	glBindBuffer(GL_ARRAY_BUFFER, OGLRef.vboVertexID);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, OGLRef.iboIndexID);
	
	glEnableVertexAttribArray(OGLVertexAttributeID_Position);
	glEnableVertexAttribArray(OGLVertexAttributeID_TexCoord0);
	glEnableVertexAttribArray(OGLVertexAttributeID_Color);
	
	glVertexAttribPointer(OGLVertexAttributeID_Position, 4, GL_FLOAT, GL_FALSE, sizeof(VERT), (const GLvoid *)offsetof(VERT, coord));
	glVertexAttribPointer(OGLVertexAttributeID_TexCoord0, 2, GL_FLOAT, GL_FALSE, sizeof(VERT), (const GLvoid *)offsetof(VERT, texcoord));
	glVertexAttribPointer(OGLVertexAttributeID_Color, 3, GL_UNSIGNED_BYTE, GL_TRUE, sizeof(VERT), (const GLvoid *)offsetof(VERT, color));
	
	glBindVertexArrayOES(0);
	
	return OGLERROR_NOERR;
}

void OpenGLES2Renderer::DestroyVAOs()
{
	if (!this->isVAOSupported)
	{
		return;
	}
	
	glBindVertexArrayOES(0);
	glDeleteVertexArraysOES(1, &this->ref->vaoMainStatesID);
	
	this->isVAOSupported = false;
}

Render3DError OpenGLES2Renderer::CreateFBOs()
{
	OGLESRenderRef &OGLRef = *this->ref;
	
	// Set up FBO render targets
	this->CreateClearImage();
	
	// Set up FBOs
	glGenFramebuffers(1, &OGLRef.fboClearImageID);
	
	glBindFramebuffer(GL_FRAMEBUFFER, OGLRef.fboClearImageID);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, OGLRef.texClearImageColorID, 0);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, OGLRef.texClearImageDepthStencilID, 0);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_STENCIL_ATTACHMENT, GL_TEXTURE_2D, OGLRef.texClearImageDepthStencilID, 0);
	
	if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
	{
		INFO("OpenGLES2: Failed to created FBOs. Some emulation features will be disabled.\n");
		
		glBindFramebuffer(GL_FRAMEBUFFER, 0);
		glDeleteFramebuffers(1, &OGLRef.fboClearImageID);
		this->DestroyClearImage();
		
		this->isFBOSupported = false;
		return OGLERROR_FBO_CREATE_ERROR;
	}
	
	// Set up final output FBO
	OGLRef.fboFinalOutputID = 0;
	glBindFramebuffer(GL_FRAMEBUFFER, OGLRef.fboFinalOutputID);
	
	INFO("OpenGLES2: Successfully created FBOs.\n");
	
	return OGLERROR_NOERR;
}

void OpenGLES2Renderer::DestroyFBOs()
{
	if (!this->isFBOSupported)
	{
		return;
	}
	
	OGLESRenderRef &OGLRef = *this->ref;
	
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	glDeleteFramebuffers(1, &OGLRef.fboClearImageID);
	this->DestroyClearImage();
	this->isFBOSupported = false;
}

Render3DError OpenGLES2Renderer::InitFinalRenderStates(const std::set<std::string> *oglExtensionSet)
{
	OGLESRenderRef &OGLRef = *this->ref;
	
	bool isBlendEquationSeparateSupported = this->IsExtensionPresent(oglExtensionSet, "GL_EXT_blend_minmax");
	
	// Blending Support
	if (isBlendEquationSeparateSupported)
	{
		// we want to use alpha destination blending so we can track the last-rendered alpha value
		// test: new super mario brothers renders the stormclouds at the beginning
		glBlendFuncSeparate(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA, GL_SRC_ALPHA, GL_DST_ALPHA);
		glBlendEquationSeparate(GL_FUNC_ADD, GL_MAX_EXT);
	}
	else
	{
		glBlendFuncSeparate(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA, GL_ONE, GL_DST_ALPHA);
	}
	
	// Mirrored Repeat Mode Support
	OGLRef.stateTexMirroredRepeat = GL_MIRRORED_REPEAT;
	
	// Always enable depth test, and control using glDepthMask().
	glEnable(GL_DEPTH_TEST);
	
	return OGLERROR_NOERR;
}

Render3DError OpenGLES2Renderer::InitTextures()
{
	this->ExpandFreeTextures();
	
	return OGLERROR_NOERR;
}

Render3DError OpenGLES2Renderer::InitTables()
{
	static bool needTableInit = true;
	
	if (needTableInit)
	{
		for (unsigned int i = 0; i < 32768; i++)
			dsDepthToD24S8_LUT[i] = (GLuint)DS_DEPTH15TO24(i) << 8;
		
		needTableInit = false;
	}
	
	return OGLERROR_NOERR;
}

Render3DError OpenGLES2Renderer::CreateToonTable()
{
	OGLESRenderRef &OGLRef = *this->ref;
	
	// The toon table is a special 1D texture where each pixel corresponds
	// to a specific color in the toon table.
	glGenTextures(1, &OGLRef.texToonTableID);
	glActiveTexture(GL_TEXTURE0 + OGLTextureUnitID_ToonTable);
	
	glBindTexture(GL_TEXTURE_2D, OGLRef.texToonTableID);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glBindTexture(GL_TEXTURE_2D, 0);
	
	glActiveTexture(GL_TEXTURE0);
	
	return OGLERROR_NOERR;
}

Render3DError OpenGLES2Renderer::DestroyToonTable()
{
	glActiveTexture(GL_TEXTURE0 + OGLTextureUnitID_ToonTable);
	glBindTexture(GL_TEXTURE_2D, 0);
	glActiveTexture(GL_TEXTURE0);
	glDeleteTextures(1, &this->ref->texToonTableID);
	
	return OGLERROR_NOERR;
}

Render3DError OpenGLES2Renderer::UploadToonTable(const GLuint *toonTableBuffer)
{
	glActiveTexture(GL_TEXTURE0 + OGLTextureUnitID_ToonTable);
	glBindTexture(GL_TEXTURE_2D, this->ref->texToonTableID);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 32, 1, 0, GL_RGBA, GL_UNSIGNED_BYTE, toonTableBuffer);
	glActiveTexture(GL_TEXTURE0);
	
	return OGLERROR_NOERR;
}

Render3DError OpenGLES2Renderer::CreateClearImage()
{
	OGLESRenderRef &OGLRef = *this->ref;
	
	glGenTextures(1, &OGLRef.texClearImageColorID);
	glGenTextures(1, &OGLRef.texClearImageDepthStencilID);
	
	glActiveTexture(GL_TEXTURE0 + OGLTextureUnitID_ClearImage);
	
	glBindTexture(GL_TEXTURE_2D, OGLRef.texClearImageColorID);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 256, 192, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
	
	glBindTexture(GL_TEXTURE_2D, OGLRef.texClearImageDepthStencilID);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	//glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_MODE, GL_NONE);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH24_STENCIL8_OES, 256, 192, 0, GL_DEPTH_STENCIL_OES, GL_UNSIGNED_INT_24_8_OES, NULL);
	
	glActiveTexture(GL_TEXTURE0);
	
	return OGLERROR_NOERR;
}

Render3DError OpenGLES2Renderer::DestroyClearImage()
{
	OGLESRenderRef &OGLRef = *this->ref;
	
	glActiveTexture(GL_TEXTURE0 + OGLTextureUnitID_ClearImage);
	glBindTexture(GL_TEXTURE_2D, 0);
	glActiveTexture(GL_TEXTURE0);
	glDeleteTextures(1, &OGLRef.texClearImageColorID);
	glDeleteTextures(1, &OGLRef.texClearImageDepthStencilID);
	
	return OGLERROR_NOERR;
}

Render3DError OpenGLES2Renderer::UploadClearImage(const GLushort *clearImageColorBuffer, const GLint *clearImageDepthBuffer)
{
	OGLESRenderRef &OGLRef = *this->ref;
	
	glActiveTexture(GL_TEXTURE0 + OGLTextureUnitID_ClearImage);
	
	glBindTexture(GL_TEXTURE_2D, OGLRef.texClearImageColorID);
	glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, 256, 192, GL_RGBA, GL_UNSIGNED_BYTE, clearImageColorBuffer);
	glBindTexture(GL_TEXTURE_2D, OGLRef.texClearImageDepthStencilID);
	glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, 256, 192, GL_DEPTH_STENCIL_OES, GL_UNSIGNED_INT_24_8_OES, clearImageDepthBuffer);
	glBindTexture(GL_TEXTURE_2D, 0);
	
	glActiveTexture(GL_TEXTURE0);
	
	return OGLERROR_NOERR;
}

void OpenGLES2Renderer::GetExtensionSet(std::set<std::string> *oglExtensionSet)
{
	std::string oglExtensionString = std::string((const char *)glGetString(GL_EXTENSIONS));
	
	size_t extStringStartLoc = 0;
	size_t delimiterLoc = oglExtensionString.find_first_of(' ', extStringStartLoc);
	while (delimiterLoc != std::string::npos)
	{
		std::string extensionName = oglExtensionString.substr(extStringStartLoc, delimiterLoc - extStringStartLoc);
		oglExtensionSet->insert(extensionName);
		
		extStringStartLoc = delimiterLoc + 1;
		delimiterLoc = oglExtensionString.find_first_of(' ', extStringStartLoc);
	}
	
	if (extStringStartLoc - oglExtensionString.length() > 0)
	{
		std::string extensionName = oglExtensionString.substr(extStringStartLoc, oglExtensionString.length() - extStringStartLoc);
		oglExtensionSet->insert(extensionName);
	}

	INFO("{ ExtensionSet : %s }\n", oglExtensionString.c_str());
}

Render3DError OpenGLES2Renderer::ExpandFreeTextures()
{
	static const int kInitTextures = 128;
	GLuint oglTempTextureID[kInitTextures];
	glGenTextures(kInitTextures, oglTempTextureID);
	
	for(int i=0;i<kInitTextures;i++)
	{
		this->ref->freeTextureIDs.push(oglTempTextureID[i]);
	}
	
	return OGLERROR_NOERR;
}

Render3DError OpenGLES2Renderer::SetupVertices(const VERTLIST *vertList, const POLYLIST *polyList, const INDEXLIST *indexList, GLushort *outIndexBuffer, unsigned int *outIndexCount)
{
	OGLESRenderRef &OGLRef = *this->ref;
	const unsigned int polyCount = polyList->count;
	unsigned int vertIndexCount = 0;
	
	for(unsigned int i = 0; i < polyCount; i++)
	{
		const POLY *poly = &polyList->list[indexList->list[i]];
		const unsigned int polyType = poly->type;
		
		for(unsigned int j = 0; j < polyType; j++)
		{
			const GLushort vertIndex = poly->vertIndexes[j];
				
			// While we're looping through our vertices, add each vertex index to
			// a buffer. For GFX3D_QUADS and GFX3D_QUAD_STRIP, we also add additional
			// vertices here to convert them to GL_TRIANGLES, which are much easier
			// to work with and won't be deprecated in future OpenGLES2 versions.
			outIndexBuffer[vertIndexCount++] = vertIndex;
			if (poly->vtxFormat == GFX3D_QUADS || poly->vtxFormat == GFX3D_QUAD_STRIP)
			{
				if (j == 2)
				{
					outIndexBuffer[vertIndexCount++] = vertIndex;
				}
				else if (j == 3)
				{
					outIndexBuffer[vertIndexCount++] = poly->vertIndexes[0];
				}
			}
		}
	}
	
	*outIndexCount = vertIndexCount;
	
	return OGLERROR_NOERR;
}

Render3DError OpenGLES2Renderer::EnableVertexAttributes(const VERTLIST *vertList, const GLushort *indexBuffer, const unsigned int vertIndexCount)
{
	OGLESRenderRef &OGLRef = *this->ref;
	
	if (this->isVAOSupported)
	{
		glBindVertexArrayOES(OGLRef.vaoMainStatesID);
		glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(VERT) * vertList->count, vertList);
		glBufferSubData(GL_ELEMENT_ARRAY_BUFFER, 0, vertIndexCount * sizeof(GLushort), indexBuffer);
	}
	else
	{
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, OGLRef.iboIndexID);
		glBufferSubData(GL_ELEMENT_ARRAY_BUFFER, 0, vertIndexCount * sizeof(GLushort), indexBuffer);
		
		glBindBuffer(GL_ARRAY_BUFFER, OGLRef.vboVertexID);
		glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(VERT) * vertList->count, vertList);
		
		glEnableVertexAttribArray(OGLVertexAttributeID_Position);
		glEnableVertexAttribArray(OGLVertexAttributeID_TexCoord0);
		glEnableVertexAttribArray(OGLVertexAttributeID_Color);
		
		glVertexAttribPointer(OGLVertexAttributeID_Position, 4, GL_FLOAT, GL_FALSE, sizeof(VERT), (const GLvoid *)offsetof(VERT, coord));
		glVertexAttribPointer(OGLVertexAttributeID_TexCoord0, 2, GL_FLOAT, GL_FALSE, sizeof(VERT), (const GLvoid *)offsetof(VERT, texcoord));
		glVertexAttribPointer(OGLVertexAttributeID_Color, 3, GL_UNSIGNED_BYTE, GL_TRUE, sizeof(VERT), (const GLvoid *)offsetof(VERT, color));
	}
	
	return OGLERROR_NOERR;
}

Render3DError OpenGLES2Renderer::DisableVertexAttributes()
{
	if (this->isVAOSupported)
	{
		glBindVertexArrayOES(0);
	}
	else
	{
		glDisableVertexAttribArray(OGLVertexAttributeID_Position);
		glDisableVertexAttribArray(OGLVertexAttributeID_TexCoord0);
		glDisableVertexAttribArray(OGLVertexAttributeID_Color);
		
		glBindBuffer(GL_ARRAY_BUFFER, 0);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
	}
	
	return OGLERROR_NOERR;
}

Render3DError OpenGLES2Renderer::SelectRenderingFramebuffer()
{
	OGLESRenderRef &OGLRef = *this->ref;

	OGLRef.selectedRenderingFBO = OGLRef.fboFinalOutputID;
	glBindFramebuffer(GL_FRAMEBUFFER, OGLRef.selectedRenderingFBO);
	
	return OGLERROR_NOERR;
}

Render3DError OpenGLES2Renderer::ReadBackPixels()
{
	const unsigned int i = this->doubleBufferIndex;
	
	this->gpuScreen3DHasNewData[i] = true;
	
	return OGLERROR_NOERR;
}

Render3DError OpenGLES2Renderer::DeleteTexture(const TexCacheItem *item)
{
	this->ref->freeTextureIDs.push((GLuint)item->texid);
	if(this->currTexture == item)
	{
		this->currTexture = NULL;
	}
	
	return OGLERROR_NOERR;
}

Render3DError OpenGLES2Renderer::BeginRender(const GFX3D_State *renderState)
{
	OGLESRenderRef &OGLRef = *this->ref;
	this->doubleBufferIndex = (this->doubleBufferIndex + 1) & 0x01;
	
	this->SelectRenderingFramebuffer();
	
	glUniform1i(OGLRef.uniformEnableAlphaTest, renderState->enableAlphaTest ? GL_TRUE : GL_FALSE);
	glUniform1f(OGLRef.uniformAlphaTestRef, divide5bitBy31_LUT[renderState->alphaTestRef]);
	glUniform1i(OGLRef.uniformToonShadingMode, renderState->shading);
	glUniform1i(OGLRef.uniformWBuffer, renderState->wbuffer);
	
	if(renderState->enableAlphaBlending)
	{
		glEnable(GL_BLEND);
	}
	else
	{
		glDisable(GL_BLEND);
	}
	
	glDepthMask(GL_TRUE);
	
	return OGLERROR_NOERR;
}

Render3DError OpenGLES2Renderer::PreRender(const GFX3D_State *renderState, const VERTLIST *vertList, const POLYLIST *polyList, const INDEXLIST *indexList)
{
	OGLESRenderRef &OGLRef = *this->ref;
	unsigned int vertIndexCount = 0;
	
	this->SetupVertices(vertList, polyList, indexList, OGLRef.vertIndexBuffer, &vertIndexCount);
	this->EnableVertexAttributes(vertList, OGLRef.vertIndexBuffer, vertIndexCount);
	
	return OGLERROR_NOERR;
}

Render3DError OpenGLES2Renderer::DoRender(const GFX3D_State *renderState, const VERTLIST *vertList, const POLYLIST *polyList, const INDEXLIST *indexList)
{
	OGLESRenderRef &OGLRef = *this->ref;
	u32 lastTexParams = 0;
	u32 lastTexPalette = 0;
	u32 lastPolyAttr = 0;
	u32 lastViewport = 0xFFFFFFFF;
	const unsigned int polyCount = polyList->count;
	//GLushort *indexBufferPtr = this->isVBOSupported ? 0 : OGLRef.vertIndexBuffer;
	GLushort *indexBufferPtr = 0;
	
	// Map GFX3D_QUADS and GFX3D_QUAD_STRIP to GL_TRIANGLES since we will convert them.
	//
	// Also map GFX3D_TRIANGLE_STRIP to GL_TRIANGLES. This is okay since this is actually
	// how the POLY struct stores triangle strip vertices, which is in sets of 3 vertices
	// each. This redefinition is necessary since uploading more than 3 indices at a time
	// will cause glDrawElements() to draw the triangle strip incorrectly.
	static const GLenum oglPrimitiveType[]	= {GL_TRIANGLES, GL_TRIANGLES, GL_TRIANGLES, GL_TRIANGLES,
											   GL_LINE_LOOP, GL_LINE_LOOP, GL_LINE_STRIP, GL_LINE_STRIP};
	
	static const unsigned int indexIncrementLUT[] = {3, 6, 3, 6, 3, 4, 3, 4};
	
	for(unsigned int i = 0; i < polyCount; i++)
	{
		const POLY *poly = &polyList->list[indexList->list[i]];
		
		// Set up the polygon if it changed
		if(lastPolyAttr != poly->polyAttr || i == 0)
		{
			lastPolyAttr = poly->polyAttr;
			this->SetupPolygon(poly);
		}
		
		// Set up the texture if it changed
		if(lastTexParams != poly->texParam || lastTexPalette != poly->texPalette || i == 0)
		{
			lastTexParams = poly->texParam;
			lastTexPalette = poly->texPalette;
			this->SetupTexture(poly, renderState->enableTexturing);
		}
		
		// Set up the viewport if it changed
		if(lastViewport != poly->viewport || i == 0)
		{
			lastViewport = poly->viewport;
			this->SetupViewport(poly);
		}
		
		// In wireframe mode, redefine all primitives as GL_LINE_LOOP rather than
		// setting the polygon mode to GL_LINE though glPolygonMode(). Not only is
		// drawing more accurate this way, but it also allows GFX3D_QUADS and
		// GFX3D_QUAD_STRIP primitives to properly draw as wireframe without the
		// extra diagonal line.
		const GLenum polyPrimitive = !poly->isWireframe() ? oglPrimitiveType[poly->vtxFormat] : GL_LINE_LOOP;
		
		// Render the polygon
		const unsigned int vertIndexCount = indexIncrementLUT[poly->vtxFormat];
		glDrawElements(polyPrimitive, vertIndexCount, GL_UNSIGNED_SHORT, indexBufferPtr);
		indexBufferPtr += vertIndexCount;
	}
	
	return OGLERROR_NOERR;
}

Render3DError OpenGLES2Renderer::PostRender()
{
	this->DisableVertexAttributes();
	
	return OGLERROR_NOERR;
}

Render3DError OpenGLES2Renderer::EndRender(const u64 frameCount)
{
	//needs to happen before endgl because it could free some textureids for expired cache items
	TexCache_EvictFrame();
	
	this->ReadBackPixels();
	
	return OGLERROR_NOERR;
}

Render3DError OpenGLES2Renderer::UpdateClearImage(const u16 *__restrict colorBuffer, const u16 *__restrict depthBuffer, const u8 clearStencil, const u8 xScroll, const u8 yScroll)
{
	static const size_t pixelsPerLine = 256;
	static const size_t lineCount = 192;
	static const size_t totalPixelCount = pixelsPerLine * lineCount;
	static const size_t bufferSize = totalPixelCount * sizeof(u16);
	
	static CACHE_ALIGN GLushort clearImageColorBuffer[totalPixelCount] = {0};
	static CACHE_ALIGN GLint clearImageDepthBuffer[totalPixelCount] = {0};
	static CACHE_ALIGN u16 lastColorBuffer[totalPixelCount] = {0};
	static CACHE_ALIGN u16 lastDepthBuffer[totalPixelCount] = {0};
	static u8 lastXScroll = 0;
	static u8 lastYScroll = 0;
	
	if (!this->isFBOSupported)
	{
		return OGLERROR_FEATURE_UNSUPPORTED;
	}
	
	if (lastXScroll != xScroll ||
		lastYScroll != yScroll ||
		memcmp(colorBuffer, lastColorBuffer, bufferSize) ||
		memcmp(depthBuffer, lastDepthBuffer, bufferSize) )
	{
		lastXScroll = xScroll;
		lastYScroll = yScroll;
		memcpy(lastColorBuffer, colorBuffer, bufferSize);
		memcpy(lastDepthBuffer, depthBuffer, bufferSize);
		
		unsigned int dd = totalPixelCount - pixelsPerLine;
		
		for(unsigned int iy = 0; iy < lineCount; iy++)
		{
			const unsigned int y = ((iy + yScroll) & 0xFF) << 8;
			
			for(unsigned int ix = 0; ix < pixelsPerLine; ix++)
			{
				const unsigned int x = (ix + xScroll) & 0xFF;
				const unsigned int adr = y + x;
				
				clearImageColorBuffer[dd] = colorBuffer[adr];
				clearImageDepthBuffer[dd] = dsDepthToD24S8_LUT[depthBuffer[adr] & 0x7FFF] | clearStencil;
				
				dd++;
			}
			
			dd -= pixelsPerLine * 2;
		}
		
		this->UploadClearImage(clearImageColorBuffer, clearImageDepthBuffer);
	}
	
	this->clearImageStencilValue = clearStencil;
	
	return OGLERROR_NOERR;
}

Render3DError OpenGLES2Renderer::UpdateToonTable(const u16 *toonTableBuffer)
{
	static CACHE_ALIGN u16 currentToonTable16[32] = {0};

	// Update the toon table if it changed.
	if (memcmp(currentToonTable16, toonTableBuffer, sizeof(currentToonTable16)))
	{
		memcpy(currentToonTable16, toonTableBuffer, sizeof(currentToonTable16));

		for(int i=0;i<32;i++)
			this->currentToonTable32[i] = RGB15TO32_NOALPHA(toonTableBuffer[i]);

		this->toonTableNeedsUpdate = true;
	}
	
	return OGLERROR_NOERR;
}

Render3DError OpenGLES2Renderer::ClearUsingImage() const
{
	static u8 lastClearStencil = 0;
	
	if (!this->isFBOSupported)
	{
		return OGLERROR_FEATURE_UNSUPPORTED;
	}
	
	OGLESRenderRef &OGLRef = *this->ref;
	
	//glBindFramebufferEXT(GL_READ_FRAMEBUFFER_EXT, OGLRef.fboClearImageID);
	//glBindFramebufferEXT(GL_DRAW_FRAMEBUFFER_EXT, OGLRef.selectedRenderingFBO);
	//glBlitFramebufferEXT(0, 0, 256, 192, 0, 0, 256, 192, GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT, GL_NEAREST);
	//glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, OGLRef.selectedRenderingFBO);
	
	if (lastClearStencil == this->clearImageStencilValue)
	{
		lastClearStencil = this->clearImageStencilValue;
		glClearStencil(lastClearStencil);
	}
	
	glClear(GL_STENCIL_BUFFER_BIT);
	
	return OGLERROR_NOERR;
}

Render3DError OpenGLES2Renderer::ClearUsingValues(const u8 r, const u8 g, const u8 b, const u8 a, const u32 clearDepth, const u8 clearStencil) const
{
	static u8 last_r = 0;
	static u8 last_g = 0;
	static u8 last_b = 0;
	static u8 last_a = 0;
	static u32 lastClearDepth = 0;
	static u8 lastClearStencil = 0;
	
	if (r != last_r || g != last_g || b != last_b || a != last_a)
	{
		last_r = r;
		last_g = g;
		last_b = b;
		last_a = a;
		glClearColor(divide5bitBy31_LUT[r], divide5bitBy31_LUT[g], divide5bitBy31_LUT[b], divide5bitBy31_LUT[a]);
	}
	
	if (clearDepth != lastClearDepth)
	{
		lastClearDepth = clearDepth;
		glClearDepthf((GLfloat)clearDepth / (GLfloat)0x00FFFFFF);
	}
	
	if (clearStencil != lastClearStencil)
	{
		lastClearStencil = clearStencil;
		glClearStencil(clearStencil);
	}
	
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
	
	return OGLERROR_NOERR;
}

Render3DError OpenGLES2Renderer::SetupPolygon(const POLY *thePoly)
{
	static unsigned int lastTexBlendMode = 0;
	static int lastStencilState = -1;
	
	OGLESRenderRef &OGLRef = *this->ref;
	const PolygonAttributes attr = thePoly->getAttributes();
	
	// Set up polygon ID
	glUniform1i(OGLRef.uniformPolyID, attr.polygonID);
	
	// Set up alpha value
	const GLfloat thePolyAlpha = (!attr.isWireframe && attr.isTranslucent) ? divide5bitBy31_LUT[attr.alpha] : 1.0f;
	glUniform1f(OGLRef.uniformPolyAlpha, thePolyAlpha);
	
	// Set up depth test mode
	static const GLenum oglDepthFunc[2] = {GL_LESS, GL_EQUAL};
	glDepthFunc(oglDepthFunc[attr.enableDepthTest]);
	
	// Set up culling mode
	static const GLenum oglCullingMode[4] = {GL_FRONT_AND_BACK, GL_FRONT, GL_BACK, 0};
	GLenum cullingMode = oglCullingMode[attr.surfaceCullingMode];
	
	if (cullingMode == 0)
	{
		xglDisable(GL_CULL_FACE);
	}
	else
	{
		xglEnable(GL_CULL_FACE);
		glCullFace(cullingMode);
	}
	
	// Set up depth write
	GLboolean enableDepthWrite = GL_TRUE;
	
	// Handle shadow polys. Do this after checking for depth write, since shadow polys
	// can change this too.
	if(attr.polygonMode == 3)
	{
		xglEnable(GL_STENCIL_TEST);
		if(attr.polygonID == 0)
		{
			enableDepthWrite = GL_FALSE;
			if(lastStencilState != 0)
			{
				lastStencilState = 0;
				//when the polyID is zero, we are writing the shadow mask.
				//set stencilbuf = 1 where the shadow volume is obstructed by geometry.
				//do not write color or depth information.
				glStencilFunc(GL_ALWAYS, 65, 255);
				glStencilOp(GL_KEEP, GL_REPLACE, GL_KEEP);
				glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);
			}
		}
		else
		{
			enableDepthWrite = GL_TRUE;
			if(lastStencilState != 1)
			{
				lastStencilState = 1;
				//when the polyid is nonzero, we are drawing the shadow poly.
				//only draw the shadow poly where the stencilbuf==1.
				//I am not sure whether to update the depth buffer here--so I chose not to.
				glStencilFunc(GL_EQUAL, 65, 255);
				glStencilOp(GL_KEEP, GL_KEEP, GL_KEEP);
				glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
			}
		}
	}
	else
	{
		xglEnable(GL_STENCIL_TEST);
		if(attr.isTranslucent)
		{
			lastStencilState = 3;
			glStencilFunc(GL_NOTEQUAL, attr.polygonID, 255);
			glStencilOp(GL_KEEP, GL_KEEP, GL_REPLACE);
			glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
		}
		else if(lastStencilState != 2)
		{
			lastStencilState = 2;
			glStencilFunc(GL_ALWAYS, 64, 255);
			glStencilOp(GL_REPLACE, GL_REPLACE, GL_REPLACE);
			glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
		}
	}
	
	if(attr.isTranslucent && !attr.enableAlphaDepthWrite)
	{
		enableDepthWrite = GL_FALSE;
	}
	
	glDepthMask(enableDepthWrite);
	
	// Set up texture blending mode
	if(attr.polygonMode != lastTexBlendMode)
	{
		lastTexBlendMode = attr.polygonMode;
		
		glUniform1i(OGLRef.uniformPolygonMode, attr.polygonMode);
			
		// Update the toon table if necessary
		if (this->toonTableNeedsUpdate && attr.polygonMode == 2)
		{
			this->UploadToonTable(this->currentToonTable32);
			this->toonTableNeedsUpdate = false;
		}
	}
	
	return OGLERROR_NOERR;
}

Render3DError OpenGLES2Renderer::SetupTexture(const POLY *thePoly, bool enableTexturing)
{
	OGLESRenderRef &OGLRef = *this->ref;
	const PolygonTexParams params = thePoly->getTexParams();
	
	// Check if we need to use textures
	if (thePoly->texParam == 0 || params.texFormat == TEXMODE_NONE || !enableTexturing)
	{
		glUniform1i(OGLRef.uniformHasTexture, GL_FALSE);
		
		return OGLERROR_NOERR;
	}
	
	// Enable textures if they weren't already enabled
	glUniform1i(OGLRef.uniformHasTexture, GL_TRUE);
	
	//	texCacheUnit.TexCache_SetTexture<TexFormat_32bpp>(format, texpal);
	TexCacheItem *newTexture = TexCache_SetTexture(TexFormat_32bpp, thePoly->texParam, thePoly->texPalette);
	if(newTexture != this->currTexture)
	{
		this->currTexture = newTexture;
		//has the ogl renderer initialized the texture?
		if(!this->currTexture->deleteCallback)
		{
			this->currTexture->deleteCallback = texDeleteCallback;
			
			if(OGLRef.freeTextureIDs.empty())
			{
				this->ExpandFreeTextures();
			}
			
			this->currTexture->texid = (u64)OGLRef.freeTextureIDs.front();
			OGLRef.freeTextureIDs.pop();
			
			glBindTexture(GL_TEXTURE_2D, (GLuint)this->currTexture->texid);
			
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, (params.enableRepeatS ? (params.enableMirroredRepeatS ? OGLRef.stateTexMirroredRepeat : GL_REPEAT) : GL_CLAMP_TO_EDGE));
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, (params.enableRepeatT ? (params.enableMirroredRepeatT ? OGLRef.stateTexMirroredRepeat : GL_REPEAT) : GL_CLAMP_TO_EDGE));
			
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA,
						 this->currTexture->sizeX, this->currTexture->sizeY, 0,
						 GL_RGBA, GL_UNSIGNED_BYTE, this->currTexture->decoded);
		}
		else
		{
			//otherwise, just bind it
			glBindTexture(GL_TEXTURE_2D, (GLuint)this->currTexture->texid);
		}
		
		glUniform2f(OGLRef.uniformTexScale, this->currTexture->invSizeX, this->currTexture->invSizeY);
	}
	
	return OGLERROR_NOERR;
}

Render3DError OpenGLES2Renderer::SetupViewport(const POLY *thePoly)
{
	VIEWPORT viewport;
	viewport.decode(thePoly->viewport);
	glViewport(viewport.x, viewport.y, viewport.width, viewport.height);
	
	return OGLERROR_NOERR;
}

Render3DError OpenGLES2Renderer::Reset()
{
	OGLESRenderRef &OGLRef = *this->ref;
	
	this->gpuScreen3DHasNewData[0] = false;
	this->gpuScreen3DHasNewData[1] = false;
	
	glFinish();
	
	for (unsigned int i = 0; i < 2; i++)
	{
		memset(this->GPU_screen3D[i], 0, sizeof(this->GPU_screen3D[i]));
	}
	
	memset(currentToonTable32, 0, sizeof(currentToonTable32));
	this->UpdateToonTable((u16*)currentToonTable32);
	this->toonTableNeedsUpdate = true;
	
	glUniform1f(OGLRef.uniformPolyAlpha, 1.0f);
	glUniform2f(OGLRef.uniformTexScale, 1.0f, 1.0f);
	glUniform1i(OGLRef.uniformPolyID, 0);
	glUniform1i(OGLRef.uniformHasTexture, GL_FALSE);
	glUniform1i(OGLRef.uniformPolygonMode, 0);
	glUniform1i(OGLRef.uniformToonShadingMode, 0);
	glUniform1i(OGLRef.uniformWBuffer, 0);
	glUniform1i(OGLRef.uniformEnableAlphaTest, GL_TRUE);
	glUniform1f(OGLRef.uniformAlphaTestRef, 0.0f);
	
	memset(OGLRef.vertIndexBuffer, 0, OGLRENDER_VERT_INDEX_BUFFER_COUNT * sizeof(GLushort));
	this->currTexture = NULL;
	this->doubleBufferIndex = 0;
	this->clearImageStencilValue = 0;
	
	return OGLERROR_NOERR;
}

Render3DError OpenGLES2Renderer::RenderFinish()
{
	const unsigned int i = this->doubleBufferIndex;
	
	if (!this->gpuScreen3DHasNewData[i])
	{
		return OGLERROR_NOERR;
	}
	
	OGLESRenderRef &OGLRef = *this->ref;
	
	u32 *__restrict workingBuffer = this->GPU_screen3D[i];
	glReadPixels(0, 0, 256, 192, GL_RGBA, GL_UNSIGNED_BYTE, workingBuffer);
	this->ConvertFramebuffer(workingBuffer, (u32 *)gfx3d_convertedScreen);
	
	this->gpuScreen3DHasNewData[i] = false;
	
	return OGLERROR_NOERR;
}
