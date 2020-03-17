#pragma once

class VRDebugBBoxDrawer
{
public:

	~VRDebugBBoxDrawer();

	void DrawBBoxes(EHANDLE<CBaseEntity> hEntity, bool mirrored = false);
	void DrawPoint(EHANDLE<CBaseEntity> hEntity, const Vector& point);
	void Clear(EHANDLE<CBaseEntity> hEntity);
	void ClearPoint(EHANDLE<CBaseEntity> hEntity);
	void ClearAllBut(EHANDLE<CBaseEntity> hEntity);
	void ClearAll();

	inline void SetColor(int r, int g, int b) { m_r = r; m_g = g; m_b = b; }

private:

	int m_r = 255;
	int m_g = 0;
	int m_b = 0;

	std::unordered_map<EHANDLE<CBaseEntity>, std::vector<EHANDLE<CBaseEntity>>, EHANDLE<CBaseEntity>::Hash, EHANDLE<CBaseEntity>::Equal> m_bboxes;
	std::unordered_map<EHANDLE<CBaseEntity>, EHANDLE<CBaseEntity>, EHANDLE<CBaseEntity>::Hash, EHANDLE<CBaseEntity>::Equal> m_points;
};

