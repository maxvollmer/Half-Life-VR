
// Only to be included by VRPhysicsHelper.cpp

using namespace reactphysics3d;

extern struct playermove_s* PM_GetPlayerMove(void);

// Constants
namespace
{
	constexpr const uint32_t HLVR_MAP_PHYSDATA_FILE_MAGIC = 'HLVR';
	constexpr const uint32_t HLVR_MAP_PHYSDATA_FILE_VERSION = 105;

	// Stuff needed to extract brush models
	// TODO: Use client.dll to get the address for the engine function that returns the actual model, including brush and sprite models
	constexpr const unsigned int ENGINE_MODEL_ARRAY_SIZE = 1024;
	const model_t* g_EngineModelArrayPointer = nullptr;

	constexpr const rp3d::decimal RP3D_TO_HL = 50.;
	constexpr const rp3d::decimal HL_TO_RP3D = 1. / RP3D_TO_HL;

	constexpr const int MAX_GAP_WIDTH = VEC_DUCK_HEIGHT;
	constexpr const int MAX_GAP_WIDTH_SQUARED = MAX_GAP_WIDTH * MAX_GAP_WIDTH;
	constexpr const int MIN_DISTANCE = 240 + MAX_GAP_WIDTH;

	constexpr const int PHYSICS_STEPS = 30;
	constexpr const int MAX_PHYSICS_STEPS = PHYSICS_STEPS * 1.5;
	constexpr const rp3d::decimal PHYSICS_STEP_TIME = 1. / PHYSICS_STEPS;

	constexpr const rp3d::decimal MAX_PHYSICS_TIME_PER_FRAME = 1. / 30.;  // Never drop below 30fps due to physics calculations

	constexpr const float LADDER_EPSILON = 4.f;

	enum NeedLoadFlag
	{
		IS_LOADED = 0,  // If set, this brush model is valid and loaded
		NEEDS_LOADING = 1,  // If set, this brush model still needs to be loaded
		UNREFERENCED = 2   // If set, this brush model isn't used by the current map
	};
}

#include "VRPhysicsRP3DCustomization.hpp"
#include "VRPhysicsHL2RP3D.hpp"
#include "VRPhysicsBSPModels.hpp"
#include "VRPhysicsTriangulateBSP.hpp"
#include "VRPhysicsIO.hpp"

// more helper functions (to be put into own hpp if grows too big)
namespace
{
	bool BodyIntersectsLine(rp3d::CollisionBody* body, const Vector& lineA, const Vector& lineB, Vector& hitPoint)
	{
		if (!body)
			return false;

		{
			Ray ray1{ HLVecToRP3DVec(lineA), HLVecToRP3DVec(lineB) };
			RaycastInfo raycastInfo1;
			if (body->raycast(ray1, raycastInfo1) && raycastInfo1.hitFraction < 1.f)
			{
				hitPoint = RP3DVecToHLVec(raycastInfo1.worldPoint);
				return true;
			}
		}

		{
			Ray ray2{ HLVecToRP3DVec(lineB), HLVecToRP3DVec(lineA) };
			RaycastInfo raycastInfo2;
			if (body->raycast(ray2, raycastInfo2) && raycastInfo2.hitFraction < 1.f)
			{
				hitPoint = RP3DVecToHLVec(raycastInfo2.worldPoint);
				return true;
			}
		}

		return false;
	}
}
