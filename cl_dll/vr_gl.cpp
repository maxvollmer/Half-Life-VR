
#include <windows.h>
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

PFNGLACTIVETEXTUREPROC glActiveTexture = nullptr;

void* GetOpenGLFuncAddress(const char *name)
{
#ifdef WIN32
	void *p = (void *)wglGetProcAddress(name);
	if (p == nullptr ||
		(p == (void*)0x1) || (p == (void*)0x2) || (p == (void*)0x3) ||
		(p == (void*)-1))
	{
		HMODULE module = LoadLibraryA("opengl32.dll");
		p = (void *)GetProcAddress(module, name);
	}
	return p;
#else
	return glXGetProcAddress(name);
#endif
}

bool CreateGLTexture(unsigned int* texture, int width, int height)
{
	//SetActiveTexture(GL_TEXTURE0);
	glGenTextures(1, texture);
	glEnable(GL_TEXTURE_2D);
	glBindTexture(GL_TEXTURE_2D, *texture);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_BYTE, nullptr);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glBindTexture(GL_TEXTURE_2D, 0);
	return *texture != 0; // TODO: Proper error handling
}

bool CreateGLFrameBuffer(unsigned int* framebuffer, unsigned int texture, int width, int height)
{
	glGenFramebuffers(1, framebuffer);
	glBindFramebuffer(GL_FRAMEBUFFER, *framebuffer);

	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, texture, 0);

	unsigned int rbo;
	glGenRenderbuffers(1, &rbo);
	glBindRenderbuffer(GL_RENDERBUFFER, rbo);
	glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, width, height);
	glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, rbo);
	glBindRenderbuffer(GL_RENDERBUFFER, 0);

	glBindFramebuffer(GL_FRAMEBUFFER, 0);

	return *framebuffer != 0 && rbo != 0; // TODO: Proper error handling
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

	return
		glGenFramebuffers != nullptr &&
		glGenRenderbuffers != nullptr &&
		glBindFramebuffer != nullptr &&
		glBindRenderbuffer != nullptr &&
		glFramebufferTexture2D != nullptr &&
		glRenderbufferStorage != nullptr &&
		glFramebufferRenderbuffer != nullptr &&
		glActiveTexture != nullptr;
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

void CaptureCurrentScreenToTexture(GLuint texture)
{
	GLint viewport[4];
	glGetIntegerv(GL_VIEWPORT, viewport);

	glEnable(GL_TEXTURE_2D);
	glBindTexture(GL_TEXTURE_2D, texture);
	glCopyTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, 0, 0, viewport[2], viewport[3]);
	glBindTexture(GL_TEXTURE_2D, 0);
}
