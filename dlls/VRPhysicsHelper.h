
#pragma once


namespace reactphysics3d {
	class CollisionWorld;
	class DynamicsWorld;
	class SphereShape;
	class RigidBody;
	class TriangleVertexArray;
	class TriangleMesh;
	class ConcaveMeshShape;
	class ProxyShape;
	struct Vector3;
};

class CWorld;

class VRPhysicsHelper
{
public:
	VRPhysicsHelper();
	~VRPhysicsHelper();

	// Called by engine every frame
	inline void StartFrame()
	{
		// Make sure we have an up-to-date model of the world!
		CheckWorld();
	}

	bool CheckIfLineIsBlocked(const Vector & hlPos1, const Vector &hlPos2);
	bool CheckIfLineIsBlocked(const Vector & pos1, const Vector & pos2, Vector & result);

	void RotateVector(Vector &vecToRotate, const Vector &vecAngles, const Vector &vecOffset = Vector(), const bool reverse = false);
	Vector AngularVelocityToLinearVelocity(const Vector & avelocity, const Vector & pos);

private:
	//reactphysics3d::CollisionWorld * m_collisionWorld = nullptr;
	reactphysics3d::DynamicsWorld * m_dynamicsWorld = nullptr;

	std::vector<struct reactphysics3d::Vector3> m_vertices;
	std::vector<int> m_indices;

	reactphysics3d::TriangleVertexArray* m_triangleArray = nullptr;
	reactphysics3d::TriangleMesh* m_triangleMesh = nullptr;
	reactphysics3d::ConcaveMeshShape* m_concaveMeshShape = nullptr; 
	reactphysics3d::ProxyShape* m_dynamicMapProxyShape = nullptr;

	reactphysics3d::SphereShape * m_sphereShape = nullptr;

	reactphysics3d::RigidBody* m_dynamicMap = nullptr;
	reactphysics3d::RigidBody* m_dynamicSphere = nullptr;

	const struct model_s *m_hlWorldModel = nullptr;

	bool CheckWorld();

	void InitPhysicsWorld();
	void DeletePhysicsMapData();
	void CreateMapShapeFromCurrentVerticesAndTriangles();

	bool GetPhysicsMapDataFromFile(const std::string& physicsMapDataFilePath);
	void StorePhysicsMapDataToFile(const std::string& physicsMapDataFilePath);
	void GetPhysicsMapDataFromModel();
};

extern VRPhysicsHelper gVRPhysicsHelper;