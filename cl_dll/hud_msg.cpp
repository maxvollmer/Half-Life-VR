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
//  hud_msg.cpp
//

#include "hud.h"
#include "cl_util.h"
#include "parsemsg.h"
#include "r_efx.h"

#define MAX_CLIENTS 32

extern BEAM* pBeam;
extern BEAM* pBeam2;

/// USER-DEFINED SERVER MESSAGE HANDLERS

int CHud::MsgFunc_ResetHUD(const char* pszName, int iSize, void* pbuf)
{
	ASSERT(iSize == 0);

	// clear all hud data
	HUDLIST* pList = m_pHudList;

	while (pList)
	{
		if (pList->p)
			pList->p->Reset();
		pList = pList->pNext;
	}

	// reset concussion effect
	m_iConcussionEffect = 0;

	return 1;
}

void CHud::MsgFunc_ViewMode(const char* pszName, int iSize, void* pbuf)
{
}

void CHud::MsgFunc_InitHUD(const char* pszName, int iSize, void* pbuf)
{
	// prepare all hud data
	HUDLIST* pList = m_pHudList;

	while (pList)
	{
		if (pList->p)
			pList->p->InitHUDData();
		pList = pList->pNext;
	}

	//Probably not a good place to put this.
	pBeam = pBeam2 = nullptr;
}


int CHud::MsgFunc_GameMode(const char* pszName, int iSize, void* pbuf)
{
	BEGIN_READ(pbuf, iSize);
	m_Teamplay = READ_BYTE();

	return 1;
}


int CHud::MsgFunc_Damage(const char* pszName, int iSize, void* pbuf)
{
	int armor, blood;
	Vector from;
	int i = 0;
	float count = 0.f;

	BEGIN_READ(pbuf, iSize);
	armor = READ_BYTE();
	blood = READ_BYTE();

	for (i = 0; i < 3; i++)
		from[i] = READ_COORD();

	count = (blood * 0.5) + (armor * 0.5);

	if (count < 10)
		count = 10;

	// TODO: kick viewangles,  show damage visually

	return 1;
}

int CHud::MsgFunc_Concuss(const char* pszName, int iSize, void* pbuf)
{
	BEGIN_READ(pbuf, iSize);
	m_iConcussionEffect = READ_BYTE();
	if (m_iConcussionEffect)
		this->m_StatusIcons.EnableIcon("dmg_concuss", 255, 160, 0);
	else
		this->m_StatusIcons.DisableIcon("dmg_concuss");
	return 1;
}

int CHud::MsgFunc_GroundEnt(const char* pszName, int iSize, void* pbuf)
{
	BEGIN_READ(pbuf, iSize);
	m_iGroundEntIndex = READ_SHORT();
	return 1;
}

int CHud::MsgFunc_VRCtrlEnt(const char* pszName, int iSize, void* pbuf)
{
	BEGIN_READ(pbuf, iSize);

	bool isLeftHand = READ_BYTE() != 0;
	auto& data = isLeftHand ? m_leftControllerModelData : m_rightControllerModelData;
	data.body = READ_BYTE();
	data.skin = READ_BYTE();

	data.sequence = READ_LONG();

	data.frame = READ_FLOAT();
	data.framerate = READ_FLOAT();
	data.animtime = READ_FLOAT();

	strncpy_s(data.modelname, READ_STRING(), 1024);

	return 1;
}

int CHud::MsgFunc_TrainCtrl(const char* pszName, int iSize, void* pbuf)
{
	BEGIN_READ(pbuf, iSize);
	m_trainControlPosition.x = READ_COORD();
	m_trainControlPosition.y = READ_COORD();
	m_trainControlPosition.z = READ_COORD();
	m_trainControlYaw = READ_ANGLE();
	return 1;
}

int CHud::MsgFunc_VRScrnShke(const char* pszName, int iSize, void* pbuf)
{
	BEGIN_READ(pbuf, iSize);
	m_screenShakeAmplitude = READ_FLOAT();
	m_screenShakeDuration = READ_FLOAT();
	m_screenShakeFrequency = READ_FLOAT();
	m_hasScreenShake = true;
	return 1;
}

int CHud::MsgFunc_GrbdLddr(const char* pszName, int iSize, void* pbuf)
{
	BEGIN_READ(pbuf, iSize);
	m_vrGrabbedLadderEntIndex = READ_SHORT();
	return 1;
}

int CHud::MsgFunc_PullLdg(const char* pszName, int iSize, void* pbuf)
{
	BEGIN_READ(pbuf, iSize);
	m_vrIsPullingOnLedge = READ_BYTE() != 0;
	return 1;
}

int CHud::MsgFunc_VRTouch(const char* pszName, int iSize, void* pbuf)
{
	BEGIN_READ(pbuf, iSize);
	bool isLeftHand = READ_BYTE() != 0;
	if (isLeftHand)
	{
		m_vrLeftHandTouchVibrateIntensity = READ_FLOAT();
	}
	else
	{
		m_vrRightHandTouchVibrateIntensity = READ_FLOAT();
	}
	return 1;
}
