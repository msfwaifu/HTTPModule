/*
    Initial author: (https://github.com/)Convery for Ayria.se
    License: LGPL 3.0
    Started: 2016-5-6
    Notes:
        Testing the implementation of TLS.
*/

#pragma once
#include <Configuration\All.h>
#include <Servers\ITLSServer.h>

struct HTTPSServer : public ITLSServer
{
    // Callbacks from TLS.
    virtual void onStreamdecrypted(std::string &Incomingstream) override;
    
    // Callbacks on data.
    virtual void onGET(std::string &URL, std::string Body) = 0;
    virtual void onPUT(std::string &URL, std::string Body) = 0;
    virtual void onPOST(std::string &URL, std::string Body) = 0;

    // Construct the server from a hostname.
    HTTPSServer();
    HTTPSServer(const char *Hostname);
    HTTPSServer(const char *Hostname, const char *Certificate, const char *Key);
};
