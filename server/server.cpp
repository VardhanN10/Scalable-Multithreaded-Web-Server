#include "server.h"
#include <stdexcept>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <cstring>
#include "../Exception/exception.h"

Server::Server(int port) : server_fd(create_server_socket(port)) {}

Server::~Server() {
    close(server_fd);
}

int Server::create_server_socket(int port) {
    int fd = socket(AF_INET, SOCK_STREAM, 0);
    if (fd < 0) {
        throw ServerException("Socket creation failed");
    }

    sockaddr_in address{};
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(port);

    if (bind(fd, (struct sockaddr *)&address, sizeof(address)) < 0) {
        close(fd);
        throw ServerException("Bind failed");
    }

    if (listen(fd, 10) < 0) {
        close(fd);
        throw ServerException("Listen failed");
    }

    return fd;
}

void Server::start(std::function<void(int)> handler) {
    while (true) {
        int client_socket = accept(server_fd, nullptr, nullptr);
        if (client_socket < 0) {
            continue;
        }
        handler(client_socket);
    }
}
