#pragma once

#include <exception>
#include <cstdint>
#include <stdarg.h>

#include "wrect.h"
#include "cl_dll.h"
#include "com_model.h"


#define MAX_QPATH  64   // max length of a quake game pathname
#define MAX_SFX  1024

typedef struct sfx_s
{
    char name[MAX_QPATH];
    cache_user_t cache;
    int servercount;
}sfx_t;

typedef struct sfxcache_s
{
    int length;
    int loopstart;
    int samplerate;
    int width;
    int stereo;
    byte data[1];  // variable sized
} sfxcache_t;

typedef struct
{
    sfx_t* sfx;
    float volume;
    float pitch;
    float attenuation;
    int entnum;
    int entchannel;
    vec3_t origin;
    uint64_t start;
    uint64_t end;
    //for sentence
    int isentence;
    int iword;
    //for voice sound
    sfxcache_t* voicecache;
}aud_channel_t;

typedef struct
{
    //wave info
    uint64_t length;
    uint64_t loopstart;
    uint64_t loopend;
    unsigned int samplerate;
    bool looping;
}aud_sfxcache_t;

typedef struct
{
    //s_dma.c
    void(*S_Startup)(void);
    void(*S_Init)(void);
    void(*S_Shutdown)(void);
    sfx_t* (*S_FindName)(char* name, int* pfInCache);
    sfx_t* (*S_PrecacheSound)(char* name);
    void(*SND_Spatialize)(aud_channel_t* ch);
    void(*S_Update)(float* origin, float* forward, float* right, float* up);

    void(*S_StartDynamicSound)(int entnum, int entchannel, sfx_t* sfx, float* origin, float fvol, float attenuation, int flags, int pitch);//hooked
    void(*S_StartStaticSound)(int entnum, int entchannel, sfx_t* sfx, float* origin, float fvol, float attenuation, int flags, int pitch);//hooked
    void(*S_StopSound)(int entnum, int entchannel);//hooked
    void(*S_StopAllSounds)(qboolean clear);//hooked

    //s_mem.c
    aud_sfxcache_t* (*S_LoadSound)(sfx_t* s, aud_channel_t* ch);
    //voice_sound_engine_interface.cpp
    void(*VoiceSE_NotifyFreeChannel)(int iChannel);
    void(*VoiceSE_Idle)(float frametime);
}aud_engine_t;

class SoundInitError : public std::exception
{
public:
    explicit SoundInitError(char const* const _Message) noexcept
        : std::exception{ _Message }
    {
    }
};

extern aud_engine_t gAudEngine;

extern bool g_soundHookFailed;

void S_FillAddress();
