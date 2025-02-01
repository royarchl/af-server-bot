#include "steam/steamtypes.h"

typedef struct
{
    const char* m_pchRule;
    const char* m_pchValue;
} ServerRule;

#ifdef __cplusplus
extern "C"
{
#endif

    void* steamworks_init();

    ServerRule* steawmworks_get_server_rules(void* pvAPI, const char* pchIP, uint16 usPort);

    void steamworks_initiate_callback_request();

    void steamworks_cancel_server_rules_query(void* pvAPI);

    void steamworks_shutdown(void* pvAPI, ServerRule* pRulePairs);

#ifdef __cplusplus
}
#endif
