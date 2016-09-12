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
void ITLSServer::Senddata(const size_t Socket, std::string &Databuffer)
{
    Senddata(Socket, Databuffer.data(), Databuffer.size());
}
void ITLSServer::Senddata(const size_t Socket, const void *Databuffer, const size_t Datalength)
{
    SSL_write(State[Socket], Databuffer, Datalength);
}
void ITLSServer::onStreamupdated(const size_t Socket, std::vector<uint8_t> &Incomingstream)
{
    int Readcount;
    int Writecount;

    // Insert the data into the SSL buffer.
    Writecount = BIO_write(Read_BIO[Socket], Incomingstream.data(), Incomingstream.size());

    if (!SSL_is_init_finished(State[Socket]))
    {
        SSL_do_handshake(State[Socket]);
    }
    else
    {
        auto Buffer = std::make_unique<char[]>(4096);
        Readcount = SSL_read(State[Socket], Buffer.get(), 4096);

        // Check errors.
        if (Readcount == 0)
        {
            size_t Error = SSL_get_error(State[Socket], 0);
            if (Error == SSL_ERROR_ZERO_RETURN)
            {
                // Remake the SSL state.
                {
                    SSL_free(State[Socket]);

                    Write_BIO[Socket] = BIO_new(BIO_s_mem());
                    Read_BIO[Socket] = BIO_new(BIO_s_mem());
                    BIO_set_nbio(Write_BIO[Socket], 1);
                    BIO_set_nbio(Read_BIO[Socket], 1);

                    State[Socket] = SSL_new(Context[Socket]);
                    if(!State[Socket]) DebugPrint("OpenSSL error: Failed to create the SSL state.");

                    SSL_set_bio(State[Socket], Read_BIO[Socket], Write_BIO[Socket]);        
                    SSL_set_verify(State[Socket], SSL_VERIFY_NONE, NULL);
                    SSL_set_accept_state(State[Socket]);
                }

                Incomingstream.clear();
                return;
            }                
        }

        if (Readcount > 0)
        {
            std::string Request(Buffer.get(), Readcount);
            onStreamdecrypted(Socket, Request);
        }
    }

    Incomingstream.erase(Incomingstream.begin(), Incomingstream.begin() + Writecount);
    Syncbuffers(Socket);
}

// Patch the hyperquest binary to remove SSL checks.
int mycert_verify_callback(int ok, X509_STORE_CTX *ctx)
{
    PrintFunction();
    return ok;
}
long mySSL_get_verify_result(const SSL *ssl)
{
    PrintFunction();
    return X509_V_OK;
}
static void InstallTLSPatch()
{
    static bool Patched = false;
    if (Patched) return;
    Patched = true;

    while (true)
    {
        auto Address = FindpatternText("\x8B\x44\x24\x04\x8B\x4C\x24\x08\x8B\x54\x24\x0C\x89\x88\xC0\x00\x00\x00\x89\x90\xE8\x00\x00\x00\xC3", "\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01");
        if (Address)
        {
            Insertjump(Address, (uint64_t)mycert_verify_callback);
        }

        Address = FindpatternText("\x8B\x44\x24\x04\x8B\x80\xEC\x00\x00\x00\xC3", "\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01");
        if (Address)
        {
            Insertjump(Address, (uint64_t)mySSL_get_verify_result);
            return;
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
}

// TLS-state management.
void ITLSServer::onConnect(const size_t Socket, const uint16_t Port)
{
    size_t Resultcode;
    ITCPServer::onConnect(Socket, Port);
    std::thread(InstallTLSPatch).detach();

    // Initialize the context.
    {
        Context[Socket] = SSL_CTX_new(SSLv23_server_method());
        SSL_CTX_set_verify(Context[Socket], SSL_VERIFY_NONE, NULL);

        SSL_CTX_set_options(Context[Socket], SSL_OP_SINGLE_DH_USE);
        SSL_CTX_set_ecdh_auto(Context[Socket], 1);

        uint8_t ssl_context_id[16]{ 2, 3, 4, 5, 6 };
        SSL_CTX_set_session_id_context(Context[Socket],
            (const unsigned char *)&ssl_context_id,
            sizeof(ssl_context_id));
    }

    // Load the certificate and key for this server.
    {
        Resultcode = SSL_CTX_use_certificate_file(Context[Socket], SSLCert.c_str(), SSL_FILETYPE_PEM);
        if (Resultcode != 1) DebugPrint(va("OpenSSL error: %s", ERR_error_string(Resultcode, NULL)));

        Resultcode = SSL_CTX_use_PrivateKey_file(Context[Socket], SSLKey.c_str(), SSL_FILETYPE_PEM);
        if (Resultcode != 1) DebugPrint(va("OpenSSL error: %s", ERR_error_string(Resultcode, NULL)));

        Resultcode = SSL_CTX_check_private_key(Context[Socket]);
        if (Resultcode != 1) DebugPrint(va("OpenSSL error: %s", ERR_error_string(Resultcode, NULL)));
    }

    // Create the BIO buffers.
    {
        Write_BIO[Socket] = BIO_new(BIO_s_mem());
        Read_BIO[Socket] = BIO_new(BIO_s_mem());
        BIO_set_nbio(Write_BIO[Socket], 1);
        BIO_set_nbio(Read_BIO[Socket], 1);
    }

    // Initialize the SSL state.
    {
        State[Socket] = SSL_new(Context[Socket]);
        if(!State[Socket]) DebugPrint("OpenSSL error: Failed to create the SSL state.");

        SSL_set_bio(State[Socket], Read_BIO[Socket], Write_BIO[Socket]);        
        SSL_set_verify(State[Socket], SSL_VERIFY_NONE, NULL);
        SSL_set_accept_state(State[Socket]);
    }
}
void ITLSServer::onDisconnect(const size_t Socket)
{
    ITCPServer::onDisconnect(Socket);

    //Write_BIO.erase(Socket);
    //Read_BIO.erase(Socket);
    //Context.erase(Socket);
    //State.erase(Socket);
}
void ITLSServer::Syncbuffers(const size_t Socket)
{
    if (!Streamguard[Socket].try_lock())
        Streamguard[Socket].unlock();

    Streamguard[Socket].lock();
    {
        auto Buffer = std::make_unique<char[]>(4096);
        int Readcount = BIO_read(Write_BIO[Socket], Buffer.get(), 4096);
        if(Readcount > 0)
            Outgoingstream[Socket].insert(Outgoingstream[Socket].end(), Buffer.get(), Buffer.get() + Readcount);
    }
    
    // Unlocked when the function returns.
}

// Construct the server from a hostname.
ITLSServer::ITLSServer() : ITCPServer() {};
ITLSServer::ITLSServer(const char *Hostname) : ITCPServer(Hostname) {};
ITLSServer::ITLSServer(const char *Hostname, const char *Certificate, const char *Key) : ITCPServer(Hostname) 
{
    // Initialize OpenSSL.
    {
        SSL_library_init();
        SSL_load_error_strings();
        ERR_load_BIO_strings();
        OpenSSL_add_all_algorithms();
    }

    SSLCert = Certificate;
    SSLKey = Key;
}
