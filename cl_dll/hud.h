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
//  hud.h
//
// class CHud declaration
//
// CHud handles the message, calculation, and drawing the HUD
//


#define RGB_YELLOWISH 0x00FFA000  //255,160,0
#define RGB_REDISH    0x00FF1010  //255,160,0
#define RGB_GREENISH  0x0000A000  //0,160,0

#include "wrect.h"
#include "cl_dll.h"
#include "ammo.h"

#define DHN_DRAWZERO 1
#define DHN_2DIGITS  2
#define DHN_3DIGITS  4
#define MIN_ALPHA    100

#define HUDELEM_ACTIVE 1

typedef struct
{
	int x = 0, y = 0;
} POSITION;

enum
{
	MAX_PLAYERS = 64,
	MAX_TEAMS = 64,
	MAX_TEAM_NAME = 16,
};

typedef struct
{
	unsigned char r = 0, g = 0, b = 0, a = 0;
} RGBA;

typedef struct cvar_s cvar_t;


#define HUD_ACTIVE       1
#define HUD_INTERMISSION 2

#define MAX_PLAYER_NAME_LENGTH 32

#define MAX_MOTD_LENGTH 1536

//
//-----------------------------------------------------
//
class CHudBase
{
public:
	POSITION m_pos;
	int m_type = 0;
	int m_iFlags = 0;  // active, moving,
	virtual ~CHudBase() {}
	virtual int Init(void) { return 0; }
	virtual int VidInit(void) { return 0; }
	virtual int Draw(float flTime) { return 0; }
	virtual void Think(void) { return; }
	virtual void Reset(void) { return; }
	virtual void InitHUDData(void) {}  // called every time a server is connected to
};

struct HUDLIST
{
	CHudBase* p{ nullptr };
	HUDLIST* pNext{ nullptr };
};



//
//-----------------------------------------------------
//
#include "..\game_shared\voice_status.h"
#include "hud_spectator.h"


//
//-----------------------------------------------------
//
class CHudAmmo : public CHudBase
{
public:
	int Init(void);
	int VidInit(void);
	int Draw(float flTime);
	void Think(void);
	void Reset(void);
	int DrawWList(float flTime);
	int MsgFunc_CurWeapon(const char* pszName, int iSize, void* pbuf);
	int MsgFunc_WeaponList(const char* pszName, int iSize, void* pbuf);
	int MsgFunc_AmmoX(const char* pszName, int iSize, void* pbuf);
	int MsgFunc_AmmoPickup(const char* pszName, int iSize, void* pbuf);
	int MsgFunc_WeapPickup(const char* pszName, int iSize, void* pbuf);
	int MsgFunc_ItemPickup(const char* pszName, int iSize, void* pbuf);
	int MsgFunc_HideWeapon(const char* pszName, int iSize, void* pbuf);

	void SlotInput(int iSlot);
	void _cdecl UserCmd_Slot1(void);
	void _cdecl UserCmd_Slot2(void);
	void _cdecl UserCmd_Slot3(void);
	void _cdecl UserCmd_Slot4(void);
	void _cdecl UserCmd_Slot5(void);
	void _cdecl UserCmd_Slot6(void);
	void _cdecl UserCmd_Slot7(void);
	void _cdecl UserCmd_Slot8(void);
	void _cdecl UserCmd_Slot9(void);
	void _cdecl UserCmd_Slot10(void);
	void _cdecl UserCmd_Close(void);
	void _cdecl UserCmd_NextWeapon(void);
	void _cdecl UserCmd_PrevWeapon(void);

	int FirstSlotWithWeapon();
	int FirstSlotPosWithWeapon(int slot);
	int LastSlotWithWeapon();
	int LastSlotPosWithWeapon(int slot);

	bool IsCurrentWeaponFirstWeapon();
	bool IsCurrentWeaponLastWeapon();

	WEAPON* m_pWeapon = nullptr;

private:
	float m_fFade = 0.f;
	RGBA m_rgba;
	int m_HUD_bucket0 = 0;
	int m_HUD_selection = 0;
};

//
//-----------------------------------------------------
//

class CHudAmmoSecondary : public CHudBase
{
public:
	int Init(void);
	int VidInit(void);
	void Reset(void);
	int Draw(float flTime);

	int MsgFunc_SecAmmoVal(const char* pszName, int iSize, void* pbuf);
	int MsgFunc_SecAmmoIcon(const char* pszName, int iSize, void* pbuf);

private:
	enum
	{
		MAX_SEC_AMMO_VALUES = 4
	};

	int m_HUD_ammoicon = 0;  // sprite indices
	int m_iAmmoAmounts[MAX_SEC_AMMO_VALUES] = { 0 };
	float m_fFade = 0.f;
};


#include "health.h"


#define FADE_TIME 100


//
//-----------------------------------------------------
//
class CHudGeiger : public CHudBase
{
public:
	int Init(void);
	int VidInit(void);
	int Draw(float flTime);
	int MsgFunc_Geiger(const char* pszName, int iSize, void* pbuf);

private:
	int m_iGeigerRange = 0;
};

//
//-----------------------------------------------------
//
class CHudTrain : public CHudBase
{
public:
	int Init(void);
	int VidInit(void);
	int Draw(float flTime);
	int MsgFunc_Train(const char* pszName, int iSize, void* pbuf);

	int m_iPos{ 0 };

private:
	HSPRITE_VALVE m_hSprite = 0;
};

//
//-----------------------------------------------------
//
// REMOVED: Vgui has replaced this.
//
/*
class CHudMOTD : public CHudBase
{
public:
	int Init( void );
	int VidInit( void );
	int Draw( float flTime );
	void Reset( void );

	int MsgFunc_MOTD( const char *pszName, int iSize, void *pbuf );

protected:
	static int MOTD_DISPLAY_TIME = 0;
	char m_szMOTD[ MAX_MOTD_LENGTH ];
	float m_flActiveRemaining = 0.f;
	int m_iLines = 0;
};
*/

//
//-----------------------------------------------------
//
class CHudStatusBar : public CHudBase
{
public:
	int Init(void);
	int VidInit(void);
	int Draw(float flTime);
	void Reset(void);
	void ParseStatusString(int line_num);

	int MsgFunc_StatusText(const char* pszName, int iSize, void* pbuf);
	int MsgFunc_StatusValue(const char* pszName, int iSize, void* pbuf);

protected:
	enum
	{
		MAX_STATUSTEXT_LENGTH = 128,
		MAX_STATUSBAR_VALUES = 8,
		MAX_STATUSBAR_LINES = 2,
	};

	char m_szStatusText[MAX_STATUSBAR_LINES][MAX_STATUSTEXT_LENGTH] = { 0 };  // a text string describing how the status bar is to be drawn
	char m_szStatusBar[MAX_STATUSBAR_LINES][MAX_STATUSTEXT_LENGTH] = { 0 };   // the constructed bar that is drawn
	int m_iStatusValues[MAX_STATUSBAR_VALUES] = { 0 };                        // an array of values for use in the status bar

	int m_bReparseString = 0;  // set to TRUE whenever the m_szStatusBar needs to be recalculated

	// an array of colors...one color for each line
	float* m_pflNameColors[MAX_STATUSBAR_LINES] = { 0 };
};

//
//-----------------------------------------------------
//
// REMOVED: Vgui has replaced this.
//
/*
class CHudScoreboard: public CHudBase
{
public:
	int Init( void );
	void InitHUDData( void );
	int VidInit( void );
	int Draw( float flTime );
	int DrawPlayers( int xoffset, float listslot, int nameoffset = 0, char *team = nullptr ); // returns the ypos where it finishes drawing
	void UserCmd_ShowScores( void );
	void UserCmd_HideScores( void );
	int MsgFunc_ScoreInfo( const char *pszName, int iSize, void *pbuf );
	int MsgFunc_TeamInfo( const char *pszName, int iSize, void *pbuf );
	int MsgFunc_TeamScore( const char *pszName, int iSize, void *pbuf );
	void DeathMsg( int killer, int victim );

	int m_iNumTeams = 0;

	int m_iLastKilledBy = 0;
	int m_fLastKillTime = 0;
	int m_iPlayerNum = 0;
	int m_iShowscoresHeld = 0;

	void GetAllPlayersInfo( void );
private:
	struct cvar_s *cl_showpacketloss;

};
*/

struct extra_player_info_t
{
	short frags = 0;
	short deaths = 0;
	short playerclass = 0;
	short teamnumber = 0;
	char teamname[MAX_TEAM_NAME] = { 0 };
};

struct team_info_t
{
	char name[MAX_TEAM_NAME] = { 0 };
	short frags = 0;
	short deaths = 0;
	short ping = 0;
	short packetloss = 0;
	short ownteam = 0;
	short players = 0;
	int already_drawn = 0;
	int scores_overriden = 0;
	int teamnumber = 0;
};

extern hud_player_info_t g_PlayerInfoList[MAX_PLAYERS + 1];     // player info from the engine
extern extra_player_info_t g_PlayerExtraInfo[MAX_PLAYERS + 1];  // additional player info sent directly to the client dll
extern team_info_t g_TeamInfo[MAX_TEAMS + 1];
extern int g_IsSpectator[MAX_PLAYERS + 1];


//
//-----------------------------------------------------
//
class CHudDeathNotice : public CHudBase
{
public:
	int Init(void);
	void InitHUDData(void);
	int VidInit(void);
	int Draw(float flTime);
	int MsgFunc_DeathMsg(const char* pszName, int iSize, void* pbuf);

private:
	int m_HUD_d_skull = 0;  // sprite index of skull icon
};

//
//-----------------------------------------------------
//
class CHudMenu : public CHudBase
{
public:
	int Init(void);
	void InitHUDData(void);
	int VidInit(void);
	void Reset(void);
	int Draw(float flTime);
	int MsgFunc_ShowMenu(const char* pszName, int iSize, void* pbuf);

	void SelectMenuItem(int menu_item);

	int m_fMenuDisplayed = 0;
	int m_bitsValidSlots = 0;
	float m_flShutoffTime = 0.f;
	int m_fWaitingForMore = 0;
};

//
//-----------------------------------------------------
//
class CHudSayText : public CHudBase
{
public:
	int Init(void);
	void InitHUDData(void);
	int VidInit(void);
	int Draw(float flTime);
	int MsgFunc_SayText(const char* pszName, int iSize, void* pbuf);
	void SayTextPrint(const char* pszBuf, int iBufSize, int clientIndex = -1);
	void EnsureTextFitsInOneLineAndWrapIfHaveTo(int line);
	friend class CHudSpectator;

private:
	struct cvar_s* m_HUD_saytext = nullptr;
	struct cvar_s* m_HUD_saytext_time = nullptr;
};

//
//-----------------------------------------------------
//
class CHudBattery : public CHudBase
{
public:
	int Init(void);
	int VidInit(void);
	int Draw(float flTime);
	int MsgFunc_Battery(const char* pszName, int iSize, void* pbuf);

private:
	HSPRITE_VALVE m_hSprite1 = 0;
	HSPRITE_VALVE m_hSprite2 = 0;
	wrect_t* m_prc1 = nullptr;
	wrect_t* m_prc2 = nullptr;
	int m_iBat = 0;
	float m_fFade = 0.f;
	int m_iHeight = 0;  // width of the battery innards
};


//
//-----------------------------------------------------
//
class CHudFlashlight : public CHudBase
{
public:
	int Init(void);
	int VidInit(void);
	int Draw(float flTime);
	void Reset(void);
	int MsgFunc_Flashlight(const char* pszName, int iSize, void* pbuf);
	int MsgFunc_FlashBat(const char* pszName, int iSize, void* pbuf);
	bool IsOn() const { return m_fOn != 0; }

private:
	HSPRITE_VALVE m_hSprite1 = 0;
	HSPRITE_VALVE m_hSprite2 = 0;
	HSPRITE_VALVE m_hBeam = 0;
	wrect_t* m_prc1 = nullptr;
	wrect_t* m_prc2 = nullptr;
	wrect_t* m_prcBeam = nullptr;
	float m_flBat = 0.f;
	int m_iBat = 0;
	int m_fOn = 0;
	float m_fFade = 0.f;
	int m_iWidth = 0;  // width of the battery innards
};

//
//-----------------------------------------------------
//
const int maxHUDMessages = 16;
struct message_parms_t
{
	client_textmessage_t* pMessage = nullptr;
	float time = 0.f;
	int x = 0, y = 0;
	int totalWidth = 0, totalHeight = 0;
	int width = 0;
	int lines = 0;
	int lineLength = 0;
	int length = 0;
	int r = 0, g = 0, b = 0;
	int text = 0;
	int fadeBlend = 0;
	float charTime = 0.f;
	float fadeTime = 0.f;
};

//
//-----------------------------------------------------
//

class CHudTextMessage : public CHudBase
{
public:
	int Init(void);
	static char* LocaliseTextString(const char* msg, char* dst_buffer, int buffer_size);
	static char* BufferedLocaliseTextString(const char* msg);
	const char* LookupString(const char* msg_name, int* msg_dest = nullptr);
	int MsgFunc_TextMsg(const char* pszName, int iSize, void* pbuf);
};

//
//-----------------------------------------------------
//

class CHudMessage : public CHudBase
{
public:
	int Init(void);
	int VidInit(void);
	int Draw(float flTime);
	int MsgFunc_HudText(const char* pszName, int iSize, void* pbuf);
	int MsgFunc_GameTitle(const char* pszName, int iSize, void* pbuf);

	float FadeBlend(float fadein, float fadeout, float hold, float localTime);
	int XPosition(float x, int width, int lineWidth);
	int YPosition(float y, int height);

	void MessageAdd(const char* pName, float time);
	void MessageAdd(client_textmessage_t* newMessage);
	void MessageDrawScan(client_textmessage_t* pMessage, float time);
	void MessageScanStart(void);
	void MessageScanNextChar(void);
	void Reset(void);

private:
	client_textmessage_t* m_pMessages[maxHUDMessages] = { 0 };
	float m_startTime[maxHUDMessages] = { 0 };
	message_parms_t m_parms;
	float m_gameTitleTime = 0.f;
	client_textmessage_t* m_pGameTitle = nullptr;

	int m_HUD_title_life = 0;
	int m_HUD_title_half = 0;
};

//
//-----------------------------------------------------
//
#define MAX_SPRITE_NAME_LENGTH 24

class CHudStatusIcons : public CHudBase
{
public:
	int Init(void);
	int VidInit(void);
	void Reset(void);
	int Draw(float flTime);
	int MsgFunc_StatusIcon(const char* pszName, int iSize, void* pbuf);

	enum
	{
		MAX_ICONSPRITENAME_LENGTH = MAX_SPRITE_NAME_LENGTH,
		MAX_ICONSPRITES = 4,
	};


	//had to make these public so CHud could access them (to enable concussion icon)
	//could use a friend declaration instead...
	void EnableIcon(char* pszIconName, unsigned char red, unsigned char green, unsigned char blue);
	void DisableIcon(char* pszIconName);

private:
	typedef struct
	{
		char szSpriteName[MAX_ICONSPRITENAME_LENGTH] = { 0 };
		HSPRITE_VALVE spr = 0;
		wrect_t rc;
		unsigned char r = 0, g = 0, b = 0;
	} icon_sprite_t;

	icon_sprite_t m_IconList[MAX_ICONSPRITES] = { 0 };
};

//
//-----------------------------------------------------
//


// For precise position when rendering
struct ControllerModelData
{
	struct ModelData
	{
		int body{ 0 };
		int skin{ 0 };
		float scale{ 1.f };
		int sequence{ 0 };
		float frame{ 0.f };
		float framerate{ 0.f };
		float animtime{ 0.f };
		int effects{ 0 };
		int rendermode{ 0 };
		int renderamt{ 0 };
		int renderfx{ 0 };
		color24 rendercolor{ 0, 0, 0 };
		char modelname[1024];
	};

	ModelData controller;

	bool hasDraggedEnt{ false };
	int draggedEntIndex{ 0 };
	Vector draggedEntOriginOffset;
	Vector draggedEntAnglesOffset;
	ModelData draggedEnt;
};


class CHud
{
private:
	HUDLIST* m_pHudList = nullptr;
	HSPRITE_VALVE m_hsprLogo = 0;
	int m_iLogo = 0;
	client_sprite_t* m_pSpriteList = nullptr;
	int m_iSpriteCount = 0;
	int m_iSpriteCountAllRes = 0;
	int m_iConcussionEffect = 0;

	int m_iGroundEntIndex{ 0 };
	Vector m_trainControlPosition;
	float m_trainControlYaw{ 0.f };

public:
	ControllerModelData m_leftControllerModelData{ 0 };
	ControllerModelData m_rightControllerModelData{ 0 };

	int m_vrGrabbedLadderEntIndex{ -1 };
	bool m_vrIsPullingOnLedge{ false };

	float m_screenShakeAmplitude = 0;
	float m_screenShakeDuration = 0;
	float m_screenShakeFrequency = 0;
	bool m_hasScreenShake = false;

	float m_vrLeftHandTouchVibrateIntensity = 0.f;
	float m_vrRightHandTouchVibrateIntensity = 0.f;

	HSPRITE_VALVE m_hsprCursor = 0;
	float m_flHUDDrawTime = 0.f;        // the current HUD draw time
	float m_fOldHUDDrawTime = 0.f;      // the time at which the HUD was last redrawn
	float m_flHUDDrawTimeDelta = 0.f;  // the difference between flTime and fOldTime
	Vector m_vecOrigin;
	Vector m_vecAngles;
	int m_iKeyBits = 0;
	int m_iHideHUDDisplay = 0;
	int m_iFOV = 0;
	int m_Teamplay = 0;
	int m_iRes = 0;
	cvar_t* m_pCvarStealMouse = nullptr;
	cvar_t* m_pCvarDraw = nullptr;

	int m_iFontHeight = 0;
	int DrawHudNumber(int x, int y, int iFlags, int iNumber, int r, int g, int b);
	int DrawHudString(int x, int y, int iMaxX, char* szString, int r, int g, int b);
	int DrawHudStringReverse(int xpos, int ypos, int iMinX, char* szString, int r, int g, int b);
	int DrawHudNumberString(int xpos, int ypos, int iMinX, int iNumber, int r, int g, int b);
	int GetNumWidth(int iNumber, int iFlags);

private:
	// the memory for these arrays are allocated in the first call to CHud::VidInit(), when the hud.txt and associated sprites are loaded.
	// freed in ~CHud()
	HSPRITE_VALVE* m_rghSprites = nullptr; /*[HUD_SPRITE_COUNT]*/  // the sprites loaded from hud.txt
	wrect_t* m_rgrcRects = nullptr;                                /*[HUD_SPRITE_COUNT]*/
	char* m_rgszSpriteNames = nullptr;                             /*[HUD_SPRITE_COUNT][MAX_SPRITE_NAME_LENGTH]*/

	cvar_t* default_fov = nullptr;

public:
	HSPRITE_VALVE GetSprite(int index)
	{
		return (index < 0) ? 0 : m_rghSprites[index];
	}

	wrect_t& GetSpriteRect(int index)
	{
		return m_rgrcRects[index];
	}


	int GetSpriteIndex(const char* SpriteName);  // gets a sprite index, for use in the m_rghSprites[] array

	CHudAmmo m_Ammo;
	CHudHealth m_Health;
	CHudSpectator m_Spectator;
	CHudGeiger m_Geiger;
	CHudBattery m_Battery;
	CHudTrain m_Train;
	CHudFlashlight m_Flash;
	CHudMessage m_Message;
	CHudStatusBar m_StatusBar;
	CHudDeathNotice m_DeathNotice;
	CHudSayText m_SayText;
	CHudMenu m_Menu;
	CHudAmmoSecondary m_AmmoSecondary;
	CHudTextMessage m_TextMessage;
	CHudStatusIcons m_StatusIcons;

	void Init(void);
	void VidInit(void);
	void Think(void);
	int Redraw(float flTime, int intermission);
	int UpdateClientData(client_data_t* cdata, float time);

	CHud() :
		m_iSpriteCount(0), m_pHudList(nullptr) {}
	~CHud();  // destructor, frees allocated memory

	// user messages
	int _cdecl MsgFunc_Damage(const char* pszName, int iSize, void* pbuf);
	int _cdecl MsgFunc_GameMode(const char* pszName, int iSize, void* pbuf);
	int _cdecl MsgFunc_Logo(const char* pszName, int iSize, void* pbuf);
	int _cdecl MsgFunc_ResetHUD(const char* pszName, int iSize, void* pbuf);
	void _cdecl MsgFunc_InitHUD(const char* pszName, int iSize, void* pbuf);
	void _cdecl MsgFunc_ViewMode(const char* pszName, int iSize, void* pbuf);
	int _cdecl MsgFunc_SetFOV(const char* pszName, int iSize, void* pbuf);
	int _cdecl MsgFunc_Concuss(const char* pszName, int iSize, void* pbuf);
	int _cdecl MsgFunc_GroundEnt(const char* pszName, int iSize, void* pbuf);
	int _cdecl MsgFunc_VRCtrlEnt(const char* pszName, int iSize, void* pbuf);
	int _cdecl MsgFunc_TrainCtrl(const char* pszName, int iSize, void* pbuf);
	int _cdecl MsgFunc_VRScrnShke(const char* pszName, int iSize, void* pbuf);
	int _cdecl MsgFunc_GrbdLddr(const char* pszName, int iSize, void* pbuf);
	int _cdecl MsgFunc_PullLdg(const char* pszName, int iSize, void* pbuf);
	int _cdecl MsgFunc_VRTouch(const char* pszName, int iSize, void* pbuf);

	// Screen information
	SCREENINFO m_scrinfo;

	int m_iWeaponBits = 0;
	int m_fPlayerDead = 0;
	int m_iIntermission = 0;

	// sprite indexes
	int m_HUD_number_0 = 0;

	void AddHudElem(CHudBase* p);

	// Ground entity for rotating with them in VR (since player rotation is done client side completely) - Max Vollmer, 2019-04-09
	struct cl_entity_s* GetGroundEntity();

	// HUD (train control sprites) attachment for train we are controlling
	bool GetTrainControlsOriginAndOrientation(Vector& origin, Vector& angles);
};

class TeamFortressViewport;

extern CHud gHUD;
extern TeamFortressViewport* gViewPort;

extern int g_iPlayerClass;
extern int g_iTeamNumber;
extern int g_iUser1;
extern int g_iUser2;
extern int g_iUser3;
