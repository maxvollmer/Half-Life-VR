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

#include <vector>
#include <cstring>

#include "hud.h"
#include "cl_util.h"
#include "demo.h"
#include "demo_api.h"
#include <memory.h>

#define DLLEXPORT __declspec(dllexport)

int g_demosniper = 0;
int g_demosniperdamage = 0;
float g_demosniperorg[3];
float g_demosniperangles[3];
float g_demozoom = 0.f;

extern "C"
{
	void DLLEXPORT Demo_ReadBuffer(int size, unsigned char* buffer);
}

template <typename T>
T ReadFromBuffer(unsigned char*& buffer)
{
	T value;
	std::memcpy(&value, buffer, sizeof(T));
	buffer += sizeof(T);
	return value;
}

/*
=====================
Demo_WriteBuffer

Write some data to the demo stream
=====================
*/
void Demo_WriteBuffer(int type, int size, unsigned char* buffer)
{
	std::vector<unsigned char> buf;
	buf.resize(sizeof(int));
	std::memcpy(buf.data(), &type, sizeof(int));
	buf.insert(buf.end(), buffer, buffer + size);

	// Write full buffer out
	gEngfuncs.pDemoAPI->WriteBuffer(static_cast<int>(buf.size()), buf.data());
}

/*
=====================
Demo_ReadBuffer

Engine wants us to parse some data from the demo stream
=====================
*/
void DLLEXPORT Demo_ReadBuffer(int size, unsigned char* buffer)
{
	int type = ReadFromBuffer<int>(buffer);
	switch (type)
	{
	case TYPE_SNIPERDOT:
		g_demosniper = ReadFromBuffer<int>(buffer);
		if (g_demosniper)
		{
			g_demosniperdamage = ReadFromBuffer<int>(buffer);

			g_demosniperangles[0] = ReadFromBuffer<float>(buffer);
			g_demosniperangles[1] = ReadFromBuffer<float>(buffer);
			g_demosniperangles[2] = ReadFromBuffer<float>(buffer);

			g_demosniperorg[0] = ReadFromBuffer<float>(buffer);
			g_demosniperorg[1] = ReadFromBuffer<float>(buffer);
			g_demosniperorg[2] = ReadFromBuffer<float>(buffer);
		}
		break;
	case TYPE_ZOOM:
		g_demozoom = ReadFromBuffer<float>(buffer);
		break;
	default:
		gEngfuncs.Con_DPrintf("Unknown demo buffer type, skipping.\n");
		break;
	}
}
