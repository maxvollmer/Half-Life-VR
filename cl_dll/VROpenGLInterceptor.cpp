

#include <string>
#include <iostream>

#include "EasyHook/include/easyhook.h"

#include "hud.h"
#include "cl_util.h"
#include "vr_gl.h"
#include "VROpenGLInterceptor.h"


namespace
{
	std::vector<std::shared_ptr<HOOK_TRACE_INFO>> hooks;

	bool hlvr_GLMatricesLocked = false;
	int hlvr_PushCount = 0;
}

void HLVR_LockGLMatrices()
{
	hlvr_GLMatricesLocked = true;
	hlvr_PushCount = 0;
}

void HLVR_UnlockGLMatrices()
{
	hlvr_GLMatricesLocked = false;
	hlvr_PushCount = 0;
}


extern "C" void __stdcall MyGLEnable(GLenum cap)
{
	//OutputDebugStringA("MyGLEnable\n");
	if (!(hlvr_GLMatricesLocked && cap >= GL_CLIP_PLANE0 && cap <= GL_CLIP_PLANE5))
	{
		glEnable(cap);
	}
}

extern "C" void __stdcall MyGLDisable(GLenum cap)
{
	//OutputDebugStringA("MyGLDisable\n");
	if (!(hlvr_GLMatricesLocked && cap == GL_DEPTH_TEST))
	{
		glDisable(cap);
	}
}

extern "C" void __stdcall MyGLViewport(GLint x, GLint y, GLint width, GLsizei height)
{
	//OutputDebugStringA("MyGLViewport\n");
	if (!hlvr_GLMatricesLocked)
	{
		glViewport(x, y, width, height);
	}
}

extern "C" void __stdcall MyGLPushMatrix(void)
{
	//OutputDebugStringA("MyGLPushMatrix\n");
	glPushMatrix();
	hlvr_PushCount++;
}

extern "C" void __stdcall MyGLPopMatrix(void)
{
	//OutputDebugStringA("MyGLPopMatrix\n");
	glPopMatrix();
	hlvr_PushCount--;
}

extern "C" void __stdcall MyGLLoadIdentity(void)
{
	//OutputDebugStringA("MyGLLoadIdentity\n");
	if (!hlvr_GLMatricesLocked)
	{
		glLoadIdentity();
	}
}

extern "C" void __stdcall MyGLMatrixMode(GLenum mode)
{
	//OutputDebugStringA("MyGLMatrixMode\n");
	if (!hlvr_GLMatricesLocked)
	{
		glMatrixMode(mode);
	}
}

extern "C" void __stdcall MyGLLoadMatrixd(const GLdouble * m)
{
	//OutputDebugStringA("MyGLLoadMatrixd\n");
	if (!hlvr_GLMatricesLocked)
	{
		glLoadMatrixd(m);
	}
}

extern "C" void __stdcall MyGLLoadMatrixf(const GLfloat * m)
{
	//OutputDebugStringA("MyGLLoadMatrixf\n");
	if (!hlvr_GLMatricesLocked)
	{
		glLoadMatrixf(m);
	}
}

extern "C" void __stdcall MyGLMultMatrixd(const GLdouble * m)
{
	//OutputDebugStringA("MyGLMultMatrixd\n");
	if (!hlvr_GLMatricesLocked || hlvr_PushCount > 0)
	{
		glMultMatrixd(m);
	}
}

extern "C" void __stdcall MyGLMultMatrixf(const GLfloat * m)
{
	//OutputDebugStringA("MyGLMultMatrixf\n");
	if (!hlvr_GLMatricesLocked || hlvr_PushCount > 0)
	{
		glMultMatrixf(m);
	}
}

extern "C" void __stdcall MyGLScaled(GLdouble x, GLdouble y, GLdouble z)
{
	//OutputDebugStringA("MyGLScaled\n");
	if (!hlvr_GLMatricesLocked || hlvr_PushCount > 0)
	{
		glScaled(x, y, z);
	}
}

extern "C" void __stdcall MyGLScalef(GLfloat x, GLfloat y, GLfloat z)
{
	//OutputDebugStringA("MyGLScalef\n");
	if (!hlvr_GLMatricesLocked || hlvr_PushCount > 0)
	{
		glScalef(x, y, z);
	}
}

extern "C" void __stdcall MyGLTranslated(GLdouble x, GLdouble y, GLdouble z)
{
	//OutputDebugStringA("MyGLTranslated\n");
	if (!hlvr_GLMatricesLocked || hlvr_PushCount > 0)
	{
		glTranslated(x, y, z);
	}
}

extern "C" void __stdcall MyGLTranslatef(GLfloat x, GLfloat y, GLfloat z)
{
	//OutputDebugStringA("MyGLTranslatef\n");
	if (!hlvr_GLMatricesLocked || hlvr_PushCount > 0)
	{
		glTranslatef(x, y, z);
	}
}

extern "C" void __stdcall MyGLRotated(GLdouble angle, GLdouble x, GLdouble y, GLdouble z)
{
	//OutputDebugStringA("MyGLRotated\n");
	if (!hlvr_GLMatricesLocked || hlvr_PushCount > 0)
	{
		glRotated(angle, x, y, z);
	}
}

extern "C" void __stdcall MyGLRotatef(GLfloat angle, GLfloat x, GLfloat y, GLfloat z)
{
	//OutputDebugStringA("MyGLRotatef\n");
	if (!hlvr_GLMatricesLocked || hlvr_PushCount > 0)
	{
		glRotatef(angle, x, y, z);
	}
}

extern "C" void __stdcall MyGLFrustum(GLdouble left, GLdouble right, GLdouble bottom, GLdouble top, GLdouble zNear, GLdouble zFar)
{
	//OutputDebugStringA("MyGLFrustum\n");
	if (!hlvr_GLMatricesLocked)
	{
		glFrustum(left, right, bottom, top, zNear, zFar);
	}
}

bool InterceptOpenGLCall(const char* name, void* hookmethod)
{
	std::shared_ptr<HOOK_TRACE_INFO> hook = std::make_shared<HOOK_TRACE_INFO>(HOOK_TRACE_INFO{ nullptr });
	hooks.push_back(hook);

	extern void* GetOpenGLFuncAddress(const char* name);

	NTSTATUS result = LhInstallHook(
		GetOpenGLFuncAddress(name),
		hookmethod,
		nullptr,
		hook.get());

	if (FAILED(result))
	{
		gEngfuncs.Con_DPrintf("Warning: Failed to install hook for OpenGL method %s: %s\n", name, RtlGetLastErrorString());
		return false;
	}
	else
	{
		ULONG ACLEntries[1] = { 0 };
		LhSetInclusiveACL(ACLEntries, 1, hook.get());
		//gEngfuncs.Con_DPrintf("Successfully installed hook!\n");
		return true;
	}
}

bool InterceptOpenGLCalls()
{
	return 
		InterceptOpenGLCall("glEnable", MyGLEnable) &&
		InterceptOpenGLCall("glDisable", MyGLDisable) &&
		InterceptOpenGLCall("glViewport", MyGLViewport) &&
		InterceptOpenGLCall("glPushMatrix", MyGLPushMatrix) &&
		InterceptOpenGLCall("glPopMatrix", MyGLPopMatrix) &&
		InterceptOpenGLCall("glLoadIdentity", MyGLLoadIdentity) &&
		InterceptOpenGLCall("glMatrixMode", MyGLMatrixMode) &&
		InterceptOpenGLCall("glLoadMatrixd", MyGLLoadMatrixd) &&
		InterceptOpenGLCall("glLoadMatrixf", MyGLLoadMatrixf) &&
		InterceptOpenGLCall("glMultMatrixd", MyGLMultMatrixd) &&
		InterceptOpenGLCall("glMultMatrixf", MyGLMultMatrixf) &&
		InterceptOpenGLCall("glScaled", MyGLScaled) &&
		InterceptOpenGLCall("glScalef", MyGLScalef) &&
		InterceptOpenGLCall("glTranslated", MyGLTranslated) &&
		InterceptOpenGLCall("glTranslatef", MyGLTranslatef) &&
		InterceptOpenGLCall("glRotated", MyGLRotated) &&
		InterceptOpenGLCall("glRotatef", MyGLRotatef) &&
		InterceptOpenGLCall("glFrustum", MyGLFrustum);
}
