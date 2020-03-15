
#include <string>
#include <filesystem>
#include <regex>
#include <memory>
#include <unordered_map>

#include "extdll.h"
#include "util.h"
#include "cbase.h"


// Intercept model functions to allow usage of SD versions

inline std::filesystem::path GetPathFor(const std::string& file)
{
	std::filesystem::path relativePath = UTIL_GetGameDir() + file;
	return std::filesystem::absolute(relativePath);
}

namespace
{
	std::unordered_map<std::string, std::shared_ptr<char>> gModelNames;
}

char* GetSafeModelCharPtr(const std::string& model)
{
	if (gModelNames.count(model) == 0)
	{
		std::shared_ptr<char> modelPtr(new char[model.size() + 1], [](char* s) { delete[] s; });
		std::copy(model.begin(), model.end(), modelPtr.get());
		modelPtr.get()[model.size()] = 0;
		gModelNames[model] = modelPtr;
	}
	return gModelNames[model].get();
}

char* GetSDModel(const std::string& model)
{
	if (model.find("models/") == std::string::npos)
		return nullptr;

	std::string sdModel = std::regex_replace(model, std::regex{ "models/" }, "models/SD/");
	std::filesystem::path sdModelPathRelative{ UTIL_GetGameDir() + "/" + sdModel };

	std::filesystem::path sdModelPath = std::filesystem::absolute(sdModelPathRelative);

	if (std::filesystem::exists(sdModelPath))
	{
		return GetSafeModelCharPtr(sdModel);
	}
	else
		return nullptr;
}

const char* GetHDModel(const char* model)
{
	std::string sdModel{ model };

	if (sdModel.find("models/SD/") == std::string::npos)
		return model;

	std::string hdModel = std::regex_replace(sdModel, std::regex{ "models/SD/" }, "models/");
	return GetSafeModelCharPtr(hdModel);
}

extern const char* GetAnimatedWeaponModelName(const char* modelName);

int PRECACHE_MODEL2(const char* s)
{
	const char* anims = GetAnimatedWeaponModelName(s);
	if (anims != s)
		PRECACHE_MODEL3(const_cast<char*>(anims));
	return PRECACHE_MODEL3(s);
}

int PRECACHE_GENERIC2(const char* s)
{
	const char* anims = GetAnimatedWeaponModelName(s);
	if (anims != s)
		PRECACHE_GENERIC3(const_cast<char*>(anims));
	return PRECACHE_GENERIC3(s);
}

int PRECACHE_MODEL(const char* s)
{
	int sdModelIndex = -1;
	int hdModelIndex = PRECACHE_MODEL2(s);

	char* sdModel = GetSDModel(s);
	if (sdModel != nullptr)
	{
		sdModelIndex = PRECACHE_MODEL2(sdModel);
	}

	if (sdModelIndex >= 0 && CVAR_GET_FLOAT("vr_use_hd_models") == 0.f)
		return sdModelIndex;

	return hdModelIndex;
}

int PRECACHE_GENERIC(const char* s)
{
	int sdModelIndex = -1;
	int hdModelIndex = PRECACHE_GENERIC2(s);

	char* sdModel = GetSDModel(s);
	if (sdModel != nullptr)
	{
		sdModelIndex = PRECACHE_GENERIC2(sdModel);
	}

	if (sdModelIndex >= 0 && CVAR_GET_FLOAT("vr_use_hd_models") == 0.f)
		return sdModelIndex;

	return hdModelIndex;
}

void SET_MODEL(edict_t* e, const char* m)
{
	if (CVAR_GET_FLOAT("vr_use_hd_models") == 0.f)
	{
		char* sdModel = GetSDModel(m);
		if (sdModel != nullptr)
		{
			SET_MODEL2(e, sdModel);
			return;
		}
	}

	SET_MODEL2(e, m);
}

int MODEL_INDEX(const char* m)
{
	if (CVAR_GET_FLOAT("vr_use_hd_models") == 0.f)
	{
		char* sdModel = GetSDModel(m);
		if (sdModel != nullptr)
		{
			return MODEL_INDEX2(sdModel);
		}
	}

	return MODEL_INDEX2(m);
}

void UTIL_SetModelKeepSize(edict_t* pent, const char* model)
{
	// pfnSetModel resets mins and maxs to 0, so we need to back them up and call setsize afterwards
	Vector mins = pent->v.mins;
	Vector maxs = pent->v.maxs;
	SET_MODEL2(pent, model);
	UTIL_SetSize(&pent->v, mins, maxs);
}

bool gSDModelsEnabled{ false };
void UTIL_UpdateSDModels()
{
	bool sdModelsEnabled = CVAR_GET_FLOAT("vr_use_hd_models") == 0.f;
	if (sdModelsEnabled != gSDModelsEnabled)
	{
		gSDModelsEnabled = sdModelsEnabled;

		for (int index = 0; index < gpGlobals->maxEntities; index++)
		{
			edict_t* pent = INDEXENT(index);
			if (FNullEnt(pent))
				continue;

			if (pent->v.model)
			{
				const char* modelName = STRING(pent->v.model);
				if (gSDModelsEnabled)
				{
					char* sdModel = GetSDModel(modelName);
					if (sdModel != nullptr)
					{
						UTIL_SetModelKeepSize(pent, sdModel);
					}
				}
				else if (!gSDModelsEnabled && std::string{ modelName }.find("models/SD/") != std::string::npos)
				{
					UTIL_SetModelKeepSize(pent, GetHDModel(modelName));
				}
			}
		}
	}
}
