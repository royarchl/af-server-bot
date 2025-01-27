#ifndef CSTEAMWORKS_API
#define CSTEAMWORKS_API

#include "steam/isteammatchmaking.h"

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
    }

    void RulesResponded(const char* pchRule, const char* pchValue) override;
    void RulesFailedToRespond() override;
    void RulesRefreshComplete() override;
    bool QueryServerRules(std::string szIP, uint16 usPort);
    void CopyRulesMap(std::map<std::string, std::string>& mapRulesRef) const;

private:
    std::map<std::string, std::string> m_ServerRules;
    HServerQuery                       m_QueryHandle;

    static uint32_t IPStringToHostOrder(const std::string& ipAddress);
    void            ResetQuery();
};

#endif
