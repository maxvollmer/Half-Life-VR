//========= Copyright � 1996-2002, Valve LLC, All rights reserved. ============
//
// Purpose:
//
// $NoKeywords: $
//=============================================================================

#if !defined(SCREENFADEH)
	#define SCREENFADEH
	#ifdef _WIN32
		#pragma once
	#endif

	#define FFADE_IN       0x0000  // Just here so we don't pass 0 into the function
	#define FFADE_OUT      0x0001  // Fade out (not in)
	#define FFADE_MODULATE 0x0002  // Modulate (don't blend)
	#define FFADE_STAYOUT  0x0004  // ignores the duration, stays faded out until new ScreenFade message received

typedef struct screenfade_s
{
	float fadeSpeed;                      // How fast to fade (tics / second) (+ fade in, - fade out)
	float fadeEnd;                        // When the fading hits maximum
	float fadeTotalEnd;                   // Total End Time of the fade (used for FFADE_OUT)
	float fadeReset;                      // When to reset to not fading (for fadeout and hold)
	byte fader, fadeg, fadeb, fadealpha;  // Fade color
	int fadeFlags;                        // Fading flags
} screenfade_t;

#endif  // !SCREENFADEH
