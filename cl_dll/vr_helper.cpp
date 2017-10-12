
#include <windows.h>
#include "Matrices.h"
#include "hud.h"
#include "cl_util.h"
#include "r_studioint.h"
#include "ref_params.h"
#include "vr_helper.h"
#include "vr_gl.h"
#include "vr_input.h"


#ifndef MAX_COMMAND_SIZE
#define MAX_COMMAND_SIZE 256
#endif

extern engine_studio_api_t IEngineStudio;

const Vector3 HL_TO_VR(2.3f / 10.f, 2.2f / 10.f, 2.3f / 10.f);
const Vector3 VR_TO_HL(1.f / HL_TO_VR.x, 1.f / HL_TO_VR.y, 1.f / HL_TO_VR.z);
const float FLOOR_OFFSET = 30;

cvar_t *vr_weapontilt;
cvar_t *vr_roomcrouch;


VRHelper::VRHelper()
{

}

VRHelper::~VRHelper()
{

}

void VRHelper::Init()
{
	/*if (!AcceptsDisclaimer())
	{
		Exit();
	}*/
	if (!IEngineStudio.IsHardware())
	{
		Exit(TEXT("Software mode not supported. Please start this game with OpenGL renderer. (Start Half-Life, open the Options menu, select Video, chose OpenGL as Renderer, save, quit Half-Life, then start Half-Life: VR)"));
	}
	else if (!InitAdditionalGLFunctions())
	{
		Exit(TEXT("Failed to load necessary OpenGL functions. Make sure you have a graphics card that can handle VR and up-to-date drivers, and make sure you are running the game in OpenGL mode."));
	}
	else if (!InitGLMatrixOverrideFunctions())
	{
		Exit(TEXT("Failed to load custom opengl32.dll. Make sure you launch this game through HLVRLauncher.exe, not the Steam menu."));
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
			CreateGLTexture(&vrGLMenuTexture, viewport[2], viewport[3]);
			CreateGLTexture(&vrGLHUDTexture, viewport[2], viewport[3]);
			if (vrGLLeftEyeTexture == 0 || vrGLRightEyeTexture == 0 || vrGLMenuTexture == 0 || vrGLHUDTexture == 0)
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

	//Register Helper convars
	vr_weapontilt = gEngfuncs.pfnRegisterVariable("vr_weapontilt", "-25", FCVAR_ARCHIVE);
	vr_roomcrouch = gEngfuncs.pfnRegisterVariable("vr_roomcrouch", "1", FCVAR_ARCHIVE);


	//Register Input convars 
	g_vrInput.vr_control_alwaysforward = gEngfuncs.pfnRegisterVariable("vr_control_alwaysforward", "0", FCVAR_ARCHIVE);
	g_vrInput.vr_control_deadzone = gEngfuncs.pfnRegisterVariable("vr_control_deadzone", "0.5", FCVAR_ARCHIVE);
	g_vrInput.vr_control_teleport = gEngfuncs.pfnRegisterVariable("vr_control_teleport", "0", FCVAR_ARCHIVE);
	g_vrInput.vr_control_hand = gEngfuncs.pfnRegisterVariable("vr_control_hand", "1", FCVAR_ARCHIVE);
}

/*bool VRHelper::AcceptsDisclaimer()
{
	return MessageBox(WindowFromDC(wglGetCurrentDC()), TEXT("This software is provided as is with no warranties whatsoever. This mod uses a custom opengl32.dll that might get you VAC banned. The VR experience might be unpleasant and nauseating. Only continue if you're aware of the risks and know what you are doing.\n\nDo you want to continue?"), TEXT("Disclaimer"), MB_YESNO) == IDYES;
}*/

void VRHelper::Exit(const char* lpErrorMessage)
{
	vrSystem = nullptr;
	vrCompositor = nullptr;
	if (lpErrorMessage != nullptr)
	{
		MessageBox(NULL, lpErrorMessage, TEXT("Error starting Half-Life: VR"), MB_OK);
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

extern void VectorAngles(const float *forward, float *angles);

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
	Vector4 forwardInVRSpace = mat * Vector4(0, 0, -1, 0);
	Vector4 rightInVRSpace = mat * Vector4(1, 0, 0, 0);
	Vector4 upInVRSpace = mat * Vector4(0, 1, 0, 0);

	Vector forward(forwardInVRSpace.x, -forwardInVRSpace.z, forwardInVRSpace.y);
	Vector right(rightInVRSpace.x, -rightInVRSpace.z, rightInVRSpace.y);
	Vector up(upInVRSpace.x, -upInVRSpace.z, upInVRSpace.y);

	forward.Normalize();
	right.Normalize();
	up.Normalize();

	Vector angles;
	GetAnglesFromVectors(forward, right, up, angles);
	angles.x = 360.f - angles.x;
	return angles;
}

Matrix4 GetModelViewMatrixFromAbsoluteTrackingMatrix(Matrix4 &absoluteTrackingMatrix, Vector translate)
{
	Matrix4 hlToVRScaleMatrix;
	hlToVRScaleMatrix[0] = HL_TO_VR.x;
	hlToVRScaleMatrix[5] = HL_TO_VR.y;
	hlToVRScaleMatrix[10] = HL_TO_VR.z;

	Matrix4 hlToVRTranslateMatrix;
	hlToVRTranslateMatrix[12] = translate.x;
	hlToVRTranslateMatrix[13] = translate.y;
	hlToVRTranslateMatrix[14] = translate.z;

	Matrix4 switchYAndZTransitionMatrix;
	switchYAndZTransitionMatrix[5] = 0;
	switchYAndZTransitionMatrix[6] = -1;
	switchYAndZTransitionMatrix[9] = 1;
	switchYAndZTransitionMatrix[10] = 0;

	Matrix4 modelViewMatrix = absoluteTrackingMatrix * hlToVRScaleMatrix *switchYAndZTransitionMatrix * hlToVRTranslateMatrix;
	modelViewMatrix.scale(13);
	return modelViewMatrix;
}

Vector GetOffsetInHLSpaceFromAbsoluteTrackingMatrix(const Matrix4 & absoluteTrackingMatrix)
{
	Vector4 originInVRSpace = absoluteTrackingMatrix * Vector4(0, 0, .10, 1);
	return Vector(originInVRSpace.x * VR_TO_HL.x * 10, -originInVRSpace.z * VR_TO_HL.z * 10, originInVRSpace.y * VR_TO_HL.y * 10);
}

Vector GetPositionInHLSpaceFromAbsoluteTrackingMatrix(const Matrix4 & absoluteTrackingMatrix)
{
	Vector originInRelativeHLSpace = GetOffsetInHLSpaceFromAbsoluteTrackingMatrix(absoluteTrackingMatrix);

	cl_entity_t *localPlayer = gEngfuncs.GetLocalPlayer();
	Vector clientGroundPosition = localPlayer->curstate.origin;
	clientGroundPosition.z += localPlayer->curstate.mins.z;

	return clientGroundPosition + originInRelativeHLSpace;
}

void VRHelper::PollEvents()
{
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
		{
			vr::ETrackedControllerRole controllerRole = vrSystem->GetControllerRoleForTrackedDeviceIndex(vrEvent.trackedDeviceIndex);
			if (controllerRole != vr::ETrackedControllerRole::TrackedControllerRole_Invalid)
			{
				vr::VRControllerState_t controllerState;
				vrSystem->GetControllerState(vrEvent.trackedDeviceIndex, &controllerState, sizeof(controllerState));
				g_vrInput.HandleButtonPress(vrEvent.data.controller.button, controllerState, controllerRole == vr::ETrackedControllerRole::TrackedControllerRole_LeftHand, vrEvent.eventType == vr::EVREventType::VREvent_ButtonPress);
			}
		}
		break;
		case vr::EVREventType::VREvent_ButtonTouch:
		case vr::EVREventType::VREvent_ButtonUntouch:
		default:
			break;
		}
	}

	for (int controllerRole = vr::ETrackedControllerRole::TrackedControllerRole_LeftHand; controllerRole <= vr::ETrackedControllerRole::TrackedControllerRole_RightHand; controllerRole += 1) {
		vr::VRControllerState_t controllerState;
		vrSystem->GetControllerState(vrSystem->GetTrackedDeviceIndexForControllerRole((vr::ETrackedControllerRole)controllerRole), &controllerState, sizeof(controllerState));
		g_vrInput.HandleTrackpad(vr::EVRButtonId::k_EButton_SteamVR_Touchpad, controllerState, controllerRole == vr::ETrackedControllerRole::TrackedControllerRole_LeftHand, false);
	}
}

bool VRHelper::UpdatePositions(struct ref_params_s* pparams)
{
	if (vrSystem != nullptr && vrCompositor != nullptr)
	{
		vrCompositor->GetCurrentSceneFocusProcess();
		vrCompositor->SetTrackingSpace(isVRRoomScale ? vr::TrackingUniverseStanding : vr::TrackingUniverseSeated);
		vrCompositor->WaitGetPoses(positions.m_rTrackedDevicePose, vr::k_unMaxTrackedDeviceCount, nullptr, 0);

		if (positions.m_rTrackedDevicePose[vr::k_unTrackedDeviceIndex_Hmd].bDeviceIsConnected
			&& positions.m_rTrackedDevicePose[vr::k_unTrackedDeviceIndex_Hmd].bPoseIsValid
			&& positions.m_rTrackedDevicePose[vr::k_unTrackedDeviceIndex_Hmd].eTrackingResult == vr::TrackingResult_Running_OK)
		{
			//Matrix4 m_mat4SeatedPose = ConvertSteamVRMatrixToMatrix4(vrSystem->GetSeatedZeroPoseToStandingAbsoluteTrackingPose()).invert();
			Matrix4 m_mat4StandingPose = ConvertSteamVRMatrixToMatrix4(vrSystem->GetRawZeroPoseToStandingAbsoluteTrackingPose()).invert();

			Matrix4 m_mat4HMDPose = ConvertSteamVRMatrixToMatrix4(positions.m_rTrackedDevicePose[vr::k_unTrackedDeviceIndex_Hmd].mDeviceToAbsoluteTracking).invert();

			positions.m_mat4LeftProjection = GetHMDMatrixProjectionEye(vr::Eye_Left);
			positions.m_mat4RightProjection = GetHMDMatrixProjectionEye(vr::Eye_Right);

			cl_entity_t *localPlayer = gEngfuncs.GetLocalPlayer();
			Vector clientGroundPosition = localPlayer->curstate.origin;
			clientGroundPosition.z += localPlayer->curstate.mins.z;
			positions.m_mat4LeftModelView = GetHMDMatrixPoseEye(vr::Eye_Left) * GetModelViewMatrixFromAbsoluteTrackingMatrix(m_mat4HMDPose, -clientGroundPosition);
			positions.m_mat4RightModelView = GetHMDMatrixPoseEye(vr::Eye_Right) * GetModelViewMatrixFromAbsoluteTrackingMatrix(m_mat4HMDPose, -clientGroundPosition);

			UpdateGunPosition(pparams);

			SendPositionUpdateToServer();

			return true;
		}
	}

	return false;
}

void VRHelper::PrepareVRScene(vr::EVREye eEye, struct ref_params_s* pparams)
{
	GetHLViewAnglesFromVRMatrix(positions.m_mat4LeftModelView).CopyToArray(pparams->viewangles);

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

void VRHelper::FinishVRScene(struct ref_params_s* pparams)
{
	hlvrUnlockGLMatrices();

	glMatrixMode(GL_MODELVIEW);
	glPopMatrix();

	glMatrixMode(GL_PROJECTION);
	glPopMatrix();

	glBindFramebuffer(GL_FRAMEBUFFER, 0);

	glViewport(0, 0, pparams->viewport[2], pparams->viewport[3]);
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

void VRHelper::GetViewAngles(float * angles)
{
	GetHLViewAnglesFromVRMatrix(positions.m_mat4LeftModelView).CopyToArray(angles);
}

void VRHelper::GetWalkAngles(float * angles)
{
	walkAngles.CopyToArray(angles);
}

void VRHelper::GetViewOrg(float * org)
{
	GetPositionInHLSpaceFromAbsoluteTrackingMatrix(ConvertSteamVRMatrixToMatrix4(positions.m_rTrackedDevicePose[vr::k_unTrackedDeviceIndex_Hmd].mDeviceToAbsoluteTracking)).CopyToArray(org);
}

void VRHelper::UpdateGunPosition(struct ref_params_s* pparams)
{
	cl_entity_t *viewent = gEngfuncs.GetViewModel();
	if (viewent != nullptr)
	{
		vr::TrackedDeviceIndex_t controllerIndex = vrSystem->GetTrackedDeviceIndexForControllerRole(vr::ETrackedControllerRole::TrackedControllerRole_RightHand);

		if (controllerIndex > 0 && controllerIndex != vr::k_unTrackedDeviceIndexInvalid && positions.m_rTrackedDevicePose[controllerIndex].bDeviceIsConnected && positions.m_rTrackedDevicePose[controllerIndex].bPoseIsValid)
		{
			Matrix4 controllerAbsoluteTrackingMatrix = ConvertSteamVRMatrixToMatrix4(positions.m_rTrackedDevicePose[controllerIndex].mDeviceToAbsoluteTracking);

			Vector4 originInVRSpace = controllerAbsoluteTrackingMatrix * Vector4(-.135, .23, .10, 1);
			Vector originInRelativeHLSpace(originInVRSpace.x * VR_TO_HL.x * 10, -originInVRSpace.z * VR_TO_HL.z * 10, originInVRSpace.y * VR_TO_HL.y * 10);

			cl_entity_t *localPlayer = gEngfuncs.GetLocalPlayer();
			Vector clientGroundPosition = localPlayer->curstate.origin;
			clientGroundPosition.z += localPlayer->curstate.mins.z;
			Vector originInHLSpace = clientGroundPosition + originInRelativeHLSpace;

			VectorCopy(originInHLSpace, viewent->origin);
			VectorCopy(originInHLSpace, viewent->curstate.origin);
			VectorCopy(originInHLSpace, viewent->latched.prevorigin);


			viewent->angles = GetHLAnglesFromVRMatrix(controllerAbsoluteTrackingMatrix);
			viewent->angles.x += vr_weapontilt->value;
			VectorCopy(viewent->angles, viewent->curstate.angles);
			VectorCopy(viewent->angles, viewent->latched.prevangles);


			Vector velocityInVRSpace = Vector(positions.m_rTrackedDevicePose[controllerIndex].vVelocity.v);
			Vector velocityInHLSpace(velocityInVRSpace.x * VR_TO_HL.x * 10, -velocityInVRSpace.z * VR_TO_HL.z * 10, velocityInVRSpace.y * VR_TO_HL.y * 10);
			viewent->curstate.velocity = velocityInHLSpace;
		}
		else
		{
			viewent->model = NULL;
		}
	}
}

void VRHelper::SendPositionUpdateToServer()
{
	cl_entity_t *localPlayer = gEngfuncs.GetLocalPlayer();
	cl_entity_t *viewent = gEngfuncs.GetViewModel();

	Vector hmdOffset = GetOffsetInHLSpaceFromAbsoluteTrackingMatrix(ConvertSteamVRMatrixToMatrix4(positions.m_rTrackedDevicePose[vr::k_unTrackedDeviceIndex_Hmd].mDeviceToAbsoluteTracking));
	hmdOffset.z += localPlayer->curstate.mins.z;

	vr::TrackedDeviceIndex_t leftControllerIndex = vrSystem->GetTrackedDeviceIndexForControllerRole(vr::ETrackedControllerRole::TrackedControllerRole_LeftHand);
	bool isLeftControllerValid = leftControllerIndex > 0 && leftControllerIndex != vr::k_unTrackedDeviceIndexInvalid && positions.m_rTrackedDevicePose[leftControllerIndex].bDeviceIsConnected && positions.m_rTrackedDevicePose[leftControllerIndex].bPoseIsValid;

	vr::TrackedDeviceIndex_t rightControllerIndex = vrSystem->GetTrackedDeviceIndexForControllerRole(vr::ETrackedControllerRole::TrackedControllerRole_RightHand);
	bool isRightControllerValid = leftControllerIndex > 0 && leftControllerIndex != vr::k_unTrackedDeviceIndexInvalid && positions.m_rTrackedDevicePose[leftControllerIndex].bDeviceIsConnected && positions.m_rTrackedDevicePose[leftControllerIndex].bPoseIsValid;

	Vector leftControllerOffset(0, 0, 0);
	Vector leftControllerAngles(0, 0, 0);
	if (isLeftControllerValid)
	{
		Matrix4 leftControllerAbsoluteTrackingMatrix = ConvertSteamVRMatrixToMatrix4(positions.m_rTrackedDevicePose[leftControllerIndex].mDeviceToAbsoluteTracking);
		leftControllerOffset = GetOffsetInHLSpaceFromAbsoluteTrackingMatrix(leftControllerAbsoluteTrackingMatrix);
		leftControllerOffset.z += localPlayer->curstate.mins.z;
		leftControllerAngles = GetHLAnglesFromVRMatrix(leftControllerAbsoluteTrackingMatrix);
		isLeftControllerValid = true;
	}

	Vector weaponOrigin(0, 0, 0);
	Vector weaponOffset(0, 0, 0);
	Vector weaponAngles(0, 0, 0);
	Vector weaponVelocity(0, 0, 0);
	if (isRightControllerValid)
	{
		weaponOrigin = viewent ? viewent->curstate.origin : Vector();
		weaponOffset = weaponOrigin - localPlayer->curstate.origin;
		weaponAngles = viewent ? viewent->curstate.angles : Vector();
		weaponVelocity = viewent ? viewent->curstate.velocity : Vector();
	}

	GetViewAngles(walkAngles);
	walkAngles = g_vrInput.vr_control_hand->value == 1.f ? leftControllerAngles : walkAngles;
	walkAngles = g_vrInput.vr_control_hand->value == 2.f ? weaponAngles : walkAngles;

	char cmd[MAX_COMMAND_SIZE] = { 0 };
	sprintf_s(cmd, "updatevr %.2f %.2f %.2f %.2f %.2f %.2f %.2f %.2f %.2f %.2f %.2f %.2f %.2f %.2f %.2f %.2f %.2f %.2f %i %i %.2f",
		hmdOffset.x, hmdOffset.y, hmdOffset.z,
		leftControllerOffset.x, leftControllerOffset.y, leftControllerOffset.z,
		leftControllerAngles.x, leftControllerAngles.y, leftControllerAngles.z,
		weaponOffset.x, weaponOffset.y, weaponOffset.z,
		weaponAngles.x, weaponAngles.y, weaponAngles.z,
		weaponVelocity.x, weaponVelocity.y, weaponVelocity.z,
		isLeftControllerValid ? 1 : 0,
		isRightControllerValid ? 1 : 0,
		vr_roomcrouch->value
	);
	gEngfuncs.pfnClientCmd(cmd);
}


void RenderLine(Vector v1, Vector v2, Vector color)
{
	glColor4f(color.x, color.y, color.z, 1.0f);
	glBegin(GL_LINES);
	glVertex3f(v1.x, v1.y, v1.z);
	glVertex3f(v2.x, v2.y, v2.z);
	glEnd();
}


void VRHelper::TestRenderControllerPosition(bool leftOrRight)
{
 	vr::TrackedDeviceIndex_t controllerIndex = vrSystem->GetTrackedDeviceIndexForControllerRole(leftOrRight ? vr::ETrackedControllerRole::TrackedControllerRole_LeftHand : vr::ETrackedControllerRole::TrackedControllerRole_LeftHand);

	if (controllerIndex > 0 && controllerIndex != vr::k_unTrackedDeviceIndexInvalid && positions.m_rTrackedDevicePose[controllerIndex].bDeviceIsConnected && positions.m_rTrackedDevicePose[controllerIndex].bPoseIsValid)
	{
		Matrix4 controllerAbsoluteTrackingMatrix = ConvertSteamVRMatrixToMatrix4(positions.m_rTrackedDevicePose[controllerIndex].mDeviceToAbsoluteTracking);

		Vector originInHLSpace = GetPositionInHLSpaceFromAbsoluteTrackingMatrix(controllerAbsoluteTrackingMatrix);
		
		Vector4 forwardInVRSpace = controllerAbsoluteTrackingMatrix * Vector4(0, 0, -1, 0);
		Vector4 rightInVRSpace = controllerAbsoluteTrackingMatrix * Vector4(-1, 0, 0, 0);
		Vector4 upInVRSpace = controllerAbsoluteTrackingMatrix * Vector4(0, 1, 0, 0);

		Vector forward(forwardInVRSpace.x * VR_TO_HL.x * 3, -forwardInVRSpace.z * VR_TO_HL.z * 3, forwardInVRSpace.y * VR_TO_HL.y * 3);
		Vector right(rightInVRSpace.x * VR_TO_HL.x * 3, -rightInVRSpace.z * VR_TO_HL.z * 3, rightInVRSpace.y * VR_TO_HL.y * 3);
		Vector up(upInVRSpace.x * VR_TO_HL.x * 3, -upInVRSpace.z * VR_TO_HL.z * 3, upInVRSpace.y * VR_TO_HL.y * 3);
/*
		if (leftOrRight)
		{
			right = -right; // left
		}
*/	
		RenderLine(originInHLSpace, originInHLSpace + forward, Vector(1, 0, 0));
		RenderLine(originInHLSpace, originInHLSpace + right, Vector(0, 1, 0));
		RenderLine(originInHLSpace, originInHLSpace + up, Vector(0, 0, 1));
	}
}


