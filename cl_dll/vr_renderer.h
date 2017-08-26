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

private:

	void RenderWorldBackfaces();

	unsigned int vrGLMenuTexture = 0;
	unsigned int vrGLHUDTexture = 0;

	bool isInMenu = true;

	Vector m_vBackupViewOrg;
	Vector m_vBackupViewAngles;

	VRHelper *vrHelper = nullptr;
};

extern VRRenderer gVRRenderer;
