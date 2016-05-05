/*
    Initial author: (https://github.com/)Convery for Ayria.se
    License: LGPL 3.0
    Started: 2016-5-5
    Notes:
        Buffered server with a callback on incoming data.
*/

#include <Configuration\All.h>
#include <Servers\ITLSServer.h>

// Single-socket operations.
void ITLSServer::onConnect(const size_t Socket, const uint16_t Port)
{
    SSL_set_accept_state(GetServerinfo()->State);
    ITCPServer::onConnect(Socket, Port);
}

// Callback and methods to insert data.
void ITLSServer::Senddata(std::string &Databuffer)
{
    Senddata(Databuffer.data(), Databuffer.size());
}
void ITLSServer::Senddata(const void *Databuffer, const size_t Datalength)
{
    size_t Resultcode;

    Resultcode = SSL_write(GetServerinfo()->State, Databuffer, Datalength);
    if (Resultcode == 0) DebugPrint("OpenSSL: Could not write to BIO.");
    
    // Send pending data.
    size_t Pending;
    Pending = BIO_pending(GetServerinfo()->APP_BIO);
    if (Pending)
    {
        auto Buffer = std::make_unique<char[]>(Pending);
        Resultcode = BIO_read(GetServerinfo()->APP_BIO, Buffer.get(), Pending);
        ITCPServer::Senddata(Buffer.get(), Resultcode);
    }
}
void ITLSServer::onStreamupdated(std::vector<uint8_t> &Incomingstream)
{
    size_t Resultcode;
    
    // Feed the incoming data to SSL.
    Resultcode = BIO_write(GetServerinfo()->APP_BIO, Incomingstream.data(), Incomingstream.size());
    if (Resultcode == 0) DebugPrint("OpenSSL: Could not write to BIO.");

    // If the handshake is not complete, send it again.
    if (!SSL_is_init_finished(GetServerinfo()->State))
    {
        Resultcode = SSL_do_handshake(GetServerinfo()->State);
        if (Resultcode != 1) DebugPrint(va("OpenSSL error: %s", ERR_error_string(Resultcode, NULL)));

        // Send pending data.
        size_t Pending;
        Pending = BIO_pending(GetServerinfo()->APP_BIO);
        if (Pending)
        {
            auto Buffer = std::make_unique<char[]>(Pending);
            Resultcode = BIO_read(GetServerinfo()->APP_BIO, Buffer.get(), Pending);
            ITCPServer::Senddata(Buffer.get(), Resultcode);
        }
    }
    else
    {
        auto Buffer = std::make_unique<char[]>(Incomingstream.size());
        Resultcode = SSL_read(GetServerinfo()->State, Buffer.get(), Incomingstream.size());
        if (Resultcode == 0) DebugPrint("OpenSSL: Could not read on BIO.");

        std::string Packet(Buffer.get(), Resultcode);
        onStreamdecrypted(Packet);
    }
}

// Construct the server from a hostname.
ITLSServer::ITLSServer() : ITCPServer() {};
ITLSServer::ITLSServer(const char *Hostname) : ITCPServer(Hostname) {};
ITLSServer::ITLSServer(const char *Hostname, const char *Certificate, const char *Key) : ITCPServer()
{
    size_t Resultcode;

    // Load the certificate and key for this server.
    {
        Resultcode = SSL_CTX_use_certificate_file(GetServerinfo()->Context, Certificate, SSL_FILETYPE_PEM);
        if (Resultcode != 1) DebugPrint(va("OpenSSL error: %s", ERR_error_string(Resultcode, NULL)));

        Resultcode = SSL_CTX_use_PrivateKey_file(GetServerinfo()->Context, Key, SSL_FILETYPE_PEM);
        if (Resultcode != 1) DebugPrint(va("OpenSSL error: %s", ERR_error_string(Resultcode, NULL)));

        Resultcode = SSL_CTX_check_private_key(GetServerinfo()->Context);
        if (Resultcode != 1) DebugPrint(va("OpenSSL error: %s", ERR_error_string(Resultcode, NULL)));
    }

    // Initialize the BIO buffers.
    {
        GetServerinfo()->State = SSL_new(GetServerinfo()->Context);
        if(!GetServerinfo()->State) DebugPrint("OpenSSL error: Failed to create the SSL state.");
        
        Resultcode = BIO_new_bio_pair(&GetServerinfo()->SSL_BIO, 0, &GetServerinfo()->APP_BIO, 0);
        if (Resultcode != 1) DebugPrint(va("OpenSSL error: %s", ERR_error_string(Resultcode, NULL)));

        SSL_set_bio(GetServerinfo()->State, GetServerinfo()->SSL_BIO, GetServerinfo()->APP_BIO);
    }
};
