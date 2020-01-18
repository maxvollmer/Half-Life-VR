
#include <string>
#include <algorithm>
#include <memory>
#include <vector>
#include <unordered_map>
#include <unordered_set>
#include <optional>

#include "snd_hook.h"
#include "VRSound.h"

#include "cl_entity.h"
#include "pm_defs.h"
#include "cl_util.h"

#include "fmod/inc/fmod.hpp"
#include "fmod/inc/fmod_errors.h"

#define DR_WAV_IMPLEMENTATION
#include "dr_wav.h"

#include "VRRenderer.h"

constexpr const float STATIC_CHANNEL_MAXDIST = 64000.f;
constexpr const float DYNAMIC_CHANNEL_MAXDIST = 1000.f;

constexpr const float HL_TO_METERS = 0.0254f;

constexpr const float VR_DEFAULT_FMOD_WALL_OCCLUSION = 0.4f;
constexpr const float VR_DEFAULT_FMOD_DOOR_OCCLUSION = 0.3f;
constexpr const float VR_DEFAULT_FMOD_WATER_OCCLUSION = 0.2f;
constexpr const float VR_DEFAULT_FMOD_GLASS_OCCLUSION = 0.1f;

constexpr const FMOD_VECTOR HL_TO_FMOD(const Vector& vec)
{
/*
HL.x = forward
HL.y = right
HL.z = up

FMOD.x = right
FMOD.y = up
FMOD.z = forwards
*/

	return FMOD_VECTOR{ vec.y * HL_TO_METERS, vec.z * HL_TO_METERS, vec.x * HL_TO_METERS };
}

struct LetHalfLifeHandleThisSoundException : public std::exception
{};


#define SND_VOLUME			(1 << 0)
#define SND_ATTENUATION		(1 << 1)
#define SND_LARGE_INDEX		(1 << 2)
#define SND_PITCH			(1 << 3)
#define SND_SENTENCE		(1 << 4)
#define SND_STOP			(1 << 5)
#define SND_CHANGE_VOL		(1 << 6)
#define SND_CHANGE_PITCH	(1 << 7)
#define SND_SPAWNING		(1 << 8)

namespace
{
	struct EntnumChannelSoundname
	{
		const int entnum;
		const int channel;
		const std::string soundname;

		EntnumChannelSoundname() :
			entnum{ -1 },
			channel{ -1 }
		{
		}

		EntnumChannelSoundname(int entnum, int channel, const std::string& soundname) :
			entnum{ entnum },
			channel{ channel },
			soundname{ soundname }
		{
		}

		struct Hash
		{
			std::size_t operator()(const EntnumChannelSoundname& e) const
			{
				std::hash<int> intHasher;
				std::hash<std::string> stringHasher;
				return intHasher(e.entnum) ^ intHasher(e.channel) ^ stringHasher(e.soundname);
			}
		};

		struct EqualTo
		{
			bool operator()(const EntnumChannelSoundname& e1, const EntnumChannelSoundname& e2) const
			{
				return e1.entnum == e2.entnum && e1.channel == e2.channel && e1.soundname == e2.soundname;
			}
		};
	};

	struct SoundInfo
	{
		std::string soundfile;
		float delayed_until = 0.f;
		float pitch = 0.f;
		float volume = 0.f;
		float attenuation = 0.f;
	};

	struct Sound
	{
		FMOD::Channel* fmodChannel;
		FMOD::Sound* fmodSound;
		bool startedpaused;
		float delayed_until = 0.f;				// Only set for sentences
		std::shared_ptr<Sound> nextSound;			// For sentences

		Sound() :
			fmodChannel{ nullptr },
			fmodSound{ nullptr },
			startedpaused{ false }
		{}

		Sound(FMOD::Channel* fmodChannel, FMOD::Sound* fmodSound, bool startedpaused) :
			fmodChannel{ fmodChannel },
			fmodSound{ fmodSound },
			startedpaused{ startedpaused }
		{}
	};

	FMOD::System* fmodSystem = nullptr;
	model_t* fmod_currentMapModel = nullptr;
	FMOD::Geometry* fmod_mapGeometry = nullptr;
	std::unordered_map<std::string, FMOD::Geometry*> fmod_geometries;
	std::unordered_map<EntnumChannelSoundname, Sound, EntnumChannelSoundname::Hash, EntnumChannelSoundname::EqualTo> allSounds;
	bool soundspaused = true;
}

void ReleaseAllGeometries()
{
	// fmod_mapGeometry is in fmod_geometries, don't release twice!
	for (auto& [modelname, geometry] : fmod_geometries)
	{
		if (geometry)
			geometry->release();
	}
	fmod_geometries.clear();
	fmod_mapGeometry = nullptr;
}

FMOD::Geometry* CreateFMODGeometryFromHLModel(model_t* model, float directocclusion, float reverbocclusion)
{
	if (!fmodSystem)
		return nullptr;

	auto modelgeometry = fmod_geometries.find(model->name);
	if (modelgeometry != fmod_geometries.end() && modelgeometry->second != nullptr)
	{
		return modelgeometry->second;
	}

	int numpolygons = 0;
	int numvertices = 0;
	std::vector<std::vector<FMOD_VECTOR>> polys;

	for (int i = 0; i < model->nummodelsurfaces; ++i)
	{
		glpoly_t* poly = model->surfaces[model->firstmodelsurface + i].polys;
		while (poly)
		{
			if (poly->numverts <= 0)
				continue;

			auto& verts = polys.emplace_back();
			for (int j = 0; j < poly->numverts; j++)
			{
				verts.push_back(HL_TO_FMOD(poly->verts[j]));
			}

			numvertices += poly->numverts;
			numpolygons++;

			poly = poly->next;

			// escape rings
			if (poly == model->surfaces[model->firstmodelsurface + i].polys)
				break;
		}
	}

	FMOD::Geometry* geometry = nullptr;
	FMOD_RESULT result = fmodSystem->createGeometry(numpolygons, numvertices, &geometry);
	int dummy_throwaway_polyindex = 0;
	if (result == FMOD_OK && geometry != nullptr)
	{
		for (auto& poly : polys)
		{
			FMOD_RESULT polyResult = geometry->addPolygon(directocclusion, reverbocclusion, true, static_cast<int>(poly.size()), poly.data(), &dummy_throwaway_polyindex);
			if (polyResult != FMOD_OK)
			{
				geometry->release();
				return nullptr;
			}
		}
		fmod_geometries[model->name] = geometry;
		return geometry;
	}

	if (geometry)
		geometry->release();

	return nullptr;
}

void UpdateGeometries()
{
	bool occlusion_enabled = CVAR_GET_FLOAT("vr_fmod_3d_occlusion") != 0.f;
	if (!occlusion_enabled && (fmod_mapGeometry || !fmod_geometries.empty()))
	{
		ReleaseAllGeometries();
	}

	if (occlusion_enabled)
	{
		float vr_fmod_wall_occlusion = CVAR_GET_FLOAT("vr_fmod_wall_occlusion") / 100.f;
		float vr_fmod_door_occlusion = CVAR_GET_FLOAT("vr_fmod_door_occlusion") / 100.f;
		float vr_fmod_water_occlusion = CVAR_GET_FLOAT("vr_fmod_water_occlusion") / 100.f;
		float vr_fmod_glass_occlusion = CVAR_GET_FLOAT("vr_fmod_glass_occlusion") / 100.f;

		if (!fmod_mapGeometry && fmod_currentMapModel && !fmod_currentMapModel->needload)
		{
			fmod_mapGeometry = CreateFMODGeometryFromHLModel(fmod_currentMapModel, vr_fmod_wall_occlusion, vr_fmod_wall_occlusion);
			if (!fmod_mapGeometry)
			{
				gEngfuncs.Con_DPrintf("Couldn't create FMOD geometry from map data, 3d occlusion is disabled.\n");
			}
		}

		extern playermove_t* pmove;
		if (fmod_mapGeometry && pmove)
		{
			std::unordered_set<std::string> audible_modelnames;

			for (int i = 0; i < pmove->numphysent; i++)
			{
				// not a brush model
				if (!pmove->physents[i].model || pmove->physents[i].model->name[0] != '*')
					continue;

				// default same as level
				float occlusion = vr_fmod_wall_occlusion;

				// if it moves, use door occlusion (should be a bit less than walls, but is user configurable)
				if (pmove->physents[i].movetype != MOVETYPE_NONE)
				{
					occlusion = vr_fmod_door_occlusion;
				}

				if (pmove->physents[i].solid == SOLID_NOT)
				{
					// not solid, no occlusion
					if (pmove->physents[i].skin < CONTENTS_LAVA || pmove->physents[i].skin > CONTENTS_WATER)
						continue;

					switch (pmove->physents[i].skin)
					{
					case CONTENTS_WATER:
						occlusion = vr_fmod_water_occlusion;			// water occludes very little
						break;
					case CONTENTS_SLIME:
						occlusion = vr_fmod_water_occlusion * 1.25f;	// slime occludes a bit more
						break;
					case CONTENTS_LAVA:
						occlusion = vr_fmod_water_occlusion * 1.5f;		// lava occludes even more a bit more
						break;
					}
				}
				else
				{
					if (pmove->physents[i].rendermode != 0)
					{
						// if rendermode is set assume glass
						occlusion = vr_fmod_glass_occlusion;
					}
				}

				// no occlusion, skip
				if (occlusion <= 0.f)
					continue;

				// sanitize occlusion
				if (occlusion > 1.f) occlusion = 1.f;

				// returns cached geometry if one already exists
				FMOD::Geometry* fmod_geometry = CreateFMODGeometryFromHLModel(pmove->physents[i].model, occlusion, occlusion);
				if (fmod_geometry)
				{
					// update position and rotation, and set active (covers cached geometries from entities that left and re-entered audible set)

					FMOD_VECTOR position = HL_TO_FMOD(pmove->physents[i].origin);

					Vector hlforward;
					Vector hlup;
					gEngfuncs.pfnAngleVectors(pmove->physents[i].angles, hlforward, nullptr, hlup);

					FMOD_VECTOR forward{ hlforward.x, hlforward.y, hlforward.z };
					FMOD_VECTOR up{ hlup.x, hlup.y, hlup.z };

					fmod_geometry->setPosition(&position);
					fmod_geometry->setRotation(&forward, &up);
					fmod_geometry->setActive(true);

					// keep track of all modelnames that are currently in audible set (we deactivate all other ones)
					audible_modelnames.insert(pmove->physents[i].model->name);
				}
			}

			for (auto& [modelname, fmod_geometry] : fmod_geometries)
			{
				// disable all geometries belonging to entities that left the audible set
				if (fmod_geometry != fmod_mapGeometry
					&& audible_modelnames.find(modelname) == audible_modelnames.end())
				{
					fmod_geometry->setActive(false);
				}
			}
		}
	}
}


std::string GetSoundFileForName(const std::string& name)
{
	if (name.empty())
		return name;

	if (name.substr(0, 6) == "sound/")
		return "./valve/" + name;
	else
		return "./valve/sound/" + name;
}

bool IsUISound(const std::string& name)
{
	return name.find("/sound/UI/") != name.npos;
}

bool IsPlaying(FMOD::Channel* fmodChannel)
{
	if (!fmodSystem)
		return false;

	if (!fmodChannel)
		return false;

	bool isPlaying = false;
	fmodChannel->isPlaying(&isPlaying);
	return isPlaying;
}

bool IsPlayerAudio(int entnum, float* origin)
{
	cl_entity_t* pent = gEngfuncs.GetEntityByIndex(entnum);
	if (pent == gEngfuncs.GetViewModel())
		return true;

	if (pent == SaveGetLocalPlayer())
		return true;

	if (!pent && (origin == nullptr || Vector(origin).LengthSquared() == 0.f))
		return true;

	return false;
}

void CloseMouth(int entnum)
{
	if (!fmodSystem)
		return;

	cl_entity_t* pent = gEngfuncs.GetEntityByIndex(entnum);
	if (pent)
	{
		pent->mouth.mouthopen = 0;
		pent->mouth.sndavg = 0;
		pent->mouth.sndcount = 0;
	}
}

void UpdateMouth(int entnum, Sound& sound)
{
	if (!fmodSystem)
		return;

	cl_entity_t* pent = gEngfuncs.GetEntityByIndex(entnum);
	if (!pent)
		return;

	FMOD::DSP* fft = nullptr;
	sound.fmodChannel->getDSP(FMOD_CHANNELCONTROL_DSP_HEAD, &fft);

	if (!fft)
	{
		CloseMouth(entnum);
		return;
	}

	FMOD_DSP_PARAMETER_FFT* data = nullptr;
	fft->getParameterData(FMOD_DSP_FFT_SPECTRUMDATA, (void**)&data, 0, 0, 0);
	if (!data)
	{
		CloseMouth(entnum);
		return;
	}

	float val = 0.f;
	int count = 0;
	for (int channel = 0; channel < data->numchannels; channel++)
	{
		for (int bin = 0; bin < data->length; bin++)
		{
			val += fabs(data->spectrum[channel][bin]);
			count++;
		}
	}
	float avg = val / count;

	pent->mouth.mouthopen = static_cast<byte>(avg * 255.f);
}

void UpdateSoundEffects(FMOD::Channel* fmodChannel, int entnum, bool playerUnderWater)
{
	if (!fmodSystem)
		return;

	// TODO: Get current reverb from env_sound (HUD Message!)

	// TODO
	/*
	if (underwater)
	{
		ch->source.setDirectFilter(alure::FilterParams{ direct_gain, AL_UNDERWATER_LP_GAIN * direct_gain, AL_HIGHPASS_DEFAULT_GAIN });
		ch->source.setDopplerFactor(AL_UNDERWATER_DOPPLER_FACTOR_RATIO);
	}
	else
	{
		ch->source.setDirectFilter(alure::FilterParams{ direct_gain, direct_gain, AL_HIGHPASS_DEFAULT_GAIN });
		ch->source.setDopplerFactor(1.0f);
	}
	*/
}

void UpdateSoundPosition(FMOD::Channel* fmodChannel, int entnum, float* florigin, bool onlyIfNew)
{
	if (!fmodSystem)
		return;

	FMOD_MODE mode = FMOD_DEFAULT;
	fmodChannel->getMode(&mode);
	if (!(mode & FMOD_3D))
		return;

	cl_entity_t* pent = gEngfuncs.GetEntityByIndex(entnum);
	if (!pent && !florigin)
		return;

	Vector origin;
	Vector velocity;
	if (florigin)
	{
		origin = florigin;
	}

	if (pent)
	{
		if (origin.LengthSquared() == 0.f)
		{
			origin = pent->curstate.origin;
		}
		velocity = pent->curstate.velocity;
	}

	if (origin.LengthSquared() == 0.f && velocity.LengthSquared() == 0.f)
		return;

	FMOD_VECTOR fmodPosition = HL_TO_FMOD(origin);
	FMOD_VECTOR fmodVelocity = HL_TO_FMOD(velocity);

	if (onlyIfNew)
	{
		FMOD_VECTOR previousFmodPosition{ 0.f, 0.f, 0.f };
		FMOD_VECTOR previousFmodVelocity{ 0.f, 0.f, 0.f };
		fmodChannel->get3DAttributes(&previousFmodPosition, &previousFmodVelocity);
		if (previousFmodPosition.x == fmodPosition.x && previousFmodPosition.y == fmodPosition.y && previousFmodPosition.z == fmodPosition.z
			&& previousFmodVelocity.x == fmodPosition.y && previousFmodVelocity.y == fmodVelocity.y && previousFmodVelocity.z == fmodVelocity.z)
			return;
	}

	fmodChannel->set3DAttributes(&fmodPosition, &fmodVelocity);
}

bool ParseLoopPoints(const std::string& filename, FMOD::Sound* fmodSound)
{
	if (!fmodSystem)
		return false;

	drwav wav;
	if (!drwav_init_file(&wav, filename.data(), nullptr))
		return false;

	bool isLooping = false;

	if (wav.smpl.numSampleLoops > 0)
	{
		fmodSound->setLoopPoints(wav.smpl.loops[0].start, FMOD_TIMEUNIT_RAWBYTES, wav.smpl.loops[0].end, FMOD_TIMEUNIT_RAWBYTES);
		isLooping = true;
	}
	else if (wav.cue.numCuePoints > 0)
	{
		if (wav.cue.numCuePoints > 1)
		{
			unsigned int loopstart = wav.cue.cuePoints[0].blockStart + wav.cue.cuePoints[0].sampleOffset;
			unsigned int loopend = wav.cue.cuePoints[1].blockStart + wav.cue.cuePoints[1].sampleOffset;
			fmodSound->setLoopPoints(loopstart, FMOD_TIMEUNIT_PCMBYTES, loopend, FMOD_TIMEUNIT_PCMBYTES);
		}
		else
		{
			unsigned int loopstart = wav.cue.cuePoints[0].blockStart + wav.cue.cuePoints[0].sampleOffset;
			unsigned int origloopstart = 0; unsigned int origloopend = 0;
			fmodSound->getLoopPoints(&origloopstart, FMOD_TIMEUNIT_PCMBYTES, &origloopend, FMOD_TIMEUNIT_PCMBYTES);
			fmodSound->setLoopPoints(loopstart, FMOD_TIMEUNIT_PCMBYTES, origloopend, FMOD_TIMEUNIT_PCMBYTES);
		}
		isLooping = true;
	}

	drwav_uninit(&wav);

	return isLooping;
}

std::vector<SoundInfo> ParseSentence(const std::string& sentence, float default_pitch, float default_volume, float default_attenuation)
{
	std::vector<SoundInfo> soundInfos;



	return soundInfos;
}

std::shared_ptr<Sound> CreateSound(const std::string& soundfile, bool is2DSound, bool isStaticChannel, int entnum, int entchannel, float* origin, float fpitch, float fvol, float attenuation, bool startpaused)
{
	if (!fmodSystem)
		return nullptr;

	FMOD::Sound* fmodSound = nullptr;
	FMOD_MODE fmodMode = is2DSound ? FMOD_2D : (FMOD_3D | FMOD_3D_WORLDRELATIVE | FMOD_3D_LINEARROLLOFF);
	fmodMode |= FMOD_CREATESAMPLE;
	FMOD_RESULT result1 = fmodSystem->createSound(soundfile.c_str(), fmodMode, nullptr, &fmodSound);
	if (result1 == FMOD_OK && fmodSound != nullptr)
	{
		bool isLooping = ParseLoopPoints(soundfile, fmodSound);
		if (isLooping)
		{
			fmodSound->setMode(fmodMode | FMOD_LOOP_NORMAL);
			// gEngfuncs.Con_DPrintf("[%f] LOOPING: %s, ent: %i, chan: %i, origin: %f %f %f, fvol: %f, attenuation: %f, flags: %i, pitch: %i\n", gVRRenderer.m_clientTime, sfx->name, entnum, entchannel, origin[0], origin[1], origin[2], fvol, attenuation, flags, pitch);
		}

		FMOD::Channel* fmodChannel = nullptr;
		FMOD_RESULT result2 = fmodSystem->playSound(fmodSound, nullptr, startpaused, &fmodChannel);
		if (result2 == FMOD_OK && fmodChannel != nullptr)
		{
			if (!is2DSound)
				UpdateSoundPosition(fmodChannel, entnum, origin, false);

			if (entchannel == CHAN_VOICE || entchannel == CHAN_STREAM)
			{
				FMOD::DSP* fft = nullptr;
				FMOD_RESULT result3 = fmodSystem->createDSPByType(FMOD_DSP_TYPE_FFT, &fft);
				if (result3 == FMOD_OK && fft != nullptr)
				{
					fmodChannel->addDSP(FMOD_CHANNELCONTROL_DSP_HEAD, fft);
				}
			}

			fmodChannel->setPitch(fpitch);
			fmodChannel->setVolume(fvol);

			if (attenuation > 0.f)
			{
				// different max dist on static and dynamic channels (see quake sourcecode)
				float maxdist = isStaticChannel ? STATIC_CHANNEL_MAXDIST : DYNAMIC_CHANNEL_MAXDIST;
				fmodChannel->set3DMinMaxDistance(1.f, (maxdist / attenuation) * HL_TO_METERS);
			}

			return std::make_shared<Sound>(fmodChannel, fmodSound, soundspaused);
		}
	}

	return nullptr;
}

void MyStartSound(int entnum, int entchannel, sfx_t* sfx, float* origin, float fvol, float attenuation, int flags, int pitch, bool isStaticChannel)
{
	if (!fmodSystem)
		return;

	if (!sfx)
		return;

	if (!sfx->name)
		return;

	if (!sfx->name[0])
		return;

	if (_stricmp(sfx->name, "common/null.wav") == 0)
		return;

	if (_stricmp(sfx->name, "!xxtestxx") == 0)
	{
		// This is a "speak" console command, no need to handle this with FMOD
		throw LetHalfLifeHandleThisSoundException{};
	}

	if (IsUISound(sfx->name))
	{
		// Let Half-Life handle UI sounds
		throw LetHalfLifeHandleThisSoundException{};
	}

	if ( sfx->name[0] == '?' || /*entchannel == CHAN_STREAM ||*/ (entchannel >= CHAN_NETWORKVOICE_BASE && entchannel <= CHAN_NETWORKVOICE_END))
	{
		// Let Half-Life handle streams
		gEngfuncs.Con_DPrintf("Got a stream sound, falling back to GoldSrc audio.\n");
		throw LetHalfLifeHandleThisSoundException{};
	}

	if (sfx->name[0] == '#')
	{
		// Let Half-Life handle sentences run by index (we should actually never get these anyways)
		gEngfuncs.Con_DPrintf("Got a sentence by index, falling back to GoldSrc audio.\n");
		throw LetHalfLifeHandleThisSoundException{};
	}

	if (fvol > 1.f) fvol = 1.f;
	float fpitch = pitch / 100.0f;

	auto& entnumChannelSound = allSounds.find(EntnumChannelSoundname{entnum, entchannel, sfx->name});
	// If sound already exists, modify
	if (entnumChannelSound != allSounds.end())
	{
		// Sound is playing, modify
		if (entnumChannelSound->second.fmodChannel && IsPlaying(entnumChannelSound->second.fmodChannel))
		{
			if (flags & SND_STOP)
			{
				// gEngfuncs.Con_DPrintf("[%f] SND_STOP:  ent: %i, chan: %i, soundname: %s\n", gVRRenderer.m_clientTime, entnum, entchannel, sfx->name);

				// Stop signal received, stop sound and forget
				entnumChannelSound->second.fmodChannel->stop();
				if (entnumChannelSound->second.fmodSound)
					entnumChannelSound->second.fmodSound->release();
				allSounds.erase(entnumChannelSound);
				CloseMouth(entnum);
				return;
			}
			else if (flags & (SND_CHANGE_VOL | SND_CHANGE_PITCH))
			{
				// gEngfuncs.Con_DPrintf("[%f] (SND_CHANGE_VOL | SND_CHANGE_PITCH): ent: %i, chan: %i, oldsound: %s, newsound: %s\n", gVRRenderer.m_clientTime, entnum, entchannel, entnumChannelSound->second.soundname.data(), sfx->name);
				// Modify signal received, modify sound and return

				if (flags & SND_CHANGE_PITCH)
					entnumChannelSound->second.fmodChannel->setPitch(fpitch);

				if (flags & SND_CHANGE_VOL)
					entnumChannelSound->second.fmodChannel->setVolume(fvol);

				UpdateMouth(entnum, entnumChannelSound->second);

				return;
			}
		}
		else
		{
			// gEngfuncs.Con_DPrintf("[%f] died: ent: %i, chan: %i, soundname: %s\n", gVRRenderer.m_clientTime, entnum, entchannel, sfx->name);
		}

		// Stop (if playing) and delete old sound

		if (entnumChannelSound->second.fmodChannel)
			entnumChannelSound->second.fmodChannel->stop();
		if (entnumChannelSound->second.fmodSound)
			entnumChannelSound->second.fmodSound->release();
		allSounds.erase(entnumChannelSound);
		CloseMouth(entnum);
	}

	if (flags & SND_STOP)
	{
		// Got stop signal, but already stopped or playing different audio, ignore
		// gEngfuncs.Con_DPrintf("[%f] SND_STOP ignored (already stopped): %s, ent: %i, chan: %i\n", gVRRenderer.m_clientTime, sfx->name, entnum, entchannel);
		return;
	}

	bool is2DSound = attenuation <= 0.f || IsPlayerAudio(entnum, origin);
	// gEngfuncs.Con_DPrintf("[%f] Playing new sound: %s, ent: %i, chan: %i, origin: %f %f %f, fvol: %f, attenuation: %f, flags: %i, pitch: %i, isStatic: %i\n", gVRRenderer.m_clientTime, sfx->name, entnum, entchannel, origin[0], origin[1], origin[2], fvol, attenuation, flags, pitch, isStaticChannel);

	// If we are here we have a new audiofile that needs to be played, if one already exists, we overwrite it

	// Sentences come in lists of audiofiles, so we treat normal audio playback as a list of size 1 for easier handling of both cases
	std::vector<SoundInfo> soundInfos;

	if (sfx->name[0] == '!')
	{
		soundInfos = ParseSentence(sfx->name + 1, fpitch, fvol, attenuation);
	}
	else
	{
		// Sounds that start with '*' are treated by GoldSrc as streaming sounds,
		// e.g. for "big" audiofiles that couldn't be safely loaded into a 4MB RAM computer back then.
		// We don't care and just load every file the same, so we simply drop the '*' from the name.
		std::string actualsoundfile = GetSoundFileForName(sfx->name[0] == '*' ? sfx->name + 1 : sfx->name);
		soundInfos.push_back(SoundInfo{
				actualsoundfile,
				0.f,
				fpitch,
				fvol,
				attenuation
			});
	}

	std::shared_ptr<Sound> previousSound;
	for (auto& soundInfo : soundInfos)
	{
		auto sound = CreateSound(soundInfo.soundfile, is2DSound, isStaticChannel, entnum, entchannel, origin, soundInfo.pitch, soundInfo.volume, soundInfo.attenuation, soundspaused || previousSound);
		if (sound)
		{
			if (previousSound)
			{
				previousSound->nextSound = sound;
			}
			else
			{
				allSounds[EntnumChannelSoundname{ entnum, entchannel, sfx->name }] = *sound;
			}
			previousSound = sound;
		}
	}

	// Start with closed mouth if playing new audio
	CloseMouth(entnum);
}

void MyStartDynamicSound(int entnum, int entchannel, sfx_t* sfx, float* origin, float fvol, float attenuation, int flags, int pitch)
{
	if (CVAR_GET_FLOAT("vr_use_fmod") == 0.f)
	{
		gAudEngine.S_StartDynamicSound(entnum, entchannel, sfx, origin, fvol, attenuation, flags, pitch);
		return;
	}

	try
	{
		MyStartSound(entnum, entchannel, sfx, origin, fvol, attenuation, flags, pitch, false);
	}
	catch (const LetHalfLifeHandleThisSoundException&)
	{
		gAudEngine.S_StartDynamicSound(entnum, entchannel, sfx, origin, fvol, attenuation, flags, pitch);
	}
}

void MyStartStaticSound(int entnum, int entchannel, sfx_t* sfx, float* origin, float fvol, float attenuation, int flags, int pitch)
{
	if (CVAR_GET_FLOAT("vr_use_fmod") == 0.f)
	{
		gAudEngine.S_StartStaticSound(entnum, entchannel, sfx, origin, fvol, attenuation, flags, pitch);
		return;
	}

	try
	{
		MyStartSound(entnum, entchannel, sfx, origin, fvol, attenuation, flags, pitch, true);
	}
	catch (const LetHalfLifeHandleThisSoundException&)
	{
		gAudEngine.S_StartStaticSound(entnum, entchannel, sfx, origin, fvol, attenuation, flags, pitch);
	}
}

void MyStopSound(int entnum, int entchannel)
{
	// gEngfuncs.Con_DPrintf("MyStopSound: %i, %i\n", entnum, entchannel);

	for (auto& [entnumChannelSoundname, sound] : allSounds)
	{
		if (entnumChannelSoundname.entnum == entnum && entnumChannelSoundname.channel == entchannel)
		{
			if (sound.fmodChannel)
				sound.fmodChannel->stop();
			if (sound.fmodSound)
				sound.fmodSound->release();

			sound.fmodChannel = nullptr;
			sound.fmodSound = nullptr;
			sound.startedpaused = false;
			sound.delayed_until = 0.f;
			sound.nextSound = nullptr;
		}
	}

	gAudEngine.S_StopSound(entnum, entchannel);
}

void MyStopAllSounds(qboolean clear)
{
	// gEngfuncs.Con_DPrintf("MyStopAllSounds\n");

	for (auto& [entnumChannelSoundname, sound] : allSounds)
	{
		if (sound.fmodChannel)
			sound.fmodChannel->stop();
		if (sound.fmodSound)
			sound.fmodSound->release();
	}
	allSounds.clear();

	gAudEngine.S_StopAllSounds(clear);
}

FMOD_3D_ATTRIBUTES GetFMODListenerAttributes()
{
	 FMOD_3D_ATTRIBUTES listenerattributes {
		 { 0.f, 0.f, 0.f },
		 { 0.f, 0.f, 0.f },
		 { 0.f, 0.f, 0.f },
		 { 0.f, 0.f, 0.f }
	 };

	 Vector listenerpos;
	 Vector listenerforward;
	 Vector listenerright;
	 Vector listenerup;
	 gVRRenderer.GetViewOrg(listenerpos);
	 gVRRenderer.GetViewVectors(listenerforward, listenerright, listenerup);

	 listenerattributes.position = HL_TO_FMOD(listenerpos);
	 listenerattributes.forward = FMOD_VECTOR{ listenerforward.x, listenerforward.y, listenerforward.z };
	 listenerattributes.up = FMOD_VECTOR{ listenerup.x, listenerup.y, listenerup.z };

	 cl_entity_t* localPlayer = SaveGetLocalPlayer();
	 if (localPlayer)
	 {
		 listenerattributes.velocity = HL_TO_FMOD(localPlayer->curstate.velocity);
	 }

	 return listenerattributes;
}

bool VRInitSound()
{
	if (CVAR_GET_FLOAT("vr_use_fmod") == 0.f)
		return false;

	if (g_soundHookFailed)
	{
		gEngfuncs.Con_DPrintf("Can't use FMOD, as hooking into engine sound functions has failed during game initialization.\n");
		gEngfuncs.Cvar_SetValue("vr_use_fmod", 0.f);
		return false;
	}

	if (fmodSystem)
	{
		gEngfuncs.Con_DPrintf("Couldn't create FMOD system, it already has been created. This should never happen, and if it did happen, congratulations, you broke reality.\n");
		return false;
	}

	// Stop all engine sounds, in case user switches to FMOD during play
	MyStopAllSounds(1);

	FMOD_RESULT result = FMOD::System_Create(&fmodSystem);
	if (result != FMOD_OK)
	{
		gEngfuncs.Con_DPrintf("Couldn't create FMOD system, falling back to GoldSrc audio. Error %i: %s\n", result, FMOD_ErrorString(result));
		gEngfuncs.Cvar_SetValue("vr_use_fmod", 0.f);
		return false;
	}

	fmodSystem->setGeometrySettings(8192.f * HL_TO_METERS);

	// TODO:
	// FMOD_ADVANCEDSETTINGS settings;
	// fmodSystem->setAdvancedSettings(&settings);

	result = fmodSystem->init(2000, FMOD_INIT_NORMAL, 0);
	if (result != FMOD_OK)
	{
		gEngfuncs.Con_DPrintf("Couldn't initialize FMOD system, falling back to GoldSrc audio. Error %i: %s\n", result, FMOD_ErrorString(result));
		gEngfuncs.Cvar_SetValue("vr_use_fmod", 0.f);
		return false;
	}

	return true;
}

void VRSoundUpdate(bool paused)
{
	if (CVAR_GET_FLOAT("vr_use_fmod") == 0.f)
	{
		if (fmodSystem)
		{
			fmodSystem->release();
			fmodSystem = nullptr;
			fmod_currentMapModel = nullptr;
			fmod_mapGeometry = nullptr;
			fmod_geometries.clear();
			allSounds.clear();
			soundspaused = true;
		}
		return;
	}

	if (!fmodSystem)
	{
		VRInitSound();
	}

	if (!fmodSystem)
		return;

	if (paused != soundspaused)
	{
		cl_entity_s* map = gEngfuncs.GetEntityByIndex(0);
		if (map == nullptr
			|| map->model == nullptr
			|| map->model->needload
			|| gEngfuncs.pfnGetLevelName() == nullptr
			|| _stricmp(gEngfuncs.pfnGetLevelName(), map->model->name) != 0)
		{
			// No map yet, stay paused
			paused = true;
			if (fmod_currentMapModel)
			{
				// If we had a map before, clear all sounds and clear map
				// MyStopAllSounds(1);
				ReleaseAllGeometries();
				fmod_currentMapModel = nullptr;
			}
		}

		if (!paused)
		{
			// new level, stop and clear all sounds
			if (!fmod_currentMapModel || map->model != fmod_currentMapModel)
			{
				fmod_currentMapModel = map->model;
			}
		}

		for (auto& [entnumChannel, sound] : allSounds)
		{
			sound.fmodChannel->setPaused(paused);
		}

		soundspaused = paused;
	}

	if (!soundspaused)
	{
		FMOD_3D_ATTRIBUTES listenerattributes = GetFMODListenerAttributes();
		fmodSystem->set3DListenerAttributes(0, &listenerattributes.position, &listenerattributes.velocity, &listenerattributes.forward, &listenerattributes.up);

		bool isPlayerUnderWater = false;
		extern playermove_t* pmove;
		if (pmove)
		{
			Vector playerhead;
			gVRRenderer.GetViewOrg(playerhead);
			int playerheadcontents = pmove->PM_PointContents(playerhead, nullptr);
			isPlayerUnderWater = (pmove->waterlevel > 2) || (playerheadcontents <= CONTENTS_WATER && playerheadcontents > CONTENTS_TRANSLUCENT);
		}

		for (auto& [entnumChannel, sound] : allSounds)
		{
			UpdateSoundPosition(sound.fmodChannel, entnumChannel.entnum, nullptr, true);
			UpdateSoundEffects(sound.fmodChannel, entnumChannel.entnum, isPlayerUnderWater);
			if (entnumChannel.channel == CHAN_VOICE)
			{
				UpdateMouth(entnumChannel.entnum, sound);
			}
		}

		UpdateGeometries();
	}

	fmodSystem->update();
}
