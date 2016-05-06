/*
    Initial author: (https://github.com/)Convery for Ayria.se
    License: LGPL 3.0
    Started: 2016-5-6
    Notes:
        HTTP over TLS.
*/

#include <Configuration\All.h>
#include <Servers\HTTPSServer.h>
#include "http_parser.h"

typedef struct 
{
    const char* field;
    const char* value;
    size_t field_length;
    size_t value_length;
} http_header_t;
typedef struct 
{
    char* url;
    char* method;
    int header_lines;
    http_header_t headers[32];
    const char* body;
} http_request_t;

static http_parser_settings Parsersettings;
static http_request_t Parsedrequest;
static http_parser Parser;
static HTTPSServer *Server;

// Callback and methods to insert data.
void HTTPSServer::onStreamdecrypted(std::string &Incomingstream)
{
    DebugPrint(Incomingstream.c_str());
    http_parser_execute(&Parser, &Parsersettings, Incomingstream.c_str(), Incomingstream.size());
}

// Construct the server from a hostname.
HTTPSServer::HTTPSServer() : ITLSServer() {};
HTTPSServer::HTTPSServer(const char *Hostname) : ITLSServer(Hostname) {};
HTTPSServer::HTTPSServer(const char *Hostname, const char *Certificate, const char *Key) : ITLSServer(Hostname, Certificate, Key)
{
    Server = this;

    http_parser_init(&Parser, HTTP_BOTH);
    Parsersettings.on_message_begin = [](http_parser *parser) -> int
    {
        Parsedrequest.header_lines = 0;
        return 0;
    };
    Parsersettings.on_url = [](http_parser* parser, const char* at, size_t len) -> int
    {
        Parsedrequest.url = new char[len + 1]();
        strncpy((char*) Parsedrequest.url, at, len);
        return 0;
    };
    Parsersettings.on_header_field = [](http_parser* parser, const char* at, size_t len) -> int
    {
        http_header_t *Header = &Parsedrequest.headers[Parsedrequest.header_lines];
        Header->field = new char [len + 1]();
        Header->field_length = len;

        strncpy((char*)Header->field, at, len);
        return 0;
    };
    Parsersettings.on_header_value = [](http_parser* parser, const char* at, size_t len) -> int
    {
        http_header_t *Header = &Parsedrequest.headers[Parsedrequest.header_lines];
        Header->value = new char [len + 1]();
        Header->value_length = len;

        strncpy((char*)Header->value, at, len);
        Parsedrequest.header_lines++;
        return 0;
    };
    Parsersettings.on_headers_complete = [](http_parser* parser) -> int
    {
        const char *Method = http_method_str((http_method)Parser.method);
        Parsedrequest.method = new char[std::strlen(Method) + 1]();
        strncpy(Parsedrequest.method, Method, strlen(Method));

        return 0;
    };
    Parsersettings.on_body = [](http_parser* parser, const char* at, size_t len) -> int
    {
        Parsedrequest.body = new char[len + 1]();
        strncpy((char *)Parsedrequest.body, at, len);

        return 0;
    };
    Parsersettings.on_message_complete = [](http_parser* parser) -> int
    {
        switch (FNV1_Runtime_32(Parsedrequest.method, std::strlen(Parsedrequest.method)))
        {
        case FNV1a_Compiletime_32("GET"):

        default:
            break;
        }

        return 0;
    };
};
