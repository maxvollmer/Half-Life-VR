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
	void UpdateGUI(int guiWidth, int guiHeight, bool isInGame);
	void ShutDown();

	void UpdateControllerState(VRControllerState& state);

private:
	void UpdateVRCursor(int guiWidth, int guiHeight);
	void UpdateGUIElements(int guiWidth, int guiHeight, bool isInGame);
	void DrawGUI(int guiWidth, int guiHeight);

	bool isInitialized = false;

	VRControllerState lastState;
	VRControllerState currentState;

	static VRGUIRenderer instance;
};
