#pragma once

#include "openvr/openvr.h"

#include "Matrices.h"

#include "VRCommon.h"

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
	void GetViewMatrix(vr::EVREye eEye, float* matrix);
	void GetViewOrg(float * origin);
	void GetViewVectors(Vector& forward, Vector& right, Vector& up);

	void TestRenderControllerPositions();

	void SetPose(VRPoseType poseType, const vr::TrackedDevicePose_t& pose, vr::ETrackedControllerRole role);
	void ClearPose(VRPoseType poseType);

	bool m_hasMovementAngles{ false };
	Vector m_movementAngles;
	bool m_handAnglesValid{ false };
	Vector m_handAngles;
	Vector GetMovementAngles();

private:

	bool AcceptsDisclaimer();
	void Exit(const char* lpErrorMessage = nullptr);

	void UpdateHMD();
	void UpdateControllers();
	bool UpdateController(
		vr::TrackedDeviceIndex_t controllerIndex,
		Matrix4& controllerMatrix,
		Vector& controllerOffset, Vector& controllerPosition, Vector& controllerAngles, Vector& controllerVelocity,
		Vector &controllerForward, Vector &controllerRight, Vector &controllerUp);
	void SendPositionUpdateToServer();
	void UpdateViewEnt(bool isControllerValid, const Vector& controllerPosition, const Vector& controllerAngles, const Vector& controllerVelocity);

	void SubmitImage(vr::EVREye eEye, unsigned int texture);

	void MatrixVectors(const Matrix4& source, Vector& forward, Vector& right, Vector& up);

	Matrix4 GetAbsoluteHMDTransform();
	Matrix4 GetAbsoluteControllerTransform(vr::TrackedDeviceIndex_t controllerIndex);
	Matrix4 GetAbsoluteTransform(const vr::HmdMatrix34_t& vrTransform);

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
	float m_instantRotateYawValue = 0.f;
	Vector m_currentYawOffsetDelta;
	float m_hmdDuckHeightDelta = 0.f;

	bool m_hasGroundEntityYaw{ false };
	float m_groundEntityYaw{ 0.f };
	struct cl_entity_s* m_groundEntity{ nullptr };

	float lastUpdatedVectorsFrametime = 0.f;
	Vector3 vrToHL;
	Vector3 hlToVR;
	void UpdateVRHLConversionVectors();
	void UpdateWorldRotation();

	bool m_fLeftControllerValid = false;
	Matrix4 m_leftControllerMatrix;
	Vector m_leftControllerOffset;
	Vector m_leftControllerPosition;
	Vector m_leftControllerAngles;
	Vector m_leftControllerForward;
	Vector m_leftControllerRight;
	Vector m_leftControllerUp;
	Vector m_leftControllerVelocity;

	bool m_fRightControllerValid = false;
	Matrix4 m_rightControllerMatrix;
	Vector m_rightControllerOffset;
	Vector m_rightControllerPosition;
	Vector m_rightControllerAngles;
	Vector m_rightControllerForward;
	Vector m_rightControllerRight;
	Vector m_rightControllerUp;
	Vector m_rightControllerVelocity;

public:
	const Vector3& GetVRToHL();
	const Vector3& GetHLToVR();

	bool HasValidWeaponController();
	Vector GetWeaponPosition();
	Vector GetWeaponAngles();
	Vector GetWeaponHUDPosition();
	void GetWeaponVectors(Vector& forward, Vector& right, Vector& up);
	void GetWeaponHUDMatrix(float* matrix);

	bool HasValidHandController();
	Vector GetHandPosition();
	Vector GetHandAngles();
	Vector GetHandHUDPosition();
	void GetHandVectors(Vector& forward, Vector& right, Vector& up);
	void GetHandHUDMatrix(float* matrix);

	vr::IVRSystem* GetVRSystem();

	void InstantRotateYaw(float value);

	Vector GetGunPosition();
	Vector GetAutoaimVector();
	void GetGunAim(Vector& forward, Vector& right, Vector& up, Vector& angles);
	float GetAnalogFire();
	Vector GetLocalPlayerAngles();
};
