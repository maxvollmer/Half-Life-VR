/*
This is a heavily stripped down version of metahook by nagist: https://github.com/nagist/metahook

Reduced for Half-Life: VR by Max Vollmer to minimal functionality needed for hooking into sound functions by the engine.

All credit goes to nagist for making metahook!
*/

#include <Windows.h>
#include <fstream>
#include <string>
#include <algorithm>

#include "metahook.h"
#include "interface.h"

#define BUILD_NUMBER_SIG "\xA1\x2A\x2A\x2A\x2A\x83\xEC\x08\x2A\x33\x2A\x85\xC0"
#define BUILD_NUMBER_SIG_NEW "\x55\x8B\xEC\x83\xEC\x08\xA1\x2A\x2A\x2A\x2A\x56\x33\xF6\x85\xC0\x0F\x85\x2A\x2A\x2A\x2A\x53\x33\xDB\x8B\x04\x9D"

HMODULE g_hEngineModule = NULL;
DWORD g_dwEngineBase = 0;
DWORD g_dwEngineSize = 0;
bool g_bIsNewEngine = false;
int(*g_pfnbuild_number)(void) = nullptr;
DWORD g_dwEngineBuildnum = 0;

void *MH_GetClassFuncAddr(...)
{
  DWORD address;

  __asm
  {
    lea eax, address
    mov edx, [ebp + 8]
    mov[eax], edx
  }

  return (void *)address;
}

DWORD MH_GetModuleBase(HMODULE hModule)
{
  MEMORY_BASIC_INFORMATION mem;

  if (!VirtualQuery(hModule, &mem, sizeof(MEMORY_BASIC_INFORMATION)))
    return 0;

  return (DWORD)mem.AllocationBase;
}

DWORD MH_GetModuleSize(HMODULE hModule)
{
  return ((IMAGE_NT_HEADERS *)((DWORD)hModule + ((IMAGE_DOS_HEADER *)hModule)->e_lfanew))->OptionalHeader.SizeOfImage;
}

HMODULE MH_GetEngineModule(void)
{
  return g_hEngineModule;
}

DWORD MH_GetEngineBase(void)
{
  return g_dwEngineBase;
}

DWORD MH_GetEngineSize(void)
{
  return g_dwEngineSize;
}

void *MH_SearchPattern(void *pStartSearch, DWORD dwSearchLen, char *pPattern, DWORD dwPatternLen)
{
  DWORD dwStartAddr = (DWORD)pStartSearch;
  DWORD dwEndAddr = dwStartAddr + dwSearchLen - dwPatternLen;

  while (dwStartAddr < dwEndAddr)
  {
    bool found = true;

    for (DWORD i = 0; i < dwPatternLen; i++)
    {
      char code = *(char *)(dwStartAddr + i);

      if (pPattern[i] != 0x2A && pPattern[i] != code)
      {
        found = false;
        break;
      }
    }

    if (found)
      return (void *)dwStartAddr;

    dwStartAddr++;
  }

  return 0;
}

CreateInterfaceFn MH_GetEngineFactory(void)
{
    return (CreateInterfaceFn)GetProcAddress(g_hEngineModule, "CreateInterface");
}

DWORD MH_GetEngineVersion(void)
{
  if (!g_pfnbuild_number)
    return 0;

  return g_pfnbuild_number();
}

void MH_Init()
{
    g_hEngineModule = (HMODULE)Sys_LoadModule("hw.dll");
    if (!g_hEngineModule)
    {
        // TODO: Error
        return;
    }

    g_dwEngineBase = MH_GetModuleBase(g_hEngineModule);
    g_dwEngineSize = MH_GetModuleSize(g_hEngineModule);

    g_bIsNewEngine = false;
    g_pfnbuild_number = (int(*)(void))MH_SearchPattern((void*)g_dwEngineBase, g_dwEngineSize, BUILD_NUMBER_SIG, sizeof(BUILD_NUMBER_SIG) - 1);

    if (!g_pfnbuild_number)
    {
        g_pfnbuild_number = (int(*)(void))MH_SearchPattern((void*)g_dwEngineBase, g_dwEngineSize, BUILD_NUMBER_SIG_NEW, sizeof(BUILD_NUMBER_SIG_NEW) - 1);
        g_bIsNewEngine = true;
    }

    g_dwEngineBuildnum = MH_GetEngineVersion();
}
