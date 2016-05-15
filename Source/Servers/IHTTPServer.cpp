/*
    Initial author: (https://github.com/)Convery for Ayria.se
    License: LGPL 3.0
    Started: 2016-5-6
    Notes:
        Implements parsing of the HTTP protocol.
*/

#include <Configuration\All.h>
#include <Servers\IHTTPServer.h>

// Callbacks for parsing.
size_t Parse_Messagebegin(http_parser *Parser, HTTPRequest *Request)
{
    Request->Headers.clear();
    return 0;
}
size_t Parse_URL(http_parser *Parser, HTTPRequest *Request, const char *Data, size_t Length)
{
    Request->URL = std::string(Data, Length);
    return 0;
}
size_t Parse_Headerfield(http_parser *Parser, HTTPRequest *Request, const char *Data, size_t Length)
{
    HTTPHeader Header;
    Header.Field = std::string(Data, Length);

    Request->Headers.push_back(Header);
    return 0;
}
size_t Parse_Headervalue(http_parser *Parser, HTTPRequest *Request, const char *Data, size_t Length)
{
    HTTPHeader *Header = &Request->Headers.back();
    Header->Value = std::string(Data, Length);
    return 0;
}
size_t Parse_Headerscomplete(http_parser *Parser, HTTPRequest *Request)
{
    Request->Method = http_method_str((http_method)Parser->method);
    return 0;
}
size_t Parse_Body(http_parser *Parser, HTTPRequest *Request, const char *Data, size_t Length)
{
    Request->Body += std::string(Data, Length);
    return 0;
}
size_t Parse_Messagecomplete(http_parser *Parser, HTTPRequest *Request)
{
    Request->Parsed = true;
    return 0;
}

// Callbacks on incoming data.
void IHTTPServer::onStreamupdated(std::vector<uint8_t> &Incomingstream)
{
    if (Parsedrequest.Parsed)
    {
        Parsedrequest.Body.clear();
        Parsedrequest.Headers.clear();
        Parsedrequest.Method.clear();
        Parsedrequest.URL.clear();
        Parsedrequest.Parsed = false;
    }

    // Parse the incoming data.
    size_t Read = http_parser_execute(&Parser, &Parsersettings, (const char *)Incomingstream.data(), Incomingstream.size());
    Incomingstream.erase(Incomingstream.begin(), Incomingstream.begin() + Read);

    if (Parsedrequest.Parsed)
    {
        Streamguard.unlock();
        switch (FNV1a_Runtime_32(Parsedrequest.Method.c_str(), Parsedrequest.Method.size()))
        {
            case FNV1a_Compiletime_32("GET"): onGET(Parsedrequest); break;
            case FNV1a_Compiletime_32("PUT"): onPUT(Parsedrequest); break;
            case FNV1a_Compiletime_32("POST"): onPOST(Parsedrequest); break;
            case FNV1a_Compiletime_32("COPY"): onCOPY(Parsedrequest); break;
            case FNV1a_Compiletime_32("DELETE"): onDELETE(Parsedrequest); break;
        }
        Streamguard.lock();
    }
}
void IHTTPSServer::onStreamdecrypted(std::string &Incomingstream)
{
    if (Parsedrequest.Parsed)
    {
        Parsedrequest.Body.clear();
        Parsedrequest.Headers.clear();
        Parsedrequest.Method.clear();
        Parsedrequest.URL.clear();
        Parsedrequest.Parsed = false;
    }

    // Parse the incoming data.
    size_t Read = http_parser_execute(&Parser, &Parsersettings, (const char *)Incomingstream.data(), Incomingstream.size());
    Incomingstream.erase(Incomingstream.begin(), Incomingstream.begin() + Read);

    if (Parsedrequest.Parsed)
    {
        switch (FNV1a_Runtime_32(Parsedrequest.Method.c_str(), Parsedrequest.Method.size()))
        {
            case FNV1a_Compiletime_32("GET"): onGET(Parsedrequest); break;
            case FNV1a_Compiletime_32("PUT"): onPUT(Parsedrequest); break;
            case FNV1a_Compiletime_32("POST"): onPOST(Parsedrequest); break;
            case FNV1a_Compiletime_32("COPY"): onCOPY(Parsedrequest); break;
            case FNV1a_Compiletime_32("DELETE"): onDELETE(Parsedrequest); break;
        }
    }
}

// Construct the server from a hostname.
IHTTPServer::IHTTPServer() : ITCPServer() 
{
    http_parser_init(&Parser, HTTP_BOTH);
    http_parser_settings_init(&Parsersettings);
    Parsedrequest.Parsed = false;
    Parser.data = &Parsedrequest;

    Parsersettings.on_message_begin = [](http_parser *parser) -> int
    {
        return Parse_Messagebegin(parser, (HTTPRequest *)parser->data);
    };
    Parsersettings.on_url = [](http_parser* parser, const char* at, size_t len) -> int
    {
        return Parse_URL(parser, (HTTPRequest *)parser->data, at, len);
    };
    Parsersettings.on_header_field = [](http_parser* parser, const char* at, size_t len) -> int
    {
        return Parse_Headerfield(parser, (HTTPRequest *)parser->data, at, len);
    };
    Parsersettings.on_header_value = [](http_parser* parser, const char* at, size_t len) -> int
    {
        return Parse_Headervalue(parser, (HTTPRequest *)parser->data, at, len);
    };
    Parsersettings.on_headers_complete = [](http_parser* parser) -> int
    {
        return Parse_Headerscomplete(parser, (HTTPRequest *)parser->data);
    };
    Parsersettings.on_body = [](http_parser* parser, const char* at, size_t len) -> int
    {
        return Parse_Body(parser, (HTTPRequest *)parser->data, at, len);
    };
    Parsersettings.on_message_complete = [](http_parser* parser) -> int
    {
        return Parse_Messagecomplete(parser, (HTTPRequest *)parser->data);
    };
};
IHTTPServer::IHTTPServer(const char *Hostname) : ITCPServer(Hostname) 
{
    http_parser_init(&Parser, HTTP_BOTH);
    http_parser_settings_init(&Parsersettings);
    Parsedrequest.Parsed = false;
    Parser.data = &Parsedrequest;

    Parsersettings.on_message_begin = [](http_parser *parser) -> int
    {
        return Parse_Messagebegin(parser, (HTTPRequest *)parser->data);
    };
    Parsersettings.on_url = [](http_parser* parser, const char* at, size_t len) -> int
    {
        return Parse_URL(parser, (HTTPRequest *)parser->data, at, len);
    };
    Parsersettings.on_header_field = [](http_parser* parser, const char* at, size_t len) -> int
    {
        return Parse_Headerfield(parser, (HTTPRequest *)parser->data, at, len);
    };
    Parsersettings.on_header_value = [](http_parser* parser, const char* at, size_t len) -> int
    {
        return Parse_Headervalue(parser, (HTTPRequest *)parser->data, at, len);
    };
    Parsersettings.on_headers_complete = [](http_parser* parser) -> int
    {
        return Parse_Headerscomplete(parser, (HTTPRequest *)parser->data);
    };
    Parsersettings.on_body = [](http_parser* parser, const char* at, size_t len) -> int
    {
        return Parse_Body(parser, (HTTPRequest *)parser->data, at, len);
    };
    Parsersettings.on_message_complete = [](http_parser* parser) -> int
    {
        return Parse_Messagecomplete(parser, (HTTPRequest *)parser->data);
    };
};
IHTTPSServer::IHTTPSServer() : ITLSServer() 
{
    http_parser_init(&Parser, HTTP_BOTH);
    http_parser_settings_init(&Parsersettings);
    Parsedrequest.Parsed = false;
    Parser.data = &Parsedrequest;

    Parsersettings.on_message_begin = [](http_parser *parser) -> int
    {
        return Parse_Messagebegin(parser, (HTTPRequest *)parser->data);
    };
    Parsersettings.on_url = [](http_parser* parser, const char* at, size_t len) -> int
    {
        return Parse_URL(parser, (HTTPRequest *)parser->data, at, len);
    };
    Parsersettings.on_header_field = [](http_parser* parser, const char* at, size_t len) -> int
    {
        return Parse_Headerfield(parser, (HTTPRequest *)parser->data, at, len);
    };
    Parsersettings.on_header_value = [](http_parser* parser, const char* at, size_t len) -> int
    {
        return Parse_Headervalue(parser, (HTTPRequest *)parser->data, at, len);
    };
    Parsersettings.on_headers_complete = [](http_parser* parser) -> int
    {
        return Parse_Headerscomplete(parser, (HTTPRequest *)parser->data);
    };
    Parsersettings.on_body = [](http_parser* parser, const char* at, size_t len) -> int
    {
        return Parse_Body(parser, (HTTPRequest *)parser->data, at, len);
    };
    Parsersettings.on_message_complete = [](http_parser* parser) -> int
    {
        return Parse_Messagecomplete(parser, (HTTPRequest *)parser->data);
    };
};
IHTTPSServer::IHTTPSServer(const char *Hostname) : ITLSServer(Hostname) 
{
    http_parser_init(&Parser, HTTP_BOTH);
    http_parser_settings_init(&Parsersettings);
    Parsedrequest.Parsed = false;
    Parser.data = &Parsedrequest;

    Parsersettings.on_message_begin = [](http_parser *parser) -> int
    {
        return Parse_Messagebegin(parser, (HTTPRequest *)parser->data);
    };
    Parsersettings.on_url = [](http_parser* parser, const char* at, size_t len) -> int
    {
        return Parse_URL(parser, (HTTPRequest *)parser->data, at, len);
    };
    Parsersettings.on_header_field = [](http_parser* parser, const char* at, size_t len) -> int
    {
        return Parse_Headerfield(parser, (HTTPRequest *)parser->data, at, len);
    };
    Parsersettings.on_header_value = [](http_parser* parser, const char* at, size_t len) -> int
    {
        return Parse_Headervalue(parser, (HTTPRequest *)parser->data, at, len);
    };
    Parsersettings.on_headers_complete = [](http_parser* parser) -> int
    {
        return Parse_Headerscomplete(parser, (HTTPRequest *)parser->data);
    };
    Parsersettings.on_body = [](http_parser* parser, const char* at, size_t len) -> int
    {
        return Parse_Body(parser, (HTTPRequest *)parser->data, at, len);
    };
    Parsersettings.on_message_complete = [](http_parser* parser) -> int
    {
        return Parse_Messagecomplete(parser, (HTTPRequest *)parser->data);
    };
};
IHTTPSServer::IHTTPSServer(const char *Hostname, const char *Certificate, const char *Key) : ITLSServer(Hostname, Certificate, Key) 
{
    http_parser_init(&Parser, HTTP_BOTH);
    http_parser_settings_init(&Parsersettings);
    Parsedrequest.Parsed = false;
    Parser.data = &Parsedrequest;

    Parsersettings.on_message_begin = [](http_parser *parser) -> int
    {
        return Parse_Messagebegin(parser, (HTTPRequest *)parser->data);
    };
    Parsersettings.on_url = [](http_parser* parser, const char* at, size_t len) -> int
    {
        return Parse_URL(parser, (HTTPRequest *)parser->data, at, len);
    };
    Parsersettings.on_header_field = [](http_parser* parser, const char* at, size_t len) -> int
    {
        return Parse_Headerfield(parser, (HTTPRequest *)parser->data, at, len);
    };
    Parsersettings.on_header_value = [](http_parser* parser, const char* at, size_t len) -> int
    {
        return Parse_Headervalue(parser, (HTTPRequest *)parser->data, at, len);
    };
    Parsersettings.on_headers_complete = [](http_parser* parser) -> int
    {
        return Parse_Headerscomplete(parser, (HTTPRequest *)parser->data);
    };
    Parsersettings.on_body = [](http_parser* parser, const char* at, size_t len) -> int
    {
        return Parse_Body(parser, (HTTPRequest *)parser->data, at, len);
    };
    Parsersettings.on_message_complete = [](http_parser* parser) -> int
    {
        return Parse_Messagecomplete(parser, (HTTPRequest *)parser->data);
    };
};
