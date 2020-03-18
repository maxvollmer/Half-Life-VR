
// Only to be included by VRPhysicsHelper.cpp

// File IO helper functions
namespace
{
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
}

