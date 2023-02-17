#pragma once

#include <unordered_map>
#include <string>

#include "openvr/openvr.h"

class VRInputAction
{
public:
	typedef void (*DigitalActionHandler)(const vr::InputDigitalActionData_t&, const std::string&);
	typedef void (*AnalogActionHandler)(const vr::InputAnalogActionData_t&, const std::string&);
	typedef void (*PoseActionHandler)(const vr::InputPoseActionData_t&, const std::string&);
	typedef void (*SkeletalActionHandler)(const vr::VRSkeletalSummaryData_t&, const vr::VRBoneTransform_t*, bool, bool, const std::string&);

	enum class ActionType
	{
		INVALID,
		DIGITAL,
		ANALOG,
		POSE,
		SKELETAL
	};

	VRInputAction();
	VRInputAction(const std::string& id, vr::VRActionHandle_t handle, DigitalActionHandler handler, bool handleWhenNotInGame, bool handAgnostic);
	VRInputAction(const std::string& id, vr::VRActionHandle_t handle, AnalogActionHandler handler, bool handleWhenNotInGame, bool handAgnostic);
	VRInputAction(const std::string& id, vr::VRActionHandle_t handle, PoseActionHandler handler, bool handleWhenNotInGame, bool handAgnostic);
	VRInputAction(const std::string& id, vr::VRActionHandle_t handle, SkeletalActionHandler handler, bool handleWhenNotInGame, bool handAgnostic);

	void HandleInput(bool isInGame);

private:
	void HandleDigitalInput(vr::VRInputValueHandle_t device);
	void HandleAnalogInput(vr::VRInputValueHandle_t device);
	void HandlePoseInput(vr::VRInputValueHandle_t device);
	void HandleSkeletalInput();

	std::string m_id;
	vr::VRActionHandle_t m_handle{ 0 };
	ActionType m_type{ ActionType::INVALID };
	bool m_handleWhenNotInGame{ false };
	bool m_handAgnostic{ false };

	DigitalActionHandler m_digitalActionHandler{ nullptr };
	AnalogActionHandler m_analogActionHandler{ nullptr };
	PoseActionHandler m_poseActionHandler{ nullptr };
	SkeletalActionHandler m_skeletalActionHandler{ nullptr };
};
