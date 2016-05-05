/*
    Initial author: (https://github.com/)Convery for Ayria.se
    License: LGPL 3.0
    Started: 2016-4-24
    Notes:
        Buffered server with a callback on incoming data.
*/

#pragma once
#include <Configuration\All.h>
#include <Servers\IServer.h>
#include <algorithm>
#include <queue>
#include <mutex>

// Basic datagram buffer server.
struct IUDPServer : public IServer
{
    // Data-queues for the packets.
    std::queue<std::string> Outgoingqueue;
    std::mutex Queueguard;

    // Returns false if the request could not be completed.
    virtual bool onReadrequest(char *Databuffer, size_t *Datalength)
    {
        // If we have no outgoing data, we can't handle the request.
        if (0 == Outgoingqueue.size())
            return false;

        // Copy as much data as we can to the buffer.
        Queueguard.lock();
        {
            std::string Packet = Outgoingqueue.front();
            Outgoingqueue.pop();

            size_t Bytesread = std::min(*Datalength, Packet.size());
            std::copy(Packet.begin(), Packet.begin() + Bytesread, Databuffer);
        }
        Queueguard.unlock();

        return true;
    }
    virtual bool onWriterequest(const char *Databuffer, const size_t Datalength)
    {
        // Simply call the callback with the data directly.
        std::string Packet((const char *)Databuffer, Datalength);
        onPacket(Packet);

        return true;
    }

    // Callback and methods to insert data.
    virtual void Senddata(std::string &Databuffer)
    {
        Queueguard.lock();
        {
            Outgoingqueue.push(Databuffer);
        }
        Queueguard.unlock();
    }
    virtual void Senddata(const void *Databuffer, const size_t Datalength)
    {
        std::string Packet((const char *)Databuffer, Datalength);
        return Senddata(Packet);
    }
    virtual void onPacket(std::string &Incomingstream) = 0;

    // Construct the server from a hostname.
    IUDPServer() : IServer() {};
    IUDPServer(const char *Hostname) : IServer(Hostname) {};
};
