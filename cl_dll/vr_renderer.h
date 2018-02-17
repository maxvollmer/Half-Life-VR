#pragma once

class VRHelper;

enum class VRHUDRenderType
{
	NONE=0,
	HEALTH,
	BATTERY,
	FLASHLIGHT,
	AMMO,
	AMMO_SECONDARY,
	TRAINCONTROLS,
	DAMAGEDECALS,
	PAIN
};

class VRRenderer
{
public:
	VRRenderer();
	~VRRenderer();

	void Init();
	void VidInit();

	void Frame(double time);
	void CalcRefdef(struct ref_params_s* pparams);

	void DrawTransparent();
	void DrawNormal();

	void InterceptHUDRedraw(float time, int intermission);

	void GetViewAngles(float * angles);

	// For studiomodelrenderer
	void EnableDepthTest();
	int GetLeftHandModelIndex();
	bool IsRightControllerValid();
	bool IsLeftControllerValid();
	const Vector & GetLeftHandPosition();
	const Vector & GetLeftHandAngles();

	void ReverseCullface();
	void RestoreCullface();

	// Called by HUD draw code
	void VRHUDDrawBegin(const VRHUDRenderType renderType);
	void InterceptSPR_Set(HSPRITE_VALVE hPic, int r, int g, int b);
	void InterceptSPR_DrawAdditive(int frame, int x, int y, const wrect_t *prc);
	void VRHUDDrawFinished();

	// Called by StudioModelRenderer after viewmodels have been drawn
	void RenderHUDSprites();

private:

	void RenderWorldBackfaces();
	void RenderBSPBackfaces(struct model_s* model);

	unsigned int vrGLMenuTexture = 0;
	unsigned int vrGLHUDTexture = 0;

	bool isInMenu = true;

	bool m_fIsOnlyClientDraw = false;

	VRHelper * vrHelper = nullptr;

	// HUD Render stuff
	VRHUDRenderType m_hudRenderType{VRHUDRenderType::NONE};
	HSPRITE_VALVE m_hudSpriteHandle{0};
	color24 m_hudSpriteColor{0,0,0};
	int m_iHUDFirstSpriteX = 0;
	int m_iHUDFirstSpriteY = 0;
	bool m_fIsFirstSprite = true;

	float m_hudRedrawTime{0.f};
	int m_hudRedrawIntermission{0};

	void GetStartingPosForHUDRenderType(const VRHUDRenderType m_hudRenderType, float & hudStartPositionUpOffset, float & hudStartPositionRightOffset);
};

extern VRRenderer gVRRenderer;
