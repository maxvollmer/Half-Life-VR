
// Only to be included by VRPhysicsHelper.cpp

// Custom classes/functions for reactphysics3d
namespace
{
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
