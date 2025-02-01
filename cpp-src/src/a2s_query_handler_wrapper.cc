#include "a2s_query_handler_wrapper.h"
#include "a2s_query_handler.h"

#include <map>
#include <string>

void* a2s_query_server_rules(const char* pchIP, uint16_t unPort)
{
    A2SQueryHandler handler(pchIP, unPort);

    handler.QueryServerRules();
    handler.ParseUdpPacketsResponse();

    std::map<std::string, std::string> mapTemp;
    handler.CopyRulesMap(mapTemp);

    ServerRule* pArrRulePairs = new ServerRule[mapTemp.size()];

    int i = 0;
    for (const auto& entry : mapTemp)
    {
        pArrRulePairs[i].m_pchRule  = entry.first.c_str();
        pArrRulePairs[i].m_pchValue = entry.second.c_str();
        ++i;
    }

    Payload* pPayload = new Payload{ pArrRulePairs, mapTemp.size() };
    return reinterpret_cast<void*>(pPayload);
}

void a2s_free_rules_memory(void* pArrRulePairs)
{
    if (pArrRulePairs)
    {
        delete[] reinterpret_cast<ServerRule*>(pArrRulePairs);
    }
}
