#pragma once

#include <unordered_map>
class CBaseEntity;
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
	static const VRModelInfo& GetModelInfo(CBaseEntity *pEntity);

private:
	static VRModelHelper m_instance;

	std::unordered_map<string_t, VRModelInfo>	m_modelInfoMap;
};

