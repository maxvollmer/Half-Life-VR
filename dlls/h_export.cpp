/***
*
*	Copyright (c) 1996-2002, Valve LLC. All rights reserved.
*
*	This product contains software technology licensed from Id
*	Software, Inc. ("Id Technology").  Id Technology (c) 1996 Id Software, Inc.
*	All Rights Reserved.
*
*   Use, distribution, and modification of this source code and/or resulting
*   object code is restricted to non-commercial enhancements to products from
*   Valve LLC.  All other use, distribution, or modification is prohibited
*   without written permission from Valve LLC.
*
****/
/*

===== h_export.cpp ========================================================

  Entity classes exported by Halflife.

*/

#include "extdll.h"
#include "util.h"

#include "cbase.h"

// Holds engine functionality callbacks
enginefuncs_t g_engfuncs;
globalvars_t* gpGlobals;


extern void DispatchOnFreeEntPrivateData(edict_t* pEnt);
extern void GameDLLUninit(void);
extern int DispatchShouldCollide(edict_t* pentTouched, edict_t* pentOther);

static NEW_DLL_FUNCTIONS gNewFunctionTable =
{
	DispatchOnFreeEntPrivateData,  //OnFreeEntPrivateData, //pfnOnFreeEntPrivateData
	GameDLLUninit,                 //pfnGameShutdown
	DispatchShouldCollide,         //ShouldCollide, //pfnShouldCollide
};


#ifdef _WIN32

// Required DLL entry point
BOOL WINAPI DllMain(
	HINSTANCE hinstDLL,
	DWORD fdwReason,
	LPVOID lpvReserved)
{
	if (fdwReason == DLL_PROCESS_ATTACH)
	{
	}
	else if (fdwReason == DLL_PROCESS_DETACH)
	{
	}
	return TRUE;
}

void DLLEXPORT GiveFnptrsToDll(enginefuncs_t* pengfuncsFromEngine, globalvars_t* pGlobals)
{
	memcpy(&g_engfuncs, pengfuncsFromEngine, sizeof(enginefuncs_t));
	gpGlobals = pGlobals;
}

int DLLEXPORT GetNewDLLFunctions(NEW_DLL_FUNCTIONS* pFunctionTable, int* interfaceVersion)
{
	if (!pFunctionTable || *interfaceVersion != NEW_DLL_FUNCTIONS_VERSION)
	{
		// Tell engine what version we had, so it can figure out who is out of date.
		*interfaceVersion = NEW_DLL_FUNCTIONS_VERSION;
		return false;
	}
	memcpy(pFunctionTable, &gNewFunctionTable, sizeof(NEW_DLL_FUNCTIONS));
	return true;
}

#else

extern "C"
{
	void GiveFnptrsToDll(enginefuncs_t* pengfuncsFromEngine, globalvars_t* pGlobals)
	{
		memcpy(&g_engfuncs, pengfuncsFromEngine, sizeof(enginefuncs_t));
		gpGlobals = pGlobals;
	}

	int DLLEXPORT GetNewDLLFunctions(NEW_DLL_FUNCTIONS* pFunctionTable, int* interfaceVersion)
	{
		if (!pFunctionTable || *interfaceVersion != NEW_DLL_FUNCTIONS_VERSION)
		{
			// Tell engine what version we had, so it can figure out who is out of date.
			*interfaceVersion = NEW_DLL_FUNCTIONS_VERSION;
			return false;
		}
		memcpy(pFunctionTable, &gNewFunctionTable, sizeof(NEW_DLL_FUNCTIONS));
		return true;
	}
}

#endif
