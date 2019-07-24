
#include "vr_gl.h"

PFNGLGENFRAMEBUFFERSPROC glGenFramebuffers = nullptr;
PFNGLGENRENDERBUFFERSPROC glGenRenderbuffers = nullptr;

PFNGLBINDFRAMEBUFFERPROC glBindFramebuffer = nullptr;
PFNGLBINDRENDERBUFFERPROC glBindRenderbuffer = nullptr;

PFNGLFRAMEBUFFERTEXTURE2DPROC glFramebufferTexture2D = nullptr;
PFNGLRENDERBUFFERSTORAGEPROC glRenderbufferStorage = nullptr;
PFNGLFRAMEBUFFERRENDERBUFFERPROC glFramebufferRenderbuffer = nullptr;

PFNHLVRLOCKGLMATRICESPROC hlvrLockGLMatrices = nullptr;
PFNHLVRUNLOCKGLMATRICESPROC hlvrUnlockGLMatrices = nullptr;

PFNHLVRSETCONSOLECALLBACKPROC hlvrSetConsoleCallback = nullptr;
PFNHLVRSETGENANDDELETETEXTURESCALLBACKPROC hlvrSetGenAndDeleteTexturesCallback = nullptr;
PFNHLVRSETGLBEGINCALLBACKPROC hlvrSetGLBeginCallback = nullptr;

PFNGLACTIVETEXTUREPROC glActiveTexture = nullptr;
PFNGLGENERATEMIPMAPPROC glGenerateMipmap = nullptr;

void* GetOpenGLFuncAddress(const char *name)
{
#ifdef WIN32
	void *p = (void *)wglGetProcAddress(name);
	if (p == nullptr ||
		(p == (void*)0x1) || (p == (void*)0x2) || (p == (void*)0x3) ||
		(p == (void*)-1))
	{
		HMODULE module = LoadLibraryA("opengl32.dll");
		if (module)
		{
			p = (void*)GetProcAddress(module, name);
		}
	}
	return p;
#else
	return glXGetProcAddress(name);
#endif
}

#include <functional>
#include <stdexcept>
#include <string>
#include <iostream>
#include <sstream>
#include "hud.h"
#include "cl_util.h"
#include "VRInput.h"

class OGLErrorException : public std::exception
{
public:
	OGLErrorException(const std::string& errormsg) :
		m_errormsg{ errormsg }
	{
	}

	virtual char const* what() const override
	{
		return m_errormsg.data();
	}

private:
	std::string m_errormsg;
};

void TryTryGLCall(std::function<void()> call, const char* name)
{
	call();
	auto error = glGetError();
	if (error != GL_NO_ERROR)
	{
		std::stringstream errormsg;
		errormsg << "OpenGL call \"";
		errormsg << name;
		errormsg << "\"failed with error: ";
		errormsg << error;
		throw OGLErrorException{ errormsg.str() };
	}
}

#define TryGLCall(call, ...) TryTryGLCall([&](){call(__VA_ARGS__);}, #call)

bool CreateGLTexture(unsigned int* texture, int width, int height)
{
	extern bool gIsOwnCallToGenTextures;
	gIsOwnCallToGenTextures = true;

	try
	{
		TryGLCall(glEnable, GL_TEXTURE_2D);
		TryGLCall(glEnable, GL_TEXTURE_2D);
		TryGLCall(glActiveTexture, GL_TEXTURE0);
		if (*texture)
		{
			TryGLCall(glDeleteTextures, 1, texture);
		}
		TryGLCall(glGenTextures, 1, texture);
		TryGLCall(glBindTexture, GL_TEXTURE_2D, *texture);
		TryGLCall(glTexImage2D, GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_BYTE, nullptr);
		TryGLCall(glTexParameteri, GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		TryGLCall(glTexParameteri, GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		TryGLCall(glBindTexture, GL_TEXTURE_2D, 0);
	}
	catch (const OGLErrorException& e)
	{
		gEngfuncs.Con_DPrintf("%s\n", e.what());
		std::cerr << e.what() << std::endl << std::flush;
		g_vrInput.DisplayErrorPopup(e.what());
		*texture = 0;
	}

	gIsOwnCallToGenTextures = false;
	return *texture != 0;
}

bool CreateGLFrameBuffer(unsigned int* framebuffer, unsigned int texture, int width, int height)
{
	unsigned int rbo = 0;

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
	catch (const OGLErrorException& e)
	{
		gEngfuncs.Con_DPrintf("%s\n", e.what());
		std::cerr << e.what() << std::endl << std::flush;
		g_vrInput.DisplayErrorPopup(e.what());
		*framebuffer = 0;
		rbo = 0;
	}

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

	return
		glGenFramebuffers != nullptr &&
		glGenRenderbuffers != nullptr &&
		glBindFramebuffer != nullptr &&
		glBindRenderbuffer != nullptr &&
		glFramebufferTexture2D != nullptr &&
		glRenderbufferStorage != nullptr &&
		glFramebufferRenderbuffer != nullptr &&
		glActiveTexture != nullptr &&
		glGenerateMipmap != nullptr;
}

// Separate for better error messages
bool InitGLMatrixOverrideFunctions()
{
	hlvrLockGLMatrices = (PFNHLVRLOCKGLMATRICESPROC)GetOpenGLFuncAddress("hlvrLockGLMatrices");
	hlvrUnlockGLMatrices = (PFNHLVRUNLOCKGLMATRICESPROC)GetOpenGLFuncAddress("hlvrUnlockGLMatrices");

	return
		hlvrLockGLMatrices != nullptr &&
		hlvrUnlockGLMatrices != nullptr;

}

bool InitGLCallbackFunctions()
{
	hlvrSetConsoleCallback = (PFNHLVRSETCONSOLECALLBACKPROC)GetOpenGLFuncAddress("hlvrSetConsoleCallback");
	hlvrSetGenAndDeleteTexturesCallback = (PFNHLVRSETGENANDDELETETEXTURESCALLBACKPROC)GetOpenGLFuncAddress("hlvrSetGenAndDeleteTexturesCallback");
	hlvrSetGLBeginCallback = (PFNHLVRSETGLBEGINCALLBACKPROC)GetOpenGLFuncAddress("hlvrSetGLBeginCallback");

	if (hlvrSetConsoleCallback != nullptr && hlvrSetGenAndDeleteTexturesCallback != nullptr && hlvrSetGLBeginCallback != nullptr)
	{
		hlvrSetConsoleCallback(&HLVRConsoleCallback);
		hlvrSetGenAndDeleteTexturesCallback(&HLVRGenTexturesCallback, &HLVRDeleteTexturesCallback);
		hlvrSetGLBeginCallback(&HLVRGLBeginCallback, &HLVRGLEndCallback);
		return true;
	}

	return false;
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
