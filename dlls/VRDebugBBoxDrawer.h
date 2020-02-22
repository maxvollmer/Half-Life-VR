#pragma once

class VRDebugBBoxDrawer
{
public:

	~VRDebugBBoxDrawer();

	void DrawBBoxes(EHANDLE<CBaseEntity> hEntity);
	void ClearBBoxes(EHANDLE<CBaseEntity> hEntity);

private:

	std::unordered_map<EHANDLE<CBaseEntity>, std::vector<EHANDLE<CBaseEntity>>, EHANDLE<CBaseEntity>::Hash, EHANDLE<CBaseEntity>::Equal> m_bboxes;
};

