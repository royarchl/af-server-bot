#include "steam/isteammatchmaking.h"
#include "steam/steam_api_common.h"
#include "steam/steamtypes.h"
#include <chrono>
#include <steam/steam_api.h>

#include <cstdio>
#include <iostream>
#include <string>
#include <map>
#include <arpa/inet.h>  // FOR INET_ADDR
#include <thread>

/*
 * LINKS:
 * https://partner.steamgames.com/doc/api/steam_api#SteamAPI_Init
 * https://partner.steamgames.com/doc/api/ISteamMatchmakingServers#ServerRules
 */

class CServerRulesResponse : public ISteamMatchmakingRulesResponse
{
public:
    CServerRulesResponse()
    : m_QueryHandle(k_uAPICallInvalid)
    {
    }

    static uint32_t IPStringToHostOrder(const std::string& ipAddress)
    {
        uint32_t networkIP = inet_addr(ipAddress.c_str());
        return ntohl(networkIP);
    }

    void GetRules(std::map<std::string, std::string>& rules) const { rules = m_ServerRules; }

    // RETURNS AN INSTANCE PER SERVER RULE
    virtual void RulesResponded(const char* pchRule, const char* pchValue) override
    {
        m_ServerRules[pchRule] = pchValue;
    }

    virtual void RulesFailedToRespond() override { ResetQuery(); }

    // SERVER HAS FINISHED RESPONDING (SUCCESS)
    virtual void RulesRefreshComplete() override
    {
        // RETURN THE PLAYER COUNT RATHER THAN PRINT
        for (const auto& rule : m_ServerRules)
        {
            printf("%s: %s\n", rule.first.c_str(), rule.second.c_str());
        }

        ResetQuery();
    }

    bool QueryServerRules(std::string szIP, uint16 usPort)
    {
        ISteamMatchmakingServers* pMatchmakingServers = SteamMatchmakingServers();
        if (!pMatchmakingServers)
        {
            printf("Failed to get matchmaking servers interface\n");
            return false;
        }

        m_ServerRules.clear();

        m_QueryHandle = pMatchmakingServers->ServerRules(IPStringToHostOrder(szIP), usPort, this);

        return m_QueryHandle != k_uAPICallInvalid;
    }

    void CancelQuery()
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

private:
    void ResetQuery() { m_QueryHandle = k_uAPICallInvalid; }

    std::map<std::string, std::string> m_ServerRules;

    HServerQuery m_QueryHandle;
};


int main()
{
    if (!SteamAPI_Init())
    {
        printf("Steam API failed to initialize\n");
        return -1;
    }

    // uint32      serverIP   = CServerRulesResponse.IPStringToHostOrder(sserverIP);
    std::string m_szServerIP   = "127.0.0.1";
    uint16      m_usServerPort = 27015;

    CServerRulesResponse rulesResponse;

    if (rulesResponse.QueryServerRules(m_szServerIP, m_usServerPort))
    {
        while (true)
        {
            SteamAPI_RunCallbacks();
            // ADD SLEEP OR FRAME TIMING HERE
            std::this_thread::sleep_for(std::chrono::seconds(1));
        }
    }

    SteamAPI_Shutdown();

    return 0;
}
