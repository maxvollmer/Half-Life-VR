#pragma once

class CBasePlayer;
class CBaseAnimating;

typedef int string_t;

#include "weapons.h"

class VRController
{
public:
	void Update(CBasePlayer *pPlayer, const int timestamp, const bool isValid, const bool isMirrored, const Vector & offset, const Vector & angles, const Vector & velocity, bool isDragging, int weaponId);

	CBaseEntity* GetModel() const;
	void PlayWeaponAnimation(int iAnim, int body);
	void PlayWeaponMuzzleflash();

	inline const Vector& GetOffset() const { return m_offset; }
	inline const Vector& GetPosition() const { return m_position; }
	inline const Vector& GetAngles() const { return m_angles; }
	inline const Vector& GetVelocity() const { return m_velocity; }
	inline const Vector& GetMins() const { return m_mins; }
	inline const Vector& GetMaxs() const { return m_maxs; }
	inline const float GetRadius() const { return m_radius; }
	inline const float GetRadiusSquared() const { return m_radiusSquared; }
	inline bool IsDragging() const { return m_isDragging; }
	inline bool IsValid() const { return m_isValid; }
	inline bool IsBBoxValid() const { return m_isBBoxValid; }
	inline bool IsTeleporterBlocked() const { return m_isTeleporterBlocked; }
	inline int GetWeaponId() const { return m_weaponId; }
	inline bool IsMirrored() const { return m_isMirrored; }

	bool AddTouchedEntity(EHANDLE hEntity) const;
	bool RemoveTouchedEntity(EHANDLE hEntity) const;

	bool AddHitEntity(EHANDLE hEntity) const;
	bool RemoveHitEntity(EHANDLE hEntity) const;

private:
	void ExtractBBoxIfPossibleAndNecessary();
	void SendMirroredEntityToClient(CBasePlayer *pPlayer);

	Vector m_offset;
	Vector m_position;
	Vector m_angles;
	Vector m_velocity;
	Vector m_mins;
	Vector m_maxs;
	float m_radius{ 0.f };
	float m_radiusSquared{ 0.f };
	int m_weaponId{ WEAPON_BAREHAND };
	int m_lastUpdateClienttime{ 0 };
	float m_lastUpdateServertime{ 0.f };
	bool m_isValid{ false };
	bool m_isDragging{ false };
	bool m_isBBoxValid{ false };
	bool m_isTeleporterBlocked{ true };
	bool m_isMirrored{ false };
	string_t m_modelName{ 0 };
	string_t m_bboxModelName{ 0 };
	int m_bboxModelSequence{ 0 };
	mutable EHANDLE m_hModel;
	mutable std::unordered_set<EHANDLE, EHANDLE::Hash, EHANDLE::Equal> m_touchedEntities;
	mutable std::unordered_set<EHANDLE, EHANDLE::Hash, EHANDLE::Equal> m_hitEntities;
};

