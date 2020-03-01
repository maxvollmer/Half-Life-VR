#pragma once

class CBasePlayer;
class CBaseMonster;
class VRController;
struct VRPhysicsHelperModelBBoxIntersectResult;

class VRControllerInteractionManager
{
public:
	void CheckAndPressButtons(CBasePlayer* pPlayer, const VRController& controller);
	void DoMultiControllerActions(CBasePlayer* pPlayer, const VRController& controller1, const VRController& controller2);

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
			{
			}

			const bool isSet;
			const bool didChange;
		};

		Interaction(InteractionInfo touching, InteractionInfo dragging, InteractionInfo hitting, float hitDamage, const Vector& intersectPoint) :
			touching{ touching },
			dragging{ dragging },
			hitting{ hitting },
			hitDamage{ hitDamage },
			intersectPoint{ intersectPoint }
		{
		}

		const InteractionInfo touching;
		const InteractionInfo dragging;
		const InteractionInfo hitting;
		const float hitDamage = 0.f;
		const Vector intersectPoint;
	};

	bool HandleEasterEgg(CBasePlayer* pPlayer, EHANDLE<CBaseEntity> hEntity, const VRController& controller, const Interaction& interaction);
	bool HandleRetinaScanners(CBasePlayer* pPlayer, EHANDLE<CBaseEntity> hEntity, const VRController& controller, bool isTouching, bool didTouchChange);
	bool HandleButtonsAndDoors(CBasePlayer* pPlayer, EHANDLE<CBaseEntity> hEntity, const VRController& controller, const Interaction& interaction);
	bool HandleRechargers(CBasePlayer* pPlayer, EHANDLE<CBaseEntity> hEntity, const VRController& controller, bool isTouching, bool didTouchChange);
	bool HandleTriggers(CBasePlayer* pPlayer, EHANDLE<CBaseEntity> hEntity, const VRController& controller, bool isTouching, bool didTouchChange);
	bool HandleBreakables(CBasePlayer* pPlayer, EHANDLE<CBaseEntity> hEntity, const VRController& controller, bool isTouching, bool didTouchChange, bool isHitting, bool didHitChange, float flHitDamage);
	bool HandlePushables(CBasePlayer* pPlayer, EHANDLE<CBaseEntity> hEntity, const VRController& controller, const Interaction& interaction);
	bool HandleAlliedMonsters(CBasePlayer* pPlayer, EHANDLE<CBaseEntity> hEntity, const VRController& controller, bool isTouching, bool didTouchChange, bool isHitting, bool didHitChange, float flHitDamage);
	bool HandleGrabbables(CBasePlayer* pPlayer, EHANDLE<CBaseEntity> hEntity, const VRController& controller, const Interaction& interaction);
	bool HandleLadders(CBasePlayer* pPlayer, EHANDLE<CBaseEntity> hEntity, const VRController& controller, const Interaction& interaction);
	bool HandleTossables(CBasePlayer* pPlayer, EHANDLE<CBaseEntity> hEntity, const VRController& controller, const Interaction& interaction);

	void DoFollowUnfollowCommands(CBasePlayer* pPlayer, CBaseMonster* pMonster, const VRController& controller, bool isTouching);
	void GetAngryIfAtGunpoint(CBasePlayer* pPlayer, CBaseMonster* pMonster, const VRController& controller);
	float DoDamage(CBasePlayer* pPlayer, EHANDLE<CBaseEntity> hEntity, const VRController& controller, const VRPhysicsHelperModelBBoxIntersectResult& intersectResult);

	bool CheckIfEntityAndControllerTouch(CBasePlayer* pPlayer, EHANDLE<CBaseEntity> hEntity, const VRController& controller, VRPhysicsHelperModelBBoxIntersectResult* intersectResult);
	bool IsDraggableEntity(EHANDLE<CBaseEntity> hEntity);
};
