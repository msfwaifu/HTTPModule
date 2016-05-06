/*
    Initial author: (https://github.com/)Convery for Ayria.se
    License: LGPL 3.0
    Started: 2016-5-6
    Notes:
*/

#pragma once

#include <Configuration\All.h>
#include <Servers\IHTTPServer.h>

struct Testserver : public IHTTPSServer
{
    // Callbacks on data.
    virtual void onGET(std::string &URL, std::string Body) override;
    virtual void onPUT(std::string &URL, std::string Body) override;
    virtual void onPOST(std::string &URL, std::string Body) override;

    // Construct the server from a hostname.
    Testserver() : IHTTPSServer() {};
    Testserver(const char *Hostname) : IHTTPSServer(Hostname) {};
    Testserver(const char *Hostname, const char *Certificate, const char *Key) : IHTTPSServer(Hostname, Certificate, Key) {};
};
