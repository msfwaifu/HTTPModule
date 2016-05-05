/*
    Initial author: (https://github.com/)Convery for Ayria.se
    License: LGPL 3.0
    Started: 2016-4-24
    Notes:
        Buffered server with a callback on incoming data.
        Single-socket version.
*/

#pragma once
#include <Configuration\All.h>
#include <Servers\IServer.h>
#include <algorithm>
#include <mutex>

// Server-information that can be extended in other classes.
#pragma pack(push, 1)
struct ITCPServerinfo : public IServerinfo
{
    bool Connected;
};
#pragma pack(pop)

// Basic stream buffer server.
struct ITCPServer : public IServerEx
{
    // Data-queues for the streams.
    std::vector<uint8_t> Incomingstream;
    std::vector<uint8_t> Outgoingstream;
    std::mutex Streamguard;

    // Single-socket operations.
    virtual void onDisconnect(const size_t Socket)
    {
        GetServerinfo()->Connected = false;
    }
    virtual void onConnect(const size_t Socket, const uint16_t Port)
    {
        GetServerinfo()->Connected = true;
    }
    virtual bool onReadrequestEx(const size_t Socket, char *Databuffer, size_t *Datalength)
    {
        // If we have disconnected, we should return a length of 0.
        if (false == GetServerinfo()->Connected)
        {
            *Datalength = 0;
            return true;
        }

        // If we have no outgoing data, we can't handle the request.
        if (0 == Outgoingstream.size())
            return false;

        // Copy as much data as we can to the buffer.
        Streamguard.lock();
        {
            size_t Bytesread = std::min(*Datalength, Outgoingstream.size());
            std::copy(Outgoingstream.begin(), Outgoingstream.begin() + Bytesread, Databuffer);
            Outgoingstream.erase(Outgoingstream.begin(), Outgoingstream.begin() + Bytesread);
            *Datalength = Bytesread;
        }
        Streamguard.unlock();

        return true;
    }
    virtual bool onWriterequestEx(const size_t Socket, const char *Databuffer, const size_t Datalength)
    {
        // If we are not connected, drop the data.
        if (false == GetServerinfo()->Connected)
            return false;

        // Copy all the data into our stream.
        Streamguard.lock();
        {
            Incomingstream.insert(Incomingstream.end(), Databuffer, Databuffer + Datalength);
            onStreamupdated(Incomingstream);
        }
        Streamguard.unlock();

        return true;
    }

    // Callback and methods to insert data.
    virtual void Senddata(std::string &Databuffer)
    {
        Streamguard.lock();
        {
            const char *Bytepointer = Databuffer.c_str();
            Outgoingstream.insert(Outgoingstream.end(), Bytepointer, Bytepointer + Databuffer.size());
        }
        Streamguard.unlock();
    }
    virtual void Senddata(const void *Databuffer, const size_t Datalength)
    {
        Streamguard.lock();
        {
            const char *Bytepointer = (const char *)Databuffer;
            Outgoingstream.insert(Outgoingstream.end(), Bytepointer, Bytepointer + Datalength);
        }
        Streamguard.unlock();
    }
    virtual void onStreamupdated(std::vector<uint8_t> &Incomingstream) = 0;

    // Server information in a raw format.
    ITCPServerinfo *GetServerinfo()
    {
        return (ITCPServerinfo *)Serverinfo;
    }

    // Construct the server from a hostname.
    ITCPServer() : IServerEx() {};
    ITCPServer(const char *Hostname) : IServerEx(Hostname) {};
};

