/*
    Initial author: (https://github.com/)Convery for Ayria.se
    License: LGPL 3.0
    Started: 2016-04-13
    Notes:
        A very basic buffer where data is stored with a byte-prefix.
        The buffer will grow as needed, but will not shrink.
*/

#pragma once
#include <string>
#include <memory>

// The types of data we can store.
enum BytebufferType : uint8_t
{
    BB_NONE = 0,
    BB_BOOL = 1,
    BB_SINT8 = 2,
    BB_UINT8 = 3,
    BB_SINT16 = 4,
    BB_UINT16 = 5,
    BB_SINT32 = 6,
    BB_UINT32 = 7,
    BB_SINT64 = 8,
    BB_UINT64 = 9,
    BB_FLOAT32 = 10,
    BB_FLOAT64 = 11,
    BB_ASCIISTRING = 12,
    BB_UNICODESTRING = 13,
    BB_BLOB = 14,

    BB_EOS = 99,
    BB_MAX
};

class Bytebuffer
{
    std::basic_string<uint8_t> Internalstorage;
    size_t Storageiterator;

public:
    // Constructors, any data passed fills Internalstorage.
    Bytebuffer(size_t Inputlength, const void *Inputdata = nullptr);
    Bytebuffer(std::basic_string<uint8_t> *Inputdata);
    Bytebuffer(std::string *Inputdata);
    Bytebuffer();
    ~Bytebuffer();

    // Access to the internal storage.
    size_t Size();                          // Buffer length.
    size_t Length();                        // Wrapper for Size().
    size_t Position();                      // Get the iterator.
    bool SetPosition(size_t Value);         // Set the iterator, false if out of range.
    uint8_t Peek();                         // Read one byte ahead, 0xFF on error.
    void Rewind();                          // Wrapper for SetPosition(0).
    void Clear();                           // Deallocates everything in the buffer.
    const uint8_t *Data();                  // Get a pointer to the buffers data.

    // Core functionality.
    bool ReadDatatype(BytebufferType Type);
    bool WriteDatatype(BytebufferType Type);
    bool RawRead(size_t Readcount, void *Buffer = nullptr);
    bool RawWrite(size_t Writecount, const void *Buffer = nullptr);

    // Simple datatypes.
    template <typename Type> Type Read(bool Typechecked = true);
    template <typename Type> bool Read(Type *Buffer, bool Typechecked = true);
    template <typename Type> bool Write(const Type Value, bool Typechecked = true);

    // Advanced datatypes.
    bool ReadBlob(std::string *Buffer, bool Typechecked = true);
    bool ReadBlob(std::basic_string<uint8_t> *Buffer, bool Typechecked = true);
    bool ReadBlob(uint32_t Bufferlength, void *Bufferdata, bool Typechecked = true);
    std::string ReadBlob(bool Typechecked = true);
    bool ReadString(std::string *Buffer, bool Typechecked = true);
    std::string ReadString(bool Typechecked = true);
    bool WriteBlob(const std::string *Buffer, bool Typechecked = true);
    bool WriteBlob(const std::basic_string<uint8_t> *Buffer, bool Typechecked = true);
    bool WriteBlob(uint32_t Bufferlength, const void *Bufferdata, bool Typechecked = true);
    bool WriteString(const std::string *Buffer, bool Typechecked = true);
};
