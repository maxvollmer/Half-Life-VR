#pragma once

class VRGUIRenderer
{
public:
	struct VRControllerState
	{
		float x = -1.f;
		float y = -1.f;
		bool isOverGUI = false;
		bool isPressed = false;
	};

	static VRGUIRenderer& Instance();

	void Init();
	void UpdateGUI(bool isInGame);
	void ShutDown();

	void UpdateControllerState(VRControllerState& state);

private:
	void UpdateVRCursor();
	void UpdateGUIElements(bool isInGame);
	void DrawGUI();

	bool isInitialized = false;

	VRControllerState lastState;
	VRControllerState currentState;

	static constexpr float GuiWidth = 2048.f;
	static constexpr float GuiHeight = 2048.f;

	static VRGUIRenderer instance;
};
