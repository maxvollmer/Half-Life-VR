#pragma once

class CBasePlayer;
class CBaseMonster;
class VRController;
struct VRPhysicsHelperModelBBoxIntersectResult;

class VRControllerInteractionManager
{
public:

	void CheckAndPressButtons(CBasePlayer *pPlayer, const VRController& controller);

private:
	class Interaction
	{
	public:
		class InteractionInfo
		{
		public:

			InteractionInfo(bool isSet, bool didChange) :
				isSet{ isSet },
				didChange{ didChange }
			{}

			const bool isSet;
			const bool didChange;
		};

		Interaction(InteractionInfo touching, InteractionInfo dragging, InteractionInfo hitting, float hitDamage) :
			touching{ touching },
			dragging{ dragging },
			hitting{ hitting },
			hitDamage{ hitDamage }
		{}

		const InteractionInfo touching;
		const InteractionInfo dragging;
		const InteractionInfo hitting;
		const float hitDamage;
	};

	bool HandleEasterEgg(CBasePlayer *pPlayer, EHANDLE hEntity, const VRController& controller, bool isTouching, bool didTouchChange);
	bool HandleRetinaScanners(CBasePlayer *pPlayer, EHANDLE hEntity, const VRController& controller, bool isTouching, bool didTouchChange);
	bool HandleButtons(CBasePlayer *pPlayer, EHANDLE hEntity, const VRController& controller, bool isTouching, bool didTouchChange, bool isDragging, bool didDragChange);
	bool HandleDoors(CBasePlayer *pPlayer, EHANDLE hEntity, const VRController& controller, bool isTouching, bool didTouchChange);
	bool HandleRechargers(CBasePlayer *pPlayer, EHANDLE hEntity, const VRController& controller, bool isTouching, bool didTouchChange);
	bool HandleTriggers(CBasePlayer *pPlayer, EHANDLE hEntity, const VRController& controller, bool isTouching, bool didTouchChange);
	bool HandleBreakables(CBasePlayer *pPlayer, EHANDLE hEntity, const VRController& controller, bool isTouching, bool didTouchChange, bool isHitting, bool didHitChange, float flHitDamage);
	bool HandlePushables(CBasePlayer *pPlayer, EHANDLE hEntity, const VRController& controller, bool isTouching, bool didTouchChange);
	bool HandleAlliedMonsters(CBasePlayer *pPlayer, EHANDLE hEntity, const VRController& controller, bool isTouching, bool didTouchChange, bool isHitting, bool didHitChange, float flHitDamage);
	bool HandleGrabbables(CBasePlayer *pPlayer, EHANDLE hEntity, const VRController& controller, const Interaction& interaction);

	void DoFollowUnfollowCommands(CBasePlayer *pPlayer, CBaseMonster *pMonster, const VRController& controller, bool isTouching);
	void GetAngryIfAtGunpoint(CBasePlayer *pPlayer, CBaseMonster *pMonster, const VRController& controller);
	float DoDamage(CBasePlayer *pPlayer, EHANDLE hEntity, const VRController& controller, const VRPhysicsHelperModelBBoxIntersectResult& intersectResult);

	bool CheckIfEntityAndControllerTouch(CBasePlayer* pPlayer, EHANDLE hEntity, const VRController& controller, VRPhysicsHelperModelBBoxIntersectResult* intersectResult);
	bool IsDraggableEntity(EHANDLE hEntity);
};

