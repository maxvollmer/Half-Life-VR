
#pragma warning(disable : 4244)
/*
1>..\reactphysics3d\include\collision\shapes\heightfieldshape.h(227): warning C4244: 'return': conversion from 'double' to 'reactphysics3d::decimal', possible loss of data
1>..\reactphysics3d\include\collision\shapes\heightfieldshape.h(235): warning C4244: 'return': conversion from 'reactphysics3d::decimal' to 'int', possible loss of data
*/
#include "reactphysics3d\include\reactphysics3d.h"

#include "extdll.h"

#define HARDWARE_MODE
#include "pm_defs.h"
#include "plane.h"
#include "com_model.h"
extern struct playermove_s *PM_GetPlayerMove(void);

#include <fstream>
#include <memory>

#include "VRPhysicsHelper.h"
#include "util.h"
#include "cbase.h"

#include <chrono>

constexpr const unsigned int HLVR_MAP_PHYSDATA_FILE_MAGIC = 'HLVR';


// Returns a pointer to a model_t instance holding BSP data for this entity's BSP model (if it is a BSP model) - Max Vollmer, 2018-01-21
const model_t * GetBSPModel(CBaseEntity *pEntity)
{
	playermove_s *pmove = PM_GetPlayerMove();
	if (pmove != nullptr)
	{
		const char * modelname = STRING(pEntity->pev->model);
		if (modelname != nullptr && modelname[0] == '*')
		{
			model_t *models = pmove->physents[0].model;
			if (models != nullptr)
			{
				int modelindex = atoi(&modelname[1]);
				if (modelindex > 0 && strcmp(models[modelindex].name, modelname) == 0)
				{
					return &models[modelindex];
				}
			}
		}
	}
	return nullptr;
}

// Returns a pointer to a model_t instance holding BSP data for the current map - Max Vollmer, 2018-01-21
const model_t * GetWorldBSPModel()
{
	playermove_s *pmove = PM_GetPlayerMove();
	if (pmove != nullptr)
	{
		for (const physent_t & physent : pmove->physents)
		{
			if (physent.info == 0)
			{
				return physent.model;
			}
		}
	}
	return nullptr;
}

bool IsSolidInPhysicsWorld(CBaseEntity *pEntity)
{
	return FClassnameIs(pEntity->pev, "func_wall") || FClassnameIs(pEntity->pev, "func_illusionary");
}



using namespace reactphysics3d;

constexpr float HL_TO_RP3D = 0.01f;
constexpr float RP3D_TO_HL = 100.f;


constexpr int PLAYER_WIDTH = DUCK_SIZE;
constexpr int PLAYER_WIDTH_SQUARED = PLAYER_WIDTH * PLAYER_WIDTH;
constexpr float MIN_DISTANCE = 240 + PLAYER_WIDTH;

constexpr int PHYSICS_STEPS = 30;
constexpr int MAX_PHYSICS_STEPS = PHYSICS_STEPS * 1.5;
constexpr float PHYSICS_STEP_TIME = 1.f / PHYSICS_STEPS;

constexpr double MAX_PHYSICS_TIME_PER_FRAME = 1. / 30.;	// Never drop below 30fps due to physics calculations

inline Vector3 HLVecToRP3DVec(const Vector & hlVec)
{
	return Vector3{ hlVec.x * HL_TO_RP3D, hlVec.y * HL_TO_RP3D, hlVec.z * HL_TO_RP3D };
}

const std::hash<int> intHasher;

struct VectorHash : public std::unary_function<Vector, std::size_t>
{
	inline std::size_t operator()(const Vector& v) const
	{
		return intHasher(v.x * 10) ^ intHasher(v.y * 10) ^ intHasher(v.z * 10);
	}
};
struct VectorEqual : public std::binary_function<Vector, Vector, bool>
{
	inline bool operator()(const Vector& v1, const Vector& v2) const
	{
		return fabs(v1.x - v2.x) < EPSILON && fabs(v1.y - v2.y) < EPSILON && fabs(v1.z - v2.z) < EPSILON;
	}
};

enum class PlaneMajor {
	X,
	Y,
	Z
};

inline PlaneMajor PlaneMajorFromNormal(const Vector & normal)
{
	if (fabs(normal.x) > fabs(normal.y))
	{
		if (fabs(normal.x) > fabs(normal.z))
		{
			return PlaneMajor::X;
		}
		else
		{
			return PlaneMajor::Z;
		}
	}
	else
	{
		if (fabs(normal.y) > fabs(normal.z))
		{
			return PlaneMajor::Y;
		}
		else
		{
			return PlaneMajor::Z;
		}
	}
}

class VertexMetaData
{
public:
	unsigned int numFaces{ 0 };
	double totalCos{ 0 };
	bool handled{ false };
};

class TranslatedPlane
{
public:
	TranslatedPlane(const Vector & vert, const Vector & normal)
		: normal{ normal }
		, dist{ DotProduct(normal, vert) }
		, hash{ intHasher(dist) ^ intHasher(normal.x * 10) ^ intHasher(normal.y * 10) ^ intHasher(normal.z * 10) }
		, major{ PlaneMajorFromNormal(normal) }
	{
	}

	inline bool IsCoPlanar(const TranslatedPlane & other) const
	{
		return fabs(dist - other.dist) < EPSILON
			&& fabs(normal.x - other.normal.x) < EPSILON
			&& fabs(normal.y - other.normal.y) < EPSILON
			&& fabs(normal.z - other.normal.z) < EPSILON;
	}

	inline const Vector & GetNormal() const
	{
		return normal;
	}

	inline const PlaneMajor & GetMajor() const
	{
		return major;
	}

	inline const size_t GetA() const
	{
		switch (major)
		{
		case PlaneMajor::X: return 1;
		default: return 0;
		}
	}

	inline const size_t GetB() const
	{
		switch (major)
		{
		case PlaneMajor::Z: return 1;
		default: return 2;
		}
	}

	struct Hash : public std::unary_function<TranslatedPlane, std::size_t>
	{
		inline std::size_t operator()(const TranslatedPlane& e) const
		{
			return e.hash;
		}
	};
	struct Equal : public std::binary_function<TranslatedPlane, TranslatedPlane, bool>
	{
		inline bool operator()(const TranslatedPlane& e1, const TranslatedPlane& e2) const
		{
			return e1.IsCoPlanar(e2);
		}
	};

private:
	const Vector normal;
	const float dist;
	const std::size_t hash;
	const PlaneMajor major;
};

typedef std::unordered_map<TranslatedPlane, std::unordered_map<Vector, VertexMetaData, VectorHash, VectorEqual>, TranslatedPlane::Hash, TranslatedPlane::Equal> PlaneVertexMetaDataMap;

class TranslatedFace
{
public:
	TranslatedFace(const msurface_t & face, const Vector & origin, PlaneVertexMetaDataMap & planeVertexMetaData)
		: plane{ origin + face.polys->verts[0], FBitSet(face.flags, SURF_PLANEBACK) ? -face.plane->normal : face.plane->normal }
	{
		for (int i = 0; i < face.polys->numverts; ++i)
		{
			Vector & vertexBefore = origin + face.polys->verts[(i > 0) ? (i - 1) : (face.polys->numverts - 1)];
			Vector & vertex = origin + face.polys->verts[i];
			Vector & vertexAfter = origin + face.polys->verts[(i < (face.polys->numverts - 1)) ? (i + 1) : 0];

			float vertDot = DotProduct((vertexBefore - vertex).Normalize(), (vertexAfter - vertex).Normalize());

			if (fabs(vertDot + 1.f) < EPSILON)
			{
				// Colinear vertex, drop it!
			}
			else
			{
				vertices.push_back(vertex);
				planeVertexMetaData[plane][vertex].numFaces++;
				planeVertexMetaData[plane][vertex].totalCos += vertDot - 1.f;
			}
		}
		if (vertices.size() >= 3)
		{
			vecAnyPointInside = (vertices[0] + vertices[2]) / 2;
		}
	}

	inline const std::vector<Vector> & GetVertices() const
	{
		return vertices;
	}

	// Returns those vertices of A and B that enclose a gap between these faces (if one exists)
	bool GetGapVertices(const TranslatedFace & other, std::vector<Vector> & mergedVertices, PlaneVertexMetaDataMap & planeVertexMetaData) const
	{
		if (vertices.size() < 3 || other.vertices.size() < 3)
		{
			return false;
		}
		else if (IsCloseEnough(other))
		{
			return GetGapVerticesInPlane(other, mergedVertices, planeVertexMetaData, plane.GetA(), plane.GetB());
		}
		else
		{
			return false;
		}
	}

	inline const TranslatedPlane & GetPlane() const
	{
		return plane;
	}

private:
	inline bool IsCloseEnough(const TranslatedFace & other) const
	{
		// Since faces in a HL BSP are always maximum 240 units wide,
		// if any vertex from this face is further than MIN_DISTANCE (240 + player width) units
		// away from any vertex in the other face, these faces can't have a gap too narrow for a player
		return fabs(vertices[0].x - other.vertices[0].x) < MIN_DISTANCE
			&& fabs(vertices[0].y - other.vertices[0].y) < MIN_DISTANCE
			&& fabs(vertices[0].z - other.vertices[0].z) < MIN_DISTANCE;
	}

	bool GetGapVerticesInPlane(const TranslatedFace & other, std::vector<Vector> & mergedVertices, PlaneVertexMetaDataMap & planeVertexMetaData, const size_t a, const size_t b) const
	{
		// We only want the two outermost vertices of each face, since we don't care about overlapping triangles and we don't have to deal with arbitrary shapes this way - also keeps amount of triangles down
		std::vector<Vector> verticesA, verticesB;

		// Get direction from A to B
		const Vector faceDir = (other.vecAnyPointInside - vecAnyPointInside).Normalize();

		// Loop over all vertices and check if we have a gap
		for (const Vector & vec : vertices)
		{
			// Discard vertex if it's completely enclosed by other faces
			if (CVAR_GET_FLOAT("vr_debug_physics") >= 1.f && planeVertexMetaData[plane][vec].totalCos < (EPSILON - 4.f))
			{
				continue;
			}
			// Discard point if it's on the far side of this face
			if (DotProduct((vec - vecAnyPointInside).Normalize(), faceDir) < 0)
			{
				continue;
			}
			bool foundOther = false;
			for (const Vector & vecOther : other.vertices)
			{
				// Discard vertex if it's completely enclosed by other faces
				if (CVAR_GET_FLOAT("vr_debug_physics") >= 2.f && planeVertexMetaData[plane][vecOther].totalCos < (EPSILON - 4.f))
				{
					continue;
				}
				float distanceSquared = (vec - vecOther).LengthSquared();
				if (distanceSquared < (EPSILON*EPSILON))
				{
					// If any two vertices are equal (that is: if the faces are touching), we can safely cancel this algorithm,
					// because of what we know about the simple architecture used throughout Half-Life maps:
					// In Vanilla Half-Life no touching faces will ever form a gap too small for a player,
					// if there aren't also 2 other faces that do not touch each other (and those 2 will then produce the desired vertices for our gap triangles)
					return false;
				}
				if (distanceSquared <= PLAYER_WIDTH_SQUARED)
				{
					// Only take other point if it's not on the far side of the other face
					if (DotProduct((vecOther - other.vecAnyPointInside).Normalize(), faceDir) < 0)
					{
						if (verticesB.size() < 2)
						{
							verticesB.push_back(vecOther);
						}
						else
						{
							verticesB[1] = vecOther;
						}
						planeVertexMetaData[plane][vecOther].handled = true;
						foundOther = true;
					}
				}
			}
			if (foundOther)
			{
				if (verticesA.size() < 2)
				{
					verticesA.push_back(vec);
				}
				else
				{
					verticesA[1] = vec;
				}
				planeVertexMetaData[plane][vec].handled = true;
			}
		}

		// Add the vertices we found to mergedVertices in correct order, so triangulation can be done safely
		mergedVertices.insert(mergedVertices.end(), verticesA.begin(), verticesA.end());
		mergedVertices.insert(mergedVertices.end(), verticesB.rbegin(), verticesB.rend());

		// Only return true if we have at least 3 vertices!
		return mergedVertices.size() > 3;
	}

	const TranslatedPlane plane;
	std::vector<Vector> vertices;

public:
	Vector vecAnyPointInside;
};

typedef std::unordered_map<TranslatedPlane, std::vector<TranslatedFace>, TranslatedPlane::Hash, TranslatedPlane::Equal> PlaneFacesMap;

void CollectFaces(const model_t * model, const Vector & origin, PlaneFacesMap & planeFaces, PlaneVertexMetaDataMap & planeVertexMetaData)
{
	if (model != nullptr)
	{
		for (int i = 0; i < model->nummodelsurfaces; ++i)
		{
			TranslatedFace face{ model->surfaces[model->firstmodelsurface + i], origin, planeVertexMetaData };
			planeFaces[face.GetPlane()].push_back(face);
		}
	}
}

void TriangulateGapsBetweenCoplanarFaces(const TranslatedFace & faceA, const TranslatedFace & faceB, PlaneVertexMetaDataMap & planeVertexMetaData, std::vector<Vector3> & vertices, std::vector<int> & indices)
{
	std::vector<Vector> mergedVertices;
	if (faceA.GetGapVertices(faceB, mergedVertices, planeVertexMetaData))
	{
		// Now we simply triangulate mergedVertices, which is either 3 or 4 vertices in size
		int indexoffset = vertices.size();
		for (size_t i = 0; i < mergedVertices.size(); ++i)
		{
			vertices.push_back(HLVecToRP3DVec(mergedVertices[i]));
		}

		// a-b-c
		indices.push_back(indexoffset + 0); indices.push_back(indexoffset + 1); indices.push_back(indexoffset + 2);
		// a-c-b
		indices.push_back(indexoffset + 0); indices.push_back(indexoffset + 2); indices.push_back(indexoffset + 1);

		if (mergedVertices.size() == 4)
		{
			// a-c-d
			indices.push_back(indexoffset + 0); indices.push_back(indexoffset + 2); indices.push_back(indexoffset + 3);
			// a-d-c
			indices.push_back(indexoffset + 0); indices.push_back(indexoffset + 3); indices.push_back(indexoffset + 2);
		}
	}
}

void TriangulateBSPFaces(const PlaneFacesMap & planeFaces, PlaneVertexMetaDataMap & planeVertexMetaData, std::vector<Vector3> & vertices, std::vector<int> & indices)
{
	for (auto pair : planeFaces)
	{
		const TranslatedPlane & plane = pair.first;
		const std::vector<TranslatedFace> & faces = pair.second;
		for (size_t faceIndexA = 0; faceIndexA < faces.size(); ++faceIndexA)
		{
			const TranslatedFace & faceA = faces[faceIndexA];

			// First do normal triangulation of all surfaces
			int indexoffset = vertices.size();
			for (size_t i = 0; i < faceA.GetVertices().size(); ++i)
			{
				vertices.push_back(HLVecToRP3DVec(faceA.GetVertices()[i]));
				if (i < faceA.GetVertices().size() - 2)
				{
					indices.push_back(indexoffset + i + 2);
					indices.push_back(indexoffset + i + 1);
					indices.push_back(indexoffset);
				}
			}

			// Then do triangulation of gaps between faces that are too small for a player to fit through
			for (size_t faceIndexB = faceIndexA + 1; faceIndexB < faces.size(); ++faceIndexB)
			{
				const TranslatedFace & faceB = faces[faceIndexB];
				TriangulateGapsBetweenCoplanarFaces(faceA, faceB, planeVertexMetaData, vertices, indices);
			}
		}
	}
}

// Get vertices and indices of world (triangulation of bsp data (map and non-moving solid entities))
void TriangulateBSP(PlaneFacesMap & planeFaces, PlaneVertexMetaDataMap & planeVertexMetaData, std::vector<Vector3> & vertices, std::vector<int> & indices)
{
	// Collect faces of world
	CollectFaces(GetWorldBSPModel(), Vector{}, planeFaces, planeVertexMetaData);

	// Collect faces of bsp entities
	CBaseEntity *pEnt = nullptr;
	while (UTIL_FindEntityByFilter(&pEnt, IsSolidInPhysicsWorld))
	{
		CollectFaces(GetBSPModel(pEnt), pEnt->pev->origin, planeFaces, planeVertexMetaData);
	}

	// Triangulate all faces (also triangulates gaps between faces!)
	TriangulateBSPFaces(planeFaces, planeVertexMetaData, vertices, indices);
}

void RaycastPotentialVerticeGaps(RigidBody * dynamicMap, const Vector3 & checkVertexInPhysSpace, const Vector3 & checkVertexAfterInPhysSpace, const Vector3 & checkDirInPhysSpace, const Vector3 & correctionalOffsetInPhysSpace, std::vector<Vector3> & vertices, std::vector<int> & indices, const size_t vertexIndexOffset)
{
	RaycastInfo raycastInfo1{};
	RaycastInfo raycastInfo2{};
	if (dynamicMap->raycast({ checkVertexInPhysSpace, checkVertexInPhysSpace + checkDirInPhysSpace }, raycastInfo1)
		&& raycastInfo1.hitFraction > 0.f
		&& dynamicMap->raycast({ checkVertexAfterInPhysSpace, checkVertexAfterInPhysSpace + checkDirInPhysSpace }, raycastInfo2)
		&& raycastInfo2.hitFraction > 0.f)
	{
		size_t indexoffset = vertexIndexOffset + vertices.size();
		vertices.push_back(checkVertexInPhysSpace + correctionalOffsetInPhysSpace);
		vertices.push_back(checkVertexAfterInPhysSpace - correctionalOffsetInPhysSpace);
		vertices.push_back(raycastInfo2.worldPoint - correctionalOffsetInPhysSpace);
		vertices.push_back(raycastInfo1.worldPoint + correctionalOffsetInPhysSpace);
		indices.push_back(indexoffset + 0); indices.push_back(indexoffset + 1); indices.push_back(indexoffset + 2);
		indices.push_back(indexoffset + 0); indices.push_back(indexoffset + 2); indices.push_back(indexoffset + 1);
		indices.push_back(indexoffset + 0); indices.push_back(indexoffset + 2); indices.push_back(indexoffset + 3);
		indices.push_back(indexoffset + 0); indices.push_back(indexoffset + 3); indices.push_back(indexoffset + 2);
	}
}

void FindAdditionalPotentialGapsBetweenMapFacesUsingPhysicsEngine(RigidBody * dynamicMap, const PlaneFacesMap & planeFaces, PlaneVertexMetaDataMap & planeVertexMetaData, std::vector<Vector3> & vertices, std::vector<int> & indices, const size_t vertexIndexOffset)
{
	// There are potential gaps left, that our first algorithm couldn't find.
	// We marked all potential vertices and can now just loop over them and then use the physics engine to check for remaining gaps.
	for (auto pair : planeFaces)
	{
		const TranslatedPlane & plane = pair.first;
		for (const TranslatedFace & face : pair.second)
		{
			auto vertexMetaDataMap = planeVertexMetaData[plane];
			for (size_t i = 0; i < face.GetVertices().size(); ++i)
			{
				const Vector & vertex = face.GetVertices()[i];
				const VertexMetaData & vertexMetaData = vertexMetaDataMap[vertex];
				if (vertexMetaData.totalCos >(EPSILON - 4.f) && !vertexMetaData.handled)
				{
					const Vector & vertexAfter = face.GetVertices()[(i < face.GetVertices().size() - 1) ? (i + 1) : 0];
					const VertexMetaData & vertexMetaDataAfter = vertexMetaDataMap[vertexAfter];
					if (vertexMetaDataAfter.totalCos > (EPSILON - 4.f) && !vertexMetaDataAfter.handled)
					{
						Vector dir{ vertex - vertexAfter };
						dir.InlineNormalize();

						Vector checkVertex{ vertex - dir };
						Vector checkVertexAfter{ vertexAfter + dir };

						const Vector checkDir1 = CrossProduct(dir, plane.GetNormal()) * DUCK_SIZE;
						const Vector checkDir2 = plane.GetNormal() * DUCK_SIZE;

						const Vector3 checkVertexInPhysSpace = HLVecToRP3DVec(checkVertex);
						const Vector3 checkVertexAfterInPhysSpace = HLVecToRP3DVec(checkVertexAfter);
						const Vector3 checkDir1InPhysSpace = HLVecToRP3DVec(checkDir1);
						const Vector3 checkDir2InPhysSpace = HLVecToRP3DVec(checkDir2);
						const Vector3 dirInPhysSpace = HLVecToRP3DVec(dir);

						RaycastPotentialVerticeGaps(dynamicMap, checkVertexInPhysSpace, checkVertexAfterInPhysSpace, checkDir1InPhysSpace, dirInPhysSpace, vertices, indices, vertexIndexOffset);
						RaycastPotentialVerticeGaps(dynamicMap, checkVertexInPhysSpace, checkVertexAfterInPhysSpace, checkDir2InPhysSpace, dirInPhysSpace, vertices, indices, vertexIndexOffset);
					}
				}
			}
		}
	}
}






VRPhysicsHelper::VRPhysicsHelper()
{
}

VRPhysicsHelper::~VRPhysicsHelper()
{
	//if (m_collisionWorld) delete m_collisionWorld;
	if (m_dynamicsWorld) delete m_dynamicsWorld;
	if (m_sphereShape) delete m_sphereShape;

	if (m_concaveMeshShape) delete m_concaveMeshShape;
	if (m_triangleMesh) delete m_triangleMesh;
	if (m_triangleArray) delete m_triangleArray;
}

bool VRPhysicsHelper::CheckIfLineIsBlocked(const Vector & hlPos1, const Vector &hlPos2)
{
	Vector dummy;
	return CheckIfLineIsBlocked(hlPos1, hlPos2, dummy);
}

bool VRPhysicsHelper::CheckIfLineIsBlocked(const Vector & hlPos1, const Vector &hlPos2, Vector & hlResult)
{
	if (!CheckWorld())
	{
		// no world, no trace
		hlResult = hlPos2;
		return false;
	}

	Vector3 pos1 = HLVecToRP3DVec(hlPos1);
	Vector3 pos2 = HLVecToRP3DVec(hlPos2);

	Ray ray{ pos1 , pos2, 1.f };
	RaycastInfo raycastInfo{};

	bool result = m_dynamicMap->raycast(ray, raycastInfo) && raycastInfo.hitFraction < (1.f - EPSILON);

	if (result)
	{
		hlResult.x = raycastInfo.worldPoint.x * RP3D_TO_HL;
		hlResult.y = raycastInfo.worldPoint.y * RP3D_TO_HL;
		hlResult.z = raycastInfo.worldPoint.z * RP3D_TO_HL;
	}
	else
	{
		hlResult = hlPos2;
	}

	return result;


	/*
	// Get trace dir and length
	Vector3 dir = pos2 - pos1;

	float lengthSquare = dir.lengthSquare();
	float length = sqrt(lengthSquare);

	// Position sphere at start of trace
	m_dynamicSphere->setTransform(rp3d::Transform(pos1, Quaternion::identity()));

	// Set velocity of sphere
	m_dynamicSphere->setLinearVelocity(dir);

	// Make sure sphere isn't put to sleep too early
	m_dynamicsWorld->setSleepLinearVelocity(length * 0.01f);

	// Let sphere fall down until it rests or has passed end of trace,
	// or until MAX_ITERATIONS or MAX_PHYSICS_TIME_PER_FRAME has reached (if it takes too long to calculate the path of the sphere, we can assume that it is blocked)
	int numSteps = 0;
	auto start = std::chrono::system_clock::now();
	std::chrono::duration<double> timePassed;
	do
	{
		m_dynamicSphere->setLinearVelocity(dir);
		m_dynamicsWorld->update(PHYSICS_STEP_TIME);
		timePassed = std::chrono::system_clock::now() - start;
		++numSteps;
	}
	while (
		numSteps < MAX_PHYSICS_STEPS						// Cancel loop if we need too many steps to calculate sphere's path (heuristic: probably blocked)
		&& timePassed.count() < MAX_PHYSICS_TIME_PER_FRAME	// Cancel loop if we need too much time to calculate sphere's path (heuristic: probably blocked)
		&& !m_dynamicSphere->isSleeping()					// Cancel loop if sphere stopped moving (blocked)
		&& lengthSquare > (m_dynamicSphere->getTransform().getPosition() - pos1).lengthSquare()		// Cancel loop if sphere is too far away (reached or passed end point)
		&& !m_dynamicSphere->testPointInside(pos2)													// Cancel loop if sphere reached end point
		&& (m_dynamicSphere->testPointInside(pos1) || m_dynamicSphere->raycast(ray, raycastInfo)));	// Cancel loop if sphere is not within path anymore (blocked, "rolled" away)

	ALERT(at_console, "physics steps: %i, time passed: %f\n", numSteps, timePassed.count());

	hlResult.x = m_dynamicSphere->getTransform().getPosition().x * RP3D_TO_HL;
	hlResult.y = m_dynamicSphere->getTransform().getPosition().y * RP3D_TO_HL;
	hlResult.z = m_dynamicSphere->getTransform().getPosition().z * RP3D_TO_HL;

	// Check if sphere reached endpoint
	return UTIL_PointInsideBBox(hlPos2, hlResult - VEC_DUCK_RADIUS, hlResult + VEC_DUCK_RADIUS);
	*/
}

void VRPhysicsHelper::InitPhysicsWorld()
{
	if (m_dynamicsWorld == nullptr)
	{
		//m_collisionWorld = new CollisionWorld{};
		m_dynamicsWorld = new DynamicsWorld{ Vector3{ 0, 0, 0 } };

		m_dynamicsWorld->setNbIterationsVelocitySolver(16);
		m_dynamicsWorld->setNbIterationsPositionSolver(16);

		//m_dynamicsWorld->setSleepAngularVelocity(9999);
		m_dynamicsWorld->setTimeBeforeSleep({ PHYSICS_STEP_TIME * 1.5f });

		m_dynamicMap = m_dynamicsWorld->createRigidBody(rp3d::Transform::identity());
		m_dynamicMap->setType(STATIC);
		m_dynamicMap->setLinearDamping(0);
		m_dynamicMap->setAngularDamping(0);
		m_dynamicMap->getMaterial().setBounciness(0);
		m_dynamicMap->getMaterial().setFrictionCoefficient(0);
		m_dynamicMap->getMaterial().setRollingResistance(0);

		m_dynamicSphere = m_dynamicsWorld->createRigidBody(rp3d::Transform::identity());
		m_dynamicSphere->setType(DYNAMIC);
		m_dynamicSphere->setLinearDamping(0);
		m_dynamicSphere->setAngularDamping(0);
		m_dynamicSphere->getMaterial().setBounciness(0);
		m_dynamicSphere->getMaterial().setFrictionCoefficient(0);
		m_dynamicSphere->getMaterial().setRollingResistance(0);

		m_sphereShape = new SphereShape{ DUCK_RADIUS * HL_TO_RP3D };
		m_dynamicSphere->addCollisionShape(m_sphereShape, rp3d::Transform::identity(), 10.f);
	}
}

void VRPhysicsHelper::DeletePhysicsMapData()
{
	if (m_dynamicMapProxyShape)
	{
		m_dynamicMap->removeCollisionShape(m_dynamicMapProxyShape);
		m_dynamicMapProxyShape = nullptr;
	}

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

	if (m_triangleArray)
	{
		delete m_triangleArray;
		m_triangleArray = nullptr;
	}
}

void VRPhysicsHelper::CreateMapShapeFromCurrentVerticesAndTriangles()
{
	DeletePhysicsMapData();

	m_triangleArray = new TriangleVertexArray(
		m_vertices.size(), m_vertices.data(), sizeof(Vector),
		m_indices.size() / 3, m_indices.data(), sizeof(int),
		TriangleVertexArray::VERTEX_FLOAT_TYPE, TriangleVertexArray::INDEX_INTEGER_TYPE);

	m_triangleMesh = new TriangleMesh;
	m_triangleMesh->addSubpart(m_triangleArray);

	m_concaveMeshShape = new ConcaveMeshShape(m_triangleMesh);

	m_dynamicMapProxyShape = m_dynamicMap->addCollisionShape(m_concaveMeshShape, rp3d::Transform::identity(), 1);
}

bool IsWorldValid(const model_t *world)
{
	return world != nullptr && world->needload == 0 && world->marksurfaces[0] != nullptr && world->marksurfaces[0]->polys != nullptr;
}

bool CompareWorlds(const model_t *world1, const model_t *worl2)
{
	return world1 == worl2 && world1 != nullptr && FStrEq(world1->name, worl2->name);
}

template <typename T>
void WriteBinaryData(std::ofstream& out, const T& value)
{
	out.write(reinterpret_cast<const char*>(&value), sizeof(value));
}

template <typename T>
void ReadBinaryData(std::ifstream& in, T& value)
{
	in.read(reinterpret_cast<char*>(&value), sizeof(value));
	if (in.eof())
	{
		throw std::runtime_error{ "unexpected eof" };
	}
	if (!in.good())
	{
		throw std::runtime_error{ "unexpected error" };
	}
}

bool VRPhysicsHelper::GetPhysicsMapDataFromFile(const std::string& physicsMapDataFilePath)
{
	try
	{
		std::ifstream physicsMapDataFileStream{ physicsMapDataFilePath, std::ios_base::in | std::ios_base::binary };
		if (!physicsMapDataFileStream.fail() && physicsMapDataFileStream.good())
		{
			unsigned int magic = 0;
			unsigned long long int hash = 0;
			unsigned int verticesCount = 0;
			unsigned int indicesCount = 0;

			ReadBinaryData(physicsMapDataFileStream, magic);
			ReadBinaryData(physicsMapDataFileStream, hash);
			ReadBinaryData(physicsMapDataFileStream, verticesCount);
			ReadBinaryData(physicsMapDataFileStream, indicesCount);

			// TODO: hash is ignored for now
			if (magic == HLVR_MAP_PHYSDATA_FILE_MAGIC /*&& hash == TODO*/ && verticesCount > 0 && indicesCount > 0)
			{
				DeletePhysicsMapData();
				m_vertices.clear();
				m_indices.clear();

				for (unsigned int i = 0; i < verticesCount; ++i)
				{
					reactphysics3d::Vector3 vertex;
					ReadBinaryData(physicsMapDataFileStream, vertex.x);
					ReadBinaryData(physicsMapDataFileStream, vertex.y);
					ReadBinaryData(physicsMapDataFileStream, vertex.z);
					m_vertices.push_back(vertex);
				}

				for (unsigned int i = 0; i < indicesCount; ++i)
				{
					int index = 0;
					ReadBinaryData(physicsMapDataFileStream, index);
					if (index < 0 || static_cast<unsigned>(index) > m_vertices.size())
					{
						throw std::runtime_error{ "invalid data: index out of bounds!" };
					}
					m_indices.push_back(index);
				}

				char checkeof;
				if (physicsMapDataFileStream >> checkeof) // this should fail
				{
					throw std::runtime_error{ "expected eof, but didn't reach eof" };
				}

				CreateMapShapeFromCurrentVerticesAndTriangles();

				ALERT(at_console, "Successfully loaded physics data from %s\n", physicsMapDataFilePath.c_str());
				return true;
			}
			else
			{
				throw std::runtime_error{ "invalid header" };
			}
		}
		else
		{
			ALERT(at_console, "Game must recalculate physics data because %s doesn't exist or couldn't be opened.\n", physicsMapDataFilePath.c_str());
			return false;
		}
	}
	catch (const std::runtime_error& e)
	{
		ALERT(at_console, "Game must recalculate physics data due to error while trying to parse %s: %s\n", physicsMapDataFilePath.c_str(), e.what());
		m_vertices.clear();
		m_indices.clear();
		return false;
	}
}

void VRPhysicsHelper::StorePhysicsMapDataToFile(const std::string& physicsMapDataFilePath)
{
	std::ofstream physicsMapDataFileStream{ physicsMapDataFilePath, std::ios_base::out | std::ios_base::trunc | std::ios_base::binary };
	if (!physicsMapDataFileStream.fail())
	{
		const unsigned long long int hash = 1337; // TODO!

		WriteBinaryData(physicsMapDataFileStream, HLVR_MAP_PHYSDATA_FILE_MAGIC);
		WriteBinaryData(physicsMapDataFileStream, hash);
		WriteBinaryData(physicsMapDataFileStream, static_cast<unsigned int>(m_vertices.size()));
		WriteBinaryData(physicsMapDataFileStream, static_cast<unsigned int>(m_indices.size()));

		for (unsigned int i = 0; i < m_vertices.size(); ++i)
		{
			const reactphysics3d::Vector3& vertex = m_vertices[i];
			WriteBinaryData(physicsMapDataFileStream, vertex.x);
			WriteBinaryData(physicsMapDataFileStream, vertex.y);
			WriteBinaryData(physicsMapDataFileStream, vertex.z);
		}

		for (unsigned int i = 0; i < m_indices.size(); ++i)
		{
			int index = m_indices.at(i);
			WriteBinaryData(physicsMapDataFileStream, index);
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
	DeletePhysicsMapData();

	m_vertices.clear();
	m_indices.clear();

	PlaneFacesMap planeFaces;
	PlaneVertexMetaDataMap planeVertexMetaData;

	TriangulateBSP(planeFaces, planeVertexMetaData, m_vertices, m_indices);

	CreateMapShapeFromCurrentVerticesAndTriangles();

	std::vector<Vector3> additionalVertices;
	std::vector<int> additionalIndices;
	FindAdditionalPotentialGapsBetweenMapFacesUsingPhysicsEngine(m_dynamicMap, planeFaces, planeVertexMetaData, additionalVertices, additionalIndices, m_vertices.size());

	DeletePhysicsMapData();

	m_vertices.insert(m_vertices.end(), additionalVertices.begin(), additionalVertices.end());
	m_indices.insert(m_indices.end(), additionalIndices.begin(), additionalIndices.end());

	CreateMapShapeFromCurrentVerticesAndTriangles();
}

bool VRPhysicsHelper::CheckWorld()
{
	const model_t *world = GetWorldBSPModel();

	if (!CompareWorlds(world, m_hlWorldModel) && IsWorldValid(world))
	{
		if (m_dynamicsWorld == nullptr)
		{
			InitPhysicsWorld();
		}

		const std::string mapFileName{ world->name };
		const std::string mapName{ mapFileName.substr(0, mapFileName.find_last_of(".")) };
		const std::string physicsMapDataFilePath{ UTIL_GetGameDir() + "/" + mapName + ".hlvrphysdata" };

		ALERT(at_console, "Initializing physics data for current map...\n");
		auto start = std::chrono::system_clock::now();
		std::chrono::duration<double> timePassed;

		if (!GetPhysicsMapDataFromFile(physicsMapDataFilePath))
		{
			GetPhysicsMapDataFromModel();
			StorePhysicsMapDataToFile(physicsMapDataFilePath);
		}

		timePassed = std::chrono::system_clock::now() - start;
		ALERT(at_console, "...initialized physics data in %f seconds.\n", timePassed.count());

		m_hlWorldModel = world;
	}

	return m_hlWorldModel != nullptr && m_dynamicsWorld != nullptr;
}

void RotateVectorX(Vector &vecToRotate, const float angle)
{
	if (angle != 0.f)
	{
		float fCos = cos(angle);
		float fSin = sin(angle);

		float x = vecToRotate.x;
		float z = vecToRotate.z;

		vecToRotate.x = fCos*x + fSin*z;
		vecToRotate.z = -fSin*x + fCos*z;
	}
}

void RotateVectorY(Vector &vecToRotate, const float angle)
{
	if (angle != 0.f)
	{
		float fCos = cos(angle);
		float fSin = sin(angle);

		float x = vecToRotate.x;
		float y = vecToRotate.y;

		vecToRotate.x = fCos*x - fSin*y;
		vecToRotate.y = fSin*x + fCos*y;
	}
}

void RotateVectorZ(Vector &vecToRotate, const float angle)
{
	if (angle != 0.f)
	{
		float fCos = cos(angle);
		float fSin = sin(angle);

		float y = vecToRotate.y;
		float z = vecToRotate.z;

		vecToRotate.y = fCos*y - fSin*z;
		vecToRotate.z = fSin*y + fCos*z;
	}
}

void VRPhysicsHelper::RotateVector(Vector &vecToRotate, const Vector &vecAngles, const Vector &vecOffset, const bool reverse)
{
	if (vecToRotate.Length() != 0)
	{
		vecToRotate = vecToRotate - vecOffset;

		if (reverse)
		{
			RotateVectorY(vecToRotate, vecAngles.y / 180.*M_PI);
			RotateVectorX(vecToRotate, vecAngles.x / 180.*M_PI);
			RotateVectorZ(vecToRotate, vecAngles.z / 180.*M_PI);
		}
		else
		{
			RotateVectorZ(vecToRotate, vecAngles.z / 180.*M_PI);
			RotateVectorX(vecToRotate, vecAngles.x / 180.*M_PI);
			RotateVectorY(vecToRotate, vecAngles.y / 180.*M_PI);
		}

		vecToRotate = vecToRotate + vecOffset;
	}
}

Vector VRPhysicsHelper::AngularVelocityToLinearVelocity(const Vector & avelocity, const Vector & pos)
{
	Vector rotatedPos = pos;
	RotateVector(rotatedPos, avelocity);
	return rotatedPos - pos;
}

VRPhysicsHelper gVRPhysicsHelper;