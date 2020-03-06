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

        public float GetWorldScale()
        {
            float.TryParse(GraphicsSettings[HLVRModSettings.CategoryWorldCustomizationAndScaling][HLVRModSettings.WorldScale].Value, out float value);
            return value;
        }

        public static readonly SettingCategory CategoryAntinauseaFeatures = new SettingCategory(
            new I18N.I18NString("InputSettings.AntinauseaFeatures", "Anti-nausea features"));

        public static readonly SettingCategory CategoryMovementSpeed = new SettingCategory(
            new I18N.I18NString("InputSettings.MovementSpeed", "Movement Speed"));

        public static readonly SettingCategory CategoryControlsAndAccessibility = new SettingCategory(
             new I18N.I18NString("InputSettings.ControlsAndAccessibility", "Controls & Accessibility"));

        public static readonly SettingCategory CategoryMovement = new SettingCategory(
             new I18N.I18NString("InputSettings.Movement", "Movement"));

        public static readonly SettingCategory CategoryLadders = new SettingCategory(
            new I18N.I18NString("InputSettings.Ladders", "Ladders"));

        public static readonly SettingCategory CategoryTrains = new SettingCategory(
            new I18N.I18NString("InputSettings.Trains", "Usable trains"));

        public static readonly SettingCategory CategoryMountedGuns = new SettingCategory(
            new I18N.I18NString("InputSettings.MountedGuns", "Mounted guns"));


        public static readonly SettingCategory CategoryFPSAndPerformance = new SettingCategory(
            new I18N.I18NString("GraphicsSettings.FPSAndPerformance", "FPS & Performance"));

        public static readonly SettingCategory CategoryQuality = new SettingCategory(
            new I18N.I18NString("GraphicsSettings.Quality", "Quality"),
            new SettingDependency("vr_classic_mode", "0"));

        public static readonly SettingCategory CategoryWorldCustomizationAndScaling = new SettingCategory(
            new I18N.I18NString("GraphicsSettings.WorldCustomizationAndScaling", "World customization & scaling"));
        public static readonly string WorldScale = "vr_world_scale";

        public static readonly SettingCategory CategoryHUD = new SettingCategory(
            new I18N.I18NString("GraphicsSettings.HUD", "HUD"));


        public static readonly SettingCategory CategoryFMOD = new SettingCategory(
            new I18N.I18NString("AudioSettings.FMOD", "FMOD"));

        public static readonly SettingCategory CategorySpeechRecognition = new SettingCategory(
            new I18N.I18NString("AudioSettings.SpeechRecognition", "Speech recognition"),
            new I18N.I18NString("AudioSettings.SpeechRecognition.Description", "Please note that speech recognition uses the Microsoft Speech API that is a part of Windows. You need to have Windows Speech Recognition properly configured, enabled, and running.")
            );
        public static readonly string SpeechRecognitionLanguage = "vr_speech_language_id";


        public static readonly SettingCategory CategoryAdvancedHUD = new SettingCategory(
            new I18N.I18NString("OtherSettings.AdvancedHUD", "Advanced HUD"),
            new I18N.I18NString("OtherSettings.AdvancedHUD", "Advanced HUD"),
            new SettingDependency("vr_hud_mode", "0", SettingDependency.MatchMode.MUST_NOT_MATCH));

        public static readonly SettingCategory CategoryIndividualWeaponScaling = new SettingCategory(
            new I18N.I18NString("OtherSettings.IndividualWeaponScaling", "Individual weapon scaling"));

        public static readonly SettingCategory CategoryOther = new SettingCategory(
            new I18N.I18NString("OtherSettings.Other", "Other"));



        public OrderedDictionary<SettingCategory, OrderedDictionary<string, Setting>> InputSettings = new OrderedDictionary<SettingCategory, OrderedDictionary<string, Setting>>()
        {
            { CategoryAntinauseaFeatures, new OrderedDictionary<string, Setting>() {
                { "vr_move_instant_accelerate", Setting.Create( new I18N.I18NString("vr_move_instant_accelerate", "Instant acceleration"), true ) },
                { "vr_move_instant_decelerate", Setting.Create( new I18N.I18NString("vr_move_instant_decelerate", "Instant deceleration"), true ) },
                { "vr_rotate_with_trains", Setting.Create( new I18N.I18NString("vr_rotate_with_trains", "Rotate with trains/elevators"), true ) },
                { "vr_no_gauss_recoil", Setting.Create( new I18N.I18NString("vr_no_gauss_recoil", "Disable gauss recoil"), true ) },
                { "vr_disable_triggerpush", Setting.Create( new I18N.I18NString("vr_disable_triggerpush", "Disable areas that push the player (e.g. strong wind)"), true ) },
                { "vr_disable_func_friction", Setting.Create( new I18N.I18NString("vr_disable_func_friction", "Disable slippery surfaces"), true ) },
                { "vr_xenjumpthingies_teleporteronly", Setting.Create( new I18N.I18NString("vr_xenjumpthingies_teleporteronly", "Disable being pushed up by xen jump thingies (you can use the teleporter on those things)"), true ) },
                { "vr_semicheat_spinthingyspeed", Setting.Create( new I18N.I18NString("vr_semicheat_spinthingyspeed", "Set maximum speed for those fast spinning platforms near the tentacle monster (map c1a4f) (Game Default: 110)"), SettingType.SPEED, "50" ) },
                //{ "vr_smooth_steps", Setting.Create( new I18N.I18NString("vr_smooth_steps", "Smooth out steps (may affect performance!)"), new OrderedDictionary<string, I18N.I18NString>(){ {"0", new I18N.I18NString("vr_smooth_steps.Disabled", "Disabled") }, { "1", new I18N.I18NString("vr_smooth_steps.Most", "Smooth out most steps") }, { "2", new I18N.I18NString("vr_smooth_steps.All", "Smooth out all steps") } }, "0" ) },
            } },

            { CategoryMovementSpeed, new OrderedDictionary<string, Setting>() {
                { "vr_forwardspeed", Setting.Create( new I18N.I18NString("vr_forwardspeed", "Forward speed"), SettingType.SPEED, "200" ) },
                { "vr_backspeed", Setting.Create( new I18N.I18NString("vr_backspeed", "Backward speed"), SettingType.SPEED, "200" ) },
                { "vr_sidespeed", Setting.Create( new I18N.I18NString("vr_sidespeed", "Sideways speed"), SettingType.SPEED, "200" ) },
                { "vr_upspeed", Setting.Create( new I18N.I18NString("vr_upspeed", "Up/Down speed"), SettingType.SPEED, "200" ) },
                { "vr_yawspeed", Setting.Create( new I18N.I18NString("vr_yawspeed", "Turn speed (degrees/s)"), SettingType.COUNT, "210" ) },
                { "vr_togglewalk", Setting.Create( new I18N.I18NString("vr_togglewalk", "Walk Input Mode"), new OrderedDictionary<string, I18N.I18NString>(){ {"0", new I18N.I18NString("vr_togglewalk.Hold", "Hold (Hold to Walk, release to run)") }, { "1", new I18N.I18NString("vr_togglewalk.Toggle", "Toggle (Click to switch between walking and running)") } }, "0" ) },
                { "vr_walkspeedfactor", Setting.Create( new I18N.I18NString("vr_walkspeedfactor", "Speed factor when 'walk' is active"), SettingType.FACTOR, "0.3" ) },
            } },

            { CategoryControlsAndAccessibility, new OrderedDictionary<string, Setting>() {
                { "vr_lefthand_mode", Setting.Create( new I18N.I18NString("vr_lefthand_mode", "Left hand mode"), false ) },

                { "vr_crowbar_vanilla_attack_enabled", Setting.Create( new I18N.I18NString("vr_crowbar_vanilla_attack_enabled", "Enable crowbar melee attack on 'fire' action."), false ) },

                { "vr_flashlight_toggle", Setting.Create( new I18N.I18NString("vr_flashlight_toggle", "Flashlight should toggle"), false ) },

                { "vr_enable_aim_laser", Setting.Create( new I18N.I18NString("vr_enable_aim_laser", "Enable aim laser pointer for range weapons"), false ) },

                { "vr_make_levers_nonsolid", Setting.Create( new I18N.I18NString("vr_make_levers_nonsolid", "Make levers non-solid (fixes some collision issues)"), true ) },

                { "vr_use_animated_weapons", Setting.Create( new I18N.I18NString("vr_use_animated_weapons", "Use weapon models with animated movement (affects aiming)"), false ) },

                { "vr_melee_swing_speed", Setting.Create( new I18N.I18NString("vr_melee_swing_speed", "Minimum controller speed to trigger melee damage"), SettingType.SPEED, "100" ) },

                { "vr_headset_offset", Setting.Create( new I18N.I18NString("vr_headset_offset", "HMD height offset"), SettingType.DISTANCE, "0" ) },

                { "vr_weapon_grenade_mode", Setting.Create( new I18N.I18NString("vr_weapon_grenade_mode", "Grenade throw mode"), new OrderedDictionary<string, I18N.I18NString>(){ {"0", new I18N.I18NString("vr_weapon_grenade_mode.ControllerVelocity", "Use controller velocity") }, { "1", new I18N.I18NString("vr_weapon_grenade_mode.ControllerAimGameVelocity", "Aim with controller, but fly with fixed speed (as in original game)") } }, "0" ) },

                { "vr_teleport_attachment", Setting.Create( new I18N.I18NString("vr_teleport_attachment", "Teleporter attachment"), new OrderedDictionary<string, I18N.I18NString>(){ {"0", new I18N.I18NString("vr_attachment.Hand", "Hand") }, { "1", new I18N.I18NString("vr_attachment.Weapon", "Weapon") }, { "2", new I18N.I18NString("vr_attachment.Head", "Head (HMD)") }, { "3", new I18N.I18NString("vr_attachment.Pose", "SteamVR Input pose") } }, "0" ) },
                { "vr_flashlight_attachment", Setting.Create( new I18N.I18NString("vr_flashlight_attachment", "Flashlight attachment"), new OrderedDictionary<string, I18N.I18NString>(){ {"0", new I18N.I18NString("vr_attachment.Hand", "Hand") }, { "1", new I18N.I18NString("vr_attachment.Weapon", "Weapon") }, { "2", new I18N.I18NString("vr_attachment.Head", "Head (HMD)") }, { "3", new I18N.I18NString("vr_attachment.Pose", "SteamVR Input pose") } }, "0") },
                { "vr_movement_attachment", Setting.Create( new I18N.I18NString("vr_movement_attachment", "Movement attachment"), new OrderedDictionary<string, I18N.I18NString>(){ {"0", new I18N.I18NString("vr_attachment.Hand", "Hand") }, { "1", new I18N.I18NString("vr_attachment.Weapon", "Weapon") }, { "2", new I18N.I18NString("vr_attachment.Head", "Head (HMD)") }, { "3", new I18N.I18NString("vr_attachment.Pose", "SteamVR Input pose") } }, "0") },

                { "vr_enable_interactive_debris", Setting.Create( new I18N.I18NString("vr_enable_interactive_debris", "Enable interactive debris"), true ) },
                { "vr_drag_onlyhand", Setting.Create( new I18N.I18NString("vr_drag_onlyhand", "Only allow dragging and grabbing objects with bare hands (not with a weapon)"), false ) },
            } },

            { CategoryMovement, new OrderedDictionary<string, Setting>() {
                { "vr_rl_ducking_enabled", Setting.Create( new I18N.I18NString("vr_rl_ducking_enabled", "Enable real-life crouch detection"), true ) },
                { "vr_rl_duck_height", Setting.Create( new I18N.I18NString("vr_rl_duck_height", "Real-life crouch height"), SettingType.DISTANCE, "36", new SettingDependency("vr_rl_ducking_enabled", "1") ) },
                { "vr_playerturn_enabled", Setting.Create( new I18N.I18NString("vr_playerturn_enabled", "Enable player turning"), true ) },
                { "vr_autocrouch_enabled", Setting.Create( new I18N.I18NString("vr_autocrouch_enabled", "Enabled automatic crouching"), true ) },
                { "vr_move_analogforward_inverted", Setting.Create( new I18N.I18NString("vr_move_analogforward_inverted", "Invert analog forward input"), false ) },
                { "vr_move_analogsideways_inverted", Setting.Create( new I18N.I18NString("vr_move_analogsideways_inverted", "Invert analog sideways input"), false ) },
                { "vr_move_analogupdown_inverted", Setting.Create( new I18N.I18NString("vr_move_analogupdown_inverted", "Invert analog updown input"), false ) },
                { "vr_move_analogturn_inverted", Setting.Create( new I18N.I18NString("vr_move_analogturn_inverted", "Invert analog turn input"), false ) },
                { "vr_move_analog_deadzone", Setting.Create( new I18N.I18NString("vr_move_analog_deadzone", "Minimum analog input to trigger action"), SettingType.FACTOR, "0.1" ) },
            } },

            { CategoryLadders, new OrderedDictionary<string, Setting>() {
                { "vr_ladder_immersive_movement_enabled", Setting.Create( new I18N.I18NString("vr_ladder_immersive_movement_enabled", "Enabled immersive climbing of ladders"), true ) },
                { "vr_ladder_immersive_movement_swinging_enabled", Setting.Create( new I18N.I18NString("vr_ladder_immersive_movement_swinging_enabled", "Enable momentum from immersive ladder climbing"), true, new SettingDependency("vr_ladder_immersive_movement_enabled", "1") ) },
                { "vr_ladder_legacy_movement_enabled", Setting.Create( new I18N.I18NString("vr_ladder_legacy_movement_enabled", "Enable legacy climbing of ladders"), false ) },
                { "vr_ladder_legacy_movement_only_updown", Setting.Create( new I18N.I18NString("vr_ladder_legacy_movement_only_updown", "Restrict movement on ladders to up/down only (legacy movement)"), false, new SettingDependency("vr_ladder_legacy_movement_enabled", "1") ) },
                { "vr_ladder_legacy_movement_speed", Setting.Create( new I18N.I18NString("vr_ladder_legacy_movement_speed", "Climb speed on ladders (legacy movement)"), SettingType.SPEED, "100", new SettingDependency("vr_ladder_legacy_movement_enabled", "1") ) },
            } },

            { CategoryTrains, new OrderedDictionary<string, Setting>() {
                { "vr_train_autostop", Setting.Create( new I18N.I18NString("vr_train_autostop", "Automatically stop trains when falling off"), false ) },
                { "vr_train_controls", Setting.Create( new I18N.I18NString("vr_train_controls", "Control mode"), new OrderedDictionary<string, I18N.I18NString>(){ {"0", new I18N.I18NString("vr_train_controls.VR", "VR Mode (Standing on train activates train, touching train interface controls train speed.)") }, { "1", new I18N.I18NString("vr_train_controls.Legacy", "Legacy Mode (LegacyUse activates train, forward and backward move control train speed.)") } }, "0" ) },
                { "vr_train_speed_slow", Setting.Create( new I18N.I18NString("vr_train_speed_slow", "Train speed slow"), SettingType.FACTOR, "0.2", new SettingDependency("vr_train_controls", "0") ) },
                { "vr_train_speed_medium", Setting.Create( new I18N.I18NString("vr_train_speed_medium", "Train speed medium"), SettingType.FACTOR, "0.4", new SettingDependency("vr_train_controls", "0") ) },
                { "vr_train_speed_fast", Setting.Create( new I18N.I18NString("vr_train_speed_fast", "Train speed fast"), SettingType.FACTOR, "0.6", new SettingDependency("vr_train_controls", "0") ) },
                { "vr_train_speed_back", Setting.Create( new I18N.I18NString("vr_train_speed_back", "Train speed backwards"), SettingType.FACTOR, "0.2", new SettingDependency("vr_train_controls", "0") ) },
            } },

            { CategoryMountedGuns, new OrderedDictionary<string, Setting>() {
                { "vr_tankcontrols", Setting.Create( new I18N.I18NString("vr_tankcontrols", "Immersive controls"), new OrderedDictionary<string, I18N.I18NString>(){ {"0", new I18N.I18NString("vr_tankcontrols.Disabled", "Disabled") }, { "1", new I18N.I18NString("vr_tankcontrols.EnabledOneHand", "Enabled with one hand") }, { "2", new I18N.I18NString("vr_tankcontrols.EnabledTwoHands", "Enabled with both hands") } }, "2" ) },
                { "vr_tankcontrols_max_distance", Setting.Create( new I18N.I18NString("vr_tankcontrols_max_distance", "Maximum controller distance to gun for immersive controls"), SettingType.DISTANCE, "128", new SettingDependency("vr_tankcontrols", "0", SettingDependency.MatchMode.MUST_NOT_MATCH) ) },
                { "vr_tankcontrols_instant_turn", Setting.Create( new I18N.I18NString("vr_tankcontrols_instant_turn", "Enable instant-turn (if off, guns turn with a predefined speed, lagging behind your hands)"), false, new SettingDependency("vr_tankcontrols", "0", SettingDependency.MatchMode.MUST_NOT_MATCH) ) },
                { "vr_make_mountedguns_nonsolid", Setting.Create( new I18N.I18NString("vr_make_mountedguns_nonsolid", "Make mounted guns non-solid (fixes some collision issues)"), true ) },
                { "vr_legacy_tankcontrols_enabled", Setting.Create( new I18N.I18NString("vr_legacy_tankcontrols_enabled", "Enable control of usable guns with 'LegacyUse'."), false ) },
            } },
        };

        public OrderedDictionary<SettingCategory, OrderedDictionary<string, Setting>> GraphicsSettings = new OrderedDictionary<SettingCategory, OrderedDictionary<string, Setting>>()
        {
            { CategoryFPSAndPerformance, new OrderedDictionary<string, Setting>() {
                { "vr_headset_fps", Setting.Create( new I18N.I18NString("vr_headset_fps", "FPS (Sync these with your SteamVR settings. It is highly recommended that you play Half-Life: VR at no more than 90fps.)"), SettingType.COUNT, "90" ) },
            } },

            { CategoryQuality, new OrderedDictionary<string, Setting>() {
                { "vr_use_hd_models", Setting.Create( new I18N.I18NString("vr_use_hd_models", "Use HD models"), false ) },
                { "vr_hd_textures_enabled", Setting.Create( new I18N.I18NString("vr_hd_textures_enabled", "Use HD textures"), false ) },
                { "vr_texturemode", Setting.Create( new I18N.I18NString("vr_texturemode", "Texture Mode"),
                    new OrderedDictionary<string, I18N.I18NString>(){
                        {"GL_NEAREST", new I18N.I18NString("vr_texturemode.GL_NEAREST", "GL_NEAREST (Crisp clear pixelated 1998 texture look.)") },
                        { "GL_LINEAR", new I18N.I18NString("vr_texturemode.GL_LINEAR", "GL_LINEAR (Smoothes pixels a bit, no mipmaps.)") },
                        { "GL_NEAREST_MIPMAP_NEAREST", new I18N.I18NString("vr_texturemode.GL_NEAREST_MIPMAP_NEAREST", "GL_NEAREST_MIPMAP_NEAREST (Crisp clear pixels, uses nearest mipmap.)") },
                        { "GL_LINEAR_MIPMAP_NEAREST", new I18N.I18NString("vr_texturemode.GL_LINEAR_MIPMAP_NEAREST", "GL_LINEAR_MIPMAP_NEAREST (Smoothes pixels, uses nearest mipmap.)") },
                        { "GL_NEAREST_MIPMAP_LINEAR", new I18N.I18NString("vr_texturemode.GL_NEAREST_MIPMAP_LINEAR", "GL_NEAREST_MIPMAP_LINEAR (Crisp clear pixels, merges mipmaps.)") },
                        { "GL_LINEAR_MIPMAP_LINEAR", new I18N.I18NString("vr_texturemode.GL_LINEAR_MIPMAP_LINEAR", "GL_LINEAR_MIPMAP_LINEAR (Smoothes pixels and merges mipmaps.)") },
                    }, "GL_LINEAR_MIPMAP_LINEAR" ) },
            } },

            { CategoryWorldCustomizationAndScaling, new OrderedDictionary<string, Setting>() {
                { WorldScale, Setting.Create( new I18N.I18NString(WorldScale, "World scale"), SettingType.FACTOR, "1" ) },
                { "vr_world_z_strech", Setting.Create( new I18N.I18NString("vr_world_z_strech", "World height factor"), SettingType.FACTOR, "1" ) },

                { "vr_npcscale", Setting.Create( new I18N.I18NString("vr_npcscale", "NPC scale"), SettingType.FACTOR, "1" ) },
            } },

            { CategoryHUD, new OrderedDictionary<string, Setting>() {
                { "vr_hud_mode", Setting.Create( new I18N.I18NString("vr_hud_mode", "HUD mode"), new OrderedDictionary<string, I18N.I18NString>(){ { "2", new I18N.I18NString("vr_hud_mode.Controllers", "On Controllers") }, { "1", new I18N.I18NString("vr_hud_mode.TrueHUD", "In View (True HUD)") }, { "0", new I18N.I18NString("vr_hud_mode.Disabled", "Disabled") } }, "2" ) },

                { "vr_hud_health", Setting.Create( new I18N.I18NString("vr_hud_health", "Show health in HUD"), true, new SettingDependency("vr_hud_mode", "0", SettingDependency.MatchMode.MUST_NOT_MATCH) ) },
                { "vr_hud_damage_indicator", Setting.Create( new I18N.I18NString("vr_hud_damage_indicator", "Show damage indicator in HUD"), true, new SettingDependency("vr_hud_mode", "0", SettingDependency.MatchMode.MUST_NOT_MATCH) ) },
                { "vr_hud_flashlight", Setting.Create( new I18N.I18NString("vr_hud_flashlight", "Show flashlight in HUD"), true, new SettingDependency("vr_hud_mode", "0", SettingDependency.MatchMode.MUST_NOT_MATCH) ) },
                { "vr_hud_ammo", Setting.Create( new I18N.I18NString("vr_hud_ammo", "Show ammo in HUD"), true, new SettingDependency("vr_hud_mode", "0", SettingDependency.MatchMode.MUST_NOT_MATCH) ) },
            } },
        };

        public OrderedDictionary<SettingCategory, OrderedDictionary<string, Setting>> AudioSettings = new OrderedDictionary<SettingCategory, OrderedDictionary<string, Setting>>()
        {
            { CategoryFMOD, new OrderedDictionary<string, Setting>() {
                { "vr_use_fmod", Setting.Create( new I18N.I18NString("vr_use_fmod", "Use FMOD"), true ) },
                { "vr_fmod_3d_occlusion", Setting.Create( new I18N.I18NString("vr_fmod_3d_occlusion", "Enable 3D occlusion (only works with FMOD enabled)"), true, new SettingDependency("vr_use_fmod", "1") ) },
                { "vr_fmod_wall_occlusion", Setting.Create( new I18N.I18NString("vr_fmod_wall_occlusion", "Wall occlusion (how much sound gets blocked by walls in %)"), SettingType.COUNT, "40", new SettingDependency("vr_fmod_3d_occlusion", "1") ) },
                { "vr_fmod_door_occlusion", Setting.Create( new I18N.I18NString("vr_fmod_door_occlusion", "Door occlusion (how much sound gets blocked by doors in %)"), SettingType.COUNT, "30", new SettingDependency("vr_fmod_3d_occlusion", "1") ) },
                { "vr_fmod_water_occlusion", Setting.Create( new I18N.I18NString("vr_fmod_water_occlusion", "Water occlusion (how much sound gets blocked by water in %)"), SettingType.COUNT, "20", new SettingDependency("vr_fmod_3d_occlusion", "1") ) },
                { "vr_fmod_glass_occlusion", Setting.Create( new I18N.I18NString("vr_fmod_glass_occlusion", "Glass occlusion (how much sound gets blocked by glass in %)"), SettingType.COUNT, "10", new SettingDependency("vr_fmod_3d_occlusion", "1") ) },
            } },

            { CategorySpeechRecognition, new OrderedDictionary<string, Setting>() {
                { "vr_speech_commands_enabled", Setting.Create( new I18N.I18NString("vr_speech_commands_enabled", "Enable speech recognition"), false ) },

                { SpeechRecognitionLanguage, Setting.Create( new I18N.I18NString("SpeechRecognitionLanguage", "Language"), new OrderedDictionary<string, I18N.I18NString>(){ { "1400", new I18N.I18NString("vr_speech_language_id.1400", "System Default") } }, "1400", new SettingDependency("vr_speech_commands_enabled", "1") ) },

                { "vr_speech_commands_follow", Setting.Create( new I18N.I18NString("vr_speech_commands_follow", "Follow commands"), "follow me,come on,come,come with me,lets go", new SettingDependency("vr_speech_commands_enabled", "1") ) },
                { "vr_speech_commands_wait", Setting.Create( new I18N.I18NString("vr_speech_commands_wait", "Wait commands"), "wait here,wait,stop,hold", new SettingDependency("vr_speech_commands_enabled", "1") ) },
                { "vr_speech_commands_hello", Setting.Create( new I18N.I18NString("vr_speech_commands_hello", "Greetings"), "hello,good morning,hey,hi,morning,greetings", new SettingDependency("vr_speech_commands_enabled", "1") ) },
            } },
        };

        public OrderedDictionary<SettingCategory, OrderedDictionary<string, Setting>> OtherSettings = new OrderedDictionary<SettingCategory, OrderedDictionary<string, Setting>>()
        {
            { CategoryAdvancedHUD, new OrderedDictionary<string, Setting>() {
                { "vr_hud_size", Setting.Create( new I18N.I18NString("vr_hud_size", "HUD scale"), SettingType.FACTOR, "1" ) },
                { "vr_hud_textscale", Setting.Create( new I18N.I18NString("vr_hud_textscale", "HUD text scale"), SettingType.FACTOR, "1" ) },

                { "vr_hud_ammo_offset_x", Setting.Create( new I18N.I18NString("vr_hud_ammo_offset_x", "HUD ammo display offset X"), SettingType.COUNT, "3", new SettingDependency("vr_hud_ammo", "1") ) },
                { "vr_hud_ammo_offset_y", Setting.Create( new I18N.I18NString("vr_hud_ammo_offset_y", "HUD ammo display offset Y"), SettingType.COUNT, "-3", new SettingDependency("vr_hud_ammo", "1") ) },

                { "vr_hud_flashlight_offset_x", Setting.Create( new I18N.I18NString("vr_hud_flashlight_offset_x", "HUD flashlight display offset X"), SettingType.COUNT, "-5", new SettingDependency("vr_hud_flashlight", "1") ) },
                { "vr_hud_flashlight_offset_y", Setting.Create( new I18N.I18NString("vr_hud_flashlight_offset_y", "HUD flashlight display offset Y"), SettingType.COUNT, "-2", new SettingDependency("vr_hud_flashlight", "1") ) },

                { "vr_hud_health_offset_x", Setting.Create( new I18N.I18NString("vr_hud_health_offset_x", "HUD health display offset X"), SettingType.COUNT, "-4", new SettingDependency("vr_hud_health", "1") ) },
                { "vr_hud_health_offset_y", Setting.Create( new I18N.I18NString("vr_hud_health_offset_y", "HUD health display offset Y"), SettingType.COUNT, "-3", new SettingDependency("vr_hud_health", "1") ) },
            } },

            { CategoryIndividualWeaponScaling, new OrderedDictionary<string, Setting>() {
                { "vr_weaponscale", Setting.Create( new I18N.I18NString("vr_weaponscale", "Weapon scale"), SettingType.FACTOR, "1" ) },

                { "vr_gordon_hand_scale", Setting.Create( new I18N.I18NString("vr_gordon_hand_scale", "Hand scale"), SettingType.FACTOR, "1" ) },
                { "vr_crowbar_scale", Setting.Create( new I18N.I18NString("vr_crowbar_scale", "Crowbar scale"), SettingType.FACTOR, "1" ) },

                { "vr_9mmhandgun_scale", Setting.Create( new I18N.I18NString("vr_9mmhandgun_scale", "9mm handgun scale"), SettingType.FACTOR, "1" ) },
                { "vr_357_scale", Setting.Create( new I18N.I18NString("vr_357_scale", "357 scale"), SettingType.FACTOR, "1" ) },

                { "vr_9mmar_scale", Setting.Create( new I18N.I18NString("vr_9mmar_scale", "9mm AR scale"), SettingType.FACTOR, "1" ) },
                { "vr_shotgun_scale", Setting.Create( new I18N.I18NString("vr_shotgun_scale", "Shotgun scale"), SettingType.FACTOR, "1" ) },
                { "vr_crossbow_scale", Setting.Create( new I18N.I18NString("vr_crossbow_scale", "Crossbow scale"), SettingType.FACTOR, "1" ) },

                { "vr_rpg_scale", Setting.Create( new I18N.I18NString("vr_rpg_scale", "RPG scale"), SettingType.FACTOR, "1" ) },
                { "vr_gauss_scale", Setting.Create( new I18N.I18NString("vr_gauss_scale", "Gauss scale"), SettingType.FACTOR, "1" ) },
                { "vr_egon_scale", Setting.Create( new I18N.I18NString("vr_egon_scale", "Egon scale"), SettingType.FACTOR, "1" ) },
                { "vr_hgun_scale", Setting.Create( new I18N.I18NString("vr_hgun_scale", "Hornet gun scale"), SettingType.FACTOR, "1" ) },

                { "vr_grenade_scale", Setting.Create( new I18N.I18NString("vr_grenade_scale", "Grenade scale"), SettingType.FACTOR, "1" ) },
                { "vr_tripmine_scale", Setting.Create( new I18N.I18NString("vr_tripmine_scale", "Tripmine scale"), SettingType.FACTOR, "1" ) },
                { "vr_satchel_scale", Setting.Create( new I18N.I18NString("vr_satchel_scale", "Satchel scale"), SettingType.FACTOR, "1" ) },
                { "vr_satchel_radio_scale", Setting.Create( new I18N.I18NString("vr_satchel_radio_scale", "Satchel radio scale"), SettingType.FACTOR, "1" ) },
                { "vr_squeak_scale", Setting.Create( new I18N.I18NString("vr_squeak_scale", "Snark scale"), SettingType.FACTOR, "1" ) },
            } },

            { CategoryOther, new OrderedDictionary<string, Setting>() {
                { "vr_force_introtrainride", Setting.Create( new I18N.I18NString("vr_force_introtrainride", "Enable hacky fix for issues with intro train ride"), true ) },
                { "vr_view_dist_to_walls", Setting.Create( new I18N.I18NString("vr_view_dist_to_walls", "Minimum view distance to walls"), SettingType.DISTANCE, "5" ) },
                { "vr_classic_mode", Setting.Create( new I18N.I18NString("vr_classic_mode", "Classic mode (unchanged vanilla 1998 models and textures, overrides HD graphics settings)"), false ) },
                { "vr_npc_gunpoint", Setting.Create( new I18N.I18NString("vr_npc_gunpoint", "NPCs at gunpoint"), new OrderedDictionary<string, I18N.I18NString>(){ { "0", new I18N.I18NString("vr_npc_gunpoint.Disabled", "Disabled") }, { "1", new I18N.I18NString("vr_npc_gunpoint.React", "React, but don't get angry") }, { "2", new I18N.I18NString("vr_npc_gunpoint.ReactGetAngry", "React and get angry") } }, "1" ) },
            } },
        };

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
