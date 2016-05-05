/*
    Initial author: (https://github.com/)Convery for Ayria.se
    License: LGPL 3.0
    Started: 2016-4-25
    Notes:
        The IResponse class takes and array of objects to be serialized.
        Then formats the package to fit the protocol being emulated.
*/

#pragma once
#include <Configuration\All.h>
#include <Datatypes\ISerializable.h>
#include <Servers\ITCPServer.h>
#include <Servers\IUDPServer.h>
#include <memory>

class IResponse
{
protected:
    std::vector<std::shared_ptr<ISerializable *>> Internalbuffer;
    ITCPServer *TCPParentserver;
    IUDPServer *UDPParentserver;

public:
    uint8_t Type;
    uint8_t Subtype;
    uint32_t Errorcode;

    void Addobject(ISerializable *Object)
    {
        Internalbuffer.push_back(std::make_shared<ISerializable *>(Object));
    }
    virtual size_t Sendobjects() = 0;

    // Constructor using the a server for .
    IResponse(ITCPServer *Parent)
    {
        TCPParentserver = Parent;
        UDPParentserver = nullptr;
    }
    IResponse(IUDPServer *Parent)
    {
        TCPParentserver = nullptr;
        UDPParentserver = Parent;
    }
    IResponse()
    {
        TCPParentserver = nullptr;
        UDPParentserver = nullptr;
    }
};
