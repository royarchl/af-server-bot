#include "steamworks_api.h"
#include "steam/isteammatchmaking.h"
#include "steam/steam_api_common.h"
#include "steam/steamtypes.h"

#include <arpa/inet.h>
#include <cstdio>

/* RETURNS AN INSTANCE PER SERVER-RULE */
void CSteamworksAPI::RulesResponded(const char* pchRule, const char* pchValue)
{
    printf("Rule received\n");
    m_ServerRules[pchRule] = pchValue;
}

void CSteamworksAPI::RulesFailedToRespond() { ResetQuery(); }

void CSteamworksAPI::RulesRefreshComplete()
{
    printf("Query request completed!\n");
    /*
     * I need to make this function declare to the Go side of the code that
     * everything has completed, rather than printing the details.
     */
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

void CSteamworksAPI::CancelQuery()
{
    if (m_QueryHandle != k_uAPICallInvalid)
    {
        ISteamMatchmakingServers* pMatchmakingServers = SteamMatchmakingServers();
        if (pMatchmakingServers)
        {
            pMatchmakingServers->CancelServerQuery(m_QueryHandle);
        }
        ResetQuery();
    }
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
