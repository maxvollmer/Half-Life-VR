#pragma once

class VRGameFunctions
{
public:
	static void SetSkill(int skill);
	static void SetVolume(float volume);
	static void StartNewGame(bool skipTrainRide);
	static void StartHazardCourse();
	static void ToggleMenu();
	static void QuickLoad();
	static void QuickSave();
	static void QuitGame();
	static bool QuickSaveExists();
	static void PrintToConsole(const char* s);
};
