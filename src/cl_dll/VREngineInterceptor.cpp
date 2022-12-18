
// Random implementation using std random classes for overriding engine's random functions
// For Half-Life: VR by Max Vollmer - 2020-02-22

#include "EasyHook/include/easyhook.h"

#include "VREngineInterceptor.h"
#include "VRRandom.h"

#include <vector>
#include <memory>
#include <string>
#include <sstream>

#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <io.h>
#include <stdio.h>

#include "hud.h"

#define NTDDI_VERSION           NTDDI_WIN2KSP4
#define _WIN32_WINNT            0x500
#define _WIN32_IE_              _WIN32_IE_WIN2KSP4

#include <windows.h>
#include <winnt.h>
#include <winternl.h>

#include "VRFileManager.h"

namespace
{
    std::vector<std::shared_ptr<HOOK_TRACE_INFO>> hooks;

    bool HookEngineMethod(const char* name, void* originalmethod, void* hookmethod)
    {
        std::shared_ptr<HOOK_TRACE_INFO> hook = std::make_shared<HOOK_TRACE_INFO>(HOOK_TRACE_INFO{ nullptr });
        hooks.push_back(hook);

        NTSTATUS result = LhInstallHook(
            originalmethod,
            hookmethod,
            nullptr,
            hook.get());

        if (FAILED(result))
        {
            gEngfuncs.Con_DPrintf("Warning: Failed to install hook for engine's random method %s: %S\n", name, RtlGetLastErrorString());
            return false;
        }
        else
        {
            ULONG ACLEntries[1] = { 0 };
            LhSetInclusiveACL(ACLEntries, 1, hook.get());
            return true;
        }
    }
}

bool VREngineInterceptor::HookIOFunctions()
{
#if 0
    HINSTANCE hDll = GetModuleHandleW(L"ntdll.dll");
    if (!hDll)
    {
        return false;
    }

    pfnNtCreateFile = (NTSTATUS(*)(PHANDLE, ACCESS_MASK, POBJECT_ATTRIBUTES, PIO_STATUS_BLOCK, PLARGE_INTEGER, ULONG, ULONG, ULONG, ULONG, PVOID, ULONG))GetProcAddress(hDll, "NtCreateFile");
    pfnNtDeleteFile = (NTSTATUS(*)(POBJECT_ATTRIBUTES))GetProcAddress(hDll, "NtDeleteFile");
    pfnNtLockFile = GetProcAddress(hDll, "NtLockFile");
    pfnNtUnlockFile = GetProcAddress(hDll, "NtDeleteFile");
    pfnNtQueryInformationFile = GetProcAddress(hDll, "NtQueryInformationFile");
    pfnNtSetInformationFile = GetProcAddress(hDll, "NtSetInformationFile");
    pfnNtReadFile = GetProcAddress(hDll, "NtReadFile");
    pfnNtWriteFile = GetProcAddress(hDll, "NtWriteFile");
    pfnNtMapViewOfSection = GetProcAddress(hDll, "NtMapViewOfSection");
    pfnNtUnmapViewOfSection = GetProcAddress(hDll, "NtUnmapViewOfSection");
    pfnNtQueryDirectoryFile = GetProcAddress(hDll, "NtQueryDirectoryFile");

    if (!pfnNtCreateFile
        || !pfnNtDeleteFile
        || !pfnNtLockFile
        || !pfnNtUnlockFile
        || !pfnNtQueryInformationFile
        || !pfnNtSetInformationFile
        || !pfnNtReadFile
        || !pfnNtWriteFile
        || !pfnNtMapViewOfSection
        || !pfnNtUnmapViewOfSection
        || !pfnNtQueryDirectoryFile)
    {
        return false;
    }

    return HookEngineMethod("COM_LoadFile", gEngfuncs.COM_LoadFile, VRLoadFile)
        && HookEngineMethod("COM_FreeFile", gEngfuncs.COM_FreeFile, VRFreeFile)

        && HookEngineMethod("fopen", fopen, VRFOpen)
        && HookEngineMethod("fopens", fopen_s, VRFOpenS)
        && HookEngineMethod("fread", fread, VRFRead)
        && HookEngineMethod("fclose", fclose, VRFClose)

        && HookEngineMethod("_open", _open, VROpen)
        && HookEngineMethod("_wopen", _wopen, VRWOpen)
        && HookEngineMethod("_write", _write, VRWrite)
        && HookEngineMethod("_close", _close, VRClose)

        // Zw?
        && HookEngineMethod("NtCreateFile", pfnNtCreateFile, VRNtCreateFile)
        //&& HookEngineMethod("NtDeleteFile", pfnNtDeleteFile, VRNtDeleteFile)
        /*
        && HookEngineMethod("NtLockFile", pfnNtLockFile, VRNtLockFile)
        && HookEngineMethod("NtUnlockFile", pfnNtUnlockFile, VRNtUnlockFile)
        && HookEngineMethod("NtQueryInformationFile", pfnNtQueryInformationFile, VRNtQueryInformationFile)
        && HookEngineMethod("NtSetInformationFile", pfnNtSetInformationFile, VRNtSetInformationFile)
        && HookEngineMethod("NtReadFile", pfnNtReadFile, VRNtReadFile)
        && HookEngineMethod("NtWriteFile", pfnNtWriteFile, VRNtWriteFile)
        && HookEngineMethod("NtMapViewOfSection", pfnNtMapViewOfSection, VRNtMapViewOfSection)
        && HookEngineMethod("NtUnmapViewOfSection", pfnNtUnmapViewOfSection, VRNtUnmapViewOfSection)
        && HookEngineMethod("NtQueryDirectoryFile", pfnNtQueryDirectoryFile, VRNtQueryDirectoryFile)
        */

        // && HookEngineMethod("CreateFileA", CreateFileA, VRCreateFileA)
        // && HookEngineMethod("CreateFileW", CreateFileW, VRCreateFileW)
        // && HookEngineMethod("ReadFile", ReadFile, VRReadFile)
        // && HookEngineMethod("WriteFile", WriteFile, VRWriteFile)
        // && HookEngineMethod("CloseHandle", CloseHandle, VRCloseHandle)
        ;
#else
    return false;
#endif
}

bool VREngineInterceptor::HookEngineFunctions()
{
    return HookEngineMethod("RandomFloat", gEngfuncs.pfnRandomFloat, VRRandomFloat)
        && HookEngineMethod("RandomLong", gEngfuncs.pfnRandomLong, VRRandomLong)
        ;
}

