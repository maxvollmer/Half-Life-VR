
#include "EasyHook/include/easyhook.h"

#include <vector>
#include <memory>

#include "hud.h"

namespace
{
    std::vector<std::shared_ptr<HOOK_TRACE_INFO>> hooks;
}

bool HookRandomMethod(const char* name, void* originalmethod, void* hookmethod)
{
    std::shared_ptr<HOOK_TRACE_INFO> hook = std::make_shared<HOOK_TRACE_INFO>(HOOK_TRACE_INFO{ nullptr });
    hooks.push_back(hook);

    NTSTATUS result = LhInstallHook(
        originalmethod,
        hookmethod,
        nullptr,
        hook.get());

    if (FAILED(result))
    {
        gEngfuncs.Con_DPrintf("Warning: Failed to install hook for engine's random method %s: %S\n", name, RtlGetLastErrorString());
        return false;
    }
    else
    {
        ULONG ACLEntries[1] = { 0 };
        LhSetInclusiveACL(ACLEntries, 1, hook.get());
        return true;
    }
}

void VRRandomResetSeed()
{
    extern int idum;
    idum = 0;
}

namespace
{
    int backupseed = 0;
}

void VRRandomBackupSeed()
{
    extern int idum;
    backupseed = idum;
}

void VRRandomRestoreSeed()
{
    extern int idum;
    idum = backupseed;
}

float MyRandomFloat(float flLow, float flHigh)
{
    extern float Com_RandomFloat(float flLow, float flHigh);
    return Com_RandomFloat(flLow, flHigh);
}

long MyRandomLong(long lLow, long lHigh)
{
    extern int Com_RandomLong(int lLow, int lHigh);
    return Com_RandomLong(lLow, lHigh);
}

bool VRHookRandomFunctions()
{
    return
        HookRandomMethod("RandomFloat", gEngfuncs.pfnRandomFloat, MyRandomFloat)
        && HookRandomMethod("RandomLong", gEngfuncs.pfnRandomLong, MyRandomLong);
}

