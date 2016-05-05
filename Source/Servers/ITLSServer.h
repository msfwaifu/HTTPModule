/*
    Initial author: (https://github.com/)Convery for Ayria.se
    License: LGPL 3.0
    Started: 2016-5-5
    Notes:
        Buffered server with a callback on incoming data.
*/

#pragma once
#include <Configuration\All.h>
#include <Servers\ITCPServer.h>
#include <openssl/err.h>
#include <openssl/ssl.h>
#include <openssl/conf.h>
#include <openssl/engine.h>

// Server-information that can be extended in other classes.
#pragma pack(push, 1)
struct ITLSServerinfo : public ITCPServerinfo
{
    SSL_CTX *Context;
    BIO *SSL_BIO;
    BIO *APP_BIO;
    SSL *State;
};
#pragma pack(pop)

struct ITLSServer : public ITCPServer
{
    // Single-socket operations.
    virtual void onConnect(const size_t Socket, const uint16_t Port) override;

    // Callback and methods to insert data.
    virtual void Senddata(std::string &Databuffer) override;
    virtual void Senddata(const void *Databuffer, const size_t Datalength) override;
    virtual void onStreamupdated(std::vector<uint8_t> &Incomingstream) override;
    virtual void onStreamdecrypted(std::string &Incomingstream) = 0;

    // Server information in a raw format.
    ITLSServerinfo *GetServerinfo()
    {
        return (ITLSServerinfo *)Serverinfo;
    }

    // Construct the server from a hostname.
    ITLSServer();
    ITLSServer(const char *Hostname);
    ITLSServer(const char *Hostname, const char *Certificate, const char *Key);
};
