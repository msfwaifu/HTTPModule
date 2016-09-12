/*
    Initial author: (https://github.com/)Convery for Ayria.se
    License: LGPL 3.0
    Started: 2016-5-6
    Notes:
        Implements parsing of the HTTP protocol.
*/

#pragma once
#include <Configuration\All.h>
#include <Servers\ITCPServer.h>
#include <Servers\ITLSServer.h>
#include <HTTP\http_parser.h>
#include <unordered_map>

struct HTTPHeader
{
    std::string Field;
    std::string Value;
};
struct HTTPRequest
{
    bool Parsed;
    std::string URL;
    std::string Method;
    std::vector<HTTPHeader> Headers;
    std::string Body;
};

// Standard HTTP.
struct IHTTPServer : public ITCPServer
{
    // Callbacks on data.
    virtual void onGET(const size_t Socket, HTTPRequest &Request) = 0;
    virtual void onPUT(const size_t Socket, HTTPRequest &Request) = 0;
    virtual void onPOST(const size_t Socket, HTTPRequest &Request) = 0;
    virtual void onCOPY(const size_t Socket, HTTPRequest &Request) = 0;
    virtual void onDELETE(const size_t Socket, HTTPRequest &Request) = 0;
    virtual void onStreamupdated(const size_t Socket, std::vector<uint8_t> &Incomingstream) override;

    // Local parsers.
    std::unordered_map<size_t, http_parser> Parser;
    std::unordered_map<size_t, HTTPRequest> Parsedrequest;
    std::unordered_map<size_t, http_parser_settings> Parsersettings;
    
    // Construct the server from a hostname.
    IHTTPServer();
    IHTTPServer(const char *Hostname);
};

// Encrypted HTTP.
struct IHTTPSServer : public ITLSServer
{
    // Callbacks on data.
    virtual void onGET(const size_t Socket, HTTPRequest &Request) = 0;
    virtual void onPUT(const size_t Socket, HTTPRequest &Request) = 0;
    virtual void onPOST(const size_t Socket, HTTPRequest &Request) = 0;
    virtual void onCOPY(const size_t Socket, HTTPRequest &Request) = 0;
    virtual void onDELETE(const size_t Socket, HTTPRequest &Request) = 0;
    virtual void onStreamdecrypted(const size_t Socket, std::string &Incomingstream) override;

    // Local parsers.
    std::unordered_map<size_t, http_parser> Parser;
    std::unordered_map<size_t, HTTPRequest> Parsedrequest;
    std::unordered_map<size_t, http_parser_settings> Parsersettings;

    // Construct the server from a hostname.
    IHTTPSServer();
    IHTTPSServer(const char *Hostname);
    IHTTPSServer(const char *Hostname, const char *Certificate, const char *Key);
};
