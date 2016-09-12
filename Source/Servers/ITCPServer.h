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
#include <unordered_map>
#include <algorithm>
#include <mutex>


// Server-information that can be extended in other classes.
#pragma pack(push, 1)
struct ITCPServerinfo : public IServerinfo
{
    std::shared_ptr<std::unordered_map<size_t, bool>> Connected;
};
#pragma pack(pop)

// Basic stream buffer server.
struct ITCPServer : public IServerEx
{
    // Data-queues for the streams.
    std::unordered_map<size_t, std::vector<uint8_t>> Incomingstream;
    std::unordered_map<size_t, std::vector<uint8_t>> Outgoingstream;
    std::unordered_map<size_t, std::mutex> Streamguard;

    // Single-socket operations.
    virtual void onDisconnect(const size_t Socket)
    {
        Incomingstream[Socket].clear();
        (*GetServerinfo()->Connected)[Socket] = false;        
    }
    virtual void onConnect(const size_t Socket, const uint16_t Port)
    {
        Incomingstream[Socket].clear();
        Outgoingstream[Socket].clear();
        (*GetServerinfo()->Connected)[Socket] = true;        
    }
    virtual bool onReadrequestEx(const size_t Socket, char *Databuffer, size_t *Datalength)
    {
        // If we have disconnected, we should return a length of 0.
        // But for security layers we need to send the remaining data.
        if (false == (*GetServerinfo()->Connected)[Socket] && 0 == Outgoingstream[Socket].size())
        {
            *Datalength = 0;
            return true;
        }

        // If we have no outgoing data, we can't handle the request.
        if (0 == Outgoingstream[Socket].size())
            return false;

        // Copy as much data as we can to the buffer.
        Streamguard[Socket].lock();
        {
            size_t Bytesread = std::min(*Datalength, Outgoingstream[Socket].size());
            std::copy(Outgoingstream[Socket].begin(), Outgoingstream[Socket].begin() + Bytesread, Databuffer);
            Outgoingstream[Socket].erase(Outgoingstream[Socket].begin(), Outgoingstream[Socket].begin() + Bytesread);
            *Datalength = Bytesread;
        }
        Streamguard[Socket].unlock();

        return true;
    }
    virtual bool onWriterequestEx(const size_t Socket, const char *Databuffer, const size_t Datalength)
    {
        // If we are not connected, drop the data.
        if (false == (*GetServerinfo()->Connected)[Socket])
            return false;

        // Copy all the data into our stream.
        Streamguard[Socket].lock();
        {
            Incomingstream[Socket].insert(Incomingstream[Socket].end(), Databuffer, Databuffer + Datalength);
            onStreamupdated(Socket, Incomingstream[Socket]);
        }
        Streamguard[Socket].unlock();

        return true;
    }

    // Callback and methods to insert data.
    virtual void Senddata(const size_t Socket, std::string &Databuffer)
    {
        Streamguard[Socket].lock();
        {
            const char *Bytepointer = Databuffer.c_str();
            Outgoingstream[Socket].insert(Outgoingstream[Socket].end(), Bytepointer, Bytepointer + Databuffer.size());
        }
        Streamguard[Socket].unlock();
    }
    virtual void Senddata(const size_t Socket, const void *Databuffer, const size_t Datalength)
    {
        Streamguard[Socket].lock();
        {
            const char *Bytepointer = (const char *)Databuffer;
            Outgoingstream[Socket].insert(Outgoingstream[Socket].end(), Bytepointer, Bytepointer + Datalength);
        }
        Streamguard[Socket].unlock();
    }
    virtual void onStreamupdated(const size_t Socket, std::vector<uint8_t> &Incomingstream) = 0;

    // Server information in a raw format.
    ITCPServerinfo *GetServerinfo()
    {
        return (ITCPServerinfo *)Serverinfo;
    }

    // Construct the server from a hostname.
    ITCPServer() : IServerEx() { GetServerinfo()->Connected = std::make_shared<std::unordered_map<size_t, bool>>(); };
    ITCPServer(const char *Hostname) : IServerEx(Hostname) { GetServerinfo()->Connected = std::make_shared<std::unordered_map<size_t, bool>>(); };
};
