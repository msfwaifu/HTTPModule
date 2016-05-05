/*
    Initial author: (https://github.com/)Convery for Ayria.se
    License: LGPL 3.0
    Started: 2016-04-13
    Notes:
        The entrypoint for this networkmodule.
*/

#include <Configuration\All.h>
#include <cstdarg>

extern "C"
{
    // AyriaNetwork interface, create an instance of a server.
    EXPORT_ATTR struct IServer *GetServerinstance(const char *Hostname)
    {
        return nullptr;
    }
}

#ifdef _WIN32
#include <Windows.h>

BOOLEAN WINAPI DllMain(HINSTANCE hDllHandle, DWORD nReason, LPVOID Reserved)
{
    switch ( nReason )
    {
    case DLL_PROCESS_ATTACH:
        // Rather not handle all thread updates.
        DisableThreadLibraryCalls( hDllHandle );

        // Clean the logfile so we only save this session.
        DeleteLogfile();
        break;
    }

    return TRUE;
}

#else

void __attribute__((constructor)) SOMain()
{
    // Clean the logfile so we only save this session.
    DeleteLogfile();
}
#endif
