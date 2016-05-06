/*
    Initial author: (https://github.com/)Convery for Ayria.se
    License: LGPL 3.0
    Started: 2016-5-6
    Notes:
        Testing the implementation of TLS.
*/

#include <Configuration\All.h>
#include <Servers\HTTPSServer.h>

// Callback and methods to insert data.
void HTTPSServer::onStreamdecrypted(std::string &Incomingstream)
{
    DebugPrint(Incomingstream.c_str());
}
