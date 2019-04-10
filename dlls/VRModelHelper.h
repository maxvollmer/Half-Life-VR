#pragma once

#include <unordered_map>
#include "VRCommons.h"

class CBaseEntity;
class StudioModel;
typedef int string_t;

class VRModelInfo
{
public:
	class SequenceInfo
	{
	public:
		SequenceInfo();
		SequenceInfo(Vector mins, Vector maxs, float framerate);
		const Vector bboxMins;
		const Vector bboxMaxs;
		const float bboxRadius;
		const float framerate;
	};

	VRModelInfo();
	VRModelInfo(CBaseEntity *pEntity);

	bool						m_isValid;

	string_t					m_hlName;
	std::string					m_name;
	int							m_numSequences;
	std::vector<SequenceInfo>	m_sequences;
};

class VRModelHelper
{
public:
	const VRModelInfo& GetModelInfo(CBaseEntity *pModel);
	std::vector<TransformedBBox> GetTransformedBBoxesForModel(CBaseEntity* pModel);
	bool GetAttachment(CBaseEntity *pModel, int attachmentIndex, Vector& attachment);

	static VRModelHelper& GetInstance() { return m_instance; }

private:
	static VRModelHelper m_instance;

	std::unordered_map<string_t, VRModelInfo>						m_modelInfoMap;
	std::unordered_map<std::string, std::unique_ptr<StudioModel>>	m_studioModels;
};

