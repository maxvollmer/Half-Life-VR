#include "VRSettings.h"

#include "hud.h"
#include "cl_util.h"

VRSettings g_vrSettings;

constexpr const char* VR_DEFAULT_VIEW_DIST_TO_WALLS_AS_STRING = "5";	// for cvar

void VRSettings::Init()
{
	CVAR_CREATE("vr_debug_physics", "0", 0);
	CVAR_CREATE("vr_debug_controllers", "0", 0);
	CVAR_CREATE("vr_noclip", "0", 0);

	CVAR_CREATE("vr_357_scale", "1", FCVAR_ARCHIVE);
	CVAR_CREATE("vr_9mmar_scale", "1", FCVAR_ARCHIVE);
	CVAR_CREATE("vr_9mmhandgun_scale", "1", FCVAR_ARCHIVE);
	CVAR_CREATE("vr_crossbow_scale", "1", FCVAR_ARCHIVE);
	CVAR_CREATE("vr_crowbar_scale", "1", FCVAR_ARCHIVE);
	CVAR_CREATE("vr_crowbar_vanilla_attack_enabled", "0", FCVAR_ARCHIVE);
	CVAR_CREATE("vr_disable_func_friction", "0", FCVAR_ARCHIVE);
	CVAR_CREATE("vr_disable_triggerpush", "0", FCVAR_ARCHIVE);
	CVAR_CREATE("vr_egon_scale", "1", FCVAR_ARCHIVE);
	CVAR_CREATE("vr_enable_aim_laser", "0", FCVAR_ARCHIVE);
	CVAR_CREATE("vr_flashlight_attachment", "0", FCVAR_ARCHIVE);
	CVAR_CREATE("vr_flashlight_toggle", "0", FCVAR_ARCHIVE);
	CVAR_CREATE("vr_force_introtrainride", "1", FCVAR_ARCHIVE);
	CVAR_CREATE("vr_gauss_scale", "1", FCVAR_ARCHIVE);
	CVAR_CREATE("vr_gordon_hand_scale", "1", FCVAR_ARCHIVE);
	CVAR_CREATE("vr_grenade_scale", "1", FCVAR_ARCHIVE);
	CVAR_CREATE("vr_hd_textures_enabled", "0", FCVAR_ARCHIVE);
	CVAR_CREATE("vr_hgun_scale", "1", FCVAR_ARCHIVE);
	CVAR_CREATE("vr_hud_ammo", "1", FCVAR_ARCHIVE);
	CVAR_CREATE("vr_hud_ammo_offset_x", "3", FCVAR_ARCHIVE);
	CVAR_CREATE("vr_hud_ammo_offset_y", "-3", FCVAR_ARCHIVE);
	CVAR_CREATE("vr_hud_damage_indicator", "1", FCVAR_ARCHIVE);
	CVAR_CREATE("vr_hud_flashlight", "1", FCVAR_ARCHIVE);
	CVAR_CREATE("vr_hud_flashlight_offset_x", "-5", FCVAR_ARCHIVE);
	CVAR_CREATE("vr_hud_flashlight_offset_y", "2", FCVAR_ARCHIVE);
	CVAR_CREATE("vr_hud_health", "1", FCVAR_ARCHIVE);
	CVAR_CREATE("vr_hud_health_offset_x", "-4", FCVAR_ARCHIVE);
	CVAR_CREATE("vr_hud_health_offset_y", "-3", FCVAR_ARCHIVE);
	CVAR_CREATE("vr_hud_mode", "2", FCVAR_ARCHIVE);
	CVAR_CREATE("vr_hud_size", "1", FCVAR_ARCHIVE);
	CVAR_CREATE("vr_hud_textscale", "1", FCVAR_ARCHIVE);
	CVAR_CREATE("vr_ladder_immersive_movement_enabled", "0", FCVAR_ARCHIVE);
	CVAR_CREATE("vr_ladder_immersive_movement_enabled", "1", FCVAR_ARCHIVE);
	CVAR_CREATE("vr_ladder_immersive_movement_swinging_enabled", "1", FCVAR_ARCHIVE);
	CVAR_CREATE("vr_ladder_legacy_movement_enabled", "0", FCVAR_ARCHIVE);
	CVAR_CREATE("vr_ladder_legacy_movement_only_updown", "1", FCVAR_ARCHIVE);
	CVAR_CREATE("vr_ladder_legacy_movement_speed", "100", FCVAR_ARCHIVE);
	CVAR_CREATE("vr_lefthand_mode", "0", FCVAR_ARCHIVE);
	CVAR_CREATE("vr_legacy_train_controls_enabled", "0", FCVAR_ARCHIVE);
	CVAR_CREATE("vr_melee_swing_speed", "150", FCVAR_ARCHIVE);
	CVAR_CREATE("vr_move_analogforward_inverted", "0", FCVAR_ARCHIVE);
	CVAR_CREATE("vr_move_analogsideways_inverted", "0", FCVAR_ARCHIVE);
	CVAR_CREATE("vr_move_analogturn_inverted", "0", FCVAR_ARCHIVE);
	CVAR_CREATE("vr_move_analogupdown_inverted", "0", FCVAR_ARCHIVE);
	CVAR_CREATE("vr_move_instant_accelerate", "0", FCVAR_ARCHIVE);
	CVAR_CREATE("vr_move_instant_accelerate", "1", FCVAR_ARCHIVE);
	CVAR_CREATE("vr_move_instant_decelerate", "0", FCVAR_ARCHIVE);
	CVAR_CREATE("vr_move_instant_decelerate", "1", FCVAR_ARCHIVE);
	CVAR_CREATE("vr_no_gauss_recoil", "1", FCVAR_ARCHIVE);
	CVAR_CREATE("vr_npcscale", "1", FCVAR_ARCHIVE);
	CVAR_CREATE("vr_playerturn_enabled", "1", FCVAR_ARCHIVE);
	CVAR_CREATE("vr_rl_duck_height", "100", FCVAR_ARCHIVE);
	CVAR_CREATE("vr_rl_ducking_enabled", "1", FCVAR_ARCHIVE);
	CVAR_CREATE("vr_rotate_with_trains", "1", FCVAR_ARCHIVE);
	CVAR_CREATE("vr_rpg_scale", "1", FCVAR_ARCHIVE);
	CVAR_CREATE("vr_satchel_radio_scale", "1", FCVAR_ARCHIVE);
	CVAR_CREATE("vr_satchel_scale", "1", FCVAR_ARCHIVE);
	CVAR_CREATE("vr_semicheat_spinthingyspeed", "0", FCVAR_ARCHIVE);
	CVAR_CREATE("vr_shotgun_scale", "1", FCVAR_ARCHIVE);
	CVAR_CREATE("vr_squeak_scale", "1", FCVAR_ARCHIVE);
	CVAR_CREATE("vr_teleport_attachment", "0", FCVAR_ARCHIVE);
	CVAR_CREATE("vr_tripmine_scale", "1", FCVAR_ARCHIVE);
	CVAR_CREATE("vr_use_sd_models", "0", FCVAR_ARCHIVE);
	CVAR_CREATE("vr_view_dist_to_walls", (char*)VR_DEFAULT_VIEW_DIST_TO_WALLS_AS_STRING, FCVAR_ARCHIVE);
	CVAR_CREATE("vr_weapon_grenade_mode", "0", FCVAR_ARCHIVE);
	CVAR_CREATE("vr_weaponscale", "1", FCVAR_ARCHIVE);
	CVAR_CREATE("vr_world_scale", "1", FCVAR_ARCHIVE);
	CVAR_CREATE("vr_world_z_strech", "1", FCVAR_ARCHIVE);
	CVAR_CREATE("vr_xenjumpthingies_teleporteronly", "0", FCVAR_ARCHIVE);
}

