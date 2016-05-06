/*
    Initial author: (https://github.com/)Convery for Ayria.se
    License: LGPL 3.0
    Started: 2016-5-6
    Notes:
*/

#pragma once

#include <Configuration\All.h>
#include <Servers\HTTPSServer.h>

struct Testserver : public HTTPSServer
{
    // Callbacks on data.
    virtual void onGET(std::string &URL, std::string Body) override;
    virtual void onPUT(std::string &URL, std::string Body) override;
    virtual void onPOST(std::string &URL, std::string Body) override;

    // Construct the server from a hostname.
    Testserver() : HTTPSServer() {};
    Testserver(const char *Hostname) : HTTPSServer(Hostname) {};
    Testserver(const char *Hostname, const char *Certificate, const char *Key) : HTTPSServer(Hostname, Certificate, Key) {};
};
