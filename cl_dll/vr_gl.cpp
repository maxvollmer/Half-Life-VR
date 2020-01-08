
#include "vr_gl.h"

#include "hud.h"
#include "cl_util.h"
#include "VRInput.h"

#ifndef GL_INVALID_FRAMEBUFFER_OPERATION
#define GL_INVALID_FRAMEBUFFER_OPERATION 0x0506
#endif

PFNGLGENFRAMEBUFFERSPROC glGenFramebuffers = nullptr;
PFNGLGENRENDERBUFFERSPROC glGenRenderbuffers = nullptr;

PFNGLBINDFRAMEBUFFERPROC glBindFramebuffer = nullptr;
PFNGLBINDRENDERBUFFERPROC glBindRenderbuffer = nullptr;

PFNGLFRAMEBUFFERTEXTURE2DPROC glFramebufferTexture2D = nullptr;
PFNGLRENDERBUFFERSTORAGEPROC glRenderbufferStorage = nullptr;
PFNGLFRAMEBUFFERRENDERBUFFERPROC glFramebufferRenderbuffer = nullptr;

PFNGLACTIVETEXTUREPROC glActiveTexture = nullptr;
PFNGLGENERATEMIPMAPPROC glGenerateMipmap = nullptr;

bool IsValidAddressPointer(void* p)
{
	if (p == nullptr)
		return false;

	int64_t i = reinterpret_cast<int>(p);
	if (i == -1 || (i >= 0 && i <= 3))
		return false;

	return true;
}

void* GetOpenGLFuncAddress(const char* name)
{
#ifdef WIN32
	void* p = wglGetProcAddress(name);
	if (!IsValidAddressPointer(p))
	{
		HMODULE module = LoadLibraryA("opengl32.dll");
		if (module)
		{
			p = GetProcAddress(module, name);
		}
	}
	return p;
#else
	return glXGetProcAddress(name);
#endif
}

inline std::string GLErrorString(GLenum error)
{
	switch (error)
	{
	case GL_NO_ERROR: return "GL_NO_ERROR";
	case GL_INVALID_ENUM: return "GL_INVALID_ENUM";
	case GL_INVALID_VALUE: return "GL_INVALID_VALUE";
	case GL_INVALID_OPERATION: return "GL_INVALID_OPERATION";
	case GL_INVALID_FRAMEBUFFER_OPERATION: return "GL_INVALID_FRAMEBUFFER_OPERATION";
	case GL_OUT_OF_MEMORY: return "GL_OUT_OF_MEMORY";
	case GL_STACK_UNDERFLOW: return "GL_STACK_UNDERFLOW";
	case GL_STACK_OVERFLOW: return "GL_STACK_OVERFLOW";
	default: return std::to_string(error);
	}
}

void ClearGLErrors()
{
	while (glGetError() != GL_NO_ERROR)
		;
}

void TryTryGLCall(std::function<void()> call, const std::string& name, const std::string& args)
{
	call();
	auto error = glGetError();
	if (error != GL_NO_ERROR)
	{
		std::stringstream errormsg;
		errormsg << "OpenGL call ";
		errormsg << name;
		errormsg << "(";
		errormsg << args;
		errormsg << ")";
		errormsg << " failed with error: ";
		errormsg << GLErrorString(error);
		throw OGLErrorException{ errormsg.str() };
	}
}

bool CreateGLTexture(unsigned int* texture, int width, int height)
{
	ClearGLErrors();

	try
	{
		TryGLCall(glEnable, GL_TEXTURE_2D);
		TryGLCall(glActiveTexture, GL_TEXTURE0);
		if (*texture)
		{
			TryGLCall(glDeleteTextures, 1, texture);
		}
		TryGLCall(glGenTextures, 1, texture);
		TryGLCall(glBindTexture, GL_TEXTURE_2D, *texture);
		TryGLCall(glTexImage2D, GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
		TryGLCall(glTexParameteri, GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		TryGLCall(glTexParameteri, GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		TryGLCall(glTexParameteri, GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, 0);
		TryGLCall(glBindTexture, GL_TEXTURE_2D, 0);
	}
	catch (const OGLErrorException & e)
	{
		gEngfuncs.Con_DPrintf("%s\n", e.what());
		std::cerr << e.what() << std::endl
			<< std::flush;
		g_vrInput.DisplayErrorPopup(e.what());
		*texture = 0;
	}

	ClearGLErrors();

	return *texture != 0;
}

bool CreateGLFrameBuffer(unsigned int* framebuffer, unsigned int texture, int width, int height)
{
	unsigned int rbo = 0;

	ClearGLErrors();

	try
	{
		TryGLCall(glGenFramebuffers, 1, framebuffer);
		TryGLCall(glBindFramebuffer, GL_FRAMEBUFFER, *framebuffer);

		TryGLCall(glFramebufferTexture2D, GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, texture, 0);

		TryGLCall(glGenRenderbuffers, 1, &rbo);
		TryGLCall(glBindRenderbuffer, GL_RENDERBUFFER, rbo);
		TryGLCall(glRenderbufferStorage, GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, width, height);
		TryGLCall(glFramebufferRenderbuffer, GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, rbo);
		TryGLCall(glBindRenderbuffer, GL_RENDERBUFFER, 0);

		TryGLCall(glBindFramebuffer, GL_FRAMEBUFFER, 0);
	}
	catch (const OGLErrorException & e)
	{
		gEngfuncs.Con_DPrintf("%s\n", e.what());
		std::cerr << e.what() << std::endl
			<< std::flush;
		g_vrInput.DisplayErrorPopup(e.what());
		*framebuffer = 0;
		rbo = 0;
	}

	ClearGLErrors();

	return *framebuffer != 0 && rbo != 0;
}

bool InitAdditionalGLFunctions()
{
	glGenFramebuffers = (PFNGLGENFRAMEBUFFERSPROC)GetOpenGLFuncAddress("glGenFramebuffers");
	glGenRenderbuffers = (PFNGLGENFRAMEBUFFERSPROC)GetOpenGLFuncAddress("glGenRenderbuffers");

	glBindFramebuffer = (PFNGLBINDFRAMEBUFFERPROC)GetOpenGLFuncAddress("glBindFramebuffer");
	glBindRenderbuffer = (PFNGLBINDFRAMEBUFFERPROC)GetOpenGLFuncAddress("glBindRenderbuffer");

	glFramebufferTexture2D = (PFNGLFRAMEBUFFERTEXTURE2DPROC)GetOpenGLFuncAddress("glFramebufferTexture2D");
	glRenderbufferStorage = (PFNGLRENDERBUFFERSTORAGEPROC)GetOpenGLFuncAddress("glRenderbufferStorage");
	glFramebufferRenderbuffer = (PFNGLFRAMEBUFFERRENDERBUFFERPROC)GetOpenGLFuncAddress("glFramebufferRenderbuffer");

	glActiveTexture = (PFNGLACTIVETEXTUREPROC)GetOpenGLFuncAddress("glActiveTexture");
	glGenerateMipmap = (PFNGLGENERATEMIPMAPPROC)GetOpenGLFuncAddress("glGenerateMipmap");

	return glGenFramebuffers != nullptr &&
		glGenRenderbuffers != nullptr &&
		glBindFramebuffer != nullptr &&
		glBindRenderbuffer != nullptr &&
		glFramebufferTexture2D != nullptr &&
		glRenderbufferStorage != nullptr &&
		glFramebufferRenderbuffer != nullptr &&
		glActiveTexture != nullptr &&
		glGenerateMipmap != nullptr;
}

void CaptureCurrentScreenToTexture(GLuint texture)
{
	GLint viewport[4];
	glGetIntegerv(GL_VIEWPORT, viewport);

	glEnable(GL_TEXTURE_2D);
	glBindTexture(GL_TEXTURE_2D, texture);
	glCopyTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, 0, 0, viewport[2], viewport[3]);
	glBindTexture(GL_TEXTURE_2D, 0);
}
