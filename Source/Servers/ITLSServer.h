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
#include <openssl\bio.h>
#include <openssl\ssl.h>
#include <openssl\err.h>
#include <unordered_map>

// Server-information that can be extended in other classes.
#pragma pack(push, 1)
struct ITLSServerinfo : public ITCPServerinfo
{
	std::unordered_map<size_t, SSL_CTX *> Context;    
    std::unordered_map<size_t, BIO *> Write_BIO;
    std::unordered_map<size_t, BIO *> Read_BIO;    
    std::unordered_map<size_t, SSL *> State;

	const char *SSLKey = "";
	const char *SSLCert = "";
};
#pragma pack(pop)

struct ITLSServer : public ITCPServer
{
    // Callback and methods to insert data.
    virtual void Senddata(const size_t Socket, std::string &Databuffer) override;
    virtual void Senddata(const size_t Socket, const void *Databuffer, const size_t Datalength) override;
    virtual void onStreamupdated(const size_t Socket, std::vector<uint8_t> &Incomingstream) override;
    virtual void onStreamdecrypted(const size_t Socket, std::string &Incomingstream) = 0;

	// TLS-state management.
	virtual void onConnect(const size_t Socket, const uint16_t Port) override;
	virtual void onDisconnect(const size_t Socket) override;
    virtual void Syncbuffers(const size_t Socket);

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
