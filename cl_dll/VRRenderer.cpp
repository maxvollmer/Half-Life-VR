
#include <chrono>
#include <unordered_set>

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

#include "com_weapons.h"

#include "VRRenderer.h"
#include "VRHelper.h"
#include "VRInput.h"

#define HARDWARE_MODE
#include "com_model.h"

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

float lasttime = 0.f;

void VRRenderer::Frame(double time)
{
	// make sure these are always properly set
	gEngfuncs.pfnClientCmd("fps_max 90");
	gEngfuncs.pfnClientCmd("fps_override 1");
	gEngfuncs.pfnClientCmd("gl_vsync 0");
	gEngfuncs.pfnClientCmd("default_fov 180");
	gEngfuncs.pfnClientCmd("cl_mousegrab 0");
	//gEngfuncs.pfnClientCmd("firstperson");

	UpdateGameRenderState();

	float curtime = std::chrono::duration_cast<std::chrono::duration<float>>(std::chrono::system_clock::now().time_since_epoch()).count();
	float frametime = lasttime - curtime;

	//gEngfuncs.Con_DPrintf("Menu: %i, Ingame: %i, time: %f, frametime: %f!\n", m_isInMenu, m_isInGame, float(time), frametime);

	if (m_isInMenu || !IsInGame())
	{
		g_vrInput.ShowHLMenu();
		vrHelper->PollEvents(false, m_isInMenu);
	}
	else
	{
		g_vrInput.HideHLMenu();
	}

	if (!IsInGame() || (m_isInMenu && m_wasMenuJustRendered))
	{
		CaptureCurrentScreenToTexture(vrHelper->GetVRGLMenuTexture());
		m_wasMenuJustRendered = false;
	}
}

void VRRenderer::UpdateGameRenderState()
{
	m_isInGame = m_CalcRefdefWasCalled;
	m_CalcRefdefWasCalled = false;

	m_isInMenu = !m_HUDRedrawWasCalled;
	m_HUDRedrawWasCalled = false;

	if (!m_isInMenu) m_wasMenuJustRendered = false;
}

void VRRenderer::CalcRefdef(struct ref_params_s* pparams)
{
	m_CalcRefdefWasCalled = true;

	if (m_isInMenu || !IsInGame())
	{
		// Unfortunately the OpenVR overlay menu ignores controller input when we render the game scene.
		// (in fact controllers themselves aren't rendered at all)
		// So when the menu is open, we hide the game (the user will see a skybox and the menu)
		pparams->nextView = 0;
		pparams->onlyClientDraw = 1;
		m_fIsOnlyClientDraw = true;
		return;
	}

	if (pparams->nextView == 0)
	{
		vrHelper->PollEvents(true, m_isInMenu);
		vrHelper->UpdatePositions();

		vrHelper->PrepareVRScene(vr::EVREye::Eye_Left);
		vrHelper->GetViewOrg(pparams->vieworg);
		vrHelper->GetViewAngles(vr::EVREye::Eye_Left, pparams->viewangles);

		// Update player viewangles from HMD pose
		gEngfuncs.SetViewAngles(pparams->viewangles);

		pparams->nextView = 1;
		m_fIsOnlyClientDraw = false;
	}
	else if (pparams->nextView == 1)
	{
		vrHelper->FinishVRScene(pparams->viewport[2], pparams->viewport[3]);
		vrHelper->PrepareVRScene(vr::EVREye::Eye_Right);
		vrHelper->GetViewOrg(pparams->vieworg);
		vrHelper->GetViewAngles(vr::EVREye::Eye_Right, pparams->viewangles);

		// Update player viewangles from HMD pose
		gEngfuncs.SetViewAngles(pparams->viewangles);

		pparams->nextView = 2;
		m_fIsOnlyClientDraw = false;
	}
	else
	{
		vrHelper->FinishVRScene(pparams->viewport[2], pparams->viewport[3]);
		vrHelper->SubmitImages();

		pparams->nextView = 0;
		pparams->onlyClientDraw = 1;
		m_fIsOnlyClientDraw = true;
	}
}

void VRRenderer::DrawNormal()
{
	glPushAttrib(GL_CURRENT_BIT | GL_DEPTH_BUFFER_BIT | GL_ENABLE_BIT | GL_POLYGON_BIT | GL_TEXTURE_BIT);


	glPopAttrib();
}

void VRRenderer::DrawTransparent()
{
	glPushAttrib(GL_CURRENT_BIT | GL_DEPTH_BUFFER_BIT | GL_ENABLE_BIT | GL_POLYGON_BIT | GL_TEXTURE_BIT);

	if (m_fIsOnlyClientDraw)
	{
		GLuint backgroundTexture = VRTextureHelper::Instance().GetTexture("background.png");

		glClearColor(0, 0, 0, 1);
		glClear(GL_COLOR_BUFFER_BIT);

		glDisable(GL_CULL_FACE);
		glCullFace(GL_FRONT_AND_BACK);

		glDisable(GL_DEPTH_TEST);

		glMatrixMode(GL_MODELVIEW);
		glPushMatrix();
		glLoadIdentity();

		glMatrixMode(GL_PROJECTION);
		glPushMatrix();
		glLoadIdentity();

		glEnable(GL_TEXTURE_2D);

		if (m_isInMenu)
		{
			glBindTexture(GL_TEXTURE_2D, backgroundTexture);
			m_wasMenuJustRendered = true;
		}
		else
		{
			glBindTexture(GL_TEXTURE_2D, vrHelper->GetVRTexture(vr::EVREye::Eye_Left));
			m_wasMenuJustRendered = false;
		}

		glColor4f(1, 1, 1, 1);

		float size = 1.f;

		glBegin(GL_QUADS);
		glTexCoord2f(0, 0);		glVertex3f(-size, -size, -1.f);
		glTexCoord2f(1, 0);		glVertex3f(size, -size, -1.f);
		glTexCoord2f(1, 1);		glVertex3f(size, size, -1.f);
		glTexCoord2f(0, 1);		glVertex3f(-size, size, -1.f);
		glEnd();

		glMatrixMode(GL_PROJECTION);
		glPopMatrix();

		glMatrixMode(GL_MODELVIEW);
		glPopMatrix();

		glBindTexture(GL_TEXTURE_2D, 0);
	}
	else
	{
		RenderWorldBackfaces();

		if (CVAR_GET_FLOAT("vr_debug_controllers") != 0.f)
		{
			vrHelper->TestRenderControllerPositions();
		}

		RenderHUDSprites();

		RenderScreenFade();

		m_wasMenuJustRendered = false;
	}

	glPopAttrib();
}

void VRRenderer::InterceptHUDRedraw(float time, int intermission)
{
	// We don't call the HUD redraw code here - instead we just store the parameters
	// HUD redraw will be called by VRRenderer::RenderHUDSprites(), which gets called by StudioModelRenderer after rendering the viewmodels (see further below)
	// We then intercept SPR_Set and SPR_DrawAdditive and render the HUD sprites in 3d space at the controller positions

	m_hudRedrawTime = time;
	m_hudRedrawIntermission = intermission;

	m_HUDRedrawWasCalled = true;
}

void VRRenderer::GetViewAngles(float * angles)
{
	vrHelper->GetViewAngles(vr::EVREye::Eye_Right, angles);
}

Vector VRRenderer::GetMovementAngles()
{
	return vrHelper->GetMovementAngles();
}

bool VRRenderer::ShouldMirrorCurrentModel(cl_entity_t *ent)
{
	if (!ent)
		return false;

	if (gHUD.GetMirroredEnt() == ent)
		return true;

	if (gEngfuncs.GetViewModel() == ent)
		return vrHelper->IsViewEntMirrored();

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




#define SURF_NOCULL			1		// two-sided polygon (e.g. 'water4b')
#define SURF_PLANEBACK		2		// plane should be negated
#define SURF_DRAWSKY		4		// sky surface
#define SURF_WATERCSG		8		// culled by csg (was SURF_DRAWSPRITE)
#define SURF_DRAWTURB		16		// warp surface
#define SURF_DRAWTILED		32		// face without lighmap
#define SURF_CONVEYOR		64		// scrolled texture (was SURF_DRAWBACKGROUND)
#define SURF_UNDERWATER		128		// caustics
#define SURF_TRANSPARENT	256		// it's a transparent texture (was SURF_DONTWARP)

std::unordered_map<std::string, int>		m_originalTextureIDs;
void VRRenderer::ReplaceAllTexturesWithHDVersion(struct cl_entity_s *ent, bool enableHD)
{
	if (ent != nullptr && ent->model != nullptr)
	{
		for (int i = 0; i < ent->model->numtextures; i++)
		{
			texture_t *texture = ent->model->textures[i];

			// Replace textures with HD versions (created by Cyril Paulus using ESRGAN)
			if (texture != nullptr
				&& texture->name != nullptr
				&& strcmp("sky", texture->name) != 0
				&& texture->name[0] != '{'	// skip transparent textures, they are broken
				)
			{
				// Backup
				if (m_originalTextureIDs.count(texture->name) == 0)
				{
					m_originalTextureIDs[texture->name] = texture->gl_texturenum;
				}

				// Replace
				if (enableHD)
				{
					unsigned int hdTex = VRTextureHelper::Instance().GetHDGameTexture(texture->name);
					if (hdTex)
					{
						texture->gl_texturenum = int(hdTex);
					}
				}
				// Restore
				else
				{
					texture->gl_texturenum = m_originalTextureIDs[texture->name];
				}
			}
		}
	}
}

void VRRenderer::CheckAndIfNecessaryReplaceHDTextures(struct cl_entity_s *map)
{
	bool replaceHDTextures = false;
	if (map->model != m_currentMapModel)
	{
		m_originalTextureIDs.clear();
		m_currentMapModel = map->model;
		replaceHDTextures = true;
		vrHelper->SetSkyboxFromMap(map->model->name);
	}
	bool hdTexturesEnabled = CVAR_GET_FLOAT("vr_hd_textures_enabled") != 0.f;
	if (hdTexturesEnabled != m_hdTexturesEnabled)
	{
		m_hdTexturesEnabled = hdTexturesEnabled;
		replaceHDTextures = true;
	}
	if (m_hdTexturesEnabled && !replaceHDTextures)
	{
		if (m_originalTextureIDs.empty())
		{
			replaceHDTextures = true;
		}
		else
		{
			// Check if map uses different textures than it should (can happen after loading the same map again)
			if (map->model != nullptr)
			{
				for (int i = 0; !replaceHDTextures && i < map->model->numtextures; i++)
				{
					texture_t *texture = map->model->textures[i];
					if (texture != nullptr
						&& texture->name != nullptr
						&& strcmp("sky", texture->name) != 0
						&& texture->name[0] != '{'	// skip transparent textures, they are broken
						)
					{
						if (m_originalTextureIDs.count(texture->name) == 0
							|| m_originalTextureIDs[texture->name] == texture->gl_texturenum)
						{
							replaceHDTextures = true;
							break;
						}
					}
				}
			}
		}
	}
	if (replaceHDTextures)
	{
		ReplaceAllTexturesWithHDVersion(map, m_hdTexturesEnabled);
	}
}

// This method just draws the backfaces of the entire map in black, so the player can't peak "through" walls with their VR headset
void VRRenderer::RenderWorldBackfaces()
{
	glEnable(GL_CULL_FACE);
	glCullFace(GL_FRONT);
	glBindTexture(GL_TEXTURE_2D, 0);
	glColor4f(0, 0, 0, 1);

	cl_entity_s* map = gEngfuncs.GetEntityByIndex(0);
	if (map != nullptr && map->model != nullptr)
	{
		CheckAndIfNecessaryReplaceHDTextures(map);

		RenderBSPBackfaces(map->model);

		/* lol this actaually never worked, gpGlobals->maxEntities is server side only
		int currentmessagenum = gEngfuncs.GetLocalPlayer()->curstate.messagenum;
		for (int i = 1; i < gpGlobals->maxEntities; i++)
		{
			cl_entity_t *ent = gEngfuncs.GetEntityByIndex(i);
			ReplaceAllTexturesWithHDVersion(ent);
			if (ent != nullptr
				&& ent->model != nullptr
				&& ent->model->name != nullptr
				&& ent->model->name[0] == '*'
				&& ent->curstate.messagenum == currentmessagenum)
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
		*/
	}
	else
	{
		m_currentMapModel = nullptr;
	}
}

void VRRenderer::RenderBSPBackfaces(struct model_s* model)
{
	for (int i = 0; i < model->nummodelsurfaces; i++)
	{
		int surfaceIndex = model->firstmodelsurface + i;
		msurface_t *surface = &model->surfaces[surfaceIndex];

		// Render backfaces
		if (surface->texinfo != nullptr
			&& surface->texinfo->texture != nullptr
			&& surface->texinfo->texture->name != nullptr
			&& strcmp("sky", surface->texinfo->texture->name) != 0
			&& surface->texinfo->texture->name[0] != '!'
			&& !(surface->texinfo->flags & (SURF_NOCULL | SURF_DRAWSKY | SURF_WATERCSG | SURF_TRANSPARENT))
			)
		{
			glBegin(GL_POLYGON);
			if (surface->texinfo->flags & SURF_PLANEBACK)
			{
				for (int k = 0; k < surface->polys->numverts; k++)
				{
					glVertex3fv(surface->polys->verts[k]);
				}
			}
			else
			{
				for (int k = surface->polys->numverts - 1; k >= 0; k--)
				{
					glVertex3fv(surface->polys->verts[k]);
				}
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

bool VRRenderer::IsInGame()
{
	return m_isInGame
		&& gEngfuncs.pfnGetLevelName()[0] != '\0'
		&& gEngfuncs.GetMaxClients() > 0
		&& !CL_IsDead()
		&& !gHUD.m_iIntermission;
}

cl_entity_t* VRRenderer::SaveGetLocalPlayer()
{
	if (IsInGame())
	{
		return gEngfuncs.GetLocalPlayer();
	}
	else
	{
		return nullptr;
	}
}

extern cl_entity_t* SaveGetLocalPlayer()
{
	return gVRRenderer.SaveGetLocalPlayer();
}

vr::IVRSystem* VRRenderer::GetVRSystem()
{
	return vrHelper->GetVRSystem();
}



// For ev_common.cpp
Vector VRGlobalGetGunPosition()
{
	return gVRRenderer.GetHelper()->GetGunPosition();
}
void VRGlobalGetGunAim(Vector& forward, Vector& right, Vector& up, Vector& angles)
{
	gVRRenderer.GetHelper()->GetGunAim(forward, right, up, angles);
}
