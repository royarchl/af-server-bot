#ifndef CSTEAMWORKS_API
#define CSTEAMWORKS_API

#include "steam/isteammatchmaking.h"
#include "steam/steam_api.h"

#include <map>
#include <string>
#include <cstdint>

/*
 * DOCUMENTATION:
 * - https://partner.steamgames.com/doc/api/steam_api#SteamAPI_Init
 * - https://partner.steamgames.com/doc/api/ISteamMatchmakingServers#ServerRules
 */

class CSteamworksAPI : public ISteamMatchmakingRulesResponse
{
public:
    CSteamworksAPI()
    : m_QueryHandle(k_uAPICallInvalid)
    {
        /* REMOVE before making a shared library */
        if (!SteamAPI_Init())
        {
            return;
        }
    }

    /* REMOVE before making a shared library */
    ~CSteamworksAPI()
    {
        CancelQuery();
        SteamAPI_Shutdown();
    }

    void RulesResponded(const char* pchRule, const char* pchValue) override;
    void RulesFailedToRespond() override;
    void RulesRefreshComplete() override;
    bool QueryServerRules(std::string szIP, uint16 usPort);
    void CancelQuery();
    void CopyRulesMap(std::map<std::string, std::string>& mapRulesRef) const;

private:
    std::map<std::string, std::string> m_ServerRules;
    HServerQuery                       m_QueryHandle;

    static uint32_t IPStringToHostOrder(const std::string& ipAddress);
    void            ResetQuery();
};

#endif
