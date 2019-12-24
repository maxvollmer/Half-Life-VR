//========= Copyright � 1996-2002, Valve LLC, All rights reserved. ============
//
// Purpose:
//
// $NoKeywords: $
//=============================================================================

// Triangle rendering, if any

#include "hud.h"
#include "cl_util.h"

// Triangle rendering apis are in gEngfuncs.pTriAPI

#include "const.h"
#include "entity_state.h"
#include "cl_entity.h"
#include "triangleapi.h"

#include "VRRenderer.h"

#define DLLEXPORT __declspec(dllexport)

extern "C"
{
	void DLLEXPORT HUD_DrawNormalTriangles(void);
	void DLLEXPORT HUD_DrawTransparentTriangles(void);
};

/*
=================
HUD_DrawNormalTriangles

Non-transparent triangles-- add them here
=================
*/
void DLLEXPORT HUD_DrawNormalTriangles(void)
{
	//gHUD.m_Spectator.DrawOverview();
	gVRRenderer.DrawNormal();
}

/*
=================
HUD_DrawTransparentTriangles

Render any triangles with transparent rendermode needs here
=================
*/
void DLLEXPORT HUD_DrawTransparentTriangles(void)
{
	gVRRenderer.DrawTransparent();
}
