
#include <functional>

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

enum class HUDRenderMode
{
	NONE,
	ACTUAL_HUD_IN_VIEW,
	ON_CONTROLLERS
};

#ifndef M_PI
#define M_PI 3.14159265358979323846f
#endif

namespace
{
	constexpr const float VR_HUD_ACTUALHUD_SPRITE_SCALE = 0.02f;

	constexpr const float VR_HUD_CONTROLLER_SPRITE_SCALE = 0.05f;

	constexpr const float VR_HUD_TRAINCONTROLS_SPRITE_SCALE = 0.5f;

	constexpr const int VR_HUD_SPRITE_OFFSET_STEPSIZE = 40;
}

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

void RotateVector(Vector &vecToRotate, const Vector &vecAngles, const bool reverse = false)
{
	if (vecToRotate.LengthSquared() > 0.f && vecAngles.LengthSquared() > 0.f)
	{
		if (reverse)
		{
			RotateVectorY(vecToRotate, vecAngles.y / 180.*M_PI);
			RotateVectorX(vecToRotate, vecAngles.x / 180.*M_PI);
			RotateVectorZ(vecToRotate, vecAngles.z / 180.*M_PI);
		}
		else
		{
			RotateVectorZ(vecToRotate, vecAngles.z / 180.*M_PI);
			RotateVectorX(vecToRotate, vecAngles.x / 180.*M_PI);
			RotateVectorY(vecToRotate, vecAngles.y / 180.*M_PI);
		}
	}
}

inline void HLAnglesToGLMatrix(const Vector& angles, float matrix[16])
{
	Vector modifiedAngles = -angles;
	Vector forward{ 1.f, 0.f, 0.f };
	Vector right{ 0.f, 1.f, 0.f };
	Vector up{ 0.f, 0.f, 1.f };
	RotateVector(forward, modifiedAngles, true);
	RotateVector(right, modifiedAngles, true);
	RotateVector(up, modifiedAngles, true);
	forward.CopyToArray(matrix);
	right.CopyToArray(matrix + 4);
	up.CopyToArray(matrix + 8);
	matrix[3] = matrix[7] = matrix[11] = matrix[12] = matrix[13] = matrix[14] = 0.f;
	matrix[15] = 1.f;
}

HUDRenderMode GetHUDRenderMode()
{
	switch (HUDRenderMode(int(CVAR_GET_FLOAT("vr_hud_mode"))))
	{
	case HUDRenderMode::ACTUAL_HUD_IN_VIEW:
		return HUDRenderMode::ACTUAL_HUD_IN_VIEW;
	case HUDRenderMode::ON_CONTROLLERS:
		return HUDRenderMode::ON_CONTROLLERS;
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
	case VRHUDRenderType::DAMAGEDECALS:
		return CVAR_GET_FLOAT("vr_hud_damage_indicator") != 0.f;
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

Vector GetLocalActualHUDOffset(const std::string& name)
{
	float x = CVAR_GET_FLOAT(("vr_hud_"+ name +"_offset_x").data());
	float y = CVAR_GET_FLOAT(("vr_hud_" + name + "_offset_y").data());
	return Vector{ 8.f, x, y };
}

Vector GetActualHUDOffset(const Vector& offset, const Vector& forward, const Vector& right, const Vector& up)
{
	Vector actualOffset = offset;
	float scale = CVAR_GET_FLOAT("vr_hud_size");
	if (scale > 0.f)
	{
		actualOffset.y *= scale;
		actualOffset.z *= scale;
	}
	return (forward * actualOffset.x) + (right * actualOffset.y) + (up * actualOffset.z);
}

bool VRRenderer::GetHUDAmmoOriginAndOrientation(Vector& origin, Vector& forward, Vector& right, Vector& up)
{
	if (GetHUDRenderMode() == HUDRenderMode::ACTUAL_HUD_IN_VIEW)
	{
		vrHelper->GetViewOrg(origin);
		vrHelper->GetViewVectors(forward, right, up);
		origin = origin + GetActualHUDOffset(GetLocalActualHUDOffset("ammo"), forward, right, up);
		return true;
	}
	else if (GetHUDRenderMode() == HUDRenderMode::ON_CONTROLLERS)
	{
		if (!vrHelper->HasValidWeaponController())
			return false;

		origin = vrHelper->GetWeaponHUDPosition();
		vrHelper->GetWeaponVectors(forward, right, up);
		up = vrHelper->GetWeaponHUDUp();
		return true;
	}

	return false;
}

bool VRRenderer::GetHUDHealthOriginAndOrientation(Vector& origin, Vector& forward, Vector& right, Vector& up)
{
	if (GetHUDRenderMode() == HUDRenderMode::ACTUAL_HUD_IN_VIEW)
	{
		vrHelper->GetViewOrg(origin);
		vrHelper->GetViewVectors(forward, right, up);
		origin = origin + GetActualHUDOffset(GetLocalActualHUDOffset("health"), forward, right, up);
		return true;
	}
	else if (GetHUDRenderMode() == HUDRenderMode::ON_CONTROLLERS)
	{
		if (!vrHelper->HasValidHandController())
			return false;

		origin = vrHelper->GetHandHUDPosition();
		vrHelper->GetHandVectors(forward, right, up);
		up = vrHelper->GetHandHUDUp();
		return true;
	}

	return false;
}

bool VRRenderer::GetHUDFlashlightOriginAndOrientation(Vector& origin, Vector& forward, Vector& right, Vector& up)
{
	if (GetHUDRenderMode() == HUDRenderMode::ACTUAL_HUD_IN_VIEW)
	{
		vrHelper->GetViewOrg(origin);
		vrHelper->GetViewVectors(forward, right, up);
		origin = origin + GetActualHUDOffset(GetLocalActualHUDOffset("flashlight"), forward, right, up);
		return true;
	}
	else if (GetHUDRenderMode() == HUDRenderMode::ON_CONTROLLERS)
	{
		// No flashlight on controllers (too much clutter)
		return false;
	}

	return false;
}

bool VRRenderer::GetHUDSpriteOriginAndOrientation(const VRHUDRenderType hudRenderType, Vector& origin, Vector& forward, Vector& right, Vector& up)
{
	switch (hudRenderType)
	{
	case VRHUDRenderType::AMMO:
	case VRHUDRenderType::AMMO_SECONDARY:
		return GetHUDAmmoOriginAndOrientation(origin, forward, right, up);
	case VRHUDRenderType::HEALTH:
	case VRHUDRenderType::BATTERY:
	case VRHUDRenderType::DAMAGEDECALS:
		return GetHUDHealthOriginAndOrientation(origin, forward, right, up);
	case VRHUDRenderType::FLASHLIGHT:
		return GetHUDFlashlightOriginAndOrientation(origin, forward, right, up);
	case VRHUDRenderType::TRAINCONTROLS:
	{
		Vector angles;
		if (gHUD.GetTrainControlsOriginAndOrientation(origin, angles))
		{
			AngleVectors(angles, forward, right, up);
			return true;
		}
	}
	default:
		return false;
	}
}

void RotatedGLCall(float x, float y, float z, Vector forward, Vector right, Vector up, std::function<void(float,float,float)> glCallback)
{
	// Can't wrap my head around getting a proper GL matrix from HL's euler angle mess,
	// so instead I just transform the coordinates in HL space by hand before sending them down to OpenGL
	// (can't be much slower anyways with these ffp calls...
	Vector result = (-right * x) + (forward * y) + (up * z);
	glCallback(result.x, result.y, result.z);
}

void VRRenderer::InterceptSPR_DrawAdditive(int frame, int x, int y, const wrect_t *prc)
{
	if (!ShouldRenderHUD(m_hudRenderType))
		return;

	Vector spriteOrigin, spriteForward, spriteRight, spriteUp;
	if (!GetHUDSpriteOriginAndOrientation(m_hudRenderType, spriteOrigin, spriteForward, spriteRight, spriteUp))
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
	hudStartPositionUpOffset *= GetVRHudSpriteScale(m_hudRenderType);
	hudStartPositionRightOffset *= GetVRHudSpriteScale(m_hudRenderType);

	glPushMatrix();

	// Move to controller position
	glTranslatef(spriteOrigin.x, spriteOrigin.y, spriteOrigin.z);

	//glRotatef(90, 0, 0, 1);

	// Rotate sprite
	/*
	glRotatef(spriteAngles.y + 90, 0, 0, 1);
	glRotatef(spriteAngles.x, 1, 0, 0);
	glRotatef(spriteAngles.z, 0, 0, 1);
	*/
	/*
	glRotatef(spriteAngles.y + 90, 0, 0, 1);
	glRotatef(spriteAngles.x, 1, 0, 0);
	glRotatef(spriteAngles.z, 0, 1, 0);
	*/
	/*
	float matrix[16];
	matrix[3] = matrix[7] = matrix[11] = matrix[12] = matrix[13] = matrix[14] = 0.f;
	matrix[15] = 1.f;
	spriteAngles.y = -spriteAngles.y;
	AngleVectors(spriteAngles, matrix, matrix + 4, matrix + 8);
	glMultMatrixf(matrix);
	*/
	//float matrix[16];
	//HLAnglesToGLMatrix(spriteAngles, matrix);
	//glMultMatrixf(matrix);
	//glMultMatrixf(spriteMatrix);


	// Move to starting position for HUD type
	//glTranslatef(-hudStartPositionRightOffset, 0, hudStartPositionUpOffset);
	RotatedGLCall(-hudStartPositionRightOffset, 0.f, hudStartPositionUpOffset, spriteForward, spriteRight, spriteUp, &glTranslatef);

	// Move to HUD offset
	//glTranslatef((m_iHUDFirstSpriteX - x) * GetVRHudSpriteScale(m_hudRenderType), 0, (y - m_iHUDFirstSpriteY) * GetVRHudSpriteScale(m_hudRenderType));
	RotatedGLCall((m_iHUDFirstSpriteX - x) * GetVRHudSpriteScale(m_hudRenderType), 0.f, (y - m_iHUDFirstSpriteY) * GetVRHudSpriteScale(m_hudRenderType), spriteForward, spriteRight, spriteUp, &glTranslatef);

	// Draw sprite
	glBegin(GL_QUADS);
	glTexCoord2f(textureRight, textureTop);		RotatedGLCall(0.f, 0.f, spriteHeight, spriteForward, spriteRight, spriteUp, &glVertex3f);//glVertex3i(0, 0, spriteHeight);
	glTexCoord2f(textureLeft, textureTop);		RotatedGLCall(spriteWidth, 0.f, spriteHeight, spriteForward, spriteRight, spriteUp, &glVertex3f);//glVertex3i(spriteWidth, 0, spriteHeight);
	glTexCoord2f(textureLeft, textureBottom);	RotatedGLCall(spriteWidth, 0.f, 0.f, spriteForward, spriteRight, spriteUp, &glVertex3f);//glVertex3i(spriteWidth, 0, 0);
	glTexCoord2f(textureRight, textureBottom);	RotatedGLCall(0.f, 0.f, 0.f, spriteForward, spriteRight, spriteUp, &glVertex3f);//glVertex3i(0, 0, 0);
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
	case VRHUDRenderType::DAMAGEDECALS:
		hudStartPositionUpOffset = 1.5f * VR_HUD_SPRITE_OFFSET_STEPSIZE;
		hudStartPositionRightOffset = -1.5f * VR_HUD_SPRITE_OFFSET_STEPSIZE;
		break;
	case VRHUDRenderType::FLASHLIGHT:
		hudStartPositionUpOffset = VR_HUD_SPRITE_OFFSET_STEPSIZE * 2;
		hudStartPositionRightOffset = 0;
	case VRHUDRenderType::TRAINCONTROLS:
		hudStartPositionUpOffset = 0;
		hudStartPositionRightOffset = 32;
		break;
	}
}

bool VRRenderer::HasValidHandController()
{
	return vrHelper->HasValidHandController();
}
Vector VRRenderer::GetHandPosition()
{
	return vrHelper->GetHandPosition();
}
Vector VRRenderer::GetHandAngles()
{
	return vrHelper->GetHandAngles();
}

bool VRRenderer::HasValidWeaponController()
{
	return vrHelper->HasValidWeaponController();
}
Vector VRRenderer::GetWeaponPosition()
{
	return vrHelper->GetWeaponPosition();
}
Vector VRRenderer::GetWeaponAngles()
{
	return vrHelper->GetWeaponAngles();
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

