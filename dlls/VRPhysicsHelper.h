
#pragma once

#include <vector>
#include <unordered_map>

namespace reactphysics3d {
	class CollisionWorld;
	class DynamicsWorld;
	class BoxShape;
	class CapsuleShape;
	class CollisionBody;
	class RigidBody;
	class TriangleVertexArray;
	class TriangleMesh;
	class ConcaveMeshShape;
	class ConvexMeshShape;
	class ProxyShape;
	class PolygonVertexArray;
	class PolyhedronMesh;
	class SphereShape;
	struct Vector3;
};

class CWorld;
class CBaseEntity;

class VRPhysicsHelper
{
public:
	VRPhysicsHelper();
	~VRPhysicsHelper();

	static void CreateInstance();
	static void DestroyInstance();
	static VRPhysicsHelper& Instance();

	// Called by engine every frame
	inline void StartFrame()
	{
		// Make sure we have an up-to-date model of the world!
		CheckWorld();
	}

	bool CheckIfLineIsBlocked(const Vector & hlPos1, const Vector &hlPos2);
	bool CheckIfLineIsBlocked(const Vector & pos1, const Vector & pos2, Vector & result);

	Vector RotateVectorInline(const Vector &vecToRotate, const Vector &vecAngles, const Vector &vecOffset = Vector(), const bool reverse = false);
	void RotateVector(Vector &vecToRotate, const Vector &vecAngles, const Vector &vecOffset = Vector(), const bool reverse = false);
	Vector AngularVelocityToLinearVelocity(const Vector & avelocity, const Vector & pos);

	void TraceLine(const Vector &vecStart, const Vector &vecEnd, edict_t* pentIgnore, TraceResult *ptr);

	bool ModelIntersectsBBox(CBaseEntity *pModel, const Vector& bboxCenter, const Vector& bboxMins, const Vector& bboxMaxs);
	bool ModelIntersectsCapsule(CBaseEntity *pModel, const Vector& capsuleCenter, double radius, double height);
	bool ModelIntersectsWorld(CBaseEntity *pModel);
	bool ModelsIntersect(CBaseEntity *pModel1, CBaseEntity *pModel2);

	bool GetBSPModelBBox(CBaseEntity *pModel, Vector* bboxMins, Vector* bboxMaxs, Vector* bboxCenter = nullptr);

	// easter egg, see CWorldsSmallestCup
	void SetWorldsSmallestCupPosition(CBaseEntity *pWorldsSmallestCup);
	void GetWorldsSmallestCupPosition(CBaseEntity *pWorldsSmallestCup);

	void GetWorldUnstuckDir(const Vector& pos, const Vector& velocity, Vector& unstuckdir);

	class BSPModelData
	{
	public:
		~BSPModelData();

		void CreateData(reactphysics3d::CollisionWorld* collisionWorld);
		void DeleteData();
		bool HasData() { return m_hasData; }

		std::vector<struct reactphysics3d::Vector3>		m_vertices;
		std::vector<int32_t>							m_indices;

		reactphysics3d::TriangleVertexArray*			m_triangleVertexArray{ nullptr };
		reactphysics3d::TriangleMesh*					m_triangleMesh{ nullptr };
		reactphysics3d::ConcaveMeshShape*				m_concaveMeshShape{ nullptr };
		reactphysics3d::ProxyShape*						m_proxyShape{ nullptr };
		reactphysics3d::CollisionBody*					m_collisionBody{ nullptr };

		reactphysics3d::CollisionWorld*					m_collisionWorld{ nullptr };

		bool											m_hasData{ false };
	};

	class DynamicBSPModelData
	{
	public:
		~DynamicBSPModelData();

		void CreateData(reactphysics3d::DynamicsWorld* dynamicsWorld);
		void DeleteData();
		bool HasData() { return m_hasData; }

		std::vector<struct reactphysics3d::Vector3>		m_vertices;
		std::vector<struct reactphysics3d::Vector3>		m_normals;
		std::vector<int32_t>							m_indices;

		reactphysics3d::TriangleVertexArray*			m_triangleVertexArray{ nullptr };
		reactphysics3d::TriangleMesh*					m_triangleMesh{ nullptr };
		reactphysics3d::ConcaveMeshShape*				m_concaveMeshShape{ nullptr };
		reactphysics3d::ProxyShape*						m_proxyShape{ nullptr };
		reactphysics3d::RigidBody*						m_rigidBody{ nullptr };

		reactphysics3d::DynamicsWorld*					m_dynamicsWorld{ nullptr };

		bool											m_hasData{ false };
	};

	class BBoxData
	{
	public:
		~BBoxData();

		void CreateData(reactphysics3d::CollisionWorld* collisionWorld, const Vector& boxMins, const Vector& boxMaxs);
		void DeleteData();
		bool HasData() { return m_hasData; }

		Vector											m_boxMins;
		Vector											m_boxMaxs;

		reactphysics3d::BoxShape*						m_boxShape{ nullptr };
		reactphysics3d::ProxyShape*						m_proxyShape{ nullptr };
		reactphysics3d::CollisionBody*					m_collisionBody{ nullptr };

		reactphysics3d::CollisionWorld*					m_collisionWorld{ nullptr };

		bool											m_hasData{ false };
	};

private:
	bool CheckWorld();

	void InitPhysicsWorld();

	bool GetPhysicsMapDataFromFile(const std::string& physicsMapDataFilePath);
	void StorePhysicsMapDataToFile(const std::string& physicsMapDataFilePath);
	void GetPhysicsMapDataFromModel();

	void EnsureWorldsSmallestCupExists(CBaseEntity *pWorldsSmallestCup);

	std::string												m_currentMapName;
	std::unordered_map<std::string, BSPModelData>			m_bspModelData;
	std::unordered_map<std::string, DynamicBSPModelData>	m_dynamicBSPModelData;
	std::unordered_map<std::string, std::vector<BBoxData>>	m_studioModelBBoxCache;

	const struct model_s*									m_hlWorldModel{ nullptr };
	reactphysics3d::CollisionWorld*							m_collisionWorld{ nullptr };
	reactphysics3d::DynamicsWorld*							m_dynamicsWorld{ nullptr };

	reactphysics3d::CollisionBody*							m_bboxBody{ nullptr };
	reactphysics3d::BoxShape*								m_bboxShape{ nullptr };
	reactphysics3d::ProxyShape*								m_bboxProxyShape{ nullptr };

	reactphysics3d::CollisionBody*							m_capsuleBody{ nullptr };
	reactphysics3d::CapsuleShape*							m_capsuleShape{ nullptr };
	reactphysics3d::ProxyShape*								m_capsuleProxyShape{ nullptr };

	// can't forward declare inner type so we use void*
	void*													m_worldsSmallestCupPolygonFaces{ nullptr };
	reactphysics3d::PolygonVertexArray*						m_worldsSmallestCupPolygonVertexArray{ nullptr };
	reactphysics3d::PolyhedronMesh*							m_worldsSmallestCupPolyhedronMesh{ nullptr };
	reactphysics3d::RigidBody*								m_worldsSmallestCupBody{ nullptr };
	reactphysics3d::SphereShape* 							m_worldsSmallestCupTopSphereShape{ nullptr };
	reactphysics3d::SphereShape* 							m_worldsSmallestCupBottomSphereShape{ nullptr };
	reactphysics3d::ProxyShape*								m_worldsSmallestCupTopProxyShape{ nullptr };
	reactphysics3d::ProxyShape*								m_worldsSmallestCupBottomProxyShape{ nullptr };
	float vertices[24];
	int indices[24];

	static VRPhysicsHelper*		m_instance;
};
