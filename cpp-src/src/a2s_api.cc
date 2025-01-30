#include <arpa/inet.h>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <map>
#include <netinet/in.h>
#include <string>
#include <strings.h>
#include <sys/socket.h>  // socket()
#include <unistd.h>      // close()
#include <vector>

#define FAILURE -1
#define MAX_PAYLOAD 1400
#define A2S_RULES 0x56

/*
 * Documentation:
 * - https://beej.us/guide/bgnet/html/
 */

/*
 * Steps:
 * 1. Create the socket
 * 2. Connect (bind) to the proper IP address and port - NOT NEEDED FOR UDP
 * 3. Write to the socket with an empty challenge number.
 * 4. Read the response from the socket.
 *   - another write/read might be needed for the challenge number.
 * 5. Close the socekt.
 */

void printHex(const uint8_t* data, size_t size)
{
    for (size_t i = 0; i < size; ++i)
    {
        if (i % 16 == 0)
        {
            printf("\n");
        }
        else if (i % 8 == 0)
        {
            printf("\t");
        }
        else
        {
            printf("%02X ", data[i]);
        }
    }
    printf("\n\n");
}

int main()
{
    char buffer[MAX_PAYLOAD];

    struct sockaddr_in serverAddr;
    memset(&serverAddr, 0, sizeof(serverAddr));
    serverAddr.sin_family      = AF_INET;
    serverAddr.sin_port        = htons(27015);
    serverAddr.sin_addr.s_addr = inet_addr("47.152.10.229");

    int sock = socket(AF_INET, SOCK_DGRAM, 0);  // Internet, UDP, Default

    uint8_t queryData[] = { 0xFF, 0xFF, 0xFF, 0xFF, A2S_RULES, 0xFF, 0xFF, 0xFF, 0xFF };
    sendto(
        sock, queryData, sizeof(queryData), 0, (struct sockaddr*) &serverAddr, sizeof(serverAddr));

    ssize_t bufferSize = recvfrom(sock, buffer, sizeof(buffer), 0, NULL, NULL);

    if (buffer[4] == 0x41)  // Checking for challenge response
    {
        uint32_t challengeNumber = *reinterpret_cast<uint32_t*>(&buffer[5]);

        uint8_t queryWithChall[9] = { 0xFF, 0xFF, 0xFF, 0xFF, A2S_RULES };

        for (int i = 0; i < 4; i++)
        {
            queryWithChall[5 + i] = static_cast<uint8_t>((challengeNumber >> (8 * i)) & 0xFF);
        }

        sendto(sock,
               queryWithChall,
               sizeof(queryWithChall),
               0,
               (struct sockaddr*) &serverAddr,
               sizeof(serverAddr));

        bufferSize = recvfrom(sock, buffer, sizeof(buffer), 0, NULL, NULL);
    }

    /* CREATE THE MAP OF VALUES FROM THE RESPONSE */
    std::vector<std::string> packets;
    std::string              currentPacket;

    for (size_t i = 0; i < bufferSize; ++i)
    {
        if (buffer[i] == 0x00)
        {
            if (!currentPacket.empty())
            {
                packets.push_back(currentPacket);
                currentPacket.clear();
            }
        }
        else
        {
            currentPacket += buffer[i];
        }
    }

    if (!currentPacket.empty())
    {
        packets.push_back(currentPacket);
    }

    if (packets.empty())
    {
        std::cerr << "No packets found" << std::endl;
        return -1;
    }

    std::map<std::string, std::string> rulesMap;
    for (size_t i = 1; i < packets.size(); i += 2)
    {
        if (i + 1 < packets.size())
        {
            rulesMap[packets[i]] = packets[i + 1];
        }
        else
        {
            std::cerr << "Incomplete rule-value pair!" << std::endl;
        }
    }

    printHex(reinterpret_cast<uint8_t*>(buffer), bufferSize);

    std::string header = packets[0];
    std::cout << "Header : " << header << std::endl;

    // Output the rules map
    for (const auto& rule : rulesMap)
    {
        std::cout << rule.first << " : " << rule.second << std::endl;
    }

    close(sock);

    return 0;
}
