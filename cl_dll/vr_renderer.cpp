
#include "Matrices.h"

#include "hud.h"
#include "cl_util.h"
#include "cvardef.h"
#include "usercmd.h"
#include "const.h"

#include "entity_state.h"
#include "cl_entity.h"
#include "ref_params.h"
#include "in_defs.h" // PITCH YAW ROLL
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

#include "vr_renderer.h"
#include "vr_helper.h"

#define HARDWARE_MODE
#include "com_model.h"

#include <windows.h>
#include "vr_gl.h"


extern globalvars_t *gpGlobals;

VRRenderer gVRRenderer;


VRRenderer::VRRenderer()
{
	vrHelper = new VRHelper();
}

VRRenderer::~VRRenderer()
{
	delete vrHelper;
	vrHelper = nullptr;
}

void VRRenderer::Init()
{
	vrHelper->Init();
}

void VRRenderer::VidInit()
{
}

void VRRenderer::Frame(double time)
{
	// make sure these are always properly set
	gEngfuncs.pfnClientCmd("fps_max 90");
	gEngfuncs.pfnClientCmd("fps_override 1");
	gEngfuncs.pfnClientCmd("gl_vsync 0");
	gEngfuncs.pfnClientCmd("default_fov 180");
	gEngfuncs.pfnClientCmd("cl_mousegrab 0");
	//gEngfuncs.pfnClientCmd("firstperson");

	if (isInMenu)
	{
		//vrHelper->CaptureMenuTexture();
		//CaptureCurrentScreenToTexture(vrGLMenuTexture);
	}
	else
	{
		//CaptureCurrentScreenToTexture(vrGLHUDTexture);
		isInMenu = true;
	}

	vrHelper->PollEvents();
}

void VRRenderer::CalcRefdef(struct ref_params_s* pparams)
{
	if (pparams->nextView == 0)
	{
		vrHelper->UpdatePositions(pparams);
		vrHelper->PrepareVRScene(vr::EVREye::Eye_Left, pparams);
		vrHelper->GetViewOrg(pparams->vieworg);
		vrHelper->GetViewAngles(vr::EVREye::Eye_Left, pparams->viewangles);

		// Update player viewangles from HMD pose
		gEngfuncs.SetViewAngles(pparams->viewangles);

		pparams->nextView = 1;
		m_fIsOnlyClientDraw = false;
	}
	else if (pparams->nextView == 1)
	{
		vrHelper->FinishVRScene(pparams);
		vrHelper->PrepareVRScene(vr::EVREye::Eye_Right, pparams);
		vrHelper->GetViewOrg(pparams->vieworg);
		vrHelper->GetViewAngles(vr::EVREye::Eye_Right, pparams->viewangles);

		// Update player viewangles from HMD pose
		gEngfuncs.SetViewAngles(pparams->viewangles);

		pparams->nextView = 2;
		m_fIsOnlyClientDraw = false;
	}
	else
	{
		vrHelper->FinishVRScene(pparams);
		vrHelper->SubmitImages();

		pparams->nextView = 0;
		pparams->onlyClientDraw = 1;
		m_fIsOnlyClientDraw = true;
	}
}

void VRRenderer::DrawNormal()
{
	glPushAttrib(GL_CURRENT_BIT | GL_DEPTH_BUFFER_BIT | GL_ENABLE_BIT | GL_POLYGON_BIT | GL_TEXTURE_BIT);

	if (m_fIsOnlyClientDraw)
	{
		glClearColor(0, 0, 0, 1);
		glClear(GL_COLOR_BUFFER_BIT);

		glDisable(GL_CULL_FACE);
		glCullFace(GL_FRONT_AND_BACK);

		glMatrixMode(GL_MODELVIEW);
		glPushMatrix();
		glLoadIdentity();

		glMatrixMode(GL_PROJECTION);
		glPushMatrix();
		glLoadIdentity();

		glEnable(GL_TEXTURE_2D);
		glBindTexture(GL_TEXTURE_2D, vrHelper->GetVRTexture(vr::EVREye::Eye_Left));

		glColor4f(1, 1, 1, 1);

		glBegin(GL_QUADS);
		glTexCoord2f(0, 0);		glVertex3i(-1, -1, -1);
		glTexCoord2f(1, 0);		glVertex3i(1, -1, -1);
		glTexCoord2f(1, 1);		glVertex3i(1, 1, -1);
		glTexCoord2f(0, 1);		glVertex3i(-1, 1, -1);		
		glEnd();

		glMatrixMode(GL_PROJECTION);
		glPopMatrix();

		glMatrixMode(GL_MODELVIEW);
		glPopMatrix();

		glBindTexture(GL_TEXTURE_2D, 0);
	}

	glPopAttrib();
}

void VRRenderer::DrawTransparent()
{
	if (!m_fIsOnlyClientDraw)
	{
		glPushAttrib(GL_CURRENT_BIT | GL_DEPTH_BUFFER_BIT | GL_ENABLE_BIT | GL_POLYGON_BIT | GL_TEXTURE_BIT);

		RenderWorldBackfaces();

		if (CVAR_GET_FLOAT("vr_debug_controllers") != 0.f)
		{
			vrHelper->TestRenderControllerPosition(true);
			vrHelper->TestRenderControllerPosition(false);
		}

		RenderHUDSprites();

		glPopAttrib();
	}
}

void VRRenderer::InterceptHUDRedraw(float time, int intermission)
{
	// We don't call the HUD redraw code here - instead we just store the parameters
	// HUD redraw will be called by VRRenderer::RenderHUDSprites(), which gets called by StudioModelRenderer after rendering the viewmodels (see further below)
	// We then intercept SPR_Set and SPR_DrawAdditive and render the HUD sprites in 3d space at the controller positions

	m_hudRedrawTime = time;
	m_hudRedrawIntermission = intermission;

	isInMenu = false;
}

void VRRenderer::GetViewAngles(float * angles)
{
	vrHelper->GetViewAngles(vr::EVREye::Eye_Right, angles);
}

bool VRRenderer::ShouldMirrorCurrentModel(cl_entity_t *ent)
{
	// TODO: Identify mirrored entities
	return false;
}

void VRRenderer::ReverseCullface()
{
	glFrontFace(GL_CW);
}

void VRRenderer::RestoreCullface()
{
	glFrontFace(GL_CCW);
}








// Collect faces of world
#include <vector>
#include <unordered_map>
#include <functional>

constexpr int PLAYER_WIDTH = 36;
constexpr int PLAYER_WIDTH_SQUARED = PLAYER_WIDTH*PLAYER_WIDTH;

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
	const float dist;
	const std::size_t hash;
	const PlaneMajor major;
};

typedef std::unordered_map<TranslatedPlane, std::unordered_map<Vector, VertexMetaData, VectorHash, VectorEqual>, TranslatedPlane::Hash, TranslatedPlane::Equal> PlaneVertexMetaDataMap;

class TranslatedFace
{
public:
	TranslatedFace(const msurface_t & face, const Vector & origin, PlaneVertexMetaDataMap & planeVertexMetaData)
		: plane{ origin + face.polys->verts[0], (face.flags & SURF_PLANEBACK) ? -face.plane->normal : face.plane->normal }
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

void TriangulateGapsBetweenCoplanarFaces(const TranslatedFace & faceA, const TranslatedFace & faceB, PlaneVertexMetaDataMap & planeVertexMetaData, std::vector<Vector> & vertices, std::vector<int> & indices)
{
	std::vector<Vector> mergedVertices;
	if (faceA.GetGapVertices(faceB, mergedVertices, planeVertexMetaData))
	{
		// Now we simply triangulate mergedVertices, which is either 3 or 4 vertices in size
		int indexoffset = vertices.size();
		vertices.insert(vertices.end(), mergedVertices.begin(), mergedVertices.end());

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

void TriangulateBSPFaces(const PlaneFacesMap & planeFaces, PlaneVertexMetaDataMap & planeVertexMetaData, std::vector<Vector> & vertices, std::vector<int> & indices)
{
	for (auto pair : planeFaces)
	{
		const TranslatedPlane & plane = pair.first;
		const std::vector<TranslatedFace> & faces = pair.second;
		for (size_t faceIndexA = 0; faceIndexA < faces.size(); ++faceIndexA)
		{
			const TranslatedFace & faceA = faces[faceIndexA];

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
				const TranslatedFace & faceB = faces[faceIndexB];
				TriangulateGapsBetweenCoplanarFaces(faceA, faceB, planeVertexMetaData, vertices, indices);
			}
		}
	}
}



model_s * lol = nullptr;
float loldebug = 0;
PlaneFacesMap gPlaneFaces;
PlaneVertexMetaDataMap gPlaneVertexMetaData;
std::vector<Vector> gVertices;
std::vector<int> gIndices;
bool didntprintcountyet = true;

// This method just draws the backfaces of the entire map in black, so the player can't peak "through" walls with their VR headset
void VRRenderer::RenderWorldBackfaces()
{
	glEnable(GL_CULL_FACE);
	glCullFace(GL_FRONT);
	glBindTexture(GL_TEXTURE_2D, 0);
	//glColor4f(1.f, 0.08f, 1.f, 0.58f);	// pink for debugging/testing
	glColor4f(0, 0, 0, 1);

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

		RenderBSPBackfaces(map->model);

		int currentmessagenum = gEngfuncs.GetLocalPlayer()->curstate.messagenum;

		for (int i = 1; i < gpGlobals->maxEntities; i++)
		{
			cl_entity_t *ent = gEngfuncs.GetEntityByIndex(i);
			if (ent != nullptr && ent->model != nullptr && ent->model->name != nullptr && ent->model->name[0] == '*' && ent->curstate.messagenum == currentmessagenum)
			{
				glPushMatrix();
				glTranslatef(ent->curstate.origin.x, ent->curstate.origin.y, ent->curstate.origin.z);
				glRotatef(ent->curstate.angles.x, 1, 0, 0);
				glRotatef(ent->curstate.angles.y, 0, 0, 1);
				glRotatef(ent->curstate.angles.z, 0, 1, 0);
				RenderBSPBackfaces(ent->model);
				glPopMatrix();
			}
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

#define SURF_NOCULL			1		// two-sided polygon (e.g. 'water4b')
#define SURF_PLANEBACK		2		// plane should be negated
#define SURF_DRAWSKY		4		// sky surface
#define SURF_WATERCSG		8		// culled by csg (was SURF_DRAWSPRITE)
#define SURF_DRAWTURB		16		// warp surface
#define SURF_DRAWTILED		32		// face without lighmap
#define SURF_CONVEYOR		64		// scrolled texture (was SURF_DRAWBACKGROUND)
#define SURF_UNDERWATER		128		// caustics
#define SURF_TRANSPARENT	256		// it's a transparent texture (was SURF_DONTWARP)

void VRRenderer::RenderBSPBackfaces(struct model_s* model)
{
	for (int i = 0; i < model->nummodelsurfaces; i++)
	{
		int surfaceIndex = model->firstmodelsurface + i;
		msurface_t *surface = &model->surfaces[surfaceIndex];
		if (surface->texinfo != nullptr && surface->texinfo->texture != nullptr && surface->texinfo->texture->name != nullptr && strcmp("sky", surface->texinfo->texture->name) != 0 && !(surface->texinfo->flags & SURF_DRAWSKY))
		{
			glBegin(GL_POLYGON);
			for (int k = surface->polys->numverts - 1; k >= 0; k--)
			{
				glVertex3fv(surface->polys->verts[k]);
			}
			glEnd();
		}
	}
}

// for studiomodelrenderer

void VRRenderer::EnableDepthTest()
{
	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LEQUAL);
	glDepthRange(0, 1);
}

int VRRenderer::GetLeftHandModelIndex()
{
	return gEngfuncs.pEventAPI->EV_FindModelIndex("models/v_gordon_hand.mdl");
}

bool VRRenderer::IsRightControllerValid()
{
	return vrHelper->IsRightControllerValid();
}

bool VRRenderer::IsLeftControllerValid()
{
	return vrHelper->IsLeftControllerValid();
}

const Vector & VRRenderer::GetLeftHandPosition()
{
	return vrHelper->GetLeftHandPosition();
}

const Vector & VRRenderer::GetLeftHandAngles()
{
	return vrHelper->GetLeftHandAngles();
}


// HUD Rendering on controllers
void VRRenderer::VRHUDDrawBegin(const VRHUDRenderType renderType)
{
	m_hudRenderType = renderType;
	m_iHUDFirstSpriteX = 0;
	m_iHUDFirstSpriteY = 0;
	m_fIsFirstSprite = true;
}

void VRRenderer::InterceptSPR_Set(HSPRITE_VALVE hPic, int r, int g, int b)
{
	//gEngfuncs.pfnSPR_Set(hPic, r, g, b);
	m_hudSpriteHandle = hPic;
	m_hudSpriteColor.r = byte(r);
	m_hudSpriteColor.g = byte(g);
	m_hudSpriteColor.b = byte(b);
}

#define VR_HUD_SPRITE_SCALE 0.1f
#define VR_HUD_SPRITE_OFFSET_STEPSIZE 4

void VRRenderer::InterceptSPR_DrawAdditive(int frame, int x, int y, const wrect_t *prc)
{
	//gEngfuncs.pfnSPR_DrawAdditive(frame, x, y, prc);

	if (m_fIsFirstSprite)
	{
		m_iHUDFirstSpriteX = x;
		m_iHUDFirstSpriteY = y;
		m_fIsFirstSprite = false;
	}

	const model_s * pSpriteModel = gEngfuncs.GetSpritePointer(m_hudSpriteHandle);
	if (pSpriteModel == nullptr)
	{
		gEngfuncs.Con_DPrintf("Error: HUD Sprite model is NULL!\n");
		return;
	}

	Vector controllerPos;
	//Vector controllerAngles;

	switch (m_hudRenderType)
	{
	case VRHUDRenderType::AMMO:
	case VRHUDRenderType::AMMO_SECONDARY:
		controllerPos = vrHelper->GetRightHandPosition();
		//controllerAngles = vrHelper->GetRightHandAngles();
		break;
	default:
		controllerPos = vrHelper->GetLeftHandPosition();
		//controllerAngles = vrHelper->GetLeftHandAngles();
		break;
	}

	/*
	Vector controllerViewAngles(-controllerAngles.x, controllerAngles.y, controllerAngles.z);
	Vector controllerForward;
	Vector controllerRight;
	Vector controllerUp;
	AngleVectors(controllerViewAngles, controllerForward, controllerRight, controllerUp);
	*/

	Vector viewPos;
	vrHelper->GetViewOrg(viewPos);
	Vector spriteNormal = controllerPos - viewPos;

	Vector spriteAngles;
	VectorAngles(spriteNormal, spriteAngles);

	extern engine_studio_api_t IEngineStudio;
	gEngfuncs.pTriAPI->SpriteTexture(const_cast<model_s *>(pSpriteModel), frame);
	IEngineStudio.GL_SetRenderMode(kRenderTransAdd);
	glColor4f(m_hudSpriteColor.r / 255.f, m_hudSpriteColor.g / 255.f, m_hudSpriteColor.b / 255.f, 1.f);

	float w, h;
	glGetTexLevelParameterfv(GL_TEXTURE_2D, 0, GL_TEXTURE_WIDTH, &w);
	glGetTexLevelParameterfv(GL_TEXTURE_2D, 0, GL_TEXTURE_HEIGHT, &h);

	float textureLeft, textureRight, textureTop, textureBottom;
	float spriteWidth, spriteHeight;

	// Get texture coordinates and sprite dimensions
	if (prc != nullptr)
	{
		textureLeft = static_cast<float>(prc->left - 1) / static_cast<float>(w);
		textureRight = static_cast<float>(prc->right + 1) / static_cast<float>(w);
		textureTop = static_cast<float>(prc->top + 1) / static_cast<float>(h);
		textureBottom = static_cast<float>(prc->bottom - 1) / static_cast<float>(h);

		spriteWidth = static_cast<float>(prc->right - prc->left) * VR_HUD_SPRITE_SCALE;
		spriteHeight = static_cast<float>(prc->bottom - prc->top) * VR_HUD_SPRITE_SCALE;
	}
	else
	{
		textureLeft = 1;
		textureRight = 0;
		textureTop = 0;
		textureBottom = 1;

		spriteWidth = w * VR_HUD_SPRITE_SCALE;
		spriteHeight = h * VR_HUD_SPRITE_SCALE;
	}

	float hudStartPositionUpOffset = 0;
	float hudStartPositionRightOffset = 0;
	GetStartingPosForHUDRenderType(m_hudRenderType, hudStartPositionUpOffset, hudStartPositionRightOffset);

	// Scale sprite
	cl_entity_t *viewent = gEngfuncs.GetViewModel();
	if (viewent != nullptr)
	{
		spriteWidth *= viewent->curstate.scale;
		spriteHeight *= viewent->curstate.scale;
		hudStartPositionUpOffset *= viewent->curstate.scale;
		hudStartPositionRightOffset *= viewent->curstate.scale;
	}

	glPushMatrix();

	// Move to controller position
	glTranslatef(controllerPos.x, controllerPos.y, controllerPos.z);

	// Rotate sprite
	glRotatef(spriteAngles.y + 90, 0, 0, 1);

	// Move to starting position for HUD type
	glTranslatef(-hudStartPositionRightOffset, 0, hudStartPositionUpOffset);

	// Move to HUD offset
	glTranslatef((m_iHUDFirstSpriteX - x) * VR_HUD_SPRITE_SCALE, 0, (y - m_iHUDFirstSpriteY) * VR_HUD_SPRITE_SCALE);

	// Draw sprite
	glBegin(GL_QUADS);
	glTexCoord2f(textureRight, textureTop);		glVertex3i(0, 0, spriteHeight);
	glTexCoord2f(textureLeft, textureTop);		glVertex3i(spriteWidth, 0, spriteHeight);
	glTexCoord2f(textureLeft, textureBottom);	glVertex3i(spriteWidth, 0, 0);
	glTexCoord2f(textureRight, textureBottom);	glVertex3i(0, 0, 0);
	glEnd();

	glPopMatrix();


	// gEngfuncs.Con_DPrintf("HUD: %i, %i %i, %i %i\n", m_hudRenderType, x, y, spriteWidth, spriteHeight);
}


void VRRenderer::VRHUDDrawFinished()
{
	m_hudRenderType = VRHUDRenderType::NONE;
	m_iHUDFirstSpriteX = 0;
	m_iHUDFirstSpriteY = 0;
	m_fIsFirstSprite = true;
}

void VRRenderer::RenderHUDSprites()
{
	glPushAttrib(GL_CURRENT_BIT | GL_DEPTH_BUFFER_BIT | GL_ENABLE_BIT | GL_POLYGON_BIT | GL_TEXTURE_BIT);

	glDisable(GL_CULL_FACE);
	glCullFace(GL_FRONT_AND_BACK);

	// Call default HL HUD redraw code, we intercept SPR_Set and SPR_DrawAdditive to render the sprites on the controllers
	gHUD.Redraw(m_hudRedrawTime, m_hudRedrawIntermission);

	glPopAttrib();
}

void VRRenderer::GetStartingPosForHUDRenderType(const VRHUDRenderType m_hudRenderType, float & hudStartPositionUpOffset, float & hudStartPositionRightOffset)
{
	switch (m_hudRenderType)
	{
	case VRHUDRenderType::AMMO:
		hudStartPositionUpOffset = VR_HUD_SPRITE_OFFSET_STEPSIZE;
		hudStartPositionRightOffset = -VR_HUD_SPRITE_OFFSET_STEPSIZE;
		break;
	case VRHUDRenderType::AMMO_SECONDARY:
		hudStartPositionUpOffset = VR_HUD_SPRITE_OFFSET_STEPSIZE;
		hudStartPositionRightOffset = 0;
		break;
	case VRHUDRenderType::HEALTH:
		hudStartPositionUpOffset = VR_HUD_SPRITE_OFFSET_STEPSIZE;
		hudStartPositionRightOffset = - 2 * VR_HUD_SPRITE_OFFSET_STEPSIZE;
		break;
	case VRHUDRenderType::BATTERY:
		hudStartPositionUpOffset = VR_HUD_SPRITE_OFFSET_STEPSIZE;
		hudStartPositionRightOffset = 2 * VR_HUD_SPRITE_OFFSET_STEPSIZE;
		break;
	case VRHUDRenderType::FLASHLIGHT:
		hudStartPositionUpOffset = VR_HUD_SPRITE_OFFSET_STEPSIZE *2;
		hudStartPositionRightOffset = 0;
		break;
	}
}


// For extern declarations in cl_util.h
void InterceptSPR_Set(HSPRITE_VALVE hPic, int r, int g, int b)
{
	gVRRenderer.InterceptSPR_Set(hPic, r, g, b);
}
void InterceptSPR_DrawAdditive(int frame, int x, int y, const wrect_t *prc)
{
	gVRRenderer.InterceptSPR_DrawAdditive(frame, x, y, prc);
}
