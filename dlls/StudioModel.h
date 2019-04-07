/***
*
*	Copyright (c) 1998, Valve LLC. All rights reserved.
*
*	This product contains software technology licensed from Id
*	Software, Inc. ("Id Technology").  Id Technology (c) 1996 Id Software, Inc.
*	All Rights Reserved.
*
****/

// Modified to compile with hl.dll - Max Vollmer - 2019-04-07
// Modified to work with engine models and give useful data for VR - Max Vollmer - 2019-04-07

#pragma once

#ifndef INCLUDED_STUDIOMODEL
#define INCLUDED_STUDIOMODEL

#ifndef byte
typedef unsigned char byte;
#endif // byte

#include "hlmv_mathlib.h"
#include "studio.h"

class StudioModel
{
public:
	StudioModel(void* pmodel);
	StudioModel(const char* modelname);
	~StudioModel();

	int		GetNumHitboxes();
	bool	ExtractTransformedHitbox(int boneIndex, float *mins, float *maxs, float(*transform)[4]);

	static const char* GetModelName(void *pmodel);

	int		SetSequence(int iSequence);
	int		SetBodygroup(int iGroup, int iValue);
	int		SetSkin(int iValue);
	int		SetFrame(int nFrame);
	void	AdvanceFrame(float dt);
	void	SetupModel();

	void	GetSequenceInfo(float *pflFrameRate, float *pflGroundSpeed);
	void	ExtractBbox(float *mins, float *maxs);

private:

	studiohdr_t*			LoadModel(const char *modelname);
	bool					PostLoadModel(const char *modelname);
	void					FreeModel();

	// entity settings
	vec3_t					m_origin;
	vec3_t					m_angles;
	int						m_sequence;			// sequence index
	float					m_frame;			// frame
	int						m_bodynum;			// bodypart selection	
	int						m_skinnum;			// skin group selection
	byte					m_controller[4];	// bone controllers
	byte					m_blending[2];		// animation blending
	byte					m_mouth;			// mouth position

	// internal data
	studiohdr_t				*m_pstudiohdr;
	mstudiomodel_t			*m_pmodel;

	studiohdr_t				*m_panimhdr[32];

	vec4_t					m_adj;				// FIX: non persistant, make static

	float					m_bonetransform[MAXSTUDIOBONES][3][4];	// bone transformation matrix

	void					CalcBoneAdj();
	void					CalcBoneQuaternion(int frame, float s, mstudiobone_t *pbone, mstudioanim_t *panim, float *q);
	void					CalcBonePosition(int frame, float s, mstudiobone_t *pbone, mstudioanim_t *panim, float *pos);
	void					CalcRotations(vec3_t *pos, vec4_t *q, mstudioseqdesc_t *pseqdesc, mstudioanim_t *panim, float f);
	mstudioanim_t			*GetAnim(mstudioseqdesc_t *pseqdesc);
	void					SlerpBones(vec4_t q1[], vec3_t pos1[], vec4_t q2[], vec3_t pos2[], float s);
	void					SetUpBones();
	void					SetupModel(int bodypart);
};

#endif // INCLUDED_STUDIOMODEL