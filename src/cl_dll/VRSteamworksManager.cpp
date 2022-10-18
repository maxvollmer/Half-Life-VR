
#include "VRSteamworksManager.h"
#include "VRGameFunctions.h"

#pragma warning(push) 
#pragma warning(disable: 4996)  // Steam API uses deprecated functions, suppress warnings when including Steam headers
#include "../steam/steam_api.h"
#include "../steam/isteamuserstats.h"
#pragma warning(pop)

#include <string>
#include <memory>
#include <unordered_set>
#include <sstream>

namespace
{
	bool didGetUserStats = false;
	bool achievementsNeedStoring = false;

	bool isSteamAPIInitialized = false;

	std::unordered_set<int> achievementsToGive;

	class UserStatsCallback
	{
	private:
		STEAM_CALLBACK(UserStatsCallback, OnUserStatsReceived, UserStatsReceived_t);
	};

	void UserStatsCallback::OnUserStatsReceived(UserStatsReceived_t* pCallback)
	{
		didGetUserStats = true;
	}

	std::unique_ptr<UserStatsCallback> pUserStatsCallback;
}

bool VRSteamworksManager::Init()
{
	if (!SteamAPI_Init())
	{
		VRGameFunctions::PrintToConsole("VRSteamworksManager: Couldn't init Steam API!\n");
		return false;
	}

	isSteamAPIInitialized = true;

	pUserStatsCallback = std::make_unique<UserStatsCallback>();

	// need to call this for achievements to work
	SteamUserStats()->RequestCurrentStats();

	return true;
}

void VRSteamworksManager::Update()
{
	if (!isSteamAPIInitialized)
		return;

	SteamAPI_RunCallbacks();

	if (didGetUserStats && !achievementsToGive.empty())
	{
		for (auto& achievement : achievementsToGive)
		{
			GiveAchievement((VRAchievement)achievement);
		}
		achievementsToGive.clear();
	}

	if (achievementsNeedStoring)
	{
		SteamUserStats()->StoreStats();
		achievementsNeedStoring = false;
	}


	// debug: prints all achieved and unachieved achievements to the console
	float vr_show_achievements = VRGameFunctions::GetCVar("vr_show_achievements");
	if (vr_show_achievements != 0.f)
	{
		if (didGetUserStats)
		{
			std::vector<std::string> achieved;
			std::vector<std::string> unachieved;

			int numAchievements = SteamUserStats()->GetNumAchievements();
			for (int i = 0; i < numAchievements; i++)
			{
				const char* achievementKey = SteamUserStats()->GetAchievementName(i);

				bool isHidden = SteamUserStats()->GetAchievementDisplayAttribute(achievementKey, "hidden")[0] == '1';

				bool hasAchieved = false;
				SteamUserStats()->GetAchievement(achievementKey, &hasAchieved);

				std::stringstream achievementString;

				if (isHidden)
				{
					achievementString << "[Hidden] ";
				}

				if (hasAchieved || !isHidden || vr_show_achievements == 2.f)
				{
					const char* achievementName = SteamUserStats()->GetAchievementDisplayAttribute(achievementKey, "name");
					const char* achievementDescription = SteamUserStats()->GetAchievementDisplayAttribute(achievementKey, "desc");

					achievementString << achievementName << " - " << achievementDescription;
				}

				achievementString << "\n";

				if (hasAchieved)
				{
					achieved.push_back(achievementString.str());
				}
				else
				{
					unachieved.push_back(achievementString.str());
				}
			}

			VRGameFunctions::PrintToConsole("\n");

			VRGameFunctions::PrintToConsole("**Achieved (unlocked) Achievements:**\n");
			for (auto& a : achieved)
			{
				VRGameFunctions::PrintToConsole(a.c_str());
			}

			VRGameFunctions::PrintToConsole("\n");

			VRGameFunctions::PrintToConsole("**Unachieved (locked) Achievements:**\n");
			for (auto& a : unachieved)
			{
				VRGameFunctions::PrintToConsole(a.c_str());
			}

			VRGameFunctions::PrintToConsole("\n");
			VRGameFunctions::PrintToConsole("\n");
		}
		else
		{
			VRGameFunctions::PrintToConsole("Polling Steam API, please try again in a few seconds...\n");
		}
		VRGameFunctions::SetCVar("vr_show_achievements", 0.f);
	}
}

void VRSteamworksManager::GiveAchievement(VRAchievement achievement)
{
	if (!didGetUserStats)
	{
		// remember achievement
		achievementsToGive.insert((int)achievement);

		// try again:
		if (isSteamAPIInitialized)
		{
			SteamUserStats()->RequestCurrentStats();
		}
		else
		{
			Init();
		}
		return;
	}

	if (!isSteamAPIInitialized)
		return;

	bool checkTroll = false;

	switch (achievement)
	{
	case VRAchievement::HC_SAFETYFIRST:
		SteamUserStats()->SetAchievement("SafetyFirst");
		break;
	case VRAchievement::BMI_STARTTHEGAME:
		SteamUserStats()->SetAchievement("StartGame");
		break;
	case VRAchievement::AM_HELLOGORDON:
		SteamUserStats()->SetAchievement("HelloGordon");
		break;
	case VRAchievement::AM_WELLPREPARED:
		SteamUserStats()->SetAchievement("WellPrepared");
		break;
	case VRAchievement::AM_SMALLCUP:
		SteamUserStats()->SetAchievement("SmallCup");
		break;
	case VRAchievement::AM_BOTHERSOME:
		SteamUserStats()->SetAchievement("BotherSome");
		checkTroll = true;
		break;
	case VRAchievement::AM_MAGNUSSON:
		SteamUserStats()->SetAchievement("Magnusson");
		checkTroll = true;
		break;
	case VRAchievement::AM_IMPORTANTMSG:
		SteamUserStats()->SetAchievement("ImportantMsg");
		checkTroll = true;
		break;
	case VRAchievement::AM_TROUBLE:
		SteamUserStats()->SetAchievement("Trouble");
		checkTroll = true;
		break;
	case VRAchievement::AM_LIGHTSOFF:
		SteamUserStats()->SetAchievement("LightsOff");
		checkTroll = true;
		break;
	case VRAchievement::UC_RIGHTTOOL:
		SteamUserStats()->SetAchievement("RightTool");
		break;
	case VRAchievement::UC_OOPS:
		SteamUserStats()->SetAchievement("UnforeseenConseq");
			break;
	case VRAchievement::UC_CPR:
		SteamUserStats()->SetAchievement("CPR");
			break;
	case VRAchievement::OC_OSHAVIOLATION:
		SteamUserStats()->SetAchievement("OSHAViolation");
		break;
	case VRAchievement::OC_DUCK:
		SteamUserStats()->SetAchievement("Duck");
		break;
	case VRAchievement::WGH_DOOMED:
		SteamUserStats()->SetAchievement("Doomed");
		break;
	case VRAchievement::WGH_NOTDOOMED:
		SteamUserStats()->SetAchievement("NotDoomed");
		break;
	case VRAchievement::WGH_PERFECT:
		SteamUserStats()->SetAchievement("PerfectBalance");
		break;
	case VRAchievement::BP_PERFECT:
		SteamUserStats()->SetAchievement("PerfectLanding");
		break;
	case VRAchievement::BP_BLOWUPBRIDGES:
		SteamUserStats()->SetAchievement("BridgePipeHuh");
		break;
	case VRAchievement::BP_BROKENELVTR:
		SteamUserStats()->SetAchievement("TrapElevator");
		break;
	case VRAchievement::BP_BADDESIGN:
		SteamUserStats()->SetAchievement("BadDesign");
		break;
	case VRAchievement::BP_SNEAKY:
		SteamUserStats()->SetAchievement("SneakyI");
		break;
	case VRAchievement::PU_BBQ:
		SteamUserStats()->SetAchievement("BBQ");
		break;
	case VRAchievement::OAR_CHOOCHOO:
		SteamUserStats()->SetAchievement("ChooChoo");
		break;
	case VRAchievement::OAR_INFINITY:
		SteamUserStats()->SetAchievement("Infinity");
		break;
	case VRAchievement::RP_PARENTAL:
		SteamUserStats()->SetAchievement("ParentInstinct");
		break;
	case VRAchievement::RP_MOEBIUS:
		SteamUserStats()->SetAchievement("Moebius");
		break;
	case VRAchievement::QE_RIGOROUS:
		SteamUserStats()->SetAchievement("RigorousRsrch");
		break;
	case VRAchievement::QE_EFFECTIVE:
		SteamUserStats()->SetAchievement("EffectiveRsrch");
		break;
	case VRAchievement::QE_ATALLCOSTS:
		SteamUserStats()->SetAchievement("RsrchAtAllCosts");
		break;
	case VRAchievement::QE_PRECISURGERY:
		SteamUserStats()->SetAchievement("Precisurgery");
		break;
	case VRAchievement::ST_VIEW:
		SteamUserStats()->SetAchievement("ViewI");
		break;
	case VRAchievement::ST_SNEAKY:
		SteamUserStats()->SetAchievement("SneakyII");
		break;
	case VRAchievement::FAF_FIREINHOLE:
		SteamUserStats()->SetAchievement("FireInHole");
		break;
	case VRAchievement::XEN_VIEW:
		SteamUserStats()->SetAchievement("ViewII");
		break;
	case VRAchievement::N_WHAT:
		SteamUserStats()->SetAchievement("WhatIsThat");
		break;
	case VRAchievement::N_BRRR:
		SteamUserStats()->SetAchievement("SkyBabyBrrr");
		break;
	case VRAchievement::N_EXPLORER:
		SteamUserStats()->SetAchievement("Explorer");
		break;
	case VRAchievement::END_GMAN:
		SteamUserStats()->SetAchievement("GMan");
		break;
	case VRAchievement::END_LIMITLESS:
		SteamUserStats()->SetAchievement("Limitless");
		break;
	case VRAchievement::END_UNWINNABLE:
		SteamUserStats()->SetAchievement("Unwinnable");
		break;
	case VRAchievement::GEN_REFRESHING:
		SteamUserStats()->SetAchievement("Refreshing");
		break;
	case VRAchievement::GEN_CATCH:
		SteamUserStats()->SetAchievement("Catch");
		break;
	case VRAchievement::GEN_ALIENGIB:
		SteamUserStats()->SetAchievement("Fascpecimen");
		break;
	case VRAchievement::GEN_BASEBALLED:
		SteamUserStats()->SetAchievement("Baseballed");
		break;
	case VRAchievement::GEN_TEAMPLAYER:
		SteamUserStats()->SetAchievement("TeamPlayer");
		break;
	case VRAchievement::GEN_SNIPED:
		SteamUserStats()->SetAchievement("Sniped");
		break;
	case VRAchievement::GEN_PETDOGGO:
		SteamUserStats()->SetAchievement("PetTheDoggo");
		break;
	case VRAchievement::HID_ROPES:
		SteamUserStats()->SetAchievement("Ropes");
		break;
	case VRAchievement::GEN_PACIFIST:
		SteamUserStats()->SetAchievement("Pacifist");
		break;
	case VRAchievement::HID_HOW:
		SteamUserStats()->SetAchievement("How");
		break;
	case VRAchievement::HID_NOTTRAINS:
		SteamUserStats()->SetAchievement("NotTrains");
		break;
	case VRAchievement::HID_TROLL:
		SteamUserStats()->SetAchievement("Troll");
		break;
	case VRAchievement::HID_SMALLESTCUP:
		SteamUserStats()->SetAchievement("SmallestCup");
		break;
	case VRAchievement::HID_SKIP_WGH:
		SteamUserStats()->SetAchievement("SkipWGH");
		break;
	case VRAchievement::HID_SKIP_PU:
		SteamUserStats()->SetAchievement("SkipPU");
		break;
	case VRAchievement::HID_OFFARAIL:
		SteamUserStats()->SetAchievement("OffARail");
		break;
	case VRAchievement::HID_NOTCHEATING:
		SteamUserStats()->SetAchievement("NotCheating");
		break;
	default:
		VRGameFunctions::PrintToConsole(("Invalid achievement: " + std::to_string((int)achievement) + "\n").c_str());
	}

	if (checkTroll)
	{
		bool hasTroll = false;
		if (SteamUserStats()->GetAchievement("Troll", &hasTroll) && !hasTroll)
		{
			bool hasBotherSome = false;
			bool hasMagnusson = false;
			bool hasImportantMsg = false;
			bool hasTrouble = false;
			bool hasLightsOff = false;
			if (SteamUserStats()->GetAchievement("BotherSome", &hasBotherSome) && hasBotherSome
				&& SteamUserStats()->GetAchievement("Magnusson", &hasMagnusson) && hasMagnusson
				&& SteamUserStats()->GetAchievement("ImportantMsg", &hasImportantMsg) && hasImportantMsg
				&& SteamUserStats()->GetAchievement("Trouble", &hasTrouble) && hasTrouble
				&& SteamUserStats()->GetAchievement("LightsOff", &hasLightsOff) && hasLightsOff)
			{
				SteamUserStats()->SetAchievement("Troll");
			}
		}
	}

	achievementsNeedStoring = true;
}
