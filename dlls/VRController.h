#pragma once

class CBasePlayer;
class CBaseAnimating;
class CLaserSpot;
class CVRControllerModel;

typedef int string_t;

#include "weapons.h"
#include "animation.h"

// #define RENDER_DEBUG_HITBOXES

class VRController
{
public:
	enum class TouchType
	{
		LIGHT_TOUCH,
		NORMAL_TOUCH,
		HARD_TOUCH
	};

	void Update(CBasePlayer* pPlayer, const int timestamp, const bool isValid, const bool isMirrored, const Vector& offset, const Vector& angles, const Vector& velocity, bool isDragging, VRControllerID id, int weaponId);
	void PostFrame();

	CVRControllerModel* GetModel() const;
	void PlayWeaponAnimation(int iAnim, int body);
	void PlayWeaponMuzzleflash();

	inline void SetHasFlashlight(bool hasFlashlight) { m_hasFlashlight = hasFlashlight; }

	inline VRControllerID GetID() const { return m_id; }
	inline const Vector& GetOffset() const { return m_offset; }
	inline const Vector& GetPosition() const { return m_position; }
	inline const Vector& GetAngles() const { return m_angles; }
	inline const Vector& GetPreviousAngles() const { return m_previousAngles; }
	inline const Vector& GetVelocity() const { return m_velocity; }
	inline const float GetRadius() const { return m_radius; }
	inline bool IsValid() const { return m_isValid && m_id != VRControllerID::INVALID; }
	inline int GetWeaponId() const { return m_weaponId; }
	inline bool IsMirrored() const { return m_isMirrored; }
	inline bool IsBlocked() const { return m_isBlocked; }
	inline const std::vector<StudioHitBox>& GetHitBoxes() const { return m_hitboxes; }
	inline float GetDragStartTime() const { return m_dragStartTime; }

	bool HasAnyEntites() const;

	bool IsDragging() const;
	bool GetAttachment(size_t index, Vector& attachment) const;

	const Vector GetGunPosition() const;
	const Vector GetAim() const;

	bool AddTouchedEntity(EHANDLE<CBaseEntity> hEntity) const;
	bool RemoveTouchedEntity(EHANDLE<CBaseEntity> hEntity) const;

	bool SetDraggedEntity(EHANDLE<CBaseEntity> hEntity) const;
	bool RemoveDraggedEntity(EHANDLE<CBaseEntity> hEntity) const;
	void ClearDraggedEntity() const;
	bool IsDraggedEntity(EHANDLE<CBaseEntity> hEntity) const;
	bool HasDraggedEntity() const;
	inline EHANDLE<CBaseEntity> GetDraggedEntity() const { return m_draggedEntity; }

	bool AddHitEntity(EHANDLE<CBaseEntity> hEntity) const;
	bool RemoveHitEntity(EHANDLE<CBaseEntity> hEntity) const;

	struct DraggedEntityPositions
	{
		const Vector controllerStartOffset;
		const Vector controllerStartPos;
		const Vector entityStartOrigin;
		const Vector playerStartOrigin;
		Vector controllerLastOffset;
		Vector controllerLastPos;
		Vector entityLastOrigin;
		Vector playerLastOrigin;
	};

	bool GetDraggedEntityPositions(EHANDLE<CBaseEntity> hEntity,
		Vector& controllerStartOffset,
		Vector& controllerStartPos,
		Vector& entityStartOrigin,
		Vector& playerStartOrigin,
		Vector& controllerLastOffset,
		Vector& controllerLastPos,
		Vector& entityLastOrigin,
		Vector& playerLastOrigin) const;

	void AddTouch(TouchType touch, float duration = 0.f);

private:
	void UpdateModel(CBasePlayer* pPlayer);
	void UpdateHitBoxes();
	void ClearOutDeadEntities();
	void SendEntityDataToClient(CBasePlayer* pPlayer, VRControllerID id);

	EHANDLE<CBasePlayer> m_hPlayer;

	VRControllerID m_id{ VRControllerID::INVALID };
	Vector m_offset;
	Vector m_position;
	Vector m_angles;
	Vector m_previousAngles;
	Vector m_velocity;
	int m_weaponId{ WEAPON_BAREHAND };
	int m_lastUpdateClienttime{ 0 };
	float m_lastUpdateServertime{ 0.f };
	bool m_isValid{ false };
	bool m_isDragging{ false };
	bool m_isMirrored{ false };
	bool m_isBlocked{ true };
	bool m_hasFlashlight{ false };
	string_t m_modelName{ 0 };
	string_t m_bboxModelName{ 0 };
	int m_bboxModelSequence{ 0 };
	float m_dragStartTime{ 0.f };
	float m_radius{ 0.f };

	mutable EHANDLE<CVRControllerModel> m_hModel;

	mutable std::unordered_set<EHANDLE<CBaseEntity>, EHANDLE<CBaseEntity>::Hash, EHANDLE<CBaseEntity>::Equal> m_touchedEntities;
	mutable std::unordered_set<EHANDLE<CBaseEntity>, EHANDLE<CBaseEntity>::Hash, EHANDLE<CBaseEntity>::Equal> m_hitEntities;

	mutable EHANDLE<CBaseEntity> m_draggedEntity;
	mutable std::unique_ptr<DraggedEntityPositions> m_draggedEntityPositions;

	std::vector<StudioHitBox> m_hitboxes;
	std::vector<Vector> m_attachments;
	string_t m_hitboxModelName{ 0 };
	int m_hitboxSequence{ 0 };
	float m_hitboxFrame{ 0.f };
	float m_hitboxLastUpdateTime{ 0.f };
	bool m_hitboxMirrored{ false };
	inline void ClearHitBoxes()
	{
		m_hitboxes.clear();
		m_hitboxModelName = 0;
		m_hitboxSequence = 0;
		m_hitboxFrame = 0.f;
		m_hitboxLastUpdateTime = 0.f;
		m_radius = 0.f;
		m_attachments.clear();
	}

	std::unordered_map<TouchType, float> m_touches;
};
