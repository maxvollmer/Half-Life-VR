//========= Copyright © 1996-2002, Valve LLC, All rights reserved. ============
//
// Purpose:
//
//=============================================================================

#ifndef IGAMECONSOLE_H
#define IGAMECONSOLE_H
#ifdef _WIN32
#pragma once
#endif

#include "interface.h"

class Color;
typedef unsigned int VPANEL;

//-----------------------------------------------------------------------------
// Purpose: interface to game/dev console
//-----------------------------------------------------------------------------
class IGameConsole : public IBaseInterface
{
public:
	// activates the console, makes it visible and brings it to the foreground
	virtual void Activate() = 0;

	virtual void Initialize() = 0;

	// hides the console
	virtual void Hide() = 0;

	// clears the console
	virtual void Clear() = 0;

	// return true if the console has focus
	virtual bool IsConsoleVisible() = 0;

	// prints a message to the console
	virtual void Printf(const char* format, ...) = 0;

	// printes a debug message to the console
	virtual void DPrintf(const char* format, ...) = 0;

	// printes a debug message to the console
	virtual void ColorPrintf(Color& clr, const char* format, ...) = 0;

	virtual void SetParent(VPANEL parent) = 0;
};

#define GAMECONSOLE_INTERFACE_VERSION "GameConsole003"

#endif  // IGAMECONSOLE_H