#pragma once

class VRGameFunctions
{
public:
	static void SetSkill(int skill);
	static void SetVolume(float volume);
	static void SetGraphicsMode(int graphics);
	static void SetMovement(int movement);
	static void StartNewGame(bool skipTrainRide);
	static void StartHazardCourse();
	static void OpenMenu();
	static void CloseMenu();
	static void QuickLoad();
	static void QuickSave();
	static void QuitGame();
	static bool QuickSaveExists();
	static void PrintToConsole(const char* s);
	static float GetCVar(const char* cvar);
	static void SetCVar(const char* cvar, float value);
};
