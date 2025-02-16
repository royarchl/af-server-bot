#ifndef A2S_QUERY_HANDLER
#define A2S_QUERY_HANDLER

#include <map>
#include <netinet/in.h>
#include <string>
#include <vector>

struct BufferWrapper
{
    std::map<std::string, std::string> m_mapRules;
    int                                m_nErrorCode;
    std::string                        m_szErrorMsg;
};

class A2SQueryHandler
{
public:
    A2SQueryHandler(const char* pchIP, uint16_t unPort);

    ~A2SQueryHandler();

    BufferWrapper* PackageResponse();
    void           DeleteBuffer(BufferWrapper* buffer);

private:
    struct sockaddr_in m_sockTargetAddress;
    const int          m_nSock = socket(AF_INET, SOCK_DGRAM, 0);

    std::vector<uint8_t>               QueryServer();
    std::map<std::string, std::string> ParseUdpPacketsResponse(const std::vector<uint8_t>& buffer);
};

#endif  // A2S_QUERY_HANDLER
