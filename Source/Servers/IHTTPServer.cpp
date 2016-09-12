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
void IHTTPServer::onStreamupdated(const size_t Socket, std::vector<uint8_t> &Incomingstream)
{
	// Create a parser if needed.
	if (Parser.find(Socket) == Parser.end())
	{
		http_parser_init(&Parser[Socket], HTTP_BOTH);
		http_parser_settings_init(&Parsersettings[Socket]);
		Parsedrequest[Socket].Parsed = false;
		Parser[Socket].data = &Parsedrequest[Socket];

		Parsersettings[Socket].on_message_begin = [](http_parser *parser) -> int
		{
			return Parse_Messagebegin(parser, (HTTPRequest *)parser->data);
		};
		Parsersettings[Socket].on_url = [](http_parser* parser, const char* at, size_t len) -> int
		{
			return Parse_URL(parser, (HTTPRequest *)parser->data, at, len);
		};
		Parsersettings[Socket].on_header_field = [](http_parser* parser, const char* at, size_t len) -> int
		{
			return Parse_Headerfield(parser, (HTTPRequest *)parser->data, at, len);
		};
		Parsersettings[Socket].on_header_value = [](http_parser* parser, const char* at, size_t len) -> int
		{
			return Parse_Headervalue(parser, (HTTPRequest *)parser->data, at, len);
		};
		Parsersettings[Socket].on_headers_complete = [](http_parser* parser) -> int
		{
			return Parse_Headerscomplete(parser, (HTTPRequest *)parser->data);
		};
		Parsersettings[Socket].on_body = [](http_parser* parser, const char* at, size_t len) -> int
		{
			return Parse_Body(parser, (HTTPRequest *)parser->data, at, len);
		};
		Parsersettings[Socket].on_message_complete = [](http_parser* parser) -> int
		{
			return Parse_Messagecomplete(parser, (HTTPRequest *)parser->data);
		};
	}

	// Clear any old data.
    if (Parsedrequest[Socket].Parsed)
    {
        Parsedrequest[Socket].Body.clear();
        Parsedrequest[Socket].Headers.clear();
        Parsedrequest[Socket].Method.clear();
        Parsedrequest[Socket].URL.clear();
        Parsedrequest[Socket].Parsed = false;
    }

    // Parse the incoming data.
    size_t Read = http_parser_execute(&Parser[Socket], &Parsersettings[Socket], (const char *)Incomingstream.data(), Incomingstream.size());
    Incomingstream.erase(Incomingstream.begin(), Incomingstream.begin() + Read);

    if (Parsedrequest[Socket].Parsed)
    {
        Streamguard[Socket].unlock();
        switch (FNV1a_Runtime_32(Parsedrequest[Socket].Method.c_str(), Parsedrequest[Socket].Method.size()))
        {
            case FNV1a_Compiletime_32("GET"): onGET(Socket, Parsedrequest[Socket]); break;
            case FNV1a_Compiletime_32("PUT"): onPUT(Socket, Parsedrequest[Socket]); break;
            case FNV1a_Compiletime_32("POST"): onPOST(Socket, Parsedrequest[Socket]); break;
            case FNV1a_Compiletime_32("COPY"): onCOPY(Socket, Parsedrequest[Socket]); break;
            case FNV1a_Compiletime_32("DELETE"): onDELETE(Socket, Parsedrequest[Socket]); break;
        }
        Streamguard[Socket].lock();
    }
}
void IHTTPSServer::onStreamdecrypted(const size_t Socket, std::string &Incomingstream)
{
    if (Parsedrequest[Socket].Parsed)
    {
        Parsedrequest[Socket].Body.clear();
        Parsedrequest[Socket].Headers.clear();
        Parsedrequest[Socket].Method.clear();
        Parsedrequest[Socket].URL.clear();
        Parsedrequest[Socket].Parsed = false;
    }

    // Parse the incoming data.
    size_t Read = http_parser_execute(&Parser[Socket], &Parsersettings[Socket], (const char *)Incomingstream.data(), Incomingstream.size());
    Incomingstream.erase(Incomingstream.begin(), Incomingstream.begin() + Read);

    if (Parsedrequest[Socket].Parsed)
    {
        Streamguard[Socket].unlock();
        switch (FNV1a_Runtime_32(Parsedrequest[Socket].Method.c_str(), Parsedrequest[Socket].Method.size()))
        {
            case FNV1a_Compiletime_32("GET"): onGET(Socket, Parsedrequest[Socket]); break;
            case FNV1a_Compiletime_32("PUT"): onPUT(Socket, Parsedrequest[Socket]); break;
            case FNV1a_Compiletime_32("POST"): onPOST(Socket, Parsedrequest[Socket]); break;
            case FNV1a_Compiletime_32("COPY"): onCOPY(Socket, Parsedrequest[Socket]); break;
            case FNV1a_Compiletime_32("DELETE"): onDELETE(Socket, Parsedrequest[Socket]); break;
        }
        Streamguard[Socket].lock();
    }
}

// Construct the server from a hostname.
IHTTPServer::IHTTPServer() : ITCPServer() 
{

};
IHTTPServer::IHTTPServer(const char *Hostname) : ITCPServer(Hostname) 
{
};
IHTTPSServer::IHTTPSServer() : ITLSServer() 
{
};
IHTTPSServer::IHTTPSServer(const char *Hostname) : ITLSServer(Hostname) 
{
};
IHTTPSServer::IHTTPSServer(const char *Hostname, const char *Certificate, const char *Key) : ITLSServer(Hostname, Certificate, Key) 
{
};
