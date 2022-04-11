
// Only to be included by VRPhysicsHelper.cpp

// Custom classes/functions for reactphysics3d
namespace
{
	// not used anymore, causes issues when allocators are destroyed
	/*
	template <class RP3DAllocator>
	class VRAllocator : public RP3DAllocator
	{
	public:
		virtual void* allocate(size_t size) override
		{
			if (size == 0)
				return nullptr;

			void* result = RP3DAllocator::allocate(size);

			if (!result)
				throw VRAllocatorException{ "VRAllocator: Unable to allocate memory of size " + std::to_string(size) + ", please check your system memory for errors." };

			return result;
		}
	};
	*/


	class VRDynamicsWorld : public rp3d::DynamicsWorld
	{
	public:
		VRDynamicsWorld(const rp3d::Vector3& mGravity) :
			rp3d::DynamicsWorld{ mGravity }
		{
		}
	};

	class VRCollisionWorld : public rp3d::CollisionWorld
	{
	public:
		VRCollisionWorld() :
			rp3d::CollisionWorld{}
		{
		}
	};

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
}
