
// ================================================================================================
// -*- C++ -*-
// File: opengl_proxy.cpp
// Author: Guilherme R. Lampert
// Created on: 22/11/15
// Brief: GLProxy intercepts all calls to the real OpenGL library.
//
// Source code licensed under the MIT license.
// Copyright (C) 2015 Guilherme R. Lampert
//
// This software is provided "as is" without express or implied
// warranties. You may freely copy and compile this source into
// applications you distribute provided that this copyright text
// is included in the resulting source code.
// ================================================================================================

// ================================================================================================
// Modified for Half-Life: VR by Max Vollmer, 2017-08-15
// ================================================================================================

// Added option to disable logging - Max Vollmer, 2017-08-15
// #define GLPROXY_LOG_ENABLED

// Added logging of any function call - Max Vollmer, 2017-08-17
void logCall(const char* funcName);
// #define GLPROXY_LOG_CALLS_ENABLED

#ifdef GLPROXY_LOG_CALLS_ENABLED
#define GLPROXY_LOG_CALL(funcName) logCall(funcName)
#else
#define GLPROXY_LOG_CALL(funcName) do {} while(0)
#endif


// Trim down the WinAPI crap. We also don't want WinGDI.h
// to interfere with our WGL wrapper declarations.
#define NOGDI
#define VC_EXTRALEAN
#define WIN32_LEAN_AND_MEAN

#include <windows.h>
#include <algorithm>
#include <fstream>
#include <string>
#include <vector>
#include <cstdint>
#include <cstdlib>
#include <ctime>

// We are not including 'WinGDI.h' and 'gl.h', so the
// required types must be redefined in this source file.
#define GLPROXY_NEED_OGL_TYPES
#define GLPROXY_NEED_WGL_STRUCTS

// ========================================================
// Local helper macros:
// ========================================================

// To pass the silly "constant expression" warning from Visual Studio...
#define END_MACRO while (0,0)

// Local log stream output:
#define GLPROXY_LOG(message) do { GLProxy::getLogStream() << message << std::endl; } END_MACRO

// Appends a pair tokens into a single name/identifier.
// Normally used to declared internal/built-in functions and variables.
#define STRING_JOIN2_HELPER(a, b) a ## b
#define STRING_JOIN2(a, b) STRING_JOIN2_HELPER(a, b)

// Code/text to string:
#define STRINGIZE_HELPER(str) #str
#define STRINGIZE(str) STRINGIZE_HELPER(str)

//
// Calling convention used by OGL functions on Windows is stdcall.
// OpenGL is also a C API, so the 'extern C' should be the correct approach.
// However, these qualifiers alone don't seem enough to prevent the linker from
// decorating our function names, so an additional '.def' file is also required.
//
#define GLPROXY_DECL   __stdcall
#define GLPROXY_EXTERN extern "C"

// ========================================================
// GLProxy utilities:
// ========================================================

namespace GLProxy
{

static std::string numToString(std::uint64_t num)
{
    char tempString[128];
    std::snprintf(tempString, sizeof(tempString), "%-10llu", num);
    return tempString;
}

static std::string ptrToString(const void * ptr)
{
    char tempString[128];

    #if defined(_M_IX86)
    std::snprintf(tempString, sizeof(tempString), "0x%08X",
                  reinterpret_cast<std::uintptr_t>(ptr));
    #elif defined(_M_X64)
    std::snprintf(tempString, sizeof(tempString), "0x%016llX",
                  reinterpret_cast<std::uintptr_t>(ptr));
    #endif // x86/64

    return tempString;
}

static std::string getTimeString()
{
    const std::time_t rawTime = std::time(nullptr);
    std::string ts;

    #ifdef _MSC_VER
    // Visual Studio dislikes the static buffer of std::ctime.
    char tempString[256] = {'\0'};
    ctime_s(tempString, sizeof(tempString), &rawTime);
    ts = tempString;
    #else // !_MSC_VER
    ts = std::ctime(&rawTime);
    #endif // _MSC_VER

    ts.pop_back(); // Remove the default '\n' added by ctime.
    return ts;
}

class NullBuffer : public std::streambuf
{
public:
	int overflow(int c) { return c; }
};
NullBuffer null_buffer;

static std::ostream & getLogStream()
{
#ifdef GLPROXY_LOG_ENABLED
    static std::ofstream theLog("opengl_proxy.log");
#else
	static std::ostream theLog(&null_buffer);
#endif
    return theLog;
}


static std::string lastWinErrorAsString()
{
    // Adapted from this SO thread:
    // http://stackoverflow.com/a/17387176/1198654

    DWORD errorMessageID = GetLastError();
    if (errorMessageID == 0)
    {
        return "Unknown error";
    }

    LPSTR messageBuffer = nullptr;
    constexpr DWORD fmtFlags = FORMAT_MESSAGE_ALLOCATE_BUFFER |
                               FORMAT_MESSAGE_FROM_SYSTEM     |
                               FORMAT_MESSAGE_IGNORE_INSERTS;

    const auto size = FormatMessageA(fmtFlags, nullptr, errorMessageID,
                                     MAKELANGID(LANG_NEUTRAL, SUBLANG_SYS_DEFAULT),
                                     (LPSTR)&messageBuffer, 0, nullptr);

    const std::string message{ messageBuffer, size };
    LocalFree(messageBuffer);

    return message + "(error " + std::to_string(errorMessageID) + ")";
}

static std::string getRealGLLibPath()
{
    char defaultGLLibName[1024] = {'\0'};
    GetSystemDirectoryA(defaultGLLibName, sizeof(defaultGLLibName));

    std::string result;
    if (defaultGLLibName[0] != '\0')
    {
        result += defaultGLLibName;
        result += "\\opengl32.dll";
    }
    else // Something wrong... Try a hardcoded path...
    {
        result = "C:\\windows\\system32\\opengl32.dll";
    }
    return result;
}

static HMODULE getSelfModuleHandle()
{
    //
    // This is somewhat hackish, but should work.
    // We try to get this module's address from the address
    // of one of its functions, this very function actually.
    // Worst case it fails and we return null.
    //
    // There's also the '__ImageBase' hack, but that seems even more precarious...
    // http://stackoverflow.com/a/6924293/1198654
    //
    HMODULE selfHMod = nullptr;
    GetModuleHandleExA(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS |
                       GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
                       (LPCSTR)&getSelfModuleHandle,
                       &selfHMod);
    return selfHMod;
}

DECLSPEC_NORETURN static void fatalError(const std::string & message)
{
    MessageBoxA(nullptr, message.c_str(), "GLProxy Fatal Error", MB_OK | MB_ICONERROR);

    GLPROXY_LOG("Fatal error: " << message);
    getLogStream().flush();

    std::exit(EXIT_FAILURE);
}

// ========================================================
// class OpenGLDll:
//  Simple helper class to manage loading the real OpenGL
//  Dynamic Library and fetching function pointers from it.
// ========================================================

class OpenGLDll final
{
    HMODULE     dllHandle;
    std::string dllFilePath;

public:

    // Not copyable.
    OpenGLDll(const OpenGLDll &) = delete;
    OpenGLDll & operator = (const OpenGLDll &) = delete;

    OpenGLDll()
        : dllHandle{ nullptr }
    {
        load();
    }

    ~OpenGLDll()
    {
        unload();
    }

    void load()
    {
        if (isLoaded())
        {
            fatalError("Real OpenGL DLL is already loaded!");
        }

        const auto glDllFilePath = getRealGLLibPath();
        GLPROXY_LOG("Trying to load real opengl32.dll from \"" << glDllFilePath << "\"...");

        dllHandle = LoadLibraryExA(glDllFilePath.c_str(), nullptr, LOAD_LIBRARY_SEARCH_SYSTEM32);
        if (dllHandle == nullptr)
        {
            fatalError("GLProxy unable to load the real OpenGL DLL!\n" + lastWinErrorAsString());
        }

        const auto selfHMod = getSelfModuleHandle();
        if (dllHandle == selfHMod)
        {
            fatalError("GLProxy trying to load itself as the real opengl32.dll!");
        }

        char tempString[1024] = {'\0'};
        if (GetModuleFileNameA(dllHandle, tempString, sizeof(tempString)) == 0)
        {
            GLPROXY_LOG("Unable to get Real OpenGL DLL file path!");
        }
        else
        {
            dllFilePath = tempString;
        }

        GLPROXY_LOG("\n--------------------------------------------------------");
        GLPROXY_LOG("  Real OpenGL DLL is loaded!");
        GLPROXY_LOG("  OpenGL = " + ptrToString(dllHandle) + ", GLProxy = " + ptrToString(selfHMod));
        GLPROXY_LOG("  opengl32.dll path: \"" + dllFilePath + "\"");
        GLPROXY_LOG("--------------------------------------------------------\n");
    }

    void unload()
    {
        if (isLoaded())
        {
            FreeLibrary(dllHandle);
            dllHandle = nullptr;
            dllFilePath.clear();
        }
    }

    bool isLoaded() const
    {
        return dllHandle != nullptr;
    }

    void * getFuncPtr(const char * funcName) const
    {
        if (!isLoaded())
        {
            GLPROXY_LOG("Error! Real opengl32.dll not loaded. Can't get function " << funcName);
            return nullptr;
        }

        auto fptr = GetProcAddress(dllHandle, funcName);
        if (fptr == nullptr)
        {
            GLPROXY_LOG("Error! Unable to find " << funcName);
        }

        return reinterpret_cast<void *>(fptr);
    }

    // Just one instance per process.
    // Also only attempt to load the DLL on the first reference.
    static OpenGLDll & getInstance()
    {
        static OpenGLDll glDll;
        return glDll;
    }
};

static void * getRealGLFunc(const char * funcName)
{
    auto & glDll = OpenGLDll::getInstance();
    void * addr  = glDll.getFuncPtr(funcName);
    GLPROXY_LOG("Loading real GL func: (" << ptrToString(addr) << ") " << funcName);
    return addr;
}

// ========================================================
// GL function pointers database:
// ========================================================

//
// Registry for the real functions from the GL DLL.
//
struct GLFuncBase
{
    std::uint64_t callCount; // Times called during program lifetime.
    const char * name;       // Pointer to static string. OGL function name, like "glEnable".
    const GLFuncBase * next; // Pointer to next static instance in the global list.
};

// Linked list of TGLFuncs, pointing to the actual OpenGL DLL methods.
static GLFuncBase * g_RealGLFunctions = nullptr;

//
// Each function requires a different signature...
// These are always declared as static instances.
//
template<class RetType, class... ParamTypes>
struct TGLFunc : GLFuncBase
{
    typedef RetType (GLPROXY_DECL * PtrType)(ParamTypes...);
    PtrType funcPtr; // Pointer to a GL function inside the actual OpenGL DLL.

    TGLFunc(const char * funcName)
    {
        callCount = 0;
        name      = funcName;
        funcPtr   = reinterpret_cast<PtrType>(getRealGLFunc(funcName));

        // Link to the global list:
        this->next = g_RealGLFunctions;
        g_RealGLFunctions = this;
    }
};

// Shorthand macros to simplify the TGLFunc<> declarations:
#define TGLFUNC_NAME(funcName) STRING_JOIN2(_real_, funcName)
#define TGLFUNC_DECL(funcName) TGLFUNC_NAME(funcName){ STRINGIZE(funcName) }
#define TGLFUNC_CALL(funcName, ...) (TGLFUNC_NAME(funcName).callCount++, TGLFUNC_NAME(funcName).funcPtr(__VA_ARGS__))

// ========================================================
// struct AutoReport:
//  Writes a report with the OpenGL function call counts.
// ========================================================

struct AutoReport
{
    AutoReport()
    {
        GLPROXY_LOG("\n--------------------------------------------------------");
        GLPROXY_LOG("  OPENGL32.DLL proxy report - " << getTimeString());
        GLPROXY_LOG("--------------------------------------------------------\n");
    }

    ~AutoReport()
    {
        // Gather all function pointers first so we can sort them by call count.
        std::vector<const GLFuncBase *> sortedFuncs;
        for (const GLFuncBase * func = g_RealGLFunctions; func; func = func->next)
        {
            sortedFuncs.push_back(func);
        }

        // Higher call counts first. If same call count then sort alphabetically by name.
        std::sort(std::begin(sortedFuncs), std::end(sortedFuncs),
                  [](const GLFuncBase * a, const GLFuncBase * b) -> bool
                  {
                      if (a->callCount == b->callCount)
                      {
                          return std::strcmp(a->name, b->name) < 0;
                      }
                      else
                      {
                          return a->callCount > b->callCount;
                      }
                  });

        GLPROXY_LOG("--------------------------------------------------------");
        GLPROXY_LOG("  Function call counts:");
        GLPROXY_LOG("--------------------------------------------------------\n");

        for (const GLFuncBase * func : sortedFuncs)
        {
            GLPROXY_LOG(numToString(func->callCount) << " " << func->name);
        }

        GLPROXY_LOG("\n" << sortedFuncs.size() << " GL functions were called by the application.");
    }
};

// This static instance will open the GLProxy log on startup and
// write the function call counts on shutdown via its destructor.
static AutoReport g_AutoReport;

} // namespace GLProxy {}

// ========================================================
// DllMain:
//  Note: Threads are not supported.
//  Probably a non issue, since OpenGL is single-threaded.
// ========================================================

BOOL WINAPI DllMain(HINSTANCE /* hInstDll */, DWORD reasonForDllLoad, LPVOID /* reserved */)
{
    switch (reasonForDllLoad)
    {
    case DLL_PROCESS_ATTACH :
        GLPROXY_LOG("\nDllMain: DLL_PROCESS_ATTACH\n");
        break;

    case DLL_PROCESS_DETACH :
        GLPROXY_LOG("\nDllMain: DLL_PROCESS_DETACH\n");
        break;

    default :
        break;
    } // switch (reasonForDllLoad)

    return TRUE;
}

// ================================================================================================
// These macros simplify declaring our wrapper functions:
// ================================================================================================

//
// Functions with a return value:
//

#define GLFUNC_0_WRET(retType, funcName)                         \
    GLPROXY_EXTERN retType GLPROXY_DECL funcName(void)           \
    {                                                            \
		GLPROXY_LOG_CALL(STRINGIZE(funcName));																							\
        static GLProxy::TGLFunc<retType> TGLFUNC_DECL(funcName); \
        return TGLFUNC_CALL(funcName);                           \
    }

#define GLFUNC_1_WRET(retType, funcName, t0, p0)                     \
    GLPROXY_EXTERN retType GLPROXY_DECL funcName(t0 p0)              \
    {                                                                \
		GLPROXY_LOG_CALL(STRINGIZE(funcName));																							\
        static GLProxy::TGLFunc<retType, t0> TGLFUNC_DECL(funcName); \
        return TGLFUNC_CALL(funcName, p0);                           \
    }

#define GLFUNC_2_WRET(retType, funcName, t0, p0, t1, p1)                 \
    GLPROXY_EXTERN retType GLPROXY_DECL funcName(t0 p0, t1 p1)           \
    {                                                                    \
		GLPROXY_LOG_CALL(STRINGIZE(funcName));																							\
        static GLProxy::TGLFunc<retType, t0, t1> TGLFUNC_DECL(funcName); \
        return TGLFUNC_CALL(funcName, p0, p1);                           \
    }

#define GLFUNC_3_WRET(retType, funcName, t0, p0, t1, p1, t2, p2)             \
    GLPROXY_EXTERN retType GLPROXY_DECL funcName(t0 p0, t1 p1, t2 p2)        \
    {                                                                        \
		GLPROXY_LOG_CALL(STRINGIZE(funcName));																							\
        static GLProxy::TGLFunc<retType, t0, t1, t2> TGLFUNC_DECL(funcName); \
        return TGLFUNC_CALL(funcName, p0, p1, p2);                           \
    }

#define GLFUNC_4_WRET(retType, funcName, t0, p0, t1, p1, t2, p2, t3, p3)         \
    GLPROXY_EXTERN retType GLPROXY_DECL funcName(t0 p0, t1 p1, t2 p2, t3 p3)     \
    {                                                                            \
		GLPROXY_LOG_CALL(STRINGIZE(funcName));																							\
        static GLProxy::TGLFunc<retType, t0, t1, t2, t3> TGLFUNC_DECL(funcName); \
        return TGLFUNC_CALL(funcName, p0, p1, p2, p3);                           \
    }

#define GLFUNC_5_WRET(retType, funcName, t0, p0, t1, p1, t2, p2, t3, p3, t4, p4)     \
    GLPROXY_EXTERN retType GLPROXY_DECL funcName(t0 p0, t1 p1, t2 p2, t3 p3, t4 p4)  \
    {                                                                                \
		GLPROXY_LOG_CALL(STRINGIZE(funcName));																							\
        static GLProxy::TGLFunc<retType, t0, t1, t2, t3, t4> TGLFUNC_DECL(funcName); \
        return TGLFUNC_CALL(funcName, p0, p1, p2, p3, p4);                           \
    }

#define GLFUNC_8_WRET(retType, funcName, t0, p0, t1, p1, t2, p2, t3, p3, t4, p4, t5, p5, t6, p6, t7, p7) \
    GLPROXY_EXTERN retType GLPROXY_DECL funcName(t0 p0, t1 p1, t2 p2, t3 p3, t4 p4, t5 p5, t6 p6, t7 p7) \
    {                                                                                                    \
		GLPROXY_LOG_CALL(STRINGIZE(funcName));																							\
        static GLProxy::TGLFunc<retType, t0, t1, t2, t3, t4, t5, t6, t7> TGLFUNC_DECL(funcName);         \
        return TGLFUNC_CALL(funcName, p0, p1, p2, p3, p4, p5, p6, p7);                                   \
    }

//
// Functions returning void/nothing:
//

#define GLFUNC_0(funcName)                                    \
    GLPROXY_EXTERN void GLPROXY_DECL funcName(void)           \
    {                                                         \
		GLPROXY_LOG_CALL(STRINGIZE(funcName));																							\
        static GLProxy::TGLFunc<void> TGLFUNC_DECL(funcName); \
        TGLFUNC_CALL(funcName);                               \
    }

#define GLFUNC_1(funcName, t0, p0)                                \
    GLPROXY_EXTERN void GLPROXY_DECL funcName(t0 p0)              \
    {                                                             \
		GLPROXY_LOG_CALL(STRINGIZE(funcName));																							\
        static GLProxy::TGLFunc<void, t0> TGLFUNC_DECL(funcName); \
        TGLFUNC_CALL(funcName, p0);                               \
    }

#define GLFUNC_2(funcName, t0, p0, t1, p1)                            \
    GLPROXY_EXTERN void GLPROXY_DECL funcName(t0 p0, t1 p1)           \
    {                                                                 \
		GLPROXY_LOG_CALL(STRINGIZE(funcName));																							\
        static GLProxy::TGLFunc<void, t0, t1> TGLFUNC_DECL(funcName); \
        TGLFUNC_CALL(funcName, p0, p1);                               \
    }

#define GLFUNC_3(funcName, t0, p0, t1, p1, t2, p2)                        \
    GLPROXY_EXTERN void GLPROXY_DECL funcName(t0 p0, t1 p1, t2 p2)        \
    {                                                                     \
		GLPROXY_LOG_CALL(STRINGIZE(funcName));																							\
        static GLProxy::TGLFunc<void, t0, t1, t2> TGLFUNC_DECL(funcName); \
        TGLFUNC_CALL(funcName, p0, p1, p2);                               \
    }

#define GLFUNC_4(funcName, t0, p0, t1, p1, t2, p2, t3, p3)                    \
    GLPROXY_EXTERN void GLPROXY_DECL funcName(t0 p0, t1 p1, t2 p2, t3 p3)     \
    {                                                                         \
		GLPROXY_LOG_CALL(STRINGIZE(funcName));																							\
        static GLProxy::TGLFunc<void, t0, t1, t2, t3> TGLFUNC_DECL(funcName); \
        TGLFUNC_CALL(funcName, p0, p1, p2, p3);                               \
    }

#define GLFUNC_5(funcName, t0, p0, t1, p1, t2, p2, t3, p3, t4, p4)                \
    GLPROXY_EXTERN void GLPROXY_DECL funcName(t0 p0, t1 p1, t2 p2, t3 p3, t4 p4)  \
    {                                                                             \
		GLPROXY_LOG_CALL(STRINGIZE(funcName));																							\
        static GLProxy::TGLFunc<void, t0, t1, t2, t3, t4> TGLFUNC_DECL(funcName); \
        TGLFUNC_CALL(funcName, p0, p1, p2, p3, p4);                               \
    }

#define GLFUNC_6(funcName, t0, p0, t1, p1, t2, p2, t3, p3, t4, p4, t5, p5)              \
    GLPROXY_EXTERN void GLPROXY_DECL funcName(t0 p0, t1 p1, t2 p2, t3 p3, t4 p4, t5 p5) \
    {                                                                                   \
		GLPROXY_LOG_CALL(STRINGIZE(funcName));																							\
        static GLProxy::TGLFunc<void, t0, t1, t2, t3, t4, t5> TGLFUNC_DECL(funcName);   \
        TGLFUNC_CALL(funcName, p0, p1, p2, p3, p4, p5);                                 \
    }

#define GLFUNC_7(funcName, t0, p0, t1, p1, t2, p2, t3, p3, t4, p4, t5, p5, t6, p6)             \
    GLPROXY_EXTERN void GLPROXY_DECL funcName(t0 p0, t1 p1, t2 p2, t3 p3, t4 p4, t5 p5, t6 p6) \
    {                                                                                          \
		GLPROXY_LOG_CALL(STRINGIZE(funcName));																							\
        static GLProxy::TGLFunc<void, t0, t1, t2, t3, t4, t5, t6> TGLFUNC_DECL(funcName);      \
        TGLFUNC_CALL(funcName, p0, p1, p2, p3, p4, p5, p6);                                    \
    }

#define GLFUNC_8(funcName, t0, p0, t1, p1, t2, p2, t3, p3, t4, p4, t5, p5, t6, p6, t7, p7)            \
    GLPROXY_EXTERN void GLPROXY_DECL funcName(t0 p0, t1 p1, t2 p2, t3 p3, t4 p4, t5 p5, t6 p6, t7 p7) \
    {                                                                                                 \
		GLPROXY_LOG_CALL(STRINGIZE(funcName));																							\
        static GLProxy::TGLFunc<void, t0, t1, t2, t3, t4, t5, t6, t7> TGLFUNC_DECL(funcName);         \
        TGLFUNC_CALL(funcName, p0, p1, p2, p3, p4, p5, p6, p7);                                       \
    }

#define GLFUNC_9(funcName, t0, p0, t1, p1, t2, p2, t3, p3, t4, p4, t5, p5, t6, p6, t7, p7, t8, p8)           \
    GLPROXY_EXTERN void GLPROXY_DECL funcName(t0 p0, t1 p1, t2 p2, t3 p3, t4 p4, t5 p5, t6 p6, t7 p7, t8 p8) \
    {                                                                                                        \
		GLPROXY_LOG_CALL(STRINGIZE(funcName));																							\
        static GLProxy::TGLFunc<void, t0, t1, t2, t3, t4, t5, t6, t7, t8> TGLFUNC_DECL(funcName);            \
        TGLFUNC_CALL(funcName, p0, p1, p2, p3, p4, p5, p6, p7, p8);                                          \
    }

#define GLFUNC_10(funcName, t0, p0, t1, p1, t2, p2, t3, p3, t4, p4, t5, p5, t6, p6, t7, p7, t8, p8, t9, p9)         \
    GLPROXY_EXTERN void GLPROXY_DECL funcName(t0 p0, t1 p1, t2 p2, t3 p3, t4 p4, t5 p5, t6 p6, t7 p7, t8 p8, t9 p9) \
    {                                                                                                               \
		GLPROXY_LOG_CALL(STRINGIZE(funcName));																							\
        static GLProxy::TGLFunc<void, t0, t1, t2, t3, t4, t5, t6, t7, t8, t9> TGLFUNC_DECL(funcName);               \
        TGLFUNC_CALL(funcName, p0, p1, p2, p3, p4, p5, p6, p7, p8, p9);                                             \
    }


// ================================================================================================
// "Classic OpenGL" and WGL wrappers:
// ================================================================================================

//
// Standard OpenGL data types.
//
// We don't include gl.h to avoid conflicting definitions, so these
// types are defined here in the same way they would be by gl.h.
//
#ifdef GLPROXY_NEED_OGL_TYPES
typedef double         GLclampd;
typedef double         GLdouble;
typedef float          GLclampf;
typedef float          GLfloat;
typedef int            GLint;
typedef int            GLsizei;
typedef short          GLshort;
typedef signed char    GLbyte;
typedef unsigned char  GLboolean;
typedef unsigned char  GLubyte;
typedef unsigned int   GLbitfield;
typedef unsigned int   GLenum;
typedef unsigned int   GLuint;
typedef unsigned short GLushort;
#endif // GLPROXY_NEED_OGL_TYPES

//
// WGL structures.
//
// They are defined here because including WinGDI.h would produce
// warnings or even errors about our redefined WGL function proxies.
//
#ifdef GLPROXY_NEED_WGL_STRUCTS
struct PIXELFORMATDESCRIPTOR
{
    WORD  nSize;
    WORD  nVersion;
    DWORD dwFlags;
    BYTE  iPixelType;
    BYTE  cColorBits;
    BYTE  cRedBits;
    BYTE  cRedShift;
    BYTE  cGreenBits;
    BYTE  cGreenShift;
    BYTE  cBlueBits;
    BYTE  cBlueShift;
    BYTE  cAlphaBits;
    BYTE  cAlphaShift;
    BYTE  cAccumBits;
    BYTE  cAccumRedBits;
    BYTE  cAccumGreenBits;
    BYTE  cAccumBlueBits;
    BYTE  cAccumAlphaBits;
    BYTE  cDepthBits;
    BYTE  cStencilBits;
    BYTE  cAuxBuffers;
    BYTE  iLayerType;
    BYTE  bReserved;
    DWORD dwLayerMask;
    DWORD dwVisibleMask;
    DWORD dwDamageMask;
};
struct LAYERPLANEDESCRIPTOR
{
    WORD  nSize;
    WORD  nVersion;
    DWORD dwFlags;
    BYTE  iPixelType;
    BYTE  cColorBits;
    BYTE  cRedBits;
    BYTE  cRedShift;
    BYTE  cGreenBits;
    BYTE  cGreenShift;
    BYTE  cBlueBits;
    BYTE  cBlueShift;
    BYTE  cAlphaBits;
    BYTE  cAlphaShift;
    BYTE  cAccumBits;
    BYTE  cAccumRedBits;
    BYTE  cAccumGreenBits;
    BYTE  cAccumBlueBits;
    BYTE  cAccumAlphaBits;
    BYTE  cDepthBits;
    BYTE  cStencilBits;
    BYTE  cAuxBuffers;
    BYTE  iLayerPlane;
    BYTE  bReserved;
    COLORREF crTransparent;
};
struct GLYPHMETRICSFLOAT
{
    float gmfBlackBoxX;
    float gmfBlackBoxY;
    struct
    {
        float x;
        float y;
    } gmfptGlyphOrigin;
    float gmfCellIncX;
    float gmfCellIncY;
};
struct WGLSWAP
{
    HDC  hdc;
    UINT flags;
};
#endif // GLPROXY_NEED_WGL_STRUCTS

//
// WGL functions:
//
GLFUNC_0_WRET(HDC,   wglGetCurrentDC);
GLFUNC_0_WRET(HGLRC, wglGetCurrentContext);
GLFUNC_1_WRET(BOOL,  wglDeleteContext, HGLRC, hglrc);
GLFUNC_1_WRET(BOOL,  wglSwapBuffers, HDC, hdc);
GLFUNC_1_WRET(HGLRC, wglCreateContext, HDC, hdc);
GLFUNC_1_WRET(int,   wglGetPixelFormat, HDC, hdc);
GLFUNC_2_WRET(BOOL,  wglMakeCurrent, HDC, hdc, HGLRC, hglrc);
GLFUNC_2_WRET(BOOL,  wglShareLists, HGLRC, hglrc1, HGLRC, hglrc2);
GLFUNC_2_WRET(BOOL,  wglSwapLayerBuffers, HDC, hdc, UINT, flags);
GLFUNC_2_WRET(DWORD, wglSwapMultipleBuffers, UINT, n, const WGLSWAP *, sw);
GLFUNC_2_WRET(HGLRC, wglCreateLayerContext, HDC, hdc, int, b);
GLFUNC_2_WRET(int,   wglChoosePixelFormat, HDC, hdc, const PIXELFORMATDESCRIPTOR *, pfd);
GLFUNC_3_WRET(BOOL,  wglCopyContext, HGLRC, hglrc1, HGLRC, hglrc2, UINT, flags);
GLFUNC_3_WRET(BOOL,  wglRealizeLayerPalette, HDC, hdc, int, b, BOOL, c);
GLFUNC_3_WRET(BOOL,  wglSetPixelFormat, HDC, hdc, int, b, const PIXELFORMATDESCRIPTOR *, pfd);
GLFUNC_4_WRET(BOOL,  wglUseFontBitmapsA, HDC, hdc, DWORD, b, DWORD, c, DWORD, d);
GLFUNC_4_WRET(BOOL,  wglUseFontBitmapsW, HDC, hdc, DWORD, b, DWORD, c, DWORD, d);
GLFUNC_4_WRET(int,   wglDescribePixelFormat, HDC, hdc, int, b, UINT, c, PIXELFORMATDESCRIPTOR *, pfd);
GLFUNC_5_WRET(BOOL,  wglDescribeLayerPlane, HDC, hdc, int, b, int, c, UINT, d, LAYERPLANEDESCRIPTOR *, lpd);
GLFUNC_5_WRET(int,   wglGetLayerPaletteEntries, HDC, hdc, int, b, int, c, int, d, COLORREF *, e);
GLFUNC_5_WRET(int,   wglSetLayerPaletteEntries, HDC, hdc, int, b, int, c, int, d, const COLORREF *, e);
GLFUNC_8_WRET(BOOL,  wglUseFontOutlinesA, HDC, hdc, DWORD, b, DWORD, c, DWORD, d, float, e, float, f, int, g, GLYPHMETRICSFLOAT *, gmf);
GLFUNC_8_WRET(BOOL,  wglUseFontOutlinesW, HDC, hdc, DWORD, b, DWORD, c, DWORD, d, float, e, float, f, int, g, GLYPHMETRICSFLOAT *, gmf);

//
// wglGetProcAddress is a special case. We also want to log
// which extensions got dynamically loaded by the application.
//
GLPROXY_EXTERN PROC GLPROXY_DECL wglGetProcAddress(LPCSTR funcName)
{
    static GLProxy::TGLFunc<PROC, LPCSTR> TGLFUNC_DECL(wglGetProcAddress);
    GLPROXY_LOG("wglGetProcAddress('" << funcName << "')");
    return TGLFUNC_CALL(wglGetProcAddress, funcName);
}

// This is an undocummented function, it seems. So it is probably not called by most applications...
GLPROXY_EXTERN PROC GLPROXY_DECL wglGetDefaultProcAddress(LPCSTR funcName)
{
    static GLProxy::TGLFunc<PROC, LPCSTR> TGLFUNC_DECL(wglGetDefaultProcAddress);
    GLPROXY_LOG("wglGetDefaultProcAddress('" << funcName << "')");
    return TGLFUNC_CALL(wglGetDefaultProcAddress, funcName);
}

//
// GL Functions with a return value:
//
GLFUNC_0_WRET(GLenum, glGetError);
GLFUNC_1_WRET(GLboolean, glIsEnabled, GLenum, cap);
GLFUNC_1_WRET(GLboolean, glIsList, GLuint, list);
GLFUNC_1_WRET(GLboolean, glIsTexture, GLuint, texture);
GLFUNC_1_WRET(GLint, glRenderMode, GLenum, mode);
GLFUNC_1_WRET(GLuint, glGenLists, GLsizei, range);
GLFUNC_1_WRET(const GLubyte *, glGetString, GLenum, name);
GLFUNC_3_WRET(GLboolean, glAreTexturesResident, GLsizei, n, const GLuint *, textures, GLboolean *, residences);

//
// GL Functions returning void:
//
GLFUNC_0(glEnd);
GLFUNC_0(glEndList);
GLFUNC_0(glFinish);
GLFUNC_0(glFlush);
GLFUNC_0(glInitNames);
GLFUNC_0(glPopAttrib);
GLFUNC_0(glPopClientAttrib);
GLFUNC_0(glPopName);
GLFUNC_1(glArrayElement, GLint, i);
GLFUNC_1(glBegin, GLenum, mode);
GLFUNC_1(glCallList, GLuint, list);
GLFUNC_1(glClear, GLbitfield, mask);
GLFUNC_1(glClearDepth, GLclampd, depth);
GLFUNC_1(glClearIndex, GLfloat, c);
GLFUNC_1(glClearStencil, GLint, s);
GLFUNC_1(glColor3bv, const GLbyte *, v);
GLFUNC_1(glColor3dv, const GLdouble *, v);
GLFUNC_1(glColor3fv, const GLfloat *, v);
GLFUNC_1(glColor3iv, const GLint *, v);
GLFUNC_1(glColor3sv, const GLshort *, v);
GLFUNC_1(glColor3ubv, const GLubyte *, v);
GLFUNC_1(glColor3uiv, const GLuint *, v);
GLFUNC_1(glColor3usv, const GLushort *, v);
GLFUNC_1(glColor4bv, const GLbyte *, v);
GLFUNC_1(glColor4dv, const GLdouble *, v);
GLFUNC_1(glColor4fv, const GLfloat *, v);
GLFUNC_1(glColor4iv, const GLint *, v);
GLFUNC_1(glColor4sv, const GLshort *, v);
GLFUNC_1(glColor4ubv, const GLubyte *, v);
GLFUNC_1(glColor4uiv, const GLuint *, v);
GLFUNC_1(glColor4usv, const GLushort *, v);
GLFUNC_1(glCullFace, GLenum, mode);
GLFUNC_1(glDepthFunc, GLenum, func);
GLFUNC_1(glDepthMask, GLboolean, flag);
GLFUNC_1(glDisable, GLenum, cap);
GLFUNC_1(glDisableClientState, GLenum, array);
GLFUNC_1(glDrawBuffer, GLenum, mode);
GLFUNC_1(glEdgeFlag, GLboolean, flag);
GLFUNC_1(glEdgeFlagv, const GLboolean *, flag);
GLFUNC_1(glEnableClientState, GLenum, array);
GLFUNC_1(glEvalCoord1d, GLdouble, u);
GLFUNC_1(glEvalCoord1dv, const GLdouble *, u);
GLFUNC_1(glEvalCoord1f, GLfloat, u);
GLFUNC_1(glEvalCoord1fv, const GLfloat *, u);
GLFUNC_1(glEvalCoord2dv, const GLdouble *, u);
GLFUNC_1(glEvalCoord2fv, const GLfloat *, u);
GLFUNC_1(glEvalPoint1, GLint, i);
GLFUNC_1(glFrontFace, GLenum, mode);
GLFUNC_1(glGetPolygonStipple, GLubyte *, mask);
GLFUNC_1(glIndexMask, GLuint, mask);
GLFUNC_1(glIndexd, GLdouble, c);
GLFUNC_1(glIndexdv, const GLdouble *, c);
GLFUNC_1(glIndexf, GLfloat, c);
GLFUNC_1(glIndexfv, const GLfloat *, c);
GLFUNC_1(glIndexi, GLint, c);
GLFUNC_1(glIndexiv, const GLint *, c);
GLFUNC_1(glIndexs, GLshort, c);
GLFUNC_1(glIndexsv, const GLshort *, c);
GLFUNC_1(glIndexub, GLubyte, c);
GLFUNC_1(glIndexubv, const GLubyte *, c);
GLFUNC_1(glLineWidth, GLfloat, width);
GLFUNC_1(glListBase, GLuint, base);
GLFUNC_1(glLoadName, GLuint, name);
GLFUNC_1(glLogicOp, GLenum, opcode);
GLFUNC_1(glNormal3bv, const GLbyte *, v);
GLFUNC_1(glNormal3dv, const GLdouble *, v);
GLFUNC_1(glNormal3fv, const GLfloat *, v);
GLFUNC_1(glNormal3iv, const GLint *, v);
GLFUNC_1(glNormal3sv, const GLshort *, v);
GLFUNC_1(glPassThrough, GLfloat, token);
GLFUNC_1(glPointSize, GLfloat, size);
GLFUNC_1(glPolygonStipple, const GLubyte *, mask);
GLFUNC_1(glPushAttrib, GLbitfield, mask);
GLFUNC_1(glPushClientAttrib, GLbitfield, mask);
GLFUNC_1(glPushName, GLuint, name);
GLFUNC_1(glRasterPos2dv, const GLdouble *, v);
GLFUNC_1(glRasterPos2fv, const GLfloat *, v);
GLFUNC_1(glRasterPos2iv, const GLint *, v);
GLFUNC_1(glRasterPos2sv, const GLshort *, v);
GLFUNC_1(glRasterPos3dv, const GLdouble *, v);
GLFUNC_1(glRasterPos3fv, const GLfloat *, v);
GLFUNC_1(glRasterPos3iv, const GLint *, v);
GLFUNC_1(glRasterPos3sv, const GLshort *, v);
GLFUNC_1(glRasterPos4dv, const GLdouble *, v);
GLFUNC_1(glRasterPos4fv, const GLfloat *, v);
GLFUNC_1(glRasterPos4iv, const GLint *, v);
GLFUNC_1(glRasterPos4sv, const GLshort *, v);
GLFUNC_1(glReadBuffer, GLenum, mode);
GLFUNC_1(glShadeModel, GLenum, mode);
GLFUNC_1(glStencilMask, GLuint, mask);
GLFUNC_1(glTexCoord1d, GLdouble, s);
GLFUNC_1(glTexCoord1dv, const GLdouble *, v);
GLFUNC_1(glTexCoord1f, GLfloat, s);
GLFUNC_1(glTexCoord1fv, const GLfloat *, v);
GLFUNC_1(glTexCoord1i, GLint, s);
GLFUNC_1(glTexCoord1iv, const GLint *, v);
GLFUNC_1(glTexCoord1s, GLshort, s);
GLFUNC_1(glTexCoord1sv, const GLshort *, v);
GLFUNC_1(glTexCoord2dv, const GLdouble *, v);
GLFUNC_1(glTexCoord2fv, const GLfloat *, v);
GLFUNC_1(glTexCoord2iv, const GLint *, v);
GLFUNC_1(glTexCoord2sv, const GLshort *, v);
GLFUNC_1(glTexCoord3dv, const GLdouble *, v);
GLFUNC_1(glTexCoord3fv, const GLfloat *, v);
GLFUNC_1(glTexCoord3iv, const GLint *, v);
GLFUNC_1(glTexCoord3sv, const GLshort *, v);
GLFUNC_1(glTexCoord4dv, const GLdouble *, v);
GLFUNC_1(glTexCoord4fv, const GLfloat *, v);
GLFUNC_1(glTexCoord4iv, const GLint *, v);
GLFUNC_1(glTexCoord4sv, const GLshort *, v);
GLFUNC_1(glVertex2dv, const GLdouble *, v);
GLFUNC_1(glVertex2fv, const GLfloat *, v);
GLFUNC_1(glVertex2iv, const GLint *, v);
GLFUNC_1(glVertex2sv, const GLshort *, v);
GLFUNC_1(glVertex3dv, const GLdouble *, v);
GLFUNC_1(glVertex3fv, const GLfloat *, v);
GLFUNC_1(glVertex3iv, const GLint *, v);
GLFUNC_1(glVertex3sv, const GLshort *, v);
GLFUNC_1(glVertex4dv, const GLdouble *, v);
GLFUNC_1(glVertex4fv, const GLfloat *, v);
GLFUNC_1(glVertex4iv, const GLint *, v);
GLFUNC_1(glVertex4sv, const GLshort *, v);
GLFUNC_2(glAccum, GLenum, op, GLfloat, value);
GLFUNC_2(glAlphaFunc, GLenum, func, GLclampf, ref);
GLFUNC_2(glBindTexture, GLenum, target, GLuint, texture);
GLFUNC_2(glBlendFunc, GLenum, sfactor, GLenum, dfactor);
GLFUNC_2(glClipPlane, GLenum, plane, const GLdouble *, equation);
GLFUNC_3(glColor3b, GLbyte, red, GLbyte, green, GLbyte, blue);
GLFUNC_2(glColorMaterial, GLenum, face, GLenum, mode);
GLFUNC_2(glDeleteLists, GLuint, list, GLsizei, range);
GLFUNC_2(glDeleteTextures, GLsizei, n, const GLuint *, textures);
GLFUNC_2(glDepthRange, GLclampd, zNear, GLclampd, zFar);
GLFUNC_2(glEdgeFlagPointer, GLsizei, stride, const void *, pointer);
GLFUNC_2(glEvalCoord2d, GLdouble, u, GLdouble, v);
GLFUNC_2(glEvalCoord2f, GLfloat, u, GLfloat, v);
GLFUNC_2(glEvalPoint2, GLint, i, GLint, j);
GLFUNC_2(glFogf, GLenum, pname, GLfloat, param);
GLFUNC_2(glFogfv, GLenum, pname, const GLfloat *, params);
GLFUNC_2(glFogi, GLenum, pname, GLint, param);
GLFUNC_2(glFogiv, GLenum, pname, const GLint *, params);
GLFUNC_2(glGenTextures, GLsizei, n, GLuint *, textures);
GLFUNC_2(glGetBooleanv, GLenum, pname, GLboolean *, params);
GLFUNC_2(glGetClipPlane, GLenum, plane, GLdouble *, equation);
GLFUNC_2(glGetDoublev, GLenum, pname, GLdouble *, params);
GLFUNC_2(glGetFloatv, GLenum, pname, GLfloat *, params);
GLFUNC_2(glGetIntegerv, GLenum, pname, GLint *, params);
GLFUNC_2(glGetPixelMapfv, GLenum, map, GLfloat *, values);
GLFUNC_2(glGetPixelMapuiv, GLenum, map, GLuint *, values);
GLFUNC_2(glGetPixelMapusv, GLenum, map, GLushort *, values);
GLFUNC_2(glGetPointerv, GLenum, pname, void **, params);
GLFUNC_2(glHint, GLenum, target, GLenum, mode);
GLFUNC_2(glLightModelf, GLenum, pname, GLfloat, param);
GLFUNC_2(glLightModelfv, GLenum, pname, const GLfloat *, params);
GLFUNC_2(glLightModeli, GLenum, pname, GLint, param);
GLFUNC_2(glLightModeliv, GLenum, pname, const GLint *, params);
GLFUNC_2(glLineStipple, GLint, factor, GLushort, pattern);
GLFUNC_2(glNewList, GLuint, list, GLenum, mode);
GLFUNC_2(glPixelStoref, GLenum, pname, GLfloat, param);
GLFUNC_2(glPixelStorei, GLenum, pname, GLint, param);
GLFUNC_2(glPixelTransferf, GLenum, pname, GLfloat, param);
GLFUNC_2(glPixelTransferi, GLenum, pname, GLint, param);
GLFUNC_2(glPixelZoom, GLfloat, xfactor, GLfloat, yfactor);
GLFUNC_2(glPolygonMode, GLenum, face, GLenum, mode);
GLFUNC_2(glPolygonOffset, GLfloat, factor, GLfloat, units);
GLFUNC_2(glRasterPos2d, GLdouble, x, GLdouble, y);
GLFUNC_2(glRasterPos2f, GLfloat, x, GLfloat, y);
GLFUNC_2(glRasterPos2i, GLint, x, GLint, y);
GLFUNC_2(glRasterPos2s, GLshort, x, GLshort, y);
GLFUNC_3(glRasterPos3i, GLint, x, GLint, y, GLint, z);
GLFUNC_2(glRectdv, const GLdouble *, v1, const GLdouble *, v2);
GLFUNC_2(glRectfv, const GLfloat *, v1, const GLfloat *, v2);
GLFUNC_2(glRectiv, const GLint *, v1, const GLint *, v2);
GLFUNC_2(glRectsv, const GLshort *, v1, const GLshort *, v2);
GLFUNC_2(glSelectBuffer, GLsizei, size, GLuint *, buffer);
GLFUNC_2(glTexCoord2d, GLdouble, s, GLdouble, t);
GLFUNC_2(glTexCoord2f, GLfloat, s, GLfloat, t);
GLFUNC_2(glTexCoord2i, GLint, s, GLint, t);
GLFUNC_2(glTexCoord2s, GLshort, s, GLshort, t);
GLFUNC_2(glVertex2d, GLdouble, x, GLdouble, y);
GLFUNC_2(glVertex2f, GLfloat, x, GLfloat, y);
GLFUNC_2(glVertex2i, GLint, x, GLint, y);
GLFUNC_2(glVertex2s, GLshort, x, GLshort, y);
GLFUNC_3(glCallLists, GLsizei, n, GLenum, type, const void *, lists);
GLFUNC_3(glColor3d, GLdouble, red, GLdouble, green, GLdouble, blue);
GLFUNC_3(glColor3f, GLfloat, red, GLfloat, green, GLfloat, blue);
GLFUNC_3(glColor3i, GLint, red, GLint, green, GLint, blue);
GLFUNC_3(glColor3s, GLshort, red, GLshort, green, GLshort, blue);
GLFUNC_3(glColor3ub, GLubyte, red, GLubyte, green, GLubyte, blue);
GLFUNC_3(glColor3ui, GLuint, red, GLuint, green, GLuint, blue);
GLFUNC_3(glColor3us, GLushort, red, GLushort, green, GLushort, blue);
GLFUNC_3(glDrawArrays, GLenum, mode, GLint, first, GLsizei, count);
GLFUNC_3(glEvalMesh1, GLenum, mode, GLint, i1, GLint, i2);
GLFUNC_3(glFeedbackBuffer, GLsizei, size, GLenum, type, GLfloat *, buffer);
GLFUNC_3(glGetLightfv, GLenum, light, GLenum, pname, GLfloat *, params);
GLFUNC_3(glGetLightiv, GLenum, light, GLenum, pname, GLint *, params);
GLFUNC_3(glGetMapdv, GLenum, target, GLenum, query, GLdouble *, v);
GLFUNC_3(glGetMapfv, GLenum, target, GLenum, query, GLfloat *, v);
GLFUNC_3(glGetMapiv, GLenum, target, GLenum, query, GLint *, v);
GLFUNC_3(glGetMaterialfv, GLenum, face, GLenum, pname, GLfloat *, params);
GLFUNC_3(glGetMaterialiv, GLenum, face, GLenum, pname, GLint *, params);
GLFUNC_3(glGetTexEnvfv, GLenum, target, GLenum, pname, GLfloat *, params);
GLFUNC_3(glGetTexEnviv, GLenum, target, GLenum, pname, GLint *, params);
GLFUNC_3(glGetTexGendv, GLenum, coord, GLenum, pname, GLdouble *, params);
GLFUNC_3(glGetTexGenfv, GLenum, coord, GLenum, pname, GLfloat *, params);
GLFUNC_3(glGetTexGeniv, GLenum, coord, GLenum, pname, GLint *, params);
GLFUNC_3(glGetTexParameterfv, GLenum, target, GLenum, pname, GLfloat *, params);
GLFUNC_3(glGetTexParameteriv, GLenum, target, GLenum, pname, GLint *, params);
GLFUNC_3(glIndexPointer, GLenum, type, GLsizei, stride, const void *, pointer);
GLFUNC_3(glInterleavedArrays, GLenum, format, GLsizei, stride, const void *, pointer);
GLFUNC_3(glLightf, GLenum, light, GLenum, pname, GLfloat, param);
GLFUNC_3(glLightfv, GLenum, light, GLenum, pname, const GLfloat *, params);
GLFUNC_3(glLighti, GLenum, light, GLenum, pname, GLint, param);
GLFUNC_3(glLightiv, GLenum, light, GLenum, pname, const GLint *, params);
GLFUNC_3(glMapGrid1d, GLint, un, GLdouble, u1, GLdouble, u2);
GLFUNC_3(glMapGrid1f, GLint, un, GLfloat, u1, GLfloat, u2);
GLFUNC_3(glMaterialf, GLenum, face, GLenum, pname, GLfloat, param);
GLFUNC_3(glMaterialfv, GLenum, face, GLenum, pname, const GLfloat *, params);
GLFUNC_3(glMateriali, GLenum, face, GLenum, pname, GLint, param);
GLFUNC_3(glMaterialiv, GLenum, face, GLenum, pname, const GLint *, params);
GLFUNC_3(glNormal3b, GLbyte, nx, GLbyte, ny, GLbyte, nz);
GLFUNC_3(glNormal3d, GLdouble, nx, GLdouble, ny, GLdouble, nz);
GLFUNC_3(glNormal3f, GLfloat, nx, GLfloat, ny, GLfloat, nz);
GLFUNC_3(glNormal3i, GLint, nx, GLint, ny, GLint, nz);
GLFUNC_3(glNormal3s, GLshort, nx, GLshort, ny, GLshort, nz);
GLFUNC_3(glNormalPointer, GLenum, type, GLsizei, stride, const void *, pointer);
GLFUNC_3(glPixelMapfv, GLenum, map, GLsizei, mapsize, const GLfloat *, values);
GLFUNC_3(glPixelMapuiv, GLenum, map, GLsizei, mapsize, const GLuint *, values);
GLFUNC_3(glPixelMapusv, GLenum, map, GLsizei, mapsize, const GLushort *, values);
GLFUNC_3(glPrioritizeTextures, GLsizei, n, const GLuint *, textures, const GLclampf *, priorities);
GLFUNC_3(glRasterPos3d, GLdouble, x, GLdouble, y, GLdouble, z);
GLFUNC_3(glRasterPos3f, GLfloat, x, GLfloat, y, GLfloat, z);
GLFUNC_3(glRasterPos3s, GLshort, x, GLshort, y, GLshort, z);
GLFUNC_4(glRasterPos4d, GLdouble, x, GLdouble, y, GLdouble, z, GLdouble, w);
GLFUNC_3(glStencilFunc, GLenum, func, GLint, ref, GLuint, mask);
GLFUNC_3(glStencilOp, GLenum, fail, GLenum, zfail, GLenum, zpass);
GLFUNC_3(glTexCoord3d, GLdouble, s, GLdouble, t, GLdouble, r);
GLFUNC_3(glTexCoord3f, GLfloat, s, GLfloat, t, GLfloat, r);
GLFUNC_3(glTexCoord3i, GLint, s, GLint, t, GLint, r);
GLFUNC_3(glTexCoord3s, GLshort, s, GLshort, t, GLshort, r);
GLFUNC_3(glTexEnvf, GLenum, target, GLenum, pname, GLfloat, param);
GLFUNC_3(glTexEnvfv, GLenum, target, GLenum, pname, const GLfloat *, params);
GLFUNC_3(glTexEnvi, GLenum, target, GLenum, pname, GLint, param);
GLFUNC_3(glTexEnviv, GLenum, target, GLenum, pname, const GLint *, params);
GLFUNC_3(glTexGend, GLenum, coord, GLenum, pname, GLdouble, param);
GLFUNC_3(glTexGendv, GLenum, coord, GLenum, pname, const GLdouble *, params);
GLFUNC_3(glTexGenf, GLenum, coord, GLenum, pname, GLfloat, param);
GLFUNC_3(glTexGenfv, GLenum, coord, GLenum, pname, const GLfloat *, params);
GLFUNC_3(glTexGeni, GLenum, coord, GLenum, pname, GLint, param);
GLFUNC_3(glTexGeniv, GLenum, coord, GLenum, pname, const GLint *, params);
GLFUNC_3(glTexParameterf, GLenum, target, GLenum, pname, GLfloat, param);
GLFUNC_3(glTexParameterfv, GLenum, target, GLenum, pname, const GLfloat *, params);
GLFUNC_3(glTexParameteri, GLenum, target, GLenum, pname, GLint, param);
GLFUNC_3(glTexParameteriv, GLenum, target, GLenum, pname, const GLint *, params);
GLFUNC_3(glVertex3d, GLdouble, x, GLdouble, y, GLdouble, z);
GLFUNC_3(glVertex3f, GLfloat, x, GLfloat, y, GLfloat, z);
GLFUNC_3(glVertex3i, GLint, x, GLint, y, GLint, z);
GLFUNC_3(glVertex3s, GLshort, x, GLshort, y, GLshort, z);
GLFUNC_4(glClearAccum, GLfloat, red, GLfloat, green, GLfloat, blue, GLfloat, alpha);
GLFUNC_4(glClearColor, GLclampf, red, GLclampf, green, GLclampf, blue, GLclampf, alpha);
GLFUNC_4(glColor4b, GLbyte, red, GLbyte, green, GLbyte, blue, GLbyte, alpha);
GLFUNC_4(glColor4d, GLdouble, red, GLdouble, green, GLdouble, blue, GLdouble, alpha);
GLFUNC_4(glColor4f, GLfloat, red, GLfloat, green, GLfloat, blue, GLfloat, alpha);
GLFUNC_4(glColor4i, GLint, red, GLint, green, GLint, blue, GLint, alpha);
GLFUNC_4(glColor4s, GLshort, red, GLshort, green, GLshort, blue, GLshort, alpha);
GLFUNC_4(glColor4ub, GLubyte, red, GLubyte, green, GLubyte, blue, GLubyte, alpha);
GLFUNC_4(glColor4ui, GLuint, red, GLuint, green, GLuint, blue, GLuint, alpha);
GLFUNC_4(glColor4us, GLushort, red, GLushort, green, GLushort, blue, GLushort, alpha);
GLFUNC_4(glColorMask, GLboolean, red, GLboolean, green, GLboolean, blue, GLboolean, alpha);
GLFUNC_4(glColorPointer, GLint, size, GLenum, type, GLsizei, stride, const void *, pointer);
GLFUNC_4(glDrawElements, GLenum, mode, GLsizei, count, GLenum, type, const void *, indices);
GLFUNC_4(glGetTexLevelParameterfv, GLenum, target, GLint, level, GLenum, pname, GLfloat *, params);
GLFUNC_4(glGetTexLevelParameteriv, GLenum, target, GLint, level, GLenum, pname, GLint *, params);
GLFUNC_4(glRasterPos4f, GLfloat, x, GLfloat, y, GLfloat, z, GLfloat, w);
GLFUNC_4(glRasterPos4i, GLint, x, GLint, y, GLint, z, GLint, w);
GLFUNC_4(glRasterPos4s, GLshort, x, GLshort, y, GLshort, z, GLshort, w);
GLFUNC_4(glRectd, GLdouble, x1, GLdouble, y1, GLdouble, x2, GLdouble, y2);
GLFUNC_4(glRectf, GLfloat, x1, GLfloat, y1, GLfloat, x2, GLfloat, y2);
GLFUNC_4(glRecti, GLint, x1, GLint, y1, GLint, x2, GLint, y2);
GLFUNC_4(glRects, GLshort, x1, GLshort, y1, GLshort, x2, GLshort, y2);
GLFUNC_4(glScissor, GLint, x, GLint, y, GLsizei, width, GLsizei, height);
GLFUNC_4(glTexCoord4d, GLdouble, s, GLdouble, t, GLdouble, r, GLdouble, q);
GLFUNC_4(glTexCoord4f, GLfloat, s, GLfloat, t, GLfloat, r, GLfloat, q);
GLFUNC_4(glTexCoord4i, GLint, s, GLint, t, GLint, r, GLint, q);
GLFUNC_4(glTexCoord4s, GLshort, s, GLshort, t, GLshort, r, GLshort, q);
GLFUNC_4(glTexCoordPointer, GLint, size, GLenum, type, GLsizei, stride, const void *, pointer);
GLFUNC_4(glVertex4d, GLdouble, x, GLdouble, y, GLdouble, z, GLdouble, w);
GLFUNC_4(glVertex4f, GLfloat, x, GLfloat, y, GLfloat, z, GLfloat, w);
GLFUNC_4(glVertex4i, GLint, x, GLint, y, GLint, z, GLint, w);
GLFUNC_4(glVertex4s, GLshort, x, GLshort, y, GLshort, z, GLshort, w);
GLFUNC_4(glVertexPointer, GLint, size, GLenum, type, GLsizei, stride, const void *, pointer);
GLFUNC_5(glCopyPixels, GLint, x, GLint, y, GLsizei, width, GLsizei, height, GLenum, type);
GLFUNC_5(glDrawPixels, GLsizei, width, GLsizei, height, GLenum, format, GLenum, type, const void *, pixels);
GLFUNC_5(glEvalMesh2, GLenum, mode, GLint, i1, GLint, i2, GLint, j1, GLint, j2);
GLFUNC_5(glGetTexImage, GLenum, target, GLint, level, GLenum, format, GLenum, type, void *, pixels);
GLFUNC_6(glCopyTexSubImage1D, GLenum, target, GLint, level, GLint, xoffset, GLint, x, GLint, y, GLsizei, width);
GLFUNC_6(glMap1d, GLenum, target, GLdouble, u1, GLdouble, u2, GLint, stride, GLint, order, const GLdouble *, points);
GLFUNC_6(glMap1f, GLenum, target, GLfloat, u1, GLfloat, u2, GLint, stride, GLint, order, const GLfloat *, points);
GLFUNC_6(glMapGrid2d, GLint, un, GLdouble, u1, GLdouble, u2, GLint, vn, GLdouble, v1, GLdouble, v2);
GLFUNC_6(glMapGrid2f, GLint, un, GLfloat, u1, GLfloat, u2, GLint, vn, GLfloat, v1, GLfloat, v2);
GLFUNC_6(glOrtho, GLdouble, left, GLdouble, right, GLdouble, bottom, GLdouble, top, GLdouble, zNear, GLdouble, zFar);
GLFUNC_7(glBitmap, GLsizei, width, GLsizei, height, GLfloat, xorig, GLfloat, yorig, GLfloat, xmove, GLfloat, ymove, const GLubyte *, bitmap);
GLFUNC_7(glCopyTexImage1D, GLenum, target, GLint, level, GLenum, internalFormat, GLint, x, GLint, y, GLsizei, width, GLint, border);
GLFUNC_7(glReadPixels, GLint, x, GLint, y, GLsizei, width, GLsizei, height, GLenum, format, GLenum, type, void *, pixels);
GLFUNC_7(glTexSubImage1D, GLenum, target, GLint, level, GLint, xoffset, GLsizei, width, GLenum, format, GLenum, type, const void *, pixels);
GLFUNC_8(glCopyTexImage2D, GLenum, target, GLint, level, GLenum, internalFormat, GLint, x, GLint, y, GLsizei, width, GLsizei, height, GLint, border);
GLFUNC_8(glCopyTexSubImage2D, GLenum, target, GLint, level, GLint, xoffset, GLint, yoffset, GLint, x, GLint, y, GLsizei, width, GLsizei, height);
GLFUNC_8(glTexImage1D, GLenum, target, GLint, level, GLint, internalformat, GLsizei, width, GLint, border, GLenum, format, GLenum, type, const void *, pixels);
GLFUNC_9(glTexImage2D, GLenum, target, GLint, level, GLint, internalformat, GLsizei, width, GLsizei, height, GLint, border, GLenum, format, GLenum, type, const void *, pixels);
GLFUNC_9(glTexSubImage2D, GLenum, target, GLint, level, GLint, xoffset, GLint, yoffset, GLsizei, width, GLsizei, height, GLenum, format, GLenum, type, const void *, pixels);
GLFUNC_10(glMap2d, GLenum, target, GLdouble, u1, GLdouble, u2, GLint, ustride, GLint, uorder, GLdouble, v1, GLdouble, v2, GLint, vstride, GLint, vorder, const GLdouble *, points);
GLFUNC_10(glMap2f, GLenum, target, GLfloat, u1, GLfloat, u2, GLint, ustride, GLint, uorder, GLfloat, v1, GLfloat, v2, GLint, vstride, GLint, vorder, const GLfloat *, points);



// ================================================================================================
// Matrix interception - Added for Half-Life: VR by Max Vollmer, 2017-08-15
// These interceptions make it possible to disable calls to matrix manipulation.
// This is needed because the GoldSrc engine doesn't have an option for the client.dll to set GL matrices directly.
// Half-Life: VR client.dll will set the matrices it gets from OpenVR,
// then ask this proxy dll to "lock" OpenGL Matrix functions and then let the HL engine render.
// ================================================================================================

#define GL_CLIP_PLANE0                    0x3000
#define GL_CLIP_PLANE5                    0x3005

bool hlvr_GLMatricesLocked = false;

void logCall(const char* funcName)
{
	if (hlvr_GLMatricesLocked)
	{
		GLPROXY_LOG(funcName << "();");
	}
}

GLPROXY_EXTERN void GLPROXY_DECL hlvrLockGLMatrices(void)
{
	hlvr_GLMatricesLocked = true;
	GLPROXY_LOG("===\nhlvrLockGLMatrices();");
}

GLPROXY_EXTERN void GLPROXY_DECL hlvrUnlockGLMatrices(void)
{
	GLPROXY_LOG("hlvrUnlockGLMatrices();\n===");
	hlvr_GLMatricesLocked = false;
}

GLPROXY_EXTERN void GLPROXY_DECL glEnable(GLenum cap)
{
	if (!(hlvr_GLMatricesLocked && cap >= GL_CLIP_PLANE0 && cap <= GL_CLIP_PLANE5))
	{
		static GLProxy::TGLFunc<void, GLenum> TGLFUNC_DECL(glEnable);
		TGLFUNC_CALL(glEnable, cap);
	}
}

GLPROXY_EXTERN void GLPROXY_DECL glViewport(GLint x, GLint y, GLint width, GLsizei height)
{
	if (!hlvr_GLMatricesLocked)
	{
		static GLProxy::TGLFunc<void, GLint, GLint, GLint, GLsizei> TGLFUNC_DECL(glViewport);
		TGLFUNC_CALL(glViewport, x, y, width, height);
	}
}

GLPROXY_EXTERN void GLPROXY_DECL glPopMatrix(void)
{
	if (!hlvr_GLMatricesLocked)
	{
		static GLProxy::TGLFunc<void> TGLFUNC_DECL(glPopMatrix);
		TGLFUNC_CALL(glPopMatrix);
	}
}

GLPROXY_EXTERN void GLPROXY_DECL glPushMatrix(void)
{
	if (!hlvr_GLMatricesLocked)
	{
		static GLProxy::TGLFunc<void> TGLFUNC_DECL(glPushMatrix);
		TGLFUNC_CALL(glPushMatrix);
	}
}

GLPROXY_EXTERN void GLPROXY_DECL glLoadIdentity(void)
{
	if (!hlvr_GLMatricesLocked)
	{
		static GLProxy::TGLFunc<void> TGLFUNC_DECL(glLoadIdentity);
		TGLFUNC_CALL(glLoadIdentity);
	}
}

GLPROXY_EXTERN void GLPROXY_DECL glMatrixMode(GLenum mode)					
{																	
	if (!hlvr_GLMatricesLocked)										
	{																
		static GLProxy::TGLFunc<void, GLenum> TGLFUNC_DECL(glMatrixMode);
		TGLFUNC_CALL(glMatrixMode, mode);
	}																
}

GLPROXY_EXTERN void GLPROXY_DECL glLoadMatrixd(const GLdouble * m)
{
	if (!hlvr_GLMatricesLocked)
	{
		static GLProxy::TGLFunc<void, const GLdouble *> TGLFUNC_DECL(glLoadMatrixd);
		TGLFUNC_CALL(glLoadMatrixd, m);
	}
}

GLPROXY_EXTERN void GLPROXY_DECL glLoadMatrixf(const GLfloat * m)
{
	if (!hlvr_GLMatricesLocked)
	{
		static GLProxy::TGLFunc<void, const GLfloat *> TGLFUNC_DECL(glLoadMatrixf);
		TGLFUNC_CALL(glLoadMatrixf, m);
	}
}

GLPROXY_EXTERN void GLPROXY_DECL glMultMatrixd(const GLdouble * m)
{
	if (!hlvr_GLMatricesLocked)
	{
		static GLProxy::TGLFunc<void, const GLdouble *> TGLFUNC_DECL(glMultMatrixd);
		TGLFUNC_CALL(glMultMatrixd, m);
	}
}

GLPROXY_EXTERN void GLPROXY_DECL glMultMatrixf(const GLfloat * m)
{
	if (!hlvr_GLMatricesLocked)
	{
		static GLProxy::TGLFunc<void, const GLfloat *> TGLFUNC_DECL(glMultMatrixf);
		TGLFUNC_CALL(glMultMatrixf, m);
	}
}

GLPROXY_EXTERN void GLPROXY_DECL glScaled(GLdouble x, GLdouble y, GLdouble z)
{
	if (!hlvr_GLMatricesLocked)
	{
		static GLProxy::TGLFunc<void, GLdouble, GLdouble, GLdouble> TGLFUNC_DECL(glScaled);
		TGLFUNC_CALL(glScaled, x, y, z);
	}
}

GLPROXY_EXTERN void GLPROXY_DECL glScalef(GLfloat x, GLfloat y, GLfloat z)
{
	if (!hlvr_GLMatricesLocked)
	{
		static GLProxy::TGLFunc<void, GLfloat, GLfloat, GLfloat> TGLFUNC_DECL(glScalef);
		TGLFUNC_CALL(glScalef, x, y, z);
	}
}

GLPROXY_EXTERN void GLPROXY_DECL glTranslated(GLdouble x, GLdouble y, GLdouble z)
{
	if (!hlvr_GLMatricesLocked)
	{
		static GLProxy::TGLFunc<void, GLdouble, GLdouble, GLdouble> TGLFUNC_DECL(glTranslated);
		TGLFUNC_CALL(glTranslated, x, y, z);
	}
}

GLPROXY_EXTERN void GLPROXY_DECL glTranslatef(GLfloat x, GLfloat y, GLfloat z)
{
	if (!hlvr_GLMatricesLocked)
	{
		static GLProxy::TGLFunc<void, GLfloat, GLfloat, GLfloat> TGLFUNC_DECL(glTranslatef);
		TGLFUNC_CALL(glTranslatef, x, y, z);
	}
}

GLPROXY_EXTERN void GLPROXY_DECL glRotated(GLdouble angle, GLdouble x, GLdouble y, GLdouble z)
{
	if (!hlvr_GLMatricesLocked)
	{
		static GLProxy::TGLFunc<void, GLdouble, GLdouble, GLdouble, GLdouble> TGLFUNC_DECL(glRotated);
		TGLFUNC_CALL(glRotated, angle, x, y, z);
	}
}

GLPROXY_EXTERN void GLPROXY_DECL glRotatef(GLfloat angle, GLfloat x, GLfloat y, GLfloat z)
{
	if (!hlvr_GLMatricesLocked)
	{
		static GLProxy::TGLFunc<void, GLfloat, GLfloat, GLfloat, GLfloat> TGLFUNC_DECL(glRotatef);
		TGLFUNC_CALL(glRotatef, angle, x, y, z);
	}
}

GLPROXY_EXTERN void GLPROXY_DECL glFrustum(GLdouble left, GLdouble right, GLdouble bottom, GLdouble top, GLdouble zNear, GLdouble zFar)
{
	if (!hlvr_GLMatricesLocked)
	{
		static GLProxy::TGLFunc<void, GLdouble, GLdouble, GLdouble, GLdouble, GLdouble, GLdouble> TGLFUNC_DECL(glFrustum);
		TGLFUNC_CALL(glFrustum, left, right, bottom, top, zNear, zFar);
	}
}
