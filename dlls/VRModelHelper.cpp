
#include "extdll.h"
#include "util.h"
#include "vector.h"
#include "cbase.h"

#include "VRModelHelper.h"

extern int ExtractBbox(void *pmodel, int sequence, float *mins, float *maxs);
extern int GetSequenceInfo(void *pmodel, int sequence, float *pflFrameRate, float *pflGroundSpeed);
extern int GetNumSequences(void *pmodel);

VRModelInfo::SequenceInfo::SequenceInfo() :
	bboxMins{},
	bboxMaxs{},
	bboxRadius{ 0.f },
	framerate{ 0.f }
{
}

VRModelInfo::SequenceInfo::SequenceInfo(Vector mins, Vector maxs, float framerate) :
	bboxMins{ mins },
	bboxMaxs{ maxs },
	bboxRadius{ (maxs - mins).Length() * 0.5f },
	framerate{ framerate }
{
}

VRModelInfo::VRModelInfo() :
	m_isValid{ false },
	m_hlName{ iStringNull },
	m_name{},
	m_numSequences{ 0 }
{
}

VRModelInfo::VRModelInfo(CBaseEntity *pEntity) :
	m_isValid{ false },
	m_hlName{ pEntity->pev->model },
	m_name{ STRING(m_hlName) },
	m_numSequences{ 0 }
{
	if (m_hlName && !m_name.empty())
	{
		void *pmodel = GET_MODEL_PTR(pEntity->edict());
		if (pmodel)
		{
			m_numSequences = GetNumSequences(pmodel);
			if (m_numSequences > 0)
			{
				for (int sequence = 0; sequence < m_numSequences; sequence++)
				{
					Vector mins;
					Vector maxs;
					float framerate;
					float dummy;
					if (ExtractBbox(pmodel, sequence, mins, maxs)
						&& GetSequenceInfo(pmodel, sequence, &framerate, &dummy))
					{
						m_sequences.emplace_back(mins, maxs, framerate);
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


const VRModelInfo& VRModelHelper::GetModelInfo(CBaseEntity *pEntity)
{
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

VRModelHelper VRModelHelper::m_instance{};
