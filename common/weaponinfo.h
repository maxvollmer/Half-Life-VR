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
#if !defined(WEAPONINFOH)
	#define WEAPONINFOH
	#ifdef _WIN32
		#pragma once
	#endif

// Info about weapons player might have in their possession
typedef struct weapon_data_s
{
	int m_iId = 0;
	int m_iClip = 0;

	float m_flNextPrimaryAttack = 0.f;
	float m_flNextSecondaryAttack = 0.f;
	float m_flTimeWeaponIdle = 0.f;

	int m_fInReload = 0;
	int m_fInSpecialReload = 0;
	float m_flNextReload = 0.f;
	float m_flPumpTime = 0.f;
	float m_fReloadTime = 0.f;

	float m_fAimedDamage = 0.f;
	float m_fNextAimBonus = 0.f;
	int m_fInZoom = 0;
	int m_iWeaponState = 0;

	int iuser1 = 0;
	int iuser2 = 0;
	int iuser3 = 0;
	int iuser4 = 0;
	float fuser1 = 0.f;
	float fuser2 = 0.f;
	float fuser3 = 0.f;
	float fuser4 = 0.f;
} weapon_data_t;

#endif
