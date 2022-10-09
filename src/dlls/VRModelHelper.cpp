
// StudioModel helper functions for better collision detection in VR - Max Vollmer - 2019-04-07

#include <vector>
#include <string>

#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "vector.h"

#include "VRModelHelper.h"
#include "StudioModel.h"

extern int ExtractBbox(void* pmodel, int sequence, float* mins, float* maxs);
extern int GetSequenceInfo(void* pmodel, int sequence, float* pflFrameRate, float* pflGroundSpeed, float* pflFPS, int* piNumFrames, bool* pfIsLooping);
extern int GetNumSequences(void* pmodel);

VRModelInfo::SequenceInfo::SequenceInfo() :
	bboxMins{},
	bboxMaxs{},
	bboxRadius{ 0.f },
	framerate{ 0.f },
	numFrames{ 0 },
	isLooping{ false }
{
}

VRModelInfo::SequenceInfo::SequenceInfo(Vector mins, Vector maxs, float framerate, int numFrames, bool isLooping) :
	bboxMins{ mins },
	bboxMaxs{ maxs },
	bboxRadius{ (maxs - mins).Length() * 0.5f },
	framerate{ framerate },
	numFrames{ numFrames },
	isLooping{ isLooping }
{
}

VRModelInfo::VRModelInfo() :
	m_isValid{ false },
	m_hlName{ iStringNull },
	m_name{},
	m_numSequences{ 0 }
{
}

VRModelInfo::VRModelInfo(CBaseEntity* pModel) :
	m_isValid{ false },
	m_hlName{ pModel->pev->model },
	m_name{ STRING(pModel->pev->model) },
	m_numSequences{ 0 }
{
	if (m_hlName && !m_name.empty())
	{
		void* pmodel = GET_MODEL_PTR(pModel->edict());
		if (pmodel)
		{
			m_numSequences = GetNumSequences(pmodel);
			if (m_numSequences > 0)
			{
				for (int sequence = 0; sequence < m_numSequences; sequence++)
				{
					Vector mins;
					Vector maxs;
					float framerate = 0.f;
					int numFrames = 0;
					bool isLooping;
					float dummy = 0.f;
					if (ExtractBbox(pmodel, sequence, mins, maxs) && GetSequenceInfo(pmodel, sequence, &dummy, &dummy, &framerate, &numFrames, &isLooping))
					{
						m_sequences.emplace_back(mins, maxs, framerate, numFrames, isLooping);
					}
					else
					{
						return;
					}
				}
				m_isValid = true;
			}
		}
	}
}


const VRModelInfo& VRModelHelper::GetModelInfo(CBaseEntity* pEntity)
{
	if (!pEntity)
		return VRModelInfo::INVALID;

	auto& info = m_instance.m_modelInfoMap.find(pEntity->pev->model);
	if (info != m_instance.m_modelInfoMap.end())
	{
		if (!info->second.m_isValid)
		{
			info->second = VRModelInfo{ pEntity };
		}
		return info->second;
	}
	else
	{
		return (m_instance.m_modelInfoMap[pEntity->pev->model] = VRModelInfo{ pEntity });
	}
}

/*
std::vector<TransformedBBox> VRModelHelper::GetTransformedBBoxesForModel(CBaseEntity* pModel)
{
	std::vector<TransformedBBox> result;

	const VRModelInfo& modelInfo = GetModelInfo(pModel);

	if (!modelInfo.m_isValid)
		return result;

	// Get studio model
	if (!m_studioModels[modelInfo.m_name])
	{
		m_studioModels[modelInfo.m_name] = std::make_unique<StudioModel>(modelInfo.m_name.data());
	}
	auto& studioModel = m_studioModels[modelInfo.m_name];

	// Extract integer and fractional part of frame
	float frame = 0.f;
	float advanceFrame = modf(pModel->pev->frame, &frame);

	// Setup studio model
	studioModel->SetSequence(pModel->pev->sequence);
	studioModel->SetSkin(pModel->pev->skin);
	studioModel->SetBodygroup(0, pModel->pev->body);
	studioModel->SetFrame(int(frame));
	studioModel->AdvanceFrame(advanceFrame);
	studioModel->SetupModel();

	// Get hitboxes with current transforms
	for (int i = 0, n = studioModel->GetNumHitboxes(); i < n; i++)
	{
		float mins[3];
		float maxs[3];
		float transform[3][4];
		if (studioModel->ExtractTransformedHitbox(i, mins, maxs, transform))
		{
			result.push_back(TransformedBBox{ Vector{mins}, Vector{maxs}, transform });
		}
	}

	return result;
}
*/

/*
bool VRModelHelper::GetAttachment(CBaseEntity *pModel, int attachmentIndex, bool mirrored, Vector& attachment)
{
	const VRModelInfo& modelInfo = GetModelInfo(pModel);

	if (!modelInfo.m_isValid)
		return false;

	// Get studio model
	if (!m_studioModels[modelInfo.m_name])
	{
		m_studioModels[modelInfo.m_name] = std::make_unique<StudioModel>(modelInfo.m_name.data());
	}
	auto& studioModel = m_studioModels[modelInfo.m_name];

	if (attachmentIndex >= 0 && attachmentIndex < studioModel->GetNumAttachments())
	{
		Vector dummy;
		GET_ATTACHMENT(pModel->edict(), attachmentIndex, attachment, dummy);
		return true;
	}
	else
	{
		return false;
	}
}
*/

VRModelHelper VRModelHelper::m_instance{};
const VRModelInfo VRModelInfo::INVALID{};
