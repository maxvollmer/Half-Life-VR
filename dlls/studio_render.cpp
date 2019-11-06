/***
*
*	Copyright (c) 1998, Valve LLC. All rights reserved.
*
*	This product contains software technology licensed from Id
*	Software, Inc. ("Id Technology").  Id Technology (c) 1996 Id Software, Inc.
*	All Rights Reserved.
*
****/
// studio_render.cpp: routines for drawing Half-Life 3DStudio models
// updates:
// 1-4-99	fixed AdvanceFrame wraping bug

// Modified to compile with hl.dll - Max Vollmer - 2019-04-07

#include <cstring>

#include "StudioModel.h"

#pragma warning(disable : 4244)  // double to float


void StudioModel::CalcBoneAdj()
{
	int i, j;
	float value = 0.f;
	mstudiobonecontroller_t* pbonecontroller = reinterpret_cast<mstudiobonecontroller_t*>(reinterpret_cast<byte*>(m_pstudiohdr) + m_pstudiohdr->bonecontrollerindex);

	for (j = 0; j < m_pstudiohdr->numbonecontrollers; j++)
	{
		i = pbonecontroller[j].index;
		if (i <= 3)
		{
			// check for 360% wrapping
			if (pbonecontroller[j].type & STUDIO_RLOOP)
			{
				value = m_controller[i] * (360.f / 256.f) + pbonecontroller[j].start;
			}
			else
			{
				value = m_controller[i] / 255.f;
				if (value < 0.f)
					value = 0.f;
				if (value > 1.f)
					value = 1.f;
				value = (1.f - value) * pbonecontroller[j].start + value * pbonecontroller[j].end;
			}
			// Con_DPrintf( "%d %d %f : %f\n", m_controller[j], m_prevcontroller[j], value, dadt );
		}
		else
		{
			value = m_mouth / 64.f;
			if (value > 1.f)
				value = 1.f;
			value = (1.f - value) * pbonecontroller[j].start + value * pbonecontroller[j].end;
			// Con_DPrintf("%d %f\n", mouthopen, value );
		}
		switch (pbonecontroller[j].type & STUDIO_TYPES)
		{
		case STUDIO_XR:
		case STUDIO_YR:
		case STUDIO_ZR:
			m_adj[j] = value * (M_PI / 180.f);
			break;
		case STUDIO_X:
		case STUDIO_Y:
		case STUDIO_Z:
			m_adj[j] = value;
			break;
		}
	}
}


void StudioModel::CalcBoneQuaternion(int frame, float s, mstudiobone_t* pbone, mstudioanim_t* panim, float* q)
{
	int j, k;
	vec4_t q1, q2;
	vec3_t angle1, angle2;
	mstudioanimvalue_t* panimvalue;

	for (j = 0; j < 3; j++)
	{
		if (panim->offset[j + 3] == 0)
		{
			angle2[j] = angle1[j] = pbone->value[j + 3];  // default;
		}
		else
		{
			panimvalue = reinterpret_cast<mstudioanimvalue_t*>(reinterpret_cast<byte*>(panim) + panim->offset[j + 3]);
			k = frame;
			while (panimvalue->num.total <= k)
			{
				k -= panimvalue->num.total;
				panimvalue += panimvalue->num.valid + 1;
			}
			// Bah, missing blend!
			if (panimvalue->num.valid > k)
			{
				angle1[j] = panimvalue[k + 1].value;

				if (panimvalue->num.valid > k + 1)
				{
					angle2[j] = panimvalue[k + 2].value;
				}
				else
				{
					if (panimvalue->num.total > k + 1)
						angle2[j] = angle1[j];
					else
						angle2[j] = panimvalue[panimvalue->num.valid + 2].value;
				}
			}
			else
			{
				angle1[j] = panimvalue[panimvalue->num.valid].value;
				if (panimvalue->num.total > k + 1)
				{
					angle2[j] = angle1[j];
				}
				else
				{
					angle2[j] = panimvalue[panimvalue->num.valid + 2].value;
				}
			}
			angle1[j] = pbone->value[j + 3] + angle1[j] * pbone->scale[j + 3];
			angle2[j] = pbone->value[j + 3] + angle2[j] * pbone->scale[j + 3];
		}

		if (pbone->bonecontroller[j + 3] != -1)
		{
			angle1[j] += m_adj[pbone->bonecontroller[j + 3]];
			angle2[j] += m_adj[pbone->bonecontroller[j + 3]];
		}
	}

	if (!VectorCompare(angle1, angle2))
	{
		AngleQuaternion(angle1, q1);
		AngleQuaternion(angle2, q2);
		QuaternionSlerp(q1, q2, s, q);
	}
	else
	{
		AngleQuaternion(angle1, q);
	}
}


void StudioModel::CalcBonePosition(int frame, float s, mstudiobone_t* pbone, mstudioanim_t* panim, float* pos)
{
	int j, k;
	mstudioanimvalue_t* panimvalue;

	for (j = 0; j < 3; j++)
	{
		pos[j] = pbone->value[j];  // default;
		if (panim->offset[j] != 0)
		{
			panimvalue = reinterpret_cast<mstudioanimvalue_t*>(reinterpret_cast<byte*>(panim) + panim->offset[j]);

			k = frame;
			// find span of values that includes the frame we want
			while (panimvalue->num.total <= k)
			{
				k -= panimvalue->num.total;
				panimvalue += panimvalue->num.valid + 1;
			}
			// if we're inside the span
			if (panimvalue->num.valid > k)
			{
				// and there's more data in the span
				if (panimvalue->num.valid > k + 1)
				{
					pos[j] += (panimvalue[k + 1].value * (1.f - s) + s * panimvalue[k + 2].value) * pbone->scale[j];
				}
				else
				{
					pos[j] += panimvalue[k + 1].value * pbone->scale[j];
				}
			}
			else
			{
				// are we at the end of the repeating values section and there's another section with data?
				if (panimvalue->num.total <= k + 1)
				{
					pos[j] += (panimvalue[panimvalue->num.valid].value * (1.f - s) + s * panimvalue[panimvalue->num.valid + 2].value) * pbone->scale[j];
				}
				else
				{
					pos[j] += panimvalue[panimvalue->num.valid].value * pbone->scale[j];
				}
			}
		}
		if (pbone->bonecontroller[j] != -1)
		{
			pos[j] += m_adj[pbone->bonecontroller[j]];
		}
	}
}


void StudioModel::CalcRotations(vec3_t* pos, vec4_t* q, mstudioseqdesc_t* pseqdesc, mstudioanim_t* panim, float f)
{
	int i = 0;
	int frame = 0;
	mstudiobone_t* pbone;
	float s = 0.f;

	frame = (int)f;
	s = (f - frame);

	// add in programatic controllers
	CalcBoneAdj();

	pbone = reinterpret_cast<mstudiobone_t*>(reinterpret_cast<byte*>(m_pstudiohdr) + m_pstudiohdr->boneindex);
	for (i = 0; i < m_pstudiohdr->numbones; i++, pbone++, panim++)
	{
		CalcBoneQuaternion(frame, s, pbone, panim, q[i]);
		CalcBonePosition(frame, s, pbone, panim, pos[i]);
	}

	if (pseqdesc->motiontype & STUDIO_X)
		pos[pseqdesc->motionbone][0] = 0.0;
	if (pseqdesc->motiontype & STUDIO_Y)
		pos[pseqdesc->motionbone][1] = 0.0;
	if (pseqdesc->motiontype & STUDIO_Z)
		pos[pseqdesc->motionbone][2] = 0.0;
}


mstudioanim_t* StudioModel::GetAnim(mstudioseqdesc_t* pseqdesc)
{
	mstudioseqgroup_t* pseqgroup = reinterpret_cast<mstudioseqgroup_t*>(reinterpret_cast<byte*>(m_pstudiohdr) + m_pstudiohdr->seqgroupindex) + pseqdesc->seqgroup;

	if (pseqdesc->seqgroup == 0)
	{
		return reinterpret_cast<mstudioanim_t*>(reinterpret_cast<byte*>(m_pstudiohdr) + pseqgroup->data + pseqdesc->animindex);
	}

	return reinterpret_cast<mstudioanim_t*>(reinterpret_cast<byte*>(m_panimhdr[pseqdesc->seqgroup]) + pseqdesc->animindex);
}


void StudioModel::SlerpBones(vec4_t q1[], vec3_t pos1[], vec4_t q2[], vec3_t pos2[], float s)
{
	int i = 0;
	vec4_t q3;
	float s1 = 0.f;

	if (s < 0)
		s = 0;
	else if (s > 1.0)
		s = 1.0;

	s1 = 1.0 - s;

	for (i = 0; i < m_pstudiohdr->numbones; i++)
	{
		QuaternionSlerp(q1[i], q2[i], s, q3);
		q1[i][0] = q3[0];
		q1[i][1] = q3[1];
		q1[i][2] = q3[2];
		q1[i][3] = q3[3];
		pos1[i][0] = pos1[i][0] * s1 + pos2[i][0] * s;
		pos1[i][1] = pos1[i][1] * s1 + pos2[i][1] * s;
		pos1[i][2] = pos1[i][2] * s1 + pos2[i][2] * s;
	}
}


void StudioModel::AdvanceFrame(float dt)
{
	if (!m_pstudiohdr)
		return;

	mstudioseqdesc_t* pseqdesc;
	pseqdesc = reinterpret_cast<mstudioseqdesc_t*>(reinterpret_cast<byte*>(m_pstudiohdr) + m_pstudiohdr->seqindex) + m_sequence;

	if (dt > 0.1)
		dt = 0.1f;
	m_frame += dt * pseqdesc->fps;

	if (pseqdesc->numframes <= 1)
	{
		m_frame = 0;
	}
	else
	{
		// wrap
		m_frame -= (int)(m_frame / (pseqdesc->numframes - 1)) * (pseqdesc->numframes - 1);
	}
}


int StudioModel::SetFrame(int nFrame)
{
	if (nFrame == -1)
		return m_frame;

	if (!m_pstudiohdr)
		return 0;

	mstudioseqdesc_t* pseqdesc;
	pseqdesc = reinterpret_cast<mstudioseqdesc_t*>(reinterpret_cast<byte*>(m_pstudiohdr) + m_pstudiohdr->seqindex) + m_sequence;

	m_frame = nFrame;

	if (pseqdesc->numframes <= 1)
	{
		m_frame = 0;
	}
	else
	{
		// wrap
		m_frame -= (int)(m_frame / (pseqdesc->numframes - 1)) * (pseqdesc->numframes - 1);
	}

	return m_frame;
}


void StudioModel::SetUpBones(void)
{
	int i = 0;

	mstudiobone_t* pbones;
	mstudioseqdesc_t* pseqdesc;
	mstudioanim_t* panim;

	static vec3_t pos[MAXSTUDIOBONES];
	float bonematrix[3][4];
	static vec4_t q[MAXSTUDIOBONES];

	static vec3_t pos2[MAXSTUDIOBONES];
	static vec4_t q2[MAXSTUDIOBONES];
	static vec3_t pos3[MAXSTUDIOBONES];
	static vec4_t q3[MAXSTUDIOBONES];
	static vec3_t pos4[MAXSTUDIOBONES];
	static vec4_t q4[MAXSTUDIOBONES];


	if (m_sequence >= m_pstudiohdr->numseq)
	{
		m_sequence = 0;
	}

	pseqdesc = reinterpret_cast<mstudioseqdesc_t*>(reinterpret_cast<byte*>(m_pstudiohdr) + m_pstudiohdr->seqindex) + m_sequence;

	panim = GetAnim(pseqdesc);
	CalcRotations(pos, q, pseqdesc, panim, m_frame);

	if (pseqdesc->numblends > 1)
	{
		float s = 0.f;

		panim += m_pstudiohdr->numbones;
		CalcRotations(pos2, q2, pseqdesc, panim, m_frame);
		s = m_blending[0] / 255.0;

		SlerpBones(q, pos, q2, pos2, s);

		if (pseqdesc->numblends == 4)
		{
			panim += m_pstudiohdr->numbones;
			CalcRotations(pos3, q3, pseqdesc, panim, m_frame);

			panim += m_pstudiohdr->numbones;
			CalcRotations(pos4, q4, pseqdesc, panim, m_frame);

			s = m_blending[0] / 255.0;
			SlerpBones(q3, pos3, q4, pos4, s);

			s = m_blending[1] / 255.0;
			SlerpBones(q, pos, q3, pos3, s);
		}
	}

	pbones = reinterpret_cast<mstudiobone_t*>(reinterpret_cast<byte*>(m_pstudiohdr) + m_pstudiohdr->boneindex);

	for (i = 0; i < m_pstudiohdr->numbones; i++)
	{
		QuaternionMatrix(q[i], bonematrix);

		bonematrix[0][3] = pos[i][0];
		bonematrix[1][3] = pos[i][1];
		bonematrix[2][3] = pos[i][2];

		if (pbones[i].parent == -1)
		{
			std::memcpy(m_bonetransform[i], bonematrix, sizeof(float) * 12);
		}
		else
		{
			R_ConcatTransforms(m_bonetransform[pbones[i].parent], bonematrix, m_bonetransform[i]);
		}
	}
}

void StudioModel::SetupModel(int bodypart)
{
	int index = 0;

	if (bodypart > m_pstudiohdr->numbodyparts)
	{
		// Con_DPrintf ("StudioModel::SetupModel: no such bodypart %d\n", bodypart);
		bodypart = 0;
	}

	mstudiobodyparts_t* pbodypart = reinterpret_cast<mstudiobodyparts_t*>(reinterpret_cast<byte*>(m_pstudiohdr) + m_pstudiohdr->bodypartindex) + bodypart;

	index = m_bodynum / pbodypart->base;
	index = index % pbodypart->nummodels;

	m_pmodel = reinterpret_cast<mstudiomodel_t*>(reinterpret_cast<byte*>(m_pstudiohdr) + pbodypart->modelindex) + index;
}
