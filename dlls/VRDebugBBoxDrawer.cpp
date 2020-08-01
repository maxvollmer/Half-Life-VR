
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

#define VR_SET_BEAM_ENDS(x, a, b) dynamic_cast<CBeam*>((CBaseEntity*)lasers[i * numlinesperbox + x])->SetStartAndEndPos(a, b)

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

void VRDebugBBoxDrawer::Clear(EHANDLE<CBaseEntity> hEntity)
{
	for (auto& hLaser : m_bboxes[hEntity])
	{
		UTIL_Remove(hLaser);
	}
	m_bboxes.erase(hEntity);

	ClearPoint(hEntity);
}

void VRDebugBBoxDrawer::ClearPoint(EHANDLE<CBaseEntity> hEntity)
{
	UTIL_Remove(m_points[hEntity]);
	m_points.erase(hEntity);
}

void VRDebugBBoxDrawer::ClearAllBut(EHANDLE<CBaseEntity> hEntity)
{
	std::vector<EHANDLE<CBaseEntity>> lasers = m_bboxes[hEntity];
	m_bboxes.erase(hEntity);

	EHANDLE<CBaseEntity> hPoint = m_points[hEntity];
	m_points.erase(hEntity);

	ClearAll();

	m_bboxes[hEntity] = lasers;
	m_points[hEntity] = hPoint;
}

void VRDebugBBoxDrawer::ClearAll()
{
	for (auto& [hEntity, lasers] : m_bboxes)
	{
		for (auto& hLaser : lasers)
		{
			UTIL_Remove(hLaser);
		}
	}
	m_bboxes.clear();

	for (auto& [hEntity, hPoint] : m_points)
	{
		UTIL_Remove(hPoint);
	}
	m_points.clear();
}

void VRDebugBBoxDrawer::DrawPoint(EHANDLE<CBaseEntity> hEntity, const Vector& point)
{
	if (m_points[hEntity])
	{
		m_points[hEntity]->pev->origin = point;
		UTIL_SetOrigin(m_points[hEntity]->pev, m_points[hEntity]->pev->origin);
	}
	else
	{
		CSprite* pSprite = CSprite::SpriteCreate("sprites/XSpark1.spr", point, FALSE);
		pSprite->Spawn();
		pSprite->SetTransparency(kRenderTransAdd, m_r, m_g, m_b, 255, kRenderFxNone);
		pSprite->TurnOn();
		pSprite->pev->scale = 0.1f;
		pSprite->pev->effects &= ~EF_NODRAW;
		EHANDLE<CBaseEntity> hSprite = pSprite;
		m_points[hEntity] = hSprite;
	}
}

void VRDebugBBoxDrawer::DrawBBoxes(EHANDLE<CBaseEntity> hEntity, bool mirrored)
{
	if (!hEntity)
		return;

	std::vector<StudioHitBox> studiohitboxes;

	if (std::string(STRING(hEntity->pev->model)).find(".mdl") != std::string::npos)
	{
		void* pmodel = GET_MODEL_PTR(hEntity->edict());
		if (pmodel)
		{
			int numhitboxes = GetNumHitboxes(pmodel);
			if (numhitboxes > 0)
			{
				studiohitboxes.resize(numhitboxes);
				if (!GetHitboxesAndAttachments(hEntity->pev, pmodel, hEntity->pev->sequence, hEntity->pev->frame, studiohitboxes.data(), nullptr, mirrored))
				{
					studiohitboxes.clear();
				}
			}
		}
	}

	if (studiohitboxes.empty())
	{
		studiohitboxes.push_back(StudioHitBox
			{
				hEntity->pev->origin,
				Vector{},
				hEntity->pev->mins,
				hEntity->pev->maxs,
				0
			});
	}

	constexpr const int numlinesperbox = 14;
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
		pBeam->SetColor(m_r, m_g, m_b);
		pBeam->SetBrightness(255);
		EHANDLE<CBaseEntity> hBeam = pBeam;
		lasers.push_back(hBeam);
	}

	// Update laser positions for each hitbox (14 lasers, 12 for each edge + diagonal (center to min and max))
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

		VR_SET_BEAM_ENDS(0, hitbox.origin + points[0], hitbox.origin + points[1]);
		VR_SET_BEAM_ENDS(1, hitbox.origin + points[0], hitbox.origin + points[2]);
		VR_SET_BEAM_ENDS(2, hitbox.origin + points[1], hitbox.origin + points[3]);
		VR_SET_BEAM_ENDS(3, hitbox.origin + points[2], hitbox.origin + points[3]);

		VR_SET_BEAM_ENDS(4, hitbox.origin + points[0], hitbox.origin + points[4]);
		VR_SET_BEAM_ENDS(5, hitbox.origin + points[1], hitbox.origin + points[5]);
		VR_SET_BEAM_ENDS(6, hitbox.origin + points[2], hitbox.origin + points[6]);
		VR_SET_BEAM_ENDS(7, hitbox.origin + points[3], hitbox.origin + points[7]);

		VR_SET_BEAM_ENDS(8, hitbox.origin + points[4], hitbox.origin + points[5]);
		VR_SET_BEAM_ENDS(9, hitbox.origin + points[4], hitbox.origin + points[6]);
		VR_SET_BEAM_ENDS(10, hitbox.origin + points[5], hitbox.origin + points[7]);
		VR_SET_BEAM_ENDS(11, hitbox.origin + points[6], hitbox.origin + points[7]);

		VR_SET_BEAM_ENDS(12, hitbox.abscenter, hitbox.origin + points[0]);
		VR_SET_BEAM_ENDS(13, hitbox.abscenter, hitbox.origin + points[7]);

		i++;
	}
}
