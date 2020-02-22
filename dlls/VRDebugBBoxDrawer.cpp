
#include <unordered_map>
#include <algorithm>

#include "extdll.h"
#include "util.h"
#include "vector.h"
#include "cbase.h"
#include "player.h"
#include "weapons.h"
#include "animation.h"

#include "VRDebugBBoxDrawer.h"
#include "VRPhysicsHelper.h"

VRDebugBBoxDrawer::~VRDebugBBoxDrawer()
{
	for (auto& [hEntity, lasers] : m_bboxes)
	{
		for (auto& laser : lasers)
		{
			UTIL_Remove(laser);
		}
	}
	m_bboxes.clear();
}

void VRDebugBBoxDrawer::ClearBBoxes(EHANDLE<CBaseEntity> hEntity)
{
	auto& lasers = m_bboxes[hEntity];
	for (auto& laser : lasers)
	{
		UTIL_Remove(laser);
	}
	m_bboxes.erase(hEntity);
}

void VRDebugBBoxDrawer::DrawBBoxes(EHANDLE<CBaseEntity> hEntity)
{
	if (!hEntity)
		return;

	void* pmodel = GET_MODEL_PTR(hEntity->edict());

	if (!pmodel)
	{
		ClearBBoxes(hEntity);
		return;
	}

	int numhitboxes = GetNumHitboxes(pmodel);
	if (numhitboxes <= 0)
	{
		ClearBBoxes(hEntity);
		return;
	}

	std::vector<StudioHitBox> studiohitboxes;
	studiohitboxes.resize(numhitboxes);
	if (!GetHitboxesAndAttachments(hEntity->pev, pmodel, hEntity->pev->sequence, hEntity->pev->frame, studiohitboxes.data(), nullptr, false))
	{
		ClearBBoxes(hEntity);
		return;
	}

	constexpr const int numlinesperbox = 6;
	size_t numlasers = studiohitboxes.size() * numlinesperbox;

	auto& lasers = m_bboxes[hEntity];

	// Remove any "dead" lasers (levelchange etc.)
	lasers.erase(std::remove_if(lasers.begin(), lasers.end(), [](EHANDLE<CBaseEntity>& e) { return e.Get() == nullptr; }), lasers.end());

	// Remove lasers that are "too much" (happens when switching from model with more hitboxes to model with less hitboxes)
	while (lasers.size() > numlasers)
	{
		UTIL_Remove(lasers.back());
		lasers.pop_back();
	}

	// Create any missing lasers (happens when starting new game or when switching from model with less hitboxes to model with more hitboxes)
	while (lasers.size() < numlasers)
	{
		CBeam* pBeam = CBeam::BeamCreate("sprites/xbeam1.spr", 1);
		pBeam->pev->spawnflags |= SF_BEAM_TEMPORARY;
		pBeam->Spawn();
		pBeam->PointsInit(hEntity->pev->origin, hEntity->pev->origin);
		pBeam->pev->owner = hEntity->edict();
		pBeam->SetColor(255, 0, 0);
		pBeam->SetBrightness(255);
		EHANDLE<CBaseEntity> hBeam = pBeam;
		lasers.push_back(hBeam);
	}

	// Update laser positions for each hitbox (6 lasers, 3 per opposite corner (min and max))
	int i = 0;
	for (const auto& hitbox : studiohitboxes)
	{
		Vector points[8];
		points[0] = Vector{ hitbox.mins[0], hitbox.mins[1], hitbox.mins[2] };
		points[1] = Vector{ hitbox.mins[0], hitbox.mins[1], hitbox.maxs[2] };
		points[2] = Vector{ hitbox.mins[0], hitbox.maxs[1], hitbox.mins[2] };
		points[3] = Vector{ hitbox.mins[0], hitbox.maxs[1], hitbox.maxs[2] };
		points[4] = Vector{ hitbox.maxs[0], hitbox.mins[1], hitbox.mins[2] };
		points[5] = Vector{ hitbox.maxs[0], hitbox.mins[1], hitbox.maxs[2] };
		points[6] = Vector{ hitbox.maxs[0], hitbox.maxs[1], hitbox.mins[2] };
		points[7] = Vector{ hitbox.maxs[0], hitbox.maxs[1], hitbox.maxs[2] };

		for (int j = 0; j < 8; j++)
		{
			VRPhysicsHelper::Instance().RotateVector(points[j], hitbox.angles);
		}

		dynamic_cast<CBeam*>((CBaseEntity*)lasers[i * numlinesperbox + 0])->SetStartAndEndPos(hitbox.origin + points[0], hitbox.origin + points[1]);
		dynamic_cast<CBeam*>((CBaseEntity*)lasers[i * numlinesperbox + 1])->SetStartAndEndPos(hitbox.origin + points[0], hitbox.origin + points[2]);
		dynamic_cast<CBeam*>((CBaseEntity*)lasers[i * numlinesperbox + 2])->SetStartAndEndPos(hitbox.origin + points[0], hitbox.origin + points[3]);

		dynamic_cast<CBeam*>((CBaseEntity*)lasers[i * numlinesperbox + 3])->SetStartAndEndPos(hitbox.origin + points[7], hitbox.origin + points[6]);
		dynamic_cast<CBeam*>((CBaseEntity*)lasers[i * numlinesperbox + 4])->SetStartAndEndPos(hitbox.origin + points[7], hitbox.origin + points[5]);
		dynamic_cast<CBeam*>((CBaseEntity*)lasers[i * numlinesperbox + 5])->SetStartAndEndPos(hitbox.origin + points[7], hitbox.origin + points[4]);

		i++;
	}
}
