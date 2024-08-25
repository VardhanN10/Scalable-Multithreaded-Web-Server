#ifndef SERVER_H
#define SERVER_H

#include <functional>

class Server {
public:
    explicit Server(int port);
    ~Server();
    void start(std::function<void(int)> handler);

private:
    int create_server_socket(int port);
    int server_fd;
};

#endif 
