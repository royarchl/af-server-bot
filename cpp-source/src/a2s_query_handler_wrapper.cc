#include "a2s_query_handler_wrapper.h"
#include "a2s_query_handler.h"

#include <map>
#include <string>

void* a2s_query_server_rules(const char* pchIP, uint16_t unPort)
{
    A2SQueryHandler handler(pchIP, unPort);

    BufferWrapper* testBuffer = handler.PackageResponse();

    ServerRule* pArrRulePairs = new ServerRule[testBuffer->m_mapRules.size()];

    int i = 0;
    for (const auto& rulePair : testBuffer->m_mapRules)
    {
        pArrRulePairs[i].m_pchRule  = rulePair.first.c_str();
        pArrRulePairs[i].m_pchValue = rulePair.second.c_str();
        ++i;
    }

    Payload* pPayload = new Payload{ pArrRulePairs,
                                     testBuffer->m_mapRules.size(),
                                     testBuffer->m_nErrorCode,
                                     testBuffer->m_szErrorMsg.c_str() };

    return reinterpret_cast<void*>(pPayload);
}

bool a2s_free_rules_memory(void* pPayload)
{
    if (pPayload)
    {
        Payload* payload = reinterpret_cast<Payload*>(pPayload);

        delete[] payload->m_pMapRules;
        delete payload;

        return true;
    }
    return false;
}
