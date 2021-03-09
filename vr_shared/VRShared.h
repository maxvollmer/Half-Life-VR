#pragma once


enum class VRPoseType
{
	FLASHLIGHT,
	MOVEMENT,
	TELEPORTER
};

enum class VRSpeechCommand
{
	NONE,
	FOLLOW,  // triggers follow command if looking at friendly NPC that is not following
	WAIT,    // triggers unfollow command if looking at friendly NPC that is following
	HELLO,   // triggers greeting response if looking at friendly NPC
	MUMBLE   // triggers random response if looking at friendly NPC
};

enum class VREyeMode
{
	Both,
	LeftOnly,			//  Left eye is rendered, right eye is black
	RightOnly,			// Right eye is rendered,  left eye is black
	LeftOnlyMirrored,	//  Left eye is rendered, right eye is mirror of left eye
	RightOnlyMirrored,	// Right eye is rendered,  left eye is mirror of right eye
	PancakeMode
};
