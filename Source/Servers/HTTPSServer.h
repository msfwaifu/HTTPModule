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
    // Callback and methods to insert data.
    virtual void onStreamdecrypted(std::string &Incomingstream) override;

    // Construct the server from a hostname.
    HTTPSServer() : ITLSServer() {};
    HTTPSServer(const char *Hostname) : ITLSServer(Hostname) {};
    HTTPSServer(const char *Hostname, const char *Certificate, const char *Key) : ITLSServer(Hostname, Certificate, Key) {};
};
