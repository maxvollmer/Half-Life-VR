
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

#include "vr_renderer.h"
#include "vr_helper.h"

#define HARDWARE_MODE
#include "com_model.h"

#include <windows.h>
#include "vr_gl.h"


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

void TestRenderMapInVR(vr::EVREye eEye);

void VRRenderer::CalcRefdef(struct ref_params_s* pparams)
{
	if (pparams->nextView == 0)
	{
		vrHelper->UpdatePositions(pparams);
		vrHelper->PrepareVRScene(vr::EVREye::Eye_Left, pparams);
		pparams->nextView = 1;
	}
	else if (pparams->nextView == 1)
	{
		vrHelper->FinishVRScene(pparams);
		vrHelper->PrepareVRScene(vr::EVREye::Eye_Right, pparams);
		pparams->nextView = 2;
	}
	else
	{
		vrHelper->FinishVRScene(pparams);
		vrHelper->SubmitImages();
		pparams->nextView = 0;
		pparams->onlyClientDraw = 1;

		glClearColor(0, 0, 0, 0);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		// Update player viewangles from HMD pose
		gEngfuncs.SetViewAngles(pparams->viewangles);
	}
}

void VRRenderer::DrawNormal()
{
	vrHelper->TestRenderControllerPosition();
}

void VRRenderer::DrawTransparent()
{
}

void VRRenderer::InterceptHUDRedraw(float time, int intermission)
{
	isInMenu = false;
	glClearColor(0, 0, 0, 0);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	gHUD.Redraw(time, intermission);
}

void VRRenderer::GetViewAngles(float * angles)
{
	vrHelper->GetViewAngles(angles);
}


void TestRenderMapInVR(vr::EVREye eEye)
{
	glDisable(GL_CULL_FACE);
	glCullFace(GL_NONE);

	cl_entity_s* map = gEngfuncs.GetEntityByIndex(0);
	if (map != nullptr)
	{
		model_t* model = map->model;
		if (model != nullptr)
		{
			for (int i = 0; i < model->nummodelsurfaces; i++)
			{
				int surfaceIndex = model->firstmodelsurface + i;
				msurface_t *surface = &model->surfaces[surfaceIndex];
				mtexinfo_t *texinfo = surface->texinfo;
				if (texinfo != nullptr && texinfo->texture != nullptr)
				{
					glBindTexture(GL_TEXTURE_2D, texinfo->texture->gl_texturenum);
					glBegin(GL_POLYGON);
					for (int k = surface->polys->numverts-1; k >= 0; k--)
					{
						glTexCoord2f(surface->polys->verts[k][3], surface->polys->verts[k][4]);
						glVertex3fv(surface->polys->verts[k]);
					}
					glEnd();
					glBindTexture(GL_TEXTURE_2D, 0);
				}
			}
		}
	}
}

