#ifndef A2S_QUERY_HANDLER_WRAPPER
#define A2S_QUERY_HANDLER_WRAPPER

// Using C-Std headers rather than C++
#include <stdint.h>   // For uint16_t
#include <stddef.h>   // For size_t
#include <stdbool.h>  // For bool

typedef struct
{
    const char* m_pchRule;
    const char* m_pchValue;
} ServerRule;

typedef struct
{
    const ServerRule* m_pMapRules;
    size_t            m_unRulesSize;
    int               m_nErrorCode;
    const char*       m_szErrorMsg;
} Payload;

#ifdef __cplusplus
extern "C"
{
#endif

    void* a2s_query_server_rules(const char* pchIP, uint16_t unPort);
    bool  a2s_free_rules_memory(void* pPayload);

#ifdef __cplusplus
}
#endif

#endif  // A2S_QUERY_HANDLER_WRAPPER
