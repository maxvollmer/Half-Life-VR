
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

#include "VRRenderer.h"
#include "VRHelper.h"

#define HARDWARE_MODE
#include "com_model.h"

#include "vr_gl.h"


extern globalvars_t *gpGlobals;

void VRRenderer::RenderScreenFade()
{
	// TODO...
	screenfade_t screenfade;
	gEngfuncs.pfnGetScreenFade(&screenfade);
	//screenfade.fadealpha
}

void RenderCube()
{
	float scale = 1.f;

	//Multi-colored side - FRONT
	glBindTexture(GL_TEXTURE_2D, VRTextureHelper::Instance().GetTexture("/skybox/desertft.png"));
	glBegin(GL_POLYGON);
	glTexCoord2f(0, 0);		glVertex3f(scale, -scale, -scale);
	glTexCoord2f(1, 0);		glVertex3f(scale, scale, -scale);
	glTexCoord2f(1, 1);		glVertex3f(-scale, scale, -scale);
	glTexCoord2f(0, 1);		glVertex3f(-scale, -scale, -scale);
	glEnd();

	// White side - BACK
	glBindTexture(GL_TEXTURE_2D, VRTextureHelper::Instance().GetTexture("/skybox/desertbk.png"));
	glBegin(GL_POLYGON);
	glTexCoord2f(0, 0);		glVertex3f(scale, -scale, scale);
	glTexCoord2f(1, 0);		glVertex3f(scale, scale, scale);
	glTexCoord2f(1, 1);		glVertex3f(-scale, scale, scale);
	glTexCoord2f(0, 1);		glVertex3f(-scale, -scale, scale);
	glEnd();

	// Purple side - RIGHT
	glBindTexture(GL_TEXTURE_2D, VRTextureHelper::Instance().GetTexture("/skybox/desertrt.png"));
	glBegin(GL_POLYGON);
	glTexCoord2f(0, 0);		glVertex3f(scale, -scale, -scale);
	glTexCoord2f(1, 0);		glVertex3f(scale, scale, -scale);
	glTexCoord2f(1, 1);		glVertex3f(scale, scale, scale);
	glTexCoord2f(0, 1);		glVertex3f(scale, -scale, scale);
	glEnd();

	// Green side - LEFT
	glBindTexture(GL_TEXTURE_2D, VRTextureHelper::Instance().GetTexture("/skybox/desertlf.png"));
	glBegin(GL_POLYGON);
	glTexCoord2f(0, 0);		glVertex3f(-scale, -scale, scale);
	glTexCoord2f(1, 0);		glVertex3f(-scale, scale, scale);
	glTexCoord2f(1, 1);		glVertex3f(-scale, scale, -scale);
	glTexCoord2f(0, 1);		glVertex3f(-scale, -scale, -scale);
	glEnd();

	// Blue side - TOP
	glBindTexture(GL_TEXTURE_2D, VRTextureHelper::Instance().GetTexture("/skybox/desertup.png"));
	glBegin(GL_POLYGON);
	glTexCoord2f(0, 0);		glVertex3f(scale, scale, scale);
	glTexCoord2f(1, 0);		glVertex3f(scale, scale, -scale);
	glTexCoord2f(1, 1);		glVertex3f(-scale, scale, -scale);
	glTexCoord2f(0, 1);		glVertex3f(-scale, scale, scale);
	glEnd();

	// Red side - BOTTOM
	glBindTexture(GL_TEXTURE_2D, VRTextureHelper::Instance().GetTexture("/skybox/desertdn.png"));
	glBegin(GL_POLYGON);
	glTexCoord2f(0, 0);		glVertex3f(scale, -scale, -scale);
	glTexCoord2f(1, 0);		glVertex3f(scale, -scale, scale);
	glTexCoord2f(1, 1);		glVertex3f(-scale, -scale, scale);
	glTexCoord2f(0, 1);		glVertex3f(-scale, -scale, -scale);
	glEnd();
}

void VRRenderer::RenderIntermissionRoom()
{
	glPushAttrib(GL_CURRENT_BIT | GL_DEPTH_BUFFER_BIT | GL_ENABLE_BIT | GL_POLYGON_BIT | GL_TEXTURE_BIT);

	glClearColor(0, 1, 1, 1);
	glClear(GL_COLOR_BUFFER_BIT);

	glDisable(GL_CULL_FACE);
	glDisable(GL_DEPTH_TEST);

	glEnable(GL_TEXTURE_2D);
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, 0);

	glPushMatrix();

	//Vector origin;
	//vrHelper->GetViewOrg(origin);
	//glTranslatef(origin.x, origin.y, origin.z);

	glColor4f(1, 1, 1, 1);

	RenderCube();

	glPopMatrix();

	glBindTexture(GL_TEXTURE_2D, 0);

	glPopAttrib();
}
