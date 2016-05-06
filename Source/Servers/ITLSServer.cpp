/*
    Initial author: (https://github.com/)Convery for Ayria.se
    License: LGPL 3.0
    Started: 2016-5-6
    Notes:
        Buffered server with a callback on incoming data.
*/

#include <Configuration\All.h>
#include <Servers\ITLSServer.h>

// Callback and methods to insert data.
void ITLSServer::Senddata(std::string &Databuffer) 
{
    Senddata(Databuffer.data(), Databuffer.size());
};
void ITLSServer::Senddata(const void *Databuffer, const size_t Datalength) 
{
    SSL_write(GetServerinfo()->State, Databuffer, Datalength);
};
void ITLSServer::onStreamupdated(std::vector<uint8_t> &Incomingstream) 
{
    size_t Readcount;

    // Insert the data into the SSL buffer.
    BIO_write(GetServerinfo()->Read_BIO, Incomingstream.data(), Incomingstream.size());
    Incomingstream.clear();

    if (!SSL_is_init_finished(GetServerinfo()->State))
    {
        SSL_do_handshake(GetServerinfo()->State);
    }
    else
    {
        auto Buffer = std::make_unique<char[]>(1024);
        Readcount = SSL_read(GetServerinfo()->State, Buffer.get(), 1024);

        if (Readcount > 0)
        {
            std::string Request(Buffer.get(), Readcount);
            onStreamdecrypted(Request);
        }
    }

    Syncbuffers();
};
void ITLSServer::Syncbuffers()
{
    if (!Streamguard.try_lock())
        Streamguard.unlock();

    Streamguard.lock();
    {
        auto Buffer = std::make_unique<char[]>(4096);
        int Readcount = BIO_read(GetServerinfo()->Write_BIO, Buffer.get(), 4096);
        if(Readcount > 0)
            Outgoingstream.insert(Outgoingstream.end(), Buffer.get(), Buffer.get() + Readcount);
    }
    
    // Unlocked when the function returns.
}

// Construct the server from a hostname.
ITLSServer::ITLSServer() : ITCPServer() {};
ITLSServer::ITLSServer(const char *Hostname) : ITCPServer(Hostname) {};
ITLSServer::ITLSServer(const char *Hostname, const char *Certificate, const char *Key) : ITCPServer(Hostname) 
{
    size_t Resultcode;

    // Initialize OpenSSL.
    {
        SSL_library_init();
        SSL_load_error_strings();
        ERR_load_BIO_strings();
        OpenSSL_add_all_algorithms();
    }

    // Initialize the context.
    {
        GetServerinfo()->Context = SSL_CTX_new(TLSv1_server_method());
        SSL_CTX_set_verify(GetServerinfo()->Context, SSL_VERIFY_NONE, NULL);
    }

    // Load the certificate and key for this server.
    {
        Resultcode = SSL_CTX_use_certificate_file(GetServerinfo()->Context, Certificate, SSL_FILETYPE_PEM);
        if (Resultcode != 1) DebugPrint(va("OpenSSL error: %s", ERR_error_string(Resultcode, NULL)));

        Resultcode = SSL_CTX_use_PrivateKey_file(GetServerinfo()->Context, Key, SSL_FILETYPE_PEM);
        if (Resultcode != 1) DebugPrint(va("OpenSSL error: %s", ERR_error_string(Resultcode, NULL)));

        Resultcode = SSL_CTX_check_private_key(GetServerinfo()->Context);
        if (Resultcode != 1) DebugPrint(va("OpenSSL error: %s", ERR_error_string(Resultcode, NULL)));
    }

    // Create the BIO buffers.
    {
        GetServerinfo()->Write_BIO = BIO_new(BIO_s_mem());
        GetServerinfo()->Read_BIO = BIO_new(BIO_s_mem());
        BIO_set_nbio(GetServerinfo()->Write_BIO, 1);
        BIO_set_nbio(GetServerinfo()->Read_BIO, 1);
    }

    // Initialize the SSL state.
    {
        GetServerinfo()->State = SSL_new(GetServerinfo()->Context);
        if(!GetServerinfo()->State) DebugPrint("OpenSSL error: Failed to create the SSL state.");

        SSL_set_bio(GetServerinfo()->State, GetServerinfo()->Read_BIO, GetServerinfo()->Write_BIO);        
        SSL_set_verify(GetServerinfo()->State, SSL_VERIFY_NONE, NULL);
        SSL_set_accept_state(GetServerinfo()->State);
    }
}
