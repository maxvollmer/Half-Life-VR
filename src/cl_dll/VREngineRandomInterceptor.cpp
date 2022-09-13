
// Random implementation using std random classes for overriding engine's random functions
// For Half-Life: VR by Max Vollmer - 2020-02-22

#include "EasyHook/include/easyhook.h"

#include <random>
#include <vector>
#include <memory>
#include <string>
#include <sstream>

#include "hud.h"

namespace
{
    std::vector<std::shared_ptr<HOOK_TRACE_INFO>> hooks;

    std::random_device random_device;

    std::default_random_engine seeder{ random_device() };

    std::default_random_engine rng{ random_device() };

    std::default_random_engine rngbackup = rng;

    std::uniform_real_distribution<float> floatdistribution(0.f, 1.f);
}

void VRRandomResetSeed(unsigned int seed/* = 0*/)
{
    if (seed)
    {
        rng.seed(seed);
    }
    else
    {
        static std::uniform_int_distribution<long> distribution((std::random_device::min)(), (std::random_device::max)());
        rng.seed(distribution(seeder));
    }
}

void VRRandomBackupSeed()
{
    rngbackup = rng;
}

void VRRandomRestoreSeed()
{
    rng = rngbackup;
}

float VRRandomFloat(float low, float high)
{
    return floatdistribution(rng) * (high - low) + low;
}

long VRRandomLong(long low, long high)
{
    return static_cast<long>(floatdistribution(rng) * (high + 1 - low)) + low;
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

bool VRHookRandomFunctions()
{
    return HookRandomMethod("RandomFloat", gEngfuncs.pfnRandomFloat, VRRandomFloat)
        && HookRandomMethod("RandomLong", gEngfuncs.pfnRandomLong, VRRandomLong);
}
