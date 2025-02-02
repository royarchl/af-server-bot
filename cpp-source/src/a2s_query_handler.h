#ifndef A2S_QUERY_HANDLER
#define A2S_QUERY_HANDLER

#include <map>
#include <netinet/in.h>
#include <string>

class A2SQueryHandler
{
private:
    struct BufferWrapper
    {
        uint8_t* m_punBuffer;
        size_t   m_unSize;
    };

public:
    A2SQueryHandler(const char* pchIP, uint16_t unPort);

    ~A2SQueryHandler();

    void QueryServerRules();
    void ParseUdpPacketsResponse();
    void CopyRulesMap(std::map<std::string, std::string>& mapRulesRef) const;

private:
    std::map<std::string, std::string> m_mapRules;
    struct sockaddr_in                 m_sockTargetAddress;
    const int                          m_nSock = socket(AF_INET, SOCK_DGRAM, 0);
    BufferWrapper*                     m_pBufferWrapper;
};

#endif  // A2S_QUERY_HANDLER
