#pragma once

// Bunch of simple classes, structs and other stuff - Max Vollmer - 2019-04-07

#include "vector.h"

class SimpleMatrix34
{
public:
	SimpleMatrix34() {}
	SimpleMatrix34(float(*transform)[4])
	{
		std::memcpy(this->transform, transform, sizeof(float) * 12);
	}
	float transform[12]{ 0.f };
};

class TransformedBBox
{
public:
	TransformedBBox() {}
	TransformedBBox(Vector mins, Vector maxs, float(*transform)[4]) :
		mins{ mins },
		maxs{ maxs },
		transform{ transform }
	{
	}
	const Vector mins{};
	const Vector maxs{};
	const SimpleMatrix34 transform{};
};

class GlobalXenMounds
{
public:
	void Add(const Vector& position, const string_t multi_manager);
	bool Trigger(CBasePlayer* pPlayer, const Vector& position);
	bool Has(const Vector& position);
	void Clear() { m_xen_mounds.clear(); }

private:
	std::map<const Vector, const string_t> m_xen_mounds;
};

class VRLevelChangeData
{
public:
	Vector lastHMDOffset{};
	Vector clientOriginOffset{};  // Must be Vector instead of Vector2D, so save/load works (only has FIELD_VECTOR, which expects 3 floats)
	BOOL hasData{ false };          // Must be BOOL (int), so save/load works (only has FIELD_BOOLEAN, which expects int)
	float prevYaw{ 0.f };
	float currentYaw{ 0.f };
};
