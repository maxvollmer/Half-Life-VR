
// New methods for VR added to StudioModel - Max Vollmer - 2019-04-07

#include <string>
#include <cstring>
#include "StudioModel.h"


StudioModel::StudioModel(void* pmodel)
{
	LoadModel(GetModelName(pmodel));
}

StudioModel::StudioModel(const char* modelname)
{
	extern std::string UTIL_GetGameDir();
	LoadModel((UTIL_GetGameDir() + "/" + modelname).data());
}

StudioModel::~StudioModel()
{
	FreeModel();
}

bool StudioModel::ExtractTransformedHitbox(int boneIndex, float* bbMins, float* bbMaxs, float(*bbTransform)[4])
{
	mstudiobbox_t* pbboxes = reinterpret_cast<mstudiobbox_t*>(reinterpret_cast<byte*>(m_pstudiohdr) + m_pstudiohdr->hitboxindex);
	if (boneIndex < m_pstudiohdr->numhitboxes)
	{
		VectorCopy(pbboxes[boneIndex].bbmin, bbMins);
		VectorCopy(pbboxes[boneIndex].bbmax, bbMaxs);
		std::memcpy(bbTransform, m_bonetransform[pbboxes[boneIndex].bone], sizeof(float) * 12);  //3x4 transform matrix
		return true;
	}
	else
	{
		return false;
	}
}

int StudioModel::GetNumHitboxes()
{
	if (!m_pstudiohdr)
		return 0;

	return m_pstudiohdr->numhitboxes;
}

int StudioModel::GetNumAttachments()
{
	if (!m_pstudiohdr)
		return 0;

	return m_pstudiohdr->numattachments;
}

const char* StudioModel::GetModelName(void* pmodel)
{
	return static_cast<studiohdr_t*>(pmodel)->name;
}
