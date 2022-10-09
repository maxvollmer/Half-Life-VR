
#pragma once

#include <windows.h>

#include <functional>
#include <stdexcept>
#include <string>
#include <iostream>
#include <sstream>
#include <iterator>

#include "GL/gl.h"

#define GL_FRAMEBUFFER              0x8D40
#define GL_RENDERBUFFER             0x8D41
#define GL_COLOR_ATTACHMENT0        0x8CE0
#define GL_DEPTH24_STENCIL8         0x88F0
#define GL_DEPTH_STENCIL_ATTACHMENT 0x821A
#define GL_ACTIVE_TEXTURE           0x84E0
#define GL_TEXTURE0                 0x84C0
#define GL_TEXTURE_MAX_LEVEL        0x813D

typedef void(APIENTRY* PFNGLGENFRAMEBUFFERSPROC)(GLsizei n, GLuint* framebuffers);
typedef void(APIENTRY* PFNGLGENRENDERBUFFERSPROC)(GLsizei n, GLuint* renderbuffers);

typedef void(APIENTRY* PFNGLBINDFRAMEBUFFERPROC)(GLenum target, GLuint framebuffer);
typedef void(APIENTRY* PFNGLBINDRENDERBUFFERPROC)(GLenum target, GLuint renderbuffer);

typedef void(APIENTRY* PFNGLFRAMEBUFFERTEXTURE2DPROC)(GLenum target, GLenum attachment, GLenum textarget, GLuint texture, GLint level);
typedef void(APIENTRY* PFNGLRENDERBUFFERSTORAGEPROC)(GLenum target, GLenum internalformat, GLsizei width, GLsizei height);
typedef void(APIENTRY* PFNGLFRAMEBUFFERRENDERBUFFERPROC)(GLenum target, GLenum attachment, GLenum renderbuffertarget, GLuint renderbuffer);

typedef void(APIENTRY* PFNGLACTIVETEXTUREPROC)(GLenum texture);
typedef void(APIENTRY* PFNGLGENERATEMIPMAPPROC)(GLenum texture);

extern PFNGLGENFRAMEBUFFERSPROC glGenFramebuffers;
extern PFNGLGENRENDERBUFFERSPROC glGenRenderbuffers;

extern PFNGLBINDFRAMEBUFFERPROC glBindFramebuffer;
extern PFNGLBINDRENDERBUFFERPROC glBindRenderbuffer;

extern PFNGLFRAMEBUFFERTEXTURE2DPROC glFramebufferTexture2D;
extern PFNGLRENDERBUFFERSTORAGEPROC glRenderbufferStorage;
extern PFNGLFRAMEBUFFERRENDERBUFFERPROC glFramebufferRenderbuffer;

extern PFNGLACTIVETEXTUREPROC glActiveTexture;
extern PFNGLGENERATEMIPMAPPROC glGenerateMipmap;

extern void __stdcall HLVRConsoleCallback(char* msg);
extern void __stdcall HLVRGenTexturesCallback(GLsizei, GLuint*);
extern void __stdcall HLVRDeleteTexturesCallback(GLsizei, const GLuint*);
extern void __stdcall HLVRGLBeginCallback(GLenum);
extern void __stdcall HLVRGLEndCallback();

bool CreateGLTexture(unsigned int* texture, int width, int height);
bool CreateGLFrameBuffer(unsigned int* framebuffer, unsigned int texture, int width, int height);

bool InitAdditionalGLFunctions();

void CaptureCurrentScreenToTexture(GLuint texture);


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

template<class T>
auto ArgToString(T&& t)
{
	std::stringstream s;
	s << std::forward<T>(t);
	return s.str();
}

template<class... T>
auto VarArgsToString(T&&... args)
{
	std::vector<std::string> x{ ArgToString(std::forward<T>(args))... };
	std::stringstream s;
	std::copy(x.begin(), x.end(), std::ostream_iterator<std::string>(s, ", "));
	return s.str();
}

void TryTryGLCall(std::function<void()> call, const std::string& name, const std::string& args);

#define TryGLCall(call, ...) TryTryGLCall([&]() { call(__VA_ARGS__); }, #call, VarArgsToString(__VA_ARGS__))

void ClearGLErrors();
