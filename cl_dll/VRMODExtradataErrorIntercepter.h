#pragma once

// Special class that watches for the "Fatal Error" popup window with text "Mod_Extradata: caching failed".
// It will modify the popup message to contain the name of the faulty model (last call to precache).

class VRMODExtradataErrorIntercepter
{
public:
	~VRMODExtradataErrorIntercepter();

private:
	VRMODExtradataErrorIntercepter();

	static VRMODExtradataErrorIntercepter m_instance;
};
