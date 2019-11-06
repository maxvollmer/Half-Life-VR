
#include <chrono>

#include "VRTextureHelper.h"

#include "Matrices.h"

#include "hud.h"
#include "cl_util.h"
#include "cvardef.h"
#include "usercmd.h"
#include "const.h"

#include "entity_state.h"
#include "cl_entity.h"
#include "ref_params.h"
#include "in_defs.h"  // PITCH YAW ROLL
#include "pm_movevars.h"
#include "pm_shared.h"
#include "pm_defs.h"
#include "event_api.h"
#include "pmtrace.h"
#include "screenfade.h"
#include "shake.h"
#include "hltv.h"
#include "keydefs.h"

#include "event_api.h"
#include "triangleapi.h"
#include "r_efx.h"
#include "r_studioint.h"

#include "VRRenderer.h"
#include "VRHelper.h"

#define HARDWARE_MODE
#include "com_model.h"

#include "vr_gl.h"


extern globalvars_t* gpGlobals;


// Collect faces of world
#include <vector>
#include <unordered_map>
#include <functional>

constexpr int PLAYER_WIDTH = 36;
constexpr int PLAYER_WIDTH_SQUARED = PLAYER_WIDTH * PLAYER_WIDTH;

constexpr float MIN_DISTANCE = 240 + PLAYER_WIDTH;

constexpr int HL_TO_RP3D = 1;

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
		plane{ origin + Vector(face.polys->verts[0]), (face.flags & SURF_PLANEBACK) ? -face.plane->normal : face.plane->normal }
	{
		for (int i = 0; i < face.polys->numverts; ++i)
		{
			Vector vertexBefore = origin + Vector(face.polys->verts[(i > 0) ? (i - 1) : (face.polys->numverts - 1)]);
			Vector vertex = origin + Vector(face.polys->verts[i]);
			Vector vertexAfter = origin + Vector(face.polys->verts[(i < (face.polys->numverts - 1)) ? (i + 1) : 0]);

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
		return fabs(vertices[0].x - other.vertices[0].x) < MIN_DISTANCE && fabs(vertices[0].y - other.vertices[0].y) < MIN_DISTANCE && fabs(vertices[0].z - other.vertices[0].z) < MIN_DISTANCE;
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

void CollectFaces(const model_t* model, const Vector& origin, PlaneFacesMap& planeFaces, PlaneVertexMetaDataMap& planeVertexMetaData)
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

void TriangulateGapsBetweenCoplanarFaces(const TranslatedFace& faceA, const TranslatedFace& faceB, PlaneVertexMetaDataMap& planeVertexMetaData, std::vector<Vector>& vertices, std::vector<int>& indices)
{
	std::vector<Vector> mergedVertices;
	if (faceA.GetGapVertices(faceB, mergedVertices, planeVertexMetaData))
	{
		// Now we simply triangulate mergedVertices, which is either 3 or 4 vertices in size
		int indexoffset = vertices.size();
		vertices.insert(vertices.end(), mergedVertices.begin(), mergedVertices.end());

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

void TriangulateBSPFaces(const PlaneFacesMap& planeFaces, PlaneVertexMetaDataMap& planeVertexMetaData, std::vector<Vector>& vertices, std::vector<int>& indices)
{
	for (auto pair : planeFaces)
	{
		const TranslatedPlane& plane = pair.first;
		const std::vector<TranslatedFace>& faces = pair.second;
		for (size_t faceIndexA = 0; faceIndexA < faces.size(); ++faceIndexA)
		{
			const TranslatedFace& faceA = faces[faceIndexA];

			// First do normal triangulation of all surfaces
			/*
			int indexoffset = vertices.size();
			for (size_t i = 0; i < faceA.GetVertices().size(); ++i)
			{
				vertices.push_back(faceA.GetVertices()[i] * HL_TO_RP3D);
				if (i < faceA.GetVertices().size() - 2)
				{
					indices.push_back(indexoffset + i + 2);
					indices.push_back(indexoffset + i + 1);
					indices.push_back(indexoffset);
				}
			}
			*/

			// Then do triangulation of gaps between faces that are too small for a player to fit through
			for (size_t faceIndexB = faceIndexA + 1; faceIndexB < faces.size(); ++faceIndexB)
			{
				const TranslatedFace& faceB = faces[faceIndexB];
				TriangulateGapsBetweenCoplanarFaces(faceA, faceB, planeVertexMetaData, vertices, indices);
			}
		}
	}
}



model_s* lol = nullptr;
float loldebug = 0;
PlaneFacesMap gPlaneFaces;
PlaneVertexMetaDataMap gPlaneVertexMetaData;
std::vector<Vector> gVertices;
std::vector<int> gIndices;
bool didntprintcountyet = true;

void VRRenderer::DebugRenderPhysicsPolygons()
{
	glEnable(GL_CULL_FACE);
	glCullFace(GL_FRONT);
	glBindTexture(GL_TEXTURE_2D, 0);
	glColor4f(1.f, 0.08f, 1.f, 0.58f);  // pink for debugging/testing

	cl_entity_s* map = gEngfuncs.GetEntityByIndex(0);
	if (map != nullptr && map->model != nullptr)
	{
		if (lol != map->model || loldebug != CVAR_GET_FLOAT("vr_debug_physics"))
		{
			// faces.clear();
			gPlaneVertexMetaData.clear();
			gPlaneFaces.clear();
			gVertices.clear();
			gIndices.clear();
			didntprintcountyet = true;
			lol = map->model;
			loldebug = CVAR_GET_FLOAT("vr_debug_physics");
			CollectFaces(lol, Vector(), gPlaneFaces, gPlaneVertexMetaData);
			TriangulateBSPFaces(gPlaneFaces, gPlaneVertexMetaData, gVertices, gIndices);
		}

		/*
		glDisable(GL_BLEND);
		glLineWidth(2);
		*/


		/*
		for (auto pair : gPlaneFaces)
		{
			const TranslatedPlane & plane = pair.first;
			for (const TranslatedFace & face : pair.second)
			{
				auto vertexMetaDataMap = gPlaneVertexMetaData[plane];
				for (size_t i = 0; i < face.GetVertices().size(); ++i)
				{
					const Vector & vertex = face.GetVertices()[i];
					const VertexMetaData & vertexMetaData = vertexMetaDataMap[vertex];
					if (vertexMetaData.totalCos > (EPSILON - 4.f) && !vertexMetaData.handled)
					{
						const Vector & vertexAfter = face.GetVertices()[(i < face.GetVertices().size() - 1) ? (i + 1) : 0];
						const VertexMetaData & vertexMetaDataAfter = vertexMetaDataMap[vertexAfter];
						if (vertexMetaDataAfter.totalCos > (EPSILON - 4.f) && !vertexMetaDataAfter.handled)
						{
							Vector dir{ vertex - vertexAfter };
							dir.InlineNormalize();

							Vector checkVertex{ vertex - dir };
							Vector checkVertexAfter{ vertexAfter + dir };

							const Vector checkDir = CrossProduct(dir, plane.GetNormal()) * 4;

							glColor4f(1.f, 0.f, 0.f, 1.f);
							glBegin(GL_LINES);
							glVertex3fv(checkVertex);
							glVertex3fv(checkVertex + checkDir);
							glEnd();

							glColor4f(0.f, 1.f, 0.f, 1.f);
							glBegin(GL_LINES);
							glVertex3fv(checkVertexAfter);
							glVertex3fv(checkVertexAfter + checkDir);
							glEnd();
						}
					}
				}
			}
		}
		*/


		/*
		for (auto pair : gPlaneFaces)
		{
			for (const TranslatedFace & face : pair.second)
			{
				// Draw lines of all faces in world
				glColor4f(1.f, 1.f, 0.f, 1.f);
				glBegin(GL_LINE_LOOP);
				for (const Vector & vertex : face.GetVertices())
				{
					glVertex3fv(vertex);
				}
				glEnd();
			}
		}
		*/


		/*
		// Draw lines of additional triangles
		for (size_t i = 0; i < gIndices.size(); i+=3)
		{
			glColor4f(1.f, 0.08f, 1.f, 1.f);
			glBegin(GL_LINE_LOOP);
			glVertex3fv(gVertices[gIndices[i]]);
			glVertex3fv(gVertices[gIndices[i + 1]]);
			glVertex3fv(gVertices[gIndices[i + 2]]);
			glEnd();
		}
		*/
	}
}
