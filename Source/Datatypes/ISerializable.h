/*
    Initial author: (https://github.com/)Convery for Ayria.se
    License: LGPL 3.0
    Started: 2016-4-25
    Notes:
        A basic datatype that can be serialized into a bytebuffer with ease.
        This is generally used when sending data to AyriaNetwork.
*/

#pragma once
#include <Configuration\All.h>

#pragma warning(push)
#pragma warning(disable : 100)
struct ISerializable
{
    virtual void Serialize(Bytebuffer *Buffer)
    {
        DebugPrint(va_small("Got a call to %s, this needs to be debugged.", __FUNCTION__));
    }
    virtual void Deserialize(Bytebuffer *Buffer)
    {
        DebugPrint(va_small("Got a call to %s, this needs to be debugged.", __FUNCTION__));
    }
    virtual ~ISerializable() {};
};
#pragma warning(pop)
