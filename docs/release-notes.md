# Half-Life: VR

<img src="art/game_icon.png" alt="HLVR Game Icon" width="50"/> A highly immersive Virtual Reality mod for the classic Half-Life from 1998.

---

[Back to Main README](README.md)

---

### Release Notes

#### 0.6.30-beta - TBA

##### Mod:
  - First Release on Steam
  - Fixed skybox in VR menu
  - Fixed audio left and right being switched when using FMOD
  - Fixed wall collision and other visual bugs in roomscale VR if playerturn is disabled
  - Fixed game window/menu being visible in VR during level changes
  - Fixed issue with longjump direction
  - Fixed levers and other issues with draggable entities
  - Fixed huge crate gibs in blast pit levels
  - Fixed soda cans not being drinkable
  - Added feature that smoothes movement when walking up stairs (might be buggy, enable at own risk)
  - Restored default walk speed and legacy ladder climb speed to pancake defaults
  - Added default binds for Vive, Index, Oculus Touch, and WMR

##### HLVRConfig:
  - Added game window size to launcher config
  - Added settings for stair smoothing (on/off and intensity)

---
#### 0.6.29-beta - 2021-10-02

##### Mod:
  - Fixed hornets colliding with hornet gun right when they spawn
  - Fixed nasty bug preventing players from moving downwards (sorry!)
  - Fixed rare crash if custom actions file has invalid characters

##### HLVRConfig:
  - Added GUI for custom actions

---
#### 0.6.28-beta - 2021-03-12

##### Mod:
  - Fixed rare crash bug
  - Fixed game sometimes freezing during load/save when HLVRConfig is open
  - Fixed texture mode getting reset
  - Fixed issues when teleporting while holding onto a ladder
  - Fixed issues when teleporting while holding objects
  - Fixed issues when teleporting into teleporters
  - Fixed dead headcrabs becoming alive and invincible after being picked up
  - Fixed audio being 90° rotated in FMOD
  - Fixed shooting effects coming from right hand in left-hand mode
  - Fixed crowbar not making sounds/decals when hitting level geometry
  - Fixed some controllable stationary turrets/guns/rocket launchers not reacting to VR controllers
  - Fixed invisible/mislocated VR train controls on some trains
  - Fixed rare visual issue with low res HUD sprites
  - Added proper teleporter arc
  - Teleporter beam length now takes gravity into account (longer beam in lower gravity)
  - Added option to mirror eyes if only one eye is rendered

##### HLVRConfig:
  - Added option to mirror eyes if only one eye is rendered

---
#### 0.6.27-beta - 2020-08-03

##### Mod:
  - Fixed headset aim when playing with keyboard and without controllers
  - Fixed crash when enabling aim laser while playing with keyboard and without controllers
  - Fixed shotgun shots not hitting monsters
  - Fixed gravity being applied twice each frame
  - Fixed some weapons shooting slightly to the lower left of where you actually aim
  - Fixed some levers not reacting to controller grab
  - Fixed game freezing on rare occasion when the player gets gibbed
  - Fixed trash compactor walls having VR train controls
  - Fixed crush traps not killing the player
  - Fixed issues with the corridor cut scene after Gordon gets apprehended
  - Re-enabled VR view in game window and added setting to turn it on or off
  - Added female scientist voice files recorded by Katie Otten

##### HLVRConfig:
  - Fixed issues caused by using system language settings for number conversion
  - Fixed a rare occuring crash when closing HLVRConfig shortly after changing a setting
  - Fixed launcher settings not correctly getting stored in the settings file
  - Added option to enable left hand mode
  - Added option to enable VR view in game window
  - Added option to create minidumps of Half-Life's current state, no matter what state it is in (useful for creating dumps when the game is frozen)

---
#### 0.6.26-beta - 2020-03-21

##### Mod:
  - Fixed trajecture and HUD position on 9mmAR
  - Fixed rockets fired from rocket launcher exploding immediately because they think they collided with the rocket launcher
  - Added missing xen9 sky textures

##### HLVRConfig:
  - No changes

---
#### 0.6.25-beta - 2020-03-20

##### Mod:
  - Fixed some female scientist scripts using male audio

##### HLVRConfig:
  - No changes

---
#### 0.6.24-beta - 2020-03-20

##### Mod:
  - Fixed crash bug in model cache
  - Fixed scientist audio announcement sometimes using female voices in classic mode
  - Fixed train control offsets that were wrong on some trains
  - Added missing 320px HUD textures and fixed train menu not appearing

##### HLVRConfig:
  - No changes

---
#### 0.6.23-beta - 2020-03-19

##### Mod:
  - Fixed crashes related to FMOD
  - Fixed special entities not being recognized after save/load
  - Fixed broken retina scanner after resonance cascade not reacting to touch
  - Fixed NPCs and monsters not receiving damage from melee attacks
  - Game now prints version in log on initialization

##### HLVRConfig:
  - Log output now merges duplicate lines into one line and adds a repeat counter

---
#### 0.6.22-beta - 2020-03-18

##### Mod:
  - Fixed crash bugs introduced in 0.6.20-beta
  - Something, something... what easter egg?

##### HLVRConfig:
  - No changes

---
#### 0.6.21-beta - 2020-03-17

##### Mod:
  - Fixed bug that freezes game when teleporting in a level with ladders

##### HLVRConfig:
  - No changes

---
#### 0.6.20-beta - 2020-03-17

##### Mod:
  - Fixed buttons and other entities sometimes "blocking" themselves when trying to interact
  - Fixed bug that caused too many entities being checked for collision with controllers
  - Fixed memory leaks in FMOD implementation
  - Fixed bug in physics helper affecting collisions with brush based entities
  - Fixed UV maps on SD gauss model
  - Several performance optimizations (see commit logs in source code for details)
  - Improved precision of collision detections

##### HLVRConfig:
  - No changes

---
#### 0.6.19-beta - 2020-03-16

##### Mod:
  - Fixed crashes introduced with performance optimizations in 0.6.18-beta

##### HLVRConfig:
  - No changes

---
#### 0.6.18-beta - 2020-03-16

##### Mod:
  - Fixed rare crash that occurs when generating physics data for a map
  - Fixed rare crash that occurs when loading a savegame
  - Several performance optimizations (see commit logs in source code for details)
  - Allow players to reduce number of gibs/debris spawned by breakables

##### HLVRConfig:
  - Renamed tabs for better clarity

---
#### 0.6.17-beta - 2020-03-15

##### Mod:
  - Fixed shootable rotating buttons not reacting to being shot at when "non-solid levers" is enabled
  - Fixed view height getting all messed up in some situations
  - Fixed crowbar's legacy attack not working
  - Fixed auto-duck wrongly ducking player on sloped surfaces
  - Implemented renderer optimization for people who see predominantly or only with one eye
  - Several performance optimizations (see commit logs in source code for details)

##### HLVRConfig:
  - Reorganized HLVRConfig tabs:
    - "Input" split into "Input" and "Immersion"
    - "Graphics" split into "Visuals" and "Performance"
  - Added option to enable rendering to only one eye (left or right)

---
#### 0.6.16-beta - 2020-03-14

##### Mod:
  - Fixed performance issues in custom random number generator

##### HLVRConfig:
  - No changes

---
#### 0.6.15-beta - 2020-03-13

##### Mod:
  - Fixed copypaste error causing gl_texturemode to be set to invalid value in classic mode
  - Fixed false positive error message when grabbing brush based entities

##### HLVRConfig:
  - No changes

---
#### 0.6.14-beta - 2020-03-09

##### Mod:
  - Limit error message for missing or broken HUD textures to be printed once instead of every frame
  - Added missing textures for train speed menu

##### HLVRConfig:
  - No changes

---
#### 0.6.13-beta - 2020-03-09

##### Mod:
  - Fixed HD textures not loading due bug introduced with gl_texturemode
  - Breakables spawn more gibs when melee'd by player, less otherwise
  - Default maximum number of gibs in a level reduced to 50 from 128

##### HLVRConfig:
  - Players can configure maximum number of gibs in a level

---
#### 0.6.12-beta - 2020-03-08

##### Mod:
  - Reduced number of grabbable gibs spawned by breakables
  - Set maximum number of total gibs in a level to 128

##### HLVRConfig:
  - No changes

---
#### 0.6.11-beta - 2020-03-07

##### Mod:
  - Fixed issue with ladders introduced with last update
  - Fixed hitboxes on hand models
  - Fixed weapon scale not working
  - Fixed minor issue with virtual backpack if player reselects weapon the usual way

##### HLVRConfig:
  - Fixed "Number Of Displayed Log Lines" label wrongly displaying "Language"

---
#### 0.6.10-beta - 2020-03-06

##### Mod:
  - Fixed playback of long sentences
  - Fixed NPCs not reacting to being held at gunpoint
  - Fixed grab behaving like a "magnet"
  - Fixed interaction with triggers that aren't simple boxes
  - Fixed held objects not being rendered correctly
  - HD textures now respect value of gl_texturemode
  - Classic mode sets gl_texturemode to GL_NEAREST for that crisp clear pixelated 1998 look

##### HLVRConfig:
  - Added option to disable NPCs reacting to being held at gunpoint
  - Added option to chose if NPCs being held at gunpoint for too long turn hostile or not
  - gl_texturemode can now be set in HLVRConfig

---
#### 0.6.9-beta - 2020-03-04

##### Mod:
  - Fixed crash introduced in 0.6.8-beta due to gibs being spawned without valid model
  - Fixed some bugs with height offsets
  - Fixed HUD train speed display
  - Fixed stuff lagging behind controller when being picked up
  - Stuff being held (debris, gibs etc) now interacts with buttons and other things
  - NPCs now react when thrown at with gibs
  - Implemented VR controls for usable trains
  - Added option to automatically stop trains when falling/jumping off
  - Implemented haptic feedback

##### HLVRConfig:
  - No changes

---
#### 0.6.8-beta - 2020-03-01

##### Mod:
  - Fixed bug that pushed players upwards when walking into walls
  - Fixed labcoat hand model animations and misaligned UV maps
  - Added automatic re-focus of HL window when playing to fix performance issues when HL window is not in focus
  - Soda cans can now be picked up and consumed by moving them close to one's face. They can also be thrown.
  - Thrown grenades can now be picked up and thrown again (with the risk of them exploding in your hand)
  - Live snarks can be picked up and are then turned back into a player weapon
  - Added option to allow dragging and grabbing stuff only with bare hands (not with a weapon)
  - Most small entities like wood pieces, glass shards, fleshy gibs etc. can be picked up, thrown, and pushed with hands/weapons

##### HLVRConfig:
  - Fixed HL console output not being displayed
  - Added option to set maximum lines to display in console log and added timestamps to log
  - Hide settings that depend on another setting, if that other setting is disabled
  - Show default values for settings
  - All distances are now in the same units (previously some where ingame units, some where meters, some where cm)
  - Removed "smooth steps" option from HLVRConfig as the feature is broken

---
#### 0.6.7-beta - 2020-02-20

- Fixed hand/weapon models and HUD not being rendered in new "normal" render mode

---
#### 0.6.6-beta - 2020-02-19

- Fixed bug that caused the game to replace textures with HD textures every frame
- Added option to switch between normal rendering and displaylist based rendering
- Removed view of current VR display in HL window

---
#### 0.6.5-beta - 2020-02-17

- Replaced HLVRLauncher with HLVRConfig
  - Added i18n support to HLVRConfig
  - General improvement of error feedback and user interface
  - Support for custom Half-Life installations that HLVRLauncher couldn't detect
  - Added movement speed settings
- Replaced json based config file with simple keyvalue file, and split into 2 files (mod settings and settings for HLVRConfig)
- Added support for selecting any installed language for speech recognition
- Added support for unicode speech commands
- Added input action for switching between walking and running (can be configured as toggle or hold)

---
#### 0.6.4-beta - 2020-01-26

- Fixed HLVRLauncher still checking if opengl32.dll is present
- Fixed wrong default value for engine fps
- Fixed render bug where objects wouldn't visually move
- Fixed controllers/hands wrongly trigger some things at distance
- Fixed bug in physics data loading/saving that made the mod regenerate physics data everytime
- Improved headset fps config to allow any fps value
- Renamed third party libraries and patched client.dll to import from vr directory (makes copying dlls to Half-Life directory obsolete)
- Added FMOD support, including 3d occlusion based on actual map
- Improved HLVRLauncher interface
- Added option for HMD offset (vr_headset_offset) to accommodate people of varying body sizes and/or for seated people
- Added classic mode (unmodified 1998 models and textures)

---
#### 0.6.3-beta - 2020-01-08

- Fixed jitter bugs on hand and weapon models
- Fixed glitchy finger tracking
- Fixed left hand model being slightly offset
- Fixed controllers being wrongly switched in left hand mode
- Fixed ammo-pickup bug
- Fixed some issues with grabbing when finger tracking is enabled
- Fixed view height issues
- Fixed fps lock that was set to exactly 90; now is set to at least 90 (for the 144hz folk out there)
- Fixed VR teleporter wrongly identifying locations in corners or close to walls as invalid teleporter targets
- Fixed doors and other objects not reacting to players moving into them when walking IRL
- Fixed 90° snap rotations being inverted
- Added 45° snap rotation
- Reworked render cycle and greatly improved performance
- Added user-friendly config for movement direction
- Added sanity check for physics data loaded from files
- Removed now obsolete opengl32.dll

---
#### 0.6.2-beta - 2019-12-24

- No game changes
- Updated README and released publicly

---
#### 0.6.1-beta - 2019-11-21

- Fixed several crash bugs and improved general code stability
- Fixed bug that broke moving with trains and other entities
- Fixed "invalid decompressed idat size" error for HD textures that aren't 1:1
- Disabled HD sky due to clipping issues (will be fixed later)

---
#### 0.6.0-beta - 2019-11-05
- Fixed several crash bugs and improved general code stability
- Implemented version checks for loading a savegame (prevents loading savegames from vanilla HL, which aren't compatible)
- Added workaround for bug in engine introduced with a Half-Life update on 2019-10-25 (see https://github.com/ValveSoftware/halflife/issues/2758)

---
#### 0.5.9-beta - 2019-11-03

- Fixed crash bug when engine feeds client update methods with invalid edict
- Fixed controllers activating stuff through walls
- Fixed weapons being able to shoot when controller is put through wall
- Added options for non-solid levers and mounted guns (fixes collision issues)
- Added missing skybox textures and removed obsolete city skybox
- Added option to smooth out movement on stairs
- Removed OLDHLVRLauncher

---
#### 0.5.8-beta - 2019-10-25

- Added immersive stationary gun (func_tank) controls: Grab and rotate gun with both controllers, shoot with "fire"
- "Fixed" broken infinite health door exploit (default disabled, added cheat to enable exploit)
- VR specific cheats now only work if sv_cheats is 1
- Fixed minor issue with HD sky rendering when OpenGL has an error state from something before
- Fixed tracked fingers animation being reversed
- Fixed HLVRLauncher dying on long console output from Half-Life and added log to file

---
#### 0.5.7-beta - 2019-10-24

- Fixed error due to weapon models with animated movement not being cached
- Fixed bugs in finger skeleton animation
- Fixed several issues with auto-crouch on sloped surfaces
- Added cvar vr_autocrouch_enabled

---
#### 0.5.6-beta - 2019-10-23

- Fixed animation glitch while interpolating between flat and curled fingers
- Fixed copypaste arror in action.manifest mapping right skeleton action to left skeleton input
- Fixed copypaste error in code overriding right hand with skeletal input from any hand

---
#### 0.5.5-beta - 2019-10-23

- Reduced options for vr_multipass_mode to true display list and mixed display list
- Added support for finger/hand skeleton input and corresponding cvar vr_min_fingercurl_for_grab
- Updated version scheme to include "beta" (all versions have been alpha or beta so far, but the version numbers didn't express this)
- Added "About" section that displays contents of README.txt to HLVRLauncher
- Added credits for betatesters, patreons and third party helpers to README.txt
- Added release date to README.txt and clarified some things

---
#### 0.5.4 - 2019-10-20

- Fixed some objects and traps not killing the player even though they should
- Fixed crash when trying to load texture that doesn't exist
- Fixed deploy animation in 9mmAR weapon model without animated movement

---
#### 0.5.3 - 2019-10-19

- Fixed some input actions not working anymore due to change in SteamVR beta 1.7.7 (see https://github.com/ValveSoftware/SteamVR-for-Linux/issues/236)
- Improved weapon animations and split weapon models into set with animated movement and without
- Added cvar vr_use_animated_weapons and corresponding config in HLVRLauncher (default off)

---
#### 0.5.2 - 2019-09-15

- Fixed scientists and barneys becoming non-solid and non-interactable after switching SD/HD models
- Fixed jumping out of water (was broken by auto-crouch)
- Fixed boxes being pushable while standing on them
- Fixed several rotation related bugs when spawning/teleporting/loading/levelchanging would position the player at an offset from the actual position
- Fixed HUD not being displayed properly in displaylist multipass mode
- Fixed HD textures not working in displaylist multipass mode
- Added extra sound groups with fitting audio files for scientists and barneys for responding to speech recognized greeting
- Added cvar vr_multipass_mode and corresponding config in HLVRLauncher

---
#### 0.5.1 - 2019-07-30

- Fixed "telekinetic" grab
- Fixed bug in pushable push code
- Fixed getting stuck on ladders when legacy ladder movement is disabled
- Fixed minor bug in speed setting for those fast platforms near the tentacle monster
- Fixed HD textures not working since 0.5.0-hotfix
- Fixed movement vector being upside down when using HMD as movement input direction (HMD input is already correct, but fix from 0.4.0 inverted it making it wrong again)
- Replaced cvar vr_use_sd_models with vr_use_hd_models
- Removed support for legacy input (mod now only supports SteamVR Input)
- Improved error feedback for missing SteamVR Input actions
- Implemented config tabs in HLVRLauncher
- Improved legacy ladder movement: Forward/backward movement only affects movement in z axis, sideways movement only affects movement in x and y axis
- Implemented HD skybox
- Added new input action "LetGoOffLadder" for legacy ladder climbing. When used, player will simply drop from current ladder, instead of being catapulted away as when jumping
- Added cvars vr_ledge_pull_mode (0=disabled, 1=move, 2=instant) and vr_ledge_pull_speed
- Implemented ledge pulling (grab ledge with both controllers and pull down to pull yourself onto the ledge)
- Implemented auto crouch
- Added speech recognition for using voice commands on NPCs ("follow me", "wait", "hello")
- Added cvars vr_speech_commands_enabled, vr_speech_commands_follow, vr_speech_commands_wait and vr_speech_commands_hello
- Teleporting now creates noise that AI can react to (the further the teleport distance, the "louder" the noise and thus greater attention from AI)

---
#### 0.5.0-hotfix - 2019-07-26

- Fixed performance issues that came with changes in opengl32.dll in 0.4.4
- Fixed issues with sparks and beam effects being not in sync on left and right eye
- Improved rendering performance overall

---
#### 0.5.0 - 2019-07-26

- Fixed HMD display blackscreen issue with AMD drivers 18.8.1 and up
- Fixed weapon animations not playing properly
- Fixed some rotating levers not working with grab input
- Fixed some buttons not working with touch input
- Fixed screen fade effects rendering
- Fixed some scripts using male voice for female scientists
- Added better error handling and debug output for OpenGL and OpenVR calls
- Implemented proper grab animation for hands
- Display flashlight submodel on hand model if flashlight is attached to controller
- Added cvar "vr_ladder_immersive_movement_swinging_enabled", if enabled players can push/swing from ladders (controller speed is translated onto player when letting go of ladders)
- Replaced old command line HLVRLauncher with new WPF based GUI HLVRLauncher
  - Adds interfaces for patching/unpatching Half-Life
  - Detects Half-Life installation directory using Steam library
  - Detects running Half-Life instances
  - Logs and prints Half-Life console output
  - Proper error messages and user feedback
  - Patching time is in the milliseconds (old HLVRLauncher took several seconds)
  - opengl32.dll is locked using a read handle instead of ACL/file properties

---
#### 0.4.9 - 2019-07-23

- Fixed bug where gibs (e.g. wood from broken crates) are dramatically oversized (was supposed to be fixed in 0.4.0, but the fix had a bug)
- Fixed players falling through floor after using VR teleport (was supposed to be fixed in 0.4.7, but the fix only addressed half of the problem)
- Fixed issues with pushable boxes that don't have a proper origin
- Renamed cvars "vr_ladder_movement_speed" and "vr_ladder_movement_only_updown" to "vr_ladder_legacy_movement_speed" and "vr_ladder_legacy_movement_only_updown" respectively
- Added cvars "vr_ladder_legacy_movement_enabled" and "vr_ladder_immersive_movement_enabled"
- Added cvar "vr_legacy_train_controls_enabled"
- Removed "Grab" action and added "LeftGrab" and "RightGrab"
- Removed outdated code that supports HUD sprites for resolutions below 640x480 (fixes issues with WMR)
- Added immersive ladder climbing: Grab ladders with controllers and pull up or down to move
- Added visual damage feedback in form of pulsating gradient overlay (dark blue for drowning, light blue for freezing, green for poison/radiation, red for every other damage)

---
#### 0.4.8 - 2019-07-15

- Fixed a crashbug that came with the rotating door fix in 0.4.7
- Fixed key states not being reset when dying (caused weapons to fire after respawn)
- Fixed map-spawned tripmines floating 8 units away from walls
- Fixed and improved collision/overlap detection between hands/weapons and world/objects
- Fixed "Mod_Extradata: caching failed" crashes
- Cleaned up and improved hit boxes on hand and weapon models
- Added labcoat hand model for predisaster maps
- Finished all SD models except female scientist
- Improved pushable boxes behavior
- Updated reactphysics3d to 0.7.1
- Added EasyHook32 library for fixing "Mod_Extradata: caching failed" crashes

---
#### 0.4.7 - 2019-07-11

- Fixed players falling through floor after using VR teleport
- Fixed rotatable buttons (levers, valves etc.) from losing grip or rotating back too early
- Fixed thrown items (grenades, satchels, snarks) being "hit" by melee attack right after they spawn from the player's controller
- Fixed rotating doors pulling the player due to false positive in ground entity detection
- Added relative speed check to prevent objects being "hit" by melee attack when speed of controller and object are relatively the same
- Melee attack damage is now calculated with relative speed between controller and object (more damage when hitting objects flying towards the controller)
- Hitting headcrabs with melee attack pushes them (headcrab baseball anyone?)
- Added missing animations to female scientist model
- HUD damage feedback is now red and a bit further up
- Added cvars vr_flashlight_attachment and vr_teleport_attachment (modes are: use hand, use weapon, use head, use pose)
- Made flashlight and teleporter use hand and weapon model attachments for position and direction

---
#### 0.4.6 - 2019-06-16

- Fixed bug that disabled SteamVR input

---
#### 0.4.5 - 2019-06-16

- Fixed some breakables breaking immediately when touched
- Fixed player not ducking after teleporting into ducts and other areas that require ducking
- If there was an easter egg, which there definitely isn't, a bug related to that easter egg would have been fixed. But since there is no easter egg, there was no bug and there is no fix, obviously.
- Made HLVRLauncher set opengl32.dll to read-only (in addition to the ACL settings)
- Restored original player hullsize (unfortunately hullsizes are compiled into bsp files, and thus changing them in code causes bugs)
- Added cvars vr_ladder_movement_only_updown and vr_ladder_movement_speed for improved ladder movement
- Added cvar vr_view_dist_to_walls
- Added laser spot to range weapons (except hornet gun) and corresponding cvar vr_enable_aim_laser

---
#### 0.4.4 - 2019-06-02

- Fixed player position after levelchange and save/reload
- Fixed player orientation after levelchange and save/reload
- Fixed crash bug when accessing engine pmove code before map has finished loading
- Fixed issues with opengl32.dll with some drivers

---
#### 0.4.3 - 2019-06-01

- Fixed violent up/down jittering when on platforms that move vertically
- Reduced time till auto-reload after death to 1 second (from 3 seconds)

---
#### 0.4.2 - 2019-06-01

- Fixed items not being picked up by controllers
- Fixed cvar vr_rl_duck_height being interpreted as m instead of cm
- Fixed view height when ducking
- Fixed several cvars not keeping their value between game sessions
- Fixed HLVRLauncher not working in directories with spaces
- Fixed HLVRLauncher not removing inherited ACLs from Opengl32.dll
- Fixed HLVRLauncher not properly deleting/backuping/restoring dll files
- Capped view height at ingame height to avoid clipping issues
- Made xen_plantlight react to touch by controllers
- Added new cvar "vr_use_sd_models"
- Removed "-condebug" and "-dev" from launcher to prevent crashbug in engine (see https://github.com/ValveSoftware/halflife/issues/2492)

---
#### 0.4.1 - 2019-05-30

- Fixed a bug where teleporting on some ladders wasn't possible
- Fixed player hull sizes not being properly updated in movement code
- Improved detection of groundentities (e.g. moving platforms in lambda core)
- Corrected type of new analog jump, crouch and longjump actions

---
#### 0.4.0 - 2019-05-26

- Fixed crouching between level changes not being detected
- Fixed RL crouching and ingame crouching interfering
- Fixed bug where weapon could be fired while using stationary gun
- Fixed several collision bugs
- Fixed bug where game wouldn't reload after dying
- Fixed player's bounding box preventing the player to get close to things
- Fixed 90° and 180° turn actions causing player to warp through walls
- Fixed bug that prevented loading of physics data cache
- Fixed rare freeze bug when climbing a ladder on a narrow angle due to bug in engine
- Fixed bug where gibs (e.g. wood from broken crates) are dramatically oversized
- Fixed bug where the tripmine laser would randomly attach to other entities after levelchange
- Fixed visual issues due to backface rendering
- Fixed movement vector being upside down (fixes issues in water and on ladders)
- Fixed analog movement not working on ladders
- Fixed several bugs when teleporting on ladders
- Fixed some xen jump thingies not being detected by the teleporter beam
- Fixed ingame cameras not working (that scene where Gordon gets captured)
- Fixed barnacles not causing damage
- Fixed jumping in shallow water not possible
- Fixed upward/downward movement not working in water and on ladders
- Made rotating buttons (valves, levers) actually rotatable (drag and rotate)
- Improved drag behavior of pushables
- Added analog jump, crouch and longjump actions
- Added environmental damage feedback to VR HUD
- Added cvar to control height of RL crouch
- Massive improvements to weapon models
- ESRGAN improved textures on all models

---
#### 0.3.0 - 2019-04-26

- Added error popup message with info why mod can't start
- Fixed bug where teleporting through changelevel didn't load next map
- Added world weapon models with highres textures
- Added hd skyboxes
- Fixed background texture
- Compiled with static MSVC runtime libraries

---
#### 0.2.0 - 2019-04-22

- First internal beta release

---
#### 0.1.0 - 2017-2019

- Internal developer version

---
#### 0.0.0 - 2017

- Inofficial early alpha