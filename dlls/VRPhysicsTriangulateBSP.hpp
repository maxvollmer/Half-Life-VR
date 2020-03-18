
// Only to be included by VRPhysicsHelper.cpp

// Triangulation of BSP data
namespace
{
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
}
