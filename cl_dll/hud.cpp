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
//
// hud.cpp
//
// implementation of CHud class
//

#include <cstring>

#include "hud.h"
#include "cl_util.h"
#include <string.h>
#include <stdio.h>
#include "parsemsg.h"
#include "hud_servers.h"
#include "vgui_int.h"
#include "vgui_TeamFortressViewport.h"

#include "demo.h"
#include "demo_api.h"
#include "vgui_scorepanel.h"

#include "VRRenderer.h"


class CHLVoiceStatusHelper : public IVoiceStatusHelper
{
public:
	virtual void GetPlayerTextColor(int entindex, int color[3])
	{
		color[0] = color[1] = color[2] = 255;

		if (entindex >= 0 && entindex < sizeof(g_PlayerExtraInfo) / sizeof(g_PlayerExtraInfo[0]))
		{
			int iTeam = g_PlayerExtraInfo[entindex].teamnumber;

			if (iTeam < 0)
			{
				iTeam = 0;
			}

			iTeam = iTeam % iNumberOfTeamColors;

			color[0] = iTeamColors[iTeam][0];
			color[1] = iTeamColors[iTeam][1];
			color[2] = iTeamColors[iTeam][2];
		}
	}

	virtual void UpdateCursorState()
	{
		gViewPort->UpdateCursorState();
	}

	virtual int GetAckIconHeight()
	{
		return ScreenHeight - gHUD.m_iFontHeight * 3 - 6;
	}

	virtual bool CanShowSpeakerLabels()
	{
		if (gViewPort && gViewPort->m_pScoreBoard)
			return !gViewPort->m_pScoreBoard->isVisible();
		else
			return false;
	}
};
static CHLVoiceStatusHelper g_VoiceStatusHelper;


extern client_sprite_t* GetSpriteList(client_sprite_t* pList, const char* psz, int iRes, int iCount);

extern cvar_t* sensitivity;
cvar_t* cl_lw = nullptr;

void ShutdownInput(void);

//DECLARE_MESSAGE(m_Logo, Logo)
int __MsgFunc_Logo(const char* pszName, int iSize, void* pbuf)
{
	return gHUD.MsgFunc_Logo(pszName, iSize, pbuf);
}

//DECLARE_MESSAGE(m_Logo, Logo)
int __MsgFunc_ResetHUD(const char* pszName, int iSize, void* pbuf)
{
	return gHUD.MsgFunc_ResetHUD(pszName, iSize, pbuf);
}

int __MsgFunc_InitHUD(const char* pszName, int iSize, void* pbuf)
{
	gHUD.MsgFunc_InitHUD(pszName, iSize, pbuf);
	return 1;
}

int __MsgFunc_ViewMode(const char* pszName, int iSize, void* pbuf)
{
	gHUD.MsgFunc_ViewMode(pszName, iSize, pbuf);
	return 1;
}

int __MsgFunc_SetFOV(const char* pszName, int iSize, void* pbuf)
{
	return gHUD.MsgFunc_SetFOV(pszName, iSize, pbuf);
}

int __MsgFunc_Concuss(const char* pszName, int iSize, void* pbuf)
{
	return gHUD.MsgFunc_Concuss(pszName, iSize, pbuf);
}

int __MsgFunc_GameMode(const char* pszName, int iSize, void* pbuf)
{
	return gHUD.MsgFunc_GameMode(pszName, iSize, pbuf);
}

// TFFree Command Menu
void __CmdFunc_OpenCommandMenu(void)
{
	if (gViewPort)
	{
		gViewPort->ShowCommandMenu(gViewPort->m_StandardMenu);
	}
}

// TFC "special" command
void __CmdFunc_InputPlayerSpecial(void)
{
	if (gViewPort)
	{
		gViewPort->InputPlayerSpecial();
	}
}

void __CmdFunc_CloseCommandMenu(void)
{
	if (gViewPort)
	{
		gViewPort->InputSignalHideCommandMenu();
	}
}

void __CmdFunc_ForceCloseCommandMenu(void)
{
	if (gViewPort)
	{
		gViewPort->HideCommandMenu();
	}
}

void __CmdFunc_ToggleServerBrowser(void)
{
	if (gViewPort)
	{
		gViewPort->ToggleServerBrowser();
	}
}

// TFFree Command Menu Message Handlers
int __MsgFunc_ValClass(const char* pszName, int iSize, void* pbuf)
{
	if (gViewPort)
		return gViewPort->MsgFunc_ValClass(pszName, iSize, pbuf);
	return 0;
}

int __MsgFunc_TeamNames(const char* pszName, int iSize, void* pbuf)
{
	if (gViewPort)
		return gViewPort->MsgFunc_TeamNames(pszName, iSize, pbuf);
	return 0;
}

int __MsgFunc_Feign(const char* pszName, int iSize, void* pbuf)
{
	if (gViewPort)
		return gViewPort->MsgFunc_Feign(pszName, iSize, pbuf);
	return 0;
}

int __MsgFunc_Detpack(const char* pszName, int iSize, void* pbuf)
{
	if (gViewPort)
		return gViewPort->MsgFunc_Detpack(pszName, iSize, pbuf);
	return 0;
}

int __MsgFunc_VGUIMenu(const char* pszName, int iSize, void* pbuf)
{
	if (gViewPort)
		return gViewPort->MsgFunc_VGUIMenu(pszName, iSize, pbuf);
	return 0;
}

int __MsgFunc_VRRstrYaw(const char* pszName, int iSize, void* pbuf)
{
	BEGIN_READ(pbuf, iSize);
	// a bit hacky, but oh well
	extern float g_vrRestoreYaw_PrevYaw;
	extern float g_vrRestoreYaw_CurrentYaw;
	extern bool g_vrRestoreYaw_HasData;
	g_vrRestoreYaw_PrevYaw = READ_ANGLE();
	g_vrRestoreYaw_CurrentYaw = READ_ANGLE();
	g_vrRestoreYaw_HasData = true;
	return 0;
}

int __MsgFunc_VRSpawnYaw(const char* pszName, int iSize, void* pbuf)
{
	BEGIN_READ(pbuf, iSize);
	// a bit hacky, but oh well
	extern float g_vrSpawnYaw;
	extern bool g_vrSpawnYaw_HasData;
	g_vrSpawnYaw = READ_ANGLE();
	g_vrSpawnYaw_HasData = true;
	return 0;
}

// Sends index of current grund entity
int __MsgFunc_GroundEnt(const char* pszName, int iSize, void* pbuf)
{
	return gHUD.MsgFunc_GroundEnt(pszName, iSize, pbuf);
}

int __MsgFunc_VRCtrlEnt(const char* pszName, int iSize, void* pbuf)
{
	return gHUD.MsgFunc_VRCtrlEnt(pszName, iSize, pbuf);
}

int __MsgFunc_TrainCtrl(const char* pszName, int iSize, void* pbuf)
{
	return gHUD.MsgFunc_TrainCtrl(pszName, iSize, pbuf);
}

int __MsgFunc_VRScrnShke(const char* pszName, int iSize, void* pbuf)
{
	return gHUD.MsgFunc_VRScrnShke(pszName, iSize, pbuf);
}

int __MsgFunc_GrbdLddr(const char* pszName, int iSize, void* pbuf)
{
	return gHUD.MsgFunc_GrbdLddr(pszName, iSize, pbuf);
}

int __MsgFunc_PullLdg(const char* pszName, int iSize, void* pbuf)
{
	return gHUD.MsgFunc_PullLdg(pszName, iSize, pbuf);
}

int __MsgFunc_VRUpdEgon(const char* pszName, int iSize, void* pbuf)
{
	BEGIN_READ(pbuf, iSize);
	Vector beamStartPos{ READ_FLOAT(), READ_FLOAT(), READ_FLOAT() };
	Vector beamEndPos{ READ_FLOAT(), READ_FLOAT(), READ_FLOAT() };

	extern void EV_UpdateEgon(const Vector & beamStartPos, const Vector & beamEndPos);
	EV_UpdateEgon(beamStartPos, beamEndPos);

	return 0;
}

int __MsgFunc_VRTouch(const char* pszName, int iSize, void* pbuf)
{
	return gHUD.MsgFunc_VRTouch(pszName, iSize, pbuf);
}

int __MsgFunc_MOTD(const char* pszName, int iSize, void* pbuf)
{
	if (gViewPort)
		return gViewPort->MsgFunc_MOTD(pszName, iSize, pbuf);
	return 0;
}

int __MsgFunc_BuildSt(const char* pszName, int iSize, void* pbuf)
{
	if (gViewPort)
		return gViewPort->MsgFunc_BuildSt(pszName, iSize, pbuf);
	return 0;
}

int __MsgFunc_RandomPC(const char* pszName, int iSize, void* pbuf)
{
	if (gViewPort)
		return gViewPort->MsgFunc_RandomPC(pszName, iSize, pbuf);
	return 0;
}

int __MsgFunc_ServerName(const char* pszName, int iSize, void* pbuf)
{
	if (gViewPort)
		return gViewPort->MsgFunc_ServerName(pszName, iSize, pbuf);
	return 0;
}

int __MsgFunc_ScoreInfo(const char* pszName, int iSize, void* pbuf)
{
	if (gViewPort)
		return gViewPort->MsgFunc_ScoreInfo(pszName, iSize, pbuf);
	return 0;
}

int __MsgFunc_TeamScore(const char* pszName, int iSize, void* pbuf)
{
	if (gViewPort)
		return gViewPort->MsgFunc_TeamScore(pszName, iSize, pbuf);
	return 0;
}

int __MsgFunc_TeamInfo(const char* pszName, int iSize, void* pbuf)
{
	if (gViewPort)
		return gViewPort->MsgFunc_TeamInfo(pszName, iSize, pbuf);
	return 0;
}

int __MsgFunc_Spectator(const char* pszName, int iSize, void* pbuf)
{
	if (gViewPort)
		return gViewPort->MsgFunc_Spectator(pszName, iSize, pbuf);
	return 0;
}

int __MsgFunc_AllowSpec(const char* pszName, int iSize, void* pbuf)
{
	if (gViewPort)
		return gViewPort->MsgFunc_AllowSpec(pszName, iSize, pbuf);
	return 0;
}

// This is called every time the DLL is loaded
void CHud::Init(void)
{
	HOOK_MESSAGE(Logo);
	HOOK_MESSAGE(ResetHUD);
	HOOK_MESSAGE(GameMode);
	HOOK_MESSAGE(InitHUD);
	HOOK_MESSAGE(ViewMode);
	HOOK_MESSAGE(SetFOV);
	HOOK_MESSAGE(Concuss);

	// TFFree CommandMenu
	HOOK_COMMAND("+commandmenu", OpenCommandMenu);
	HOOK_COMMAND("-commandmenu", CloseCommandMenu);
	HOOK_COMMAND("ForceCloseCommandMenu", ForceCloseCommandMenu);
	HOOK_COMMAND("special", InputPlayerSpecial);
	HOOK_COMMAND("togglebrowser", ToggleServerBrowser);

	HOOK_MESSAGE(ValClass);
	HOOK_MESSAGE(TeamNames);
	HOOK_MESSAGE(Feign);
	HOOK_MESSAGE(Detpack);
	HOOK_MESSAGE(MOTD);
	HOOK_MESSAGE(BuildSt);
	HOOK_MESSAGE(RandomPC);
	HOOK_MESSAGE(ServerName);
	HOOK_MESSAGE(ScoreInfo);
	HOOK_MESSAGE(TeamScore);
	HOOK_MESSAGE(TeamInfo);

	HOOK_MESSAGE(Spectator);
	HOOK_MESSAGE(AllowSpec);

	// VGUI Menus
	HOOK_MESSAGE(VGUIMenu);

	// Messages for VR
	HOOK_MESSAGE(VRRstrYaw);
	HOOK_MESSAGE(GroundEnt);
	HOOK_MESSAGE(VRCtrlEnt);
	HOOK_MESSAGE(VRSpawnYaw);
	HOOK_MESSAGE(TrainCtrl);
	HOOK_MESSAGE(VRScrnShke);
	HOOK_MESSAGE(GrbdLddr);
	HOOK_MESSAGE(PullLdg);
	HOOK_MESSAGE(VRUpdEgon);
	HOOK_MESSAGE(VRTouch);

	CVAR_CREATE("hud_classautokill", "1", FCVAR_ARCHIVE | FCVAR_USERINFO);  // controls whether or not to suicide immediately on TF class switch
	CVAR_CREATE("hud_takesshots", "0", FCVAR_ARCHIVE);                      // controls whether or not to automatically take screenshots at the end of a round


	m_iLogo = 0;
	m_iFOV = 0;

	CVAR_CREATE("zoom_sensitivity_ratio", "1.2", 0);
	default_fov = CVAR_CREATE("default_fov", "180", 0);
	m_pCvarStealMouse = CVAR_CREATE("hud_capturemouse", "0", FCVAR_ARCHIVE);
	m_pCvarDraw = CVAR_CREATE("hud_draw", "1", FCVAR_ARCHIVE);
	cl_lw = gEngfuncs.pfnGetCvarPointer("cl_lw");

	m_pSpriteList = nullptr;

	// Clear any old HUD list
	if (m_pHudList)
	{
		while (m_pHudList)
		{
			HUDLIST* pList = m_pHudList;
			m_pHudList = m_pHudList->pNext;
			delete pList;
		}
		m_pHudList = nullptr;
	}

	// In case we get messages before the first update -- time will be valid
	m_flHUDDrawTime = 1.0;

	m_Ammo.Init();
	m_Health.Init();
	m_SayText.Init();
	m_Spectator.Init();
	m_Geiger.Init();
	m_Train.Init();
	m_Battery.Init();
	m_Flash.Init();
	m_Message.Init();
	m_StatusBar.Init();
	m_DeathNotice.Init();
	m_AmmoSecondary.Init();
	m_TextMessage.Init();
	m_StatusIcons.Init();
	GetClientVoiceMgr()->Init(&g_VoiceStatusHelper, reinterpret_cast<vgui::Panel**>(&gViewPort));

	m_Menu.Init();

	ServersInit();

	MsgFunc_ResetHUD(0, 0, nullptr);
}

// CHud destructor
// cleans up memory allocated for m_rg* arrays
CHud ::~CHud()
{
	delete[] m_rghSprites;
	delete[] m_rgrcRects;
	delete[] m_rgszSpriteNames;

	if (m_pHudList)
	{
		while (m_pHudList)
		{
			HUDLIST* pList = m_pHudList;
			m_pHudList = m_pHudList->pNext;
			delete pList;
		}
		m_pHudList = nullptr;
	}

	ServersShutdown();
}

// GetSpriteIndex()
// searches through the sprite list loaded from hud.txt for a name matching SpriteName
// returns an index into the gHUD.m_rghSprites[] array
// returns 0 if sprite not found
int CHud::GetSpriteIndex(const char* SpriteName)
{
	// look through the loaded sprite name list for SpriteName
	for (int i = 0; i < m_iSpriteCount; i++)
	{
		if (strncmp(SpriteName, m_rgszSpriteNames + (i * MAX_SPRITE_NAME_LENGTH), MAX_SPRITE_NAME_LENGTH) == 0)
			return i;
	}

	return -1;  // invalid sprite
}

void CHud::VidInit(void)
{
	m_scrinfo.iSize = sizeof(m_scrinfo);
	GetScreenInfo(&m_scrinfo);

	// ----------
	// Load Sprites
	// ---------
	//	m_hsprFont = LoadSprite("sprites/%d_font.spr");

	m_hsprLogo = 0;
	m_hsprCursor = 0;

	if (ScreenWidth < 640)
		m_iRes = 320;
	else
		m_iRes = 640;

	// Only load this once
	if (!m_pSpriteList)
	{
		// we need to load the hud.txt, and all sprites within
		m_pSpriteList = SPR_GetList("sprites/hud.txt", &m_iSpriteCountAllRes);

		if (m_pSpriteList)
		{
			// count the number of sprites of the appropriate res
			m_iSpriteCount = 0;
			client_sprite_t* p = m_pSpriteList;
			for (int j = 0; j < m_iSpriteCountAllRes; j++)
			{
				if (p->iRes == m_iRes)
					m_iSpriteCount++;
				p++;
			}

			// allocated memory for sprite handle arrays
			m_rghSprites = new HSPRITE_VALVE[m_iSpriteCount];
			m_rgrcRects = new wrect_t[m_iSpriteCount];
			m_rgszSpriteNames = new char[m_iSpriteCount * MAX_SPRITE_NAME_LENGTH];

			p = m_pSpriteList;
			int index = 0;
			for (int j = 0; j < m_iSpriteCountAllRes; j++)
			{
				if (p->iRes == m_iRes)
				{
					char sz[256];
					sprintf_s(sz, "sprites/%s.spr", p->szSprite);
					m_rghSprites[index] = SPR_Load(sz);
					m_rgrcRects[index] = p->rc;
					strncpy_s(&m_rgszSpriteNames[index * MAX_SPRITE_NAME_LENGTH], MAX_SPRITE_NAME_LENGTH, p->szName, MAX_SPRITE_NAME_LENGTH);

					index++;
				}

				p++;
			}
		}
	}
	else
	{
		// we have already have loaded the sprite reference from hud.txt, but
		// we need to make sure all the sprites have been loaded (we've gone through a transition, or loaded a save game)
		client_sprite_t* p = m_pSpriteList;
		int index = 0;
		for (int j = 0; j < m_iSpriteCountAllRes; j++)
		{
			if (p->iRes == m_iRes)
			{
				char sz[256];
				sprintf_s(sz, "sprites/%s.spr", p->szSprite);
				m_rghSprites[index] = SPR_Load(sz);
				index++;
			}

			p++;
		}
	}

	// assumption: number_1, number_2, etc, are all listed and loaded sequentially
	m_HUD_number_0 = GetSpriteIndex("number_0");

	m_iFontHeight = m_rgrcRects[m_HUD_number_0].bottom - m_rgrcRects[m_HUD_number_0].top;

	m_Ammo.VidInit();
	m_Health.VidInit();
	m_Spectator.VidInit();
	m_Geiger.VidInit();
	m_Train.VidInit();
	m_Battery.VidInit();
	m_Flash.VidInit();
	m_Message.VidInit();
	m_StatusBar.VidInit();
	m_DeathNotice.VidInit();
	m_SayText.VidInit();
	m_Menu.VidInit();
	m_AmmoSecondary.VidInit();
	m_TextMessage.VidInit();
	m_StatusIcons.VidInit();
	GetClientVoiceMgr()->VidInit();

	gVRRenderer.VidInit();
}

int CHud::MsgFunc_Logo(const char* pszName, int iSize, void* pbuf)
{
	BEGIN_READ(pbuf, iSize);

	// update Train data
	m_iLogo = READ_BYTE();

	return 1;
}

float g_lastFOV = 0.0;

/*
============
COM_FileBase
============
*/
// Extracts the base name of a file (no path, no extension, assumes '/' as path separator)
void COM_FileBase(const char* in, char* out, int outsize)
{
	int len, start, end;

	len = strlen(in);

	// scan backward for '.'
	end = len - 1;
	while (end && in[end] != '.' && in[end] != '/' && in[end] != '\\')
		end--;

	if (in[end] != '.')  // no '.', copy to end
		end = len - 1;
	else
		end--;  // Found ',', copy to left of '.'


	// Scan backward for '/'
	start = len - 1;
	while (start >= 0 && in[start] != '/' && in[start] != '\\')
		start--;

	if (in[start] != '/' && in[start] != '\\')
		start = 0;
	else
		start++;

	// Length of new sting
	len = end - start + 1;

	// Copy partial string
	strncpy_s(out, outsize , &in[start], len);
	// Terminate it
	out[len] = 0;
}

/*
=================
HUD_IsGame

=================
*/
int HUD_IsGame(const char* game)
{
	const char* gamedir;
	char gd[1024];

	gamedir = gEngfuncs.pfnGetGameDirectory();
	if (gamedir && gamedir[0])
	{
		COM_FileBase(gamedir, gd, 1024);
		if (!_stricmp(gd, game))
			return 1;
	}
	return 0;
}

/*
=====================
HUD_GetFOV

Returns last FOV
=====================
*/
float HUD_GetFOV(void)
{
	if (gEngfuncs.pDemoAPI->IsRecording())
	{
		// Write it
		unsigned char buf[sizeof(float)];
		std::memcpy(buf, &g_lastFOV, sizeof(float));
		Demo_WriteBuffer(TYPE_ZOOM, sizeof(float), buf);
	}

	if (gEngfuncs.pDemoAPI->IsPlayingback())
	{
		g_lastFOV = g_demozoom;
	}
	return g_lastFOV;
}

int CHud::MsgFunc_SetFOV(const char* pszName, int iSize, void* pbuf)
{
	BEGIN_READ(pbuf, iSize);

	int newfov = READ_BYTE();
	int def_fov = CVAR_GET_FLOAT("default_fov");

	//Weapon prediction already takes care of changing the fog. ( g_lastFOV ).
	if (cl_lw && cl_lw->value)
		return 1;

	g_lastFOV = newfov;

	if (newfov == 0)
	{
		m_iFOV = def_fov;
	}
	else
	{
		m_iFOV = newfov;
	}

	// the clients fov is actually set in the client data update section of the hud

	return 1;
}


void CHud::AddHudElem(CHudBase* phudelem)
{
	if (!phudelem)
		return;

	HUDLIST* pdl = new HUDLIST{ 0 };
	if (!pdl)
		return;

	pdl->p = phudelem;

	if (!m_pHudList)
	{
		m_pHudList = pdl;
		return;
	}

	HUDLIST* ptemp = m_pHudList;
	while (ptemp->pNext)
		ptemp = ptemp->pNext;
	ptemp->pNext = pdl;
}


// Ground entity for rotating with them in VR (since player rotation is done client side completely) - Max Vollmer, 2019-04-09
cl_entity_t* CHud::GetGroundEntity()
{
	if (m_iGroundEntIndex > 0)
	{
		return gEngfuncs.GetEntityByIndex(m_iGroundEntIndex);
	}
	else
	{
		return nullptr;
	}
}

bool CHud::GetTrainControlsOriginAndOrientation(Vector& origin, Vector& angles)
{
	if (m_Train.m_iPos)
	{
		origin = m_trainControlPosition;
		angles.x = -20.f;
		angles.y = m_trainControlYaw;
		angles.z = 0.f;
		return true;
	}
	return false;
}


// Used by hl_weapons.cpp and view.cpp when switching to hand model
bool PlayerHasSuit()
{
	return gHUD.m_iWeaponBits & (1 << (WEAPON_SUIT));
}
