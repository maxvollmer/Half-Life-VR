
#pragma warning(disable : 4244)
/*
Disable warnings for:
1>..\reactphysics3d\include\collision\shapes\heightfieldshape.h(227): warning C4244: 'return': conversion from 'double' to 'reactphysics3d::decimal', possible loss of data
1>..\reactphysics3d\include\collision\shapes\heightfieldshape.h(235): warning C4244: 'return': conversion from 'reactphysics3d::decimal' to 'int', possible loss of data
*/

#define IS_DOUBLE_PRECISION_ENABLED
#define HARDWARE_MODE

#include "reactphysics3d/include/reactphysics3d.h"

#include "extdll.h"
#include "pm_defs.h"
#include "plane.h"
#include "com_model.h"
#include "studio.h"

#include <fstream>
#include <memory>

#include "VRPhysicsHelper.h"
#include "VRModelHelper.h"

#include "util.h"
#include "cbase.h"
#include "effects.h"
#include "game.h"

#include "animation.h"

#include <chrono>
#include <cstdlib>

#include "VRPhysicsHelperHelpers.hpp"

// global for animation.cpp
void CalculateHitboxAbsCenter(StudioHitBox& hitbox)
{
	hitbox.abscenter = hitbox.origin + VRPhysicsHelper::Instance().RotateVectorInline((hitbox.mins + hitbox.maxs) * 0.5f, hitbox.angles);
}

// global for bmodels.cpp and VRCrushEntityHandler
const model_t* VRGetBSPModel(CBaseEntity* pEntity)
{
	return GetBSPModel(pEntity);
}

// global for CWorldsSmallestCup
const model_t* VRGetBSPModel(edict_t* pent)
{
	return GetBSPModel(pent);
}


//public facing interface. wraps internal functions in try-catch for proper error handling
void VRPhysicsHelper::TraceLine(const Vector& vecStart, const Vector& vecEnd, edict_t* pentIgnore, TraceResult* ptr)
{
	try
	{
		Internal_TraceLine(vecStart, vecEnd, pentIgnore, ptr);
	}
	catch (VRException& e)
	{
		ALERT(at_console, "ERROR: VRException in VRPhysicsHelper::TraceLine: %s\n", e.what());
		ptr->fAllSolid = true;
		ptr->flFraction = 0.f;
	}
}
bool VRPhysicsHelper::CheckIfLineIsBlocked(const Vector& hlPos1, const Vector& hlPos2)
{
	Vector dummy;
	return CheckIfLineIsBlocked(hlPos1, hlPos2, dummy);
}
bool VRPhysicsHelper::CheckIfLineIsBlocked(const Vector& hlPos1, const Vector& hlPos2, Vector& result)
{
	try
	{
		return Internal_CheckIfLineIsBlocked(hlPos1, hlPos2, result);
	}
	catch (VRException& e)
	{
		ALERT(at_console, "ERROR: VRException in VRPhysicsHelper::CheckIfLineIsBlocked: %s\n", e.what());
		return false;
	}
}
bool VRPhysicsHelper::ModelIntersectsBBox(CBaseEntity* pModel, const Vector& bboxCenter, const Vector& bboxMins, const Vector& bboxMaxs, const Vector& bboxAngles, VRPhysicsHelperModelBBoxIntersectResult* result)
{
	try
	{
		return Internal_ModelIntersectsBBox(pModel, bboxCenter, bboxMins, bboxMaxs, bboxAngles, result);
	}
	catch (VRException& e)
	{
		ALERT(at_console, "ERROR: VRException in VRPhysicsHelper::ModelIntersectsBBox: %s\n", e.what());
		return false;
	}
}
bool VRPhysicsHelper::ModelIntersectsCapsule(CBaseEntity* pModel, const Vector& capsuleCenter, double radius, double height)
{
	try
	{
		return Internal_ModelIntersectsCapsule(pModel, capsuleCenter, radius, height);
	}
	catch (VRException& e)
	{
		ALERT(at_console, "ERROR: VRException in VRPhysicsHelper::ModelIntersectsCapsule: %s\n", e.what());
		return false;
	}
}
bool VRPhysicsHelper::ModelIntersectsLine(CBaseEntity* pModel, const Vector& lineA, const Vector& lineB, VRPhysicsHelperModelBBoxIntersectResult* result)
{
	try
	{
		return Internal_ModelIntersectsLine(pModel, lineA, lineB, result);
	}
	catch (VRException& e)
	{
		ALERT(at_console, "ERROR: VRException in VRPhysicsHelper::ModelIntersectsLine: %s\n", e.what());
		return false;
	}
}
bool VRPhysicsHelper::BBoxIntersectsLine(const Vector& bboxCenter, const Vector& bboxMins, const Vector& bboxMaxs, const Vector& bboxAngles, const Vector& lineA, const Vector& lineB, VRPhysicsHelperModelBBoxIntersectResult* result)
{
	try
	{
		return Internal_BBoxIntersectsLine(bboxCenter, bboxMins, bboxMaxs, bboxAngles, lineA, lineB, result);
	}
	catch (VRException& e)
	{
		ALERT(at_console, "ERROR: VRException in VRPhysicsHelper::BBoxIntersectsLine: %s\n", e.what());
		return false;
	}
}


// VRPhysicsHelper implementation
VRPhysicsHelper::VRPhysicsHelper()
{
}

VRPhysicsHelper::~VRPhysicsHelper()
{
	try
	{
		m_bspModelData.clear();
		m_dynamicBSPModelData.clear();
		m_hitboxCaches.clear();

		if (m_collisionWorld)
		{
			if (m_capsuleBody)
			{
				if (m_capsuleProxyShape)
				{
					m_capsuleBody->removeCollisionShape(m_capsuleProxyShape);
					m_capsuleProxyShape = nullptr;
				}
				m_collisionWorld->destroyCollisionBody(m_capsuleBody);
				m_capsuleBody = nullptr;
			}

			delete m_collisionWorld;
			m_collisionWorld = nullptr;
		}

		if (m_dynamicsWorld)
		{
			if (m_worldsSmallestCupBody)
			{
				if (m_worldsSmallestCupTopProxyShape)
				{
					m_worldsSmallestCupBody->removeCollisionShape(m_worldsSmallestCupTopProxyShape);
					m_worldsSmallestCupTopProxyShape = nullptr;
				}
				if (m_worldsSmallestCupBottomProxyShape)
				{
					m_worldsSmallestCupBody->removeCollisionShape(m_worldsSmallestCupBottomProxyShape);
					m_worldsSmallestCupBottomProxyShape = nullptr;
				}
				m_dynamicsWorld->destroyRigidBody(m_worldsSmallestCupBody);
				m_worldsSmallestCupBody = nullptr;
			}

			delete m_dynamicsWorld;
			m_dynamicsWorld = nullptr;
		}

		if (m_capsuleShape)
		{
			delete m_capsuleShape;
			m_capsuleShape = nullptr;
		}

		if (m_worldsSmallestCupTopSphereShape)
		{
			delete m_worldsSmallestCupTopSphereShape;
			m_worldsSmallestCupTopSphereShape = nullptr;
		}

		if (m_worldsSmallestCupBottomSphereShape)
		{
			delete m_worldsSmallestCupBottomSphereShape;
			m_worldsSmallestCupBottomSphereShape = nullptr;
		}

		if (m_worldsSmallestCupPolyhedronMesh)
		{
			delete m_worldsSmallestCupPolyhedronMesh;
			m_worldsSmallestCupPolyhedronMesh = nullptr;
		}

		if (m_worldsSmallestCupPolygonVertexArray)
		{
			delete m_worldsSmallestCupPolygonVertexArray;
			m_worldsSmallestCupPolygonVertexArray = nullptr;
		}

		if (m_worldsSmallestCupPolygonFaces)
		{
			delete[] static_cast<reactphysics3d::PolygonVertexArray::PolygonFace*>(m_worldsSmallestCupPolygonFaces);
			m_worldsSmallestCupPolygonFaces = nullptr;
		}
	}
	catch (VRException& e)
	{
		ALERT(at_console, "ERROR: VRException in ~VRPhysicsHelper: %s\n", e.what());
	}
	catch (...) {}
}

bool VRPhysicsHelper::Internal_CheckIfLineIsBlocked(const Vector& hlPos1, const Vector& hlPos2, Vector& hlResult)
{
	if (!CheckWorld())
	{
		// no world, no trace
		hlResult = hlPos2;
		return false;
	}

	Vector3 pos1 = HLVecToRP3DVec(hlPos1);
	Vector3 pos2 = HLVecToRP3DVec(hlPos2);

	Ray ray{ pos1, pos2, 1. };
	RaycastInfo raycastInfo{};

	bool result = m_bspModelData[m_currentMapName].m_collisionBody->raycast(ray, raycastInfo) && raycastInfo.hitFraction < (1. - EPSILON);

	if (result)
	{
		hlResult = RP3DVecToHLVec(raycastInfo.worldPoint);
	}
	else
	{
		hlResult = hlPos2;
	}

	return result;
}

bool VRPhysicsHelper::TestCollision(reactphysics3d::CollisionBody* body1, reactphysics3d::CollisionBody* body2, VRPhysicsHelperModelBBoxIntersectResult* result)
{
	if (m_collisionWorld->testAABBOverlap(body1, body2) && m_collisionWorld->testOverlap(body1, body2))
	{
		if (result)
		{
			std::vector<reactphysics3d::Vector3> manifoldPoints;
			MyCollisionCallback callback{ [&manifoldPoints](const reactphysics3d::CollisionCallback::CollisionCallbackInfo& collisionCallbackInfo) {
				auto contactManifoldElement = collisionCallbackInfo.contactManifoldElements;
				while (contactManifoldElement)
				{
					auto contactManifold = contactManifoldElement->getContactManifold();
					while (contactManifold)
					{
						auto localToBodyTransform = contactManifold->getShape1()->getLocalToBodyTransform();
						auto contactPoint = contactManifold->getContactPoints();
						while (contactPoint)
						{
							manifoldPoints.push_back(localToBodyTransform * contactPoint->getLocalPointOnShape1());
							contactPoint = contactPoint->getNext();
						}
						contactManifold = contactManifold->getNext();
					}
					contactManifoldElement = contactManifoldElement->getNext();
				}
			} };
			m_collisionWorld->testCollision(body1, body2, &callback);
			if (!manifoldPoints.empty())
			{
				reactphysics3d::Vector3 averagePoint;
				for (auto& point : manifoldPoints)
					averagePoint += point;
				averagePoint /= manifoldPoints.size();
				result->hitpoint = RP3DVecToHLVec(body1->getWorldPoint(averagePoint));
				result->hasresult = true;
			}
		}
		return true;
	}
	return false;
}

reactphysics3d::CollisionBody* VRPhysicsHelper::GetHitBoxBody(size_t cacheIndex, const Vector& bboxCenter, const Vector& bboxMins, const Vector& bboxMaxs, const Vector& bboxAngles)
{
	if (fabs(bboxMaxs.x - bboxMins.x) < EPSILON || fabs(bboxMaxs.y - bboxMins.y) < EPSILON || fabs(bboxMaxs.z - bboxMins.z) < EPSILON)
		return nullptr;

	if (cacheIndex >= m_hitboxCaches.size())
		m_hitboxCaches.resize(cacheIndex + 1);

	auto& hitboxCache = m_hitboxCaches[cacheIndex];

	HitBox hitbox{ bboxMins, bboxMaxs };
	std::shared_ptr<HitBoxModelData> data;

	if (hitboxCache.count(hitbox) == 0)
	{
		data = std::make_shared<HitBoxModelData>();
		data->CreateData(m_collisionWorld, bboxCenter, bboxMins, bboxMaxs, bboxAngles);
		hitboxCache[hitbox] = data;
	}
	else
	{
		data = hitboxCache[hitbox];
		data->UpdateTransform(bboxCenter, bboxAngles);
	}

	return data->m_collisionBody;
}

bool VRPhysicsHelper::Internal_ModelIntersectsCapsule(CBaseEntity* pModel, const Vector& capsuleCenter, double radius, double height)
{
	if (!CheckWorld())
		return false;

	if (!pModel->pev->model)
		return false;

	auto& bspModelData = m_bspModelData.find(std::string{ STRING(pModel->pev->model) });
	if (bspModelData == m_bspModelData.end())
		return false;

	if (!bspModelData->second.HasData())
		return false;

	if (!m_capsuleBody)
	{
		m_capsuleBody = m_collisionWorld->createCollisionBody(rp3d::Transform::identity());
	}

	if (m_capsuleProxyShape)
	{
		m_capsuleBody->removeCollisionShape(m_capsuleProxyShape);
		m_capsuleProxyShape = nullptr;
	}
	if (m_capsuleShape)
	{
		delete m_capsuleShape;
		m_capsuleShape = nullptr;
	}

	m_capsuleShape = new CapsuleShape{ (radius)*HL_TO_RP3D, (height)*HL_TO_RP3D };
	m_capsuleProxyShape = m_capsuleBody->addCollisionShape(m_capsuleShape, rp3d::Transform::identity());
	m_capsuleBody->setTransform(rp3d::Transform{ HLVecToRP3DVec(capsuleCenter), HLAnglesToRP3DQuaternion(Vector{0.f, 0.f, 90.f}) });

	bspModelData->second.m_collisionBody->setTransform(rp3d::Transform{ HLVecToRP3DVec(pModel->pev->origin), HLAnglesToRP3DQuaternion(pModel->pev->angles) });

	return TestCollision(bspModelData->second.m_collisionBody, m_capsuleBody);
}

bool VRPhysicsHelper::ModelBBoxIntersectsBBox(
	const Vector& bboxCenter1, const Vector& bboxMins1, const Vector& bboxMaxs1, const Vector& bboxAngles1, const Vector& bboxCenter2, const Vector& bboxMins2, const Vector& bboxMaxs2, const Vector& bboxAngles2, VRPhysicsHelperModelBBoxIntersectResult* result)
{
	if (!CheckWorld())
		return false;

	auto bboxBody1 = GetHitBoxBody(0, bboxCenter1, bboxMins1, bboxMaxs1, bboxAngles1);
	if (!bboxBody1)
		return false;

	auto bboxBody2 = GetHitBoxBody(1, bboxCenter2, bboxMins2, bboxMaxs2, bboxAngles2);
	if (!bboxBody2)
		return false;

	if (TestCollision(bboxBody1, bboxBody2, result))
	{
		// If r3pd didn't return a manifold point, get middle point between rotated box centers
		if (!result->hasresult)
		{
			Vector center1 = bboxCenter1 + RotateVectorInline((bboxMins1 + bboxMaxs1) * 0.5f, bboxAngles1);
			Vector center2 = bboxCenter2 + RotateVectorInline((bboxMins2 + bboxMaxs2) * 0.5f, bboxAngles2);
			result->hitpoint = (center1 + center2) * 0.5f;
			result->hasresult = true;
		}
		return true;
	}

	return false;
}

bool VRPhysicsHelper::Internal_BBoxIntersectsLine(const Vector& bboxCenter, const Vector& bboxMins, const Vector& bboxMaxs, const Vector& bboxAngles, const Vector& lineA, const Vector& lineB, VRPhysicsHelperModelBBoxIntersectResult* result)
{
	if (!CheckWorld())
		return false;

	auto bboxBody = GetHitBoxBody(0, bboxCenter, bboxMins, bboxMaxs, bboxAngles);
	if (!bboxBody)
		return false;

	Vector hitPoint;
	if (BodyIntersectsLine(bboxBody, lineA, lineB, hitPoint))
	{
		if (result)
		{
			result->hasresult = true;
			result->hitpoint = hitPoint;
		}
		return true;
	}

	return false;
}

bool VRPhysicsHelper::Internal_ModelIntersectsLine(CBaseEntity* pModel, const Vector& lineA, const Vector& lineB, VRPhysicsHelperModelBBoxIntersectResult* result)
{
	if (!CheckWorld())
		return false;

	if (!pModel->pev->model)
	{
		// No model, use bounding box
		return BBoxIntersectsLine(pModel->pev->origin, pModel->pev->mins, pModel->pev->maxs, Vector{}, lineA, lineB, result);
	}

	std::string modelName{ STRING(pModel->pev->model) };

	auto& bspModelData = m_bspModelData.find(modelName);
	if (bspModelData != m_bspModelData.end() && bspModelData->second.HasData())
	{
		// BSP model
		Vector hitPoint;
		if (BodyIntersectsLine(bspModelData->second.m_collisionBody, lineA, lineB, hitPoint))
		{
			if (result)
			{
				result->hasresult = true;
				result->hitpoint = hitPoint;
			}
			return true;
		}
		else
		{
			return false;
		}
	}
	else
	{
		// Studio model
		void* pmodel = GET_MODEL_PTR(pModel->edict());
		if (!pmodel)
		{
			// Invalid studio model, use bounding box
			return BBoxIntersectsLine(pModel->pev->origin, pModel->pev->mins, pModel->pev->maxs, Vector{}, lineA, lineB, result);
		}

		int numhitboxes = GetNumHitboxes(pmodel);
		if (numhitboxes <= 0)
		{
			// No hitboxes, use bounding box
			return BBoxIntersectsLine(pModel->pev->origin, pModel->pev->mins, pModel->pev->maxs, Vector{}, lineA, lineB, result);
		}

		std::vector<StudioHitBox> studiohitboxes;
		studiohitboxes.resize(numhitboxes);
		if (GetHitboxesAndAttachments(pModel->pev, pmodel, pModel->pev->sequence, pModel->pev->frame, studiohitboxes.data(), nullptr, false))
		{
			for (const auto& studiohitbox : studiohitboxes)
			{
				if (BBoxIntersectsLine(studiohitbox.origin, studiohitbox.mins, studiohitbox.maxs, studiohitbox.angles, lineA, lineB, result))
				{
					if (result)
					{
						result->hitgroup = studiohitbox.hitgroup;
					}
					return true;
				}
			}
			return false;
		}
		else
		{
			// Failed to get hitboxes, use bounding box
			return BBoxIntersectsLine(pModel->pev->origin, pModel->pev->mins, pModel->pev->maxs, Vector{}, lineA, lineB, result);
		}
	}
}

bool VRPhysicsHelper::Internal_ModelIntersectsBBox(CBaseEntity* pModel, const Vector& bboxCenter, const Vector& bboxMins, const Vector& bboxMaxs, const Vector& bboxAngles, VRPhysicsHelperModelBBoxIntersectResult* result)
{
	if (!CheckWorld())
		return false;

	if (!pModel->pev->model)
	{
		// No model, use bounding box
		return ModelBBoxIntersectsBBox(pModel->pev->origin, pModel->pev->mins, pModel->pev->maxs, Vector{}, bboxCenter, bboxMins, bboxMaxs, bboxAngles, result);
	}

	std::string modelName{ STRING(pModel->pev->model) };

	auto& bspModelData = m_bspModelData.find(modelName);
	if (bspModelData != m_bspModelData.end() && bspModelData->second.HasData())
	{
		// BSP model
		auto bboxBody = GetHitBoxBody(0, bboxCenter, bboxMins, bboxMaxs, bboxAngles);
		if (!bboxBody)
			return false;

		bspModelData->second.m_collisionBody->setTransform(rp3d::Transform{ HLVecToRP3DVec(pModel->pev->origin), HLAnglesToRP3DQuaternion(pModel->pev->angles) });
		return TestCollision(bspModelData->second.m_collisionBody, bboxBody, result);
	}
	else
	{
		// Studio model
		void* pmodel = nullptr;
		if (modelName.find(".mdl") == modelName.size() - 4)
		{
			pmodel = GET_MODEL_PTR(pModel->edict());
		}
		if (!pmodel)
		{
			// Invalid studio model, use bounding box
			// TODO: If it's a sprite, a sphere is probably better
			return ModelBBoxIntersectsBBox(pModel->pev->origin, pModel->pev->mins, pModel->pev->maxs, Vector{}, bboxCenter, bboxMins, bboxMaxs, bboxAngles, result);
		}

		int numhitboxes = GetNumHitboxes(pmodel);
		if (numhitboxes <= 0)
		{
			// No hitboxes, use bounding box
			return ModelBBoxIntersectsBBox(pModel->pev->origin, pModel->pev->mins, pModel->pev->maxs, Vector{}, bboxCenter, bboxMins, bboxMaxs, bboxAngles, result);
		}

		std::vector<StudioHitBox> studiohitboxes;
		studiohitboxes.resize(numhitboxes);
		if (GetHitboxesAndAttachments(pModel->pev, pmodel, pModel->pev->sequence, pModel->pev->frame, studiohitboxes.data(), nullptr, false))
		{
			for (const auto& studiohitbox : studiohitboxes)
			{
				if (ModelBBoxIntersectsBBox(studiohitbox.origin, studiohitbox.mins, studiohitbox.maxs, studiohitbox.angles, bboxCenter, bboxMins, bboxMaxs, bboxAngles, result))
				{
					if (result)
					{
						result->hitgroup = studiohitbox.hitgroup;
					}
					return true;
				}
			}
			return false;
		}
		else
		{
			// Failed to get hitboxes, use bounding box
			return ModelBBoxIntersectsBBox(pModel->pev->origin, pModel->pev->mins, pModel->pev->maxs, Vector{}, bboxCenter, bboxMins, bboxMaxs, bboxAngles, result);
		}
	}
}

void VRPhysicsHelper::Internal_TraceLine(const Vector& vecStart, const Vector& vecEnd, edict_t* pentIgnore, TraceResult* ptr)
{
	UTIL_TraceLine(vecStart, vecEnd, ignore_monsters, dont_ignore_glass, pentIgnore, ptr);

	if (CheckWorld())
	{
		Vector3 pos1 = HLVecToRP3DVec(vecStart);
		Vector3 pos2 = HLVecToRP3DVec(vecEnd);

		Ray ray{ pos1, pos2, 1. };
		RaycastInfo raycastInfo{};

		if (m_bspModelData[m_currentMapName].m_collisionBody->raycast(ray, raycastInfo) && raycastInfo.hitFraction < ptr->flFraction)
		{
			ptr->vecEndPos = RP3DVecToHLVec(raycastInfo.worldPoint);
			ptr->vecPlaneNormal = RP3DVecToHLVec(raycastInfo.worldNormal).Normalize();
			ptr->flPlaneDist = DotProduct(ptr->vecPlaneNormal, -ptr->vecEndPos);

			ptr->fAllSolid = false;
			ptr->fInOpen = false;
			ptr->fInWater = UTIL_PointContents(ptr->vecEndPos) == CONTENTS_WATER;
			ptr->flFraction = raycastInfo.hitFraction;
			ptr->iHitgroup = 0;
			ptr->pHit = INDEXENT(0);
		}

		// Check ladders as well...
		for (edict_t* pent = FIND_ENTITY_BY_CLASSNAME(nullptr, "func_ladder");
			!FNullEnt(pent);
			pent = FIND_ENTITY_BY_CLASSNAME(pent, "func_ladder"))
		{
			Vector3 ladderSize = HLVecToRP3DVec(pent->v.size);
			Vector3 ladderPosition = HLVecToRP3DVec(pent->v.absmin);

			if (ladderSize.x <= 0. || ladderSize.y <= 0. || ladderSize.z <= 0.)
			{
				ALERT(at_console, "Warning, found ladder with invalid size: %s (%f %f %f)\n", STRING(pent->v.targetname), pent->v.size.x, pent->v.size.y, pent->v.size.z);
				pent = FIND_ENTITY_BY_CLASSNAME(pent, "func_ladder");
				continue;
			}

			BoxShape boxShape{ ladderSize };

			CollisionBody* body = m_collisionWorld->createCollisionBody(rp3d::Transform{ ladderPosition, Matrix3x3::identity() });
			body->addCollisionShape(&boxShape, rp3d::Transform::identity());

			if (body->raycast(ray, raycastInfo))
			{
				Vector hitPoint = RP3DVecToHLVec(raycastInfo.worldPoint);
				float distanceToPreviousPoint = (hitPoint - ptr->vecEndPos).Length();
				if (raycastInfo.hitFraction <= ptr->flFraction || distanceToPreviousPoint < LADDER_EPSILON)
				{
					ptr->vecEndPos = RP3DVecToHLVec(raycastInfo.worldPoint);
					ptr->vecPlaneNormal = RP3DVecToHLVec(raycastInfo.worldNormal).Normalize();
					ptr->flPlaneDist = DotProduct(ptr->vecPlaneNormal, -ptr->vecEndPos);

					ptr->fAllSolid = false;
					ptr->fInOpen = false;
					ptr->fInWater = UTIL_PointContents(ptr->vecEndPos) == CONTENTS_WATER;
					ptr->flFraction = raycastInfo.hitFraction;
					ptr->iHitgroup = 0;
					ptr->pHit = pent;
				}
			}

			m_collisionWorld->destroyCollisionBody(body);
		}
	}
}

void VRPhysicsHelper::InitPhysicsWorld()
{
	if (m_collisionWorld == nullptr)
	{
		m_collisionWorld = new VRCollisionWorld{};
	}
	if (m_dynamicsWorld == nullptr)
	{
		m_dynamicsWorld = new VRDynamicsWorld{ rp3d::Vector3{0, 0, 0} };
	}
}

VRPhysicsHelper::HitBoxModelData::~HitBoxModelData()
{
	try
	{
		DeleteData();
	}
	catch (...) {}
}

void VRPhysicsHelper::HitBoxModelData::CreateData(CollisionWorld* collisionWorld, const Vector& origin, const Vector& mins, const Vector& maxs, const Vector& angles)
{
	m_mins = mins;
	m_maxs = maxs;
	m_center = (m_mins + m_maxs) * 0.5f;

	Vector3 size = HLVecToRP3DVec(maxs - mins);

	m_collisionBody = collisionWorld->createCollisionBody(rp3d::Transform::identity());
	m_bboxShape = new BoxShape{ size * 0.5 };
	m_proxyShape = m_collisionBody->addCollisionShape(m_bboxShape, rp3d::Transform::identity());

	m_collisionWorld = collisionWorld;

	m_hasData = true;
	UpdateTransform(origin, angles);
}

void VRPhysicsHelper::HitBoxModelData::UpdateTransform(const Vector& origin, const Vector& angles)
{
	if (!m_hasData)
		return;

	Vector3 rotatedCenter = HLVecToRP3DVec(VRPhysicsHelper::Instance().RotateVectorInline(m_center, angles));

	m_collisionBody->setTransform(rp3d::Transform{ HLVecToRP3DVec(origin) + rotatedCenter, HLAnglesToRP3DQuaternion(angles) });
}

void VRPhysicsHelper::HitBoxModelData::DeleteData()
{
	m_hasData = false;

	if (m_collisionBody)
	{
		if (m_proxyShape)
		{
			m_collisionBody->removeCollisionShape(m_proxyShape);
		}
		m_collisionWorld->destroyCollisionBody(m_collisionBody);
	}

	m_collisionBody = nullptr;
	m_proxyShape = nullptr;

	if (m_bboxShape)
	{
		delete m_bboxShape;
		m_bboxShape = nullptr;
	}
}


VRPhysicsHelper::BSPModelData::~BSPModelData()
{
	try
	{
		m_vertices.clear();
		m_indices.clear();
		DeleteData();
	}
	catch (...) {}
}

void VRPhysicsHelper::BSPModelData::CreateData(CollisionWorld* collisionWorld)
{
	// TODO: Why do we still get invalid triangles?
	TEMPTODO_RemoveInvalidTriangles(m_vertices, m_indices);
	if (m_indices.empty())
	{
		ALERT(at_console, "Warning(BSPModelData::CreateData): All triangles are invalid, skipping.\n");
		return;
	}

	m_triangleVertexArray = new TriangleVertexArray(
		m_vertices.size(), m_vertices.data(), sizeof(reactphysics3d::Vector3), m_indices.size() / 3, m_indices.data(), sizeof(int32_t) * 3, TriangleVertexArray::VertexDataType::VERTEX_DOUBLE_TYPE, TriangleVertexArray::IndexDataType::INDEX_INTEGER_TYPE);

	m_triangleMesh = new TriangleMesh;
	m_triangleMesh->addSubpart(m_triangleVertexArray);

	m_concaveMeshShape = new ConcaveMeshShape(m_triangleMesh);

	m_collisionBody = collisionWorld->createCollisionBody(rp3d::Transform::identity());

	m_proxyShape = m_collisionBody->addCollisionShape(m_concaveMeshShape, rp3d::Transform::identity());

	m_collisionWorld = collisionWorld;

	m_hasData = true;
}

void VRPhysicsHelper::BSPModelData::DeleteData()
{
	m_hasData = false;

	if (m_collisionBody)
	{
		if (m_proxyShape)
		{
			m_collisionBody->removeCollisionShape(m_proxyShape);
		}
		m_collisionWorld->destroyCollisionBody(m_collisionBody);
	}

	m_collisionBody = nullptr;
	m_proxyShape = nullptr;

	if (m_concaveMeshShape)
	{
		delete m_concaveMeshShape;
		m_concaveMeshShape = nullptr;
	}

	if (m_triangleMesh)
	{
		delete m_triangleMesh;
		m_triangleMesh = nullptr;
	}

	if (m_triangleVertexArray)
	{
		delete m_triangleVertexArray;
		m_triangleVertexArray = nullptr;
	}
}

VRPhysicsHelper::DynamicBSPModelData::~DynamicBSPModelData()
{
	try
	{
		m_vertices.clear();
		m_normals.clear();
		m_indices.clear();
		DeleteData();
	}
	catch (...) {}
}

void VRPhysicsHelper::DynamicBSPModelData::CreateData(DynamicsWorld* dynamicsWorld)
{
	for (auto& normal : m_normals)
	{
		normal.normalize();
		assert(normal.lengthSquare() > rp3d::decimal(0.8));
	}

	m_triangleVertexArray = new TriangleVertexArray(
		m_vertices.size(), m_vertices.data(), sizeof(reactphysics3d::Vector3), m_normals.data(), sizeof(reactphysics3d::Vector3), m_indices.size() / 3, m_indices.data(), sizeof(int32_t) * 3, TriangleVertexArray::VertexDataType::VERTEX_DOUBLE_TYPE, TriangleVertexArray::NormalDataType::NORMAL_DOUBLE_TYPE, TriangleVertexArray::IndexDataType::INDEX_INTEGER_TYPE);

	m_triangleMesh = new TriangleMesh;
	m_triangleMesh->addSubpart(m_triangleVertexArray);

	m_concaveMeshShape = new ConcaveMeshShape(m_triangleMesh);

	m_rigidBody = dynamicsWorld->createRigidBody(rp3d::Transform::identity());
	m_rigidBody->setType(BodyType::STATIC);
	m_rigidBody->enableGravity(false);

	m_proxyShape = m_rigidBody->addCollisionShape(m_concaveMeshShape, rp3d::Transform::identity(), 1.);

	m_dynamicsWorld = dynamicsWorld;

	m_hasData = true;
}

void VRPhysicsHelper::DynamicBSPModelData::DeleteData()
{
	m_hasData = false;

	if (m_rigidBody)
	{
		if (m_proxyShape)
		{
			m_rigidBody->removeCollisionShape(m_proxyShape);
		}
		m_dynamicsWorld->destroyRigidBody(m_rigidBody);
	}

	m_rigidBody = nullptr;
	m_proxyShape = nullptr;

	if (m_concaveMeshShape)
	{
		delete m_concaveMeshShape;
		m_concaveMeshShape = nullptr;
	}

	if (m_triangleMesh)
	{
		delete m_triangleMesh;
		m_triangleMesh = nullptr;
	}

	if (m_triangleVertexArray)
	{
		delete m_triangleVertexArray;
		m_triangleVertexArray = nullptr;
	}
}

bool VRPhysicsHelper::GetPhysicsMapDataFromFile(const std::string& mapFilePath, const std::string& physicsMapDataFilePath)
{
	m_bspModelData.clear();
	m_dynamicBSPModelData.clear();

	try
	{
		std::ifstream physicsMapDataFileStream{ physicsMapDataFilePath, std::ios_base::in | std::ios_base::binary };
		if (!physicsMapDataFileStream.fail() && physicsMapDataFileStream.good())
		{
			uint32_t magic = 0;
			uint32_t version = 0;
			uint64_t hash = 0;

			ReadBinaryData(physicsMapDataFileStream, magic);
			ReadBinaryData(physicsMapDataFileStream, version);
			ReadBinaryData(physicsMapDataFileStream, hash);

			if (magic == HLVR_MAP_PHYSDATA_FILE_MAGIC && version == HLVR_MAP_PHYSDATA_FILE_VERSION && hash != 0)
			{
				if (hash != HashFileData(mapFilePath))
				{
					ALERT(at_console, "Game must recalculate physics data because %s has changed.\n", mapFilePath.c_str());
					return false;
				}

				{
					uint32_t bspDataCount = 0;
					ReadBinaryData(physicsMapDataFileStream, bspDataCount);
					if (bspDataCount > 0)
					{
						for (uint32_t bspDataIndex = 0; bspDataIndex < bspDataCount; bspDataIndex++)
						{
							uint32_t verticesCount = 0;
							uint32_t indicesCount = 0;
							ReadBinaryData(physicsMapDataFileStream, verticesCount);
							ReadBinaryData(physicsMapDataFileStream, indicesCount);

							std::string modelname = ReadString(physicsMapDataFileStream);
							if (!modelname.empty())
							{
								ReadVerticesAndIndices(
									physicsMapDataFileStream,
									verticesCount,
									0,
									indicesCount,
									m_bspModelData[modelname].m_vertices,
									nullptr,
									m_bspModelData[modelname].m_indices);
								m_bspModelData[modelname].CreateData(m_collisionWorld);
							}
							else
							{
								throw VRException{ "invalid bsp data name at index " + std::to_string(bspDataIndex) };
							}
						}
					}
					else
					{
						throw VRException{ "invalid bsp data size" };
					}
				}

				{
					uint32_t dynamicBSPDataCount = 0;
					ReadBinaryData(physicsMapDataFileStream, dynamicBSPDataCount);
					if (dynamicBSPDataCount > 0)
					{
						for (uint32_t dynamicBSPDataIndex = 0; dynamicBSPDataIndex < dynamicBSPDataCount; dynamicBSPDataIndex++)
						{
							uint32_t verticesCount = 0;
							uint32_t indicesCount = 0;
							ReadBinaryData(physicsMapDataFileStream, verticesCount);
							ReadBinaryData(physicsMapDataFileStream, indicesCount);

							std::string modelname = ReadString(physicsMapDataFileStream);
							if (!modelname.empty())
							{
								ReadVerticesAndIndices(
									physicsMapDataFileStream,
									verticesCount,
									verticesCount,
									indicesCount,
									m_dynamicBSPModelData[modelname].m_vertices,
									&m_dynamicBSPModelData[modelname].m_normals,
									m_dynamicBSPModelData[modelname].m_indices);
								m_dynamicBSPModelData[modelname].CreateData(m_dynamicsWorld);
							}
							else
							{
								throw VRException{ "invalid dynamic bsp data name at index " + std::to_string(dynamicBSPDataIndex) };
							}
						}
					}
					else
					{
						throw VRException{ "invalid dynamic dynamic bsp data size" };
					}
				}

				char checkeof;
				if (physicsMapDataFileStream >> checkeof)  // this should fail
				{
					throw VRException{ "expected eof, but didn't reach eof" };
				}

				if (m_bspModelData.count(m_currentMapName) == 0 || m_dynamicBSPModelData.count(m_currentMapName) == 0)
				{
					throw VRException{ "sanity check failed, invalid physics data" };
				}

				ALERT(at_console, "Successfully loaded physics data from %s\n", physicsMapDataFilePath.c_str());
				return true;
			}
			else
			{
				throw VRException{ "invalid header" };
			}
		}
		else
		{
			ALERT(at_console, "Game must recalculate physics data because %s doesn't exist or couldn't be opened.\n", physicsMapDataFilePath.c_str());
			return false;
		}
	}
	catch (VRException& e)
	{
		ALERT(at_console, "Game must recalculate physics data due to error while trying to parse %s: %s\n", physicsMapDataFilePath.c_str(), e.what());
		m_bspModelData.clear();
		m_dynamicBSPModelData.clear();
		return false;
	}
}

void VRPhysicsHelper::StorePhysicsMapDataToFile(const std::string& mapFilePath, const std::string& physicsMapDataFilePath)
{
	std::ofstream physicsMapDataFileStream{ physicsMapDataFilePath, std::ios_base::out | std::ios_base::trunc | std::ios_base::binary };
	if (!physicsMapDataFileStream.fail())
	{
		uint64_t hash = HashFileData(mapFilePath);
		if (hash == 0)
		{
			ALERT(at_console, "Couldn't store physics data in file %s! Hash generation for bsp file %s failed!\n", physicsMapDataFilePath.c_str(), mapFilePath.c_str());
			return;
		}

		WriteBinaryData(physicsMapDataFileStream, HLVR_MAP_PHYSDATA_FILE_MAGIC);
		WriteBinaryData(physicsMapDataFileStream, HLVR_MAP_PHYSDATA_FILE_VERSION);
		WriteBinaryData(physicsMapDataFileStream, hash);

		WriteBinaryData(physicsMapDataFileStream, static_cast<uint32_t>(m_bspModelData.size()));  // bspDataCount
		for (auto& bspModelData : m_bspModelData)
		{
			WriteBinaryData(physicsMapDataFileStream, static_cast<uint32_t>(bspModelData.second.m_vertices.size()));
			WriteBinaryData(physicsMapDataFileStream, static_cast<uint32_t>(bspModelData.second.m_indices.size()));
			WriteString(physicsMapDataFileStream, bspModelData.first);

			for (unsigned int i = 0; i < bspModelData.second.m_vertices.size(); ++i)
			{
				const reactphysics3d::Vector3& vertex = bspModelData.second.m_vertices[i];
				WriteBinaryData(physicsMapDataFileStream, vertex.x);
				WriteBinaryData(physicsMapDataFileStream, vertex.y);
				WriteBinaryData(physicsMapDataFileStream, vertex.z);
			}

			for (unsigned int i = 0; i < bspModelData.second.m_indices.size(); ++i)
			{
				int index = bspModelData.second.m_indices.at(i);
				WriteBinaryData(physicsMapDataFileStream, index);
			}
		}

		WriteBinaryData(physicsMapDataFileStream, static_cast<uint32_t>(m_dynamicBSPModelData.size()));
		for (auto& bspModelData : m_dynamicBSPModelData)
		{
			WriteBinaryData(physicsMapDataFileStream, static_cast<uint32_t>(bspModelData.second.m_vertices.size()));
			WriteBinaryData(physicsMapDataFileStream, static_cast<uint32_t>(bspModelData.second.m_indices.size()));
			WriteString(physicsMapDataFileStream, bspModelData.first);

			for (unsigned int i = 0; i < bspModelData.second.m_vertices.size(); ++i)
			{
				const reactphysics3d::Vector3& vertex = bspModelData.second.m_vertices[i];
				WriteBinaryData(physicsMapDataFileStream, vertex.x);
				WriteBinaryData(physicsMapDataFileStream, vertex.y);
				WriteBinaryData(physicsMapDataFileStream, vertex.z);
			}

			for (unsigned int i = 0; i < bspModelData.second.m_normals.size(); ++i)
			{
				const reactphysics3d::Vector3& normal = bspModelData.second.m_normals[i];
				WriteBinaryData(physicsMapDataFileStream, normal.x);
				WriteBinaryData(physicsMapDataFileStream, normal.y);
				WriteBinaryData(physicsMapDataFileStream, normal.z);
			}

			for (unsigned int i = 0; i < bspModelData.second.m_indices.size(); ++i)
			{
				int index = bspModelData.second.m_indices.at(i);
				WriteBinaryData(physicsMapDataFileStream, index);
			}
		}
		ALERT(at_console, "Successfully stored physics data in file %s!\n", physicsMapDataFilePath.c_str());
	}
	else
	{
		ALERT(at_console, "Couldn't store physics data in file %s! Check reading/writing privileges on your map folder.\n", physicsMapDataFilePath.c_str());
	}
}

void VRPhysicsHelper::GetPhysicsMapDataFromModel()
{
	const std::string mapModelNameStart{ "maps/" };

	m_bspModelData.clear();
	m_dynamicBSPModelData.clear();

	// create bsp data for collision world
	for (int index = 0; index < gpGlobals->maxEntities; index++)
	{
		edict_t* pent = INDEXENT(index);
		if (FNullEnt(pent) && !FWorldEnt(pent))
			continue;

		const model_t* model = GetBSPModel(pent);
		if (!model)
			continue;

		const std::string modelName{ model->name };
		bool isMapModel = (modelName.compare(0, mapModelNameStart.length(), mapModelNameStart) == 0);

		auto& bspModelData = m_bspModelData[modelName];

		PlaneFacesMap planeFaces;
		PlaneVertexMetaDataMap planeVertexMetaData;

		TriangulateBSPModel(model, planeFaces, planeVertexMetaData, bspModelData.m_vertices, nullptr, bspModelData.m_indices, isMapModel);
		bspModelData.CreateData(m_collisionWorld);

		if (isMapModel)
		{
			// Triangulate gaps for collision world
			std::vector<Vector3> additionalVertices;
			std::vector<int> additionalIndices;
			FindAdditionalPotentialGapsBetweenMapFacesUsingPhysicsEngine(bspModelData.m_collisionBody, planeFaces, planeVertexMetaData, additionalVertices, additionalIndices, bspModelData.m_vertices.size());

			bspModelData.DeleteData();

			bspModelData.m_vertices.insert(bspModelData.m_vertices.end(), additionalVertices.begin(), additionalVertices.end());
			bspModelData.m_indices.insert(bspModelData.m_indices.end(), additionalIndices.begin(), additionalIndices.end());
			bspModelData.CreateData(m_collisionWorld);
		}
	}


	// create bsp data for dynamics world (for now only map)
	const model_t* mapModel = GetWorldBSPModel();
	if (mapModel)
	{
		// Create map model for dynamic world
		auto& dynamicBSPModelData = m_dynamicBSPModelData[std::string{ mapModel->name }];
		PlaneFacesMap planeFaces;
		PlaneVertexMetaDataMap planeVertexMetaData;
		TriangulateBSPModel(mapModel, planeFaces, planeVertexMetaData, dynamicBSPModelData.m_vertices, &dynamicBSPModelData.m_normals, dynamicBSPModelData.m_indices, false);
		dynamicBSPModelData.CreateData(m_dynamicsWorld);
	}
}

bool VRPhysicsHelper::CheckWorld()
{
	const model_t* world = GetWorldBSPModel();

	try
	{
		if (!CompareWorlds(world, m_hlWorldModel))
		{
			m_bspModelData.clear();
			m_dynamicBSPModelData.clear();
			m_hlWorldModel = nullptr;
			m_currentMapName = "";

			if (IsWorldValid(world))
			{
				if (m_collisionWorld == nullptr)
				{
					InitPhysicsWorld();
				}

				m_currentMapName = std::string{ world->name };
				const std::string mapName{ m_currentMapName.substr(0, m_currentMapName.find_last_of(".")) };
				const std::string physicsMapDataFilePath{ UTIL_GetGameDir() + "/" + mapName + ".hlvrphysdata" };
				const std::string mapFilePath{ UTIL_GetGameDir() + "/" + mapName + ".bsp" };

				ALERT(at_console, "Initializing physics data for current map (%s)...\n", m_currentMapName.data());
				auto start = std::chrono::system_clock::now();
				std::chrono::duration<double> timePassed;

				if (!GetPhysicsMapDataFromFile(mapFilePath, physicsMapDataFilePath))
				{
					try
					{
						GetPhysicsMapDataFromModel();
					}
					catch (VRException& e)
					{
						ALERT(at_console, "ERROR: Couldn't create physics data for map %s, some VR features will not work correctly: %s\n", physicsMapDataFilePath.c_str(), e.what());
						m_bspModelData.clear();
						m_dynamicBSPModelData.clear();
						m_hlWorldModel = world; // Don't try again
						return false;
					}
					StorePhysicsMapDataToFile(mapFilePath, physicsMapDataFilePath);
				}

				timePassed = std::chrono::system_clock::now() - start;
				ALERT(at_console, "...initialized physics data in %f seconds.\n", timePassed.count());

				m_hlWorldModel = world;
			}
		}
	}
	catch (VRException& e)
	{
		ALERT(at_console, "ERROR: VRException in VRPhysicsHelper::CheckWorld: %s\n", e.what());
	}

	return m_hlWorldModel != nullptr && m_hlWorldModel == world && m_collisionWorld != nullptr && !m_bspModelData.empty() && m_bspModelData.count(m_currentMapName) > 0 && m_dynamicBSPModelData.count(m_currentMapName) > 0;
}

void VRPhysicsHelper::RotateVectorX(Vector& vecToRotate, const float angle)
{
	if (angle != 0.f)
	{
		float fCos = cos(angle);
		float fSin = sin(angle);

		float x = vecToRotate.x;
		float z = vecToRotate.z;

		vecToRotate.x = fCos * x + fSin * z;
		vecToRotate.z = -fSin * x + fCos * z;
	}
}

void VRPhysicsHelper::RotateVectorY(Vector& vecToRotate, const float angle)
{
	if (angle != 0.f)
	{
		float fCos = cos(angle);
		float fSin = sin(angle);

		float x = vecToRotate.x;
		float y = vecToRotate.y;

		vecToRotate.x = fCos * x - fSin * y;
		vecToRotate.y = fSin * x + fCos * y;
	}
}

void VRPhysicsHelper::RotateVectorZ(Vector& vecToRotate, const float angle)
{
	if (angle != 0.f)
	{
		float fCos = cos(angle);
		float fSin = sin(angle);

		float y = vecToRotate.y;
		float z = vecToRotate.z;

		vecToRotate.y = fCos * y - fSin * z;
		vecToRotate.z = fSin * y + fCos * z;
	}
}

Vector VRPhysicsHelper::RotateVectorInline(const Vector& vecToRotate, const Vector& vecAngles, const Vector& vecOffset, const bool reverse)
{
	Vector rotatedVector = vecToRotate;
	RotateVector(rotatedVector, vecAngles, vecOffset, reverse);
	return rotatedVector;
}

void VRPhysicsHelper::RotateVector(Vector& vecToRotate, const Vector& vecAngles, const Vector& vecOffset, const bool reverse)
{
	if (vecToRotate.Length() != 0)
	{
		vecToRotate = vecToRotate - vecOffset;

		if (reverse)
		{
			RotateVectorY(vecToRotate, vecAngles.y / 180. * M_PI);
			RotateVectorX(vecToRotate, vecAngles.x / 180. * M_PI);
			RotateVectorZ(vecToRotate, vecAngles.z / 180. * M_PI);
		}
		else
		{
			RotateVectorZ(vecToRotate, vecAngles.z / 180. * M_PI);
			RotateVectorX(vecToRotate, vecAngles.x / 180. * M_PI);
			RotateVectorY(vecToRotate, vecAngles.y / 180. * M_PI);
		}

		vecToRotate = vecToRotate + vecOffset;
	}
}

Vector VRPhysicsHelper::AngularVelocityToLinearVelocity(const Vector& avelocity, const Vector& pos)
{
	Vector rotatedPos = pos;
	RotateVector(rotatedPos, avelocity);
	return rotatedPos - pos;
}

void VRPhysicsHelper::CreateInstance()
{
	assert(!m_instance);
	m_instance = new VRPhysicsHelper();
}

void VRPhysicsHelper::DestroyInstance()
{
	assert(m_instance);
	delete m_instance;
	m_instance = nullptr;
}

VRPhysicsHelper& VRPhysicsHelper::Instance()
{
	assert(m_instance);
	return *m_instance;
}

VRPhysicsHelper* VRPhysicsHelper::m_instance{ nullptr };
