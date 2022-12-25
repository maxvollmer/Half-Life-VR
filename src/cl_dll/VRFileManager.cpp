
#include "hud.h"

#include <iostream>
#include <fstream>

#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <io.h>
#include <stdio.h>

#define NTDDI_VERSION           NTDDI_WIN2KSP4
#define _WIN32_WINNT            0x500
#define _WIN32_IE_              _WIN32_IE_WIN2KSP4

#include <windows.h>
#include <winnt.h>
#include <winternl.h>

#include "VRFileManager.h"



NTSTATUS(*pfnNtCreateFile)(
    PHANDLE,
    ACCESS_MASK,
    POBJECT_ATTRIBUTES,
    PIO_STATUS_BLOCK,
    PLARGE_INTEGER,
    ULONG,
    ULONG,
    ULONG,
    ULONG,
    PVOID,
    ULONG
    ) = nullptr;
NTSTATUS(*pfnNtDeleteFile)(POBJECT_ATTRIBUTES) = nullptr;
void* pfnNtLockFile = nullptr;
void* pfnNtUnlockFile = nullptr;
void* pfnNtQueryInformationFile = nullptr;
void* pfnNtSetInformationFile = nullptr;
void* pfnNtReadFile = nullptr;
void* pfnNtWriteFile = nullptr;
void* pfnNtMapViewOfSection = nullptr;
void* pfnNtUnmapViewOfSection = nullptr;
void* pfnNtQueryDirectoryFile = nullptr;


unsigned char* VRLoadFile(const char* path, int usehunk, int* pLength)
{
    gEngfuncs.Con_DPrintf("VRLoadFile: %s, usehunk: %i\n", path, usehunk);

    std::ofstream myfile;
    myfile.open("example.bin", std::ios::out | std::ios::app | std::ios::binary);

    unsigned char* result = gEngfuncs.COM_LoadFile(path, usehunk, pLength);

    if (pLength)
    {
        gEngfuncs.Con_DPrintf("result: %i, length: %i\n", result, *pLength);
    }
    else
    {
        gEngfuncs.Con_DPrintf("result: %i, length: NULL\n", result);
    }

    return result;
}

void VRFreeFile(const void* buffer)
{
    gEngfuncs.Con_DPrintf("VRFreeFile: %i\n", buffer);

    gEngfuncs.COM_FreeFile(buffer);
}


int VROpen(char const* const _FileName, int const _OFlag, int const _PMode)
{
    gEngfuncs.Con_DPrintf("VROpen: %s, %i, %i\n", _FileName, _OFlag, _PMode);

    return _open(_FileName, _OFlag, _PMode);
}

int VRWOpen(wchar_t const* const _FileName, int const _OFlag, int const _PMode)
{
    gEngfuncs.Con_DPrintf("VRWOpen: ???, %i, %i\n", _OFlag, _PMode);

    return _wopen(_FileName, _OFlag, _PMode);
}

int VRWrite(int _FileHandle, void const* _Buf, unsigned int _MaxCharCount)
{
    gEngfuncs.Con_DPrintf("VRWrite: %i, %i, %i\n", _FileHandle, _Buf, _MaxCharCount);

    return _write(_FileHandle, _Buf, _MaxCharCount);
}

int VRClose(int _FileHandle)
{
    gEngfuncs.Con_DPrintf("VRClose: %i\n", _FileHandle);

    return _close(_FileHandle);
}


void* VRCreateFileA(
    const char* lpFileName,
    unsigned long dwDesiredAccess,
    unsigned long dwShareMode,
    void* lpSecurityAttributes,
    unsigned long dwCreationDisposition,
    unsigned long dwFlagsAndAttributes,
    void* hTemplateFile
)
{
    std::string bla = lpFileName;
    if (bla.find(".bsp") != std::string::npos)
    {
        gEngfuncs.Con_DPrintf("VRCreateFileA: %s\n", lpFileName);
    }

    return CreateFileA(lpFileName, dwDesiredAccess, dwShareMode, (LPSECURITY_ATTRIBUTES)lpSecurityAttributes, dwCreationDisposition, dwFlagsAndAttributes, hTemplateFile);
}

void* VRCreateFileW(
    const wchar_t* lpFileName,
    unsigned long dwDesiredAccess,
    unsigned long dwShareMode,
    void* lpSecurityAttributes,
    unsigned long dwCreationDisposition,
    unsigned long dwFlagsAndAttributes,
    void* hTemplateFile
)
{
    gEngfuncs.Con_DPrintf("VRCreateFileW\n");

    return CreateFileW(lpFileName, dwDesiredAccess, dwShareMode, (LPSECURITY_ATTRIBUTES)lpSecurityAttributes, dwCreationDisposition, dwFlagsAndAttributes, hTemplateFile);
}


int VRReadFile(
    void* hFile,
    void* lpBuffer,
    unsigned long nNumberOfBytesToRead,
    unsigned long* lpNumberOfBytesRead,
    void* lpOverlapped
)
{
    gEngfuncs.Con_DPrintf("VRReadFile: %i, %i\n", hFile, nNumberOfBytesToRead);

    return ReadFile(hFile, lpBuffer, nNumberOfBytesToRead, lpNumberOfBytesRead, (LPOVERLAPPED)lpOverlapped);
}

int VRWriteFile(
    void* hFile,
    void* lpBuffer,
    unsigned long nNumberOfBytesToWrite,
    unsigned long* lpNumberOfBytesWritten,
    void* lpOverlapped
)
{
    gEngfuncs.Con_DPrintf("VRWriteFile: %i, %i\n", hFile, nNumberOfBytesToWrite);

    return WriteFile(hFile, lpBuffer, nNumberOfBytesToWrite, lpNumberOfBytesWritten, (LPOVERLAPPED)lpOverlapped);
}

int VRCloseHandle(void* hObject)
{
    gEngfuncs.Con_DPrintf("VRCloseHandle: %i\n", hObject);

    return CloseHandle(hObject);
}


int VRFOpenS(void** _Stream, char const* _FileName, char const* _Mode)
{
    gEngfuncs.Con_DPrintf("VRFOpenS: %s, mode: %s\n", _FileName, _Mode);

    int result = fopen_s((FILE**)_Stream, _FileName, _Mode);


    if (result)
    {
        gEngfuncs.Con_DPrintf("result: %i (NULL)\n", result);
    }
    else
    {
        gEngfuncs.Con_DPrintf("result: %i (%i)\n", result, *_Stream);
    }

    return result;
}

void* VRFOpen(char const* _FileName, char const* _Mode)
{
    gEngfuncs.Con_DPrintf("VRFOpen: %s, mode: %s\n", _FileName, _Mode);

    void* result = fopen(_FileName, _Mode);

    gEngfuncs.Con_DPrintf("result: %i\n", result);

    return result;
}

unsigned int VRFRead(void* _Buffer, unsigned int _ElementSize, unsigned int _ElementCount, void* _Stream)
{
    gEngfuncs.Con_DPrintf("VRFRead: %i, %i (%i, %i)\n", _Stream, _ElementSize, _ElementCount, _Buffer);

    int result = fread(_Buffer, _ElementSize, _ElementCount, (FILE*)_Stream);

    gEngfuncs.Con_DPrintf("result: %i\n", result);

    return result;
}

int VRFClose(void* _Stream)
{
    gEngfuncs.Con_DPrintf("VRFClose: %i\n", _Stream);

    int result = fclose((FILE*)_Stream);

    gEngfuncs.Con_DPrintf("result: %i\n", result);

    return result;
}



NTSTATUS VRNtCreateFile(
    PHANDLE            FileHandle,
    ACCESS_MASK        DesiredAccess,
    POBJECT_ATTRIBUTES ObjectAttributes,
    PIO_STATUS_BLOCK   IoStatusBlock,
    PLARGE_INTEGER     AllocationSize,
    ULONG              FileAttributes,
    ULONG              ShareAccess,
    ULONG              CreateDisposition,
    ULONG              CreateOptions,
    PVOID              EaBuffer,
    ULONG              EaLength
)
{
    std::string name(ObjectAttributes->ObjectName->Buffer, ObjectAttributes->ObjectName->Buffer + ObjectAttributes->ObjectName->Length);

    gEngfuncs.Con_DPrintf("VRNtCreateFile: %s\n", name.c_str());

    return pfnNtCreateFile(FileHandle,
        DesiredAccess,
        ObjectAttributes,
        IoStatusBlock,
        AllocationSize,
        FileAttributes,
        ShareAccess,
        CreateDisposition,
        CreateOptions,
        EaBuffer,
        EaLength);
}

NTSTATUS VRNtDeleteFile(
    POBJECT_ATTRIBUTES ObjectAttributes
)
{
    std::string name(ObjectAttributes->ObjectName->Buffer, ObjectAttributes->ObjectName->Buffer + ObjectAttributes->ObjectName->Length);

    gEngfuncs.Con_DPrintf("VRNtDeleteFile: %s\n", name.c_str());

    return pfnNtDeleteFile(ObjectAttributes);
}

void VRNtDeleteFile() {}
void VRNtLockFile() {}
void VRNtUnlockFile() {}
void VRNtQueryInformationFile() {}
void VRNtSetInformationFile() {}
void VRNtReadFile() {}
void VRNtWriteFile() {}
void VRNtMapViewOfSection() {}
void VRNtUnmapViewOfSection() {}
void VRNtQueryDirectoryFile() {}
