/*
 * Copyright (C) Alex Nekipelov (alex@nekipelov.net)
 * License: MIT
 * Original code: https://github.com/nekipelov/httpparser
 */

#ifndef HTTPPARSER_RESPONSE_H
#define HTTPPARSER_RESPONSE_H

#include <sstream>
#include <string>
#include <vector>

namespace httpparser
{

struct Response
{
    Response() : versionMajor(0), versionMinor(0), keepAlive(false), statusCode(0)
    {
    }

    struct HeaderItem
    {
        std::string name;
        std::string value;
    };

    int versionMajor;
    int versionMinor;
    std::vector<HeaderItem> headers;
    std::vector<char> content;
    bool keepAlive;

    unsigned int statusCode;
    std::string status;

    std::string inspect() const
    {
        /**
         * Serializes the response into a string.
         */
        std::stringstream stream;
        stream << "HTTP/" << versionMajor << "." << versionMinor << " " << statusCode << " " << status << "\r\n";

        for (const auto &header : headers)
        {
            stream << header.name << ": " << header.value << "\r\n";
        }

        stream << "\r\n";

        std::string data(content.begin(), content.end());
        stream << data;
        return stream.str();
    }
};

} // namespace httpparser

#endif // HTTPPARSER_RESPONSE_H
