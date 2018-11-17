#pragma once

#include "openvr/openvr.h"

extern void VectorAngles(const float *forward, float *angles);

class Positions
{
public:
	vr::TrackedDevicePose_t m_rTrackedDevicePose[vr::k_unMaxTrackedDeviceCount];

	Matrix4 m_mat4LeftProjection;
	Matrix4 m_mat4RightProjection;

	Matrix4 m_mat4LeftModelView;
	Matrix4 m_mat4RightModelView;
};

class VRHelper
{
public:
	VRHelper();
	~VRHelper();

	void Init();

	void PollEvents();
	bool UpdatePositions(struct ref_params_s* pparams);
	void SubmitImages();
	void PrepareVRScene(vr::EVREye eEye, struct ref_params_s* pparams);
	void FinishVRScene(struct ref_params_s* pparams);

	unsigned int GetVRTexture(vr::EVREye eEye);

	void GetViewAngles(vr::EVREye eEye, float * angles);
	void GetViewOrg(float * origin);

	void TestRenderControllerPosition(bool leftOrRight);

private:

	bool AcceptsDisclaimer();
	void Exit(const char* lpErrorMessage = nullptr);

	void UpdateGunPosition(struct ref_params_s* pparams);
	void SendPositionUpdateToServer();

	void SubmitImage(vr::EVREye eEye, unsigned int texture);

	Matrix4 GetAbsoluteHMDTransform();
	Matrix4 GetAbsoluteControllerTransform(vr::TrackedDeviceIndex_t controllerIndex);

	Matrix4 GetHMDMatrixProjectionEye(vr::EVREye eEye);
	Matrix4 GetHMDMatrixPoseEye(vr::EVREye eEye);
	Matrix4 ConvertSteamVRMatrixToMatrix4(const vr::HmdMatrix34_t &matPose);
	Matrix4 ConvertSteamVRMatrixToMatrix4(const vr::HmdMatrix44_t &matPose);

	Matrix4 GetModelViewMatrixFromAbsoluteTrackingMatrix(const Matrix4 &absoluteTrackingMatrix, const Vector translate);
	Vector GetOffsetInHLSpaceFromAbsoluteTrackingMatrix(const Matrix4 & absoluteTrackingMatrix);
	Vector GetPositionInHLSpaceFromAbsoluteTrackingMatrix(const Matrix4 & absoluteTrackingMatrix);

	Vector GetHLViewAnglesFromVRMatrix(const Matrix4 &mat);
	Vector GetHLAnglesFromVRMatrix(const Matrix4 &mat);

	Positions positions;

	bool isVRRoomScale = true;

	vr::IVRSystem *vrSystem = nullptr;
	vr::IVRCompositor *vrCompositor = nullptr;

	unsigned int vrGLLeftEyeFrameBuffer = 0;
	unsigned int vrGLRightEyeFrameBuffer = 0;

	unsigned int vrGLLeftEyeTexture = 0;
	unsigned int vrGLRightEyeTexture = 0;

	unsigned int vrGLMenuTexture = 0;
	unsigned int vrGLHUDTexture = 0;

	unsigned int vrRenderWidth = 0;
	unsigned int vrRenderHeight = 0;

	int m_vrUpdateTimestamp = 0;

	float m_lastYawUpdateTime = -1.f;
	float m_prevYaw = 0.f;
	float m_currentYaw = 0.f;
	Vector m_currentYawOffsetDelta;
	float m_hmdDuckHeightDelta = 0.f;

	float lastUpdatedVectorsFrametime = 0.f;
	Vector3 vrToHL;
	Vector3 hlToVR;
	void UpdateVRHLConversionVectors();
	void UpdateWorldRotation();

	bool m_fRightControllerValid = false;
	bool m_fLeftControllerValid = false;
	Vector m_leftControllerPosition;
	Vector m_leftControllerAngles;
	Vector m_rightControllerPosition;
	Vector m_rightControllerAngles;

public:
	const Vector3 & GetVRToHL();
	const Vector3 & GetHLToVR();

	bool IsRightControllerValid();
	bool IsLeftControllerValid();
	const Vector & GetLeftHandPosition();
	const Vector & GetLeftHandAngles();
	const Vector & GetRightHandPosition();
	const Vector & GetRightHandAngles();
};
