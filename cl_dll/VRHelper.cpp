
#include "Matrices.h"
#include "hud.h"
#include "cl_util.h"
#include "r_studioint.h"
#include "ref_params.h"
#include "VRHelper.h"
#include "VRTextureHelper.h"
#include "vr_gl.h"
#include "VRInput.h"
#include <string>
#include <algorithm>

#ifndef DUCK_SIZE
#define DUCK_SIZE 36
#endif

#ifndef MAX_COMMAND_SIZE
#define MAX_COMMAND_SIZE 256
#endif

const Vector3 HL_TO_VR(1.44f / 10.f, 2.0f / 10.f, 1.44f / 10.f);
const Vector3 VR_TO_HL(1.f / HL_TO_VR.x, 1.f / HL_TO_VR.y, 1.f / HL_TO_VR.z);

// Set by message from server on load/restore/levelchange
float g_vrRestoreYaw_PrevYaw = 0.f;
float g_vrRestoreYaw_CurrentYaw = 0.f;
bool g_vrRestoreYaw_HasData = false;

// Set by message from server on spawn
float g_vrSpawnYaw = 0.f;
bool g_vrSpawnYaw_HasData = false;

extern engine_studio_api_t IEngineStudio;

extern cl_entity_t* SaveGetLocalPlayer();

enum class VRControllerID : int32_t {
	WEAPON = 0,
	HAND
};

VRHelper::VRHelper()
{
}

VRHelper::~VRHelper()
{
}

#define MAGIC_HL_TO_VR_FACTOR 3.75f
#define MAGIC_VR_TO_HL_FACTOR 10.f

void VRHelper::UpdateVRHLConversionVectors()
{
	float worldScale = CVAR_GET_FLOAT("vr_world_scale");
	float worldZStretch = CVAR_GET_FLOAT("vr_world_z_strech");

	if (worldScale < 0.1f)
	{
		worldScale = 0.1f;
	}
	else if (worldScale > 100.f)
	{
		worldScale = 100.f;
	}

	if (worldZStretch < 0.1f)
	{
		worldZStretch = 0.1f;
	}
	else if (worldZStretch > 100.f)
	{
		worldZStretch = 100.f;
	}

	hlToVR.x = worldScale * 1.f / MAGIC_HL_TO_VR_FACTOR;
	hlToVR.y = worldScale * 1.f / MAGIC_HL_TO_VR_FACTOR * worldZStretch;
	hlToVR.z = worldScale * 1.f / MAGIC_HL_TO_VR_FACTOR;

	vrToHL.x = MAGIC_VR_TO_HL_FACTOR * 1.f / hlToVR.x;
	vrToHL.y = MAGIC_VR_TO_HL_FACTOR * 1.f / hlToVR.y;
	vrToHL.z = MAGIC_VR_TO_HL_FACTOR * 1.f / hlToVR.z;
}

void VRHelper::UpdateWorldRotation()
{
	// Is rotating disabled?
	if (CVAR_GET_FLOAT("vr_playerturn_enabled") == 0.f)
	{
		m_lastYawUpdateTime = -1;
		m_prevYaw = 0.f;
		m_currentYaw = 0.f;
		m_instantRotateYawValue = 0.f;
		m_currentYawOffsetDelta = Vector{};
		return;
	}

	if (m_lastYawUpdateTime == gHUD.m_flTime)
	{
		// already up-to-date
		return;
	}

	// New game
	if (m_lastYawUpdateTime == -1.f || m_lastYawUpdateTime > gHUD.m_flTime)
	{
		m_lastYawUpdateTime = gHUD.m_flTime;
		m_prevYaw = 0.f;
		m_currentYaw = 0.f;
		m_instantRotateYawValue = 0.f;
		m_currentYawOffsetDelta = Vector{};
		return;
	}

	// Get angle from save/restore or from user input
	if (g_vrRestoreYaw_HasData)
	{
		m_prevYaw = g_vrRestoreYaw_PrevYaw;
		m_currentYaw = g_vrRestoreYaw_CurrentYaw;
		m_instantRotateYawValue = 0.f;
		g_vrRestoreYaw_PrevYaw = 0.f;
		g_vrRestoreYaw_CurrentYaw = 0.f;
		g_vrRestoreYaw_HasData = false;

		// Normalize angles
		m_prevYaw = std::fmodf(m_prevYaw, 360.f);
		if (m_prevYaw < 0.f) m_prevYaw += 360.f;
		m_currentYaw = std::fmodf(m_currentYaw, 360.f);
		if (m_currentYaw < 0.f) m_currentYaw += 360.f;
	}
	else
	{
		if (g_vrSpawnYaw_HasData)
		{
			// Fix spawn yaw
			Vector currentViewAngles;
			GetViewAngles(vr::EVREye::Eye_Left, currentViewAngles);
			m_currentYaw += g_vrSpawnYaw - currentViewAngles.y;
			g_vrSpawnYaw = 0.f;
			g_vrSpawnYaw_HasData = false;
		}

		// Already up to date
		if (gHUD.m_flTime == m_lastYawUpdateTime)
		{
			return;
		}

		// Get time since last update
		float deltaTime = gHUD.m_flTime - m_lastYawUpdateTime;
		// Rotate
		m_prevYaw = m_currentYaw;
		if (g_vrInput.RotateLeft())
		{
			m_currentYaw += deltaTime * CVAR_GET_FLOAT("cl_yawspeed");
		}
		else if (g_vrInput.RotateRight())
		{
			m_currentYaw -= deltaTime * CVAR_GET_FLOAT("cl_yawspeed");
		}

		// Add in analog VR input
		if (fabs(g_vrInput.analogturn) > EPSILON)
		{
			m_currentYaw += deltaTime * g_vrInput.analogturn * CVAR_GET_FLOAT("cl_yawspeed");
			g_vrInput.analogturn = 0.f;
		}

		// Calculate in instant rotation values from VR input
		m_currentYaw += m_instantRotateYawValue;
		m_instantRotateYawValue = 0.f;

		// Rotate with trains and platforms
		cl_entity_t *groundEntity = gHUD.GetGroundEntity();
		if (groundEntity)
		{
			if (groundEntity != m_groundEntity)
			{
				m_groundEntity = groundEntity;
				m_hasGroundEntityYaw = false;
			}
			if (CVAR_GET_FLOAT("vr_rotate_with_trains") != 0.f)
			{
				if (m_hasGroundEntityYaw)
				{
					m_currentYaw += groundEntity->angles.y - m_groundEntityYaw;
				}
			}
			m_groundEntityYaw = groundEntity->angles.y;
			m_hasGroundEntityYaw = true;
		}
		else
		{
			m_groundEntity = nullptr;
			m_groundEntityYaw = 0.f;
			m_hasGroundEntityYaw = false;
		}
		//g_vrInput.MoveWithGroundEntity(m_groundEntity);

		// Normalize angle
		m_currentYaw = std::fmodf(m_currentYaw, 360.f);
		if (m_currentYaw < 0.f) m_currentYaw += 360.f;

		// Remember time
		m_lastYawUpdateTime = gHUD.m_flTime;
	}
}

void VRHelper::InstantRotateYaw(float value)
{
	if (CVAR_GET_FLOAT("vr_playerturn_enabled") == 0.f)
		return;

	if (g_vrSpawnYaw_HasData)
		return;

	if (g_vrRestoreYaw_HasData)
		return;

	m_instantRotateYawValue += value;
}

const Vector3 & VRHelper::GetVRToHL()
{
	return vrToHL;
}

const Vector3 & VRHelper::GetHLToVR()
{
	return hlToVR;
}

void VRHelper::Init()
{
	if (!AcceptsDisclaimer())
	{
		Exit();
	}
	else if (!IEngineStudio.IsHardware())
	{
		Exit(TEXT("Software mode not supported. Please start this game with OpenGL renderer. (Start Half-Life, open the Options menu, select Video, chose OpenGL as Renderer, save, quit Half-Life, then start Half-Life: VR)"));
	}
	else if (!InitAdditionalGLFunctions())
	{
		Exit(TEXT("Failed to load necessary OpenGL functions. Make sure you have a graphics card that can handle VR and up-to-date drivers, and make sure you are running the game in OpenGL mode."));
	}
	else if (!InitGLMatrixOverrideFunctions())
	{
		Exit(TEXT("Failed to load custom opengl32.dll. Make sure you launch this game through HLVRLauncher.exe, not the Steam menu. (Tipp: You can add a custom game shortcut to HLVRLauncher.exe in your Steam library.))"));
	}
	else
	{
		vr::EVRInitError vrInitError;
		vrSystem = vr::VR_Init(&vrInitError, vr::EVRApplicationType::VRApplication_Scene);
		vrCompositor = vr::VRCompositor();
		if (vrInitError != vr::EVRInitError::VRInitError_None || vrSystem == nullptr || vrCompositor == nullptr)
		{
			Exit(TEXT("Failed to initialize VR enviroment. Make sure your headset is properly connected and SteamVR is running."));
		}
		else
		{
			vrSystem->GetRecommendedRenderTargetSize(&vrRenderWidth, &vrRenderHeight);
			CreateGLTexture(&vrGLLeftEyeTexture, vrRenderWidth, vrRenderHeight);
			CreateGLTexture(&vrGLRightEyeTexture, vrRenderWidth, vrRenderHeight);
			int viewport[4];
			glGetIntegerv(GL_VIEWPORT, viewport);
			m_vrGLMenuTextureWidth = viewport[2];
			m_vrGLMenuTextureHeight = viewport[3];
			CreateGLTexture(&vrGLMenuTexture, viewport[2], viewport[3]);
			if (vrGLLeftEyeTexture == 0 || vrGLRightEyeTexture == 0 || vrGLMenuTexture == 0)
			{
				Exit(TEXT("Failed to initialize OpenGL textures for VR enviroment. Make sure you have a graphics card that can handle VR and up-to-date drivers."));
			}
			else
			{
				CreateGLFrameBuffer(&vrGLLeftEyeFrameBuffer, vrGLLeftEyeTexture, vrRenderWidth, vrRenderHeight);
				CreateGLFrameBuffer(&vrGLRightEyeFrameBuffer, vrGLRightEyeTexture, vrRenderWidth, vrRenderHeight);
				if (vrGLLeftEyeFrameBuffer == 0 || vrGLRightEyeFrameBuffer == 0)
				{
					Exit(TEXT("Failed to initialize OpenGL framebuffers for VR enviroment. Make sure you have a graphics card that can handle VR and up-to-date drivers."));
				}
			}
		}
	}

	CVAR_CREATE("vr_debug_physics", "0", 0);
	CVAR_CREATE("vr_debug_controllers", "0", 0);
	CVAR_CREATE("vr_movecontrols", "0", FCVAR_ARCHIVE);
	CVAR_CREATE("vr_world_scale", "1", FCVAR_ARCHIVE);
	CVAR_CREATE("vr_world_z_strech", "1", FCVAR_ARCHIVE);
	CVAR_CREATE("vr_lefthand_mode", "0", FCVAR_ARCHIVE);
	CVAR_CREATE("vr_playerturn_enabled", "0", FCVAR_ARCHIVE);
	CVAR_CREATE("vr_rotate_with_trains", "1", FCVAR_ARCHIVE);
	CVAR_CREATE("vr_flashlight_toggle", "0", FCVAR_ARCHIVE);
	CVAR_CREATE("vr_hud_mode", "2", FCVAR_ARCHIVE);
	CVAR_CREATE("vr_hud_ammo", "1", FCVAR_ARCHIVE);
	CVAR_CREATE("vr_hud_health", "1", FCVAR_ARCHIVE);
	CVAR_CREATE("vr_hud_flashlight", "1", FCVAR_ARCHIVE);
	CVAR_CREATE("vr_hud_size", "1", FCVAR_ARCHIVE);
	CVAR_CREATE("vr_hud_textscale", "1", FCVAR_ARCHIVE);
	CVAR_CREATE("vr_hd_textures_enabled", "0", FCVAR_ARCHIVE);
	CVAR_CREATE("vr_move_instant_decelerate", "1", FCVAR_ARCHIVE);
	CVAR_CREATE("vr_move_instant_accelerate", "1", FCVAR_ARCHIVE);
	CVAR_CREATE("vr_move_analogforward_inverted", "0", FCVAR_ARCHIVE);
	CVAR_CREATE("vr_move_analogsideways_inverted", "0", FCVAR_ARCHIVE);
	CVAR_CREATE("vr_move_analogupdown_inverted", "0", FCVAR_ARCHIVE);
	CVAR_CREATE("vr_move_analogturn_inverted", "0", FCVAR_ARCHIVE);
	CVAR_CREATE("vr_hud_ammo_offset_x", "3", FCVAR_ARCHIVE);
	CVAR_CREATE("vr_hud_ammo_offset_y", "-3", FCVAR_ARCHIVE);
	CVAR_CREATE("vr_hud_health_offset_x", "-4", FCVAR_ARCHIVE);
	CVAR_CREATE("vr_hud_health_offset_y", "-3", FCVAR_ARCHIVE);
	CVAR_CREATE("vr_hud_flashlight_offset_x", "-5", FCVAR_ARCHIVE);
	CVAR_CREATE("vr_hud_flashlight_offset_y", "2", FCVAR_ARCHIVE);

	g_vrInput.Init();

	UpdateVRHLConversionVectors();

	// Set skybox
	SetSkybox("desert");
}

bool VRHelper::AcceptsDisclaimer()
{
	return true;
}

void VRHelper::Exit(const char* lpErrorMessage)
{
	vrSystem = nullptr;
	vrCompositor = nullptr;
	if (lpErrorMessage != nullptr)
	{
		gEngfuncs.Con_DPrintf("Error starting Half-Life: VR: %s\n", lpErrorMessage);
		std::cerr << "Error starting Half-Life: VR: " << lpErrorMessage << std::endl << std::flush;
	}
	vr::VR_Shutdown();
	gEngfuncs.pfnClientCmd("quit");
	std::exit(lpErrorMessage != nullptr ? 1 : 0);
}

Matrix4 VRHelper::GetHMDMatrixProjectionEye(vr::EVREye eEye)
{
	float fNearZ = 0.01f;
	float fFarZ = 8192.f;
	return ConvertSteamVRMatrixToMatrix4(vrSystem->GetProjectionMatrix(eEye, fNearZ, fFarZ));
}

Matrix4 VRHelper::GetHMDMatrixPoseEye(vr::EVREye nEye)
{
	return ConvertSteamVRMatrixToMatrix4(vrSystem->GetEyeToHeadTransform(nEye)).invert();
}

Matrix4 VRHelper::ConvertSteamVRMatrixToMatrix4(const vr::HmdMatrix34_t &mat)
{
	return Matrix4(
		mat.m[0][0], mat.m[1][0], mat.m[2][0], 0.0f,
		mat.m[0][1], mat.m[1][1], mat.m[2][1], 0.0f,
		mat.m[0][2], mat.m[1][2], mat.m[2][2], 0.0f,
		mat.m[0][3], mat.m[1][3], mat.m[2][3], 0.1f
	);
}

Matrix4 VRHelper::ConvertSteamVRMatrixToMatrix4(const vr::HmdMatrix44_t &mat)
{
	return Matrix4(
		mat.m[0][0], mat.m[1][0], mat.m[2][0], mat.m[3][0],
		mat.m[0][1], mat.m[1][1], mat.m[2][1], mat.m[3][1],
		mat.m[0][2], mat.m[1][2], mat.m[2][2], mat.m[3][2],
		mat.m[0][3], mat.m[1][3], mat.m[2][3], mat.m[3][3]
	);
}

Vector VRHelper::GetHLViewAnglesFromVRMatrix(const Matrix4 &mat)
{
	Vector4 v1 = mat * Vector4(1, 0, 0, 0);
	Vector4 v2 = mat * Vector4(0, 1, 0, 0);
	Vector4 v3 = mat * Vector4(0, 0, 1, 0);
	v1.normalize();
	v2.normalize();
	v3.normalize();
	Vector angles;
	VectorAngles(Vector(-v1.z, -v2.z, -v3.z), angles);
	angles.x = 360.f - angles.x;	// viewangles pitch is inverted
	return angles;
}

Vector VRHelper::GetHLAnglesFromVRMatrix(const Matrix4 &mat)
{
	Vector4 forwardInVRSpace = mat * Vector4{ 0, 0, -1, 0 };
	Vector4 rightInVRSpace = mat * Vector4{ 1, 0, 0, 0 };
	Vector4 upInVRSpace = mat * Vector4{ 0, 1, 0, 0 };

	Vector forward{ forwardInVRSpace.x, -forwardInVRSpace.z, forwardInVRSpace.y };
	Vector right{ rightInVRSpace.x, -rightInVRSpace.z, rightInVRSpace.y };
	Vector up{ upInVRSpace.x, -upInVRSpace.z, upInVRSpace.y };

	forward.Normalize();
	right.Normalize();
	up.Normalize();

	Vector angles;
	GetAnglesFromVectors(forward, right, up, angles);
	angles.x = 360.f - angles.x;
	return angles;
}

Matrix4 VRHelper::GetModelViewMatrixFromAbsoluteTrackingMatrix(const Matrix4 &absoluteTrackingMatrix, const Vector translate)
{
	Matrix4 hlToVRScaleMatrix;
	hlToVRScaleMatrix[0] = GetHLToVR().x;
	hlToVRScaleMatrix[5] = GetHLToVR().y;
	hlToVRScaleMatrix[10] = GetHLToVR().z;

	Matrix4 hlToVRTranslateMatrix;
	hlToVRTranslateMatrix[12] = translate.x;
	hlToVRTranslateMatrix[13] = translate.y;
	hlToVRTranslateMatrix[14] = translate.z;

	Matrix4 switchYAndZTransitionMatrix;
	switchYAndZTransitionMatrix[5] = 0;
	switchYAndZTransitionMatrix[6] = -1;
	switchYAndZTransitionMatrix[9] = 1;
	switchYAndZTransitionMatrix[10] = 0;

	Matrix4 modelViewMatrix = absoluteTrackingMatrix * hlToVRScaleMatrix * switchYAndZTransitionMatrix * hlToVRTranslateMatrix;
	return modelViewMatrix.scale(10);
}

Vector VRHelper::GetOffsetInHLSpaceFromAbsoluteTrackingMatrix(const Matrix4 & absoluteTrackingMatrix)
{
	Vector4 originInVRSpace = absoluteTrackingMatrix * Vector4(0, 0, 0, 1);
	return Vector(originInVRSpace.x * GetVRToHL().x, -originInVRSpace.z * GetVRToHL().z, originInVRSpace.y * GetVRToHL().y);
}

Vector VRHelper::GetPositionInHLSpaceFromAbsoluteTrackingMatrix(const Matrix4 & absoluteTrackingMatrix)
{
	Vector originInRelativeHLSpace = GetOffsetInHLSpaceFromAbsoluteTrackingMatrix(absoluteTrackingMatrix);

	cl_entity_t *localPlayer = SaveGetLocalPlayer();
	if (localPlayer)
	{
		Vector clientGroundPosition = localPlayer->curstate.origin;
		clientGroundPosition.z += localPlayer->curstate.mins.z;
		return clientGroundPosition + originInRelativeHLSpace;
	}
	else
	{
		return originInRelativeHLSpace;
	}
}

void VRHelper::PollEvents(bool isInGame, bool isInMenu)
{
	UpdateVRHLConversionVectors();
	UpdateWorldRotation();
	vr::VREvent_t vrEvent;
	while (vrSystem != nullptr && vrSystem->PollNextEvent(&vrEvent, sizeof(vr::VREvent_t)))
	{
		switch (vrEvent.eventType)
		{
		case vr::EVREventType::VREvent_Quit:
		case vr::EVREventType::VREvent_ProcessQuit:
		case vr::EVREventType::VREvent_QuitAborted_UserPrompt:
		case vr::EVREventType::VREvent_QuitAcknowledged:
		case vr::EVREventType::VREvent_DriverRequestedQuit:
			Exit();
			return;
		case vr::EVREventType::VREvent_ButtonPress:
		case vr::EVREventType::VREvent_ButtonUnpress:
			if (g_vrInput.IsLegacyInput())
			{
				vr::ETrackedControllerRole controllerRole = vrSystem->GetControllerRoleForTrackedDeviceIndex(vrEvent.trackedDeviceIndex);
				if (controllerRole != vr::ETrackedControllerRole::TrackedControllerRole_Invalid)
				{
					vr::VRControllerState_t controllerState;
					vrSystem->GetControllerState(vrEvent.trackedDeviceIndex, &controllerState, sizeof(controllerState));
					g_vrInput.LegacyHandleButtonPress(vrEvent.data.controller.button, controllerState, controllerRole == vr::ETrackedControllerRole::TrackedControllerRole_LeftHand, vrEvent.eventType == vr::EVREventType::VREvent_ButtonPress);
				}
			}
			break;
		case vr::EVREventType::VREvent_ButtonTouch:
		case vr::EVREventType::VREvent_ButtonUntouch:
			if (g_vrInput.IsLegacyInput())
			{
				vr::ETrackedControllerRole controllerRole = vrSystem->GetControllerRoleForTrackedDeviceIndex(vrEvent.trackedDeviceIndex);
				if (controllerRole != vr::ETrackedControllerRole::TrackedControllerRole_Invalid)
				{
					vr::VRControllerState_t controllerState;
					vrSystem->GetControllerState(vrEvent.trackedDeviceIndex, &controllerState, sizeof(controllerState));
					g_vrInput.LegacyHandleButtonTouch(vrEvent.data.controller.button, controllerState, controllerRole == vr::ETrackedControllerRole::TrackedControllerRole_LeftHand, vrEvent.eventType == vr::EVREventType::VREvent_ButtonPress);
				}
			}
			break;
		default:
			break;
		}
	}
	if (!g_vrInput.IsLegacyInput())
	{
		g_vrInput.HandleInput();
	}
}

bool VRHelper::UpdatePositions()
{
	UpdateVRHLConversionVectors();
	UpdateWorldRotation();

	if (vrSystem != nullptr && vrCompositor != nullptr)
	{
		vrCompositor->SetTrackingSpace(isVRRoomScale ? vr::TrackingUniverseStanding : vr::TrackingUniverseSeated);
		vrCompositor->WaitGetPoses(positions.m_rTrackedDevicePose, vr::k_unMaxTrackedDeviceCount, nullptr, 0);

		if (positions.m_rTrackedDevicePose[vr::k_unTrackedDeviceIndex_Hmd].bDeviceIsConnected
			&& positions.m_rTrackedDevicePose[vr::k_unTrackedDeviceIndex_Hmd].bPoseIsValid
			&& positions.m_rTrackedDevicePose[vr::k_unTrackedDeviceIndex_Hmd].eTrackingResult == vr::TrackingResult_Running_OK)
		{
			UpdateHMD();
			UpdateControllers();
			SendPositionUpdateToServer();
			return true;
		}
	}

	return false;
}

void VRHelper::PrepareVRScene(vr::EVREye eEye)
{
	glBindFramebuffer(GL_FRAMEBUFFER, eEye == vr::EVREye::Eye_Left ? vrGLLeftEyeFrameBuffer : vrGLRightEyeFrameBuffer);

	glViewport(0, 0, vrRenderWidth, vrRenderHeight);

	glClearColor(0, 0, 0, 0);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	glMatrixMode(GL_PROJECTION);
	glPushMatrix();
	glLoadIdentity();
	glLoadMatrixf(eEye == vr::EVREye::Eye_Left ? positions.m_mat4LeftProjection.get() : positions.m_mat4RightProjection.get());

	glMatrixMode(GL_MODELVIEW);
	glPushMatrix();
	glLoadIdentity();
	glLoadMatrixf(eEye == vr::EVREye::Eye_Left ? positions.m_mat4LeftModelView.get() : positions.m_mat4RightModelView.get());

	glDisable(GL_CULL_FACE);
	glCullFace(GL_NONE);

	hlvrLockGLMatrices();
}

void VRHelper::FinishVRScene(float width, float height)
{
	hlvrUnlockGLMatrices();

	glMatrixMode(GL_MODELVIEW);
	glPopMatrix();

	glMatrixMode(GL_PROJECTION);
	glPopMatrix();

	glBindFramebuffer(GL_FRAMEBUFFER, 0);

	glViewport(0, 0, width, height);
}

void VRHelper::SubmitImage(vr::EVREye eEye, unsigned int texture)
{
	vr::Texture_t vrTexture;
	vrTexture.eType = vr::ETextureType::TextureType_OpenGL;
	vrTexture.eColorSpace = vr::EColorSpace::ColorSpace_Auto;
	vrTexture.handle = reinterpret_cast<void*>(texture);

	vrCompositor->Submit(eEye, &vrTexture);
}

void VRHelper::SubmitImages()
{
	SubmitImage(vr::EVREye::Eye_Left, vrGLLeftEyeTexture);
	SubmitImage(vr::EVREye::Eye_Right, vrGLRightEyeTexture);
	vrCompositor->PostPresentHandoff();
}

unsigned int VRHelper::GetVRTexture(vr::EVREye eEye)
{
	return eEye == vr::EVREye::Eye_Left ? vrGLLeftEyeTexture : vrGLRightEyeTexture;
}

unsigned int VRHelper::GetVRGLMenuTexture()
{
	int viewport[4];
	glGetIntegerv(GL_VIEWPORT, viewport);
	if (m_vrGLMenuTextureWidth != viewport[2] || m_vrGLMenuTextureHeight != viewport[3])
	{
		m_vrGLMenuTextureWidth = viewport[2];
		m_vrGLMenuTextureHeight = viewport[3];
		CreateGLTexture(&vrGLMenuTexture, m_vrGLMenuTextureWidth, m_vrGLMenuTextureHeight);
	}
	return vrGLMenuTexture;
}

void VRHelper::GetViewAngles(vr::EVREye eEye, float * angles)
{
	if (eEye == vr::EVREye::Eye_Left)
	{
		GetHLViewAnglesFromVRMatrix(positions.m_mat4LeftModelView).CopyToArray(angles);
	}
	else
	{
		GetHLViewAnglesFromVRMatrix(positions.m_mat4RightModelView).CopyToArray(angles);
	}
}

void ExtractRotationMatrix(const Matrix4& source, float* target)
{
	const float* sourceTranspose = const_cast<Matrix4&>(source).getTranspose();
	for (int i = 0; i < 16; i++)
	{
		target[i] = sourceTranspose[i];
	}
	target[3] = target[7] = target[11] = target[12] = target[13] = target[14] = 0.f;
	target[15] = 1.f;
}

void VRHelper::MatrixVectors(const Matrix4& source, Vector& forward, Vector& right, Vector& up)
{
	Vector4 forwardInVRSpace = source * Vector4(0, 0, -1, 0);
	Vector4 rightInVRSpace = source * Vector4(1, 0, 0, 0);
	Vector4 upInVRSpace = source * Vector4(0, 1, 0, 0);

	forward = Vector{ forwardInVRSpace.x * GetVRToHL().x, -forwardInVRSpace.z * GetVRToHL().z, forwardInVRSpace.y * GetVRToHL().y };
	right = Vector{ rightInVRSpace.x * GetVRToHL().x, -rightInVRSpace.z * GetVRToHL().z, rightInVRSpace.y * GetVRToHL().y };
	up = Vector{ upInVRSpace.x * GetVRToHL().x, -upInVRSpace.z * GetVRToHL().z, upInVRSpace.y * GetVRToHL().y };

	forward.InlineNormalize();
	right.InlineNormalize();
	up.InlineNormalize();
}

void VRHelper::GetViewMatrix(vr::EVREye eEye, float* matrix)
{
	if (eEye == vr::EVREye::Eye_Left)
	{
		ExtractRotationMatrix(positions.m_mat4LeftModelView, matrix);
	}
	else
	{
		ExtractRotationMatrix(positions.m_mat4RightModelView, matrix);
	}
}

void VRHelper::GetViewVectors(Vector& forward, Vector& right, Vector& up)
{
	MatrixVectors(GetAbsoluteHMDTransform(), forward, right, up);
}

Matrix4 VRHelper::GetAbsoluteHMDTransform()
{
	auto vrTransform = positions.m_rTrackedDevicePose[vr::k_unTrackedDeviceIndex_Hmd].mDeviceToAbsoluteTracking;
	auto hlTransform = ConvertSteamVRMatrixToMatrix4(vrTransform);

	if (g_vrInput.IsDucking())
	{
		float duckHeightInVR = (DUCK_SIZE - 1.f) * GetHLToVR().y * hlTransform.get()[15];
		float hmdHeight = hlTransform.get()[13];
		const_cast<float*>(hlTransform.get())[13] = (std::min)(hmdHeight, duckHeightInVR);
		m_hmdDuckHeightDelta = hmdHeight - hlTransform.get()[13];
	}

	if (CVAR_GET_FLOAT("vr_playerturn_enabled") == 0.f || m_currentYaw == 0.f)
		return hlTransform;

	if (m_prevYaw != m_currentYaw)
	{
		Matrix4 hlPrevYawTransform = Matrix4{}.rotateY(m_prevYaw) * hlTransform;
		Matrix4 hlCurrentYawTransform = Matrix4{}.rotateY(m_currentYaw) * hlTransform;

		Vector3 previousOffset{ hlPrevYawTransform.get()[12], hlPrevYawTransform.get()[13], hlPrevYawTransform.get()[14] };
		Vector3 newOffset{ hlCurrentYawTransform.get()[12], hlCurrentYawTransform.get()[13], hlCurrentYawTransform.get()[14] };

		Vector3 yawOffsetDelta = newOffset - previousOffset;

		m_currentYawOffsetDelta = Vector{
			yawOffsetDelta.x * GetVRToHL().x,
			-yawOffsetDelta.z * GetVRToHL().z,
			yawOffsetDelta.y * GetVRToHL().y
		};
	}
	else
	{
		m_currentYawOffsetDelta = Vector{};
	}

	return Matrix4{}.rotateY(m_currentYaw) * hlTransform;
}

Matrix4 VRHelper::GetAbsoluteTransform(const vr::HmdMatrix34_t& vrTransform)
{
	auto hlTransform = ConvertSteamVRMatrixToMatrix4(vrTransform);

	if (g_vrInput.IsDucking())
	{
		float controllerHeight = hlTransform.get()[13];
		const_cast<float*>(hlTransform.get())[13] = controllerHeight - m_hmdDuckHeightDelta;
	}

	if (CVAR_GET_FLOAT("vr_playerturn_enabled") == 0.f || m_currentYaw == 0.f)
		return hlTransform;

	auto vrRawHMDTransform = positions.m_rTrackedDevicePose[vr::k_unTrackedDeviceIndex_Hmd].mDeviceToAbsoluteTracking;
	auto hlRawHMDTransform = ConvertSteamVRMatrixToMatrix4(vrRawHMDTransform);
	auto hlCurrentYawHMDTransform = GetAbsoluteHMDTransform();

	Vector3 rawHMDPos{ hlRawHMDTransform.get()[12], hlRawHMDTransform.get()[13], hlRawHMDTransform.get()[14] };
	Vector3 currentYawHMDPos{ hlCurrentYawHMDTransform.get()[12], hlCurrentYawHMDTransform.get()[13], hlCurrentYawHMDTransform.get()[14] };

	Vector4 controllerPos{ hlTransform.get()[12], hlTransform.get()[13], hlTransform.get()[14], 1.f };
	controllerPos.x -= rawHMDPos.x;
	controllerPos.z -= rawHMDPos.z;
	Vector4 rotatedControllerPos = Matrix4{}.rotateY(m_currentYaw) * controllerPos;
	rotatedControllerPos.x += currentYawHMDPos.x;
	rotatedControllerPos.y = controllerPos.y;
	rotatedControllerPos.z += currentYawHMDPos.z;

	Matrix4 hlRotatedTransform = Matrix4{}.rotateY(m_currentYaw) * hlTransform;

	return Matrix4{
		hlRotatedTransform.get()[0],
		hlRotatedTransform.get()[1],
		hlRotatedTransform.get()[2],
		hlRotatedTransform.get()[3],
		hlRotatedTransform.get()[4],
		hlRotatedTransform.get()[5],
		hlRotatedTransform.get()[6],
		hlRotatedTransform.get()[7],
		hlRotatedTransform.get()[8],
		hlRotatedTransform.get()[9],
		hlRotatedTransform.get()[10],
		hlRotatedTransform.get()[11],
		rotatedControllerPos.x,
		rotatedControllerPos.y,
		rotatedControllerPos.z,
		hlRotatedTransform.get()[15]
	};
}

Matrix4 VRHelper::GetAbsoluteControllerTransform(vr::TrackedDeviceIndex_t controllerIndex)
{
	auto vrTransform = positions.m_rTrackedDevicePose[controllerIndex].mDeviceToAbsoluteTracking;
	return GetAbsoluteTransform(vrTransform);
}

void VRHelper::GetViewOrg(float * origin)
{
	GetPositionInHLSpaceFromAbsoluteTrackingMatrix(GetAbsoluteHMDTransform()).CopyToArray(origin);
}

bool VRHelper::UpdateController(
	vr::TrackedDeviceIndex_t controllerIndex,
	Matrix4& controllerMatrix,
	Vector& controllerOffset, Vector& controllerPosition, Vector& controllerAngles, Vector& controllerVelocity,
	Vector &controllerForward, Vector &controllerRight, Vector &controllerUp)
{
	if (controllerIndex > 0 && controllerIndex != vr::k_unTrackedDeviceIndexInvalid && positions.m_rTrackedDevicePose[controllerIndex].bDeviceIsConnected && positions.m_rTrackedDevicePose[controllerIndex].bPoseIsValid)
	{
		cl_entity_t *localPlayer = SaveGetLocalPlayer();

		Vector velocityInVRSpace = Vector(positions.m_rTrackedDevicePose[controllerIndex].vVelocity.v);
		if (CVAR_GET_FLOAT("vr_playerturn_enabled") != 0.f)
		{
			Vector3 rotatedVelocity = Matrix4{}.rotateY(m_currentYaw) * Vector3(velocityInVRSpace.x, velocityInVRSpace.y, velocityInVRSpace.z);
			velocityInVRSpace = Vector(rotatedVelocity.x, rotatedVelocity.y, rotatedVelocity.z);
		}

		controllerMatrix = GetAbsoluteControllerTransform(controllerIndex);
		MatrixVectors(controllerMatrix, controllerForward, controllerRight, controllerUp);
		controllerOffset = GetOffsetInHLSpaceFromAbsoluteTrackingMatrix(controllerMatrix);
		if (localPlayer) controllerOffset.z += localPlayer->curstate.mins.z;
		controllerPosition = GetPositionInHLSpaceFromAbsoluteTrackingMatrix(controllerMatrix);
		controllerAngles = GetHLAnglesFromVRMatrix(controllerMatrix);
		controllerVelocity = Vector{ velocityInVRSpace.x * GetVRToHL().x, -velocityInVRSpace.z * GetVRToHL().z, velocityInVRSpace.y * GetVRToHL().y };

		return true;
	}
	return false;
}

void VRHelper::UpdateHMD()
{
	Matrix4 m_mat4HMDPose = GetAbsoluteHMDTransform().invert();

	positions.m_mat4LeftProjection = GetHMDMatrixProjectionEye(vr::Eye_Left);
	positions.m_mat4RightProjection = GetHMDMatrixProjectionEye(vr::Eye_Right);

	Vector clientGroundPosition;
	cl_entity_t *localPlayer = SaveGetLocalPlayer();
	if (localPlayer)
	{
		clientGroundPosition = localPlayer->curstate.origin;
		clientGroundPosition.z += localPlayer->curstate.mins.z;
	}
	positions.m_mat4LeftModelView = GetHMDMatrixPoseEye(vr::Eye_Left) * GetModelViewMatrixFromAbsoluteTrackingMatrix(m_mat4HMDPose, -clientGroundPosition);
	positions.m_mat4RightModelView = GetHMDMatrixPoseEye(vr::Eye_Right) * GetModelViewMatrixFromAbsoluteTrackingMatrix(m_mat4HMDPose, -clientGroundPosition);
}

void VRHelper::UpdateControllers()
{
	bool leftHandMode = CVAR_GET_FLOAT("vr_lefthand_mode") != 0.f;

	vr::TrackedDeviceIndex_t leftControllerIndex = vrSystem->GetTrackedDeviceIndexForControllerRole(vr::ETrackedControllerRole::TrackedControllerRole_LeftHand);
	m_fLeftControllerValid = UpdateController(
		leftControllerIndex,
		m_leftControllerMatrix,
		m_leftControllerOffset, m_leftControllerPosition, m_leftControllerAngles, m_leftControllerVelocity,
		m_leftControllerForward, m_leftControllerRight, m_leftControllerUp);

	vr::TrackedDeviceIndex_t rightControllerIndex = vrSystem->GetTrackedDeviceIndexForControllerRole(vr::ETrackedControllerRole::TrackedControllerRole_RightHand);
	m_fRightControllerValid = UpdateController(
		rightControllerIndex,
		m_rightControllerMatrix,
		m_rightControllerOffset, m_rightControllerPosition, m_rightControllerAngles, m_rightControllerVelocity,
		m_rightControllerForward, m_rightControllerRight, m_rightControllerUp);

	bool areBothControllersValid = m_fLeftControllerValid && m_fRightControllerValid;

	bool useLeftControllerForViewEntity = (areBothControllersValid && leftHandMode) || (!areBothControllersValid && m_fLeftControllerValid);

	if (useLeftControllerForViewEntity)
	{
		m_handAnglesValid = m_fRightControllerValid;
		m_handAngles = m_rightControllerAngles;
		UpdateViewEnt(m_fLeftControllerValid, m_leftControllerPosition, m_leftControllerAngles, m_leftControllerVelocity, true);
	}
	else
	{
		m_handAnglesValid = m_fLeftControllerValid;
		m_handAngles = m_leftControllerAngles;
		UpdateViewEnt(m_fRightControllerValid, m_rightControllerPosition, m_rightControllerAngles, m_rightControllerVelocity, false);
	}
}

void VRHelper::SendPositionUpdateToServer()
{
	cl_entity_t *localPlayer = SaveGetLocalPlayer();
	Vector hmdOffset = GetOffsetInHLSpaceFromAbsoluteTrackingMatrix(GetAbsoluteHMDTransform());
	if (localPlayer) hmdOffset.z += localPlayer->curstate.mins.z;

	char cmdHMD[MAX_COMMAND_SIZE] = { 0 };
	sprintf_s(cmdHMD, "vrupd_hmd %i %.2f %.2f %.2f %.2f %.2f %.2f %.2f %.2f",
		m_vrUpdateTimestamp,
		hmdOffset.x, hmdOffset.y, hmdOffset.z,
		m_currentYawOffsetDelta.x, m_currentYawOffsetDelta.y, m_currentYawOffsetDelta.z,
		m_prevYaw, m_currentYaw	// for save/restore
	);
	m_currentYawOffsetDelta = Vector{}; // reset after sending

	bool leftHandMode = CVAR_GET_FLOAT("vr_lefthand_mode") != 0.f;
	bool leftDragOn = g_vrInput.IsDragOn(vr::TrackedControllerRole_LeftHand);
	bool rightDragOn = g_vrInput.IsDragOn(vr::TrackedControllerRole_RightHand);

	VRControllerID leftControllerID;
	VRControllerID rightControllerID;

	// either both valid or both invalid - chose controller id based on left hand mode
	if (m_fLeftControllerValid == m_fRightControllerValid)
	{
		leftControllerID = leftHandMode ? VRControllerID::WEAPON : VRControllerID::HAND;
		rightControllerID = leftHandMode ? VRControllerID::HAND : VRControllerID::WEAPON;
	}
	else // only one is valid, chose controller id based on the valid one (the valid one is the weapon)
	{
		leftControllerID = m_fLeftControllerValid ? VRControllerID::WEAPON : VRControllerID::HAND;
		rightControllerID = m_fRightControllerValid ? VRControllerID::WEAPON : VRControllerID::HAND;
	}

	char cmdLeftController[MAX_COMMAND_SIZE] = { 0 };
	sprintf_s(cmdLeftController, "vrupdctrl %i %i %i %i %.2f %.2f %.2f %.2f %.2f %.2f %.2f %.2f %.2f %i",
		m_vrUpdateTimestamp,
		m_fLeftControllerValid ? 1 : 0,
		leftControllerID,
		1/*isMirrored*/,
		m_leftControllerOffset.x, m_leftControllerOffset.y, m_leftControllerOffset.z,
		m_leftControllerAngles.x, m_leftControllerAngles.y, m_leftControllerAngles.z,
		m_leftControllerVelocity.x, m_leftControllerVelocity.y, m_leftControllerVelocity.z,
		leftDragOn ? 1 : 0
	);

	char cmdRightController[MAX_COMMAND_SIZE] = { 0 };
	sprintf_s(cmdRightController, "vrupdctrl %i %i %i %i %.2f %.2f %.2f %.2f %.2f %.2f %.2f %.2f %.2f %i",
		m_vrUpdateTimestamp,
		m_fRightControllerValid ? 1 : 0,
		rightControllerID,
		0/*isMirrored*/,
		m_rightControllerOffset.x, m_rightControllerOffset.y, m_rightControllerOffset.z,
		m_rightControllerAngles.x, m_rightControllerAngles.y, m_rightControllerAngles.z,
		m_rightControllerVelocity.x, m_rightControllerVelocity.y, m_rightControllerVelocity.z,
		rightDragOn ? 1 : 0
	);

	gEngfuncs.pfnClientCmd(cmdHMD);
	gEngfuncs.pfnClientCmd(cmdLeftController);
	gEngfuncs.pfnClientCmd(cmdRightController);

	m_vrUpdateTimestamp++;
}

void VRHelper::UpdateViewEnt(bool isControllerValid, const Vector& controllerPosition, const Vector& controllerAngles, const Vector& controllerVelocity, bool isMirrored)
{
	cl_entity_t *viewent = gEngfuncs.GetViewModel();
	if (!viewent)
	{
		mIsViewEntMirrored = false;
		return;
	}

	mIsViewEntMirrored = isMirrored;

	if (isControllerValid)
	{
		VectorCopy(controllerPosition, viewent->origin);
		VectorCopy(controllerPosition, viewent->curstate.origin);
		VectorCopy(controllerPosition, viewent->latched.prevorigin);
		VectorCopy(controllerAngles, viewent->angles);
		VectorCopy(controllerAngles, viewent->curstate.angles);
		VectorCopy(controllerAngles, viewent->latched.prevangles);
		viewent->curstate.velocity = controllerVelocity;
	}
	else
	{
		cl_entity_t *localPlayer = SaveGetLocalPlayer();
		if (localPlayer)
		{
			VectorCopy(localPlayer->curstate.origin, viewent->origin);
			VectorCopy(localPlayer->curstate.origin, viewent->curstate.origin);
			VectorCopy(localPlayer->curstate.origin, viewent->latched.prevorigin);
			VectorCopy(GetLocalPlayerAngles(), viewent->angles);
			VectorCopy(GetLocalPlayerAngles(), viewent->curstate.angles);
			VectorCopy(GetLocalPlayerAngles(), viewent->latched.prevangles);
			viewent->curstate.velocity = localPlayer->curstate.velocity;
		}
	 }
}

void VRHelper::SetPose(VRPoseType poseType, const vr::TrackedDevicePose_t& pose, vr::ETrackedControllerRole role)
{
	if (!pose.bPoseIsValid || !pose.bDeviceIsConnected)
	{
		ClearPose(poseType);
		return;
	}

	if (poseType == VRPoseType::FLASHLIGHT)
	{
		cl_entity_t *localPlayer = SaveGetLocalPlayer();
		Matrix4 flashlightAbsoluteTrackingMatrix = GetAbsoluteTransform(pose.mDeviceToAbsoluteTracking);
		Vector flashlightOffset = GetOffsetInHLSpaceFromAbsoluteTrackingMatrix(flashlightAbsoluteTrackingMatrix);
		if (localPlayer) flashlightOffset.z += localPlayer->curstate.mins.z;
		Vector flashlightAngles = GetHLAnglesFromVRMatrix(flashlightAbsoluteTrackingMatrix);
		char cmdFlashlight[MAX_COMMAND_SIZE] = { 0 };
		sprintf_s(cmdFlashlight, "vr_flashlight 1 %.2f %.2f %.2f %.2f %.2f %.2f",
			flashlightOffset.x, flashlightOffset.y, flashlightOffset.z,
			flashlightAngles.x, flashlightAngles.y, flashlightAngles.z
		);
		gEngfuncs.pfnClientCmd(cmdFlashlight);
	}
	else if (poseType == VRPoseType::TELEPORTER)
	{
		cl_entity_t *localPlayer = SaveGetLocalPlayer();
		Matrix4 teleporterAbsoluteTrackingMatrix = GetAbsoluteTransform(pose.mDeviceToAbsoluteTracking);
		Vector teleporterOffset = GetOffsetInHLSpaceFromAbsoluteTrackingMatrix(teleporterAbsoluteTrackingMatrix);
		if (localPlayer) teleporterOffset.z += localPlayer->curstate.mins.z;
		Vector teleporterAngles = GetHLAnglesFromVRMatrix(teleporterAbsoluteTrackingMatrix);
		char cmdTeleporter[MAX_COMMAND_SIZE] = { 0 };
		sprintf_s(cmdTeleporter, "vr_teleporter 1 %.2f %.2f %.2f %.2f %.2f %.2f",
			teleporterOffset.x, teleporterOffset.y, teleporterOffset.z,
			teleporterAngles.x, teleporterAngles.y, teleporterAngles.z
		);
		gEngfuncs.pfnClientCmd(cmdTeleporter);
	}
	else if (poseType == VRPoseType::MOVEMENT)
	{
		Matrix4 movementAbsoluteTrackingMatrix = GetAbsoluteTransform(pose.mDeviceToAbsoluteTracking);
		m_movementAngles = GetHLAnglesFromVRMatrix(movementAbsoluteTrackingMatrix);
		m_hasMovementAngles = true;
	}
}

void VRHelper::ClearPose(VRPoseType poseType)
{
	if (poseType == VRPoseType::FLASHLIGHT)
	{
		gEngfuncs.pfnClientCmd("vr_flashlight 0");
	}
	else if (poseType == VRPoseType::TELEPORTER)
	{
		char cmdTeleporter[MAX_COMMAND_SIZE] = { 0 };
		sprintf_s(cmdTeleporter, "vr_teleporter %i", int(VRControllerID::HAND));
		gEngfuncs.pfnClientCmd(cmdTeleporter);
	}
	else if (poseType == VRPoseType::MOVEMENT)
	{
		m_hasMovementAngles = false;
	}
}

Vector VRHelper::GetMovementAngles()
{
	if (m_hasMovementAngles)
	{
		return m_movementAngles;
	}
	else if (m_handAnglesValid)
	{
		return m_handAngles;
	}
	else
	{
		return GetHLViewAnglesFromVRMatrix(positions.m_mat4RightModelView);
	}
}

void RenderLine(Vector v1, Vector v2, Vector color)
{
	glBindTexture(GL_TEXTURE_2D, 0);
	glDisable(GL_BLEND);
	glColor4f(color.x, color.y, color.z, 1.0f);
	glBegin(GL_LINES);
	glVertex3f(v1.x, v1.y, v1.z);
	glVertex3f(v2.x, v2.y, v2.z);
	glEnd();
}

void VRHelper::TestRenderControllerPositions()
{
	if (m_fLeftControllerValid)
	{
		RenderLine(m_leftControllerPosition, m_leftControllerPosition + m_leftControllerForward * 16.f, Vector(1, 0, 0));
		RenderLine(m_leftControllerPosition, m_leftControllerPosition - m_leftControllerRight * 16.f, Vector(0, 1, 0));
		RenderLine(m_leftControllerPosition, m_leftControllerPosition + m_leftControllerUp * 16.f, Vector(0, 0, 1));
		RenderLine(m_leftControllerPosition, m_leftControllerPosition + m_leftControllerVelocity, Vector(1, 0, 1));
	}

	if (m_fRightControllerValid)
	{
		RenderLine(m_rightControllerPosition, m_rightControllerPosition + m_rightControllerForward * 16.f, Vector(1, 0, 0));
		RenderLine(m_rightControllerPosition, m_rightControllerPosition + m_rightControllerRight * 16.f, Vector(0, 1, 0));
		RenderLine(m_rightControllerPosition, m_rightControllerPosition + m_rightControllerUp * 16.f, Vector(0, 0, 1));
		RenderLine(m_rightControllerPosition, m_rightControllerPosition + m_rightControllerVelocity, Vector(1, 0, 1));
	}

	Vector org, forward, right, up;
	GetViewOrg(org);
	GetViewVectors(forward, right, up);
	org = org + forward * 16.f;
	RenderLine(org, org + forward * 16.f, Vector(1, 0, 0));
	RenderLine(org, org + right * 16.f, Vector(0, 1, 0));
	RenderLine(org, org + up * 16.f, Vector(0, 0, 1));
}

bool VRHelper::HasValidWeaponController()
{
	// if only one controller is active, it automatically becomes the weapon controller
	return m_fLeftControllerValid || m_fRightControllerValid;
	/*
	if (CVAR_GET_FLOAT("vr_lefthand_mode") != 0.f)
	{
		return m_fLeftControllerValid;
	}
	else
	{
		return m_fRightControllerValid;
	}
	*/
}

bool VRHelper::HasValidHandController()
{
	// if only one controller is active, it automatically becomes the weapon controller,
	// thus the hand controller is only valid if both are active
	return m_fLeftControllerValid && m_fRightControllerValid;
	/*
	if (CVAR_GET_FLOAT("vr_lefthand_mode") != 0.f)
	{
		return m_fRightControllerValid;
	}
	else
	{
		return m_fLeftControllerValid;
	}
	*/
}

vr::IVRSystem* VRHelper::GetVRSystem()
{
	return vrSystem;
}

// VR related functions
extern struct cl_entity_s *GetViewEntity(void);
#define VR_MUZZLE_ATTACHMENT 0
Vector VRHelper::GetGunPosition()
{
	extern int VRGlobalNumAttachmentsForEntity(cl_entity_t* ent);
	if (HasValidWeaponController() && GetViewEntity() && VRGlobalNumAttachmentsForEntity(GetViewEntity()) >= 1)
	{
		return GetViewEntity()->attachment[VR_MUZZLE_ATTACHMENT];
	}
	else
	{
		return GetWeaponPosition();
	}
}

Vector VRHelper::GetAutoaimVector()
{
	extern int VRGlobalNumAttachmentsForEntity(cl_entity_t* ent);
	if (HasValidWeaponController() && GetViewEntity() && VRGlobalNumAttachmentsForEntity(GetViewEntity()) >= 2)
	{
		Vector pos1 = GetViewEntity()->attachment[VR_MUZZLE_ATTACHMENT];
		Vector pos2 = GetViewEntity()->attachment[VR_MUZZLE_ATTACHMENT + 1];
		Vector dir = (pos2 - pos1).Normalize();
		return dir;
	}
	Vector forward;
	Vector angles = GetWeaponAngles();
	angles.x = -angles.x;
	gEngfuncs.pfnAngleVectors(angles, forward, nullptr, nullptr);
	return forward;
}

void VRHelper::GetGunAim(Vector& forward, Vector& right, Vector& up, Vector& angles)
{
	forward = GetAutoaimVector();
	angles = GetWeaponAngles();
	angles.x = -angles.x;
	gEngfuncs.pfnAngleVectors(angles, nullptr, right, up);
}

Vector VRHelper::GetWeaponPosition()
{
	if (HasValidWeaponController())
	{
		if (HasValidHandController())	// if hand is valid decide weapon controller based on left hand mode
		{
			return (CVAR_GET_FLOAT("vr_lefthand_mode") != 0.f) ? m_leftControllerPosition : m_rightControllerPosition;
		}
		else	// if hand is not valid, decide weapon controller based on which controller is valid
		{
			return m_fLeftControllerValid ? m_leftControllerPosition : m_rightControllerPosition;
		}
	}
	else
	{
		Vector pos;
		GetViewOrg(pos);
		return pos;
	}
}
Vector VRHelper::GetWeaponAngles()
{
	if (HasValidWeaponController())
	{
		if (HasValidHandController())	// if hand is valid decide weapon controller based on left hand mode
		{
			return (CVAR_GET_FLOAT("vr_lefthand_mode") != 0.f) ? m_leftControllerAngles : m_rightControllerAngles;
		}
		else	// if hand is not valid, decide weapon controller based on which controller is valid
		{
			return m_fLeftControllerValid ? m_leftControllerAngles : m_rightControllerAngles;
		}
	}
	else
	{
		Vector angles = GetLocalPlayerAngles();
		angles.x = -angles.x;
		return angles;
	}
}
Vector VRHelper::GetWeaponHUDPosition()
{
	extern int VRGlobalNumAttachmentsForEntity(cl_entity_t* ent);
	if (HasValidWeaponController() && GetViewEntity() && VRGlobalNumAttachmentsForEntity(GetViewEntity()) >= 3)
	{
		return GetViewEntity()->attachment[VR_MUZZLE_ATTACHMENT + 2];
	}
	else
	{
		return GetWeaponPosition();
	}
}
Vector VRHelper::GetWeaponHUDUp()
{
	extern int VRGlobalNumAttachmentsForEntity(cl_entity_t* ent);
	if (HasValidWeaponController() && GetViewEntity() && VRGlobalNumAttachmentsForEntity(GetViewEntity()) >= 4)
	{
		Vector pos1 = GetViewEntity()->attachment[VR_MUZZLE_ATTACHMENT + 2];
		Vector pos2 = GetViewEntity()->attachment[VR_MUZZLE_ATTACHMENT + 3];
		Vector dir = (pos2 - pos1).Normalize();
		return dir;
	}
	else
	{
		Vector dummy, up;
		GetWeaponVectors(dummy, dummy, up);
		return up;
	}
}
void VRHelper::GetWeaponHUDMatrix(float* matrix)
{
	if (HasValidWeaponController())
	{
		if (HasValidHandController())	// if hand is valid decide weapon controller based on left hand mode
		{
			ExtractRotationMatrix((CVAR_GET_FLOAT("vr_lefthand_mode") != 0.f) ? m_leftControllerMatrix : m_rightControllerMatrix, matrix);
		}
		else	// if hand is not valid, decide weapon controller based on which controller is valid
		{
			ExtractRotationMatrix((m_fLeftControllerValid) ? m_leftControllerMatrix : m_rightControllerMatrix, matrix);
		}
	}
}
void VRHelper::GetWeaponVectors(Vector& forward, Vector& right, Vector& up)
{
	if (HasValidWeaponController())
	{
		bool useLeftController;
		if (HasValidHandController())	// if hand is valid decide weapon controller based on left hand mode
		{
			useLeftController = CVAR_GET_FLOAT("vr_lefthand_mode") != 0.f;
		}
		else	// if hand is not valid, decide weapon controller based on which controller is valid
		{
			useLeftController = m_fLeftControllerValid;
		}
		if (useLeftController)
		{
			forward = m_leftControllerForward;
			right = m_leftControllerRight;
			up = m_leftControllerUp;
		}
		else
		{
			forward = m_rightControllerForward;
			right = m_rightControllerRight;
			up = m_rightControllerUp;
		}
	}
}


Vector VRHelper::GetHandPosition()
{
	if (HasValidHandController())
	{
		if (CVAR_GET_FLOAT("vr_lefthand_mode") != 0.f)
			return m_rightControllerPosition;
		else
			return m_leftControllerPosition;
	}
	else
	{
		Vector pos;
		GetViewOrg(pos);
		return pos;
	}
}
Vector VRHelper::GetHandAngles()
{
	if (HasValidHandController())
	{
		if (CVAR_GET_FLOAT("vr_lefthand_mode") != 0.f)
			return m_rightControllerAngles;
		else
			return m_leftControllerAngles;
	}
	else
	{
		return GetLocalPlayerAngles();
	}
}
Vector VRHelper::GetHandHUDPosition()
{
	cl_entity_t* pEnt = gHUD.GetHandControllerEntity();
	extern int VRGlobalNumAttachmentsForEntity(cl_entity_t* ent);
	if (HasValidWeaponController() && pEnt && VRGlobalNumAttachmentsForEntity(pEnt) >= 3)
	{
		return pEnt->attachment[VR_MUZZLE_ATTACHMENT + 2];
	}
	else
	{
		return GetHandPosition();
	}
}
Vector VRHelper::GetHandHUDUp()
{
	cl_entity_t* pEnt = gHUD.GetHandControllerEntity();
	extern int VRGlobalNumAttachmentsForEntity(cl_entity_t* ent);
	if (HasValidWeaponController() && pEnt && VRGlobalNumAttachmentsForEntity(pEnt) >= 4)
	{
		Vector pos1 = pEnt->attachment[VR_MUZZLE_ATTACHMENT + 2];
		Vector pos2 = pEnt->attachment[VR_MUZZLE_ATTACHMENT + 3];
		Vector dir = (pos2 - pos1).Normalize();
		return dir;
	}
	else
	{
		Vector dummy, up;
		GetHandVectors(dummy, dummy, up);
		return up;
	}
}
void VRHelper::GetHandHUDMatrix(float* matrix)
{
	if (HasValidHandController())
	{
		ExtractRotationMatrix((CVAR_GET_FLOAT("vr_lefthand_mode") != 0.f) ? m_rightControllerMatrix : m_leftControllerMatrix, matrix);
	}
}
void VRHelper::GetHandVectors(Vector& forward, Vector& right, Vector& up)
{
	if (HasValidHandController())
	{
		if (CVAR_GET_FLOAT("vr_lefthand_mode") != 0.f)
		{
			forward = m_rightControllerForward;
			right = m_rightControllerRight;
			up = m_rightControllerUp;
		}
		else
		{
			forward = m_leftControllerForward;
			right = m_leftControllerRight;
			up = m_leftControllerUp;
		}
	}
}

float VRHelper::GetAnalogFire()
{
	return g_vrInput.analogfire;
}

Vector VRHelper::GetLocalPlayerAngles()
{
	cl_entity_t *localPlayer = SaveGetLocalPlayer();
	if (localPlayer)
	{
		Vector angles = localPlayer->curstate.angles;
		angles.x *= -3.0f;
		return angles;
	}
	else
	{
		Vector viewangles;
		GetViewAngles(vr::Eye_Left, viewangles);
		viewangles.x = -viewangles.x;
		return viewangles;
	}
}

void VRHelper::SetSkybox(const char* name)
{
	if (!vrCompositor)
		return;

	vr::Texture_t skyboxTextures[6];
	for (int i = 0; i < 6; i++)
	{
		skyboxTextures[i].eType = vr::TextureType_OpenGL;
		skyboxTextures[i].eColorSpace = vr::ColorSpace_Auto;
		skyboxTextures[i].handle = reinterpret_cast<void*>(VRTextureHelper::Instance().GetSkyboxTexture(name, i));
	}
	vrCompositor->SetSkyboxOverride(skyboxTextures, 6);
}

void VRHelper::SetSkyboxFromMap(const char* mapName)
{
	SetSkybox(VRTextureHelper::Instance().GetSkyboxNameFromMapName(mapName));
}
