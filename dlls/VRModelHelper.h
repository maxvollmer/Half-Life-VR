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
		SequenceInfo(Vector mins, Vector maxs, float framerate, int numFrames, bool isLooping);
		const Vector bboxMins;
		const Vector bboxMaxs;
		const float bboxRadius = 0.f;
		const float framerate = 0.f;
		const int numFrames = 0;
		const bool isLooping;
	};

	VRModelInfo();
	VRModelInfo(CBaseEntity* pEntity);

	bool m_isValid;

	string_t m_hlName;
	std::string m_name;
	int m_numSequences = 0;
	std::vector<SequenceInfo> m_sequences;

	static const VRModelInfo INVALID;
};

class VRModelHelper
{
public:
	const VRModelInfo& GetModelInfo(CBaseEntity* pModel);
	//std::vector<TransformedBBox> GetTransformedBBoxesForModel(CBaseEntity* pModel);
	//bool GetAttachment(CBaseEntity *pModel, int attachmentIndex, bool mirrored, Vector& attachment);

	static VRModelHelper& GetInstance() { return m_instance; }

private:
	static VRModelHelper m_instance;

	std::unordered_map<string_t, VRModelInfo> m_modelInfoMap;
	std::unordered_map<std::string, std::unique_ptr<StudioModel>> m_studioModels;
};
