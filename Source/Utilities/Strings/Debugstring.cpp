/*
    Initial author: (https://github.com/)Convery for Ayria.se
    License: LGPL 3.0
    Started: 2016-04-13
    Notes:
        A simple system for logging strings to files.
*/

#include <Configuration\All.h>
#include <Utilities\All.h>
#include "Debugstring.h"
#include <stdio.h>
#include <mutex>

// Full filepath to the logfile.
constexpr const char *Filepath = LOGFILEDIR EXTENSIONNAME ".log";

void AppendToLogfile(const char *Message, const char *Prefix)
{
    std::FILE *Filehandle;
    static std::mutex WriteMutex;

    WriteMutex.lock();
    {
        Filehandle = fopen(Filepath, "a");
        if (Filehandle)
        {
            fprintf(Filehandle, "[%-7s] ", Prefix);
            fputs(Message, Filehandle);
            fputs("\n", Filehandle);
            fclose(Filehandle);
        }

        // Duplicate the message to stdout.
        {
#ifdef DEBUGTOSTREAM
            fprintf(stdout, "[%-7s] ", Prefix);
            fputs(Message, stdout);
            fputs("\n", stdout);
#endif
        }
    }
    WriteMutex.unlock();
}
void DeleteLogfile()
{
    Filesystem::Createdir(LOGFILEDIR);
    std::remove(Filepath);
}
