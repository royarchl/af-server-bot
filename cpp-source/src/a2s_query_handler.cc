#include "a2s_query_handler.h"

#include <arpa/inet.h>
#include <cstring>
#include <map>
#include <stdexcept>
#include <string>
#include <unistd.h>


struct BufferWrapperException : public std::runtime_error
{
public:
    BufferWrapperException(int errorCode, const std::string& errorMsg)
    : std::runtime_error(errorMsg)
    , m_errorCode(errorCode)
    {
    }

    int getErrorCode() const { return m_errorCode; }

private:
    int m_errorCode;
};


A2SQueryHandler::A2SQueryHandler(const char* pchIP, uint16_t unPort)
{
    if (m_nSock == -1)
    {
        // Never caught by anything; this ~should just exit/break
        throw BufferWrapperException(5001, "Failure to create socket");
    }

    m_sockTargetAddress = {
        .sin_family = AF_INET,
        .sin_port   = htons(unPort),
        .sin_addr   = { inet_addr(pchIP) },
    };
}

A2SQueryHandler::~A2SQueryHandler() { close(m_nSock); }

BufferWrapper* A2SQueryHandler::PackageResponse()
{
    try
    {
        std::vector<uint8_t>               serverResponse = QueryServer();
        std::map<std::string, std::string> rulesMap       = ParseUdpPacketsResponse(serverResponse);

        return new BufferWrapper{ rulesMap, 0, {} };
    }
    catch (const BufferWrapperException& e)
    {
        return new BufferWrapper{ {}, e.getErrorCode(), e.what() };
    }
    return new BufferWrapper;
}

void A2SQueryHandler::DeleteBuffer(BufferWrapper* buffer) { delete buffer; }

// Remove the try-catch block, and instead move it to the PackageResponse() method
// - This method will simply throw errors that will be caught in the parent method
std::vector<uint8_t> A2SQueryHandler::QueryServer()
{
    static constexpr size_t  sMAX_PAYLOAD   = 1400;
    static constexpr uint8_t sA2S_RULES     = 0x56;
    static constexpr uint8_t sS2C_CHALLENGE = 0x41;
    static constexpr int     SOCKET_ERROR   = -1;

    std::vector<uint8_t> buffer(sMAX_PAYLOAD);
    std::vector<uint8_t> query{ 0xFF, 0xFF, 0xFF, 0xFF, sA2S_RULES, 0xFF, 0xFF, 0xFF, 0xFF };

    auto sendQuery = [this](const std::vector<uint8_t>& queryData) -> bool
    {
        auto result = sendto(m_nSock,
                             queryData.data(),
                             queryData.size(),
                             0,
                             reinterpret_cast<const sockaddr*>(&m_sockTargetAddress),
                             sizeof(m_sockTargetAddress));
        return result != SOCKET_ERROR;
    };

    auto receiveResponse = [this](std::vector<uint8_t>& buffer, int timeout_ms = 5000) -> ssize_t
    {
        buffer.resize(sMAX_PAYLOAD);
        uint8_t* raw_buffer = buffer.data();

        struct timeval tv;
        tv.tv_sec  = timeout_ms / 1000;
        tv.tv_usec = (timeout_ms % 1000) * 1000;
        if (setsockopt(m_nSock, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv)) < 0)
        {
            throw BufferWrapperException(5002, "Failure to apply socket timeout");
        }

        auto result = recvfrom(m_nSock, raw_buffer, sMAX_PAYLOAD, 0, nullptr, nullptr);

        if (result == SOCKET_ERROR)
        {
            if (errno == EAGAIN || errno == EWOULDBLOCK)
            {
                throw BufferWrapperException(5003,
                                             "Response timeout (request exceeded "
                                                 + std::to_string(timeout_ms / 1000) + "s)");
            }
            throw BufferWrapperException(5004, "Failure to receive data");
        }
        return result;
    };

    if (!sendQuery(query))
    {
        throw BufferWrapperException(5005, "Failure to send initial query");
    }

    ssize_t bufferSize = receiveResponse(buffer);
    if (bufferSize < 5)
    {
        throw BufferWrapperException(5006, "Server response too small");
    }

    if (buffer[4] == sS2C_CHALLENGE)
    {
        uint32_t challengeNumber;
        std::memcpy(&challengeNumber, &buffer[5], sizeof(challengeNumber));

        for (int i = 0; i < sizeof(challengeNumber); ++i)
        {
            query[5 + i] = static_cast<uint8_t>((challengeNumber >> (8 * i)) & 0xFF);
        }

        if (!sendQuery(query))
        {
            throw BufferWrapperException(5007, "Failure to send challenge query");
        }

        bufferSize = receiveResponse(buffer);

        if (bufferSize < 5)
        {
            throw BufferWrapperException(5008, "Challenge response too small");
        }
    }

    return buffer;
}

std::map<std::string, std::string> A2SQueryHandler::ParseUdpPacketsResponse(
    const std::vector<uint8_t>& buffer)
{
    static constexpr uint8_t NULL_TERMINATOR = 0x00;

    std::vector<std::string> packets;
    std::string              currentPacket;

    for (const auto& byte : buffer)
    {
        if (byte == NULL_TERMINATOR)
        {
            if (!currentPacket.empty())
            {
                packets.push_back(currentPacket);
                currentPacket.clear();
            }
        }
        else
        {
            currentPacket += byte;
        }
    }

    if (!currentPacket.empty())
    {
        packets.push_back(currentPacket);
    }

    if (packets.empty())
    {
        throw BufferWrapperException(4001, "No packets found");
    }

    std::map<std::string, std::string> returnMap;
    for (size_t i = 1; i < packets.size(); i += 2)
    {
        if (i + 1 < packets.size())
        {
            returnMap[packets[i]] = packets[i + 1];
        }
        else
        {
            throw BufferWrapperException(4002, "Incomplete rule-value pair");
        }
    }
    return returnMap;
}
