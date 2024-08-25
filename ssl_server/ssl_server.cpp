#include "ssl_server.h"
#include <iostream>
#include <unistd.h>
#include <openssl/ssl.h>
#include <openssl/err.h>

SSLServer::SSLServer(int port) : Server(port), ssl_ctx(create_ssl_context())
{
    configure_ssl_context(ssl_ctx);
}

SSLServer::~SSLServer()
{
    SSL_CTX_free(ssl_ctx);
}

SSL_CTX *SSLServer::create_ssl_context()
{
    const SSL_METHOD *method = TLS_server_method();
    SSL_CTX *ctx = SSL_CTX_new(method);
    if (!ctx)
    {
        ERR_print_errors_fp(stderr);
        exit(EXIT_FAILURE);
    }
    return ctx;
}

void SSLServer::configure_ssl_context(SSL_CTX *ctx)
{
    if (SSL_CTX_use_certificate_file(ctx, "../cert/server.crt", SSL_FILETYPE_PEM) <= 0 ||
        SSL_CTX_use_PrivateKey_file(ctx, "../cert/server.key", SSL_FILETYPE_PEM) <= 0)
    {
        ERR_print_errors_fp(stderr);
        exit(EXIT_FAILURE);
    }
}

void SSLServer::start_ssl(std::function<void(int, SSL_CTX *)> handler)
{
    while (true)
    {
        int client_socket = accept(server_fd, nullptr, nullptr);
        if (client_socket < 0)
        {
            perror("accept failed");
            continue;
        }
        handler(client_socket, ssl_ctx);
    }
}
