/*
    Initial author: (https://github.com/)Convery for Ayria.se
    License: LGPL 3.0
    Started: 2016-04-13
    Notes:
        A very basic buffer where data is stored with a byte-prefix.
        The buffer will grow as needed, but will not shrink.
*/

#include <algorithm>
#include "Bytebuffer.h"
#include <Configuration\All.h>

// Constructors, any data passed fills Internalstorage.
#pragma optimize( "", off )
Bytebuffer::Bytebuffer(size_t Inputlength, const void *Inputdata)
{
    if (Inputdata != nullptr)
        Internalstorage.append((uint8_t *)Inputdata, Inputlength);
    else
        Internalstorage.reserve(Inputlength);

    Storageiterator = 0;
}
Bytebuffer::Bytebuffer(std::basic_string<uint8_t> *Inputdata)
{
    Internalstorage.insert(0, *Inputdata);
    Storageiterator = 0;
}
Bytebuffer::Bytebuffer(std::string *InputData)
{
    Internalstorage.append((uint8_t *)InputData->data(), InputData->length());
    Storageiterator = 0;
}
Bytebuffer::Bytebuffer()
{
    Storageiterator = 0;
}
Bytebuffer::~Bytebuffer()
{
    // Clear the memory and deallocate.
    std::fill(Internalstorage.begin(), Internalstorage.end(), 0);
    Internalstorage.clear();
    Internalstorage.shrink_to_fit();
    Storageiterator = 0;
}
#pragma optimize( "", on )

// Access to the internal storage.
size_t Bytebuffer::Size()
{
    return Internalstorage.size();
}
size_t Bytebuffer::Length()
{
    return Size();
}
size_t Bytebuffer::Position()
{
    return Storageiterator;
}
bool Bytebuffer::SetPosition(size_t Value)
{
    if (Value < Internalstorage.size())
        Storageiterator = Value;

    return Storageiterator == Value;
}
uint8_t Bytebuffer::Peek()
{
    uint8_t Byte = 0xFF;
    if (RawRead(1, &Byte))
        SetPosition(Position() - 1);

    return Byte;
}
void Bytebuffer::Rewind()
{
    SetPosition(0);
}
void Bytebuffer::Clear()
{
    // Clear the memory and deallocate.
    std::fill(Internalstorage.begin(), Internalstorage.end(), 0);
    Internalstorage.clear();
    Internalstorage.shrink_to_fit();
    Storageiterator = 0;
}
const uint8_t *Bytebuffer::Data()
{
    return Internalstorage.data();
}

// Core functionality.
bool Bytebuffer::ReadDatatype(BytebufferType Type)
{
    if (Peek() == Type)
        return SetPosition(Position() + 1);
    else
        return false;
}
bool Bytebuffer::WriteDatatype(BytebufferType Type)
{
    return RawWrite(1, &Type);
}
bool Bytebuffer::RawRead(size_t Readcount, void *Buffer)
{
    // Range check, we do not make partial reads.
    if ((Storageiterator + Readcount) > Internalstorage.size())
    {
        DebugPrint(va_small("%s tried to read out of bounds, missing bytes: %i", __func__, (Storageiterator + Readcount) - Internalstorage.size()));
        return false;
    }

    // If a buffer was provided, copy into it.
    if(Buffer) 
        std::memcpy(Buffer, Internalstorage.data() + Storageiterator, Readcount);

    // Increment the iterator and return.
    Storageiterator += Readcount;
    return true;
}
bool Bytebuffer::RawWrite(size_t Writecount, const void *Buffer)
{
    // If a buffer was not provided, we'll just increment the iterator.
    if (!Buffer)
    {
        // Any old data remains in the buffer, this is intended behavior.
        Storageiterator += Writecount;
        if (Storageiterator > Internalstorage.size())
            if(Storageiterator < Internalstorage.size() + 1024)
                Internalstorage.reserve(Storageiterator);
            else
                DebugPrint(va_small("%s tried to increase the buffer by %i bytes", __func__, (Storageiterator + Writecount) - Internalstorage.size()));
        return true;
    }

    // If we are at the end of the buffer, append.
    if (Storageiterator == Internalstorage.size())
    {
        Internalstorage.append((const uint8_t *)Buffer, Writecount);
        Storageiterator += Writecount;
        return true;
    }

    // Range check, the internal state has been corrupted and should be debugged.
    if (Storageiterator > Internalstorage.size())
    {
        DebugPrint(va_small("%s tried to write out of bounds, missing bytes: %i", __func__, (Storageiterator + Writecount) - Internalstorage.size()));
        return false;
    }

    // If there's room in the buffer, copy the data.
    if ((Storageiterator + Writecount) < Internalstorage.size())
    {
        Internalstorage.replace(Storageiterator, Writecount, (const uint8_t *)Buffer, Writecount);
        Storageiterator += Writecount;
        return true;
    }

    // Write the data as both types.
    size_t Allocated = Internalstorage.size() - Storageiterator;
    return RawWrite(Allocated, Buffer) && RawWrite(Writecount - Allocated, (uint8_t *)Buffer + Allocated);
}

// Simple specialized read/write templates.
#define SIMPLE_TEMPLATE(Type, Enum)                                     \
template <> bool Bytebuffer::Read(Type *Buffer, bool Typechecked)       \
{                                                                       \
    if (!Typechecked || ReadDatatype(Enum))                             \
        return RawRead(sizeof(*Buffer), Buffer);                        \
    return false;                                                       \
};                                                                      \
template <> Type Bytebuffer::Read(bool Typechecked)                     \
{                                                                       \
    Type Result{};                                                      \
    Read(&Result, Typechecked);                                         \
    return Result;                                                      \
};                                                                      \
template <> bool Bytebuffer::Write(const Type Buffer, bool Typechecked) \
{                                                                       \
    if (Typechecked)                                                    \
        WriteDatatype(Enum);                                            \
    return RawWrite(sizeof(Buffer), &Buffer);                           \
};

SIMPLE_TEMPLATE(bool, BB_BOOL);
SIMPLE_TEMPLATE(char, BB_SINT8);
SIMPLE_TEMPLATE(int8_t, BB_SINT8);
SIMPLE_TEMPLATE(uint8_t, BB_UINT8);
SIMPLE_TEMPLATE(int16_t, BB_SINT16);
SIMPLE_TEMPLATE(uint16_t, BB_UINT16);
SIMPLE_TEMPLATE(int32_t, BB_SINT32);
SIMPLE_TEMPLATE(uint32_t, BB_UINT32);
SIMPLE_TEMPLATE(int64_t, BB_SINT64);
SIMPLE_TEMPLATE(uint64_t, BB_UINT64);
SIMPLE_TEMPLATE(float, BB_FLOAT32);
SIMPLE_TEMPLATE(double, BB_FLOAT64);

// Advanced datatypes.
bool Bytebuffer::ReadBlob(std::string *Buffer, bool Typechecked)
{
    if (!Typechecked || ReadDatatype(BB_BLOB))
    {
        uint32_t Bloblength = Read<uint32_t>();

        // Range check, same as would be done in RawRead().
        if ((Position() + Bloblength) > Size())
        {
            DebugPrint(va_small("%s, blob(%u) is larger than the buffer. Verify your endians.", __func__, Bloblength));
            return false;
        }

        // Very slow read.
        while (Bloblength--)
            Buffer->push_back(Read<uint8_t>(false));
        return true;
    }
    return false;
}
bool Bytebuffer::ReadBlob(std::basic_string<uint8_t> *Buffer, bool Typechecked)
{
    if (!Typechecked || ReadDatatype(BB_BLOB))
    {
        uint32_t Bloblength = Read<uint32_t>();

        // Range check, same as would be done in RawRead().
        if ((Position() + Bloblength) > Size())
        {
            DebugPrint(va_small("%s, blob(%u) is larger than the buffer. Verify your endians.", __func__, Bloblength));
            return false;
        }

        // Very slow read.
        while (Bloblength--)
            Buffer->push_back(Read<uint8_t>(false));
        return true;
    }
    return false;
}
bool Bytebuffer::ReadBlob(uint32_t Bufferlength, void *Bufferdata, bool Typechecked)
{
    if (!Typechecked || ReadDatatype(BB_BLOB))
    {
        uint32_t Bloblength = Read<uint32_t>();

        // Range check, same as would be done in RawRead().
        if ((Position() + Bloblength) > Size())
        {
            DebugPrint(va_small("%s, blob(%u) is larger than the buffer. Verify your endians.", __func__, Bloblength));
            return false;
        }

        return RawRead(std::min(Bloblength, Bufferlength), Bufferdata);
    }
    return false;
}
std::string Bytebuffer::ReadBlob(bool Typechecked)
{
    std::string Result;
    ReadBlob(&Result, Typechecked);
    return Result;
}
bool Bytebuffer::ReadString(std::string *Buffer, bool Typechecked)
{
    if (!Typechecked || ReadDatatype(BB_ASCIISTRING))
    {
        size_t Length = std::strlen((char *)Internalstorage.data() + Storageiterator) + 1;
        Buffer->append((char *)Internalstorage.data() + Storageiterator);
        return RawRead(Length);
    }

    return false;
}
std::string Bytebuffer::ReadString(bool Typechecked)
{
    std::string Result;
    ReadString(&Result, Typechecked);
    return Result;
}
bool Bytebuffer::WriteBlob(const std::string *Buffer, bool Typechecked)
{
    if (Typechecked)
        WriteDatatype(BB_BLOB);

    return Write(uint32_t(Buffer->size())) && RawWrite(Buffer->size(), Buffer->data());
}
bool Bytebuffer::WriteBlob(const std::basic_string<uint8_t> *Buffer, bool Typechecked)
{
    if (Typechecked)
        WriteDatatype(BB_BLOB);

    return Write(uint32_t(Buffer->size())) && RawWrite(Buffer->size(), Buffer->data());
}
bool Bytebuffer::WriteBlob(uint32_t Bufferlength, const void *Bufferdata, bool Typechecked)
{
    if (Typechecked)
        WriteDatatype(BB_BLOB);

    return Write(Bufferlength) && RawWrite(Bufferlength, Bufferdata);
}
bool Bytebuffer::WriteString(const std::string *Buffer, bool Typechecked)
{
    if (Typechecked)
        WriteDatatype(BB_ASCIISTRING);

    return RawWrite(Buffer->size() + 1, Buffer->c_str());
}
