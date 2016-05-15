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

typedef struct HTTPHeader
{
    std::string Field;
    std::string Value;
};
typedef struct HTTPRequest
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
    virtual void onGET(HTTPRequest &Request) = 0;
    virtual void onPUT(HTTPRequest &Request) = 0;
    virtual void onPOST(HTTPRequest &Request) = 0;
    virtual void onCOPY(HTTPRequest &Request) = 0;
    virtual void onDELETE(HTTPRequest &Request) = 0;
    virtual void onStreamupdated(std::vector<uint8_t> &Incomingstream) override;

    // Local parser.
    http_parser Parser;
    HTTPRequest Parsedrequest;
    http_parser_settings Parsersettings;
    
    // Construct the server from a hostname.
    IHTTPServer();
    IHTTPServer(const char *Hostname);
};

// Encrypted HTTP.
struct IHTTPSServer : public ITLSServer
{
    // Callbacks on data.
    virtual void onGET(HTTPRequest &Request) = 0;
    virtual void onPUT(HTTPRequest &Request) = 0;
    virtual void onPOST(HTTPRequest &Request) = 0;
    virtual void onCOPY(HTTPRequest &Request) = 0;
    virtual void onDELETE(HTTPRequest &Request) = 0;
    virtual void onStreamdecrypted(std::string &Incomingstream) override;

    // Local parser.
    http_parser Parser;
    HTTPRequest Parsedrequest;
    http_parser_settings Parsersettings;

    // Construct the server from a hostname.
    IHTTPSServer();
    IHTTPSServer(const char *Hostname);
    IHTTPSServer(const char *Hostname, const char *Certificate, const char *Key);
};
