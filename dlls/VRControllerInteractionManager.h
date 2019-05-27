#pragma once

class CBasePlayer;
class CBaseMonster;
class VRController;

class VRControllerInteractionManager
{
public:

	void CheckAndPressButtons(CBasePlayer *pPlayer, const VRController& controller);

private:
	float DoDamage(CBasePlayer *pPlayer, EHANDLE hEntity, const VRController& controller);
	bool HandleEasterEgg(CBasePlayer *pPlayer, EHANDLE hEntity, const VRController& controller, bool isTouching, bool didTouchChange);
	bool HandleRetinaScanners(CBasePlayer *pPlayer, EHANDLE hEntity, const VRController& controller, bool isTouching, bool didTouchChange);
	bool HandleButtons(CBasePlayer *pPlayer, EHANDLE hEntity, const VRController& controller, bool isTouching, bool didTouchChange, bool isDragging, bool didDragChange);
	bool HandleDoors(CBasePlayer *pPlayer, EHANDLE hEntity, const VRController& controller, bool isTouching, bool didTouchChange);
	bool HandleRechargers(CBasePlayer *pPlayer, EHANDLE hEntity, const VRController& controller, bool isTouching, bool didTouchChange);
	bool HandleTriggers(CBasePlayer *pPlayer, EHANDLE hEntity, const VRController& controller, bool isTouching, bool didTouchChange);
	bool HandleBreakables(CBasePlayer *pPlayer, EHANDLE hEntity, const VRController& controller, bool isTouching, bool didTouchChange);
	bool HandlePushables(CBasePlayer *pPlayer, EHANDLE hEntity, const VRController& controller, bool isTouching, bool didTouchChange);
	bool HandleAlliedMonsters(CBasePlayer *pPlayer, EHANDLE hEntity, const VRController& controller, bool isTouching, bool didTouchChange, bool isHitting, bool didHitChange, float flHitDamage);
	void GetAngryIfAtGunpoint(CBasePlayer *pPlayer, CBaseMonster *pMonster, const VRController& controller);
	void DoFollowUnfollowCommands(CBasePlayer *pPlayer, CBaseMonster *pMonster, const VRController& controller, bool isTouching);
	bool IsDraggableEntity(EHANDLE hEntity);
};

