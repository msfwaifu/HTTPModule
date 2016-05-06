/*
    Initial author: (https://github.com/)Convery for Ayria.se
    License: LGPL 3.0
    Started: 2016-5-5
    Notes:
        Buffered server with a callback on incoming data.
*/

#include <Configuration\All.h>
#include <Servers\ITLSServer.h>

#define SSL_WHERE_INFO(ssl, w, flag, msg) {                \
    if(w & flag) {                                         \
      printf("%20.20s", msg);                              \
      printf(" - %30.30s ", SSL_state_string_long(ssl));   \
      printf(" - %5.10s ", SSL_state_string(ssl));         \
      printf("\n");                                        \
    }                                                      \
  } 

// Verify the certificates.
int ssl_verify_peer(int ok, X509_STORE_CTX* ctx) 
{
    return 1;
}
void ssl_info_callback(const SSL* ssl, int where, int ret) 
{

    if(ret == 0) 
    {
        printf("-- ssl_info_callback: error occured.\n");
        return;
    }

    SSL_WHERE_INFO(ssl, where, SSL_CB_LOOP, "LOOP");
    SSL_WHERE_INFO(ssl, where, SSL_CB_HANDSHAKE_START, "HANDSHAKE START");
    SSL_WHERE_INFO(ssl, where, SSL_CB_HANDSHAKE_DONE, "HANDSHAKE DONE");
}
DH *get_dh512()
{
    static unsigned char dh512_p[]={
        0x9B,0x9A,0x2B,0x34,0xDA,0x9A,0x55,0x53,0x47,0xDB,0xCF,0xB4,
        0x26,0xAA,0x4D,0xFD,0x01,0x91,0x4A,0x19,0xE0,0x90,0xFA,0x6B,
        0x99,0xD6,0xE2,0x78,0xF3,0x31,0xD3,0x93,0x9B,0x7B,0xE1,0x65,
        0x57,0xFD,0x4D,0x2C,0x4E,0x17,0xE1,0xAC,0x30,0xB7,0xD0,0xA6,
        0x80,0x13,0xEE,0x37,0xD1,0x83,0xCD,0x5F,0x88,0x38,0x79,0x9C,
        0xFD,0xCE,0x85,0xED,
    };
    static unsigned char dh512_g[]={
        0x8B,0x17,0x22,0x46,0x30,0xAD,0xE5,0x06,0x42,0x60,0x15,0x79,
        0xA2,0x2F,0xD9,0xAA,0x7B,0xD7,0x8A,0x6F,0x39,0xEB,0x13,0x38,
        0x54,0xA6,0xBE,0xAD,0xC6,0x6A,0x17,0x95,0xBE,0x8B,0x29,0xE0,
        0x60,0x14,0x72,0xC9,0x5C,0x84,0x5D,0xD6,0x8B,0x57,0xD9,0x9D,
        0x08,0x60,0x73,0x78,0x3F,0xDD,0x26,0x2C,0x40,0x63,0xCF,0xE0,
        0xDC,0x58,0x7A,0x9C,
    };
    DH *dh;

    if ((dh=DH_new()) == NULL) return(NULL);
    dh->p=BN_bin2bn(dh512_p,sizeof(dh512_p),NULL);
    dh->g=BN_bin2bn(dh512_g,sizeof(dh512_g),NULL);
    if ((dh->p == NULL) || (dh->g == NULL))
    { DH_free(dh); return(NULL); }
    dh->length = 160;
    return(dh);
}

void Returndata(ITLSServer *Server, std::string *Data)
{
    Server->Streamguard.lock();
    {
        const char *Bytepointer = (const char *)Data->data();
        Server->Outgoingstream.insert(Server->Outgoingstream.end(), Bytepointer, Bytepointer + Data->size());
    }
    Server->Streamguard.unlock();

    delete Data;
}

// Single-socket operations.
void ITLSServer::onConnect(const size_t Socket, const uint16_t Port)
{
    ITCPServer::onConnect(Socket, Port);
    SSL_set_accept_state(GetServerinfo()->State);
    size_t Resultcode = SSL_accept(GetServerinfo()->State);
    if (Resultcode != 1)
    {
    }
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
        if (Resultcode != 1)
        {
        }

        // Send pending data.
        size_t Pending;
        Pending = BIO_pending(GetServerinfo()->APP_BIO);
        if (Pending)
        {
            auto Buffer = std::make_unique<char[]>(Pending);
            Resultcode = BIO_read(GetServerinfo()->APP_BIO, Buffer.get(), Pending);
            std::thread(Returndata, this, new std::string(Buffer.get(), Resultcode)).detach();
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
ITLSServer::ITLSServer(const char *Hostname, const char *Certificate, const char *Key) : ITCPServer(Hostname)
{
    size_t Resultcode;

    // Initialize OpenSSL.
    {
        SSL_library_init();
        SSL_load_error_strings();
        ERR_load_BIO_strings();
        OpenSSL_add_all_algorithms();
        ERR_load_crypto_strings();
    }

    // Initialize the context.
    {
        GetServerinfo()->Context = SSL_CTX_new(TLSv1_1_method());

        size_t Options = SSL_OP_NO_SSLv2 | SSL_OP_NO_SSLv3;
        SSL_CTX_set_options(GetServerinfo()->Context, Options);

        SSL_CTX_set_mode(GetServerinfo()->Context, SSL_MODE_AUTO_RETRY |
            SSL_MODE_ACCEPT_MOVING_WRITE_BUFFER        |
            SSL_MODE_ENABLE_PARTIAL_WRITE              |
            SSL_MODE_RELEASE_BUFFERS
        );

        Resultcode = SSL_CTX_set_cipher_list(GetServerinfo()->Context, "ALL:!ADH:!LOW:!EXP:!MD5:@STRENGTH");
        if (Resultcode != 1) DebugPrint(va("OpenSSL error: %s", ERR_error_string(Resultcode, NULL)));

        Resultcode = SSL_CTX_set_tmp_dh(GetServerinfo()->Context, get_dh512());
        if (Resultcode != 1) DebugPrint(va("OpenSSL error: %s", ERR_error_string(Resultcode, NULL)));

        Resultcode = SSL_CTX_set_tlsext_use_srtp(GetServerinfo()->Context, "SRTP_AES128_CM_SHA1_80");
        if (Resultcode != 0) DebugPrint(va("OpenSSL error: %s", ERR_error_string(Resultcode, NULL)));

        SSL_CTX_set_verify(GetServerinfo()->Context, SSL_VERIFY_PEER, ssl_verify_peer);
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

    // Initialize the BIO buffers.
    {
        GetServerinfo()->State = SSL_new(GetServerinfo()->Context);
        if(!GetServerinfo()->State) DebugPrint("OpenSSL error: Failed to create the SSL state.");
        
        Resultcode = BIO_new_bio_pair(&GetServerinfo()->SSL_BIO, 0, &GetServerinfo()->APP_BIO, 0);
        if (Resultcode != 1) DebugPrint(va("OpenSSL error: %s", ERR_error_string(Resultcode, NULL)));

        SSL_set_bio(GetServerinfo()->State, GetServerinfo()->SSL_BIO, GetServerinfo()->SSL_BIO);

        SSL_set_info_callback(GetServerinfo()->State, ssl_info_callback);
    }
};
