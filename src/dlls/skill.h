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
//=========================================================
// skill.h - skill level concerns
//=========================================================

struct skilldata_t
{
	int iSkillLevel = 0;  // game skill level

	// Monster Health & Damage
	float agruntHealth = 0.f;
	float agruntDmgPunch = 0.f;

	float apacheHealth = 0.f;

	float barneyHealth = 0.f;

	float bigmommaHealthFactor = 0.f;  // Multiply each node's health by this
	float bigmommaDmgSlash = 0.f;      // melee attack damage
	float bigmommaDmgBlast = 0.f;      // mortar attack damage
	float bigmommaRadiusBlast = 0.f;   // mortar attack radius

	float bullsquidHealth = 0.f;
	float bullsquidDmgBite = 0.f;
	float bullsquidDmgWhip = 0.f;
	float bullsquidDmgSpit = 0.f;

	float gargantuaHealth = 0.f;
	float gargantuaDmgSlash = 0.f;
	float gargantuaDmgFire = 0.f;
	float gargantuaDmgStomp = 0.f;

	float hassassinHealth = 0.f;

	float headcrabHealth = 0.f;
	float headcrabDmgBite = 0.f;

	float hgruntHealth = 0.f;
	float hgruntDmgKick = 0.f;
	float hgruntShotgunPellets = 0.f;
	float hgruntGrenadeSpeed = 0.f;

	float houndeyeHealth = 0.f;
	float houndeyeDmgBlast = 0.f;

	float slaveHealth = 0.f;
	float slaveDmgClaw = 0.f;
	float slaveDmgClawrake = 0.f;
	float slaveDmgZap = 0.f;

	float ichthyosaurHealth = 0.f;
	float ichthyosaurDmgShake = 0.f;

	float leechHealth = 0.f;
	float leechDmgBite = 0.f;

	float controllerHealth = 0.f;
	float controllerDmgZap = 0.f;
	float controllerSpeedBall = 0.f;
	float controllerDmgBall = 0.f;

	float nihilanthHealth = 0.f;
	float nihilanthZap = 0.f;

	float scientistHealth = 0.f;

	float snarkHealth = 0.f;
	float snarkDmgBite = 0.f;
	float snarkDmgPop = 0.f;

	float zombieHealth = 0.f;
	float zombieDmgOneSlash = 0.f;
	float zombieDmgBothSlash = 0.f;

	float turretHealth = 0.f;
	float miniturretHealth = 0.f;
	float sentryHealth = 0.f;


	// Player Weapons
	float plrDmgCrowbar = 0.f;
	float plrDmg9MM = 0.f;
	float plrDmg357 = 0.f;
	float plrDmgMP5 = 0.f;
	float plrDmgM203Grenade = 0.f;
	float plrDmgBuckshot = 0.f;
	float plrDmgCrossbowClient = 0.f;
	float plrDmgCrossbowMonster = 0.f;
	float plrDmgRPG = 0.f;
	float plrDmgGauss = 0.f;
	float plrDmgEgonNarrow = 0.f;
	float plrDmgEgonWide = 0.f;
	float plrDmgHornet = 0.f;
	float plrDmgHandGrenade = 0.f;
	float plrDmgSatchel = 0.f;
	float plrDmgTripmine = 0.f;

	// weapons shared by monsters
	float monDmg9MM = 0.f;
	float monDmgMP5 = 0.f;
	float monDmg12MM = 0.f;
	float monDmgHornet = 0.f;

	// health/suit charge
	float suitchargerCapacity = 0.f;
	float batteryCapacity = 0.f;
	float healthchargerCapacity = 0.f;
	float healthkitCapacity = 0.f;
	float scientistHeal = 0.f;

	// monster damage adj
	float monHead = 0.f;
	float monChest = 0.f;
	float monStomach = 0.f;
	float monLeg = 0.f;
	float monArm = 0.f;

	// player damage adj
	float plrHead = 0.f;
	float plrChest = 0.f;
	float plrStomach = 0.f;
	float plrLeg = 0.f;
	float plrArm = 0.f;
};

extern DLL_GLOBAL skilldata_t gSkillData;
float GetSkillCvar(char* pName);

extern DLL_GLOBAL int g_iSkillLevel;

#define SKILL_EASY   1
#define SKILL_MEDIUM 2
#define SKILL_HARD   3
