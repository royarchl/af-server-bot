#include "steamworks_api.h"
#include "steam/isteammatchmaking.h"
#include "steam/steamtypes.h"

#include <arpa/inet.h>

/* RETURNS AN INSTANCE PER SERVER-RULE */
void CSteamworksAPI::RulesResponded(const char* pchRule, const char* pchValue)
{
    m_ServerRules[pchRule] = pchValue;
}

void CSteamworksAPI::RulesFailedToRespond() { ResetQuery(); }

void CSteamworksAPI::RulesRefreshComplete()
{
    /* RETURN THE PLAYER COUNT RATHER THAN PRINT */
    for (const auto& rule : m_ServerRules)
    {
        printf("%s: %s\n", rule.first.c_str(), rule.second.c_str());
    }

    ResetQuery();
}

bool CSteamworksAPI::QueryServerRules(std::string szIP, uint16 usPort)
{
    ISteamMatchmakingServers* pMatchmakingServers = SteamMatchmakingServers();
    if (!pMatchmakingServers)
    {
        return false;
    }

    m_ServerRules.clear();

    m_QueryHandle = pMatchmakingServers->ServerRules(IPStringToHostOrder(szIP), usPort, this);

    return m_QueryHandle != k_uAPICallInvalid;
}

void CSteamworksAPI::CopyRulesMap(std::map<std::string, std::string>& mapRulesRef) const
{
    mapRulesRef = m_ServerRules;
}

uint32_t CSteamworksAPI::IPStringToHostOrder(const std::string& ipAddress)
{
    uint32_t networkIP = inet_addr(ipAddress.c_str());
    return ntohl(networkIP);
}

void CSteamworksAPI::ResetQuery() { m_QueryHandle = k_uAPICallInvalid; }
