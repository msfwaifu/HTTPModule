/*
    Initial author: (https://github.com/)Convery for Ayria.se
    License: LGPL 3.0
    Started: 2016-4-24
    Notes:
        The basic interface for exporting servers from modules.
        The system should be implemented as nonblocking.
*/

#pragma once
#include <Configuration\All.h>

// Server-information that can be extended in other classes.
#pragma pack(push, 1)
struct IServerinfo
{
    char Hostname[64];
    uint8_t Hostinfo[16];
    uint32_t Hostaddress;
    bool Extendedserver;
};
#pragma pack(pop)

// The simple interface, implement everything yourself.
struct IServer
{
    // Returns false if the request could not be completed.
    virtual bool onReadrequest(char *Databuffer, size_t *Datalength) = 0;
    virtual bool onWriterequest(const char *Databuffer, const size_t Datalength) = 0;

    // Server information in a raw format.
    uint8_t Serverinfo[128];
    IServerinfo *GetServerinfo()
    {
        return (IServerinfo *)Serverinfo;
    }

    // Construct the server from a hostname.
    IServer()
    {
        // Clear the serverinfo and set the correct interface.
        std::memset(Serverinfo, 0, sizeof(Serverinfo));
        GetServerinfo()->Extendedserver = 0;
    }
    IServer(const char *Hostname)
    {
        // Clear the serverinfo and set the correct interface.
        std::memset(Serverinfo, 0, sizeof(Serverinfo));
        GetServerinfo()->Extendedserver = 0;

        // Copy the host information and address.
        std::strncpy(GetServerinfo()->Hostname, Hostname, 63);
        GetServerinfo()->Hostaddress = FNV1a_Runtime_32(Hostname, std::strlen(Hostname));
    }
};

// The extended interface, when you need even more customization.
struct IServerEx : public IServer
{
    // Per socket operations.
    virtual void onDisconnect(const size_t Socket) = 0;
    virtual void onConnect(const size_t Socket, const uint16_t Port) = 0;
    virtual bool onReadrequestEx(const size_t Socket, char *Databuffer, size_t *Datalength) = 0;
    virtual bool onWriterequestEx(const size_t Socket, const char *Databuffer, const size_t Datalength) = 0;

    // Construct the server from a hostname.
    IServerEx() : IServer()
    {
        // Set the info to extended mode.
        GetServerinfo()->Extendedserver = 1;
    }
    IServerEx(const char *Hostname) : IServer(Hostname)
    {
        // Set the info to extended mode.
        GetServerinfo()->Extendedserver = 1;
    }
};
