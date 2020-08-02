#pragma once

#include <array>

#include "openvr/openvr.h"

#include "Matrices.h"

#include "VRCommon.h"

extern void VectorAngles(const float* forward, float* angles);

class Positions
{
public:
	std::array<vr::TrackedDevicePose_t, vr::k_unMaxTrackedDeviceCount> m_rTrackedDevicePose = {{ 0 }};

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

	enum class VRSceneMode
	{
		LeftEye,
		RightEye,
		Engine
	};

	void PollEvents(bool isInGame, bool isInMenu);
	bool UpdatePositions(int viewent);
	void SubmitImages();
	void PrepareVRScene(VRSceneMode sceneMode);
	void FinishVRScene(float width, float height);

	unsigned int GetVRTexture(vr::EVREye eEye);
	unsigned int GetVRGLMenuTexture();

	int GetVRGLMenuTextureWidth() { return m_vrGLMenuTextureWidth; }
	int GetVRGLMenuTextureHeight() { return m_vrGLMenuTextureHeight; }

	void GetViewAngles(vr::EVREye eEye, float* angles);
	void GetViewMatrix(vr::EVREye eEye, float* matrix);
	void GetViewOrg(float* origin);
	void GetViewVectors(Vector& forward, Vector& right, Vector& up);

	void TestRenderControllerPositions();

	void SetPose(VRPoseType poseType, const vr::TrackedDevicePose_t& pose, vr::ETrackedControllerRole role);
	void ClearPose(VRPoseType poseType);

	bool m_hasMovementAngles{ false };
	Vector m_movementAngles;
	Vector GetMovementAngles();

	void SetSkybox(const char* name);
	void SetSkyboxFromMap(const char* namename);

private:
	void Exit(const char* lpErrorMessage = nullptr);

	void UpdateHMD(int viewent);
	void UpdateControllers();
	bool UpdateController(
		vr::TrackedDeviceIndex_t controllerIndex,
		Matrix4& controllerMatrix,
		Vector& controllerOffset,
		Vector& controllerPosition,
		Vector& controllerAngles,
		Vector& controllerVelocity,
		Vector& controllerForward,
		Vector& controllerRight,
		Vector& controllerUp);
	void SendPositionUpdateToServer();
	void UpdateViewEnt(bool isControllerValid, const Vector& controllerPosition, const Vector& controllerAngles, const Vector& controllerVelocity, bool isMirrored);

	void InternalSubmitImages();
	void InternalSubmitImage(vr::EVREye eEye);

	void MatrixVectors(const Matrix4& source, Vector& forward, Vector& right, Vector& up);

	Matrix4 GetAbsoluteHMDTransform();
	Matrix4 GetAbsoluteControllerTransform(vr::TrackedDeviceIndex_t controllerIndex);
	Matrix4 GetAbsoluteTransform(const vr::HmdMatrix34_t& vrTransform);

	Matrix4 GetHMDMatrixProjectionEye(vr::EVREye eEye);
	Matrix4 GetHMDMatrixPoseEye(vr::EVREye eEye);
	Matrix4 ConvertSteamVRMatrixToMatrix4(const vr::HmdMatrix34_t& matPose);
	Matrix4 ConvertSteamVRMatrixToMatrix4(const vr::HmdMatrix44_t& matPose);

	Matrix4 GetModelViewMatrixFromAbsoluteTrackingMatrix(const Matrix4& absoluteTrackingMatrix, const Vector translate);
	Vector GetOffsetInHLSpaceFromAbsoluteTrackingMatrix(const Matrix4& absoluteTrackingMatrix);
	Vector GetPositionInHLSpaceFromAbsoluteTrackingMatrix(const Matrix4& absoluteTrackingMatrix);

	Vector GetHLViewAnglesFromVRMatrix(const Matrix4& mat);
	Vector GetHLAnglesFromVRMatrix(const Matrix4& mat);

	float GetStepHeight();
	float m_stepHeight{ 0 };
	Vector m_stepHeightOrigin;

	Positions positions;

	bool isVRRoomScale = true;

	float m_worldScale = 1.f;
	float m_worldZStretch = 1.f;

	vr::IVRSystem* vrSystem = nullptr;
	vr::IVRCompositor* vrCompositor = nullptr;

	unsigned int vrGLLeftEyeFrameBuffer = 0;
	unsigned int vrGLRightEyeFrameBuffer = 0;

	unsigned int vrGLLeftEyeTexture = 0;
	unsigned int vrGLRightEyeTexture = 0;
	unsigned int vrGLMenuTexture = 0;

	int m_vrGLMenuTextureWidth = 0;
	int m_vrGLMenuTextureHeight = 0;

	unsigned int vrRenderWidth = 0;
	unsigned int vrRenderHeight = 0;

	int m_vrUpdateTimestamp = 0;

	double m_lastYawUpdateTime = -1.f;
	float m_prevYaw = 0.f;
	float m_currentYaw = 0.f;
	bool m_hasReceivedYawUpdate = false;
	bool m_hasReceivedSpawnYaw = false;
	float m_instantRotateYawValue = 0.f;
	Vector2D m_currentYawOffsetDelta;

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

	bool mIsViewEntMirrored{ false };

	float m_hmdHeightOffset{ 0.f };

	int vr_scenePushCount = 0;

public:
	const Vector3& GetVRToHL();
	const Vector3& GetHLToVR();
	float UnitToMeter(float unit);

	bool HasValidLeftController() { return m_fLeftControllerValid; }
	Vector GetLeftControllerPosition() { return m_leftControllerPosition; }
	Vector GetLeftControllerAngles() { return m_leftControllerAngles; }

	bool HasValidRightController() { return m_fRightControllerValid; }
	Vector GetRightControllerPosition() { return m_rightControllerPosition; }
	Vector GetRightControllerAngles() { return m_rightControllerAngles; }

	bool HasValidWeaponController();
	Vector GetWeaponPosition();
	Vector GetWeaponAngles();
	Vector GetWeaponHUDPosition();
	Vector GetWeaponHUDUp();
	void GetWeaponVectors(Vector& forward, Vector& right, Vector& up);
	void GetWeaponHUDMatrix(float* matrix);

	bool HasValidHandController();
	Vector GetHandPosition();
	Vector GetHandAngles();
	Vector GetHandHUDPosition();
	Vector GetHandHUDUp();
	void GetHandVectors(Vector& forward, Vector& right, Vector& up);
	void GetHandHUDMatrix(float* matrix);

	vr::IVRSystem* GetVRSystem();

	void InstantRotateYaw(float value);

	Vector GetGunPosition();
	Vector GetAutoaimVector();
	void GetGunAim(Vector& forward, Vector& right, Vector& up, Vector& angles);
	float GetAnalogFire();
	Vector GetLocalPlayerAngles();

	bool IsViewEntMirrored() { return mIsViewEntMirrored; }

	bool CanAttack();
};
