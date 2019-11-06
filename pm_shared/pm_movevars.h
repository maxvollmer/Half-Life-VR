//========= Copyright � 1996-2002, Valve LLC, All rights reserved. ============
//
// Purpose:
//
// $NoKeywords: $
//=============================================================================

// pm_movevars.h
#if !defined(PM_MOVEVARSH)
#define PM_MOVEVARSH

// movevars_t                  // Physics variables.
typedef struct movevars_s movevars_t;

struct movevars_s
{
	float gravity = 0.f;    // Gravity for map
	float stopspeed = 0.f;  // Deceleration when not moving
	float maxspeed = 0.f;   // Max allowed speed
	float spectatormaxspeed = 0.f;
	float accelerate = 0.f;       // Acceleration factor
	float airaccelerate = 0.f;    // Same for when in open air
	float wateraccelerate = 0.f;  // Same for when in water
	float friction = 0.f;
	float edgefriction = 0.f;   // Extra friction near dropofs
	float waterfriction = 0.f;  // Less in water
	float entgravity = 0.f;     // 1.0
	float bounce = 0.f;         // Wall bounce value. 1.0
	float stepsize = 0.f;       // sv_stepsize;
	float maxvelocity = 0.f;    // maximum server velocity.
	float zmax = 0.f;           // Max z-buffer range (for GL)
	float waveHeight = 0.f;     // Water wave height (for GL)
	qboolean footsteps;   // Play footstep sounds
	char skyName[32];     // Name of the sky map
	float rollangle = 0.f;
	float rollspeed = 0.f;
	float skycolor_r = 0.f;  // Sky color
	float skycolor_g = 0.f;  //
	float skycolor_b = 0.f;  //
	float skyvec_x = 0.f;    // Sky vector
	float skyvec_y = 0.f;    //
	float skyvec_z = 0.f;    //
};

extern movevars_t movevars;

#endif
