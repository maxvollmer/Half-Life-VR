/***
*
*	Copyright (c) 1998, Valve LLC. All rights reserved.
*
*	This product contains software technology licensed from Id
*	Software, Inc. ("Id Technology").  Id Technology (c) 1996 Id Software, Inc.
*	All Rights Reserved.
*
****/
// updates:
// 1-4-99	fixed file texture load and file read bug

// Modified to compile with hl.dll - Max Vollmer - 2019-04-07

////////////////////////////////////////////////////////////////////////

#include <vector>
#include <string>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "StudioModel.h"

#pragma warning(disable : 4244)  // double to float

void StudioModel::FreeModel()
{
	if (m_pstudiohdr)
		free(m_pstudiohdr);

	m_pstudiohdr = 0;

	int i = 0;
	for (i = 0; i < 32; i++)
	{
		if (m_panimhdr[i])
		{
			free(m_panimhdr[i]);
			m_panimhdr[i] = 0;
		}
	}
}

studiohdr_t* StudioModel::LoadModel(const char* modelname)
{
	if (!modelname)
		return 0;

	// load the model
	FILE* fp = nullptr;
	fopen_s(&fp, modelname, "rb");
	if (!fp)
		return 0;

	fseek(fp, 0, SEEK_END);
	long size = ftell(fp);
	fseek(fp, 0, SEEK_SET);

	std::vector<char> buffer;
	buffer.resize(size);

	fread(buffer.data(), size, 1, fp);
	fclose(fp);

	if (strncmp(buffer.data(), "IDST", 4) && strncmp(buffer.data(), "IDSQ", 4))
	{
		return 0;
	}

	if (!strncmp(buffer.data(), "IDSQ", 4) && !m_pstudiohdr)
	{
		return 0;
	}

	// UNDONE: free texture memory

	studiohdr_t* phdr = reinterpret_cast<studiohdr_t*>(buffer.data());

	if (!m_pstudiohdr)
	{
		m_pstudiohdr = phdr;
	}

	return phdr;
}



bool StudioModel::PostLoadModel(const char* modelname)
{
	// preload animations
	if (m_pstudiohdr->numseqgroups > 1)
	{
		for (int i = 1; i < m_pstudiohdr->numseqgroups; i++)
		{
			std::string seqgroupname = modelname;
			seqgroupname = seqgroupname.substr(0, seqgroupname.size() - 4) + std::to_string(i) + ".mdl";
			m_panimhdr[i] = LoadModel(seqgroupname.data());
			if (!m_panimhdr[i])
			{
				FreeModel();
				return false;
			}
		}
	}

	SetSequence(0);

	for (int n = 0; n < m_pstudiohdr->numbodyparts; n++)
	{
		SetBodygroup(n, 0);
	}

	SetSkin(0);

	return true;
}

constexpr const int VR_MAX_VALID_MODEL_SEQUENCE_BBOX_SIZE = 4096;

void StudioModel::ExtractBbox(float* mins, float* maxs)
{
	mstudioseqdesc_t* pseqdesc = reinterpret_cast<mstudioseqdesc_t*>(reinterpret_cast<byte*>(m_pstudiohdr) + m_pstudiohdr->seqindex);

	mins[0] = pseqdesc[m_sequence].bbmin[0];
	mins[1] = pseqdesc[m_sequence].bbmin[1];
	mins[2] = pseqdesc[m_sequence].bbmin[2];

	maxs[0] = pseqdesc[m_sequence].bbmax[0];
	maxs[1] = pseqdesc[m_sequence].bbmax[1];
	maxs[2] = pseqdesc[m_sequence].bbmax[2];

	// Some of our weapons hide parts 9999 units off the model origin (e.g. the RPG rocket when firing)
	// The bounding boxes then are invalid, which we fix by simply taking the bounding box for the idle animation (index 0)
	vec3_t size;
	VectorSubtract(maxs, mins, size);
	if (VectorLength(size) > VR_MAX_VALID_MODEL_SEQUENCE_BBOX_SIZE)
	{
		mins[0] = pseqdesc[0].bbmin[0];
		mins[1] = pseqdesc[0].bbmin[1];
		mins[2] = pseqdesc[0].bbmin[2];

		maxs[0] = pseqdesc[0].bbmax[0];
		maxs[1] = pseqdesc[0].bbmax[1];
		maxs[2] = pseqdesc[0].bbmax[2];
	}
}

void StudioModel::GetSequenceInfo(float* pflFrameRate, float* pflGroundSpeed)
{
	mstudioseqdesc_t* pseqdesc = reinterpret_cast<mstudioseqdesc_t*>(reinterpret_cast<byte*>(m_pstudiohdr) + m_pstudiohdr->seqindex) + (int)m_sequence;

	if (pseqdesc->numframes > 1)
	{
		*pflFrameRate = 256 * pseqdesc->fps / (pseqdesc->numframes - 1);
		*pflGroundSpeed = sqrtf(pseqdesc->linearmovement[0] * pseqdesc->linearmovement[0] + pseqdesc->linearmovement[1] * pseqdesc->linearmovement[1] + pseqdesc->linearmovement[2] * pseqdesc->linearmovement[2]);
		*pflGroundSpeed = *pflGroundSpeed * pseqdesc->fps / (pseqdesc->numframes - 1);
	}
	else
	{
		*pflFrameRate = 256.f;
		*pflGroundSpeed = 0.f;
	}
}

int StudioModel::SetSequence(int iSequence)
{
	if (iSequence > m_pstudiohdr->numseq)
		return m_sequence;

	m_sequence = iSequence;
	m_frame = 0;

	return m_sequence;
}

int StudioModel::SetBodygroup(int iGroup, int iValue)
{
	if (!m_pstudiohdr)
		return 0;

	if (iGroup > m_pstudiohdr->numbodyparts)
		return -1;

	mstudiobodyparts_t* pbodypart = reinterpret_cast<mstudiobodyparts_t*>(reinterpret_cast<byte*>(m_pstudiohdr) + m_pstudiohdr->bodypartindex) + iGroup;

	int iCurrent = (m_bodynum / pbodypart->base) % pbodypart->nummodels;

	if (iValue >= pbodypart->nummodels)
		return iCurrent;

	m_bodynum = (m_bodynum - (iCurrent * pbodypart->base) + (iValue * pbodypart->base));

	return iValue;
}


int StudioModel::SetSkin(int iValue)
{
	if (!m_pstudiohdr)
		return 0;

	if (iValue >= m_pstudiohdr->numskinfamilies)
	{
		return m_skinnum;
	}

	m_skinnum = iValue;

	return iValue;
}

void StudioModel::SetupModel()
{
	if (!m_pstudiohdr)
		return;

	if (m_pstudiohdr->numbodyparts == 0)
		return;

	SetUpBones();

	for (int i = 0; i < m_pstudiohdr->numbodyparts; i++)
	{
		SetupModel(i);
	}
}
