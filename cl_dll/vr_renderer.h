#pragma once

class VRHelper;

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

	void GetWalkAngles(float * angles);

private:

	void RenderWorldBackfaces();
	void RenderBSPBackfaces(struct model_s* model);

	unsigned int vrGLMenuTexture = 0;
	unsigned int vrGLHUDTexture = 0;

	bool isInMenu = true;

	VRHelper *vrHelper = nullptr;
};

extern VRRenderer gVRRenderer;
