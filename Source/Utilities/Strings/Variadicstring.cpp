/*
    Initial author: (https://github.com/)Convery for Ayria.se
    License: LGPL 3.0
    Started: 2016-04-13
    Notes:
        Create a new string from variadic arguments.
        va("%i hedgehogs!", 42) == "42 hedgehogs!" 
*/

#include <mutex>
#include <cstdarg>
#include "Variadicstring.h"

// The buffersize is the total size for each specialized version.
#define BUFFER_SIZE 1024
#define SMALL_COUNT 8       // 8 buffers of 128 bytes each.
#define LARGE_COUNT 2       // 2 buffers of 512 bytes each.

// The buffers for the output strings.
char SmallBuffer[SMALL_COUNT][BUFFER_SIZE / SMALL_COUNT];
char LargeBuffer[LARGE_COUNT][BUFFER_SIZE / LARGE_COUNT];
uint32_t SmallIndex = 0;
uint32_t LargeIndex = 0;

// Lock to prevent two strings from using the same index.
std::mutex Indexguard;

// Specialized versions.
const char *va_small(const char *fmt, ...)
{
    std::va_list VariadicList;
    char *Resultbuffer;

    // Prevent modification to the index.
    Indexguard.lock();
    {
        // Set the result to the pre-allocated buffer.
        Resultbuffer = SmallBuffer[SmallIndex];

        // Clear any old data in the buffer.
        std::memset(Resultbuffer, 0, BUFFER_SIZE / SMALL_COUNT);

        // Update the index and leave the mutex so other threads can use it.
        SmallIndex = (SmallIndex + 1) % SMALL_COUNT;
    }
    Indexguard.unlock();

    // Create a new string from the arguments and truncate if too long.
    va_start(VariadicList, fmt);
    std::vsnprintf(Resultbuffer, BUFFER_SIZE / SMALL_COUNT, fmt, VariadicList);
    va_end(VariadicList);
    
    return Resultbuffer;
}
const char *va_large(const char *fmt, ...)
{
    std::va_list VariadicList;
    char *Resultbuffer;

    // Prevent modification to the index.
    Indexguard.lock();
    {
        // Set the result to the pre-allocated buffer.
        Resultbuffer = LargeBuffer[LargeIndex];

        // Clear any old data in the buffer.
        std::memset(Resultbuffer, 0, BUFFER_SIZE / LARGE_COUNT);

        // Update the index and leave the mutex so other threads can use it.
        LargeIndex = (LargeIndex + 1) % LARGE_COUNT;
    }
    Indexguard.unlock();

    // Create a new string from the arguments and truncate if too long.
    va_start(VariadicList, fmt);
    std::vsnprintf(Resultbuffer, BUFFER_SIZE / LARGE_COUNT, fmt, VariadicList);
    va_end(VariadicList);

    return Resultbuffer;
}

// Generic version, performance warning.
const char *va(const char *fmt, ...)
{
    char Resultbuffer[BUFFER_SIZE / LARGE_COUNT]{};
    std::va_list VariadicList;
    int32_t Resultlength;    

    // Create a new string from the arguments and truncate if too long.
    va_start(VariadicList, fmt);
    Resultlength = std::vsnprintf(Resultbuffer, BUFFER_SIZE / LARGE_COUNT, fmt, VariadicList);
    va_end(VariadicList);

    // Send the string to the specialized version for storage.
    if (Resultlength > BUFFER_SIZE / SMALL_COUNT)
        return va_large(Resultbuffer);
    else
        return va_small(Resultbuffer);
}
