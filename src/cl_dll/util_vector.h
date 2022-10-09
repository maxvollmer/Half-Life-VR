/***
*
*	Copyright (c) 1996-2002, Valve LLC. All rights reserved.
*
*	This product contains software technology licensed from Id
*	Software, Inc. ("Id Technology").  Id Technology (c) 1996 Id Software, Inc.
*	All Rights Reserved.
*
*   Use, distribution, and modification of this source code and/or resulting
*   object code is restricted to non-commercial enhancements to products from
*   Valve LLC.  All other use, distribution, or modification is prohibited
*   without written permission from Valve LLC.
*
****/
//  Vector.h
// A subset of the extdll.h in the project HL Entity DLL
//

#pragma once

// Misc C-runtime library headers
#include "STDIO.H"
#include "STDLIB.H"
#include "MATH.H"

// Header file containing definition of globalvars_t and entvars_t
typedef int func_t;    //
typedef int string_t;  // from engine's pr_comp.h;
typedef float vec_t;   // needed before including progdefs.h

//=========================================================
// 2DVector - used for many pathfinding and many other
// operations that are treated as planar rather than 3d.
//=========================================================
class Vector2D
{
public:
	inline Vector2D(void)
	{
		x = 0;
		y = 0;
	}
	inline Vector2D(float X, float Y)
	{
		x = X;
		y = Y;
	}
	inline Vector2D operator+(const Vector2D& v) const { return Vector2D(x + v.x, y + v.y); }
	inline Vector2D operator-(const Vector2D& v) const { return Vector2D(x - v.x, y - v.y); }
	inline Vector2D operator*(float fl) const { return Vector2D(x * fl, y * fl); }
	inline Vector2D operator/(float fl) const { return Vector2D(x / fl, y / fl); }

	inline float Length(void) const { return sqrtf(x * x + y * y); }

	inline Vector2D Normalize(void) const
	{
		Vector2D vec2;

		float flLen = Length();
		if (flLen == 0)
		{
			return Vector2D((float)0, (float)0);
		}
		else
		{
			flLen = 1 / flLen;
			return Vector2D(x * flLen, y * flLen);
		}
	}

	vec_t x, y;
};

inline float DotProduct(const Vector2D& a, const Vector2D& b) { return (a.x * b.x + a.y * b.y); }
inline Vector2D operator*(float fl, const Vector2D& v) { return v * fl; }

//=========================================================
// 3D Vector
//=========================================================
class Vector  // same data-layout as engine's vec3_t,
{             //		which is a vec_t[3]
public:
	// Construction/destruction
	Vector()
	{
		x = 0;
		y = 0;
		z = 0;
	}
	Vector(const Vector& v)
	{
		x = v.x;
		y = v.y;
		z = v.z;
	}
	Vector(float X, float Y, float Z)
	{
		x = X;
		y = Y;
		z = Z;
	}
	Vector(double X, double Y, double Z)
	{
		x = (float)X;
		y = (float)Y;
		z = (float)Z;
	}
	Vector(int X, int Y, int Z)
	{
		x = (float)X;
		y = (float)Y;
		z = (float)Z;
	}
	Vector(const float rgfl[3])
	{
		if (rgfl)
		{
			x = rgfl[0];
			y = rgfl[1];
			z = rgfl[2];
		}
		else
		{
			x = y = z = 0.f;
		}
	}

	// Operators
	inline Vector operator-(void) const { return Vector(-x, -y, -z); }
	inline bool operator==(const Vector& v) const { return x == v.x && y == v.y && z == v.z; }
	inline bool operator!=(const Vector& v) const { return !(*this == v); }
	inline Vector operator+(const Vector& v) const { return Vector(x + v.x, y + v.y, z + v.z); }
	inline Vector operator-(const Vector& v) const { return Vector(x - v.x, y - v.y, z - v.z); }
	inline Vector operator*(float fl) const { return Vector(x * fl, y * fl, z * fl); }
	inline Vector operator/(float fl) const { return Vector(x / fl, y / fl, z / fl); }

	inline Vector operator+=(const Vector& v) { *this = *this + v; return *this; }
	inline Vector operator-=(const Vector& v) { *this = *this - v; return *this; }
	inline Vector operator*=(float fl) { *this = *this * fl; return *this; }
	inline Vector operator/=(float fl) { *this = *this / fl; return *this; }

	inline Vector operator-(const Vector2D& v) const { return Vector(x - v.x, y - v.y, z); }
	inline Vector operator+(const Vector2D& v) const { return Vector(x + v.x, y + v.y, z); }

	// Methods
	inline void CopyToArray(float* rgfl) const { rgfl[0] = x, rgfl[1] = y, rgfl[2] = z; }
	inline float Length() const { return sqrtf(LengthSquared()); }
	inline float LengthSquared() const { return x * x + y * y + z * z; }  // Added squared length for convenience - Max Vollmer, 2018-01-28
	operator float* () { return &x; }                                          // Vectors will now automatically convert to float * when needed
	operator const float* () const { return &x; }                              // Vectors will now automatically convert to float * when needed
	inline Vector Normalize(void) const
	{
		float flLen = Length();
		if (flLen == 0)
			return Vector(0, 0, 1);  // ????
		flLen = 1 / flLen;
		return Vector(x * flLen, y * flLen, z * flLen);
	}
	inline void InlineNormalize(void)
	{
		float flLen = Length();
		if (flLen == 0)
		{
			z = 1;
		}
		else
		{
			x /= flLen;
			y /= flLen;
			z /= flLen;
		}
	}

	inline Vector2D Make2D(void) const
	{
		Vector2D Vec2;

		Vec2.x = x;
		Vec2.y = y;

		return Vec2;
	}
	inline float Length2D(void) const { return sqrtf(x * x + y * y); }

	// Members
	vec_t x = 0.f, y = 0.f, z = 0.f;
};
inline Vector operator*(float fl, const Vector& v) { return v * fl; }
inline float DotProduct(const Vector& a, const Vector& b) { return (a.x * b.x + a.y * b.y + a.z * b.z); }
inline Vector CrossProduct(const Vector& a, const Vector& b) { return Vector(a.y * b.z - a.z * b.y, a.z * b.x - a.x * b.z, a.x * b.y - a.y * b.x); }

#define vec3_t Vector
