using HLVRConfig.Utilities.Settings;
using Microsoft.Collections.Extensions;
using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace HLVRConfig.Utilities
{
    public class HLVRModSettings : ISettingsContainer
    {
        public OrderedDictionary<string, I18N.I18NString> SpeechRecognitionLanguages
        {
            get
            {
                return AudioSettings[CategorySpeechRecognition][SpeechRecognitionLanguage].AllowedValues;
            }
        }

        public static readonly I18N.I18NString CategorySpeechRecognition = new I18N.I18NString("AudioSettings.SpeechRecognition", "Speech recognition");
        public static readonly string SpeechRecognitionLanguage = "vr_speech_language_id";


        public OrderedDictionary<I18N.I18NString, OrderedDictionary<string, Setting>> InputSettings = new OrderedDictionary<I18N.I18NString, OrderedDictionary<string, Setting>>()
        {
            { new I18N.I18NString("InputSettings.AntinauseaFeatures", "Anti-nausea features"), new OrderedDictionary<string, Setting>() {
                { "vr_move_instant_accelerate", Setting.Create( new I18N.I18NString("vr_move_instant_accelerate", "Instant acceleration"), true ) },
                { "vr_move_instant_decelerate", Setting.Create( new I18N.I18NString("vr_move_instant_decelerate", "Instant deceleration"), true ) },
                { "vr_rotate_with_trains", Setting.Create( new I18N.I18NString("vr_rotate_with_trains", "Rotate with trains/elevators"), true ) },
                { "vr_no_gauss_recoil", Setting.Create( new I18N.I18NString("vr_no_gauss_recoil", "Disable gauss recoil"), true ) },
                { "vr_disable_triggerpush", Setting.Create( new I18N.I18NString("vr_disable_triggerpush", "Disable areas that push the player (e.g. strong wind)"), true ) },
                { "vr_disable_func_friction", Setting.Create( new I18N.I18NString("vr_disable_func_friction", "Disable slippery surfaces"), true ) },
                { "vr_xenjumpthingies_teleporteronly", Setting.Create( new I18N.I18NString("vr_xenjumpthingies_teleporteronly", "Disable being pushed up by xen jump thingies (you can use the teleporter on those things)"), true ) },
                { "vr_semicheat_spinthingyspeed", Setting.Create( new I18N.I18NString("vr_semicheat_spinthingyspeed", "Set maximum speed for those fast spinning platforms near the tentacle monster (map c1a4f) (game default: 110, mod default: 50)"), 50 ) },
                { "vr_smooth_steps", Setting.Create( new I18N.I18NString("vr_smooth_steps", "Smooth out steps (may affect performance!)"), 0, new OrderedDictionary<string, I18N.I18NString>(){ {"0", new I18N.I18NString("vr_smooth_steps.Disabled", "Disabled") }, { "1", new I18N.I18NString("vr_smooth_steps.Most", "Smooth out most steps") }, { "2", new I18N.I18NString("vr_smooth_steps.All", "Smooth out all steps") } } ) },
            } },

            { new I18N.I18NString("InputSettings.ControlsAndAccessibility", "Controls & Accessibility"), new OrderedDictionary<string, Setting>() {
                { "vr_lefthand_mode", Setting.Create( new I18N.I18NString("vr_lefthand_mode", "Left hand mode"), false ) },

                { "vr_crowbar_vanilla_attack_enabled", Setting.Create( new I18N.I18NString("vr_crowbar_vanilla_attack_enabled", "Enable crowbar melee attack on 'fire' action."), false ) },

                { "vr_flashlight_toggle", Setting.Create( new I18N.I18NString("vr_flashlight_toggle", "Flashlight should toggle"), false ) },

                { "vr_enable_aim_laser", Setting.Create( new I18N.I18NString("vr_enable_aim_laser", "Enable aim laser pointer for range weapons"), false ) },

                { "vr_make_levers_nonsolid", Setting.Create( new I18N.I18NString("vr_make_levers_nonsolid", "Make levers non-solid (fixes some collision issues)"), true ) },

                { "vr_use_animated_weapons", Setting.Create( new I18N.I18NString("vr_use_animated_weapons", "Use weapon models with animated movement (affects aiming)"), false ) },

                { "vr_melee_swing_speed", Setting.Create( new I18N.I18NString("vr_melee_swing_speed", "Minimum controller speed to trigger melee damage"), 100 ) },

                { "vr_headset_offset", Setting.Create( new I18N.I18NString("vr_headset_offset", "HMD height offset"), 0 ) },

                { "vr_weapon_grenade_mode", Setting.Create( new I18N.I18NString("vr_weapon_grenade_mode", "Grenade throw mode"), 0, new OrderedDictionary<string, I18N.I18NString>(){ {"0", new I18N.I18NString("vr_weapon_grenade_mode.ControllerVelocity", "Use controller velocity") }, { "1", new I18N.I18NString("vr_weapon_grenade_mode.ControllerAimGameVelocity", "Aim with controller, but fly with fixed speed (as in original game)") } } ) },

                { "vr_ledge_pull_mode", Setting.Create( new I18N.I18NString("vr_ledge_pull_mode", "Ledge pulling"), 1, new OrderedDictionary<string, I18N.I18NString>(){ {"0", new I18N.I18NString("vr_ledge_pull_mode.Disabled", "Disabled") }, { "1", new I18N.I18NString("vr_ledge_pull_mode.Move", "Move onto ledge when pulling") }, { "2", new I18N.I18NString("vr_ledge_pull_mode.Teleport", "Teleport onto ledge instantly when pulling") } } ) },
                { "vr_ledge_pull_speed", Setting.Create( new I18N.I18NString("vr_ledge_pull_speed", "Minimum controller speed to trigger ledge pulling"), 50 ) },

                { "vr_teleport_attachment", Setting.Create( new I18N.I18NString("vr_teleport_attachment", "Teleporter attachment"), 0, new OrderedDictionary<string, I18N.I18NString>(){ {"0", new I18N.I18NString("vr_attachment.Hand", "Hand") }, { "1", new I18N.I18NString("vr_attachment.Weapon", "Weapon") }, { "2", new I18N.I18NString("vr_attachment.Head", "Head (HMD)") }, { "3", new I18N.I18NString("vr_attachment.Pose", "SteamVR Input pose") } } ) },
                { "vr_flashlight_attachment", Setting.Create( new I18N.I18NString("vr_flashlight_attachment", "Flashlight attachment"), 0 , new OrderedDictionary<string, I18N.I18NString>(){ {"0", new I18N.I18NString("vr_attachment.Hand", "Hand") }, { "1", new I18N.I18NString("vr_attachment.Weapon", "Weapon") }, { "2", new I18N.I18NString("vr_attachment.Head", "Head (HMD)") }, { "3", new I18N.I18NString("vr_attachment.Pose", "SteamVR Input pose") } }) },
                { "vr_movement_attachment", Setting.Create( new I18N.I18NString("vr_movement_attachment", "Movement attachment"), 0 , new OrderedDictionary<string, I18N.I18NString>(){ {"0", new I18N.I18NString("vr_attachment.Hand", "Hand") }, { "1", new I18N.I18NString("vr_attachment.Weapon", "Weapon") }, { "2", new I18N.I18NString("vr_attachment.Head", "Head (HMD)") }, { "3", new I18N.I18NString("vr_attachment.Pose", "SteamVR Input pose") } }) },
            } },

            { new I18N.I18NString("InputSettings.Movement", "Movement"), new OrderedDictionary<string, Setting>() {
                { "vr_rl_ducking_enabled", Setting.Create( new I18N.I18NString("vr_rl_ducking_enabled", "Enable real-life crouch detection"), true ) },
                { "vr_rl_duck_height", Setting.Create( new I18N.I18NString("vr_rl_duck_height", "Real-life crouch height"), 100 ) },
                { "vr_playerturn_enabled", Setting.Create( new I18N.I18NString("vr_playerturn_enabled", "Enable player turning"), true ) },
                { "vr_autocrouch_enabled", Setting.Create( new I18N.I18NString("vr_autocrouch_enabled", "Enabled automatic crouching"), true ) },
                { "vr_move_analogforward_inverted", Setting.Create( new I18N.I18NString("vr_move_analogforward_inverted", "Invert analog forward input"), false ) },
                { "vr_move_analogsideways_inverted", Setting.Create( new I18N.I18NString("vr_move_analogsideways_inverted", "Invert analog sideways input"), false ) },
                { "vr_move_analogupdown_inverted", Setting.Create( new I18N.I18NString("vr_move_analogupdown_inverted", "Invert analog updown input"), false ) },
                { "vr_move_analogturn_inverted", Setting.Create( new I18N.I18NString("vr_move_analogturn_inverted", "Invert analog turn input"), false ) },
            } },

            { new I18N.I18NString("InputSettings.Ladders", "Ladders"), new OrderedDictionary<string, Setting>() {
                { "vr_ladder_immersive_movement_enabled", Setting.Create( new I18N.I18NString("vr_ladder_immersive_movement_enabled", "Enabled immersive climbing of ladders"), true ) },
                { "vr_ladder_immersive_movement_swinging_enabled", Setting.Create( new I18N.I18NString("vr_ladder_immersive_movement_swinging_enabled", "Enable momentum from immersive ladder climbing"), true ) },
                { "vr_ladder_legacy_movement_enabled", Setting.Create( new I18N.I18NString("vr_ladder_legacy_movement_enabled", "Enable legacy climbing of ladders"), false ) },
                { "vr_ladder_legacy_movement_only_updown", Setting.Create( new I18N.I18NString("vr_ladder_legacy_movement_only_updown", "Restrict movement on ladders to up/down only (legacy movement)"), false ) },
                { "vr_ladder_legacy_movement_speed", Setting.Create( new I18N.I18NString("vr_ladder_legacy_movement_speed", "Climb speed on ladders (legacy movement)"), 100 ) },
            } },

            { new I18N.I18NString("InputSettings.Trains", "Trains"), new OrderedDictionary<string, Setting>() {
                // { "vr_legacy_train_controls_enabled", Setting.Create( new I18N.I18NString("vr_legacy_train_controls_enabled", "Enable control of usable trains with 'LegacyUse' and forward/backward movement."), true ) },
            } },

            { new I18N.I18NString("InputSettings.MountedGuns", "Mounted guns"), new OrderedDictionary<string, Setting>() {
                { "vr_tankcontrols", Setting.Create( new I18N.I18NString("vr_tankcontrols", "Immersive controls"), 2, new OrderedDictionary<string, I18N.I18NString>(){ {"0", new I18N.I18NString("vr_tankcontrols.Disabled", "Disabled") }, { "1", new I18N.I18NString("vr_tankcontrols.EnabledOneHand", "Enabled with one hand") }, { "2", new I18N.I18NString("vr_tankcontrols.EnabledTwoHands", "Enabled with both hands") } } ) },
                { "vr_tankcontrols_max_distance", Setting.Create( new I18N.I18NString("vr_tankcontrols_max_distance", "Maximum controller distance to gun for immersive controls"), 128 ) },
                { "vr_tankcontrols_instant_turn", Setting.Create( new I18N.I18NString("vr_tankcontrols_instant_turn", "Enable instant-turn (if off, guns turn with a predefined speed, lagging behind your hands)"), false ) },
                { "vr_make_mountedguns_nonsolid", Setting.Create( new I18N.I18NString("vr_make_mountedguns_nonsolid", "Make mounted guns non-solid (fixes some collision issues)"), true ) },
                { "vr_legacy_tankcontrols_enabled", Setting.Create( new I18N.I18NString("vr_legacy_tankcontrols_enabled", "Enable control of usable guns with 'LegacyUse'."), false ) },
            } },
        };

        public OrderedDictionary<I18N.I18NString, OrderedDictionary<string, Setting>> GraphicsSettings = new OrderedDictionary<I18N.I18NString, OrderedDictionary<string, Setting>>()
        {
            { new I18N.I18NString("GraphicsSettings.FPSAndPerformance", "FPS & Performance"), new OrderedDictionary<string, Setting>() {
                { "vr_headset_fps", Setting.Create( new I18N.I18NString("vr_headset_fps", "VR FPS (actual fps in your headset, sync these with your SteamVR settings)"), 90 ) },
                { "vr_displaylist_fps", Setting.Create( new I18N.I18NString("vr_displaylist_fps", "Engine FPS (for animations and moving objects, keep these as low as possible for best performance)"), 25 ) },
                { "vr_displaylist_synced", Setting.Create( new I18N.I18NString("vr_displaylist_synced", "Sync VR and Engine FPS (fixes model jitter, but poor performance)"), false ) },
            } },

            { new I18N.I18NString("GraphicsSettings.Quality", "Quality"), new OrderedDictionary<string, Setting>() {
                { "vr_use_hd_models", Setting.Create( new I18N.I18NString("vr_use_hd_models", "Use HD models"), false ) },
                { "vr_hd_textures_enabled", Setting.Create( new I18N.I18NString("vr_hd_textures_enabled", "Use HD textures"), false ) }
            } },

            { new I18N.I18NString("GraphicsSettings.WorldCustomizationAndScaling", "World customization & scaling"), new OrderedDictionary<string, Setting>() {
                { "vr_world_scale", Setting.Create( new I18N.I18NString("vr_world_scale", "World scale"), 1.0f ) },
                { "vr_world_z_strech", Setting.Create( new I18N.I18NString("vr_world_z_strech", "World height factor"), 1.0f ) },

                { "vr_npcscale", Setting.Create( new I18N.I18NString("vr_npcscale", "NPC scale"), 1.0f ) },
            } },

            { new I18N.I18NString("GraphicsSettings.HUD", "HUD"), new OrderedDictionary<string, Setting>() {
                { "vr_hud_mode", Setting.Create( new I18N.I18NString("vr_hud_mode", "HUD mode"), 2, new OrderedDictionary<string, I18N.I18NString>(){ { "2", new I18N.I18NString("vr_hud_mode.Controllers", "On Controllers") }, { "1", new I18N.I18NString("vr_hud_mode.TrueHUD", "In View (True HUD)") }, { "0", new I18N.I18NString("vr_hud_mode.Disabled", "Disabled") } } ) },

                { "vr_hud_health", Setting.Create( new I18N.I18NString("vr_hud_health", "Show health in HUD"), true ) },
                { "vr_hud_damage_indicator", Setting.Create( new I18N.I18NString("vr_hud_damage_indicator", "Show damage indicator in HUD"), true ) },
                { "vr_hud_flashlight", Setting.Create( new I18N.I18NString("vr_hud_flashlight", "Show flashlight in HUD"), true ) },
                { "vr_hud_ammo", Setting.Create( new I18N.I18NString("vr_hud_ammo", "Show ammo in HUD"), true ) },
            } },
        };

        public OrderedDictionary<I18N.I18NString, OrderedDictionary<string, Setting>> AudioSettings = new OrderedDictionary<I18N.I18NString, OrderedDictionary<string, Setting>>()
        {
            { new I18N.I18NString("AudioSettings.FMOD", "FMOD"), new OrderedDictionary<string, Setting>() {
                { "vr_use_fmod", Setting.Create( new I18N.I18NString("vr_use_fmod", "Use FMOD"), true ) },
                { "vr_fmod_3d_occlusion", Setting.Create( new I18N.I18NString("vr_fmod_3d_occlusion", "Enable 3D occlusion (only works with FMOD enabled)"), true ) },
                { "vr_fmod_wall_occlusion", Setting.Create( new I18N.I18NString("vr_fmod_wall_occlusion", "Wall occlusion (how much sound gets blocked by walls in %)"), 40 ) },
                { "vr_fmod_door_occlusion", Setting.Create( new I18N.I18NString("vr_fmod_door_occlusion", "Door occlusion (how much sound gets blocked by doors in %)"), 30 ) },
                { "vr_fmod_water_occlusion", Setting.Create( new I18N.I18NString("vr_fmod_water_occlusion", "Water occlusion (how much sound gets blocked by water in %)"), 20 ) },
                { "vr_fmod_glass_occlusion", Setting.Create( new I18N.I18NString("vr_fmod_glass_occlusion", "Glass occlusion (how much sound gets blocked by glass in %)"), 10 ) },
            } },

            { CategorySpeechRecognition, new OrderedDictionary<string, Setting>() {
                { "vr_speech_commands_enabled", Setting.Create( new I18N.I18NString("vr_speech_commands_enabled", "Enable speech recognition"), false ) },

                { SpeechRecognitionLanguage, Setting.Create( new I18N.I18NString("SpeechRecognitionLanguage", "Language"), "1400", new OrderedDictionary<string, I18N.I18NString>(){ { "1400", new I18N.I18NString("vr_speech_language_id.1400", "System Default") } } ) },

                { "vr_speech_commands_follow", Setting.Create( new I18N.I18NString("vr_speech_commands_follow", "Follow commands"), "follow me,come on,come,come with me,lets go" ) },
                { "vr_speech_commands_wait", Setting.Create( new I18N.I18NString("vr_speech_commands_wait", "Wait commands"), "wait here,wait,stop,hold") },
                { "vr_speech_commands_hello", Setting.Create( new I18N.I18NString("vr_speech_commands_hello", "Greetings"), "hello,good morning,hey,hi,morning,greetings" ) },
            } },
        };

        public OrderedDictionary<I18N.I18NString, OrderedDictionary<string, Setting>> OtherSettings = new OrderedDictionary<I18N.I18NString, OrderedDictionary<string, Setting>>()
        {
            { new I18N.I18NString("OtherSettings.AdvancedHUD", "Advanced HUD"), new OrderedDictionary<string, Setting>() {
                { "vr_hud_size", Setting.Create( new I18N.I18NString("vr_hud_size", "HUD scale"), 1 ) },
                { "vr_hud_textscale", Setting.Create( new I18N.I18NString("vr_hud_textscale", "HUD text scale"), 1 ) },

                { "vr_hud_ammo_offset_x", Setting.Create( new I18N.I18NString("vr_hud_ammo_offset_x", "HUD ammo display offset X"), 3 ) },
                { "vr_hud_ammo_offset_y", Setting.Create( new I18N.I18NString("vr_hud_ammo_offset_y", "HUD ammo display offset Y"), -3 ) },

                { "vr_hud_flashlight_offset_x", Setting.Create( new I18N.I18NString("vr_hud_flashlight_offset_x", "HUD flashlight display offset X"), -5 ) },
                { "vr_hud_flashlight_offset_y", Setting.Create( new I18N.I18NString("vr_hud_flashlight_offset_y", "HUD flashlight display offset Y"), -2 ) },

                { "vr_hud_health_offset_x", Setting.Create( new I18N.I18NString("vr_hud_health_offset_x", "HUD health display offset X"), -4 ) },
                { "vr_hud_health_offset_y", Setting.Create( new I18N.I18NString("vr_hud_health_offset_y", "HUD health display offset Y"), -3 ) },
            } },

            { new I18N.I18NString("OtherSettings.IndividualWeaponScaling", "Individual weapon scaling"), new OrderedDictionary<string, Setting>() {
                { "vr_weaponscale", Setting.Create( new I18N.I18NString("vr_weaponscale", "Weapon scale"), 1.0f ) },

                { "vr_gordon_hand_scale", Setting.Create( new I18N.I18NString("vr_gordon_hand_scale", "Hand scale"), 1.0f ) },
                { "vr_crowbar_scale", Setting.Create( new I18N.I18NString("vr_crowbar_scale", "Crowbar scale"), 1.0f ) },

                { "vr_9mmhandgun_scale", Setting.Create( new I18N.I18NString("vr_9mmhandgun_scale", "9mm handgun scale"), 1.0f ) },
                { "vr_357_scale", Setting.Create( new I18N.I18NString("vr_357_scale", "357 scale"), 1.0f ) },

                { "vr_9mmar_scale", Setting.Create( new I18N.I18NString("vr_9mmar_scale", "9mm AR scale"), 1.0f ) },
                { "vr_shotgun_scale", Setting.Create( new I18N.I18NString("vr_shotgun_scale", "Shotgun scale"), 1.0f ) },
                { "vr_crossbow_scale", Setting.Create( new I18N.I18NString("vr_crossbow_scale", "Crossbow scale"), 1.0f ) },

                { "vr_rpg_scale", Setting.Create( new I18N.I18NString("vr_rpg_scale", "RPG scale"), 1.0f ) },
                { "vr_gauss_scale", Setting.Create( new I18N.I18NString("vr_gauss_scale", "Gauss scale"), 1.0f ) },
                { "vr_egon_scale", Setting.Create( new I18N.I18NString("vr_egon_scale", "Egon scale"), 1.0f ) },
                { "vr_hgun_scale", Setting.Create( new I18N.I18NString("vr_hgun_scale", "Hornet gun scale"), 1.0f ) },

                { "vr_grenade_scale", Setting.Create( new I18N.I18NString("vr_grenade_scale", "Grenade scale"), 1.0f ) },
                { "vr_tripmine_scale", Setting.Create( new I18N.I18NString("vr_tripmine_scale", "Tripmine scale"), 1.0f ) },
                { "vr_satchel_scale", Setting.Create( new I18N.I18NString("vr_satchel_scale", "Satchel"), 1.0f ) },
                { "vr_satchel_radio_scale", Setting.Create( new I18N.I18NString("vr_satchel_radio_scale", "Satchel radio scale"), 1.0f ) },
                { "vr_squeak_scale", Setting.Create( new I18N.I18NString("vr_squeak_scale", "Snark scale"), 1.0f ) },
            } },

            { new I18N.I18NString("OtherSettings.Other", "Other"), new OrderedDictionary<string, Setting>() {
                { "vr_force_introtrainride", Setting.Create( new I18N.I18NString("vr_force_introtrainride", "Enable hacky fix for issues with intro train ride"), true ) },
                { "vr_view_dist_to_walls", Setting.Create( new I18N.I18NString("vr_view_dist_to_walls", "Minimum view distance to walls"), 5.0f ) },
                { "vr_classic_mode", Setting.Create( new I18N.I18NString("vr_classic_mode", "Classic mode (unchanged vanilla 1998 models and textures, overrides HD graphics settings)"), false ) },
            } },
        };

        private static void CopyNewSettings(OrderedDictionary<string, OrderedDictionary<string, Setting>> settings, OrderedDictionary<string, OrderedDictionary<string, Setting>> newSettings)
        {
            foreach (var settingCategory in settings)
            {
                foreach (var settingName in settingCategory.Value.Keys)
                {
                    try
                    {
                        settingCategory.Value[settingName].Value = newSettings[settingCategory.Key][settingName].Value;
                    }
                    catch (KeyNotFoundException) { }
                }
            }
        }

        public IEnumerable<KeyValuePair<string, Setting>> KeyValuePairs
        {
            get
            {
                foreach (var foo in new[] { InputSettings, GraphicsSettings, AudioSettings, OtherSettings })
                {
                    foreach (var bar in foo)
                    {
                        foreach (var boo in bar.Value)
                        {
                            yield return boo;
                        }
                    }
                }
            }
        }

        public bool SetValue(string key, string value)
        {
            foreach (var foo in new[] { InputSettings, GraphicsSettings, AudioSettings, OtherSettings })
            {
                foreach (var bar in foo)
                {
                    if (bar.Value.TryGetValue(key, out Setting setting))
                    {
                        setting.SetValue(value);
                        return true;
                    }
                }
            }
            return false;
        }

        public HLVRModSettings()
        {
        }
    }
}
