#include "steam/steam_api_common.h"
#include "steam/steamtypes.h"
#include <chrono>
#include <steam/steam_api.h>

#include "steamworks_api.h"

#include <cstdio>
#include <string>
#include <arpa/inet.h>  // FOR INET_ADDR
#include <thread>

/*
 * LINKS:
 * https://partner.steamgames.com/doc/api/steam_api#SteamAPI_Init
 * https://partner.steamgames.com/doc/api/ISteamMatchmakingServers#ServerRules
 */

// int main()
// {
//     // if (!SteamAPI_Init())
//     // {
//     //     printf("Steam API failed to initialize\n");
//     //     return -1;
//     // }
//
//     std::string m_szServerIP   = "127.0.0.1";
//     uint16      m_usServerPort = 27015;
//
//     CSteamworksAPI apiResponse;
//     if (apiResponse.QueryServerRules(m_szServerIP, m_usServerPort))
//     {
//         while (true)
//         {
//             printf("Attempting calback...\n");
//             SteamAPI_RunCallbacks();
//             std::this_thread::sleep_for(std::chrono::seconds(1));
//         }
//     }
//
//     // SteamAPI_Shutdown();
//
//     return 0;
// }
