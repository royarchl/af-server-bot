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

    ServerRule* steawmworks_get_rules(void* pvAPI, const char* pchIP, uint16 usPort);

    void steamworks_shutdown(void* pvAPI, ServerRule* pRulePairs);

#ifdef __cplusplus
}
#endif
