#ifndef A2S_QUERY_HANDLER_WRAPPER
#define A2S_QUERY_HANDLER_WRAPPER

// Using C-Std headers rather than C++
#include <stdint.h>  // For uint16_t
#include <stddef.h>  // For size_t

typedef struct
{
    const char* m_pchRule;
    const char* m_pchValue;
} ServerRule;

typedef struct
{
    const ServerRule* m_pMapRules;
    const size_t      m_unRulesSize;
} Payload;

#ifdef __cplusplus
extern "C"
{
#endif

    void* a2s_query_server_rules(const char* pchIP, uint16_t unPort);
    void  a2s_free_rules_memory(void* pArrRulePairs);

#ifdef __cplusplus
}
#endif

#endif  // A2S_QUERY_HANDLER_WRAPPER
