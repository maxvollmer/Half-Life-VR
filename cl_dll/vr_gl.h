
#pragma once

#include "GL/gl.h"

#define GL_FRAMEBUFFER 0x8D40
#define GL_RENDERBUFFER 0x8D41
#define GL_COLOR_ATTACHMENT0 0x8CE0
#define GL_DEPTH24_STENCIL8 0x88F0
#define GL_DEPTH_STENCIL_ATTACHMENT 0x821A
#define GL_ACTIVE_TEXTURE 0x84E0
#define GL_TEXTURE0 0x84C0

typedef void (APIENTRY * PFNGLGENFRAMEBUFFERSPROC) (GLsizei n, GLuint* framebuffers);
typedef void (APIENTRY * PFNGLGENRENDERBUFFERSPROC) (GLsizei n, GLuint* renderbuffers);

typedef void (APIENTRY * PFNGLBINDFRAMEBUFFERPROC) (GLenum target, GLuint framebuffer);
typedef void (APIENTRY * PFNGLBINDRENDERBUFFERPROC) (GLenum target, GLuint renderbuffer);

typedef void (APIENTRY * PFNGLFRAMEBUFFERTEXTURE2DPROC) (GLenum target, GLenum attachment, GLenum textarget, GLuint texture, GLint level);
typedef void (APIENTRY * PFNGLRENDERBUFFERSTORAGEPROC) (GLenum target, GLenum internalformat, GLsizei width, GLsizei height);
typedef void (APIENTRY * PFNGLFRAMEBUFFERRENDERBUFFERPROC) (GLenum target, GLenum attachment, GLenum renderbuffertarget, GLuint renderbuffer);

typedef void (APIENTRY * PFNGLACTIVETEXTUREPROC) (GLenum texture);

typedef void (APIENTRY * PFNHLVRLOCKGLMATRICESPROC) (void);
typedef void (APIENTRY * PFNHLVRUNLOCKGLMATRICESPROC) (void);

extern PFNGLGENFRAMEBUFFERSPROC glGenFramebuffers;
extern PFNGLGENRENDERBUFFERSPROC glGenRenderbuffers;

extern PFNGLBINDFRAMEBUFFERPROC glBindFramebuffer;
extern PFNGLBINDRENDERBUFFERPROC glBindRenderbuffer;

extern PFNGLFRAMEBUFFERTEXTURE2DPROC glFramebufferTexture2D;
extern PFNGLRENDERBUFFERSTORAGEPROC glRenderbufferStorage;
extern PFNGLFRAMEBUFFERRENDERBUFFERPROC glFramebufferRenderbuffer;

extern PFNHLVRLOCKGLMATRICESPROC hlvrLockGLMatrices;
extern PFNHLVRUNLOCKGLMATRICESPROC hlvrUnlockGLMatrices;

extern PFNGLACTIVETEXTUREPROC glActiveTexture;

bool CreateGLTexture(unsigned int* texture, int width, int height);
bool CreateGLFrameBuffer(unsigned int* framebuffer, unsigned int texture, int width, int height);

bool InitAdditionalGLFunctions();
bool InitGLMatrixOverrideFunctions();

void CaptureCurrentScreenToTexture(GLuint texture);

