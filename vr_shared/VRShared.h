#pragma once


enum class VRSpeechCommand
{
	NONE,
	FOLLOW,  // triggers follow command if looking at friendly NPC that is not following
	WAIT,    // triggers unfollow command if looking at friendly NPC that is following
	HELLO,   // triggers greeting response if looking at friendly NPC
	MUMBLE   // triggers random response if looking at friendly NPC
};
