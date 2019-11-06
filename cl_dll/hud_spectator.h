//========= Copyright � 1996-2002, Valve LLC, All rights reserved. ============
//
// Purpose:
//
// $NoKeywords: $
//=============================================================================

#ifndef SPECTATOR_H
#define SPECTATOR_H
#pragma once

#include "cl_entity.h"



#define INSET_OFF        0
#define INSET_CHASE_FREE 1
#define INSET_IN_EYE     2
#define INSET_MAP_FREE   3
#define INSET_MAP_CHASE  4

#define MAX_SPEC_HUD_MESSAGES 8



#define OVERVIEW_TILE_SIZE  128  // don't change this
#define OVERVIEW_MAX_LAYERS 1

//-----------------------------------------------------------------------------
// Purpose: Handles the drawing of the spectator stuff (camera & top-down map and all the things on it )
//-----------------------------------------------------------------------------

typedef struct overviewInfo_s
{
	char map[64] = { 0 };   // cl.levelname or empty
	vec3_t origin;  // center of map
	float zoom = 0.f;     // zoom of map images
	int layers = 0;     // how may layers do we have
	float layersHeights[OVERVIEW_MAX_LAYERS] = { 0 };
	char layersImages[OVERVIEW_MAX_LAYERS][255] = { 0 };
	qboolean rotated = 0;  // are map images rotated (90 degrees) ?

	int insetWindowX = 0;
	int insetWindowY = 0;
	int insetWindowHeight = 0;
	int insetWindowWidth = 0;
} overviewInfo_t;

typedef struct overviewEntity_s
{
	HSPRITE_VALVE hSprite = 0;
	struct cl_entity_s* entity = nullptr;
	double killTime = 0.0;
} overviewEntity_t;

#define MAX_OVERVIEW_ENTITIES 128

class CHudSpectator : public CHudBase
{
public:
	void Reset();
	int ToggleInset(bool allowOff);
	void CheckSettings();
	void InitHUDData(void);
	bool AddOverviewEntityToList(HSPRITE_VALVE sprite, cl_entity_t* ent, double killTime);
	void DeathMessage(int victim);
	bool AddOverviewEntity(int type, struct cl_entity_s* ent, const char* modelname);
	void CheckOverviewEntities();
	void DrawOverview();
	void DrawOverviewEntities();
	void GetMapPosition(float* returnvec);
	void DrawOverviewLayer();
	void LoadMapSprites();
	bool ParseOverviewFile();
	bool IsActivePlayer(cl_entity_t* ent);
	void SetModes(int iMainMode, int iInsetMode);
	void HandleButtonsDown(int ButtonPressed);
	void HandleButtonsUp(int ButtonPressed);
	void FindNextPlayer(bool bReverse);
	void DirectorMessage(int iSize, void* pbuf);
	void SetSpectatorStartPosition();
	int Init();
	int VidInit();

	int Draw(float flTime);

	int m_iDrawCycle = 0;
	client_textmessage_t m_HUDMessages[MAX_SPEC_HUD_MESSAGES] = { 0 };
	char m_HUDMessageText[MAX_SPEC_HUD_MESSAGES][128] = { 0 };
	int m_lastHudMessage = 0;
	overviewInfo_t m_OverviewData;
	overviewEntity_t m_OverviewEntities[MAX_OVERVIEW_ENTITIES] = { 0 };
	int m_iObserverFlags = 0;
	int m_iSpectatorNumber = 0;

	float m_mapZoom = 0.f;     // zoom the user currently uses
	vec3_t m_mapOrigin;  // origin where user rotates around
	cvar_t* m_drawnames = nullptr;
	cvar_t* m_drawcone = nullptr;
	cvar_t* m_drawstatus = nullptr;
	cvar_t* m_autoDirector = nullptr;
	cvar_t* m_pip = nullptr;


	qboolean m_chatEnabled = 0;

	vec3_t m_cameraOrigin;  // a help camera
	vec3_t m_cameraAngles;  // and it's angles


private:
	vec3_t m_vPlayerPos[MAX_PLAYERS] = { 0 };
	HSPRITE_VALVE m_hsprPlayerBlue = 0;
	HSPRITE_VALVE m_hsprPlayerRed = 0;
	HSPRITE_VALVE m_hsprPlayer = 0;
	HSPRITE_VALVE m_hsprCamera = 0;
	HSPRITE_VALVE m_hsprPlayerDead = 0;
	HSPRITE_VALVE m_hsprViewcone = 0;
	HSPRITE_VALVE m_hsprUnkownMap = 0;
	HSPRITE_VALVE m_hsprBeam = 0;
	HSPRITE_VALVE m_hCrosshair = 0;

	wrect_t m_crosshairRect;

	struct model_s* m_MapSprite = nullptr;  // each layer image is saved in one sprite, where each tile is a sprite frame
	float m_flNextObserverInput = 0.f;
	float m_zoomDelta = 0.f;
	float m_moveDelta = 0.f;
	int m_lastPrimaryObject = 0;
	int m_lastSecondaryObject = 0;
};

#endif  // SPECTATOR_H
