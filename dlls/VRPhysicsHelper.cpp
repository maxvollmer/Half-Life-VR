
#pragma warning(disable : 4244)
/*
Disable warnings for:
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

constexpr const uint32_t HLVR_MAP_PHYSDATA_FILE_MAGIC = 'HLVR';
constexpr const uint32_t HLVR_MAP_PHYSDATA_FILE_VERSION = 100;

// Stuff needed to extract brush models
constexpr const unsigned int ENGINE_MODEL_ARRAY_SIZE = 1024;
const model_t* g_EngineModelArrayPointer = nullptr;

enum NeedLoadFlag {
	IS_LOADED = 0,		// If set, this brush model is valid and loaded
	NEEDS_LOADING = 1,	// If set, this brush model still needs to be loaded
	UNREFERENCED = 2	// If set, this brush model isn't used by the current map
};

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

bool IsValidSpriteName(const char* name)
{
	return name[0] != 0 && name[63] == 0 && std::string(name).find(".spr") == strlen(name) - 4;
}

const model_t* GetEngineModelArray()
{
	if (g_EngineModelArrayPointer == nullptr)
	{
		// Get the world model and check that it's valid (loaded)
		const model_t *worldmodel = GetWorldBSPModel();
		if (worldmodel != nullptr && worldmodel->needload == 0)
		{
			g_EngineModelArrayPointer = worldmodel;

			// Walk backwards to find start of array (there might be cached sprites in the array before the first map got loaded)
			while ((g_EngineModelArrayPointer - 1)->type == mod_sprite && IsValidSpriteName((g_EngineModelArrayPointer - 1)->name))
			{
				g_EngineModelArrayPointer--;
			}
		}
	}
	return g_EngineModelArrayPointer;
}

const model_t* FindEngineModelByName(const char* name)
{
	const model_t* modelarray = GetEngineModelArray();
	if (modelarray != nullptr)
	{
		for (size_t i = 0; i < ENGINE_MODEL_ARRAY_SIZE; ++i)
		{
			if (modelarray[i].needload == IS_LOADED && strcmp(modelarray[i].name, name) == 0)
			{
				return modelarray + i;
			}
		}
	}
	return nullptr;
}

// Returns a pointer to a model_t instance holding BSP data for this entity's BSP model (if it is a BSP model) - Max Vollmer, 2018-01-21
const model_t * GetBSPModel(CBaseEntity *pEntity)
{
	// Check if entity is world
	if (pEntity->edict() == INDEXENT(0))
		return GetWorldBSPModel();

	const char* modelname = STRING(pEntity->pev->model);

	// Check that the entity has a brush model
	if (modelname != nullptr && modelname[0] == '*')
	{
		return FindEngineModelByName(modelname);
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

inline Vector RP3DVecToHLVec(const Vector3 & rp3dVec)
{
	return Vector{ rp3dVec.x * RP3D_TO_HL, rp3dVec.y * RP3D_TO_HL, rp3dVec.z * RP3D_TO_HL };
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

size_t CollectFaces(const model_t * model, const Vector & origin, PlaneFacesMap & planeFaces, PlaneVertexMetaDataMap & planeVertexMetaData)
{
	size_t faceCount = 0;
	if (model != nullptr)
	{
		for (int i = 0; i < model->nummodelsurfaces; ++i)
		{
			TranslatedFace face{ model->surfaces[model->firstmodelsurface + i], origin, planeVertexMetaData };
			planeFaces[face.GetPlane()].push_back(face);
			++faceCount;
		}
	}
	return faceCount;
}

void TriangulateGapsBetweenCoplanarFaces(const TranslatedFace & faceA, const TranslatedFace & faceB, PlaneVertexMetaDataMap & planeVertexMetaData, std::vector<Vector3> & vertices, std::vector<int> & indices)
{
	std::vector<Vector> mergedVertices;
	if (faceA.GetGapVertices(faceB, mergedVertices, planeVertexMetaData))
	{
		// Get rid of duplicate vertices
		mergedVertices.erase(std::unique(mergedVertices.begin(), mergedVertices.end()), mergedVertices.end());

		// Only add vertices if we have at least 3 (for one triangle)
		if (mergedVertices.size() >= 3)
		{
			// Add vertices
			int indexoffset = vertices.size();
			for (size_t i = 0; i < mergedVertices.size(); ++i)
			{
				vertices.push_back(HLVecToRP3DVec(mergedVertices[i]));
			}

			// Triangulate vertices

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

			// Get face vertices
			std::vector<Vector3> faceVertices;
			for (size_t i = 0; i < faceA.GetVertices().size(); ++i)
			{
				faceVertices.push_back(HLVecToRP3DVec(faceA.GetVertices()[i]));
			}

			// Get rid of duplicate vertices
			faceVertices.erase(std::unique(faceVertices.begin(), faceVertices.end()), faceVertices.end());

			// Skip invalid face
			if (faceVertices.size() < 3)
				continue;

			// Triangulate
			int indexoffset = vertices.size();
			for (size_t i = 0; i < faceVertices.size() - 2; ++i)
			{
				indices.push_back(indexoffset + i + 2);
				indices.push_back(indexoffset + i + 1);
				indices.push_back(indexoffset);
			}

			// Add vertices
			vertices.insert(vertices.end(), faceVertices.begin(), faceVertices.end());

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
		// Get gap vertices
		std::vector<Vector3> gapVertices;
		gapVertices.push_back(checkVertexInPhysSpace + correctionalOffsetInPhysSpace);
		gapVertices.push_back(checkVertexAfterInPhysSpace - correctionalOffsetInPhysSpace);
		gapVertices.push_back(raycastInfo2.worldPoint - correctionalOffsetInPhysSpace);
		gapVertices.push_back(raycastInfo1.worldPoint + correctionalOffsetInPhysSpace);

		// Remove duplicates
		gapVertices.erase(std::unique(gapVertices.begin(), gapVertices.end()), gapVertices.end());

		// Add vertices if at least 3
		if (gapVertices.size() >= 3)
		{
			size_t indexoffset = vertexIndexOffset + vertices.size();
			vertices.insert(vertices.end(), gapVertices.begin(), gapVertices.end());
			indices.push_back(indexoffset + 0); indices.push_back(indexoffset + 1); indices.push_back(indexoffset + 2);
			indices.push_back(indexoffset + 0); indices.push_back(indexoffset + 2); indices.push_back(indexoffset + 1);
			if (gapVertices.size() == 4)
			{
				indices.push_back(indexoffset + 0); indices.push_back(indexoffset + 2); indices.push_back(indexoffset + 3);
				indices.push_back(indexoffset + 0); indices.push_back(indexoffset + 3); indices.push_back(indexoffset + 2);
			}
		}
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
	DeletePhysicsMapData();

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
		hlResult = RP3DVecToHLVec(raycastInfo.worldPoint);
	}
	else
	{
		hlResult = hlPos2;
	}

	return result;
}

constexpr const float LADDER_EPSILON = 4.f;

void VRPhysicsHelper::TraceLine(const Vector &vecStart, const Vector &vecEnd, edict_t* pentIgnore, TraceResult *ptr)
{
	UTIL_TraceLine(vecStart, vecEnd, ignore_monsters, dont_ignore_glass, pentIgnore, ptr);

	if (CheckWorld())
	{
		Vector3 pos1 = HLVecToRP3DVec(vecStart);
		Vector3 pos2 = HLVecToRP3DVec(vecEnd);

		Ray ray{ pos1, pos2, 1.f };
		RaycastInfo raycastInfo{};

		if (m_dynamicMap->raycast(ray, raycastInfo) && raycastInfo.hitFraction < ptr->flFraction)
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
		CBaseEntity *pEntity = nullptr;
		while (UTIL_FindAllEntities(&pEntity))
		{
			if (FClassnameIs(pEntity->pev, "func_ladder"))
			{
				Vector3 ladderSize = HLVecToRP3DVec(pEntity->pev->size);
				Vector3 ladderPosition = HLVecToRP3DVec(pEntity->pev->absmin);

				if (ladderSize.x <= 0.f
					|| ladderSize.y <= 0.f
					|| ladderSize.z <= 0.f)
				{
					ALERT(at_console, "Warning, found ladder with invalid size: %s (%f %f %f)\n", STRING(pEntity->pev->targetname), pEntity->pev->size.x, pEntity->pev->size.y, pEntity->pev->size.z);
					continue;
				}

				BoxShape boxShape{ ladderSize };

				RigidBody* body = m_dynamicsWorld->createRigidBody(rp3d::Transform::identity());
				body->setType(BodyType::STATIC);
				body->addCollisionShape(&boxShape, rp3d::Transform{ ladderPosition, Matrix3x3::identity() }, 10.f);

				if (body->raycast(ray, raycastInfo))
				{
					Vector hitPoint = RP3DVecToHLVec(raycastInfo.worldPoint);
					float distanceToPreviousPoint = (hitPoint - ptr->vecEndPos).Length();
					if (raycastInfo.hitFraction <= ptr->flFraction
						|| distanceToPreviousPoint < LADDER_EPSILON)
					{
						ptr->vecEndPos = RP3DVecToHLVec(raycastInfo.worldPoint);
						ptr->vecPlaneNormal = RP3DVecToHLVec(raycastInfo.worldNormal).Normalize();
						ptr->flPlaneDist = DotProduct(ptr->vecPlaneNormal, -ptr->vecEndPos);

						ptr->fAllSolid = false;
						ptr->fInOpen = false;
						ptr->fInWater = UTIL_PointContents(ptr->vecEndPos) == CONTENTS_WATER;
						ptr->flFraction = raycastInfo.hitFraction;
						ptr->iHitgroup = 0;
						ptr->pHit = pEntity->edict();
					}
				}

				m_dynamicsWorld->destroyRigidBody(body);
			}
		}
	}
}

void VRPhysicsHelper::InitPhysicsWorld()
{
	if (m_dynamicsWorld == nullptr)
	{
		m_dynamicsWorld = new DynamicsWorld{ Vector3{ 0, 0, 0 } };

		m_dynamicsWorld->setNbIterationsVelocitySolver(16);
		m_dynamicsWorld->setNbIterationsPositionSolver(16);

		m_dynamicsWorld->setTimeBeforeSleep({ PHYSICS_STEP_TIME * 1.5f });

		m_dynamicMap = m_dynamicsWorld->createRigidBody(rp3d::Transform::identity());
		m_dynamicMap->setType(BodyType::STATIC);
		m_dynamicMap->setLinearDamping(0);
		m_dynamicMap->setAngularDamping(0);
		m_dynamicMap->getMaterial().setBounciness(0);
		m_dynamicMap->getMaterial().setFrictionCoefficient(0);
		m_dynamicMap->getMaterial().setRollingResistance(0);

		m_dynamicSphere = m_dynamicsWorld->createRigidBody(rp3d::Transform::identity());
		m_dynamicSphere->setType(BodyType::DYNAMIC);
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

void TEMPTODO_RemoveInvalidTriangles(std::vector<Vector3> & vertices, std::vector<int32_t>& indices)
{
	// remove invalid triangles (TODO: Find out where these come from, this shouldn't be happening anymore!)
	for (uint32_t i = 0; i < indices.size(); i += 3)
	{
		if (vertices[indices[i]] == vertices[indices[i + 1]]
			|| vertices[indices[i]] == vertices[indices[i + 2]]
			|| vertices[indices[i + 1]] == vertices[indices[i + 2]])
		{
			ALERT(at_console, "(VRPhysicsHelper)Warning: Found invalid triangle at index %i, removed!\n", i);
			indices[i] = -1;
			indices[i + 1] = -1;
			indices[i + 2] = -1;
		}
	}
	indices.erase(std::remove_if(indices.begin(), indices.end(), [](const int32_t& i) { return i < 0; }), indices.end());
}

void VRPhysicsHelper::CreateMapShapeFromCurrentVerticesAndTriangles()
{
	DeletePhysicsMapData();

	TEMPTODO_RemoveInvalidTriangles(m_vertices, m_indices);

	m_triangleArray = new TriangleVertexArray(
		m_vertices.size(), m_vertices.data(), sizeof(Vector),
		m_indices.size() / 3, m_indices.data(), sizeof(int32_t) * 3,
		TriangleVertexArray::VertexDataType::VERTEX_FLOAT_TYPE, TriangleVertexArray::IndexDataType::INDEX_INTEGER_TYPE);

	m_triangleMesh = new TriangleMesh;
	m_triangleMesh->addSubpart(m_triangleArray);

	m_concaveMeshShape = new ConcaveMeshShape(m_triangleMesh);

	m_dynamicMapProxyShape = m_dynamicMap->addCollisionShape(m_concaveMeshShape, rp3d::Transform::identity(), 1);
}

bool DoesAnyBrushModelNeedLoading(const model_t *const models)
{
	CBaseEntity *pEnt = nullptr;
	while (UTIL_FindEntityByFilter(&pEnt, IsSolidInPhysicsWorld))
	{
		const model_t *const model = GetBSPModel(pEnt);
		if (model == nullptr || model->needload != 0)
		{
			return true;
		}
	}
	return false;
}

bool IsWorldValid(const model_t *world)
{
	return
		world != nullptr
		&& world->needload == 0
		&& world->marksurfaces[0] != nullptr
		&& world->marksurfaces[0]->polys != nullptr
		&& !DoesAnyBrushModelNeedLoading(world);
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
			uint32_t magic = 0;
			uint32_t version = 0;
			uint64_t hash = 0;
			uint32_t verticesCount = 0;
			uint32_t indicesCount = 0;

			ReadBinaryData(physicsMapDataFileStream, magic);
			ReadBinaryData(physicsMapDataFileStream, version);
			ReadBinaryData(physicsMapDataFileStream, hash);
			ReadBinaryData(physicsMapDataFileStream, verticesCount);
			ReadBinaryData(physicsMapDataFileStream, indicesCount);

			// TODO: hash is ignored for now
			if (magic == HLVR_MAP_PHYSDATA_FILE_MAGIC
				&& version == HLVR_MAP_PHYSDATA_FILE_VERSION
				/*&& hash == TODO*/
				&& verticesCount > 0 && indicesCount > 0)
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
		uint64_t hash = 1337; // TODO!

		WriteBinaryData(physicsMapDataFileStream, HLVR_MAP_PHYSDATA_FILE_MAGIC);
		WriteBinaryData(physicsMapDataFileStream, HLVR_MAP_PHYSDATA_FILE_VERSION);
		WriteBinaryData(physicsMapDataFileStream, hash);
		WriteBinaryData(physicsMapDataFileStream, static_cast<uint32_t>(m_vertices.size()));
		WriteBinaryData(physicsMapDataFileStream, static_cast<uint32_t>(m_indices.size()));

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
