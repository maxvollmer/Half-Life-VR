
#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "player.h"
#include "pm_defs.h"
#include "pm_shared.h"
#include "com_model.h"

#include "VRNetworkManager.h"

#pragma warning(push) 
#pragma warning(disable: 4996)

#include "../steam/steam_api_common.h"
#include "../steam/steam_api.h"
#include "../steam/isteamfriends.h"

#pragma warning(pop)

bool didthething = false;
ISteamNetworking* steamNetworking = nullptr;
CSteamID steamID;

int messagecounter = 0;

void BlebSendMessage()
{
    std::string message = "testmessage" + std::to_string(messagecounter++);

    ALERT(at_console, "Sending message: %s\n", message.c_str());

    steamNetworking->AcceptP2PSessionWithUser(steamID);
    steamNetworking->SendP2PPacket(steamID, message.c_str(), message.size() + 1, EP2PSend::k_EP2PSendReliable, 1337);
}

void ReceiveMessages()
{
    uint32_t msgSize = 0;
    while (steamNetworking->IsP2PPacketAvailable(&msgSize, 1337))
    {
        CSteamID remoteSteamID;
        std::string message;
        message.resize(msgSize);
        if (steamNetworking->ReadP2PPacket(message.data(), msgSize, &msgSize, &remoteSteamID, 1337))
        {
            ALERT(at_console, "Received message: %s from %i\n", message.c_str(), remoteSteamID.ConvertToUint64());
        }
        else
        {
            ALERT(at_console, "Failed to receive message\n");
        }
    }
}

void VRNetworkManager::InitNetwork()
{
    if (steamNetworking)
    {
        BlebSendMessage();
        ReceiveMessages();
    }

    if (didthething)
        return;

    didthething = true;

    ALERT(at_console, "blub\n");

    if (SteamAPI_RestartAppIfNecessary(70))
    {
        ALERT(at_console, "SteamAPI_RestartAppIfNecessary said yes.\n");
        return;
    }

    if (!SteamAPI_Init())
    {
        ALERT(at_console, "SteamAPI_Init() failed.\n");
        return;
    }

    ISteamFriends* steamFriends = SteamFriends();

    if (!steamFriends)
    {
        ALERT(at_console, "SteamFriends is NULL.\n");
        return;
    }

    const char* name = steamFriends->GetPersonaName();
    ALERT(at_console, "GetPersonaName: '%s'\n", name ? name : "NULL");

    ISteamUser* SteamUserPtr = SteamUser();

    if (!SteamUserPtr)
    {
        ALERT(at_console, "SteamUserPtr is NULL.\n");
        return;
    }

    steamID = SteamUserPtr->GetSteamID();
    ALERT(at_console, "SteamID: '%i'\n", steamID.ConvertToUint64());

    steamNetworking = SteamNetworking();

    if (!steamNetworking)
    {
        ALERT(at_console, "SteamNetworking is NULL.\n");
        return;
    }

    return;
}

