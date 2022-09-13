
#include <string>
#include <iostream>

#include "EasyHook/include/easyhook.h"

#include "hud.h"
#include "cl_util.h"
#include "const.h"
#include "com_model.h"
#include "studio.h"
#include "cl_entity.h"
#include "r_studioint.h"

extern engine_studio_api_t IEngineStudio;

void* MyModExtradata(struct model_s* mod)
{
	extern studiohdr_t* Mod_Extradata(const char* callerInfo, cl_entity_t * ent, model_t * mod);
	return Mod_Extradata("EngineHook", nullptr, mod);
}

namespace
{
	HOOK_TRACE_INFO hHook = { nullptr };
}

void InstallModExtraDataCacheHook()
{
	NTSTATUS result = LhInstallHook(
		IEngineStudio.DONOTUSEMod_ExtradataDONOTUSE,
		MyModExtradata,
		nullptr,
		&hHook);

	if (FAILED(result))
	{
		std::wstring ws(RtlGetLastErrorString());
		std::string s{ ws.begin(), ws.end() };
		gEngfuncs.Con_DPrintf("Warning: Failed to install Mod_Extradata hook: %s\n", s.c_str());
	}
	else
	{
		ULONG ACLEntries[1] = { 0 };
		LhSetInclusiveACL(ACLEntries, 1, &hHook);
		//gEngfuncs.Con_DPrintf("Successfully installed hook!\n");
	}
}
