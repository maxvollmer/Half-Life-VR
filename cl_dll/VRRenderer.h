#pragma once

#define VEC_HULL_MIN      Vector(-16, -16, -36)
#define VEC_HULL_MAX      Vector(16, 16, 36)
#define VEC_DUCK_HULL_MIN Vector(-16, -16, -18)
#define VEC_DUCK_HULL_MAX Vector(16, 16, 18)

class VRHelper;

enum class VRHUDRenderType
{
	NONE = 0,
	HEALTH,
	BATTERY,
	FLASHLIGHT,
	AMMO,
	AMMO_SECONDARY,
	TRAINCONTROLS,
	DAMAGEDECALS,
	PAIN
};

namespace vr
{
	class IVRSystem;
}

class VRRenderer
{
public:
	VRRenderer();
	~VRRenderer();

	enum class MultiPassMode : int
	{
		TRUE_DISPLAYLIST = 0,
		MIXED_DISPLAYLIST = 1
	};

	void Init();
	void VidInit();

	void Frame(double time);
	void UpdateGameRenderState();
	void CalcRefdef(struct ref_params_s* pparams);

	void DrawTransparent();
	void DrawNormal();
	void DrawHDSkyBox();

	void InterceptHUDRedraw(float time, int intermission);

	void GetViewAngles(float* angles);
	Vector GetMovementAngles();


	// For studiomodelrenderer
	void EnableDepthTest();
	void ReverseCullface();
	void RestoreCullface();

	bool ShouldMirrorCurrentModel(cl_entity_t* ent);
	bool IsCurrentModelHandWithSkeletalData(cl_entity_t* ent, float fingerCurl[5]);

	// Called in Frame if no game is running
	void RenderIntermissionRoom();

	// Called in DrawTransparent()
	void RenderHUDSprites();
	void RenderScreenOverlays();
	void DrawScreenFade();

	// Called by HUD draw code
	void VRHUDDrawBegin(const VRHUDRenderType renderType);
	void InterceptSPR_Set(HSPRITE_VALVE hPic, int r, int g, int b);
	void InterceptSPR_DrawAdditive(int frame, int x, int y, const wrect_t* prc);
	void VRHUDDrawFinished();

	VRHelper* GetHelper() { return vrHelper; }
	vr::IVRSystem* GetVRSystem();

	bool HasValidHandController();
	Vector GetHandPosition();
	Vector GetHandAngles();

	bool HasValidWeaponController();
	Vector GetWeaponPosition();
	Vector GetWeaponAngles();

	bool IsDeadInGame();
	bool IsInGame();
	struct cl_entity_s* SaveGetLocalPlayer();

	void SetViewOfs(const Vector& viewOfs);

	void RotateVectorX(Vector& vecToRotate, const float angle);
	void RotateVectorY(Vector& vecToRotate, const float angle);
	void RotateVectorZ(Vector& vecToRotate, const float angle);
	void RotateVector(Vector& vecToRotate, const Vector& vecAngles, const bool reverse = false);
	void HLAnglesToGLMatrix(const Vector& angles, float matrix[16]);

private:
	struct model_s* m_currentMapModel{ nullptr };
	bool m_hdTexturesEnabled{ false };
	void CheckAndIfNecessaryReplaceHDTextures();
	void ReplaceAllTexturesWithHDVersion(struct cl_entity_s* ent, bool enableHD);
	void CheckAndIfNecessaryReplaceHDTextures(struct cl_entity_s* ent);

	void DebugRenderPhysicsPolygons();

	bool m_fIsOnlyClientDraw = false;

	VRHelper* vrHelper = nullptr;

	bool m_isInGame{ false };
	bool m_isInMenu{ false };
	bool m_wasMenuJustRendered{ false };
	bool m_CalcRefdefWasCalled{ false };
	bool m_HUDRedrawWasCalled{ false };

	// HUD Render stuff
	VRHUDRenderType m_hudRenderType{ VRHUDRenderType::NONE };
	HSPRITE_VALVE m_hudSpriteHandle{ 0 };
	color24 m_hudSpriteColor{ 0, 0, 0 };
	int m_iHUDFirstSpriteX = 0;
	int m_iHUDFirstSpriteY = 0;
	bool m_fIsFirstSprite = true;

	float m_hudRedrawTime{ 0.f };
	int m_hudRedrawIntermission{ 0 };

	bool m_isDrawingDamage{ false };
	float m_flDrawingDamageTime = 0.f;
	Vector m_damageColor;

	unsigned int m_displayList{ 0 };

	bool m_isVeryFirstFrameEver{ true };

	void GetStartingPosForHUDRenderType(const VRHUDRenderType m_hudRenderType, float& hudStartPositionUpOffset, float& hudStartPositionRightOffset);
	bool GetHUDSpriteOriginAndOrientation(const VRHUDRenderType m_hudRenderType, Vector& origin, Vector& forward, Vector& right, Vector& up);
	bool GetHUDAmmoOriginAndOrientation(Vector& origin, Vector& forward, Vector& right, Vector& up);
	bool GetHUDHealthOriginAndOrientation(Vector& origin, Vector& forward, Vector& right, Vector& up);
	bool GetHUDFlashlightOriginAndOrientation(Vector& origin, Vector& forward, Vector& right, Vector& up);
};

extern VRRenderer gVRRenderer;
