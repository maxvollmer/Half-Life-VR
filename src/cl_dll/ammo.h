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

#ifndef __AMMO_H__
#define __AMMO_H__

#define MAX_WEAPON_NAME 128


#define WEAPON_FLAGS_SELECTONEMPTY 1

struct WEAPON
{
	char szName[MAX_WEAPON_NAME] = { 0 };
	int iAmmoType = 0;
	int iAmmo2Type = 0;
	int iMax1 = 0;
	int iMax2 = 0;
	int iSlot = 0;
	int iSlotPos = 0;
	int iFlags = 0;
	int iId = 0;
	int iClip = 0;

	int iCount = 0;  // # of itesm in plist

	HSPRITE_VALVE hActive = 0;
	wrect_t rcActive;
	HSPRITE_VALVE hInactive = 0;
	wrect_t rcInactive;
	HSPRITE_VALVE hAmmo = 0;
	wrect_t rcAmmo;
	HSPRITE_VALVE hAmmo2 = 0;
	wrect_t rcAmmo2;
	HSPRITE_VALVE hCrosshair = 0;
	wrect_t rcCrosshair;
	HSPRITE_VALVE hAutoaim = 0;
	wrect_t rcAutoaim;
	HSPRITE_VALVE hZoomedCrosshair = 0;
	wrect_t rcZoomedCrosshair;
	HSPRITE_VALVE hZoomedAutoaim = 0;
	wrect_t rcZoomedAutoaim;
};

typedef int AMMO;


#endif
