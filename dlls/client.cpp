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
// Robin, 4-22-98: Moved set_suicide_frame() here from player.cpp to allow us to
//				   have one without a hardcoded player.mdl in tf_client.cpp

/*

===== client.cpp ========================================================

  client/server game specific stuff

*/

#include <vector>
#include <unordered_map>
#include <algorithm>

#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "saverestore.h"
#include "player.h"
#include "spectator.h"
#include "client.h"
#include "soundent.h"
#include "gamerules.h"
#include "game.h"
#include "customentity.h"
#include "weapons.h"
#include "weaponinfo.h"
#include "usercmd.h"
#include "netadr.h"

#include "VRPhysicsHelper.h"
#include "vr_shared/VRShared.h"

extern DLL_GLOBAL ULONG g_ulModelIndexPlayer;
extern DLL_GLOBAL BOOL g_fGameOver;
extern DLL_GLOBAL int g_iSkillLevel;
extern DLL_GLOBAL ULONG g_ulFrameCount;

extern void CopyToBodyQue(entvars_t* pev);
extern int giPrecacheGrunt;
extern int gmsgSayText;

extern int g_teamplay;

void LinkUserMessages(void);

/*
 * used by kill command and disconnect command
 * ROBIN: Moved here from player.cpp, to allow multiple player models
 */
void set_suicide_frame(entvars_t* pev)
{
	if (FNullEnt(pev))
		return;

	if (!FStrEq(STRING(pev->model), "models/player.mdl"))
		return;  // allready gibbed

	//	pev->frame		= $deatha11;
	pev->solid = SOLID_NOT;
	pev->movetype = MOVETYPE_TOSS;
	pev->deadflag = DEAD_DEAD;
	pev->nextthink = -1;
}


/*
===========
ClientConnect

called when a player connects to a server
============
*/
BOOL ClientConnect(edict_t* pEntity, const char* pszName, const char* pszAddress, char szRejectReason[128])
{
	// Don't restore entities in game loaded from invalid savegame (see world.cpp RestoreGlobalState)
	extern bool g_didRestoreSaveGameFail;
	if (g_didRestoreSaveGameFail)
		return FALSE;

	if (!pEntity)
		return FALSE;

	if (!pEntity->pvPrivateData)
		ClientPutInServer(pEntity);

	if (FNullEnt(pEntity))
		return FALSE;

	if (!g_pGameRules)
		return TRUE;

	return g_pGameRules->ClientConnected(pEntity, pszName, pszAddress, szRejectReason);

	// a client connecting during an intermission can cause problems
	//	if (intermission_running)
	//		ExitIntermission ();
}


/*
===========
ClientDisconnect

called when a player disconnects from a server

GLOBALS ASSUMED SET:  g_fGameOver
============
*/
void ClientDisconnect(edict_t* pEntity)
{
	if (FNullEnt(pEntity))
		return;

	if (g_fGameOver)
		return;

	char text[256];
	sprintf_s(text, "- %s has left the game\n", STRING(pEntity->v.netname));
	MESSAGE_BEGIN(MSG_ALL, gmsgSayText, nullptr);
	WRITE_BYTE(ENTINDEX(pEntity));
	WRITE_STRING(text);
	MESSAGE_END();

	CSound* pSound;
	pSound = CSoundEnt::SoundPointerForIndex(CSoundEnt::ClientSoundIndex(pEntity));
	{
		// since this client isn't around to think anymore, reset their sound.
		if (pSound)
		{
			pSound->Reset();
		}
	}

	// since the edict doesn't get deleted, fix it so it doesn't interfere.
	pEntity->v.takedamage = DAMAGE_NO;  // don't attract autoaim
	pEntity->v.solid = SOLID_NOT;  // nonsolid
	UTIL_SetOrigin(&pEntity->v, pEntity->v.origin);

	if (g_pGameRules)
		g_pGameRules->ClientDisconnected(pEntity);
}


// called by ClientKill and DeadThink
void respawn(entvars_t* pev, BOOL fCopyCorpse)
{
	if (FNullEnt(pev))
		return;

	if (gpGlobals->coop || gpGlobals->deathmatch)
	{
		if (fCopyCorpse)
		{
			// make a copy of the dead body for appearances sake
			CopyToBodyQue(pev);
		}

		// respawn player
		CBasePlayer* pPlayer = CBaseEntity::SafeInstance<CBasePlayer>(pev);
		if (pPlayer)
			pPlayer->Spawn();
	}
	else
	{  // restart the entire server
		SERVER_COMMAND("reload\n");
	}
}

/*
============
ClientKill

Player entered the suicide command

GLOBALS ASSUMED SET:  g_ulModelIndexPlayer
============
*/
void ClientKill(edict_t* pEntity)
{
	if (FNullEnt(pEntity))
		return;

	CBasePlayer* pl = CBaseEntity::SafeInstance<CBasePlayer>(pEntity);
	if (!pl)
		return;

	if (pl->m_fNextSuicideTime > gpGlobals->time)
		return;  // prevent suiciding too ofter

	pl->m_fNextSuicideTime = gpGlobals->time + 1;  // don't let them suicide for 5 seconds after suiciding

	// have the player kill themself
	pl->pev->health = 0;
	pl->Killed(pl->pev, DMG_GENERIC, GIB_NEVER);
}

/*
===========
ClientPutInServer

called each time a player is spawned
============
*/
void ClientPutInServer(edict_t* pEntity)
{
	if (!FNullEnt(pEntity))
	{
		ALERT(at_console, "ClientPutInServer: Got a player that already spawned!\n");
		g_engfuncs.pfnFreeEntPrivateData(pEntity);
		pEntity->pvPrivateData = nullptr;
		pEntity->v.pContainingEntity = pEntity;
	}

	CBasePlayer* pPlayer = GetClassPtr<CBasePlayer>(&pEntity->v);
	pPlayer->SetCustomDecalFrames(-1);  // Assume none;

	// Allocate a CBasePlayer for pev, and call spawn
	pPlayer->Spawn();

	// Reset interpolation during first frame
	pPlayer->pev->effects |= EF_NOINTERP;

	pPlayer->pev->iuser1 = 0;	// disable any spec modes
	pPlayer->pev->iuser2 = 0;
}

#include "voice_gamemgr.h"
extern CVoiceGameMgr g_VoiceGameMgr;

//// HOST_SAY
// String comes in as
// say blah blah blah
// or as
// blah blah blah
//
void Host_Say(edict_t* pEntity, int teamonly)
{
	if (FNullEnt(pEntity))
		return;

	// We can get a raw string now, without the "say " prepended
	if (CMD_ARGC() == 0)
		return;

	CBasePlayer* player = CBaseEntity::SafeInstance<CBasePlayer>(pEntity);
	if (!player)
		return;

	//Not yet.
	if (player->m_flNextChatTime > gpGlobals->time)
		return;

	int j = 0;
	char* p;
	char text[128];
	char szTemp[256];
	const char* cpSay = "say";
	const char* cpSayTeam = "say_team";
	const char* pcmd = CMD_ARGV(0);

	if (!_stricmp(pcmd, cpSay) || !_stricmp(pcmd, cpSayTeam))
	{
		if (CMD_ARGC() >= 2)
		{
			p = const_cast<char*>(CMD_ARGS());	// TODO: Evil const_cast
		}
		else
		{
			// say with a blank message, nothing to do
			return;
		}
	}
	else  // Raw text, need to prepend argv[0]
	{
		if (CMD_ARGC() >= 2)
		{
			sprintf_s(szTemp, "%s %s", pcmd, CMD_ARGS());
		}
		else
		{
			// Just a one word command, use the first word...sigh
			sprintf_s(szTemp, "%s", pcmd);
		}
		p = szTemp;
	}

	// remove quotes if present
	if (*p == '"')
	{
		p++;
		p[strlen(p) - 1] = 0;
	}

	// make sure the text has content
	char* pc = p;
	for (; pc != nullptr && *pc != 0; pc++)
	{
		if (isprint(*pc) && !isspace(*pc))
		{
			pc = nullptr;  // we've found an alphanumeric character,  so text is valid
			break;
		}
	}
	if (pc != nullptr)
		return;  // no character found, so say nothing

	// turn on color set 2  (color on,  no sound)
	if (teamonly)
		sprintf_s(text, "%c(TEAM) %s: ", 2, STRING(pEntity->v.netname));
	else
		sprintf_s(text, "%c%s: ", 2, STRING(pEntity->v.netname));

	j = sizeof(text) - 2 - strlen(text);  // -2 for /n and null terminator
	if ((int)strlen(p) > j)
		p[j] = 0;

	strcat_s(text, p);
	strcat_s(text, "\n");


	player->m_flNextChatTime = gpGlobals->time + CHAT_INTERVAL;

	// loop through all players
	// Start with the first player.
	// This may return the world in single player if the client types something between levels or during spawn
	// so check it, or it will infinite loop

	CBasePlayer* client = nullptr;
	while (((client = dynamic_cast<CBasePlayer*>(UTIL_FindEntityByClassname(client, "player"))) != nullptr) && (!FNullEnt(client->edict())))
	{
		if (!client->pev)
			continue;

		if (client->edict() == pEntity)
			continue;

		if (!(client->IsNetClient()))  // Not a client ? (should never be true)
			continue;

		// can the receiver hear the sender? or have they muted them?
		if (g_VoiceGameMgr.PlayerHasBlockedPlayer(client, player))
			continue;

		if (teamonly && g_pGameRules->PlayerRelationship(client, CBaseEntity::InstanceOrWorld(pEntity)) != GR_TEAMMATE)
			continue;

		MESSAGE_BEGIN(MSG_ONE, gmsgSayText, nullptr, client->pev);
		WRITE_BYTE(ENTINDEX(pEntity));
		WRITE_STRING(text);
		MESSAGE_END();
	}

	// print to the sending client
	MESSAGE_BEGIN(MSG_ONE, gmsgSayText, nullptr, &pEntity->v);
	WRITE_BYTE(ENTINDEX(pEntity));
	WRITE_STRING(text);
	MESSAGE_END();

	// echo to server console
	g_engfuncs.pfnServerPrint(text);

	char* temp;
	if (teamonly)
		temp = "say_team";
	else
		temp = "say";

	// team match?
	if (g_teamplay)
	{
		UTIL_LogPrintf("\"%s<%i><%s><%s>\" %s \"%s\"\n",
			STRING(pEntity->v.netname),
			GETPLAYERUSERID(pEntity),
			GETPLAYERAUTHID(pEntity),
			g_engfuncs.pfnInfoKeyValue(g_engfuncs.pfnGetInfoKeyBuffer(pEntity), "model"),
			temp,
			p);
	}
	else
	{
		UTIL_LogPrintf("\"%s<%i><%s><%i>\" %s \"%s\"\n",
			STRING(pEntity->v.netname),
			GETPLAYERUSERID(pEntity),
			GETPLAYERAUTHID(pEntity),
			GETPLAYERUSERID(pEntity),
			temp,
			p);
	}
}


/*
===========
ClientCommand
called each time a player uses a "cmd" command
============
*/
extern bool AreCheatsEnabled();

// Use CMD_ARGV,  CMD_ARGV, and CMD_ARGC to get pointers the character string command.
void ClientCommand(edict_t* pEntity)
{
	const char* pcmd = CMD_ARGV(0);

	// Is the client spawned yet?
	if (FNullEnt(pEntity))
		return;

	CBasePlayer* pPlayer = CBaseEntity::SafeInstance<CBasePlayer>(pEntity);
	if (!pPlayer)
		return;

	if (FStrEq(pcmd, "say"))
	{
		Host_Say(pEntity, 0);
	}
	else if (FStrEq(pcmd, "say_team"))
	{
		Host_Say(pEntity, 1);
	}
	else if (FStrEq(pcmd, "fullupdate"))
	{
		pPlayer->ForceClientDllUpdate();
	}
	else if (FStrEq(pcmd, "give"))
	{
		if (AreCheatsEnabled())
		{
			int iszItem = ALLOC_STRING(CMD_ARGV(1));  // Make a copy of the classname
			pPlayer->GiveNamedItem(STRING(iszItem));
		}
	}

	else if (FStrEq(pcmd, "drop"))
	{
		// player is dropping an item.
		pPlayer->DropPlayerItem(CMD_ARGV(1));
	}
	else if (FStrEq(pcmd, "fov"))
	{
		if (AreCheatsEnabled() && CMD_ARGC() > 1)
		{
			pPlayer->m_iFOV = atoi(CMD_ARGV(1));
		}
		else
		{
			CLIENT_PRINTF(pEntity, print_console, UTIL_VarArgs("\"fov\" is \"%d\"\n", pPlayer->m_iFOV));
		}
	}
	else if (FStrEq(pcmd, "use"))
	{
		pPlayer->SelectItem(CMD_ARGV(1));
	}
	else if (strstr(pcmd, "weapon_") == pcmd)
	{
		pPlayer->SelectItem(pcmd);
	}
	else if (FStrEq(pcmd, "lastinv"))
	{
		pPlayer->SelectLastItem();
	}
	else if (FStrEq(pcmd, "spectate") && (pPlayer->pev->flags & FL_PROXY))  // added for proxy support
	{
		edict_t* pentSpawnSpot = g_pGameRules->GetPlayerSpawnSpot(pPlayer);
		pPlayer->StartObserver(pPlayer->pev->origin, VARS(pentSpawnSpot)->angles);
	}
	else if (FStrEq(pcmd, "vr_flashlight"))
	{
		if (atoi(CMD_ARGV(1)))
		{
			Vector offset(atof(CMD_ARGV(2)), atof(CMD_ARGV(3)), atof(CMD_ARGV(4)));
			Vector angles(atof(CMD_ARGV(5)), atof(CMD_ARGV(6)), atof(CMD_ARGV(7)));
			pPlayer->SetFlashlightPose(offset, angles);
		}
		else
		{
			pPlayer->ClearFlashlightPose();
		}
	}
	else if (FStrEq(pcmd, "vr_teleporter"))
	{
		if (atoi(CMD_ARGV(1)))
		{
			Vector offset(atof(CMD_ARGV(2)), atof(CMD_ARGV(3)), atof(CMD_ARGV(4)));
			Vector angles(atof(CMD_ARGV(5)), atof(CMD_ARGV(6)), atof(CMD_ARGV(7)));
			pPlayer->SetTeleporterPose(offset, angles);
		}
		else
		{
			pPlayer->ClearTeleporterPose();
		}
	}
	else if (FStrEq(pcmd, "vr_anlgfire"))
	{
		pPlayer->SetAnalogFire(atof(CMD_ARGV(1)));
	}
	else if (FStrEq(pcmd, "vr_lngjump"))
	{
		pPlayer->DoLongJump();
	}
	else if (FStrEq(pcmd, "vr_restartmap"))
	{
		pPlayer->RestartCurrentMap();
	}
	else if (FStrEq(pcmd, "vr_wpnanim"))  // Client side weapon animations are now sent to the server - Max Vollmer, 2019-04-13
	{
		int sequence = atoi(CMD_ARGV(1));
		int body = atoi(CMD_ARGV(2));
		pPlayer->PlayVRWeaponAnimation(sequence, body);
	}
	else if (FStrEq(pcmd, "vr_muzzleflash"))  // Client side weapon animations are now sent to the server - Max Vollmer, 2019-04-13
	{
		pPlayer->PlayVRWeaponMuzzleflash();
	}
	else if (FStrEq(pcmd, "vrupd_hmd"))  // Client sends update for VR related data - Max Vollmer, 2017-08-18
	{
		int size = CMD_ARGC();
		if (size == 14)
		{
			int timestamp = atoi(CMD_ARGV(1));
			Vector2D offset(atof(CMD_ARGV(2)), atof(CMD_ARGV(3)));
			float offsetZ = atof(CMD_ARGV(4));
			Vector forward(atof(CMD_ARGV(5)), atof(CMD_ARGV(6)), atof(CMD_ARGV(7)));
			Vector2D yawOffsetDelta(atof(CMD_ARGV(8)), atof(CMD_ARGV(9)));
			float prevYaw = atof(CMD_ARGV(10));
			float currentYaw = atof(CMD_ARGV(11));
			bool hasReceivedRestoreYawMsg = atoi(CMD_ARGV(12)) != 0;
			bool hasReceivedSpawnYaw = atoi(CMD_ARGV(13)) != 0;
			pPlayer->UpdateVRHeadset(timestamp, offset, offsetZ, forward, yawOffsetDelta, prevYaw, currentYaw, hasReceivedRestoreYawMsg, hasReceivedSpawnYaw);
		}
		else
		{
			char errormsg[1024] = { 0 };
			sprintf_s(errormsg, "Invalid vr update (%i): %s %s!\n", size, pcmd, CMD_ARGS());
			ClientPrint(&pEntity->v, HUD_PRINTCONSOLE, errormsg);
		}
	}
	else if (FStrEq(pcmd, "vrupdctrl"))  // Client sends update for VR related data - Max Vollmer, 2017-08-18
	{
		int size = CMD_ARGC();
		if (size == 15)
		{
			int timestamp = atoi(CMD_ARGV(1));
			bool isValid = atoi(CMD_ARGV(2)) != 0;
			VRControllerID id = VRControllerID(atoi(CMD_ARGV(3)));
			bool isMirrored = atoi(CMD_ARGV(4)) != 0;
			Vector offset(atof(CMD_ARGV(5)), atof(CMD_ARGV(6)), atof(CMD_ARGV(7)));
			Vector angles(atof(CMD_ARGV(8)), atof(CMD_ARGV(9)), atof(CMD_ARGV(10)));
			Vector velocity(atof(CMD_ARGV(11)), atof(CMD_ARGV(12)), atof(CMD_ARGV(13)));
			bool isDragging = atoi(CMD_ARGV(14)) != 0;
			pPlayer->UpdateVRController(id, timestamp, isValid, isMirrored, offset, angles, velocity, isDragging);
		}
		else
		{
			char errormsg[1024] = { 0 };
			sprintf_s(errormsg, "Invalid vr update (%i): %s %s!\n", size, pcmd, CMD_ARGS());
			ClientPrint(&pEntity->v, HUD_PRINTCONSOLE, errormsg);
		}
	}
	else if (FStrEq(pcmd, "vrtele"))
	{
		if (atoi(CMD_ARGV(1)))
		{
			pPlayer->StartVRTele();
		}
		else
		{
			pPlayer->StopVRTele();
		}
	}
	else if (FStrEq(pcmd, "vrspeech"))
	{
		pPlayer->HandleSpeechCommand(VRSpeechCommand(atoi(CMD_ARGV(1))));
	}
	else if (g_pGameRules && g_pGameRules->ClientCommand(pPlayer, pcmd))
	{
		// MenuSelect returns true only if the command is properly handled,  so don't print a warning
	}
	else
	{
		// tell the user they entered an unknown command
		char command[128];

		// check the length of the command (prevents crash)
		// max total length is 192 ...and we're adding a string below ("Unknown command: %s\n")
		strncpy_s(command, pcmd, 127);
		command[127] = '\0';

		// tell the user they entered an unknown command
		ClientPrint(&pEntity->v, HUD_PRINTCONSOLE, UTIL_VarArgs("Unknown command: %s\n", command));
	}
}


/*
========================
ClientUserInfoChanged

called after the player changes
userinfo - gives dll a chance to modify it before
it gets sent into the rest of the engine.
========================
*/
void ClientUserInfoChanged(edict_t* pEntity, char* infobuffer)
{
	// Is the client spawned yet?
	if (FNullEnt(pEntity))
		return;

	// msg everyone if someone changes their name,  and it isn't the first time (changing no name to current name)
	if (pEntity->v.netname && STRING(pEntity->v.netname)[0] != 0 && !FStrEq(STRING(pEntity->v.netname), g_engfuncs.pfnInfoKeyValue(infobuffer, "name")))
	{
		char sName[256];
		char* pName = g_engfuncs.pfnInfoKeyValue(infobuffer, "name");
		strncpy_s(sName, pName, sizeof(sName) - 1);
		sName[sizeof(sName) - 1] = '\0';

		// First parse the name and remove any %'s
		for (char* pApersand = sName; pApersand != nullptr && *pApersand != 0; pApersand++)
		{
			// Replace it with a space
			if (*pApersand == '%')
				*pApersand = ' ';
		}

		// Set the name
		g_engfuncs.pfnSetClientKeyValue(ENTINDEX(pEntity), infobuffer, "name", sName);

		char text[256];
		sprintf_s(text, "* %s changed name to %s\n", STRING(pEntity->v.netname), g_engfuncs.pfnInfoKeyValue(infobuffer, "name"));
		MESSAGE_BEGIN(MSG_ALL, gmsgSayText, nullptr);
		WRITE_BYTE(ENTINDEX(pEntity));
		WRITE_STRING(text);
		MESSAGE_END();

		// team match?
		if (g_teamplay)
		{
			UTIL_LogPrintf("\"%s<%i><%s><%s>\" changed name to \"%s\"\n",
				STRING(pEntity->v.netname),
				GETPLAYERUSERID(pEntity),
				GETPLAYERAUTHID(pEntity),
				g_engfuncs.pfnInfoKeyValue(infobuffer, "model"),
				g_engfuncs.pfnInfoKeyValue(infobuffer, "name"));
		}
		else
		{
			UTIL_LogPrintf("\"%s<%i><%s><%i>\" changed name to \"%s\"\n",
				STRING(pEntity->v.netname),
				GETPLAYERUSERID(pEntity),
				GETPLAYERAUTHID(pEntity),
				GETPLAYERUSERID(pEntity),
				g_engfuncs.pfnInfoKeyValue(infobuffer, "name"));
		}
	}

	CBasePlayer* pPlayer = CBaseEntity::SafeInstance<CBasePlayer>(&pEntity->v);
	if (g_pGameRules && pPlayer)
		g_pGameRules->ClientUserInfoChanged(pPlayer, infobuffer);
}

static int g_serveractive = 0;

void ServerDeactivate(void)
{
	// It's possible that the engine will call this function more times than is necessary
	//  Therefore, only run it one time for each call to ServerActivate
	if (g_serveractive != 1)
	{
		return;
	}

	g_serveractive = 0;

	// Peform any shutdown operations here...
	//

	VRPhysicsHelper::DestroyInstance();
}

void ServerActivate(edict_t* pEdictList, int edictCount, int clientMax)
{
	// Don't activate the server if we loaded from invalid savegame (see world.cpp RestoreGlobalState)
	extern bool g_didRestoreSaveGameFail;
	if (g_didRestoreSaveGameFail)
		return;

	VRPhysicsHelper::CreateInstance();

	// Every call to ServerActivate should be matched by a call to ServerDeactivate
	g_serveractive = 1;

	// Clients have not been initialized yet
	for (int i = 0; i < edictCount; i++)
	{
		if (pEdictList[i].free)
			continue;

		// Clients aren't necessarily initialized until ClientPutInServer()
		if (i < clientMax || !pEdictList[i].pvPrivateData)
			continue;

		CBaseEntity*  pClass = CBaseEntity::SafeInstance<CBaseEntity>(&pEdictList[i]);
		// Activate this entity if it's got a class & isn't dormant
		if (pClass && !(pClass->pev->flags & FL_DORMANT))
		{
			pClass->Activate();
		}
		else
		{
			ALERT(at_console, "Can't instance %s\n", STRING(pEdictList[i].v.classname));
		}
	}

	// Link user messages here to make sure first client can get them...
	LinkUserMessages();
}


/*
================
PlayerPreThink

Called every frame before physics are run
================
*/
void PlayerPreThink(edict_t* pEntity)
{
	if (!pEntity->pvPrivateData)
		ClientPutInServer(pEntity);

	CBasePlayer* pPlayer = CBaseEntity::SafeInstance<CBasePlayer>(pEntity);
	if (pPlayer)
		pPlayer->PreThink();
}

/*
================
PlayerPostThink

Called every frame after physics are run
================
*/
void PlayerPostThink(edict_t* pEntity)
{
	CBasePlayer* pPlayer = CBaseEntity::SafeInstance<CBasePlayer>(pEntity);
	if (pPlayer)
	{
		pPlayer->PostThink();

		extern void EnsureEasteregg(CBasePlayer* pPlayer);
		EnsureEasteregg(pPlayer);
	}
}



void ParmsNewLevel(void)
{
}


void ParmsChangeLevel(void)
{
	// retrieve the pointer to the save data
	SAVERESTOREDATA* pSaveData = static_cast<SAVERESTOREDATA*>(gpGlobals->pSaveData);

	if (pSaveData)
		pSaveData->connectionCount = BuildChangeList(pSaveData->levelList, MAX_LEVEL_CONNECTIONS);
}


//
// GLOBALS ASSUMED SET:  g_ulFrameCount
//
void StartFrame(void)
{
	extern void VRClearCvarCache();
	VRClearCvarCache();

	// Don't think in game loaded from invalid savegame (see world.cpp RestoreGlobalState)
	extern bool g_didRestoreSaveGameFail;
	if (g_didRestoreSaveGameFail)
	{
		// We cannot shut down a server, but we can switch to crossfire
		extern bool g_didRestoreSaveGameFail_MapChangedToSafety;
		if (!g_didRestoreSaveGameFail_MapChangedToSafety)
		{
			g_engfuncs.pfnChangeLevel("crossfire", nullptr);
			g_didRestoreSaveGameFail_MapChangedToSafety = true;
		}
		return;
	}

	UTIL_UpdateSDModels();

	VRPhysicsHelper::Instance().StartFrame();

	if (g_pGameRules)
		g_pGameRules->Think();

	if (g_fGameOver)
		return;

	gpGlobals->teamplay = teamplay.value;
	g_ulFrameCount++;
}


void ClientPrecache(void)
{
	// setup precaches always needed
	PRECACHE_SOUND("player/sprayer.wav");  // spray paint sound for PreAlpha

	// PRECACHE_SOUND("player/pl_jumpland2.wav");		// UNDONE: play 2x step sound

	PRECACHE_SOUND("player/pl_fallpain2.wav");
	PRECACHE_SOUND("player/pl_fallpain3.wav");

	PRECACHE_SOUND("player/pl_step1.wav");  // walk on concrete
	PRECACHE_SOUND("player/pl_step2.wav");
	PRECACHE_SOUND("player/pl_step3.wav");
	PRECACHE_SOUND("player/pl_step4.wav");

	PRECACHE_SOUND("common/npc_step1.wav");  // NPC walk on concrete
	PRECACHE_SOUND("common/npc_step2.wav");
	PRECACHE_SOUND("common/npc_step3.wav");
	PRECACHE_SOUND("common/npc_step4.wav");

	PRECACHE_SOUND("player/pl_metal1.wav");  // walk on metal
	PRECACHE_SOUND("player/pl_metal2.wav");
	PRECACHE_SOUND("player/pl_metal3.wav");
	PRECACHE_SOUND("player/pl_metal4.wav");

	PRECACHE_SOUND("player/pl_dirt1.wav");  // walk on dirt
	PRECACHE_SOUND("player/pl_dirt2.wav");
	PRECACHE_SOUND("player/pl_dirt3.wav");
	PRECACHE_SOUND("player/pl_dirt4.wav");

	PRECACHE_SOUND("player/pl_duct1.wav");  // walk in duct
	PRECACHE_SOUND("player/pl_duct2.wav");
	PRECACHE_SOUND("player/pl_duct3.wav");
	PRECACHE_SOUND("player/pl_duct4.wav");

	PRECACHE_SOUND("player/pl_grate1.wav");  // walk on grate
	PRECACHE_SOUND("player/pl_grate2.wav");
	PRECACHE_SOUND("player/pl_grate3.wav");
	PRECACHE_SOUND("player/pl_grate4.wav");

	PRECACHE_SOUND("player/pl_slosh1.wav");  // walk in shallow water
	PRECACHE_SOUND("player/pl_slosh2.wav");
	PRECACHE_SOUND("player/pl_slosh3.wav");
	PRECACHE_SOUND("player/pl_slosh4.wav");

	PRECACHE_SOUND("player/pl_tile1.wav");  // walk on tile
	PRECACHE_SOUND("player/pl_tile2.wav");
	PRECACHE_SOUND("player/pl_tile3.wav");
	PRECACHE_SOUND("player/pl_tile4.wav");
	PRECACHE_SOUND("player/pl_tile5.wav");

	PRECACHE_SOUND("player/pl_swim1.wav");  // breathe bubbles
	PRECACHE_SOUND("player/pl_swim2.wav");
	PRECACHE_SOUND("player/pl_swim3.wav");
	PRECACHE_SOUND("player/pl_swim4.wav");

	PRECACHE_SOUND("player/pl_ladder1.wav");  // climb ladder rung
	PRECACHE_SOUND("player/pl_ladder2.wav");
	PRECACHE_SOUND("player/pl_ladder3.wav");
	PRECACHE_SOUND("player/pl_ladder4.wav");

	PRECACHE_SOUND("player/pl_wade1.wav");  // wade in water
	PRECACHE_SOUND("player/pl_wade2.wav");
	PRECACHE_SOUND("player/pl_wade3.wav");
	PRECACHE_SOUND("player/pl_wade4.wav");

	PRECACHE_SOUND("debris/wood1.wav");  // hit wood texture
	PRECACHE_SOUND("debris/wood2.wav");
	PRECACHE_SOUND("debris/wood3.wav");

	PRECACHE_SOUND("plats/train_use1.wav");  // use a train
	PRECACHE_SOUND("plats/train_use2.wav");  // use a train in VR

	PRECACHE_SOUND("buttons/spark5.wav");  // hit computer texture
	PRECACHE_SOUND("buttons/spark6.wav");
	PRECACHE_SOUND("debris/glass1.wav");
	PRECACHE_SOUND("debris/glass2.wav");
	PRECACHE_SOUND("debris/glass3.wav");

	PRECACHE_SOUND(SOUND_FLASHLIGHT_ON);
	PRECACHE_SOUND(SOUND_FLASHLIGHT_OFF);

	// player gib sounds
	PRECACHE_SOUND("common/bodysplat.wav");

	// player pain sounds
	PRECACHE_SOUND("player/pl_pain2.wav");
	PRECACHE_SOUND("player/pl_pain4.wav");
	PRECACHE_SOUND("player/pl_pain5.wav");
	PRECACHE_SOUND("player/pl_pain6.wav");
	PRECACHE_SOUND("player/pl_pain7.wav");

	PRECACHE_MODEL("models/player.mdl");

	// hud sounds

	PRECACHE_SOUND("common/wpn_hudoff.wav");
	PRECACHE_SOUND("common/wpn_hudon.wav");
	PRECACHE_SOUND("common/wpn_moveselect.wav");
	PRECACHE_SOUND("common/wpn_select.wav");
	PRECACHE_SOUND("common/wpn_denyselect.wav");


	// geiger sounds

	PRECACHE_SOUND("player/geiger6.wav");
	PRECACHE_SOUND("player/geiger5.wav");
	PRECACHE_SOUND("player/geiger4.wav");
	PRECACHE_SOUND("player/geiger3.wav");
	PRECACHE_SOUND("player/geiger2.wav");
	PRECACHE_SOUND("player/geiger1.wav");

	if (giPrecacheGrunt)
		UTIL_PrecacheOther("monster_human_grunt");

	PRECACHE_MODEL("sprites/black.spr");

	PRECACHE_MODEL("models/cup.mdl");
	PRECACHE_SOUND("easteregg/smallestcup.wav");

	// Clear global VR stuff here - Max Vollmer, 2018-04-02
	extern GlobalXenMounds gGlobalXenMounds;
	extern std::unordered_map<EHANDLE<CBaseEntity>, EHANDLE<CBaseEntity>, EHANDLE<CBaseEntity>::Hash, EHANDLE<CBaseEntity>::Equal> g_vrRetinaScanners;
	extern std::unordered_set<EHANDLE<CBaseEntity>, EHANDLE<CBaseEntity>::Hash, EHANDLE<CBaseEntity>::Equal> g_vrRetinaScannerButtons;
	extern bool g_vrNeedRecheckForSpecialEntities;
	gGlobalXenMounds.Clear();
	g_vrRetinaScanners.clear();
	g_vrRetinaScannerButtons.clear();
	g_vrNeedRecheckForSpecialEntities = true;
}

/*
===============
GetGameDescription

Returns the descriptive name of this .dll.  E.g., Half-Life, or Team Fortress 2
===============
*/
const char* GetGameDescription()
{
	if (g_pGameRules)  // this function may be called before the world has spawned, and the game rules initialized
		return g_pGameRules->GetGameDescription();
	else
		return "Half-Life: VR";
}

/*
================
Sys_Error

Engine is going to shut down, allows setting a breakpoint in game .dll to catch that occasion
================
*/
void Sys_Error(const char* error_string)
{
	// Default case, do nothing.  MOD AUTHORS:  Add code ( e.g., _asm { int 3 }; here to cause a breakpoint for debugging your game .dlls
}

/*
================
PlayerCustomization

A new player customization has been registered on the server
UNDONE:  This only sets the # of frames of the spray can logo
animation right now.
================
*/
void PlayerCustomization(edict_t* pEntity, customization_t* pCust)
{
	CBasePlayer* pPlayer = CBaseEntity::SafeInstance<CBasePlayer>(pEntity);
	if (!pPlayer)
	{
		ALERT(at_console, "PlayerCustomization:  Couldn't get player!\n");
		return;
	}

	if (!pCust)
	{
		ALERT(at_console, "PlayerCustomization:  nullptr customization!\n");
		return;
	}

	switch (pCust->resource.type)
	{
	case t_decal:
		pPlayer->SetCustomDecalFrames(pCust->nUserData2);  // Second int is max # of frames.
		break;
	case t_sound:
	case t_skin:
	case t_model:
		// Ignore for now.
		break;
	default:
		ALERT(at_console, "PlayerCustomization:  Unknown customization type!\n");
		break;
	}
}

/*
================
SpectatorConnect

A spectator has joined the game
================
*/
void SpectatorConnect(edict_t* pEntity)
{
	CBaseSpectator* pPlayer = CBaseEntity::SafeInstance<CBaseSpectator>(pEntity);
	if (pPlayer)
		pPlayer->SpectatorConnect();
}

/*
================
SpectatorConnect

A spectator has left the game
================
*/
void SpectatorDisconnect(edict_t* pEntity)
{
	CBaseSpectator* pPlayer = CBaseEntity::SafeInstance<CBaseSpectator>(pEntity);
	if (pPlayer)
		pPlayer->SpectatorDisconnect();
}

/*
================
SpectatorConnect

A spectator has sent a usercmd
================
*/
void SpectatorThink(edict_t* pEntity)
{
	CBaseSpectator* pPlayer = CBaseEntity::SafeInstance<CBaseSpectator>(pEntity);
	if (pPlayer)
		pPlayer->SpectatorThink();
}

////////////////////////////////////////////////////////
// PAS and PVS routines for client messaging
//

namespace
{
	unsigned char* m_pvsCache = nullptr;
}

/*
================
SetupVisibility

A client can have a separate "view entity" indicating that their view should depend on the origin of that
view entity.  If that's the case, then pViewEntity will be non-nullptr and will be used.  Otherwise, the current
entity's origin is used.  Either is offset by the view_ofs to get the eye position.

From the eye position, we set up the PAS and PVS to use for filtering network messages to the client.  At this point, we could
 override the actual PAS or PVS values, or use a different origin.

NOTE:  Do not cache the values of pas and pvs, as they depend on reusable memory in the engine, they are only good for this one frame
================
*/
void SetupVisibility(edict_t* pViewEntity, edict_t* pClient, unsigned char** pvs, unsigned char** pas)
{
	// Find the client's PVS
	edict_t* pView = !FNullEnt(pViewEntity) ? pViewEntity : pClient;

	if (FNullEnt(pView) || pClient->v.flags & FL_PROXY)
	{
		*pvs = nullptr;  // the spectator proxy sees
		*pas = nullptr;  // and hears everything
		m_pvsCache = nullptr;
		return;
	}

	Vector org = pView->v.origin + pView->v.view_ofs;
	*pvs = ENGINE_SET_PVS(org);
	*pas = ENGINE_SET_PAS(org);

	m_pvsCache = *pvs;
}

#include "entity_state.h"

/*
AddToFullPack

Return 1 if the entity state has been filled in for the ent and the entity will be propagated to the client, 0 otherwise

state is the server maintained copy of the state info that is transmitted to the client
a MOD could alter values copied into state to send the "host" a different look for a particular entity update, etc.
e and ent are the entity that is being added to the update, if 1 is returned
host is the player's edict of the player whom we are sending the update to
player is 1 if the ent/e is a player and 0 otherwise
pSet is either the PAS or PVS that we previous set up.  We can use it to ask the engine to filter the entity against the PAS or PVS.
we could also use the pas/ pvs that we set in SetupVisibility, if we wanted to.  Caching the value is valid in that case, but still only for the current frame
*/
int AddToFullPack(struct entity_state_s* state, int e, edict_t* ent, edict_t* host, int hostflags, int player, unsigned char* pSet)
{
	// If pSet is nullptr, then the test will always succeed and the entity will be added to the update
	bool isInPVS = ENGINE_CHECK_VISIBILITY(ent, pSet);

	// Remember if entity is in PVS
	if (m_pvsCache == pSet)
	{
		EHANDLE<CBaseEntity> hEnt = CBaseEntity::SafeInstance<CBaseEntity>(ent);
		if (hEnt)
		{
			hEnt->m_isInPVS = isInPVS;
		}
	}

	int i = 0;

	// don't send if flagged for NODRAW and it's not the host getting the message
	if ((ent->v.effects == EF_NODRAW) &&
		(ent != host))
		return 0;

	// Ignore ents without valid / visible models
	if (!ent->v.modelindex || !STRING(ent->v.model))
		return 0;

	// Don't send spectators to other players
	if ((ent->v.flags & FL_SPECTATOR) && (ent != host))
	{
		return 0;
	}

	// Ignore if not the host and not touching a PVS/PAS leaf
	if (ent != host)
	{
		if (!isInPVS)
		{
			return 0;
		}
	}


	// Don't send entity to local client if the client says it's predicting the entity itself.
	if (ent->v.flags & FL_SKIPLOCALHOST)
	{
		if ((hostflags & 1) && (ent->v.owner == host))
			return 0;
	}

	if (host->v.groupinfo)
	{
		UTIL_SetGroupTrace(host->v.groupinfo, GROUP_OP_AND);

		// Should always be set, of course
		if (ent->v.groupinfo)
		{
			if (g_groupop == GROUP_OP_AND)
			{
				if (!(ent->v.groupinfo & host->v.groupinfo))
					return 0;
			}
			else if (g_groupop == GROUP_OP_NAND)
			{
				if (ent->v.groupinfo & host->v.groupinfo)
					return 0;
			}
		}

		UTIL_UnsetGroupTrace();
	}

	memset(state, 0, sizeof(*state));

	// Assign index so we can track this entity from frame to frame and
	//  delta from it.
	state->number = e;
	state->entityType = ENTITY_NORMAL;

	// Flag custom entities.
	if (ent->v.flags & FL_CUSTOMENTITY)
	{
		state->entityType = ENTITY_BEAM;
	}

	//
	// Copy state data
	//

	// Round animtime to nearest millisecond
	state->animtime = (int)(1000.0 * ent->v.animtime) / 1000.0;

	memcpy(state->origin, ent->v.origin, 3 * sizeof(float));
	memcpy(state->angles, ent->v.angles, 3 * sizeof(float));
	memcpy(state->mins, ent->v.mins, 3 * sizeof(float));
	memcpy(state->maxs, ent->v.maxs, 3 * sizeof(float));

	memcpy(state->startpos, ent->v.startpos, 3 * sizeof(float));
	memcpy(state->endpos, ent->v.endpos, 3 * sizeof(float));

	state->impacttime = ent->v.impacttime;
	state->starttime = ent->v.starttime;

	state->modelindex = ent->v.modelindex;

	state->frame = ent->v.frame;

	state->skin = ent->v.skin;
	state->effects = ent->v.effects;

	// This non-player entity is being moved by the game .dll and not the physics simulation system
	//  make sure that we interpolate it's position on the client if it moves
	if (!player &&
		ent->v.animtime &&
		ent->v.velocity[0] == 0 &&
		ent->v.velocity[1] == 0 &&
		ent->v.velocity[2] == 0)
	{
		state->eflags |= EFLAG_SLERP;
	}

	state->scale = ent->v.scale;
	state->solid = ent->v.solid;
	state->colormap = ent->v.colormap;

	state->movetype = ent->v.movetype;
	state->sequence = ent->v.sequence;
	state->framerate = ent->v.framerate;
	state->body = ent->v.body;

	for (i = 0; i < 4; i++)
	{
		state->controller[i] = ent->v.controller[i];
	}

	for (i = 0; i < 2; i++)
	{
		state->blending[i] = ent->v.blending[i];
	}

	state->rendermode = ent->v.rendermode;
	state->renderamt = ent->v.renderamt;
	state->renderfx = ent->v.renderfx;
	state->rendercolor.r = ent->v.rendercolor.x;
	state->rendercolor.g = ent->v.rendercolor.y;
	state->rendercolor.b = ent->v.rendercolor.z;

	state->aiment = 0;
	if (ent->v.aiment)
	{
		state->aiment = ENTINDEX(ent->v.aiment);
	}

	state->owner = 0;
	if (ent->v.owner)
	{
		int owner = ENTINDEX(ent->v.owner);

		// Only care if owned by a player
		if (owner >= 1 && owner <= gpGlobals->maxClients)
		{
			state->owner = owner;
		}
	}

	// HACK:  Somewhat...
	// Class is overridden for non-players to signify a breakable glass object ( sort of a class? )
	if (!player)
	{
		state->playerclass = ent->v.playerclass;
	}

	// Special stuff for players only
	if (player)
	{
		memcpy(state->basevelocity, ent->v.basevelocity, 3 * sizeof(float));

		state->weaponmodel = MODEL_INDEX(STRING(ent->v.weaponmodel));
		state->gaitsequence = ent->v.gaitsequence;
		state->spectator = ent->v.flags & FL_SPECTATOR;
		state->friction = ent->v.friction;

		state->gravity = ent->v.gravity;
		//		state->team			= ent->v.team;
		//
		state->usehull = (ent->v.flags & FL_DUCKING) ? 1 : 0;
		state->health = ent->v.health;
	}

	return 1;
}

// defaults for clientinfo messages
#define DEFAULT_VIEWHEIGHT 28

/*
===================
CreateBaseline

Creates baselines used for network encoding, especially for player data since players are not spawned until connect time.
===================
*/
void CreateBaseline(int player, int eindex, struct entity_state_s* baseline, struct edict_s* entity, int playermodelindex, vec3_t player_mins, vec3_t player_maxs)
{
	baseline->origin = entity->v.origin;
	baseline->angles = entity->v.angles;
	baseline->frame = entity->v.frame;
	baseline->skin = (short)entity->v.skin;

	// render information
	baseline->rendermode = (byte)entity->v.rendermode;
	baseline->renderamt = (byte)entity->v.renderamt;
	baseline->rendercolor.r = (byte)entity->v.rendercolor.x;
	baseline->rendercolor.g = (byte)entity->v.rendercolor.y;
	baseline->rendercolor.b = (byte)entity->v.rendercolor.z;
	baseline->renderfx = (byte)entity->v.renderfx;

	if (player)
	{
		//baseline->mins			= player_mins;
		//baseline->maxs			= player_maxs;
		baseline->mins = entity->v.mins;
		baseline->maxs = entity->v.maxs;

		baseline->colormap = eindex;
		baseline->modelindex = playermodelindex;
		baseline->friction = 1.0;
		baseline->movetype = MOVETYPE_NOCLIP;  // MOVETYPE_WALK;

		baseline->scale = entity->v.scale;
		baseline->solid = SOLID_SLIDEBOX;
		baseline->framerate = 1.0;
		baseline->gravity = 0.0;
	}
	else
	{
		baseline->mins = entity->v.mins;
		baseline->maxs = entity->v.maxs;

		baseline->colormap = 0;
		baseline->modelindex = entity->v.modelindex;  //SV_ModelIndex(pr_strings + entity->v.model);
		baseline->movetype = entity->v.movetype;

		baseline->scale = entity->v.scale;
		baseline->solid = entity->v.solid;
		baseline->framerate = entity->v.framerate;
		baseline->gravity = entity->v.gravity;
	}
}

typedef struct
{
	char name[32];
	int field = 0;
} entity_field_alias_t;

#define FIELD_ORIGIN0 0
#define FIELD_ORIGIN1 1
#define FIELD_ORIGIN2 2
#define FIELD_ANGLES0 3
#define FIELD_ANGLES1 4
#define FIELD_ANGLES2 5

static entity_field_alias_t entity_field_alias[] =
{
	{"origin[0]", 0},
	{"origin[1]", 0},
	{"origin[2]", 0},
	{"angles[0]", 0},
	{"angles[1]", 0},
	{"angles[2]", 0},
};

void Entity_FieldInit(struct delta_s* pFields)
{
	entity_field_alias[FIELD_ORIGIN0].field = DELTA_FINDFIELD(pFields, entity_field_alias[FIELD_ORIGIN0].name);
	entity_field_alias[FIELD_ORIGIN1].field = DELTA_FINDFIELD(pFields, entity_field_alias[FIELD_ORIGIN1].name);
	entity_field_alias[FIELD_ORIGIN2].field = DELTA_FINDFIELD(pFields, entity_field_alias[FIELD_ORIGIN2].name);
	entity_field_alias[FIELD_ANGLES0].field = DELTA_FINDFIELD(pFields, entity_field_alias[FIELD_ANGLES0].name);
	entity_field_alias[FIELD_ANGLES1].field = DELTA_FINDFIELD(pFields, entity_field_alias[FIELD_ANGLES1].name);
	entity_field_alias[FIELD_ANGLES2].field = DELTA_FINDFIELD(pFields, entity_field_alias[FIELD_ANGLES2].name);
}

/*
==================
Entity_Encode

Callback for sending entity_state_t info over network.
FIXME:  Move to script
==================
*/
void Entity_Encode(struct delta_s* pFields, const unsigned char* from, const unsigned char* to)
{
	static int initialized = 0;
	if (!initialized)
	{
		Entity_FieldInit(pFields);
		initialized = 1;
	}

	const entity_state_t* f = reinterpret_cast<const entity_state_t*>(from);
	const entity_state_t* t = reinterpret_cast<const entity_state_t*>(to);

	// Never send origin to local player, it's sent with more resolution in clientdata_t structure
	int localplayer = (t->number - 1) == ENGINE_CURRENT_PLAYER();
	if (localplayer)
	{
		DELTA_UNSETBYINDEX(pFields, entity_field_alias[FIELD_ORIGIN0].field);
		DELTA_UNSETBYINDEX(pFields, entity_field_alias[FIELD_ORIGIN1].field);
		DELTA_UNSETBYINDEX(pFields, entity_field_alias[FIELD_ORIGIN2].field);
	}

	if ((t->impacttime != 0) && (t->starttime != 0))
	{
		DELTA_UNSETBYINDEX(pFields, entity_field_alias[FIELD_ORIGIN0].field);
		DELTA_UNSETBYINDEX(pFields, entity_field_alias[FIELD_ORIGIN1].field);
		DELTA_UNSETBYINDEX(pFields, entity_field_alias[FIELD_ORIGIN2].field);

		DELTA_UNSETBYINDEX(pFields, entity_field_alias[FIELD_ANGLES0].field);
		DELTA_UNSETBYINDEX(pFields, entity_field_alias[FIELD_ANGLES1].field);
		DELTA_UNSETBYINDEX(pFields, entity_field_alias[FIELD_ANGLES2].field);
	}

	if ((t->movetype == MOVETYPE_FOLLOW) &&
		(t->aiment != 0))
	{
		DELTA_UNSETBYINDEX(pFields, entity_field_alias[FIELD_ORIGIN0].field);
		DELTA_UNSETBYINDEX(pFields, entity_field_alias[FIELD_ORIGIN1].field);
		DELTA_UNSETBYINDEX(pFields, entity_field_alias[FIELD_ORIGIN2].field);
	}
	else if (t->aiment != f->aiment)
	{
		DELTA_SETBYINDEX(pFields, entity_field_alias[FIELD_ORIGIN0].field);
		DELTA_SETBYINDEX(pFields, entity_field_alias[FIELD_ORIGIN1].field);
		DELTA_SETBYINDEX(pFields, entity_field_alias[FIELD_ORIGIN2].field);
	}
}

static entity_field_alias_t player_field_alias[] =
{
	{"origin[0]", 0},
	{"origin[1]", 0},
	{"origin[2]", 0},
};

void Player_FieldInit(struct delta_s* pFields)
{
	player_field_alias[FIELD_ORIGIN0].field = DELTA_FINDFIELD(pFields, player_field_alias[FIELD_ORIGIN0].name);
	player_field_alias[FIELD_ORIGIN1].field = DELTA_FINDFIELD(pFields, player_field_alias[FIELD_ORIGIN1].name);
	player_field_alias[FIELD_ORIGIN2].field = DELTA_FINDFIELD(pFields, player_field_alias[FIELD_ORIGIN2].name);
}

/*
==================
Player_Encode

Callback for sending entity_state_t for players info over network.
==================
*/
void Player_Encode(struct delta_s* pFields, const unsigned char* from, const unsigned char* to)
{
	static int initialized = 0;
	if (!initialized)
	{
		Player_FieldInit(pFields);
		initialized = 1;
	}

	const entity_state_t* f = reinterpret_cast<const entity_state_t*>(from);
	const entity_state_t* t = reinterpret_cast<const entity_state_t*>(to);

	// Never send origin to local player, it's sent with more resolution in clientdata_t structure
	int localplayer = (t->number - 1) == ENGINE_CURRENT_PLAYER();
	if (localplayer)
	{
		DELTA_UNSETBYINDEX(pFields, entity_field_alias[FIELD_ORIGIN0].field);
		DELTA_UNSETBYINDEX(pFields, entity_field_alias[FIELD_ORIGIN1].field);
		DELTA_UNSETBYINDEX(pFields, entity_field_alias[FIELD_ORIGIN2].field);
	}

	if ((t->movetype == MOVETYPE_FOLLOW) &&
		(t->aiment != 0))
	{
		DELTA_UNSETBYINDEX(pFields, entity_field_alias[FIELD_ORIGIN0].field);
		DELTA_UNSETBYINDEX(pFields, entity_field_alias[FIELD_ORIGIN1].field);
		DELTA_UNSETBYINDEX(pFields, entity_field_alias[FIELD_ORIGIN2].field);
	}
	else if (t->aiment != f->aiment)
	{
		DELTA_SETBYINDEX(pFields, entity_field_alias[FIELD_ORIGIN0].field);
		DELTA_SETBYINDEX(pFields, entity_field_alias[FIELD_ORIGIN1].field);
		DELTA_SETBYINDEX(pFields, entity_field_alias[FIELD_ORIGIN2].field);
	}
}

#define CUSTOMFIELD_ORIGIN0  0
#define CUSTOMFIELD_ORIGIN1  1
#define CUSTOMFIELD_ORIGIN2  2
#define CUSTOMFIELD_ANGLES0  3
#define CUSTOMFIELD_ANGLES1  4
#define CUSTOMFIELD_ANGLES2  5
#define CUSTOMFIELD_SKIN     6
#define CUSTOMFIELD_SEQUENCE 7
#define CUSTOMFIELD_ANIMTIME 8

entity_field_alias_t custom_entity_field_alias[] =
{
	{"origin[0]", 0},
	{"origin[1]", 0},
	{"origin[2]", 0},
	{"angles[0]", 0},
	{"angles[1]", 0},
	{"angles[2]", 0},
	{"skin", 0},
	{"sequence", 0},
	{"animtime", 0},
};

void Custom_Entity_FieldInit(struct delta_s* pFields)
{
	custom_entity_field_alias[CUSTOMFIELD_ORIGIN0].field = DELTA_FINDFIELD(pFields, custom_entity_field_alias[CUSTOMFIELD_ORIGIN0].name);
	custom_entity_field_alias[CUSTOMFIELD_ORIGIN1].field = DELTA_FINDFIELD(pFields, custom_entity_field_alias[CUSTOMFIELD_ORIGIN1].name);
	custom_entity_field_alias[CUSTOMFIELD_ORIGIN2].field = DELTA_FINDFIELD(pFields, custom_entity_field_alias[CUSTOMFIELD_ORIGIN2].name);
	custom_entity_field_alias[CUSTOMFIELD_ANGLES0].field = DELTA_FINDFIELD(pFields, custom_entity_field_alias[CUSTOMFIELD_ANGLES0].name);
	custom_entity_field_alias[CUSTOMFIELD_ANGLES1].field = DELTA_FINDFIELD(pFields, custom_entity_field_alias[CUSTOMFIELD_ANGLES1].name);
	custom_entity_field_alias[CUSTOMFIELD_ANGLES2].field = DELTA_FINDFIELD(pFields, custom_entity_field_alias[CUSTOMFIELD_ANGLES2].name);
	custom_entity_field_alias[CUSTOMFIELD_SKIN].field = DELTA_FINDFIELD(pFields, custom_entity_field_alias[CUSTOMFIELD_SKIN].name);
	custom_entity_field_alias[CUSTOMFIELD_SEQUENCE].field = DELTA_FINDFIELD(pFields, custom_entity_field_alias[CUSTOMFIELD_SEQUENCE].name);
	custom_entity_field_alias[CUSTOMFIELD_ANIMTIME].field = DELTA_FINDFIELD(pFields, custom_entity_field_alias[CUSTOMFIELD_ANIMTIME].name);
}

/*
==================
Custom_Encode

Callback for sending entity_state_t info ( for custom entities ) over network.
FIXME:  Move to script
==================
*/
void Custom_Encode(struct delta_s* pFields, const unsigned char* from, const unsigned char* to)
{
	static int initialized = 0;
	if (!initialized)
	{
		Custom_Entity_FieldInit(pFields);
		initialized = 1;
	}

	const entity_state_t* f = reinterpret_cast<const entity_state_t*>(from);
	const entity_state_t* t = reinterpret_cast<const entity_state_t*>(to);

	int beamType = t->rendermode & 0x0f;

	if (beamType != BEAM_POINTS && beamType != BEAM_ENTPOINT)
	{
		DELTA_UNSETBYINDEX(pFields, custom_entity_field_alias[CUSTOMFIELD_ORIGIN0].field);
		DELTA_UNSETBYINDEX(pFields, custom_entity_field_alias[CUSTOMFIELD_ORIGIN1].field);
		DELTA_UNSETBYINDEX(pFields, custom_entity_field_alias[CUSTOMFIELD_ORIGIN2].field);
	}

	if (beamType != BEAM_POINTS)
	{
		DELTA_UNSETBYINDEX(pFields, custom_entity_field_alias[CUSTOMFIELD_ANGLES0].field);
		DELTA_UNSETBYINDEX(pFields, custom_entity_field_alias[CUSTOMFIELD_ANGLES1].field);
		DELTA_UNSETBYINDEX(pFields, custom_entity_field_alias[CUSTOMFIELD_ANGLES2].field);
	}

	if (beamType != BEAM_ENTS && beamType != BEAM_ENTPOINT)
	{
		DELTA_UNSETBYINDEX(pFields, custom_entity_field_alias[CUSTOMFIELD_SKIN].field);
		DELTA_UNSETBYINDEX(pFields, custom_entity_field_alias[CUSTOMFIELD_SEQUENCE].field);
	}

	// animtime is compared by rounding first
	// see if we really shouldn't actually send it
	if ((int)f->animtime == (int)t->animtime)
	{
		DELTA_UNSETBYINDEX(pFields, custom_entity_field_alias[CUSTOMFIELD_ANIMTIME].field);
	}
}

/*
=================
RegisterEncoders

Allows game .dll to override network encoding of certain types of entities and tweak values, etc.
=================
*/
void RegisterEncoders(void)
{
	DELTA_ADDENCODER("Entity_Encode", Entity_Encode);
	DELTA_ADDENCODER("Custom_Encode", Custom_Encode);
	DELTA_ADDENCODER("Player_Encode", Player_Encode);
}

int GetWeaponData(struct edict_s* player, struct weapon_data_s* info)
{
	memset(info, 0, 32 * sizeof(weapon_data_t));

#if defined(CLIENT_WEAPONS)
	if (FNullEnt(player))
		return 1;

	CBasePlayer* pl = CBaseEntity::SafeInstance<CBasePlayer>(player);

	if (!pl)
		return 1;

	// go through all of the weapons and make a list of the ones to pack
	for (int i = 0; i < MAX_ITEM_TYPES; i++)
	{
		if (pl->m_rgpPlayerItems[i])
		{
			// there's a weapon here. Should I pack it?
			EHANDLE<CBasePlayerItem> pPlayerItem = pl->m_rgpPlayerItems[i];

			while (pPlayerItem)
			{
				EHANDLE<CBasePlayerWeapon> gun = pPlayerItem;
				if (gun && gun->UseDecrement())
				{
					// Get The ID.
					ItemInfo II;
					memset(&II, 0, sizeof(II));
					gun->GetItemInfo(&II);

					if (II.iId >= 0 && II.iId < 32)
					{
						weapon_data_t& item = info[II.iId];

						item.m_iId = II.iId;
						item.m_iClip = gun->m_iClip;

						//item->m_flTimeWeaponIdle		= (std::max)( gun->m_flTimeWeaponIdle.GetRaw(), -0.001f );
						//item->m_flNextPrimaryAttack		= (std::max)( gun->m_flNextPrimaryAttack.GetRaw(), -0.001f );
						//item->m_flNextSecondaryAttack	= (std::max)( gun->m_flNextSecondaryAttack.GetRaw(), -0.001f );
						item.m_flTimeWeaponIdle = (std::max)(gun->m_flTimeWeaponIdle, -0.001f);
						item.m_flNextPrimaryAttack = (std::max)(gun->m_flNextPrimaryAttack, -0.001f);
						item.m_flNextSecondaryAttack = (std::max)(gun->m_flNextSecondaryAttack, -0.001f);
						item.m_fInReload = gun->m_fInReload;
						item.m_fInSpecialReload = gun->m_fInSpecialReload;
						item.fuser1 = (std::max)(gun->pev->fuser1, -0.001f);
						item.fuser2 = gun->m_flStartThrow;
						item.fuser3 = gun->m_flReleaseThrow;
						item.iuser1 = gun->m_chargeReady;
						item.iuser2 = gun->m_fInAttack;
						item.iuser3 = gun->m_fireState;

						//						item->m_flPumpTime				= max( gun->m_flPumpTime, -0.001 );
					}
				}
				pPlayerItem = pPlayerItem->m_pNext;
			}
		}
	}
#endif
	return 1;
}

/*
=================
UpdateClientData

Data sent to current client only
engine sets cd to 0 before calling.
=================
*/

void UpdateClientData(const struct edict_s* ent, int sendweapons, struct clientdata_s* cd)
{
	if (FNullEnt(ent))
		return;

	cd->flags = ent->v.flags;
	cd->health = ent->v.health;

	cd->viewmodel = MODEL_INDEX(STRING(ent->v.viewmodel));

	cd->waterlevel = ent->v.waterlevel;
	cd->watertype = ent->v.watertype;
	cd->weapons = ent->v.weapons;

	CBasePlayer* pl = CBaseEntity::SafeInstance<CBasePlayer>(ent);

	// Vectors
	if (pl != nullptr)
	{
		cd->origin = pl->GetClientOrigin();
		cd->view_ofs = pl->GetClientViewOfs();
	}
	else
	{
		cd->origin = ent->v.origin;
		cd->view_ofs = ent->v.view_ofs;
	}
	cd->velocity = ent->v.velocity;
	cd->punchangle = ent->v.punchangle;

	cd->bInDuck = ent->v.bInDuck;
	cd->flTimeStepSound = ent->v.flTimeStepSound;
	cd->flDuckTime = ent->v.flDuckTime;
	cd->flSwimTime = ent->v.flSwimTime;
	cd->waterjumptime = ent->v.teleport_time;

	strcpy_s(cd->physinfo, ENGINE_GETPHYSINFO(ent));

	cd->maxspeed = ent->v.maxspeed;
	cd->fov = ent->v.fov;
	cd->weaponanim = ent->v.weaponanim;

	cd->pushmsec = ent->v.pushmsec;

#if defined(CLIENT_WEAPONS)
	if (sendweapons)
	{
		if (pl)
		{
			cd->m_flNextAttack = pl->m_flNextAttack;
			cd->fuser2 = pl->m_flNextAmmoBurn;
			cd->fuser3 = pl->m_flAmmoStartCharge;
			cd->vuser1.x = pl->ammo_9mm;
			cd->vuser1.y = pl->ammo_357;
			cd->vuser1.z = pl->ammo_argrens;
			cd->ammo_nails = pl->ammo_bolts;
			cd->ammo_shells = pl->ammo_buckshot;
			cd->ammo_rockets = pl->ammo_rockets;
			cd->ammo_cells = pl->ammo_uranium;
			cd->vuser2.x = pl->ammo_hornets;

			if (pl->m_pActiveItem)
			{
				EHANDLE<CBasePlayerWeapon> gun = pl->m_pActiveItem;
				if (gun && gun->UseDecrement())
				{
					ItemInfo II;
					memset(&II, 0, sizeof(II));
					gun->GetItemInfo(&II);

					cd->m_iId = II.iId;

					cd->vuser3.z = gun->m_iSecondaryAmmoType;
					cd->vuser4.x = gun->m_iPrimaryAmmoType;
					cd->vuser4.y = pl->m_rgAmmo[gun->m_iPrimaryAmmoType];
					cd->vuser4.z = pl->m_rgAmmo[gun->m_iSecondaryAmmoType];

					if (pl->m_pActiveItem->m_iId == WEAPON_RPG)
					{
						EHANDLE<CRpg> rpg = pl->m_pActiveItem;
						if (rpg)
						{
							cd->vuser2.y = rpg->m_fSpotActive;
							cd->vuser2.z = rpg->m_cActiveRockets;
						}
					}
				}
			}
		}
	}
#endif
}

/*
=================
CmdStart

We're about to run this usercmd for the specified player.  We can set up groupinfo and masking here, etc.
This is the time to examine the usercmd for anything extra.  This call happens even if think does not.
=================
*/
void CmdStart(const edict_t* player, const struct usercmd_s* cmd, unsigned int random_seed)
{
	if (FNullEnt(player))
		return;

	CBasePlayer* pl = CBaseEntity::SafeInstance<CBasePlayer>(player);

	if (!pl)
		return;

	if (pl->pev->groupinfo != 0)
	{
		UTIL_SetGroupTrace(pl->pev->groupinfo, GROUP_OP_AND);
	}

	pl->random_seed = random_seed;
}

/*
=================
CmdEnd

Each cmdstart is exactly matched with a cmd end, clean up any group trace flags, etc. here
=================
*/
void CmdEnd(const edict_t* player)
{
	if (FNullEnt(player))
		return;

	CBasePlayer* pl = CBaseEntity::SafeInstance<CBasePlayer>(player);

	if (!pl)
		return;
	if (pl->pev->groupinfo != 0)
	{
		UTIL_UnsetGroupTrace();
	}
}

/*
================================
ConnectionlessPacket

 Return 1 if the packet is valid.  Set response_buffer_size if you want to send a response packet.  Incoming, it holds the max
  size of the response_buffer, so you must zero it out if you choose not to respond.
================================
*/
int ConnectionlessPacket(const struct netadr_s* net_from, const char* args, char* response_buffer, int* response_buffer_size)
{
	// Parse stuff from args
	int max_buffer_size = *response_buffer_size;

	// Zero it out since we aren't going to respond.
	// If we wanted to response, we'd write data into response_buffer
	*response_buffer_size = 0;

	// Since we don't listen for anything here, just respond that it's a bogus message
	// If we didn't reject the message, we'd return 1 for success instead.
	return 0;
}



/*
================================
GetHullBounds

  Engine calls this to enumerate player collision hulls, for prediction.  Return 0 if the hullnumber doesn't exist.
================================
*/

float* g_pEngineHullMins[4]{ nullptr };
float* g_pEngineHullMaxs[4]{ nullptr };

int InternalGetHullBounds(int hullnumber, float* mins, float* maxs, bool calledFromPMInit)
{
	if (!calledFromPMInit)
	{
		g_pEngineHullMins[hullnumber] = mins;
		g_pEngineHullMaxs[hullnumber] = maxs;
	}

	switch (hullnumber)
	{
	case 0:  // Normal player
		VEC_HULL_MIN.CopyToArray(mins);
		VEC_HULL_MAX.CopyToArray(maxs);
		return 1;
	case 1:  // Crouched player
		VEC_DUCK_HULL_MIN.CopyToArray(mins);
		VEC_DUCK_HULL_MAX.CopyToArray(maxs);
		return 1;
	case 2:  // Point based hull
		Vector{}.CopyToArray(mins);
		Vector{}.CopyToArray(maxs);
		return 1;
	}

	return 0;
}

int GetHullBounds(int hullnumber, float* mins, float* maxs)
{
	return InternalGetHullBounds(hullnumber, mins, maxs, false);
}

/*
================================
CreateInstancedBaselines

Create pseudo-baselines for items that aren't placed in the map at spawn time, but which are likely
to be created during play ( e.g., grenades, ammo packs, projectiles, corpses, etc. )
================================
*/
void CreateInstancedBaselines(void)
{
}

/*
================================
InconsistentFile

One of the ENGINE_FORCE_UNMODIFIED files failed the consistency check for the specified player
 Return 0 to allow the client to continue, 1 to force immediate disconnection ( with an optional disconnect message of up to 256 characters )
================================
*/
int InconsistentFile(const edict_t* player, const char* filename, char* disconnect_message)
{
	// Server doesn't care?
	if (CVAR_GET_FLOAT("mp_consistency") != 1)
		return 0;

	// Default behavior is to kick the player
	sprintf_s(disconnect_message, 256, "Server is enforcing file consistency for %s\n", filename);

	// Kick now with specified disconnect message.
	return 1;
}

/*
================================
AllowLagCompensation

 The game .dll should return 1 if lag compensation should be allowed ( could also just set
  the sv_unlag cvar.
 Most games right now should return 0, until client-side weapon prediction code is written
  and tested for them ( note you can predict weapons, but not do lag compensation, too,
  if you want.
================================
*/
int AllowLagCompensation(void)
{
	return 1;
}
