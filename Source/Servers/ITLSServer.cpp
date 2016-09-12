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
	SSL_write(GetServerinfo()->State[Socket], Databuffer, Datalength);
}
void ITLSServer::onStreamupdated(const size_t Socket, std::vector<uint8_t> &Incomingstream)
{
	int Readcount;
    int Writecount;

	// Insert the data into the SSL buffer.
    Writecount = BIO_write(GetServerinfo()->Read_BIO[Socket], Incomingstream.data(), Incomingstream.size());

    if (!SSL_is_init_finished(GetServerinfo()->State[Socket]))
    {
        SSL_do_handshake(GetServerinfo()->State[Socket]);
    }
	else
    {
        auto Buffer = std::make_unique<char[]>(4096);
        Readcount = SSL_read(GetServerinfo()->State[Socket], Buffer.get(), 4096);

        // Check errors.
        if (Readcount == 0)
        {
            size_t Error = SSL_get_error(GetServerinfo()->State[Socket], 0);
            if (Error == SSL_ERROR_ZERO_RETURN)
            {
                // Remake the SSL state.
                {
                    SSL_free(GetServerinfo()->State[Socket]);

                    GetServerinfo()->Write_BIO[Socket] = BIO_new(BIO_s_mem());
                    GetServerinfo()->Read_BIO[Socket] = BIO_new(BIO_s_mem());
                    BIO_set_nbio(GetServerinfo()->Write_BIO[Socket], 1);
                    BIO_set_nbio(GetServerinfo()->Read_BIO[Socket], 1);

                    GetServerinfo()->State[Socket] = SSL_new(GetServerinfo()->Context[Socket]);
                    if(!GetServerinfo()->State[Socket]) DebugPrint("OpenSSL error: Failed to create the SSL state.");

                    SSL_set_bio(GetServerinfo()->State[Socket], GetServerinfo()->Read_BIO[Socket], GetServerinfo()->Write_BIO[Socket]);        
                    SSL_set_verify(GetServerinfo()->State[Socket], SSL_VERIFY_NONE, NULL);
                    SSL_set_accept_state(GetServerinfo()->State[Socket]);
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

// TLS-state management.
void ITLSServer::onConnect(const size_t Socket, const uint16_t Port)
{
	size_t Resultcode;
	ITCPServer::onConnect(Socket, Port);

	// Initialize the context.
    {
        GetServerinfo()->Context[Socket] = SSL_CTX_new(SSLv23_server_method());
        SSL_CTX_set_verify(GetServerinfo()->Context[Socket], SSL_VERIFY_NONE, NULL);

        SSL_CTX_set_options(GetServerinfo()->Context[Socket], SSL_OP_SINGLE_DH_USE);
        SSL_CTX_set_ecdh_auto(GetServerinfo()->Context[Socket], 1);

        uint8_t ssl_context_id[16]{ 2, 3, 4, 5, 6 };
        SSL_CTX_set_session_id_context(GetServerinfo()->Context[Socket],
            (const unsigned char *)&ssl_context_id,
            sizeof(ssl_context_id));
    }

    // Load the certificate and key for this server.
    {
        Resultcode = SSL_CTX_use_certificate_file(GetServerinfo()->Context[Socket], GetServerinfo()->SSLCert, SSL_FILETYPE_PEM);
        if (Resultcode != 1) DebugPrint(va("OpenSSL error: %s", ERR_error_string(Resultcode, NULL)));

        Resultcode = SSL_CTX_use_PrivateKey_file(GetServerinfo()->Context[Socket], GetServerinfo()->SSLKey, SSL_FILETYPE_PEM);
        if (Resultcode != 1) DebugPrint(va("OpenSSL error: %s", ERR_error_string(Resultcode, NULL)));

        Resultcode = SSL_CTX_check_private_key(GetServerinfo()->Context[Socket]);
        if (Resultcode != 1) DebugPrint(va("OpenSSL error: %s", ERR_error_string(Resultcode, NULL)));
    }

    // Create the BIO buffers.
    {
        GetServerinfo()->Write_BIO[Socket] = BIO_new(BIO_s_mem());
        GetServerinfo()->Read_BIO[Socket] = BIO_new(BIO_s_mem());
        BIO_set_nbio(GetServerinfo()->Write_BIO[Socket], 1);
        BIO_set_nbio(GetServerinfo()->Read_BIO[Socket], 1);
    }

    // Initialize the SSL state.
    {
        GetServerinfo()->State[Socket] = SSL_new(GetServerinfo()->Context[Socket]);
        if(!GetServerinfo()->State[Socket]) DebugPrint("OpenSSL error: Failed to create the SSL state.");

        SSL_set_bio(GetServerinfo()->State[Socket], GetServerinfo()->Read_BIO[Socket], GetServerinfo()->Write_BIO[Socket]);        
        SSL_set_verify(GetServerinfo()->State[Socket], SSL_VERIFY_NONE, NULL);
        SSL_set_accept_state(GetServerinfo()->State[Socket]);
    }
}
void ITLSServer::onDisconnect(const size_t Socket)
{
	ITCPServer::onDisconnect(Socket);

	GetServerinfo()->Write_BIO.erase(Socket);
	GetServerinfo()->Read_BIO.erase(Socket);
	GetServerinfo()->Context.erase(Socket);
	GetServerinfo()->State.erase(Socket);
}
void ITLSServer::Syncbuffers(const size_t Socket)
{
	if (!Streamguard[Socket].try_lock())
        Streamguard[Socket].unlock();

    Streamguard[Socket].lock();
    {
        auto Buffer = std::make_unique<char[]>(4096);
        int Readcount = BIO_read(GetServerinfo()->Write_BIO[Socket], Buffer.get(), 4096);
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

	GetServerinfo()->SSLCert = Certificate;
	GetServerinfo()->SSLKey = Key;
}
