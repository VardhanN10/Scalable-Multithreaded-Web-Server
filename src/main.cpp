#include <iostream>
#include <fstream>
#include <unistd.h>
#include <vector>
#include <string>
#include <memory>
#include <thread>
#include <chrono>
#include "../server/server.h"
#include "../threadpool/thread_pool.h"
#include "../ssl_server/ssl_server.h"
#include "../Exception/exception.h"
#include "../cache/cache.h"


Cache cache;

std::vector<int> read_ports_from_config(const std::string &config_file)
{
    std::vector<int> ports;
    std::ifstream file(config_file);

    if (!file)
    {
        throw ConfigException("Unable to open configuration file: " + config_file);
    }

    std::string line;
    while (std::getline(file, line))
    {
        line.erase(0, line.find_first_not_of(" \t"));
        line.erase(line.find_last_not_of(" \t") + 1);

        if (line.empty() || line[0] == '#')
        {
            continue;
        }

        try
        {
            int port = std::stoi(line);
            if (port < 1 || port > 65535)
            {
                throw ConfigException("Invalid port number: " + line);
            }
            ports.push_back(port);
        }
        catch (std::invalid_argument &)
        {
            throw ConfigException("Invalid port number: " + line);
        }
    }

    return ports;
}

void handle_client(int client_socket)
{
    char buffer[1024] = {0};
    ssize_t bytes_read = read(client_socket, buffer, sizeof(buffer));

    if (bytes_read < 0)
    {
        std::cerr << "Failed to read from client socket." << std::endl;
        close(client_socket);
        return;
    }
    else if (bytes_read == 0)
    {
        std::cerr << "Client closed the connection before sending data." << std::endl;
        close(client_socket);
        return;
    }

    std::string request(buffer, bytes_read);
    std::cout << "=== Received Request ===" << std::endl;
    std::cout << request << std::endl;
    std::cout << "=========================" << std::endl;

    std::string path = "/";
    size_t pos = request.find(" ");
    if (pos != std::string::npos)
    {
        size_t end_pos = request.find(" ", pos + 1);
        if (end_pos != std::string::npos)
        {
            path = request.substr(pos + 1, end_pos - pos - 1);
        }
    }

    std::string response;
    try
    {
        try
        {
            std::string cached_response = cache.get(path);
            response = cached_response;
            std::cout << "Cache hit for path: " << path << std::endl;
        }
        catch (const CacheException &)
        {
            std::cerr << "Cache miss for path: " << path << std::endl;
            if (path == "/")
            {
                response = "HTTP/1.1 200 OK\r\nContent-Type: text/plain\r\n\r\nWelcome To Web Server";
            }
            else
            {
                response = "HTTP/1.1 404 Not Found\r\nContent-Type: text/plain\r\n\r\n404 Not Found";
            }
            cache.set(path, response);
        }
    }
    catch (const std::exception &e)
    {
        std::cerr << "General error: " << e.what() << std::endl;
        response = "HTTP/1.1 500 Internal Server Error\r\nContent-Type: text/plain\r\n\r\nInternal Server Error";
    }

    ssize_t bytes_sent = send(client_socket, response.c_str(), response.size(), 0);
    if (bytes_sent < 0)
    {
        std::cerr << "Failed to send response to client." << std::endl;
    }
    close(client_socket);
}
int main()
{
    try
    {
        cache.clear();
        std::vector<int> ports = read_ports_from_config("config.txt");
        ThreadPool pool(20);

        std::vector<std::unique_ptr<Server>> servers;
        std::vector<std::thread> server_threads;

        for (int port : ports)
        {
            try
            {
                auto http_server = std::make_unique<Server>(port);
                servers.push_back(std::move(http_server));

                server_threads.emplace_back([server = servers.back().get(), &pool]
                                            { server->start([&](int client_socket)
                                                            { pool.enqueue_task([client_socket]
                                                                                { handle_client(client_socket); }); }); });

                std::cout << "Server started on port " << port << std::endl;
            }
            catch (const ServerException &e)
            {
                std::cerr << "Failed to start server on port " << port << ": " << e.what() << std::endl;
                return EXIT_FAILURE;
            }
        }

        std::cout << "Servers are running. Press Ctrl+C to stop." << std::endl;

        for (auto &thread : server_threads)
        {
            thread.join();
        }
    }
    catch (const std::exception &e)
    {
        std::cerr << "Error: " << e.what() << std::endl;
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
