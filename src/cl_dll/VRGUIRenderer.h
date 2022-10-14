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

	void Init(int guiWidth, int guiHeight);
	void UpdateGUI(int guiWidth, int guiHeight, bool isInGame);
	void ShutDown();

	void UpdateControllerState(VRControllerState& state);

private:
	void UpdateVRCursor();
	void UpdateGUIElements(bool isInGame);
	void DrawGUI();

	void VerticalSpacer(int height);
	void HorizontalSpacer(int width);

	int m_guiWidth{ 2048 };
	int m_guiHeight{ 2048 };
	float m_guiScaleW{ 1.f };
	float m_guiScaleH{ 1.f };

	bool isInitialized{ false };

	VRControllerState lastState;
	VRControllerState currentState;

	static VRGUIRenderer instance;
};
