#include "steamworks_api_wrapper.h"
#include "steam/steam_api.h"
#include "steamworks_api.h"

#include <map>
#include <string>


/* void* is used as a C-style, generic pointer */
void* steamworks_init()
{
    if (!SteamAPI_Init())
    {
        return nullptr;
    }

    /*
     * This API pointer is initialized for the purpose of keeping track
     * of the object betweeen function calls.
     */
    CSteamworksAPI* pvAPI = new CSteamworksAPI();

    return pvAPI;
}

ServerRule* steawmworks_get_rules(void* pvAPI, const char* pchIP, uint16 usPort)
{
    CSteamworksAPI* steamAPI = reinterpret_cast<CSteamworksAPI*>(pvAPI);

    if (!steamAPI->QueryServerRules(pchIP, usPort))
    {
        return nullptr;
    }

    /* Make sure the callbacks have finished before continuing. */

    std::map<std::string, std::string> rules;
    steamAPI->CopyRulesMap(rules);

    /*
     * If memory isn't properly being freed by the shutdown function, update
     * this to use malloc rather than new so that Go can handle freeing it.
     */
    ServerRule* pRulePairs = new ServerRule[rules.size()];

    int i = 0;
    for (const auto& entry : rules)
    {
        pRulePairs[i].m_pchRule  = entry.first.c_str();
        pRulePairs[i].m_pchValue = entry.second.c_str();
        ++i;
    }
    return pRulePairs;
}

void steamworks_shutdown(void* pvAPI, ServerRule* pRulePairs)
{
    CSteamworksAPI* steamAPI = reinterpret_cast<CSteamworksAPI*>(pvAPI);

    SteamAPI_Shutdown();

    delete[] pRulePairs;
    delete steamAPI;
}
