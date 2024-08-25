#ifndef SSL_SERVER_H
#define SSL_SERVER_H

#include <openssl/ssl.h>
#include <openssl/err.h>
#include "../server/server.h"
#include <sys/socket.h>

class SSLServer : public Server
{
public:
    explicit SSLServer(int port);
    ~SSLServer();
    void start_ssl(std::function<void(int, SSL_CTX *)> handler);
    int get_server_fd() const { return server_fd; }

private:
    SSL_CTX *create_ssl_context();
    void configure_ssl_context(SSL_CTX *ctx);

    SSL_CTX *ssl_ctx; // SSL context
protected:
    int server_fd;
};

#endif // SSL_SERVER_H