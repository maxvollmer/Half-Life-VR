#pragma once

class CBasePlayer;
class CBaseMonster;
class VRController;
struct VRPhysicsHelperModelBBoxIntersectResult;

class VRControllerInteractionManager
{
public:
	void CheckAndPressButtons(CBasePlayer* pPlayer, VRController& controller);
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

		bool IsInteresting();

		const InteractionInfo touching;
		const InteractionInfo dragging;
		const InteractionInfo hitting;
		const float hitDamage = 0.f;
		const Vector intersectPoint;
	};

	bool HandleButtonsAndDoors(CBasePlayer* pPlayer, CBaseEntity* pEntity, const VRController& controller, const Interaction& interaction);
	bool HandleRechargers(CBasePlayer* pPlayer, CBaseEntity* pEntity, const VRController& controller, const Interaction& interaction);
	bool HandleTriggers(CBasePlayer* pPlayer, CBaseEntity* pEntity, const VRController& controller, const Interaction& interaction);
	bool HandleBreakables(CBasePlayer* pPlayer, CBaseEntity* pEntity, const VRController& controller, const Interaction& interaction);
	bool HandlePushables(CBasePlayer* pPlayer, CBaseEntity* pEntity, const VRController& controller, const Interaction& interaction);
	bool HandleAlliedMonsters(CBasePlayer* pPlayer, CBaseEntity* pEntity, const VRController& controller, const Interaction& interaction);
	bool HandleGrabbables(CBasePlayer* pPlayer, CBaseEntity* pEntity, const VRController& controller, const Interaction& interaction);
	bool HandleLadders(CBasePlayer* pPlayer, CBaseEntity* pEntity, const VRController& controller, const Interaction& interaction);
	bool HandleTossables(CBasePlayer* pPlayer, CBaseEntity* pEntity, const VRController& controller, const Interaction& interaction);

	void DoFollowUnfollowCommands(CBasePlayer* pPlayer, CTalkMonster* pMonster, const VRController& controller, bool isTouching);
	void GetAngryIfAtGunpoint(CBasePlayer* pPlayer, CTalkMonster* pMonster, const VRController& controller);

	float DoDamage(CBasePlayer* pPlayer, CBaseEntity* pEntity, const VRController& controller, const VRPhysicsHelperModelBBoxIntersectResult& intersectResult);

	bool CheckIfEntityAndControllerTouch(CBasePlayer* pPlayer, CBaseEntity* pEntity, const VRController& controller, VRPhysicsHelperModelBBoxIntersectResult* intersectResult);
	bool IsDraggableEntity(CBaseEntity* pEntity);
	bool IsDraggableEntityThatCanTouchStuff(CBaseEntity* pEntity);
};
