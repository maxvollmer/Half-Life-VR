
#pragma warning(disable : 4244)
/*
Disable warnings for:
1>..\reactphysics3d\include\collision\shapes\heightfieldshape.h(227): warning C4244: 'return': conversion from 'double' to 'reactphysics3d::decimal', possible loss of data
1>..\reactphysics3d\include\collision\shapes\heightfieldshape.h(235): warning C4244: 'return': conversion from 'reactphysics3d::decimal' to 'int', possible loss of data
*/
namespace reactphysics3d
{
#define IS_DOUBLE_PRECISION_ENABLED
}
#include "reactphysics3d/include/reactphysics3d.h"

#include "extdll.h"

#define HARDWARE_MODE
#include "pm_defs.h"
#include "plane.h"
#include "com_model.h"
#include "studio.h"
extern struct playermove_s* PM_GetPlayerMove(void);

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

constexpr const uint32_t HLVR_MAP_PHYSDATA_FILE_MAGIC = 'HLVR';
constexpr const uint32_t HLVR_MAP_PHYSDATA_FILE_VERSION = 105;

// Stuff needed to extract brush models
constexpr const unsigned int ENGINE_MODEL_ARRAY_SIZE = 1024;
const model_t* g_EngineModelArrayPointer = nullptr;

enum NeedLoadFlag
{
	IS_LOADED = 0,  // If set, this brush model is valid and loaded
	NEEDS_LOADING = 1,  // If set, this brush model still needs to be loaded
	UNREFERENCED = 2   // If set, this brush model isn't used by the current map
};

// For animation.cpp
void CalculateHitboxAbsCenter(StudioHitBox& hitbox)
{
	hitbox.abscenter = hitbox.origin + VRPhysicsHelper::Instance().RotateVectorInline((hitbox.mins + hitbox.maxs) * 0.5f, hitbox.angles);
}

namespace
{
	class VRException : public std::exception
	{
	private:
		std::string m_message;

	public:
		VRException(const std::string& message) :
			std::exception{},
			m_message{ message }
		{

		}

		virtual char const* what() const override
		{
			return m_message.data();
		}
	};

	class VRAllocatorException : public VRException
	{
	public:
		VRAllocatorException(const std::string& message) :
			VRException{ message }
		{
		}
	};

	template <class RP3DAllocator>
	class VRAllocator : public RP3DAllocator
	{
	public:

		virtual void* allocate(size_t size) override
		{
			if (size == 0)
				throw VRAllocatorException{ "VRAllocator: Cannot allocate memory of size 0" };

			void* result = RP3DAllocator::allocate(size);

			if (!result)
				throw VRAllocatorException{ "VRAllocator: Unable to allocate memory of size " + std::to_string(size) + ", please check your system memory for errors." };

			return result;
		}
	};

	template <class RP3DAllocator>
	inline static RP3DAllocator* GetVRAllocator()
	{
		static std::unique_ptr<VRAllocator<RP3DAllocator>> m_instance;
		if (!m_instance)
			m_instance = std::make_unique<VRAllocator<RP3DAllocator>>();
		return m_instance.get();
	}

	class VRDynamicsWorld : public rp3d::DynamicsWorld
	{
	public:
		VRDynamicsWorld(const rp3d::Vector3& mGravity) :
			rp3d::DynamicsWorld{ mGravity }
		{
			this->mMemoryManager.setBaseAllocator(GetVRAllocator<rp3d::DefaultAllocator>());
			this->mMemoryManager.setPoolAllocator(GetVRAllocator<rp3d::DefaultPoolAllocator>());
			this->mMemoryManager.setSingleFrameAllocator(GetVRAllocator<rp3d::DefaultSingleFrameAllocator>());
		}
	};
	class VRCollisionWorld : public rp3d::CollisionWorld
	{
	public:
		VRCollisionWorld() :
			rp3d::CollisionWorld{}
		{
			this->mMemoryManager.setBaseAllocator(GetVRAllocator<rp3d::DefaultAllocator>());
			this->mMemoryManager.setPoolAllocator(GetVRAllocator<rp3d::DefaultPoolAllocator>());
			this->mMemoryManager.setSingleFrameAllocator(GetVRAllocator<rp3d::DefaultSingleFrameAllocator>());
		}
	};
}

uint64_t HashFileData(const std::string& filename)
{
	try
	{
		std::hash<std::string> stringhasher;
		std::string data(std::istreambuf_iterator<char>(std::fstream(filename, std::ios_base::binary | std::ios_base::in)), std::istreambuf_iterator<char>());
		return stringhasher(data);
	}
	catch (...)
	{
		return 0;
	}
}

class MyCollisionCallback : public reactphysics3d::CollisionCallback
{
public:
	MyCollisionCallback(std::function<void(const CollisionCallbackInfo&)> callback) :
		m_callback{ callback }
	{
	}

	virtual void notifyContact(const CollisionCallbackInfo& collisionCallbackInfo) override
	{
		m_callback(collisionCallbackInfo);
	}

private:
	const std::function<void(const CollisionCallbackInfo&)> m_callback;
};

// Returns a pointer to a model_t instance holding BSP data for the current map - Max Vollmer, 2018-01-21
const model_t* GetWorldBSPModel()
{
	playermove_s* pmove = PM_GetPlayerMove();
	if (pmove != nullptr)
	{
		for (const physent_t& physent : pmove->physents)
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
		const model_t* worldmodel = GetWorldBSPModel();
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
const model_t* GetBSPModel(edict_t* pent)
{
	// Check if entity is world
	if (pent == INDEXENT(0))
		return GetWorldBSPModel();

	const char* modelname = STRING(pent->v.model);

	// Check that the entity has a brush model
	if (modelname != nullptr && modelname[0] == '*')
	{
		return FindEngineModelByName(modelname);
	}

	return nullptr;
}

// Returns a pointer to a model_t instance holding BSP data for this entity's BSP model (if it is a BSP model) - Max Vollmer, 2018-01-21
const model_t* GetBSPModel(CBaseEntity* pEntity)
{
	return GetBSPModel(pEntity->edict());
}

bool IsSolidInPhysicsWorld(edict_t* pent)
{
	return FClassnameIs(&pent->v, "func_wall") || FClassnameIs(&pent->v, "func_illusionary");
}



using namespace reactphysics3d;

constexpr const rp3d::decimal RP3D_TO_HL = 50.;
constexpr const rp3d::decimal HL_TO_RP3D = 1. / RP3D_TO_HL;


constexpr const int MAX_GAP_WIDTH = VEC_DUCK_HEIGHT;
constexpr const int MAX_GAP_WIDTH_SQUARED = MAX_GAP_WIDTH * MAX_GAP_WIDTH;
constexpr const int MIN_DISTANCE = 240 + MAX_GAP_WIDTH;

constexpr const int PHYSICS_STEPS = 30;
constexpr const int MAX_PHYSICS_STEPS = PHYSICS_STEPS * 1.5;
constexpr const rp3d::decimal PHYSICS_STEP_TIME = 1. / PHYSICS_STEPS;

constexpr const rp3d::decimal MAX_PHYSICS_TIME_PER_FRAME = 1. / 30.;  // Never drop below 30fps due to physics calculations


void TEMPTODO_RemoveInvalidTriangles(const std::vector<Vector3>& vertices, std::vector<int32_t>& indices)
{
	// remove invalid triangles (TODO: Find out where these come from, this shouldn't be happening anymore!)
	for (uint32_t i = 0; i < indices.size(); i += 3)
	{
		if ((vertices[indices[i]] - vertices[indices[i + 1]]).lengthSquare() < EPSILON || (vertices[indices[i]] - vertices[indices[i + 2]]).lengthSquare() < EPSILON || (vertices[indices[i + 1]] - vertices[indices[i + 2]]).lengthSquare() < EPSILON)
		{
#ifdef _DEBUG
			ALERT(at_console, "(VRPhysicsHelper)Warning: Found invalid triangle at index %i, removed!\n", i);
#endif
			indices[i] = -1;
			indices[i + 1] = -1;
			indices[i + 2] = -1;
		}
	}
	indices.erase(std::remove_if(indices.begin(), indices.end(), [](const int32_t& i) { return i < 0; }), indices.end());
}


inline Vector3 HLVecToRP3DVec(const Vector& hlVec)
{
	return Vector3{ hlVec.x * HL_TO_RP3D, hlVec.y * HL_TO_RP3D, hlVec.z * HL_TO_RP3D };
}

inline Vector RP3DVecToHLVec(const Vector3& rp3dVec)
{
	return Vector{ float(rp3dVec.x * RP3D_TO_HL), float(rp3dVec.y * RP3D_TO_HL), float(rp3dVec.z * RP3D_TO_HL) };
}

inline rp3d::Quaternion HLAnglesToRP3DQuaternion(const Vector& angles)
{
	// up / down
#define PITCH 0
// left / right
#define YAW 1
// fall over
#define ROLL 2

	rp3d::Quaternion rollQuaternion;
	rollQuaternion.setAllValues(sin(double(angles[ROLL]) * M_PI / 360.), 0., 0., cos(double(angles[ROLL]) * M_PI / 360.));

	rp3d::Quaternion pitchQuaternion;
	pitchQuaternion.setAllValues(0., sin(double(angles[PITCH]) * M_PI / 360.), 0., cos(double(angles[PITCH]) * M_PI / 360.));

	rp3d::Quaternion yawQuaternion;
	yawQuaternion.setAllValues(0., 0., sin(double(angles[YAW]) * M_PI / 360.), cos(double(angles[YAW]) * M_PI / 360.));

	return yawQuaternion * pitchQuaternion * rollQuaternion;
}


inline Vector RP3DTransformToHLAngles(const rp3d::Matrix3x3& matrix)
{
	Vector forward{ float(matrix.getRow(0).x), float(matrix.getRow(0).y), float(matrix.getRow(0).z) };
	Vector right{ float(matrix.getRow(1).x), float(matrix.getRow(1).y), float(matrix.getRow(1).z) };
	Vector up{ float(matrix.getRow(2).x), float(matrix.getRow(2).y), float(matrix.getRow(2).z) };

	Vector angles;
	UTIL_GetAnglesFromVectors(forward, right, up, angles);

	return Vector{ angles.x, angles.y, angles.z };
}



const std::hash<int> intHasher;

class VectorHash
{
public:
	inline std::size_t operator()(const Vector& v) const
	{
		return intHasher(v.x * 10) ^ intHasher(v.y * 10) ^ intHasher(v.z * 10);
	}
};
class VectorEqual
{
public:
	inline bool operator()(const Vector& v1, const Vector& v2) const
	{
		return fabs(v1.x - v2.x) < EPSILON && fabs(v1.y - v2.y) < EPSILON && fabs(v1.z - v2.z) < EPSILON;
	}
};

enum class PlaneMajor
{
	X,
	Y,
	Z
};

inline PlaneMajor PlaneMajorFromNormal(const Vector& normal)
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
	TranslatedPlane(const Vector& vert, const Vector& normal) :
		normal{ normal }, dist{ DotProduct(normal, vert) }, hash{ intHasher(dist) ^ intHasher(normal.x * 10) ^ intHasher(normal.y * 10) ^ intHasher(normal.z * 10) }, major{ PlaneMajorFromNormal(normal) }
	{
	}

	inline bool IsCoPlanar(const TranslatedPlane& other) const
	{
		return fabs(dist - other.dist) < EPSILON && fabs(normal.x - other.normal.x) < EPSILON && fabs(normal.y - other.normal.y) < EPSILON && fabs(normal.z - other.normal.z) < EPSILON;
	}

	inline const Vector& GetNormal() const
	{
		return normal;
	}

	inline const PlaneMajor& GetMajor() const
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

	class Hash
	{
	public:
		inline std::size_t operator()(const TranslatedPlane& e) const
		{
			return e.hash;
		}
	};
	class Equal
	{
	public:
		inline bool operator()(const TranslatedPlane& e1, const TranslatedPlane& e2) const
		{
			return e1.IsCoPlanar(e2);
		}
	};

private:
	const Vector normal;
	const float dist = 0.f;
	const std::size_t hash;
	const PlaneMajor major;
};

typedef std::unordered_map<TranslatedPlane, std::unordered_map<Vector, VertexMetaData, VectorHash, VectorEqual>, TranslatedPlane::Hash, TranslatedPlane::Equal> PlaneVertexMetaDataMap;

class TranslatedFace
{
public:
	TranslatedFace(const msurface_t& face, const Vector& origin, PlaneVertexMetaDataMap& planeVertexMetaData) :
		plane{ origin + face.polys->verts[0], FBitSet(face.flags, SURF_PLANEBACK) ? -face.plane->normal : face.plane->normal }
	{
		for (int i = 0; i < face.polys->numverts; ++i)
		{
			Vector& vertexBefore = origin + face.polys->verts[(i > 0) ? (i - 1) : (face.polys->numverts - 1)];
			Vector& vertex = origin + face.polys->verts[i];
			Vector& vertexAfter = origin + face.polys->verts[(i < (face.polys->numverts - 1)) ? (i + 1) : 0];

			Vector v1 = (vertexBefore - vertex).Normalize();
			Vector v2 = (vertexAfter - vertex).Normalize();
			float vertDot = DotProduct(v1, v2);

			if (fabs(vertDot + 1.f) < EPSILON)
			{
				// Colinear vertex, drop it!
			}
			else
			{
				vertices.push_back(vertex);
				planeVertexMetaData[plane][vertex].numFaces++;
				planeVertexMetaData[plane][vertex].totalCos += double(vertDot) - 1.0;
			}
		}
		if (vertices.size() >= 3)
		{
			vecAnyPointInside = (vertices[0] + vertices[2]) / 2;
		}
	}

	inline const std::vector<Vector>& GetVertices() const
	{
		return vertices;
	}

	// Returns those vertices of A and B that enclose a gap between these faces (if one exists)
	bool GetGapVertices(const TranslatedFace& other, std::vector<Vector>& mergedVertices, PlaneVertexMetaDataMap& planeVertexMetaData) const
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

	inline const TranslatedPlane& GetPlane() const
	{
		return plane;
	}

private:
	inline bool IsCloseEnough(const TranslatedFace& other) const
	{
		// Since faces in a HL BSP are always maximum 240 units wide,
		// if any vertex from this face is further than MIN_DISTANCE (240 + player width) units
		// away from any vertex in the other face, these faces can't have a gap too narrow for a player
		return fabs(vertices[0].x - other.vertices[0].x) < float(MIN_DISTANCE) && fabs(vertices[0].y - other.vertices[0].y) < float(MIN_DISTANCE) && fabs(vertices[0].z - other.vertices[0].z) < float(MIN_DISTANCE);
	}

	bool GetGapVerticesInPlane(const TranslatedFace& other, std::vector<Vector>& mergedVertices, PlaneVertexMetaDataMap& planeVertexMetaData, const size_t a, const size_t b) const
	{
		// We only want the two outermost vertices of each face, since we don't care about overlapping triangles and we don't have to deal with arbitrary shapes this way - also keeps amount of triangles down
		std::vector<Vector> verticesA, verticesB;

		// Get direction from A to B
		const Vector faceDir = (other.vecAnyPointInside - vecAnyPointInside).Normalize();

		// Loop over all vertices and check if we have a gap
		for (const Vector& vec : vertices)
		{
			// Discard vertex if it's completely enclosed by other faces
			if (CVAR_GET_FLOAT("vr_debug_physics") >= 1.f && planeVertexMetaData[plane][vec].totalCos < (EPSILON_D - 4.0))
			{
				continue;
			}
			// Discard point if it's on the far side of this face
			if (DotProduct((vec - vecAnyPointInside).Normalize(), faceDir) < 0)
			{
				continue;
			}
			bool foundOther = false;
			for (const Vector& vecOther : other.vertices)
			{
				// Discard vertex if it's completely enclosed by other faces
				if (CVAR_GET_FLOAT("vr_debug_physics") >= 2.f && planeVertexMetaData[plane][vecOther].totalCos < (EPSILON_D - 4.0))
				{
					continue;
				}
				float distanceSquared = (vec - vecOther).LengthSquared();
				if (distanceSquared < (EPSILON * EPSILON))
				{
					// If any two vertices are equal (that is: if the faces are touching), we can safely cancel this algorithm,
					// because of what we know about the simple architecture used throughout Half-Life maps:
					// In Vanilla Half-Life no touching faces will ever form a gap too small for a player,
					// if there aren't also 2 other faces that do not touch each other (and those 2 will then produce the desired vertices for our gap triangles)
					return false;
				}
				if (distanceSquared <= MAX_GAP_WIDTH_SQUARED)
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

size_t CollectFaces(const model_t* model, const Vector& origin, PlaneFacesMap& planeFaces, PlaneVertexMetaDataMap& planeVertexMetaData)
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

void TriangulateGapsBetweenCoplanarFaces(const TranslatedFace& faceA, const TranslatedFace& faceB, PlaneVertexMetaDataMap& planeVertexMetaData, std::vector<Vector3>& vertices, std::vector<int>& indices)
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
			indices.push_back(indexoffset + 0);
			indices.push_back(indexoffset + 1);
			indices.push_back(indexoffset + 2);
			// a-c-b
			indices.push_back(indexoffset + 0);
			indices.push_back(indexoffset + 2);
			indices.push_back(indexoffset + 1);

			if (mergedVertices.size() == 4)
			{
				// a-c-d
				indices.push_back(indexoffset + 0);
				indices.push_back(indexoffset + 2);
				indices.push_back(indexoffset + 3);
				// a-d-c
				indices.push_back(indexoffset + 0);
				indices.push_back(indexoffset + 3);
				indices.push_back(indexoffset + 2);
			}
		}
	}
}

void TriangulateBSPFaces(const PlaneFacesMap& planeFaces, PlaneVertexMetaDataMap& planeVertexMetaData, std::vector<Vector3>& vertices, std::vector<Vector3>* normals, std::vector<int>& indices, bool triangulateGaps)
{
	for (auto pair : planeFaces)
	{
		const TranslatedPlane& plane = pair.first;
		const std::vector<TranslatedFace>& faces = pair.second;
		for (size_t faceIndexA = 0; faceIndexA < faces.size(); ++faceIndexA)
		{
			const TranslatedFace& faceA = faces[faceIndexA];

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

			if (!triangulateGaps && normals)
			{
				// Add normals (only if we don't triangulate gaps)
				for (size_t i = 0; i < faceVertices.size(); i++)
				{
					normals->push_back(HLVecToRP3DVec(-faceA.GetPlane().GetNormal()));
				}
			}

			// Then do triangulation of gaps between faces that are too small for a player to fit through
			if (triangulateGaps)
			{
				for (size_t faceIndexB = faceIndexA + 1; faceIndexB < faces.size(); ++faceIndexB)
				{
					const TranslatedFace& faceB = faces[faceIndexB];
					TriangulateGapsBetweenCoplanarFaces(faceA, faceB, planeVertexMetaData, vertices, indices);
				}
			}
		}
	}
}

// Get vertices and indices of world (triangulation of bsp data (map and non-moving solid entities))
void TriangulateBSPModel(const model_t* model, PlaneFacesMap& planeFaces, PlaneVertexMetaDataMap& planeVertexMetaData, std::vector<Vector3>& vertices, std::vector<Vector3>* normals, std::vector<int>& indices, bool triangulateGaps)
{
	// Collect faces of bsp model
	CollectFaces(model, Vector{}, planeFaces, planeVertexMetaData);

	// If world, also collect faces of non-moving solid entities
	if (model == GetWorldBSPModel())
	{
		for (int index = 1; index < gpGlobals->maxEntities; index++)
		{
			edict_t* pent = INDEXENT(index);
			if (FNullEnt(pent))
				continue;

			if (!IsSolidInPhysicsWorld(pent))
				continue;

			CollectFaces(GetBSPModel(pent), pent->v.origin, planeFaces, planeVertexMetaData);
		}
	}

	// Triangulate all faces (also triangulates gaps between faces!)
	TriangulateBSPFaces(planeFaces, planeVertexMetaData, vertices, normals, indices, triangulateGaps);
}

void RaycastPotentialVerticeGaps(CollisionBody* dynamicMap, const Vector3& checkVertexInPhysSpace, const Vector3& checkVertexAfterInPhysSpace, const Vector3& checkDirInPhysSpace, const Vector3& correctionalOffsetInPhysSpace, std::vector<Vector3>& vertices, std::vector<int>& indices, const size_t vertexIndexOffset)
{
	RaycastInfo raycastInfo1{};
	RaycastInfo raycastInfo2{};
	if (dynamicMap->raycast({ checkVertexInPhysSpace, checkVertexInPhysSpace + checkDirInPhysSpace }, raycastInfo1) && raycastInfo1.hitFraction > 0. && raycastInfo1.hitFraction < 1. && dynamicMap->raycast({ checkVertexAfterInPhysSpace, checkVertexAfterInPhysSpace + checkDirInPhysSpace }, raycastInfo2) && raycastInfo2.hitFraction > 0. && raycastInfo2.hitFraction < 1.)
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
			indices.push_back(indexoffset + 0);
			indices.push_back(indexoffset + 1);
			indices.push_back(indexoffset + 2);
			indices.push_back(indexoffset + 0);
			indices.push_back(indexoffset + 2);
			indices.push_back(indexoffset + 1);
			if (gapVertices.size() == 4)
			{
				indices.push_back(indexoffset + 0);
				indices.push_back(indexoffset + 2);
				indices.push_back(indexoffset + 3);
				indices.push_back(indexoffset + 0);
				indices.push_back(indexoffset + 3);
				indices.push_back(indexoffset + 2);
			}
		}
	}
}

void FindAdditionalPotentialGapsBetweenMapFacesUsingPhysicsEngine(CollisionBody* dynamicMap, const PlaneFacesMap& planeFaces, PlaneVertexMetaDataMap& planeVertexMetaData, std::vector<Vector3>& vertices, std::vector<int>& indices, const size_t vertexIndexOffset)
{
	// There are potential gaps left, that our first algorithm couldn't find.
	// We marked all potential vertices and can now just loop over them and then use the physics engine to check for remaining gaps.
	for (auto pair : planeFaces)
	{
		const TranslatedPlane& plane = pair.first;
		for (const TranslatedFace& face : pair.second)
		{
			auto vertexMetaDataMap = planeVertexMetaData[plane];
			for (size_t i = 0; i < face.GetVertices().size(); ++i)
			{
				const Vector& vertex = face.GetVertices()[i];
				const VertexMetaData& vertexMetaData = vertexMetaDataMap[vertex];
				if (vertexMetaData.totalCos > (EPSILON_D - 4.0) && !vertexMetaData.handled)
				{
					const Vector& vertexAfter = face.GetVertices()[(i < face.GetVertices().size() - 1) ? (i + 1) : 0];
					const VertexMetaData& vertexMetaDataAfter = vertexMetaDataMap[vertexAfter];
					if (vertexMetaDataAfter.totalCos > (EPSILON_D - 4.0) && !vertexMetaDataAfter.handled)
					{
						Vector dir{ vertex - vertexAfter };
						dir.InlineNormalize();

						Vector checkVertex{ vertex - dir };
						Vector checkVertexAfter{ vertexAfter + dir };

						const Vector checkDir1 = CrossProduct(dir, plane.GetNormal()) * MAX_GAP_WIDTH;
						const Vector checkDir2 = plane.GetNormal() * MAX_GAP_WIDTH;

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
	catch (...) {}
}

bool VRPhysicsHelper::CheckIfLineIsBlocked(const Vector& hlPos1, const Vector& hlPos2)
{
	Vector dummy;
	return CheckIfLineIsBlocked(hlPos1, hlPos2, dummy);
}

bool VRPhysicsHelper::CheckIfLineIsBlocked(const Vector& hlPos1, const Vector& hlPos2, Vector& hlResult)
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

bool VRPhysicsHelper::GetBSPModelBBox(CBaseEntity* pModel, Vector* bboxMins, Vector* bboxMaxs, Vector* bboxCenter)
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

	bspModelData->second.m_collisionBody->setTransform(rp3d::Transform::identity());

	if (bboxMins)
		*bboxMins = RP3DVecToHLVec(bspModelData->second.m_collisionBody->getAABB().getMin());

	if (bboxMaxs)
		*bboxMaxs = RP3DVecToHLVec(bspModelData->second.m_collisionBody->getAABB().getMax());

	if (bboxCenter)
		*bboxCenter = RP3DVecToHLVec(bspModelData->second.m_collisionBody->getAABB().getCenter());

	return true;
}

/*
inline void CreateHitBoxVertices(const Vector& bboxMins, const Vector& bboxMaxs, std::vector<reactphysics3d::Vector3>& vertices, std::vector<int32_t>& indices)
{
	static constexpr const int32_t indexarray[] =
	{
		2, 1, 0,		3, 2, 0,		// front
		6, 5, 4,		7, 6, 4,		// back
		10, 9, 8,		11, 10, 8,		// top
		14, 13, 12,		15, 14, 12,		// bottom
		18, 17, 16,		19, 18, 16,		// right
		22, 21, 20,		23, 22, 20,		// left
	};

	const reactphysics3d::Vector3 vertexarray[] =
	{
		HLVecToRP3DVec(Vector{ bboxMins.x, bboxMins.y, bboxMaxs.z }),
		HLVecToRP3DVec(Vector{ bboxMins.x, bboxMins.y, bboxMins.z }),
		HLVecToRP3DVec(Vector{ bboxMins.x, bboxMaxs.y, bboxMins.z }),
		HLVecToRP3DVec(Vector{ bboxMins.x, bboxMaxs.y, bboxMaxs.z }),

		HLVecToRP3DVec(Vector{ bboxMaxs.x, bboxMins.y, bboxMaxs.z }),
		HLVecToRP3DVec(Vector{ bboxMaxs.x, bboxMins.y, bboxMins.z }),
		HLVecToRP3DVec(Vector{ bboxMins.x, bboxMins.y, bboxMins.z }),
		HLVecToRP3DVec(Vector{ bboxMins.x, bboxMins.y, bboxMaxs.z }),

		HLVecToRP3DVec(Vector{ bboxMins.x, bboxMaxs.y, bboxMaxs.z }),
		HLVecToRP3DVec(Vector{ bboxMins.x, bboxMaxs.y, bboxMins.z }),
		HLVecToRP3DVec(Vector{ bboxMaxs.x, bboxMaxs.y, bboxMins.z }),
		HLVecToRP3DVec(Vector{ bboxMaxs.x, bboxMaxs.y, bboxMaxs.z }),

		HLVecToRP3DVec(Vector{ bboxMaxs.x, bboxMins.y, bboxMaxs.z }),
		HLVecToRP3DVec(Vector{ bboxMins.x, bboxMins.y, bboxMaxs.z }),
		HLVecToRP3DVec(Vector{ bboxMins.x, bboxMaxs.y, bboxMaxs.z }),
		HLVecToRP3DVec(Vector{ bboxMaxs.x, bboxMaxs.y, bboxMaxs.z }),

		HLVecToRP3DVec(Vector{ bboxMaxs.x, bboxMaxs.y, bboxMins.z }),
		HLVecToRP3DVec(Vector{ bboxMins.x, bboxMaxs.y, bboxMins.z }),
		HLVecToRP3DVec(Vector{ bboxMins.x, bboxMins.y, bboxMins.z }),
		HLVecToRP3DVec(Vector{ bboxMaxs.x, bboxMins.y, bboxMins.z }),

		HLVecToRP3DVec(Vector{ bboxMaxs.x, bboxMaxs.y, bboxMaxs.z }),
		HLVecToRP3DVec(Vector{ bboxMaxs.x, bboxMaxs.y, bboxMins.z }),
		HLVecToRP3DVec(Vector{ bboxMaxs.x, bboxMins.y, bboxMins.z }),
		HLVecToRP3DVec(Vector{ bboxMaxs.x, bboxMins.y, bboxMaxs.z }),
	};

	vertices.assign(vertexarray, vertexarray + sizeof(vertexarray) / sizeof(vertexarray[0]));
	indices.assign(indexarray, indexarray + sizeof(indexarray) / sizeof(indexarray[0]));
}
*/

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
		//CreateHitBoxVertices(bboxMins, bboxMaxs, data->m_vertices, data->m_indices);
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


bool VRPhysicsHelper::ModelIntersectsCapsule(CBaseEntity* pModel, const Vector& capsuleCenter, double radius, double height)
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

// internal helper function
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

bool VRPhysicsHelper::BBoxIntersectsLine(const Vector& bboxCenter, const Vector& bboxMins, const Vector& bboxMaxs, const Vector& bboxAngles, const Vector& lineA, const Vector& lineB, VRPhysicsHelperModelBBoxIntersectResult* result)
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

bool VRPhysicsHelper::ModelIntersectsLine(CBaseEntity* pModel, const Vector& lineA, const Vector& lineB, VRPhysicsHelperModelBBoxIntersectResult* result)
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

bool VRPhysicsHelper::ModelIntersectsBBox(CBaseEntity* pModel, const Vector& bboxCenter, const Vector& bboxMins, const Vector& bboxMaxs, const Vector& bboxAngles, VRPhysicsHelperModelBBoxIntersectResult* result)
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

constexpr const float LADDER_EPSILON = 4.f;

void VRPhysicsHelper::TraceLine(const Vector& vecStart, const Vector& vecEnd, edict_t* pentIgnore, TraceResult* ptr)
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


bool DoesAnyBrushModelNeedLoading(const model_t* const models)
{
	for (int index = 0; index < gpGlobals->maxEntities; index++)
	{
		edict_t* pent = INDEXENT(index);
		if (FNullEnt(pent) && !FWorldEnt(pent))
			continue;

		if (!IsSolidInPhysicsWorld(pent))
			continue;

		const model_t* const model = GetBSPModel(pent);
		if (model == nullptr || model->needload != 0)
		{
			return true;
		}
	}
	return false;
}

bool IsWorldValid(const model_t* world)
{
	return world != nullptr && world->needload == 0 && world->marksurfaces[0] != nullptr && world->marksurfaces[0]->polys != nullptr && !DoesAnyBrushModelNeedLoading(world);
}

bool CompareWorlds(const model_t* world1, const model_t* worl2)
{
	return world1 == worl2 && world1 != nullptr && FStrEq(world1->name, worl2->name);
}

template<typename T>
void WriteBinaryData(std::ofstream& out, const T& value)
{
	out.write(reinterpret_cast<const char*>(&value), sizeof(value));
}

template<typename T>
void ReadBinaryData(std::ifstream& in, T& value)
{
	in.read(reinterpret_cast<char*>(&value), sizeof(value));
	if (in.eof())
	{
		throw VRException{ "unexpected eof" };
	}
	if (!in.good())
	{
		throw VRException{ "unexpected error" };
	}
}

void WriteString(std::ofstream& out, const std::string& value)
{
	WriteBinaryData(out, value.size());
	out.write(value.data(), value.size());
}

std::string ReadString(std::ifstream& in)
{
	size_t size;
	ReadBinaryData(in, size);
	std::string result;
	result.resize(size);
	in.read(const_cast<char*>(result.data()), size);
	if (in.eof())
	{
		throw VRException{ "unexpected eof" };
	}
	if (!in.good())
	{
		throw VRException{ "unexpected error" };
	}
	return result;
}

void ReadVerticesAndIndices(
	std::ifstream& physicsMapDataFileStream,
	uint32_t verticesCount,
	uint32_t normalsCount,
	uint32_t indicesCount,
	std::vector<Vector3>& vertices,
	std::vector<Vector3>* normals,
	std::vector<int32_t>& indices)
{
	for (unsigned int i = 0; i < verticesCount; ++i)
	{
		reactphysics3d::Vector3 vertex;
		ReadBinaryData(physicsMapDataFileStream, vertex.x);
		ReadBinaryData(physicsMapDataFileStream, vertex.y);
		ReadBinaryData(physicsMapDataFileStream, vertex.z);
		vertices.push_back(vertex);
	}

	for (unsigned int i = 0; i < normalsCount; ++i)
	{
		reactphysics3d::Vector3 normal;
		ReadBinaryData(physicsMapDataFileStream, normal.x);
		ReadBinaryData(physicsMapDataFileStream, normal.y);
		ReadBinaryData(physicsMapDataFileStream, normal.z);
		normals->push_back(normal);
	}

	for (unsigned int i = 0; i < indicesCount; ++i)
	{
		int index = 0;
		ReadBinaryData(physicsMapDataFileStream, index);
		if (index < 0 || static_cast<unsigned>(index) > vertices.size())
		{
			throw VRException{ "invalid bsp data: index out of bounds!" };
		}
		indices.push_back(index);
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
	catch (const VRException& e)
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
				catch (const VRException & e)
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

	return m_hlWorldModel != nullptr && m_hlWorldModel == world && m_collisionWorld != nullptr && !m_bspModelData.empty() && m_bspModelData.count(m_currentMapName) > 0 && m_dynamicBSPModelData.count(m_currentMapName) > 0;
}

void RotateVectorX(Vector& vecToRotate, const float angle)
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

void RotateVectorY(Vector& vecToRotate, const float angle)
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

void RotateVectorZ(Vector& vecToRotate, const float angle)
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


void VRPhysicsHelper::EnsureWorldsSmallestCupExists(CBaseEntity* pWorldsSmallestCup)
{
	if (!m_worldsSmallestCupBody)
	{
		const auto& modelInfo = VRModelHelper::GetInstance().GetModelInfo(pWorldsSmallestCup);
		Vector mins = modelInfo.m_sequences[0].bboxMins;
		Vector maxs = modelInfo.m_sequences[0].bboxMaxs;
		rp3d::decimal bottomRadius = max(maxs.x - mins.x, maxs.y - mins.y) * 0.5 * HL_TO_RP3D;
		rp3d::decimal topRadius = bottomRadius * 1.1;
		rp3d::decimal height = double(maxs.z) - double(mins.z) * HL_TO_RP3D;

		m_worldsSmallestCupTopSphereShape = new SphereShape{ topRadius };
		m_worldsSmallestCupBottomSphereShape = new SphereShape{ bottomRadius };

		m_worldsSmallestCupBody = m_dynamicsWorld->createRigidBody(rp3d::Transform{ HLVecToRP3DVec(pWorldsSmallestCup->pev->origin), HLAnglesToRP3DQuaternion(pWorldsSmallestCup->pev->angles) });
		m_worldsSmallestCupTopProxyShape = m_worldsSmallestCupBody->addCollisionShape(m_worldsSmallestCupTopSphereShape, rp3d::Transform{ rp3d::Vector3{0.f, 0.f, height - topRadius}, rp3d::Matrix3x3::identity() }, 0.6);
		m_worldsSmallestCupBottomProxyShape = m_worldsSmallestCupBody->addCollisionShape(m_worldsSmallestCupBottomSphereShape, rp3d::Transform{ rp3d::Vector3{0.f, 0.f, bottomRadius}, rp3d::Matrix3x3::identity() }, 0.5);
		m_worldsSmallestCupBody->setType(BodyType::DYNAMIC);
	}
}

void VRPhysicsHelper::SetWorldsSmallestCupPosition(CBaseEntity* pWorldsSmallestCup)
{
	if (!CheckWorld())
		return;

	EnsureWorldsSmallestCupExists(pWorldsSmallestCup);

	// set position and orientation
	m_worldsSmallestCupBody->setTransform(rp3d::Transform{ HLVecToRP3DVec(pWorldsSmallestCup->pev->origin), HLAnglesToRP3DQuaternion(pWorldsSmallestCup->pev->angles) });
	m_worldsSmallestCupBody->setAngularVelocity(rp3d::Vector3::zero());
	m_worldsSmallestCupBody->setLinearVelocity(HLVecToRP3DVec(pWorldsSmallestCup->pev->velocity));

	// wake up
	m_worldsSmallestCupBody->setIsActive(true);
	m_worldsSmallestCupBody->setIsSleeping(false);
}

void VRPhysicsHelper::GetWorldsSmallestCupPosition(CBaseEntity* pWorldsSmallestCup)
{
	if (!CheckWorld())
		return;

	EnsureWorldsSmallestCupExists(pWorldsSmallestCup);

	m_dynamicsWorld->setGravity(HLVecToRP3DVec(Vector{ 0., 0., -g_psv_gravity->value }));
	m_dynamicsWorld->update(1. / 90.);

	const rp3d::Transform& transform = m_worldsSmallestCupBody->getTransform();
	pWorldsSmallestCup->pev->origin = RP3DVecToHLVec(transform.getPosition());
	pWorldsSmallestCup->pev->angles = RP3DTransformToHLAngles(transform.getOrientation().getMatrix());
	pWorldsSmallestCup->pev->velocity = RP3DVecToHLVec(m_worldsSmallestCupBody->getLinearVelocity());
}
