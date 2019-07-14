#pragma once

class CBasePlayer;
class CBaseAnimating;

typedef int string_t;

#include "weapons.h"

class VRController
{
public:
#ifndef CLIENT_DLL
	~VRController();
#endif

	struct HitBox
	{
		Vector origin;
		Vector angles;
		Vector mins;
		Vector maxs;
	};

	void Update(CBasePlayer *pPlayer, const int timestamp, const bool isValid, const bool isMirrored, const Vector & offset, const Vector & angles, const Vector & velocity, bool isDragging, VRControllerID id, int weaponId);

	CBaseEntity* GetModel() const;
	void PlayWeaponAnimation(int iAnim, int body);
	void PlayWeaponMuzzleflash();

	inline VRControllerID GetID() const { return m_id; }
	inline const Vector& GetOffset() const { return m_offset; }
	inline const Vector& GetPosition() const { return m_position; }
	inline const Vector& GetAngles() const { return m_angles; }
	inline const Vector& GetVelocity() const { return m_velocity; }
	//inline const Vector& GetMins() const { return m_mins; }
	//inline const Vector& GetMaxs() const { return m_maxs; }
	inline const float GetRadius() const { return m_radius; }
	//inline const float GetRadiusSquared() const { return m_radiusSquared; }
	inline bool IsDragging() const { return m_isDragging; }
	inline bool IsValid() const { return m_isValid && m_id != VRControllerID::INVALID; }
	//inline bool IsBBoxValid() const { return m_isBBoxValid; }
	inline int GetWeaponId() const { return m_weaponId; }
	inline bool IsMirrored() const { return m_isMirrored; }

	inline const std::vector<HitBox>& GetHitBoxes() const { return m_hitboxes; }
	//bool CheckHitBox(const Vector& origin, const Vector& mins, const Vector& maxs) const;

	const Vector GetGunPosition() const;
	const Vector GetAim() const;

	bool AddTouchedEntity(EHANDLE hEntity) const;
	bool RemoveTouchedEntity(EHANDLE hEntity) const;

	bool AddDraggedEntity(EHANDLE hEntity) const;
	bool RemoveDraggedEntity(EHANDLE hEntity) const;
	bool IsDraggedEntity(EHANDLE hEntity) const;

	bool AddHitEntity(EHANDLE hEntity) const;
	bool RemoveHitEntity(EHANDLE hEntity) const;

private:
	void UpdateModel(CBasePlayer* pPlayer);
	void UpdateLaserSpot();
	//void ExtractBBoxIfPossibleAndNecessary();
	void UpdateHitBoxes();
	void SendEntityDataToClient(CBasePlayer *pPlayer, VRControllerID id);

	VRControllerID	m_id{ VRControllerID::INVALID };
	Vector m_offset;
	Vector m_position;
	Vector m_angles;
	Vector m_velocity;
	int m_weaponId{ WEAPON_BAREHAND };
	int m_lastUpdateClienttime{ 0 };
	float m_lastUpdateServertime{ 0.f };
	bool m_isValid{ false };
	bool m_isDragging{ false };
	bool m_isMirrored{ false };
	string_t m_modelName{ 0 };
	string_t m_bboxModelName{ 0 };
	int m_bboxModelSequence{ 0 };
	mutable EHANDLE m_hModel;
	mutable EHANDLE m_hLaserSpot;
	mutable std::unordered_set<EHANDLE, EHANDLE::Hash, EHANDLE::Equal> m_touchedEntities;
	mutable std::unordered_set<EHANDLE, EHANDLE::Hash, EHANDLE::Equal> m_draggedEntities;
	mutable std::unordered_set<EHANDLE, EHANDLE::Hash, EHANDLE::Equal> m_hitEntities;

	std::vector<HitBox> m_hitboxes;
	string_t m_hitboxModelName{ 0 };
	int m_hitboxSequence{ 0 };
	float m_hitboxFrame{ 0.f };
	float m_hitboxLastUpdateTime{ 0.f };
	float m_radius{ 0.f };
	inline void ClearHitBoxes()
	{
		m_hitboxes.clear();
		m_hitboxModelName = 0;
		m_hitboxSequence = 0;
		m_hitboxFrame = 0.f;
		m_hitboxLastUpdateTime = 0.f;
		m_radius = 0.f;
	}

	// TODO TEMP
	void DebugDrawHitBoxes(CBasePlayer *pPlayer);
	mutable std::vector<EHANDLE>		m_hDebugBBoxes;
};

