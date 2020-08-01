#include "VRSettings.h"

#include <filesystem>
#include <iostream>
#include <fstream>
#include <regex>

#define NOGDI
#define VC_EXTRALEAN
#define WIN32_LEAN_AND_MEAN

#include <Windows.h>

#include "hud.h"
#include "cl_util.h"
#include "VRCommon.h"
#include "VRSpeechListener.h"

VRSettings g_vrSettings;

constexpr const char* VR_DEFAULT_VIEW_DIST_TO_WALLS_AS_STRING = "5";  // for cvar


void VRSettings::Init()
{
	// These are separate, we don't store them and don't make them accessible through HLVRConfig's config menu
	CVAR_CREATE("vr_debug_physics", "0", 0);
	CVAR_CREATE("vr_debug_controllers", "0", 0);
	CVAR_CREATE("vr_noclip", "0", 0);
	CVAR_CREATE("vr_cheat_enable_healing_exploit", "0", 0);

	// These are all stored in hlsettings.cfg and synchronized with HLVRConfig
	RegisterCVAR("vr_357_scale", "1");
	RegisterCVAR("vr_9mmar_scale", "1");
	RegisterCVAR("vr_9mmhandgun_scale", "1");
	RegisterCVAR("vr_crossbow_scale", "1");
	RegisterCVAR("vr_crowbar_scale", "1");
	RegisterCVAR("vr_crowbar_vanilla_attack_enabled", "0");
	RegisterCVAR("vr_disable_func_friction", "1");
	RegisterCVAR("vr_disable_triggerpush", "1");
	RegisterCVAR("vr_egon_scale", "1");
	RegisterCVAR("vr_enable_aim_laser", "0");
	RegisterCVAR("vr_speech_commands_enabled", "0");
	RegisterCVAR("vr_speech_language_id", "1400");
	RegisterCVAR("vr_flashlight_attachment", "0");
	RegisterCVAR("vr_flashlight_toggle", "0");
	RegisterCVAR("vr_movement_attachment", "0");
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
	RegisterCVAR("vr_train_controls", "0");
	RegisterCVAR("vr_train_autostop", "0");
	RegisterCVAR("vr_train_speed_slow", "0.2");
	RegisterCVAR("vr_train_speed_medium", "0.4");
	RegisterCVAR("vr_train_speed_fast", "0.6");
	RegisterCVAR("vr_train_speed_back", "0.2");
	RegisterCVAR("vr_melee_swing_speed", "100");
	RegisterCVAR("vr_move_analogforward_inverted", "0");
	RegisterCVAR("vr_move_analogsideways_inverted", "0");
	RegisterCVAR("vr_move_analogturn_inverted", "0");
	RegisterCVAR("vr_move_analogupdown_inverted", "0");
	RegisterCVAR("vr_move_analog_deadzone", "0.1");
	RegisterCVAR("vr_move_instant_accelerate", "1");
	RegisterCVAR("vr_move_instant_decelerate", "1");
	RegisterCVAR("vr_no_gauss_recoil", "1");
	RegisterCVAR("vr_npcscale", "1");
	RegisterCVAR("vr_playerturn_enabled", "1");
	RegisterCVAR("vr_rl_duck_height", "36");
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
	RegisterCVAR("vr_view_dist_to_walls", VR_DEFAULT_VIEW_DIST_TO_WALLS_AS_STRING);
	RegisterCVAR("vr_weapon_grenade_mode", "0");
	RegisterCVAR("vr_weaponscale", "1");
	RegisterCVAR("vr_world_scale", "1");
	RegisterCVAR("vr_world_z_strech", "1");
	RegisterCVAR("vr_xenjumpthingies_teleporteronly", "0");
	RegisterCVAR("vr_headset_fps", "90");
	RegisterCVAR("vr_autocrouch_enabled", "1");
	RegisterCVAR("vr_tankcontrols", "2");
	RegisterCVAR("vr_tankcontrols_max_distance", "128");
	RegisterCVAR("vr_legacy_tankcontrols_enabled", "0");
	RegisterCVAR("vr_make_levers_nonsolid", "1");
	RegisterCVAR("vr_make_mountedguns_nonsolid", "1");
	RegisterCVAR("vr_tankcontrols_instant_turn", "0");
	RegisterCVAR("vr_smooth_steps", "0");
	RegisterCVAR("vr_headset_offset", "0");
	RegisterCVAR("vr_classic_mode", "0");
	RegisterCVAR("vr_enable_interactive_debris", "1");
	RegisterCVAR("vr_max_interactive_debris", "50");
	RegisterCVAR("vr_drag_onlyhand", "0");
	RegisterCVAR("vr_npc_gunpoint", "1");
	RegisterCVAR("vr_eye_mode", "0");
	RegisterCVAR("vr_breakable_gib_percentage", "100");
	RegisterCVAR("vr_breakable_max_gibs", "50");

	RegisterCVAR("vr_use_fmod", "1");
	RegisterCVAR("vr_fmod_3d_occlusion", "1");
	RegisterCVAR("vr_fmod_wall_occlusion", "40");
	RegisterCVAR("vr_fmod_door_occlusion", "30");
	RegisterCVAR("vr_fmod_water_occlusion", "20");
	RegisterCVAR("vr_fmod_glass_occlusion", "10");

	RegisterCVAR("vr_forwardspeed", "200");		// cl_forwardspeed
	RegisterCVAR("vr_backspeed", "200");		// cl_backspeed
	RegisterCVAR("vr_sidespeed", "200");		// cl_sidespeed
	RegisterCVAR("vr_upspeed", "200");			// cl_upspeed
	RegisterCVAR("vr_yawspeed", "210");			// cl_yawspeed
	RegisterCVAR("vr_walkspeedfactor", "0.3");	// cl_movespeedkey
	RegisterCVAR("vr_togglewalk", "0");

	RegisterCVAR("vr_texturemode", "GL_LINEAR_MIPMAP_LINEAR");
	RegisterCVAR("vr_display_game", "0");

	// Initialize time that settings file was last changed
	std::filesystem::path settingsPath = GetPathFor("/hlvr.cfg");
	if (std::filesystem::exists(settingsPath))
	{
		m_lastSettingsFileChangedTime = std::filesystem::last_write_time(settingsPath).time_since_epoch().count();
	}
	else
	{
		m_lastSettingsFileChangedTime = 0;
	}

	// Create a watch handle, so we can observe if hlvr.cfg has changed
	m_watchVRFolderHandle = FindFirstChangeNotificationW((L"\\\\?\\" + GetPathFor("").wstring()).c_str(), FALSE, FILE_NOTIFY_CHANGE_LAST_WRITE);

	// If possible load existing settings
	UpdateCVARSFromFile();
}

bool VRSettings::WasSettingsFileChanged()
{
	DWORD result = WaitForSingleObjectEx(m_watchVRFolderHandle, 0, FALSE);
	if (result == WAIT_OBJECT_0)
	{
		std::filesystem::path settingsPath = GetPathFor("/hlvr.cfg");
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
		const char* value = CVAR_GET_STRING(cvar.first.data());
		if (cvar.second != value)
		{
			cvar.second = value;
			cvarsChanged = true;
		}
	}
	return cvarsChanged;
}

void SyncCvars(const char* src, const char* dst)
{
	gEngfuncs.Cvar_SetValue(dst, gEngfuncs.pfnGetCvarFloat(src));
}

void VRSettings::CheckCVARsForChanges()
{
	extern void VRClearCvarCache();
	void VRClearCvarCache();

	// make sure these are always properly set
	float vr_headset_fps = gEngfuncs.pfnGetCvarFloat("vr_headset_fps");
	if (vr_headset_fps <= 0.f)
	{
		gEngfuncs.Cvar_SetValue("vr_headset_fps", 90.f);
		vr_headset_fps = 90.f;
	}
	std::string fps_max_cmd = "fps_max " + std::to_string(static_cast<int>(vr_headset_fps));
	gEngfuncs.pfnClientCmd(fps_max_cmd.c_str());
	gEngfuncs.pfnClientCmd("fps_override 1");
	gEngfuncs.pfnClientCmd("gl_vsync 0");
	gEngfuncs.pfnClientCmd("default_fov 180");
	gEngfuncs.pfnClientCmd("cl_mousegrab 0");
	//gEngfuncs.pfnClientCmd("firstperson");

	// reset cheats if cheats are disabled
	if (gEngfuncs.pfnGetCvarFloat("sv_cheats") == 0.f)
	{
		gEngfuncs.Cvar_SetValue("vr_debug_physics", 0.f);
		gEngfuncs.Cvar_SetValue("vr_debug_controllers", 0.f);
		gEngfuncs.Cvar_SetValue("vr_noclip", 0.f);
		gEngfuncs.Cvar_SetValue("vr_cheat_enable_healing_exploit", 0.f);
	}

	SyncCvars("vr_forwardspeed", "cl_forwardspeed");
	SyncCvars("vr_backspeed", "cl_backspeed");
	SyncCvars("vr_sidespeed", "cl_sidespeed");
	SyncCvars("vr_upspeed", "cl_upspeed");
	SyncCvars("vr_yawspeed", "cl_yawspeed");
	SyncCvars("vr_walkspeedfactor", "cl_movespeedkey");

	// reset HD cvars if classic mode is enabled
	if (gEngfuncs.pfnGetCvarFloat("vr_classic_mode") != 0.f)
	{
		gEngfuncs.Cvar_SetValue("vr_hd_textures_enabled", 0.f);
		gEngfuncs.Cvar_SetValue("vr_use_hd_models", 0.f);
		gEngfuncs.pfnClientCmd("gl_texturemode GL_NEAREST");
		gEngfuncs.pfnClientCmd("vr_texturemode GL_NEAREST");
	}
	else
	{
		UpdateTextureMode();
	}

	// only check every 100~200ms
	auto now = std::chrono::time_point_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now()).time_since_epoch().count();
	if (now < m_nextSettingCheckTime)
		return;

	m_nextSettingCheckTime = now + 100 + (rand() % 100);

	if (WasSettingsFileChanged() || m_needsRetry == RetryMode::LOAD)
	{
		UpdateCVARSFromFile();
	}
	else if (WasAnyCVARChanged() || m_needsRetry == RetryMode::WRITE)
	{
		UpdateFileFromCVARS();
	}
}

void SetCVAR(const char* name, const char* value)
{
	gEngfuncs.Cvar_SetValue(const_cast<char*>(name), atof(value));
	gEngfuncs.pfnClientCmd((std::string(name) + " " + value).data());
}

void VRSettings::UpdateCVARSFromFile()
{
	static const std::regex settingslineregex{ "^(.+)=(.+)$" };

	m_needsRetry = RetryMode::NONE;

	try
	{
		std::filesystem::path settingsPath = GetPathFor("/hlvr.cfg");
		if (std::filesystem::exists(settingsPath))
		{
			std::ifstream settingsstream(settingsPath);
			if (settingsstream)
			{
				std::string setting;
				while (std::getline(settingsstream, setting))
				{
					std::smatch match;
					if (std::regex_match(setting, match, settingslineregex) && match.size() == 3)
					{
						auto cvarname = match[1].str();
						auto value = match[2].str();

						// sanitize input: only load settings that we actually care about
						if (m_cvarCache.find(cvarname) != m_cvarCache.end())
						{
							// TODO: sanitize input: how to make sure that value doesn't murder goldsrc?

							SetCVAR(cvarname.data(), value.data());
							m_cvarCache[cvarname] = value;
						}
						else
						{
							// VRSpeechListener sanitizes input itself and filters out stuff that isn't a commandstring
							VRSpeechListener::Instance().RegisterCommandString(cvarname, value);
						}
					}
				}
				m_lastSettingsFileChangedTime = std::filesystem::last_write_time(settingsPath).time_since_epoch().count();
			}
			else
			{
				m_needsRetry = RetryMode::LOAD;
			}
		}
	}
	catch (std::exception& e)
	{
		gEngfuncs.Con_DPrintf("Couldn't load settings from hlvr.cfg, error: %s\n", e.what());
	}
}

void LoadCvarsIntoSettings(const std::unordered_map<std::string, std::string>& settings, const std::unordered_map<std::string, std::string>& cvar_cache)
{
	for (auto [cvarname, value] : settings)
	{
		auto cachedcvar = cvar_cache.find(cvarname);
		if (cachedcvar != cvar_cache.end())
		{
			value = cachedcvar->second;
		}
	}
}

void VRSettings::UpdateFileFromCVARS()
{
	m_needsRetry = RetryMode::NONE;

	try
	{
		std::filesystem::path settingsPath = GetPathFor("/hlvr.cfg");
		if (std::filesystem::exists(settingsPath))
		{
			std::ifstream settingsinstream(settingsPath);
			if (settingsinstream)
			{
					std::ofstream settingsoutstream(settingsPath);
					if (settingsoutstream)
					{
						std::vector<std::string> settingslines;
						for (auto [cvarname, value] : m_cvarCache)
						{
							settingslines.push_back(cvarname + "=" + value);
						}
						VRSpeechListener::Instance().ExportCommandStrings(settingslines);
						std::sort(settingslines.begin(), settingslines.end(), [](const std::string& a, const std::string& b) { return a < b; });

						for (auto& settingsline : settingslines)
						{
							settingsoutstream << settingsline << std::endl;
						}

						settingsoutstream.flush();
						settingsoutstream.close();
						m_lastSettingsFileChangedTime = std::filesystem::last_write_time(settingsPath).time_since_epoch().count();
					}
					else
					{
						m_needsRetry = RetryMode::WRITE;
					}
			}
			else
			{
				m_needsRetry = RetryMode::WRITE;
			}
		}
	}
	catch (std::exception & e)
	{
		gEngfuncs.Con_DPrintf("Couldn't store settings to hlvr.cfg, error: %s\n", e.what());
	}
}

void VRSettings::RegisterCVAR(const char* name, const char* value)
{
	CVAR_CREATE(name, value, FCVAR_ARCHIVE);
	m_cvarCache[name] = value;
}

void VRSettings::InitialUpdateCVARSFromFile()
{
	UpdateCVARSFromFile();
}

void VRSettings::UpdateTextureMode()
{
	std::string vr_texturemode = CVAR_GET_STRING("vr_texturemode");
	std::transform(vr_texturemode.begin(), vr_texturemode.end(), vr_texturemode.begin(), ::toupper);

	std::string gl_texturemode = CVAR_GET_STRING("gl_texturemode");
	std::transform(gl_texturemode.begin(), gl_texturemode.end(), gl_texturemode.begin(), ::toupper);

	std::string newtexturemode;
	if (vr_texturemode != m_vr_texturemode)
	{
		newtexturemode = vr_texturemode;
	}
	else if (gl_texturemode != m_gl_texturemode)
	{
		newtexturemode = gl_texturemode;
	}

	if (!newtexturemode.empty())
	{
		if (newtexturemode == "GL_NEAREST")
		{
			gEngfuncs.pfnClientCmd("gl_texturemode GL_NEAREST");
			gEngfuncs.pfnClientCmd("vr_texturemode GL_NEAREST");
		}
		else if (newtexturemode == "GL_LINEAR")
		{
			gEngfuncs.pfnClientCmd("gl_texturemode GL_LINEAR");
			gEngfuncs.pfnClientCmd("vr_texturemode GL_LINEAR");
		}
		else if (newtexturemode == "GL_NEAREST_MIPMAP_NEAREST")
		{
			gEngfuncs.pfnClientCmd("gl_texturemode GL_NEAREST_MIPMAP_NEAREST");
			gEngfuncs.pfnClientCmd("vr_texturemode GL_NEAREST_MIPMAP_NEAREST");
		}
		else if (newtexturemode == "GL_LINEAR_MIPMAP_NEAREST")
		{
			gEngfuncs.pfnClientCmd("gl_texturemode GL_LINEAR_MIPMAP_NEAREST");
			gEngfuncs.pfnClientCmd("vr_texturemode GL_LINEAR_MIPMAP_NEAREST");
		}
		else if (newtexturemode == "GL_NEAREST_MIPMAP_LINEAR")
		{
			gEngfuncs.pfnClientCmd("gl_texturemode GL_NEAREST_MIPMAP_LINEAR");
			gEngfuncs.pfnClientCmd("vr_texturemode GL_NEAREST_MIPMAP_LINEAR");
		}
		else /*if (newtexturemode == "GL_LINEAR_MIPMAP_LINEAR")*/
		{
			gEngfuncs.pfnClientCmd("gl_texturemode GL_LINEAR_MIPMAP_LINEAR");
			gEngfuncs.pfnClientCmd("vr_texturemode GL_LINEAR_MIPMAP_LINEAR");
		}
		m_vr_texturemode = newtexturemode;
		m_gl_texturemode = newtexturemode;
	}
}
