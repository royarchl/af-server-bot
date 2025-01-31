#include "a2s_query_handler.h"

#include <arpa/inet.h>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <iostream>
#include <sys/socket.h>
#include <sys/types.h>
#include <vector>

#define sFAILURE -1
#define sMAX_PAYLOAD 1400
#define sA2S_RULES 0x56
#define sS2C_CHALLENGE 0x41
#define NULL_TERMINATOR 0x00


A2SQueryHandler::A2SQueryHandler(const char* pchIP, uint16_t unPort)
{
    m_sockTargetAddress = {
        .sin_family = AF_INET,
        .sin_port   = htons(unPort),
        .sin_addr   = { inet_addr(pchIP) },
    };
}

A2SQueryHandler::~A2SQueryHandler()
{
    // C++ Standard views deleting a nullptr as a non-op so it's fine
    delete[] m_pBufferWrapper->m_punBuffer;
    delete m_pBufferWrapper;
}

void A2SQueryHandler::QueryServerRules()
{
    uint8_t* pArrBuffer = new uint8_t[sMAX_PAYLOAD];  // Allocate on the heap

    uint8_t arrQueryData[] = { 0xFF, 0xFF, 0xFF, 0xFF, sA2S_RULES, 0xFF, 0xFF, 0xFF, 0xFF };
    sendto(m_nSock,
           arrQueryData,
           sizeof(arrQueryData),
           0,
           reinterpret_cast<struct sockaddr*>(&m_sockTargetAddress),
           sizeof(m_sockTargetAddress));

    // ssize_t has range [-1...2^15-1] and int has range [-2^15...2^15-1]
    ssize_t nBufferSize = recvfrom(m_nSock, pArrBuffer, sMAX_PAYLOAD, 0, NULL, NULL);

    if (pArrBuffer[4] == sS2C_CHALLENGE)
    {
        uint32_t unChallengeNumber;  // Challenge number is 4 bytes
        memcpy(&unChallengeNumber, &pArrBuffer[5], sizeof(unChallengeNumber));

        uint8_t arrQueryWithChall[9] = { 0xFF, 0xFF, 0xFF, 0xFF, sA2S_RULES };

        for (int i = 0; i < sizeof(unChallengeNumber); ++i)
        {
            arrQueryWithChall[5 + i] = static_cast<uint8_t>((unChallengeNumber >> (8 * i)) & 0xFF);
        }

        sendto(m_nSock,
               arrQueryWithChall,
               sizeof(arrQueryWithChall),
               0,
               reinterpret_cast<struct sockaddr*>(&m_sockTargetAddress),
               sizeof(m_sockTargetAddress));

        // nBufferSize = recvfrom(m_nSock, pArrBuffer, sizeof(pArrBuffer), 0, NULL, NULL);
        nBufferSize = recvfrom(m_nSock, pArrBuffer, sMAX_PAYLOAD, 0, NULL, NULL);
    }

    m_pBufferWrapper = new BufferWrapper{ pArrBuffer, static_cast<size_t>(nBufferSize) };
}

void A2SQueryHandler::ParseUdpPacketsResponse()
{
    std::vector<std::string> packets;
    std::string              currentPacket;

    for (size_t i = 0; i < m_pBufferWrapper->m_unSize; ++i)
    {
        if (m_pBufferWrapper->m_punBuffer[i] == NULL_TERMINATOR)
        {
            if (!currentPacket.empty())
            {
                packets.push_back(currentPacket);
                currentPacket.clear();
            }
        }
        else
        {
            currentPacket += m_pBufferWrapper->m_punBuffer[i];
        }
    }

    if (!currentPacket.empty())
    {
        packets.push_back(currentPacket);
    }

    if (packets.empty())
    {
        std::cerr << "No packets found" << std::endl;
        return;
    }

    for (size_t i = 1; i < packets.size(); i += 2)
    {
        if (i + 1 < packets.size())
        {
            m_mapRules[packets[i]] = packets[i + 1];
        }
        else
        {
            std::cerr << "Incomplete rule-value pair" << std::endl;
        }
    }

    for (const auto& rule : m_mapRules)
    {
        std::cout << rule.first << ": " << rule.second << std::endl;
    }
}

void A2SQueryHandler::CopyRulesMap(std::map<std::string, std::string>& mapRulesRef) const
{
    mapRulesRef = m_mapRules;
}
