/*
This is a heavily stripped down version of snd_hook.cpp from MetaAudio by LAGonauta: https://github.com/LAGonauta/MetaAudio/ (based on MetaAudio by hzqst)

Reduced for Half-Life: VR by Max Vollmer to minimal functionality needed for hooking into sound functions by the engine.

All credit goes to LAGonauta and hzqst for making MetaAudio!
*/

#include "EasyHook/include/easyhook.h"

#include <stdarg.h>
#include <exception>
#include <cstdint>
#include <memory>
#include <vector>

#include "snd_hook.h"
#include "metahook.h"

aud_engine_t gAudEngine;

bool g_soundHookFailed = true;  // Init as true, so we won't initialize FMOD before VRHookSoundFunctions has successfully hooked engine functions.

//Error when can't find sig
void Sys_ErrorEx(const char* fmt, ...)
{
    va_list argptr;
    char msg[1024];

    va_start(argptr, fmt);
    _vsnprintf_s(msg, sizeof(msg), fmt, argptr);
    va_end(argptr);

    throw SoundInitError{ msg };
}

#define GetCallAddress(addr) (addr + (*(DWORD *)((addr)+1)) + 5)
#define Sig_NotFound(name) Sys_ErrorEx("Could not find entrypoint for %s\nEngine buildnum: %d", #name, g_dwEngineBuildnum);
#define Sig_FuncNotFound(name) if(!gAudEngine.name) Sig_NotFound(name)
#define Sig_AddrNotFound(name) if(!addr) Sig_NotFound(name)

#define Sig_Length(a) (sizeof(a)-1)
#define Search_Pattern(sig)MH_SearchPattern((void *)g_dwEngineBase, g_dwEngineSize, sig, Sig_Length(sig));
#define Search_Pattern_From(func, sig) MH_SearchPattern((void *)gAudEngine.func, g_dwEngineSize - (DWORD)gAudEngine.func + g_dwEngineBase, sig, Sig_Length(sig));

//Signatures for 6153
#define S_INIT_SIG_NEW "\x68\x2A\x2A\x2A\x2A\xE8\x2A\x2A\x2A\x2A\xE8\x2A\x2A\x2A\x2A\x68\x2A\x2A\x2A\x2A\xE8\x2A\x2A\x2A\x2A\x83\xC4\x08\x85\xC0"
#define S_SHUTDOWN_SIG_NEW "\xE8\x2A\x2A\x2A\x2A\xA1\x2A\x2A\x2A\x2A\x33\xC9"
#define S_FINDNAME_SIG_NEW "\x55\x8B\xEC\x53\x56\x8B\x75\x08\x33\xDB\x85\xF6"
#define S_PRECACHESOUND_SIG_NEW "\x55\x8B\xEC\xA1\x2A\x2A\x2A\x2A\x56\x85\xC0\x74\x2A\xD9\x05"
#define SND_SPATIALIZE_SIG_NEW "\x55\x8B\xEC\x83\xEC\x1C\x8B\x0D\x2A\x2A\x2A\x2A\x56"
#define S_STARTDYNAMICSOUND_SIG_NEW "\x55\x8B\xEC\x83\xEC\x48\xA1\x2A\x2A\x2A\x2A\x53\x56\x57\x85\xC0\xC7\x45\xFC\x00\x00\x00\x00"
#define S_STARTSTATICSOUND_SIG_NEW "\x55\x8B\xEC\x83\xEC\x44\x53\x56\x57\x8B\x7D\x10\x85\xFF\xC7\x45\xFC\x00\x00\x00\x00"
#define S_STOPSOUND_SIG_NEW "\x55\x8B\xEC\xA1\x2A\x2A\x2A\x2A\x57\xBF\x04\x00\x00\x00\x3B\xC7"
#define S_STOPALLSOUNDS_SIG_NEW "\x55\x8B\xEC\xA1\x2A\x2A\x2A\x2A\x85\xC0\x74\x4F\x56\xC7\x05"
#define S_UPDATE_SIG_NEW "\x55\x8B\xEC\xA1\x2A\x2A\x2A\x2A\x85\xC0\x0F\x84\x2A\x2A\x00\x00\xA1\x2A\x2A\x2A\x2A\x85\xC0\x0F\x8F\x2A\x2A\x00\x00"
#define S_LOADSOUND_SIG_NEW "\x55\x8B\xEC\x81\xEC\x44\x05\x00\x00\x53\x56\x8B\x75\x08"
#define S_LOADSOUND_SIG_NEWEST "\x55\x8B\xEC\x81\xEC\x28\x05\x00\x00\x53\x8B\x5D\x08\x56\x57\x8A"
#define SEQUENCE_GETSENTENCEBYINDEX_SIG_NEW "\x55\x8B\xEC\x56\x8B\x35\x2A\x2A\x2A\x2A\x85\xF6\x57\x74\x2A\x8B\x7D\x08\x8B\x06\x57\x50\xE8"
#define VOICESE_IDLE_SIG_NEW "\x55\x8B\xEC\xA0\x2A\x2A\x2A\x2A\xD9\x05\x2A\x2A\x2A\x2A\x84\xC0\x74\x2A\xD8\x1D"
#ifdef _DEBUG
#define SYS_ERROR_SIG_NEW "\x55\x8B\xEC\x81\xEC\x00\x04\x00\x00\x8B\x4D\x08\x8D\x45\x0C\x50\x51\x8D\x95\x00\xFC\xFF\xFF"
#endif

//Signatures for 3266
#define S_INIT_SIG "\x83\xEC\x08\x68\x2A\x2A\x2A\x2A\xE8\x2A\x2A\x2A\x2A\xE8\x2A\x2A\x2A\x2A\x68\x2A\x2A\x2A\x2A\xE8\x2A\x2A\x2A\x2A\x83\xC4\x08\x85\xC0"
#define S_SHUTDOWN_SIG S_SHUTDOWN_SIG_NEW
#define S_FINDNAME_SIG "\x53\x55\x8B\x6C\x24\x0C\x33\xDB\x56\x57\x85\xED"
#define S_PRECACHESOUND_SIG "\xA1\x2A\x2A\x2A\x2A\x56\x85\xC0\x74\x2A\xD9\x05"
#define SND_SPATIALIZE_SIG "\x83\xEC\x34\x8B\x0D\x2A\x2A\x2A\x2A\x53\x56"
#define S_STARTDYNAMICSOUND_SIG "\x83\xEC\x48\xA1\x2A\x2A\x2A\x2A\x53\x55\x56\x85\xC0\x57\xC7\x44\x24\x10\x00\x00\x00\x00"
#define S_STARTSTATICSOUND_SIG "\x83\xEC\x44\x53\x55\x8B\x6C\x24\x58\x56\x85\xED\x57"
#define S_STOPSOUND_SIG "\xA1\x2A\x2A\x2A\x2A\x57\xBF\x04\x00\x00\x00\x3B\xC7"
#define S_STOPALLSOUNDS_SIG "\xA1\x2A\x2A\x2A\x2A\x85\xC0\x74\x2A\x56\xC7\x05"
#define S_UPDATE_SIG "\xA1\x2A\x2A\x2A\x2A\x85\xC0\x0F\x84\x2A\x2A\x00\x00\xA1\x2A\x2A\x2A\x2A\x85\xC0\x0F\x8F\x2A\x2A\x00\x00"
#define S_LOADSOUND_SIG "\x81\xEC\x2A\x2A\x00\x00\x53\x8B\x9C\x24\x2A\x2A\x00\x00\x55\x56\x8A\x03\x57"
#define SEQUENCE_GETSENTENCEBYINDEX_SIG "\x56\x8B\x35\x2A\x2A\x2A\x2A\x85\xF6\x57\x74\x2A\x8B\x7C\x24\x0C\x8B\x06\x57\x50\xE8"
#define VOICESE_IDLE_SIG "\xA0\x2A\x2A\x2A\x2A\xD9\x05\x2A\x2A\x2A\x2A\x84\xC0\x74\x2A\xD8\x1D"
#ifdef _DEBUG
#define SYS_ERROR_SIG "\x8B\x4C\x24\x04\x81\xEC\x00\x04\x00\x00\x8D\x84\x24\x08\x04\x00\x00\x8D\x54\x24\x00\x50\x51\x68\x00\x04\x00\x00\x52\xE8\x2A\x2A\x2A\x2A\xA1\x2A\x2A\x2A\x2A\x83\xC4\x10\x85\xC0"
#endif

void S_FillAddress()
{
    MH_Init();

    if (g_dwEngineBuildnum >= 5953)
    {
        gAudEngine.S_Init = (void(*)(void))Search_Pattern(S_INIT_SIG_NEW);
        Sig_FuncNotFound(S_Init);

        gAudEngine.S_Shutdown = (void(*)(void))Search_Pattern_From(S_Init, S_SHUTDOWN_SIG_NEW);
        Sig_FuncNotFound(S_Shutdown);

        gAudEngine.S_FindName = (sfx_t * (*)(char*, int*))Search_Pattern_From(S_Shutdown, S_FINDNAME_SIG_NEW);
        Sig_FuncNotFound(S_FindName);

        gAudEngine.S_PrecacheSound = (sfx_t * (*)(char*))Search_Pattern_From(S_FindName, S_PRECACHESOUND_SIG_NEW);
        Sig_FuncNotFound(S_PrecacheSound);

        gAudEngine.SND_Spatialize = (void(*)(aud_channel_t*))Search_Pattern_From(S_PrecacheSound, SND_SPATIALIZE_SIG_NEW);
        Sig_FuncNotFound(SND_Spatialize);

        gAudEngine.S_StartDynamicSound = (void(*)(int, int, sfx_t*, float*, float, float, int, int))Search_Pattern_From(SND_Spatialize, S_STARTDYNAMICSOUND_SIG_NEW);
        Sig_FuncNotFound(S_StartDynamicSound);

        gAudEngine.S_StartStaticSound = (void(*)(int, int, sfx_t*, float*, float, float, int, int))Search_Pattern_From(S_StartDynamicSound, S_STARTSTATICSOUND_SIG_NEW);
        Sig_FuncNotFound(S_StartStaticSound);

        gAudEngine.S_StopSound = (void(*)(int, int))Search_Pattern_From(S_StartStaticSound, S_STOPSOUND_SIG_NEW);
        Sig_FuncNotFound(S_StopSound);

        gAudEngine.S_StopAllSounds = (void(*)(qboolean))Search_Pattern_From(S_StopSound, S_STOPALLSOUNDS_SIG_NEW);
        Sig_FuncNotFound(S_StopAllSounds);

        gAudEngine.S_Update = (void(*)(float*, float*, float*, float*))Search_Pattern_From(S_StopAllSounds, S_UPDATE_SIG_NEW);
        Sig_FuncNotFound(S_Update);

        if (g_dwEngineBuildnum >= 8279)
        {
            gAudEngine.S_LoadSound = (aud_sfxcache_t * (*)(sfx_t*, aud_channel_t*))Search_Pattern_From(S_Update, S_LOADSOUND_SIG_NEWEST);
        }
        else
        {
            gAudEngine.S_LoadSound = (aud_sfxcache_t * (*)(sfx_t*, aud_channel_t*))Search_Pattern_From(S_Update, S_LOADSOUND_SIG_NEW);
        }
        Sig_FuncNotFound(S_LoadSound);

        // gAudEngine.SequenceGetSentenceByIndex = (sentenceEntry_s * (*)(unsigned int))Search_Pattern(SEQUENCE_GETSENTENCEBYINDEX_SIG_NEW);
        // Sig_FuncNotFound(SequenceGetSentenceByIndex);

        gAudEngine.VoiceSE_Idle = (void(*)(float))Search_Pattern(VOICESE_IDLE_SIG_NEW);
        Sig_FuncNotFound(VoiceSE_Idle);
    }
    else
    {
        gAudEngine.S_Init = (void(*)(void))Search_Pattern(S_INIT_SIG);;
        Sig_FuncNotFound(S_Init);

        gAudEngine.S_Shutdown = (void(*)(void))Search_Pattern_From(S_Init, S_SHUTDOWN_SIG);
        Sig_FuncNotFound(S_Shutdown);

        gAudEngine.S_FindName = (sfx_t * (*)(char*, int*))Search_Pattern_From(S_Shutdown, S_FINDNAME_SIG);
        Sig_FuncNotFound(S_FindName);

        gAudEngine.S_PrecacheSound = (sfx_t * (*)(char*))Search_Pattern_From(S_FindName, S_PRECACHESOUND_SIG);
        Sig_FuncNotFound(S_PrecacheSound);

        gAudEngine.SND_Spatialize = (void(*)(aud_channel_t*))Search_Pattern_From(S_PrecacheSound, SND_SPATIALIZE_SIG);
        Sig_FuncNotFound(SND_Spatialize);

        gAudEngine.S_StartDynamicSound = (void(*)(int, int, sfx_t*, float*, float, float, int, int))Search_Pattern_From(SND_Spatialize, S_STARTDYNAMICSOUND_SIG);
        Sig_FuncNotFound(S_StartDynamicSound);

        gAudEngine.S_StartStaticSound = (void(*)(int, int, sfx_t*, float*, float, float, int, int))Search_Pattern_From(S_StartDynamicSound, S_STARTSTATICSOUND_SIG);
        Sig_FuncNotFound(S_StartStaticSound);

        gAudEngine.S_StopSound = (void(*)(int, int))Search_Pattern_From(S_StartStaticSound, S_STOPSOUND_SIG);
        Sig_FuncNotFound(S_StopSound);

        gAudEngine.S_StopAllSounds = (void(*)(qboolean))Search_Pattern_From(S_StopSound, S_STOPALLSOUNDS_SIG);
        Sig_FuncNotFound(S_StopAllSounds);

        gAudEngine.S_Update = (void(*)(float*, float*, float*, float*))Search_Pattern_From(S_StopAllSounds, S_UPDATE_SIG);
        Sig_FuncNotFound(S_Update);

        gAudEngine.S_LoadSound = (aud_sfxcache_t * (*)(sfx_t*, aud_channel_t*))Search_Pattern_From(S_Update, S_LOADSOUND_SIG);
        Sig_FuncNotFound(S_LoadSound);

        // gAudEngine.SequenceGetSentenceByIndex = (sentenceEntry_s * (*)(unsigned int))Search_Pattern(SEQUENCE_GETSENTENCEBYINDEX_SIG);
        // Sig_FuncNotFound(SequenceGetSentenceByIndex);

        gAudEngine.VoiceSE_Idle = (void(*)(float))Search_Pattern(VOICESE_IDLE_SIG);
        Sig_FuncNotFound(VoiceSE_Idle);
    }
}

namespace
{
    std::vector<std::shared_ptr<HOOK_TRACE_INFO>> hooks;
}

bool HookSoundMethod(const char* name, void* originalmethod, void* hookmethod)
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
        gEngfuncs.Con_DPrintf("Warning: Failed to install hook for sound method %s: %S\n", name, RtlGetLastErrorString());
        return false;
    }
    else
    {
        ULONG ACLEntries[1] = { 0 };
        LhSetInclusiveACL(ACLEntries, 1, hook.get());
        //gEngfuncs.Con_DPrintf("Successfully installed hook!\n");
        return true;
    }
}

bool VRHookSoundFunctions()
{
    try
    {
        S_FillAddress();
    }
    catch (const SoundInitError & e)
    {
        gEngfuncs.Con_DPrintf("Couldn't find engine sound function addresses, FMOD will not be available. Error: %s\n", e.what());
        gEngfuncs.Cvar_SetValue("vr_use_fmod", 0.f);
        g_soundHookFailed = true;
        return false;
    }

    extern void MyStartDynamicSound(int entnum, int entchannel, sfx_t * sfx, float* origin, float fvol, float attenuation, int flags, int pitch);
    extern void MyStartStaticSound(int entnum, int entchannel, sfx_t * sfx, float* origin, float fvol, float attenuation, int flags, int pitch);
    extern void MyStopSound(int entnum, int entchannel);
    extern void MyStopAllSounds(qboolean clear);
    extern bool VRInitSound();

    bool success =
        HookSoundMethod("StartStaticSound", gAudEngine.S_StartStaticSound, MyStartStaticSound) &&
        HookSoundMethod("StartDynamicSound", gAudEngine.S_StartDynamicSound, MyStartDynamicSound) &&
        HookSoundMethod("StopSound", gAudEngine.S_StopSound, MyStopSound) &&
        HookSoundMethod("StopAllSounds", gAudEngine.S_StopAllSounds, MyStopAllSounds);

    if (!success)
    {
        gEngfuncs.Con_DPrintf("Failed to hook into engine sound functions, FMOD will not be available.\n");
        gEngfuncs.Cvar_SetValue("vr_use_fmod", 0.f);
        g_soundHookFailed = true;
        return false;
    }

    g_soundHookFailed = false;
    return true;
}
