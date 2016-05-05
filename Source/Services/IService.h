/*
    Initial author: (https://github.com/)Convery for Ayria.se
    License: LGPL 3.0
    Started: 2016-4-25
    Notes:
        A service is a submodule for a server that handles requests.
        For example handling REST paths or custom protocol requests.
*/

#pragma once
#include <Configuration\All.h>
#include <Servers\IServer.h>

struct IService
{
    // If a subtype is used, e.g. Account::GetUsername. ServiceID::TaskID.
    uint8_t TaskID;

    // Service information and handler. Handler returns if the data was handled.
    virtual bool HandlePacket(struct IServer *Caller, std::string &PacketData) = 0;
    virtual uint32_t ServiceID() = 0;
};
