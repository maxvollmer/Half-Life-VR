
#include <iostream>
#include <fstream>
#include <string>
#include <algorithm>
#include <memory>
#include <vector>
#include <unordered_map>
#include <unordered_set>
#include <optional>
#include <regex>

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

#define SND_VOLUME			(1 << 0)
#define SND_ATTENUATION		(1 << 1)
#define SND_LARGE_INDEX		(1 << 2)
#define SND_PITCH			(1 << 3)
#define SND_SENTENCE		(1 << 4)
#define SND_STOP			(1 << 5)
#define SND_CHANGE_VOL		(1 << 6)
#define SND_CHANGE_PITCH	(1 << 7)
#define SND_SPAWNING		(1 << 8)

// global for VRClientDLLUnloadHandler
bool g_fmodIgnoreEverythingWeAreShuttingDown = false;

namespace
{
	// We have so much stuff here, probs would make sense to actually create a VRSound class, but oh well

	constexpr const float STATIC_CHANNEL_MAXDIST = 64000.f;
	constexpr const float DYNAMIC_CHANNEL_MAXDIST = 1000.f;

	constexpr const float HL_TO_METERS = 0.0254f;

	constexpr const float VR_DEFAULT_FMOD_WALL_OCCLUSION = 0.4f;
	constexpr const float VR_DEFAULT_FMOD_DOOR_OCCLUSION = 0.3f;
	constexpr const float VR_DEFAULT_FMOD_WATER_OCCLUSION = 0.2f;
	constexpr const float VR_DEFAULT_FMOD_GLASS_OCCLUSION = 0.1f;

	constexpr const float VOX_PERIOD_PAUSETIME = 0.43f;
	constexpr const float VOX_COMMA_PAUSETIME = 0.25f;

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

	enum StartSoundResult
	{
		Good,
		NotGood,
		LetHalfLifeHandleThisSound
	};

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

		float delayed_by = 0.f;
		float start_offset = 0.f;
		float end_offset = 0.f;
		float timecompress = 0.f;

		float pitch = 0.f;
		float volume = 0.f;
	};

	struct Sound
	{
		FMOD::Channel* fmodChannel{ nullptr };
		FMOD::Sound* fmodSound{ nullptr };
		FMOD::DSP* timestretchDSP{ nullptr };
		FMOD::DSP* fftDSP{ nullptr };

		std::shared_ptr<Sound> nextSound;		// For sentences
		float delayed_by{ 0.f };				// Only set for sentences
		float end_offset{ 0.f };				// Only set for sentences
		double time_marker{ -1.0 };				// Is set as soon as VRSoundUpdate touches this sound for the first time
		bool has_started{ false };				// Set to true when VRSoundUpdate starts playing this sound for the first time

		void Release()
		{
			if (!g_fmodIgnoreEverythingWeAreShuttingDown)
			{
				if (timestretchDSP)
				{
					if (fmodChannel) fmodChannel->removeDSP(timestretchDSP);
					timestretchDSP->release();
				}
				if (fftDSP)
				{
					if (fmodChannel) fmodChannel->removeDSP(fftDSP);
					fftDSP->release();
				}
				if (fmodChannel) fmodChannel->stop();
				if (fmodSound) fmodSound->release();
			}

			fmodChannel = nullptr;
			fmodSound = nullptr;
			timestretchDSP = nullptr;
			fftDSP = nullptr;
		}

		Sound() {}

		Sound(FMOD::Channel* fmodChannel, FMOD::Sound* fmodSound, FMOD::DSP* timestretchDSP, FMOD::DSP* fftDSP, bool startedpaused) :
			fmodChannel{ fmodChannel },
			fmodSound{ fmodSound },
			timestretchDSP{ timestretchDSP },
			fftDSP{ fftDSP }
		{}
	};

	FMOD::System* fmodSystem = nullptr;
	model_t* fmod_currentMapModel = nullptr;
	FMOD::Geometry* fmod_mapGeometry = nullptr;
	std::unordered_map<std::string, FMOD::Geometry*> fmod_geometries;
	std::unordered_map<EntnumChannelSoundname, Sound, EntnumChannelSoundname::Hash, EntnumChannelSoundname::EqualTo> g_allSounds;
	bool g_soundspaused = true;
	double g_soundtime = 0.0;

	std::unordered_map<std::string, std::string> g_allSentences;				// This contains all sentences from valve/sound/sentences.txt and vr/sound/sentences.txt, where the latter overrides the former
	std::unordered_map<std::string, std::vector<SoundInfo>> g_parsedSentences;	// This is a cache for all sentences that are in use.
}


#ifdef DEBUG_FMOD_MEMORY
namespace
{
	std::unordered_set<void*> g_keepTrackOfMemory;
	int alloccalls = 0;
	int freecalls = 0;
	int realloccalls = 0;

	void* F_CALLBACK FMODMemoryAlloc(unsigned int size, FMOD_MEMORY_TYPE type, const char* sourcestr)
	{
		alloccalls++;
		void* result = std::malloc(size);
		g_keepTrackOfMemory.insert(result);
		return result;
	}
	void* F_CALLBACK FMODMemoryRealloc(void* ptr, unsigned int size, FMOD_MEMORY_TYPE type, const char* sourcestr)
	{
		realloccalls++;
		void* result = std::realloc(ptr, size);
		if (ptr != result)
		{
			g_keepTrackOfMemory.erase(ptr);
			g_keepTrackOfMemory.insert(result);
		}
		gEngfuncs.Con_DPrintf("fmod_realloc: %i (%i, %i, %i, %i)\n", (int)g_keepTrackOfMemory.size(), alloccalls, realloccalls, freecalls, (alloccalls - freecalls));
		return result;
	}
	void F_CALLBACK FMODMemoryFree(void* ptr, FMOD_MEMORY_TYPE type, const char* sourcestr)
	{
		freecalls++;
		std::free(ptr);
		g_keepTrackOfMemory.erase(ptr);
		gEngfuncs.Con_DPrintf("fmod_free: %i (%i, %i, %i, %i)\n", (int)g_keepTrackOfMemory.size(), alloccalls, realloccalls, freecalls, (alloccalls - freecalls));
	}
}
#endif


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

std::string GetFullSoundFilePath(const std::string& name)
{
	if (name.empty())
		return name;

	std::string vrfilename;
	std::string valvefilename;

	if (name.substr(0, 6) == "sound/")
	{
		vrfilename = "./vr/" + name;
		valvefilename = "./valve/" + name;
	}
	else
	{
		vrfilename = "./vr/sound/" + name;
		valvefilename = "./valve/sound/" + name;
	}

	if (std::ifstream{ vrfilename }.good())
	{
		return vrfilename;
	}
	else if(std::ifstream{ valvefilename }.good())
	{
		return valvefilename;
	}
	else
	{
		return "";
	}
}

bool IsUISound(const std::string& name)
{
	return name.find("sound/UI/") == 0;
}

bool IsPlaying(FMOD::Channel* fmodChannel)
{
	if (!fmodSystem)
		return false;

	if (!fmodChannel)
		return false;

	bool isPlaying = false;
	FMOD_RESULT result = fmodChannel->isPlaying(&isPlaying);
	return result == FMOD_OK && isPlaying;
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

	if (!sound.fftDSP)
	{
		CloseMouth(entnum);
		return;
	}

	FMOD_DSP_PARAMETER_FFT* data = nullptr;
	FMOD_RESULT result2 = sound.fftDSP->getParameterData(FMOD_DSP_FFT_SPECTRUMDATA, (void**)&data, 0, 0, 0);
	if (result2 != FMOD_OK || !data)
	{
		CloseMouth(entnum);
		return;
	}

	double val = 0.f;
	int count = 0;
	for (int channel = 0; channel < data->numchannels; channel++)
	{
		for (int bin = 0; bin < data->length; bin++)
		{
			val += fabs(data->spectrum[channel][bin]);
			count++;
		}
	}
	if (count > 0 && data->numchannels > 0)
	{
		double avg = val / data->numchannels;
		if (avg < 0.f) avg = 0.f;
		if (avg > 1.f) avg = 1.f;
		pent->mouth.mouthopen = static_cast<byte>(avg * 255.0);
	}
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

void LoadAllSentences(const std::string& filename)
{
	static const std::regex sentenceRegex{ "^\\s*([a-zA-Z0-9_]+)\\s+(.*)$" };

	std::ifstream file;
	file.open(filename);
	if (file.is_open())
	{
		std::string line;
		while (std::getline(file, line))
		{
			std::smatch match;
			if (std::regex_match(line, match, sentenceRegex) && match.size() == 3)
			{
				std::string sentenceName = match[1].str();
				std::string sentence = match[2].str();
				g_allSentences[sentenceName] = sentence;
			}
		}
		file.close();
	}
}

void LoadAllSentences()
{
	if (!g_allSentences.empty())
		return;

	LoadAllSentences("./valve/sound/sentences.txt");
	LoadAllSentences("./vr/sound/sentences.txt");

	if (g_allSentences.empty())
	{
		gEngfuncs.Con_DPrintf("Couldn't load sentences.txt. NPC conversations, HEV suit messages and Black Mesa speaker announcements won't play while using FMOD. Disable FMOD to hear these sounds.\n");
	}
}

std::vector<std::string> GetSentenceTokens(const std::string& sentence)
{
	// sentences look like this: optionalpath/firstfile(optionalparameters) secondfile(optionalparameters) thirdfile(optionalparameters) fourthfile(optionalparameters) etc(etc)
	// (optional parameters) may contain spaces, so we need to split along spaces, except if inside brackets
	static const std::regex sentenceSplitRegex{ "\\s+(?![^(]*\\))" };

	std::vector<std::string> sentenceTokens;
	std::copy(
		std::sregex_token_iterator{ sentence.begin(), sentence.end(), sentenceSplitRegex, -1 },
		std::sregex_token_iterator{},
		std::back_inserter(sentenceTokens));

	return sentenceTokens;
}

std::string RemovePathFromSentence(const std::string& sentence, std::string& path)
{
	// sentences look like this: optionalpath/firstfile(optionalparameters) secondfile(optionalparameters) etc(etc)
	// here we look for optionalpath/
	static const std::regex pathExtractRegex{ "^\\s*([^\\s^/]+/)(.+)$" };
	std::smatch match;
	if (std::regex_match(sentence, match, pathExtractRegex))
	{
		// optionalpath/ exists, set it, and return the sentence from after the /
		path = match[1];
		return match[2];
	}
	else
	{
		// optionalpath/ doesn't exist, set default vox, and return the original unmodified sentence
		path = "vox/";
		return sentence;
	}
}

void ParseVOXParams(const std::string& params, float* pitch, float* volume, float* start_offset, float* end_offset, float* timecompress)
{
	// params look like this: (CXXX CXXX CXXX)
	// where C is any of the parameters below and XXX is an integer
	/*
	parameters
	p -> pitch
	v -> volume
	s -> start (offset start of wave percent)
	e -> end (offset end of wave percent)
	t -> timecompress (% of wave that GoldSrc skips, we transform this into a timestretch multiplier value)
	*/

	static const std::regex paramsRegex{ "\\((?:([pvset][0-9]+)\\s*)+\\)" };
	std::smatch match;
	if (std::regex_match(params, match, paramsRegex))
	{
		for (size_t i = 1; i < match.size(); i++)
		{
			std::string param = match[1];
			switch (param[0])
			{
			case 'p':
				*pitch = std::atoi(&param[1]) / 100.f;
				break;
			case 'v':
				*volume = std::atoi(&param[1]) / 100.f;
				break;
			case 's':
				*start_offset = std::atoi(&param[1]) / 100.f;
				break;
			case 'e':
				*end_offset = std::atoi(&param[1]) / 100.f;
				break;
			case 't':
				*timecompress = std::atoi(&param[1]) / 100.f;
				break;
			}
		}
	}
}

std::string ParseVOX(const std::string& sound, float* next_delayed_by)
{
	/*
	special characters
	',' -> 250ms pause
	'.' -> 430ms pause
	*/

	if (sound.rbegin()[0] == '.')
	{
		*next_delayed_by = VOX_PERIOD_PAUSETIME;
		return sound.substr(0, sound.size() - 1);
	}
	else if (sound.rbegin()[0] == ',')
	{
		*next_delayed_by = VOX_COMMA_PAUSETIME;
		return sound.substr(0, sound.size() - 1);
	}
	else
	{
		*next_delayed_by = 0.f;
		return sound;
	}
}

std::vector<SoundInfo> ParseSentence(const std::string& sentenceName, float initial_pitch, float initial_volume)
{
	auto parsedSentence = g_parsedSentences.find(sentenceName);
	if (parsedSentence != g_parsedSentences.end())
		return parsedSentence->second;

	auto sentencePair = g_allSentences.find(sentenceName);
	if (sentencePair != g_allSentences.end())
	{
		std::string path;
		std::string sentence = RemovePathFromSentence(sentencePair->second, path);

#ifdef _DEBUG
		gEngfuncs.Con_DPrintf("sentence name: %s, full sentence: %s, path: %s, sentence wo path: %s\n", sentenceName.c_str(), sentencePair->second.c_str(), path.c_str(), sentence.c_str());
#endif

		std::vector<SoundInfo> soundInfos;

		float default_pitch = initial_pitch;
		float default_volume = initial_volume;
		float default_start_offset = 0.f; 
		float default_end_offset = 0.f; 
		float default_timecompress = 0.f;
		float next_delayed_by = 0.f;

		std::vector<std::string> sentenceTokens = GetSentenceTokens(sentence);
		for (std::string token : sentenceTokens)
		{
			// token is either name of audio file or params or name of audio file with params
			if (token[0] == '(')
			{
				// token is params if it starts with '('
				ParseVOXParams(token, &default_pitch, &default_volume, &default_start_offset, &default_end_offset, &default_timecompress);
			}
			else
			{
				// token is name of audio file with params, or just name of audio file
				// we use a regex with an optional 2nd capture group for the params
				// if no params exist, that group will simply give us an empty result, and ParseVOXParams will be a no-op
				static const std::regex tokenRegex{ "^([^\\(]+)(\\([^\\)]+\\))?$" };
				std::smatch match;
				if (std::regex_match(token, match, tokenRegex))
				{
					SoundInfo soundInfo;
					soundInfo.delayed_by = next_delayed_by;
					soundInfo.soundfile = path + ParseVOX(match[1], &next_delayed_by) + ".wav";

					soundInfo.pitch = default_pitch;
					soundInfo.volume = default_volume;
					soundInfo.start_offset = default_start_offset;
					soundInfo.end_offset = default_end_offset;
					soundInfo.timecompress = default_timecompress;

					ParseVOXParams(match[2], &soundInfo.pitch, &soundInfo.volume, &soundInfo.start_offset, &soundInfo.end_offset, &soundInfo.timecompress);

					soundInfos.push_back(soundInfo);
				}
			}
		}

		g_parsedSentences[sentenceName] = soundInfos;
		return soundInfos;
	}

	return {};
}

std::shared_ptr<Sound> CreateSound(
	const std::string& soundfile,
	bool is2DSound,
	bool isStaticChannel,
	int entnum,
	int entchannel,
	float* origin,
	float fpitch,
	float fvol,
	float attenuation,
	float start_offset,
	float timecompress)
{
	if (!fmodSystem)
		return nullptr;

	if (start_offset >= 1.f)
		return nullptr;

	if (timecompress >= 1.f)
		return nullptr;

	std::string actualsoundfile = GetFullSoundFilePath(soundfile);
	if (actualsoundfile.empty())
		return nullptr;

	FMOD::Sound* fmodSound = nullptr;
	FMOD_MODE fmodMode = is2DSound ? FMOD_2D : (FMOD_3D | FMOD_3D_WORLDRELATIVE | FMOD_3D_LINEARROLLOFF);
	fmodMode |= FMOD_CREATESAMPLE;
	FMOD_RESULT result1 = fmodSystem->createSound(actualsoundfile.c_str(), fmodMode, nullptr, &fmodSound);
	if (result1 == FMOD_OK && fmodSound != nullptr)
	{
		bool isLooping = ParseLoopPoints(actualsoundfile, fmodSound);
		if (isLooping)
		{
			fmodSound->setMode(fmodMode | FMOD_LOOP_NORMAL);
		}

		FMOD::Channel* fmodChannel = nullptr;
		FMOD_RESULT result2 = fmodSystem->playSound(fmodSound, nullptr, true, &fmodChannel);

		if (result2 == FMOD_OK && fmodChannel != nullptr)
		{
			if (!is2DSound)
				UpdateSoundPosition(fmodChannel, entnum, origin, false);

			if (attenuation > 0.f)
			{
				// different max dist on static and dynamic channels (see quake sourcecode)
				float maxdist = isStaticChannel ? STATIC_CHANNEL_MAXDIST : DYNAMIC_CHANNEL_MAXDIST;
				fmodChannel->set3DMinMaxDistance(1.f, (maxdist / attenuation) * HL_TO_METERS);
			}

			fmodChannel->setPitch(fpitch);
			fmodChannel->setVolume(fvol);

			FMOD::DSP* timestretchDSP = nullptr;
			FMOD::DSP* fftDSP = nullptr;

			if (timecompress > 0.f)
			{
				FMOD_RESULT result3 = fmodSystem->createDSPByType(FMOD_DSP_TYPE_PITCHSHIFT, &timestretchDSP);
				if (result3 == FMOD_OK && timestretchDSP != nullptr)
				{
					float timestretch = 1.f - timecompress;
					timestretchDSP->setParameterFloat(0, 1.f / timestretch);	// pitch (counters timestretch)
					timestretchDSP->setParameterFloat(1, 4096);					// buffer size
					fmodChannel->addDSP(FMOD_CHANNELCONTROL_DSP_HEAD, timestretchDSP);
					fmodChannel->setPitch(timestretch * fpitch);
				}
			}

			if (entchannel == CHAN_VOICE || entchannel == CHAN_STREAM)
			{
				FMOD_RESULT result3 = fmodSystem->createDSPByType(FMOD_DSP_TYPE_FFT, &fftDSP);
				if (result3 == FMOD_OK && fftDSP != nullptr)
				{
					fmodChannel->addDSP(FMOD_CHANNELCONTROL_DSP_HEAD, fftDSP);
				}
				fmodChannel->setPriority(0); // give channel highest priority, so it won't be stolen by some random sound
			}

			if (start_offset > 0.f)
			{
				unsigned int length = 0;
				fmodSound->getLength(&length, FMOD_TIMEUNIT_MS);
				fmodChannel->setPosition(length * start_offset, FMOD_TIMEUNIT_MS);
			}

			return std::make_shared<Sound>(fmodChannel, fmodSound, timestretchDSP, fftDSP, g_soundspaused);
		}
	}

	return nullptr;
}

void MyStopSound(int entnum, int entchannel)
{
	for (auto& [entnumChannelSoundname, sound] : g_allSounds)
	{
		if (entnumChannelSoundname.entnum == entnum && entnumChannelSoundname.channel == entchannel)
		{
			sound.Release();
		}
	}

	gAudEngine.S_StopSound(entnum, entchannel);
}

void MyStopAllSounds(qboolean clear)
{
	for (auto& [entnumChannelSoundname, sound] : g_allSounds)
	{
		sound.Release();
	}
	g_allSounds.clear();

	gAudEngine.S_StopAllSounds(clear);

	g_soundtime = 0.0;
}

StartSoundResult MyStartSound(int entnum, int entchannel, sfx_t* sfx, float* origin, float fvol, float attenuation, int flags, int pitch, bool isStaticChannel)
{
	if (!fmodSystem)
		return StartSoundResult::NotGood;

	if (!sfx)
		return StartSoundResult::NotGood;

	if (!sfx->name)
		return StartSoundResult::NotGood;

	if (!sfx->name[0])
		return StartSoundResult::NotGood;

	if (_stricmp(sfx->name, "common/null.wav") == 0)
		return StartSoundResult::NotGood;

	if (_stricmp(sfx->name, "!xxtestxx") == 0)
	{
		// This is a "speak" console command, no need to handle this with FMOD
		return StartSoundResult::LetHalfLifeHandleThisSound;
	}

	if (IsUISound(sfx->name))
	{
		// Let Half-Life handle UI sounds
		return StartSoundResult::LetHalfLifeHandleThisSound;
	}

	if ( sfx->name[0] == '?' || /*entchannel == CHAN_STREAM ||*/ (entchannel >= CHAN_NETWORKVOICE_BASE && entchannel <= CHAN_NETWORKVOICE_END))
	{
		// Let Half-Life handle streams
#ifdef _DEBUG
		gEngfuncs.Con_DPrintf("Got a stream sound, falling back to GoldSrc audio.\n");
#endif
		return StartSoundResult::LetHalfLifeHandleThisSound;
	}

	if (sfx->name[0] == '#')
	{
		// Let Half-Life handle sentences run by index (we should actually never get these anyways)
#ifdef _DEBUG
		gEngfuncs.Con_DPrintf("Got a sentence by index, falling back to GoldSrc audio.\n");
#endif
		return StartSoundResult::LetHalfLifeHandleThisSound;
	}

	if (fvol > 1.f) fvol = 1.f;
	float fpitch = pitch / 100.0f;

	auto& entnumChannelSound = g_allSounds.find(EntnumChannelSoundname{entnum, entchannel, sfx->name});
	// If sound already exists, modify
	if (entnumChannelSound != g_allSounds.end())
	{
		// Sound is playing, modify
		if (entnumChannelSound->second.fmodChannel && IsPlaying(entnumChannelSound->second.fmodChannel))
		{
			if (flags & SND_STOP)
			{
				// Stop signal received, stop sound and forget
				entnumChannelSound->second.Release();
				g_allSounds.erase(entnumChannelSound);
				CloseMouth(entnum);
				return StartSoundResult::Good;
			}
			else if (flags & (SND_CHANGE_VOL | SND_CHANGE_PITCH))
			{
				// Modify signal received, modify sound and return

				if (flags & SND_CHANGE_PITCH)
					entnumChannelSound->second.fmodChannel->setPitch(fpitch);

				if (flags & SND_CHANGE_VOL)
					entnumChannelSound->second.fmodChannel->setVolume(fvol);

				UpdateMouth(entnum, entnumChannelSound->second);

				return StartSoundResult::Good;
			}
		}

		// Stop (if playing) and delete old sound
		entnumChannelSound->second.Release();
		g_allSounds.erase(entnumChannelSound);
		CloseMouth(entnum);
	}

	if (flags & SND_STOP)
	{
		// Got stop signal, but already stopped or playing different audio, ignore
		return StartSoundResult::Good;
	}

	bool is2DSound = attenuation <= 0.f || IsPlayerAudio(entnum, origin);

	// If we are here we have a new audiofile that needs to be played
	// if this is a voice, we stop all previous voices first
	if (entchannel == CHAN_VOICE || entchannel == CHAN_STREAM)
	{
		MyStopSound(entnum, entchannel);
	}

	// Sentences come in lists of audiofiles, so we treat normal audio playback as a list of size 1 for easier handling of both cases
	std::vector<SoundInfo> soundInfos;

	if (sfx->name[0] == '!')
	{
		soundInfos = ParseSentence(sfx->name + 1, fpitch, fvol);
	}
	else
	{
		// Sounds that start with '*' are treated by GoldSrc as streaming sounds,
		// e.g. for "big" audiofiles that couldn't be safely loaded into a 4MB RAM computer back then.
		// We don't care and just load every file the same, so we simply drop the '*' from the name.
		std::string soundfile = sfx->name[0] == '*' ? sfx->name + 1 : sfx->name;
		soundInfos.push_back(SoundInfo{
				soundfile,

				0.f,
				0.f,
				0.f,
				0.f,

				fpitch,
				fvol
			});
	}

	std::shared_ptr<Sound> firstSound;
	std::shared_ptr<Sound> previousSound;
	for (auto& soundInfo : soundInfos)
	{
		auto sound = CreateSound(
			soundInfo.soundfile,
			is2DSound,
			isStaticChannel,
			entnum,
			entchannel,
			origin,
			soundInfo.pitch,
			soundInfo.volume,
			attenuation,
			soundInfo.start_offset,
			soundInfo.timecompress
		);

		if (sound)
		{
			sound->delayed_by = soundInfo.delayed_by;
			sound->end_offset = soundInfo.end_offset;

			if (!firstSound)
			{
				firstSound = sound;
			}
			if (previousSound)
			{
				previousSound->nextSound = sound;
			}
			else
			{
				previousSound = sound;
			}

			previousSound = sound;
		}
	}
	if (firstSound)
	{
		g_allSounds[EntnumChannelSoundname{ entnum, entchannel, sfx->name }] = *firstSound;
	}

	// Start with closed mouth if playing new audio
	CloseMouth(entnum);

	return StartSoundResult::Good;
}

void MyStartDynamicSound(int entnum, int entchannel, sfx_t* sfx, float* origin, float fvol, float attenuation, int flags, int pitch)
{
	if (CVAR_GET_FLOAT("vr_use_fmod") == 0.f)
	{
		gAudEngine.S_StartDynamicSound(entnum, entchannel, sfx, origin, fvol, attenuation, flags, pitch);
		return;
	}

	StartSoundResult result = MyStartSound(entnum, entchannel, sfx, origin, fvol, attenuation, flags, pitch, false);
	if (result == StartSoundResult::LetHalfLifeHandleThisSound)
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

	StartSoundResult result = MyStartSound(entnum, entchannel, sfx, origin, fvol, attenuation, flags, pitch, true);
	if (result == StartSoundResult::LetHalfLifeHandleThisSound)
	{
		gAudEngine.S_StartStaticSound(entnum, entchannel, sfx, origin, fvol, attenuation, flags, pitch);
	}
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

bool VRInitFMOD()
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

	FMOD_RESULT result = FMOD_OK;

#ifdef DEBUG_FMOD_MEMORY
	result = FMOD::Memory_Initialize(0, 0, FMODMemoryAlloc, FMODMemoryRealloc, FMODMemoryFree);
	if (result != FMOD_OK)
	{
		gEngfuncs.Con_DPrintf("Couldn't initialize FMOD memory. Error %i: %s\n", result, FMOD_ErrorString(result));
	}
#endif

	// Stop all engine sounds, in case user switches to FMOD during play
	MyStopAllSounds(1);

	result = FMOD::System_Create(&fmodSystem);
	if (result != FMOD_OK)
	{
		gEngfuncs.Con_DPrintf("Couldn't create FMOD system, falling back to GoldSrc audio. Error %i: %s\n", result, FMOD_ErrorString(result));
		gEngfuncs.Cvar_SetValue("vr_use_fmod", 0.f);
		return false;
	}

	result = fmodSystem->setGeometrySettings(8192.f * HL_TO_METERS);

	// TODO:
	// FMOD_ADVANCEDSETTINGS settings;
	// fmodSystem->setAdvancedSettings(&settings);

	result = fmodSystem->init(4093, FMOD_INIT_NORMAL, 0);
	if (result != FMOD_OK)
	{
		gEngfuncs.Con_DPrintf("Couldn't initialize FMOD system, falling back to GoldSrc audio. Error %i: %s\n", result, FMOD_ErrorString(result));
		gEngfuncs.Cvar_SetValue("vr_use_fmod", 0.f);
		return false;
	}

	LoadAllSentences();

	g_soundtime = 0.0;

	gEngfuncs.Con_DPrintf("FMOD successfully initialized. Game sounds are now played back using FMOD.\n");
	return true;
}

void VRSoundUpdate(bool paused, double frametime)
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
			g_allSounds.clear();
			g_soundspaused = true;
			g_soundtime = 0.0;
		}
		return;
	}

	if (!fmodSystem)
	{
		VRInitFMOD();
	}

	if (!fmodSystem)
		return;

	if (paused != g_soundspaused)
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
			g_soundtime = 0.0;
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
			// new level
			if (!fmod_currentMapModel || map->model != fmod_currentMapModel)
			{
				g_soundtime = 0.0;
				fmod_currentMapModel = map->model;
			}
		}

		for (auto& [entnumChannel, sound] : g_allSounds)
		{
			// pause all sounds, but only unpause sounds that have started
			if (paused || sound.has_started)
			{
				sound.fmodChannel->setPaused(paused);
			}
		}

		g_soundspaused = paused;
	}

	if (!g_soundspaused)
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

		for (auto& [entnumChannel, sound] : g_allSounds)
		{
			if (!sound.fmodChannel || !sound.fmodSound)
				continue;

			UpdateSoundPosition(sound.fmodChannel, entnumChannel.entnum, nullptr, true);
			UpdateSoundEffects(sound.fmodChannel, entnumChannel.entnum, isPlayerUnderWater);
			if (entnumChannel.channel == CHAN_VOICE || entnumChannel.channel == CHAN_STREAM)
			{
				UpdateMouth(entnumChannel.entnum, sound);
			}
			if (sound.time_marker < 0.f)
			{
				sound.time_marker = g_soundtime;
			}

			if (sound.has_started)
			{
				// TODO: end_offset
				if (sound.end_offset > 0.f)
				{
				}

				bool hasNextSound = !!sound.nextSound;
				bool isPlaying = IsPlaying(sound.fmodChannel);

				if (hasNextSound && !isPlaying)
				{
					Sound copy = *sound.nextSound;
					sound.Release();
					sound = copy;
				}
			}
			else
			{
				if ((sound.time_marker + sound.delayed_by) <= g_soundtime)
				{
					sound.fmodChannel->setPaused(false);
					sound.has_started = true;
				}
			}
		}

		UpdateGeometries();

		g_soundtime += frametime;
	}

	fmodSystem->update();
}
