#include "a2s_query_handler.h"

#include <iostream>
#include <map>
#include <string>

int main()
{
    A2SQueryHandler handler("47.152.10.229", 27015);

    handler.QueryServerRules();
    handler.ParseUdpPacketsResponse();

    std::map<std::string, std::string> testMap;
    handler.CopyRulesMap(testMap);

    for (const auto& rule : testMap)
    {
        std::cout << rule.first << ": " << rule.second << std::endl;
    }


    return 0;
}
