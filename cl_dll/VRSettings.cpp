#include "VRSettings.h"

#include "nlohmann/json.hpp"

#include <filesystem>
#include <iostream>
#include <fstream>

#define NOGDI
#define VC_EXTRALEAN
#define WIN32_LEAN_AND_MEAN

#include <Windows.h>

#include "hud.h"
#include "cl_util.h"
#include "VRCommon.h"

VRSettings g_vrSettings;

constexpr const char* VR_DEFAULT_VIEW_DIST_TO_WALLS_AS_STRING = "5";	// for cvar


void VRSettings::Init()
{
	// These are separate, we don't store them and don't make them accessible through HLVRLauncher's config menu
	CVAR_CREATE("vr_debug_physics", "0", 0);
	CVAR_CREATE("vr_debug_controllers", "0", 0);
	CVAR_CREATE("vr_noclip", "0", 0);
	CVAR_CREATE("vr_cheat_enable_healing_exploit", "0", 0);

	// These are all stored in hlsettings.cfg and synchronized with HLVRLauncher
	RegisterCVAR("vr_357_scale", "1");
	RegisterCVAR("vr_9mmar_scale", "1");
	RegisterCVAR("vr_9mmhandgun_scale", "1");
	RegisterCVAR("vr_crossbow_scale", "1");
	RegisterCVAR("vr_crowbar_scale", "1");
	RegisterCVAR("vr_crowbar_vanilla_attack_enabled", "0");
	RegisterCVAR("vr_disable_func_friction", "0");
	RegisterCVAR("vr_disable_triggerpush", "0");
	RegisterCVAR("vr_egon_scale", "1");
	RegisterCVAR("vr_enable_aim_laser", "0");
	RegisterCVAR("vr_speech_commands_enabled", "0");
	RegisterCVAR("vr_flashlight_attachment", "0");
	RegisterCVAR("vr_flashlight_toggle", "0");
	RegisterCVAR("vr_force_introtrainride", "1");
	RegisterCVAR("vr_gauss_scale", "1");
	RegisterCVAR("vr_gordon_hand_scale", "1");
	RegisterCVAR("vr_grenade_scale", "1");
	RegisterCVAR("vr_hd_textures_enabled", "0");
	RegisterCVAR("vr_hgun_scale", "1");
	RegisterCVAR("vr_hud_ammo", "1");
	RegisterCVAR("vr_hud_ammo_offset_x", "3");
	RegisterCVAR("vr_hud_ammo_offset_y", "-3");
	RegisterCVAR("vr_hud_damage_indicator", "1");
	RegisterCVAR("vr_hud_flashlight", "1");
	RegisterCVAR("vr_hud_flashlight_offset_x", "-5");
	RegisterCVAR("vr_hud_flashlight_offset_y", "2");
	RegisterCVAR("vr_hud_health", "1");
	RegisterCVAR("vr_hud_health_offset_x", "-4");
	RegisterCVAR("vr_hud_health_offset_y", "-3");
	RegisterCVAR("vr_hud_mode", "2");
	RegisterCVAR("vr_hud_size", "1");
	RegisterCVAR("vr_hud_textscale", "1");
	RegisterCVAR("vr_ladder_immersive_movement_enabled", "1");
	RegisterCVAR("vr_ladder_immersive_movement_swinging_enabled", "1");
	RegisterCVAR("vr_ladder_legacy_movement_enabled", "0");
	RegisterCVAR("vr_ladder_legacy_movement_only_updown", "1");
	RegisterCVAR("vr_ladder_legacy_movement_speed", "100");
	RegisterCVAR("vr_ledge_pull_mode", "1");
	RegisterCVAR("vr_ledge_pull_speed", "50");
	RegisterCVAR("vr_lefthand_mode", "0");
	RegisterCVAR("vr_legacy_train_controls_enabled", "1");
	RegisterCVAR("vr_melee_swing_speed", "150");
	RegisterCVAR("vr_move_analogforward_inverted", "0");
	RegisterCVAR("vr_move_analogsideways_inverted", "0");
	RegisterCVAR("vr_move_analogturn_inverted", "0");
	RegisterCVAR("vr_move_analogupdown_inverted", "0");
	RegisterCVAR("vr_move_instant_accelerate", "0");
	RegisterCVAR("vr_move_instant_accelerate", "1");
	RegisterCVAR("vr_move_instant_decelerate", "0");
	RegisterCVAR("vr_move_instant_decelerate", "1");
	RegisterCVAR("vr_no_gauss_recoil", "1");
	RegisterCVAR("vr_npcscale", "1");
	RegisterCVAR("vr_playerturn_enabled", "1");
	RegisterCVAR("vr_rl_duck_height", "100");
	RegisterCVAR("vr_rl_ducking_enabled", "1");
	RegisterCVAR("vr_rotate_with_trains", "1");
	RegisterCVAR("vr_rpg_scale", "1");
	RegisterCVAR("vr_satchel_radio_scale", "1");
	RegisterCVAR("vr_satchel_scale", "1");
	RegisterCVAR("vr_semicheat_spinthingyspeed", "50");
	RegisterCVAR("vr_shotgun_scale", "1");
	RegisterCVAR("vr_squeak_scale", "1");
	RegisterCVAR("vr_teleport_attachment", "0");
	RegisterCVAR("vr_tripmine_scale", "1");
	RegisterCVAR("vr_use_hd_models", "0");
	RegisterCVAR("vr_use_animated_weapons", "0");
	RegisterCVAR("vr_view_dist_to_walls", (char*)VR_DEFAULT_VIEW_DIST_TO_WALLS_AS_STRING);
	RegisterCVAR("vr_weapon_grenade_mode", "0");
	RegisterCVAR("vr_weaponscale", "1");
	RegisterCVAR("vr_world_scale", "1");
	RegisterCVAR("vr_world_z_strech", "1");
	RegisterCVAR("vr_xenjumpthingies_teleporteronly", "0");
	RegisterCVAR("vr_multipass_mode", "0");
	RegisterCVAR("vr_min_fingercurl_for_grab", "70");
	RegisterCVAR("vr_autocrouch_enabled", "1");
	RegisterCVAR("vr_tankcontrols", "2");
	RegisterCVAR("vr_tankcontrols_max_distance", "128");
	RegisterCVAR("vr_legacy_tankcontrols_enabled", "0");
	RegisterCVAR("vr_make_levers_nonsolid", "1");
	RegisterCVAR("vr_make_mountedguns_nonsolid", "1");
	RegisterCVAR("vr_tankcontrols_instant_turn", "0");
	RegisterCVAR("vr_smooth_steps", "2");

	RegisterCVAR("vr_speech_commands_follow", "follow-me|come|lets-go");
	RegisterCVAR("vr_speech_commands_wait", "wait|stop|hold");
	RegisterCVAR("vr_speech_commands_hello", "hello|good-morning|hey|hi|morning|greetings");

	// Initialize time that settings file was last changed
	std::filesystem::path settingsPath = GetPathFor("/hlvrsettings.cfg");
	if (std::filesystem::exists(settingsPath))
	{
		m_lastSettingsFileChangedTime = std::filesystem::last_write_time(settingsPath).time_since_epoch().count();
	}
	else
	{
		m_lastSettingsFileChangedTime = 0;
	}

	// Create a watch handle, so we can observe if hlvrsettings.cfg has changed
	m_watchVRFolderHandle = FindFirstChangeNotificationW((L"\\\\?\\" + GetPathFor("").wstring()).c_str(), FALSE, FILE_NOTIFY_CHANGE_LAST_WRITE);

	// If possible load existing settings
	UpdateCVARSFromJson();
}

bool VRSettings::WasSettingsFileChanged()
{
	DWORD result = WaitForSingleObjectEx(m_watchVRFolderHandle, 0, FALSE);
	if (result == WAIT_OBJECT_0)
	{
		std::filesystem::path settingsPath = GetPathFor("/hlvrsettings.cfg");
		if (std::filesystem::exists(settingsPath))
		{
			auto filechangedtime = std::filesystem::last_write_time(settingsPath).time_since_epoch().count();
			if (filechangedtime > m_lastSettingsFileChangedTime)
			{
				m_lastSettingsFileChangedTime = filechangedtime;
				return true;
			}
		}
		FindNextChangeNotification(m_watchVRFolderHandle);
	}
	return false;
}

bool VRSettings::WasAnyCVARChanged()
{
	bool cvarsChanged = false;
	for (auto& cvar : m_cvarCache)
	{
		char* value = CVAR_GET_STRING(cvar.first.data());
		if (cvar.second != value)
		{
			cvar.second = value;
			cvarsChanged = true;
		}
	}
	return cvarsChanged;
}

void VRSettings::CheckCVARsForChanges()
{
	// make sure these are always properly set
	gEngfuncs.pfnClientCmd("fps_max 90");
	gEngfuncs.pfnClientCmd("fps_override 1");
	gEngfuncs.pfnClientCmd("gl_vsync 0");
	gEngfuncs.pfnClientCmd("default_fov 180");
	gEngfuncs.pfnClientCmd("cl_mousegrab 0");
	//gEngfuncs.pfnClientCmd("firstperson");

	// reset cheats if cheats are disabled
	if (CVAR_GET_FLOAT("sv_cheats") == 0.f)
	{
		gEngfuncs.Cvar_SetValue("vr_debug_physics", 0.f);
		gEngfuncs.Cvar_SetValue("vr_debug_controllers", 0.f);
		gEngfuncs.Cvar_SetValue("vr_noclip", 0.f);
		gEngfuncs.Cvar_SetValue("vr_cheat_enable_healing_exploit", 0.f);
	}

	// only check every 100ms
	auto now = std::chrono::time_point_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now()).time_since_epoch().count();
	if (now < m_nextSettingCheckTime)
		return;

	m_nextSettingCheckTime = now + 100;

	if (WasSettingsFileChanged() || m_needsRetry == RetryMode::LOAD)
	{
		UpdateCVARSFromJson();
	}
	else if (WasAnyCVARChanged() || m_needsRetry == RetryMode::WRITE)
	{
		UpdateJsonFromCVARS();
	}
}

void VRSettings::UpdateCVARSFromJson()
{
	m_needsRetry = RetryMode::NONE;

	std::filesystem::path settingsPath = GetPathFor("/hlvrsettings.cfg");
	if (std::filesystem::exists(settingsPath))
	{
		std::ifstream settingsstream(settingsPath);
		if (settingsstream)
		{
			nlohmann::json settings;
			try
			{
				settingsstream >> settings;

				for (auto& settingCategory : settings["ModSettings"])
				{
					for (auto& setting : settingCategory.items())
					{
						const std::string& cvarname = setting.key();
						const std::string& value = setting.value()["Value"];
						SetCVAR(cvarname.data(), value.data());
					}
				}

				m_lastSettingsFileChangedTime = std::filesystem::last_write_time(settingsPath).time_since_epoch().count();
			}
			catch(const nlohmann::detail::exception& e)
			{
				gEngfuncs.Con_DPrintf("Couldn't load settings from hlvrsettings.cfg, error: %i (%s)\n", e.id, e.what());
			}
		}
		else
		{
			m_needsRetry = RetryMode::LOAD;
		}
	}
}

void VRSettings::UpdateJsonFromCVARS()
{
	m_needsRetry = RetryMode::NONE;

	std::filesystem::path settingsPath = GetPathFor("/hlvrsettings.cfg");
	if (std::filesystem::exists(settingsPath))
	{
		std::ifstream settingsinstream(settingsPath);
		if (settingsinstream)
		{
			try
			{
				nlohmann::json settings;
				settingsinstream >> settings;
				settingsinstream.close();

				for (auto& settingCategory : settings["ModSettings"])
				{
					for (auto& setting : settingCategory.items())
					{
						const std::string& cvarname = setting.key();
						auto cachedcvar = m_cvarCache.find(cvarname);
						if (cachedcvar != m_cvarCache.end())
						{
							setting.value()["Value"] = cachedcvar->second;
						}
					}
				}

				std::ofstream settingsoutstream(settingsPath);
				if (settingsoutstream)
				{
					settingsoutstream << settings;
					settingsoutstream.flush();
					settingsoutstream.close();
					m_lastSettingsFileChangedTime = std::filesystem::last_write_time(settingsPath).time_since_epoch().count();
				}
				else
				{
					m_needsRetry = RetryMode::WRITE;
				}
			}
			catch (const nlohmann::detail::exception& e)
			{
				gEngfuncs.Con_DPrintf("Couldn't store settings to hlvrsettings.cfg, error: %i (%s)\n", e.id, e.what());
			}
		}
		else
		{
			m_needsRetry = RetryMode::WRITE;
		}
	}
}

void VRSettings::RegisterCVAR(const char* name, const char* value)
{
	CVAR_CREATE(name, value, FCVAR_ARCHIVE);
	m_cvarCache[name] = value;
}

void VRSettings::SetCVAR(const char* name, const char* value)
{
	gEngfuncs.Cvar_SetValue(const_cast<char*>(name), atof(value));
	gEngfuncs.pfnClientCmd((std::string(name) + " " + value).data());
}

void VRSettings::InitialUpdateCVARSFromJson()
{
	UpdateCVARSFromJson();
}
