#pragma once

extern NTSTATUS(*pfnNtCreateFile)(
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
    );
extern NTSTATUS(*pfnNtDeleteFile)(POBJECT_ATTRIBUTES);
extern void* pfnNtLockFile;
extern void* pfnNtUnlockFile;
extern void* pfnNtQueryInformationFile;
extern void* pfnNtSetInformationFile;
extern void* pfnNtReadFile;
extern void* pfnNtWriteFile;
extern void* pfnNtMapViewOfSection;
extern void* pfnNtUnmapViewOfSection;
extern void* pfnNtQueryDirectoryFile;



byte* VRLoadFile(const char* path, int usehunk, int* pLength);

void VRFreeFile(const void* buffer);

void* VRFOpen(char const* _FileName, char const* _Mode);

int VRFOpenS(void** _Stream, char const* _FileName, char const* _Mode);

unsigned int VRFRead(void* _Buffer, unsigned int _ElementSize, unsigned int _ElementCount, void* _Stream);

int VRFClose(void* _Stream);



int VROpen(char const* const _FileName, int const _OFlag, int const _PMode);
int VRWOpen(wchar_t const* const _FileName, int const _OFlag, int const _PMode);
int VRWrite(int _FileHandle, void const* _Buf, unsigned int _MaxCharCount);
int VRClose(int _FileHandle);


void* VRCreateFileA(
    const char* lpFileName,
    unsigned long dwDesiredAccess,
    unsigned long dwShareMode,
    void* lpSecurityAttributes,
    unsigned long dwCreationDisposition,
    unsigned long dwFlagsAndAttributes,
    void* hTemplateFile
);
void* VRCreateFileW(
    const wchar_t* lpFileName,
    unsigned long dwDesiredAccess,
    unsigned long dwShareMode,
    void* lpSecurityAttributes,
    unsigned long dwCreationDisposition,
    unsigned long dwFlagsAndAttributes,
    void* hTemplateFile
);
int VRReadFile(
    void* hFile,
    void* lpBuffer,
    unsigned long nNumberOfBytesToRead,
    unsigned long* lpNumberOfBytesRead,
    void* lpOverlapped
);
int VRWriteFile(
    void* hFile,
    void* lpBuffer,
    unsigned long nNumberOfBytesToWrite,
    unsigned long* lpNumberOfBytesWritten,
    void* lpOverlapped
);
int VRCloseHandle(void* hObject);



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
);

NTSTATUS VRNtDeleteFile(
    POBJECT_ATTRIBUTES ObjectAttributes
);
