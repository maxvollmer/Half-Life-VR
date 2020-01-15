/*
This is a heavily stripped down version of metahook by nagist: https://github.com/nagist/metahook

Reduced for Half-Life: VR by Max Vollmer to minimal functionality needed for hooking into sound functions by the engine.

All credit goes to nagist for making metahook!
*/

#ifndef _METAHOOK_H
#define _METAHOOK_H

#include <Windows.h>

extern DWORD g_dwEngineBase;
extern DWORD g_dwEngineSize;
extern DWORD g_dwEngineBuildnum;

void* MH_SearchPattern(void* pStartSearch, DWORD dwSearchLen, char* pPattern, DWORD dwPatternLen);

void MH_Init();

#endif
