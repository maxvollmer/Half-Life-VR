
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

enum class HUDRenderMode
{
	NONE,
	ACTUAL_HUD_IN_VIEW,
	ON_CONTROLLERS_ALIGNED_WITH_CONTROLLERS,
	ON_CONTROLLERS_ALIGNED_WITH_VIEW
};

#ifndef M_PI
#define M_PI 3.14159265358979323846f
#endif

void RotateVectorX(Vector &vecToRotate, const float angle)
{
	if (angle != 0.f)
	{
		float fCos = cos(angle);
		float fSin = sin(angle);

		float x = vecToRotate.x;
		float z = vecToRotate.z;

		vecToRotate.x = fCos * x + fSin * z;
		vecToRotate.z = -fSin * x + fCos * z;
	}
}

void RotateVectorY(Vector &vecToRotate, const float angle)
{
	if (angle != 0.f)
	{
		float fCos = cos(angle);
		float fSin = sin(angle);

		float x = vecToRotate.x;
		float y = vecToRotate.y;

		vecToRotate.x = fCos * x - fSin * y;
		vecToRotate.y = fSin * x + fCos * y;
	}
}

void RotateVectorZ(Vector &vecToRotate, const float angle)
{
	if (angle != 0.f)
	{
		float fCos = cos(angle);
		float fSin = sin(angle);

		float y = vecToRotate.y;
		float z = vecToRotate.z;

		vecToRotate.y = fCos * y - fSin * z;
		vecToRotate.z = fSin * y + fCos * z;
	}
}

void RotateVector(Vector &vecToRotate, const Vector &vecAngles)
{
	if (vecToRotate.LengthSquared() > 0.f && vecAngles.LengthSquared() > 0.f)
	{
		RotateVectorZ(vecToRotate, vecAngles.z / 180.*M_PI);
		RotateVectorX(vecToRotate, vecAngles.x / 180.*M_PI);
		RotateVectorY(vecToRotate, vecAngles.y / 180.*M_PI);
	}
}

HUDRenderMode GetHUDRenderMode()
{
	switch (HUDRenderMode(int(CVAR_GET_FLOAT("vr_hud_mode"))))
	{
	case HUDRenderMode::ACTUAL_HUD_IN_VIEW:
		return HUDRenderMode::ACTUAL_HUD_IN_VIEW;
	case HUDRenderMode::ON_CONTROLLERS_ALIGNED_WITH_CONTROLLERS:
		return HUDRenderMode::ON_CONTROLLERS_ALIGNED_WITH_CONTROLLERS;
	case HUDRenderMode::ON_CONTROLLERS_ALIGNED_WITH_VIEW:
		return HUDRenderMode::ON_CONTROLLERS_ALIGNED_WITH_VIEW;
	default:
		return HUDRenderMode::NONE;
	}
}

bool ShouldRenderHUD(const VRHUDRenderType hudRenderType)
{
	if (hudRenderType == VRHUDRenderType::TRAINCONTROLS)
		return true;

	if (GetHUDRenderMode() == HUDRenderMode::NONE)
		return false;

	switch (hudRenderType)
	{
	case VRHUDRenderType::AMMO:
	case VRHUDRenderType::AMMO_SECONDARY:
		return CVAR_GET_FLOAT("vr_hud_ammo") != 0.f;
	case VRHUDRenderType::HEALTH:
	case VRHUDRenderType::BATTERY:
		return CVAR_GET_FLOAT("vr_hud_health") != 0.f;
	case VRHUDRenderType::FLASHLIGHT:
		return CVAR_GET_FLOAT("vr_hud_flashlight") != 0.f;
	case VRHUDRenderType::TRAINCONTROLS:
		return true;
	default:
		return false;
	}
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
	if (!ShouldRenderHUD(m_hudRenderType))
		return;

	//gEngfuncs.pfnSPR_Set(hPic, r, g, b);
	m_hudSpriteHandle = hPic;
	m_hudSpriteColor.r = byte(r);
	m_hudSpriteColor.g = byte(g);
	m_hudSpriteColor.b = byte(b);
}

namespace
{
	const Vector VR_HUD_ACTUALHUD_AMMO_OFFSET{ 8.f, 0.f, 0.f };
	const Vector VR_HUD_ACTUALHUD_HEALTH_OFFSET{ 8.f, 1.f, 1.f };

	constexpr const float VR_HUD_TRAINCONTROLS_SPRITE_SCALE = 1.f;
	constexpr const float VR_HUD_CONTROLLER_SPRITE_SCALE = 0.1f;
	constexpr const float VR_HUD_ACTUALHUD_SPRITE_SCALE = 0.1f;

	constexpr const int VR_HUD_SPRITE_OFFSET_STEPSIZE = 4;
}

float GetVRHudSpriteScale(const VRHUDRenderType hudRenderType)
{
	if (hudRenderType == VRHUDRenderType::TRAINCONTROLS)
		return VR_HUD_TRAINCONTROLS_SPRITE_SCALE;

	float scale;
	if (GetHUDRenderMode() == HUDRenderMode::ACTUAL_HUD_IN_VIEW)
	{
		scale = VR_HUD_ACTUALHUD_SPRITE_SCALE;
		float scale2 = CVAR_GET_FLOAT("vr_hud_size");
		if (scale2 > 0.f)
			scale *= scale2;
	}
	else
	{
		scale = VR_HUD_CONTROLLER_SPRITE_SCALE;
	}

	float scale2 = CVAR_GET_FLOAT("vr_hud_textscale");
	if (scale2 > 0.f)
		scale *= scale2;

	return scale;
}

Vector GetActualHUDOffset(const Vector& offset, const Vector& angles)
{
	Vector actualOffset = offset;
	float scale = CVAR_GET_FLOAT("vr_hud_size");
	if (scale > 0.f)
	{
		actualOffset.y *= scale;
		actualOffset.z *= scale;
	}
	RotateVector(actualOffset, angles);
	return actualOffset;
}

void VRRenderer::GetViewAlignedHUDOrientationFromPosition(const Vector& origin, Vector& angles)
{
	Vector viewPos;
	vrHelper->GetViewOrg(viewPos);
	VectorAngles((origin - viewPos).Normalize(), angles);
}

bool VRRenderer::GetHUDAmmoOriginAndOrientation(Vector&origin, Vector&angles)
{
	if (GetHUDRenderMode() == HUDRenderMode::ACTUAL_HUD_IN_VIEW)
	{
		vrHelper->GetViewAngles(vr::Eye_Left, angles);
		vrHelper->GetViewOrg(origin);
		origin = origin + GetActualHUDOffset(VR_HUD_ACTUALHUD_AMMO_OFFSET, angles);
		return true;
	}
	else
	{
		if (!vrHelper->HasValidWeaponController())
			return false;
		origin = vrHelper->GetWeaponHUDPosition();
		if (GetHUDRenderMode() == HUDRenderMode::ON_CONTROLLERS_ALIGNED_WITH_CONTROLLERS)
		{
			angles = vrHelper->GetWeaponAngles();
		}
		else
		{
			GetViewAlignedHUDOrientationFromPosition(origin, angles);
		}
		return true;
	}
}

bool VRRenderer::GetHUDHealthOriginAndOrientation(Vector&origin, Vector&angles)
{
	if (GetHUDRenderMode() == HUDRenderMode::ACTUAL_HUD_IN_VIEW)
	{
		vrHelper->GetViewAngles(vr::Eye_Left, angles);
		vrHelper->GetViewOrg(origin);
		origin = origin + GetActualHUDOffset(VR_HUD_ACTUALHUD_HEALTH_OFFSET, angles);
		return true;
	}
	else
	{
		if (!vrHelper->HasValidHandController())
			return false;
		origin = vrHelper->GetHandHUDPosition();
		if (GetHUDRenderMode() == HUDRenderMode::ON_CONTROLLERS_ALIGNED_WITH_CONTROLLERS)
		{
			angles = vrHelper->GetHandAngles();
		}
		else
		{
			GetViewAlignedHUDOrientationFromPosition(origin, angles);
		}
	}
}

bool VRRenderer::GetHUDSpriteOriginAndOrientation(const VRHUDRenderType hudRenderType, Vector&origin, Vector&angles)
{
	switch (hudRenderType)
	{
	case VRHUDRenderType::AMMO:
	case VRHUDRenderType::AMMO_SECONDARY:
		return GetHUDAmmoOriginAndOrientation(origin, angles);
	case VRHUDRenderType::HEALTH:
	case VRHUDRenderType::BATTERY:
	case VRHUDRenderType::FLASHLIGHT:
		return GetHUDHealthOriginAndOrientation(origin, angles);
	case VRHUDRenderType::TRAINCONTROLS:
		return gHUD.GetTrainControlsOriginAndOrientation(origin, angles);
	default:
		return false;
	}
}

void VRRenderer::InterceptSPR_DrawAdditive(int frame, int x, int y, const wrect_t *prc)
{
	if (!ShouldRenderHUD(m_hudRenderType))
		return;

	Vector spriteOrigin;
	Vector spriteAngles;
	if (!GetHUDSpriteOriginAndOrientation(m_hudRenderType, spriteOrigin, spriteAngles))
		return;

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

		spriteWidth = static_cast<float>(prc->right - prc->left) * GetVRHudSpriteScale(m_hudRenderType);
		spriteHeight = static_cast<float>(prc->bottom - prc->top) * GetVRHudSpriteScale(m_hudRenderType);
	}
	else
	{
		textureLeft = 1;
		textureRight = 0;
		textureTop = 0;
		textureBottom = 1;

		spriteWidth = w * GetVRHudSpriteScale(m_hudRenderType);
		spriteHeight = h * GetVRHudSpriteScale(m_hudRenderType);
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
	glTranslatef(spriteOrigin.x, spriteOrigin.y, spriteOrigin.z);

	// Rotate sprite
	glRotatef(spriteAngles.y + 90, 0, 0, 1);

	// Move to starting position for HUD type
	glTranslatef(-hudStartPositionRightOffset, 0, hudStartPositionUpOffset);

	// Move to HUD offset
	glTranslatef((m_iHUDFirstSpriteX - x) * GetVRHudSpriteScale(m_hudRenderType), 0, (y - m_iHUDFirstSpriteY) * GetVRHudSpriteScale(m_hudRenderType));

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

	// Disable depth test if actual HUD (render over everything)
	if (GetHUDRenderMode() == HUDRenderMode::ACTUAL_HUD_IN_VIEW)
	{
		glDisable(GL_DEPTH_TEST);
	}

	// Call default HL HUD redraw code, we intercept SPR_Set and SPR_DrawAdditive to render the sprites on the controllers
	gHUD.Redraw(m_hudRedrawTime, m_hudRedrawIntermission);

	glPopAttrib();
}

void VRRenderer::GetStartingPosForHUDRenderType(const VRHUDRenderType hudRenderType, float & hudStartPositionUpOffset, float & hudStartPositionRightOffset)
{
	switch (hudRenderType)
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
		hudStartPositionRightOffset = -2 * VR_HUD_SPRITE_OFFSET_STEPSIZE;
		break;
	case VRHUDRenderType::BATTERY:
		hudStartPositionUpOffset = VR_HUD_SPRITE_OFFSET_STEPSIZE;
		hudStartPositionRightOffset = 2 * VR_HUD_SPRITE_OFFSET_STEPSIZE;
		break;
	case VRHUDRenderType::FLASHLIGHT:
		hudStartPositionUpOffset = VR_HUD_SPRITE_OFFSET_STEPSIZE * 2;
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

