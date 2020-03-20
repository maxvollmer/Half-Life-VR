
#include <functional>
#include <regex>
#include <unordered_set>

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
#include "VRTextureHelper.h"

#define HARDWARE_MODE
#include "com_model.h"

#include "vr_gl.h"


extern globalvars_t* gpGlobals;

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

	constexpr const float VR_HUD_TRAINCONTROLS_SPRITE_SCALE = 0.25f;

	constexpr const int VR_HUD_SPRITE_OFFSET_STEPSIZE = 40;

	std::unordered_set<std::string> g_hudMissingSpriteTextures;

	struct HUDSpriteSize
	{
		int width = 0;
		int height = 0;
	};
	std::unordered_map<std::string, HUDSpriteSize> g_hudSpriteResolutions = {
		{std::string{"sprites/640hud1.spr"}, HUDSpriteSize{256, 256}},
		{std::string{"sprites/640hud2.spr"}, HUDSpriteSize{256, 256}},
		{std::string{"sprites/640hud3.spr"}, HUDSpriteSize{256, 256}},
		{std::string{"sprites/640hud4.spr"}, HUDSpriteSize{256, 256}},
		{std::string{"sprites/640hud5.spr"}, HUDSpriteSize{256, 256}},
		{std::string{"sprites/640hud6.spr"}, HUDSpriteSize{256, 256}},
		{std::string{"sprites/640hud7.spr"}, HUDSpriteSize{256, 128}},
		{std::string{"sprites/640hud8.spr"}, HUDSpriteSize{256, 64}},
		{std::string{"sprites/640hud9.spr"}, HUDSpriteSize{256, 64}},
		{std::string{"sprites/640_train.spr"}, HUDSpriteSize{96, 96}},

		{std::string{"sprites/320hud1.spr"}, HUDSpriteSize{256, 256}},
		{std::string{"sprites/320hud2.spr"}, HUDSpriteSize{128, 128}},
		{std::string{"sprites/320hud3.spr"}, HUDSpriteSize{256, 64}},
		{std::string{"sprites/320hud4.spr"}, HUDSpriteSize{256, 32}},
		{std::string{"sprites/320_train.spr"}, HUDSpriteSize{48, 48}},
	};
}  // namespace

void VRRenderer::RotateVectorX(Vector& vecToRotate, const float angle)
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

void VRRenderer::RotateVectorY(Vector& vecToRotate, const float angle)
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

void VRRenderer::RotateVectorZ(Vector& vecToRotate, const float angle)
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

void VRRenderer::RotateVector(Vector& vecToRotate, const Vector& vecAngles, const bool reverse)
{
	if (vecToRotate.LengthSquared() > 0.f && vecAngles.LengthSquared() > 0.f)
	{
		if (reverse)
		{
			RotateVectorY(vecToRotate, vecAngles.y / 180. * M_PI);
			RotateVectorX(vecToRotate, vecAngles.x / 180. * M_PI);
			RotateVectorZ(vecToRotate, vecAngles.z / 180. * M_PI);
		}
		else
		{
			RotateVectorZ(vecToRotate, vecAngles.z / 180. * M_PI);
			RotateVectorX(vecToRotate, vecAngles.x / 180. * M_PI);
			RotateVectorY(vecToRotate, vecAngles.y / 180. * M_PI);
		}
	}
}

void VRRenderer::HLAnglesToGLMatrix(const Vector& angles, float matrix[16])
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

	float scale = 0.f;
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
	float x = CVAR_GET_FLOAT(("vr_hud_" + name + "_offset_x").data());
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

void RotatedGLCall(float x, float y, float z, Vector forward, Vector right, Vector up, std::function<void(float, float, float)> glCallback)
{
	// Can't wrap my head around getting a proper GL matrix from HL's euler angle mess,
	// so instead I just transform the coordinates in HL space by hand before sending them down to OpenGL
	// (can't be much slower anyways with these ffp calls...
	Vector result = (-right * x) + (forward * y) + (up * z);
	glCallback(result.x, result.y, result.z);
}

void VRRenderer::InterceptSPR_DrawAdditive(int frame, int x, int y, const wrect_t* prc)
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

	const model_s* pSpriteModel = gEngfuncs.GetSpritePointer(m_hudSpriteHandle);
	if (pSpriteModel == nullptr)
	{
		gEngfuncs.Con_DPrintf("Warning: HUD sprite model is nullptr!\n");
		return;
	}

	extern engine_studio_api_t IEngineStudio;
	IEngineStudio.GL_SetRenderMode(kRenderTransAdd);
	glColor4f(m_hudSpriteColor.r / 255.f, m_hudSpriteColor.g / 255.f, m_hudSpriteColor.b / 255.f, 1.f);

	glEnable(GL_TEXTURE_2D);
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, 0);

	int texture = 0;

	// "sprites/640hud7.spr" -> "hud/640hud7.png"
	std::string hudTextureName = std::regex_replace(pSpriteModel->name, std::regex{ "^sprites/([a-z0-9_]+)\\.spr$" }, m_hdTexturesEnabled ? "/hud/HD/$1.png" : "/hud/$1.png");
	if (hudTextureName.find("320_train") != std::string::npos)
	{
		hudTextureName = std::regex_replace(hudTextureName, std::regex{ "^(.+)320_train(.+)$" }, "$1640_train$2");
	}
	if (frame == 0 && hudTextureName.find("640_train") == std::string::npos)
	{
		texture = VRTextureHelper::Instance().GetTexture(hudTextureName);
	}
	if (texture == 0 || frame > 0)
	{
		// "hud/640_train.png" -> "hud/640_train_frame_xxx.png"
		hudTextureName = std::regex_replace(hudTextureName, std::regex{ "^(.+)\\.png$" }, "$1_frame_" + std::to_string(frame) + ".png");
		texture = VRTextureHelper::Instance().GetTexture(hudTextureName);
	}

	// give up
	if (texture == 0)
	{
		if (g_hudMissingSpriteTextures.count(pSpriteModel->name) == 0)
		{
			g_hudMissingSpriteTextures.insert(pSpriteModel->name);
			gEngfuncs.Con_DPrintf("Warning: HUD sprite model %s has no texture!\n", pSpriteModel->name);
		}
		return;
	}

	g_hudMissingSpriteTextures.erase(pSpriteModel->name);

	glBindTexture(GL_TEXTURE_2D, texture);

	// Half-Life thinks HUD textures are fixed size sprites and gives us absolute coordinates for that size.
	// We get the size here independent of the actual texture resolution, and then further below we calculate proper texture coordinates from 0 to 1.
	float w = g_hudSpriteResolutions[std::string{ pSpriteModel->name }].width;
	float h = g_hudSpriteResolutions[std::string{ pSpriteModel->name }].height;

	float textureLeft, textureRight, textureTop, textureBottom;
	float spriteWidth, spriteHeight;

	// Get texture coordinates and sprite dimensions
	if (prc != nullptr)
	{
		textureLeft = static_cast<float>(prc->left - 1) / static_cast<float>(w);
		textureRight = static_cast<float>(prc->right + 1) / static_cast<float>(w);
		textureTop = static_cast<float>(prc->top + 1) / static_cast<float>(h);
		textureBottom = static_cast<float>(prc->bottom - 1) / static_cast<float>(h);

		spriteWidth = static_cast<float>(prc->right - prc->left)* GetVRHudSpriteScale(m_hudRenderType);
		spriteHeight = static_cast<float>(prc->bottom - prc->top)* GetVRHudSpriteScale(m_hudRenderType);
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

	// Move to starting position for HUD type
	RotatedGLCall(-hudStartPositionRightOffset, 0.f, hudStartPositionUpOffset, spriteForward, spriteRight, spriteUp, &glTranslatef);

	// Move to HUD offset
	RotatedGLCall((m_iHUDFirstSpriteX - x) * GetVRHudSpriteScale(m_hudRenderType), 0.f, (y - m_iHUDFirstSpriteY) * GetVRHudSpriteScale(m_hudRenderType), spriteForward, spriteRight, spriteUp, &glTranslatef);

	// Draw sprite
	glBegin(GL_QUADS);
	glTexCoord2f(textureRight, textureTop);
	RotatedGLCall(0.f, 0.f, spriteHeight, spriteForward, spriteRight, spriteUp, &glVertex3f);
	glTexCoord2f(textureLeft, textureTop);
	RotatedGLCall(spriteWidth, 0.f, spriteHeight, spriteForward, spriteRight, spriteUp, &glVertex3f);
	glTexCoord2f(textureLeft, textureBottom);
	RotatedGLCall(spriteWidth, 0.f, 0.f, spriteForward, spriteRight, spriteUp, &glVertex3f);
	glTexCoord2f(textureRight, textureBottom);
	RotatedGLCall(0.f, 0.f, 0.f, spriteForward, spriteRight, spriteUp, &glVertex3f);
	glEnd();

	glBindTexture(GL_TEXTURE_2D, 0);

	glPopMatrix();
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

void VRRenderer::GetStartingPosForHUDRenderType(const VRHUDRenderType hudRenderType, float& hudStartPositionUpOffset, float& hudStartPositionRightOffset)
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
		hudStartPositionRightOffset = 48;
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

bool VRRenderer::HasValidLeftController()
{
	return vrHelper->HasValidLeftController();
}
Vector VRRenderer::GetLeftControllerPosition()
{
	return vrHelper->GetLeftControllerPosition();
}
Vector VRRenderer::GetLeftControllerAngles()
{
	return vrHelper->GetLeftControllerAngles();
}

bool VRRenderer::HasValidRightController()
{
	return vrHelper->HasValidRightController();
}
Vector VRRenderer::GetRightControllerPosition()
{
	return vrHelper->GetRightControllerPosition();
}
Vector VRRenderer::GetRightControllerAngles()
{
	return vrHelper->GetRightControllerAngles();
}

// For extern declarations in cl_util.h
void InterceptSPR_Set(HSPRITE_VALVE hPic, int r, int g, int b)
{
	gVRRenderer.InterceptSPR_Set(hPic, r, g, b);
}
void InterceptSPR_DrawAdditive(int frame, int x, int y, const wrect_t* prc)
{
	gVRRenderer.InterceptSPR_DrawAdditive(frame, x, y, prc);
}
