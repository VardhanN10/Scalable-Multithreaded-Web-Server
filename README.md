# Scalable Multi-Threaded Web Server

## Project Overview

This project implements a high-performance, multi-threaded web server similar to Nginx or Apache. It showcases fundamental web server functionalities, including HTTP request handling, concurrency, and basic security. Additionally, the project features advanced caching mechanisms and load balancing to handle high traffic efficiently.

### Core Components

- **HTTP Request Handling**: A full HTTP/1.1 server capable of handling GET, POST, and other methods.
- **Concurrency**: Utilizes multithreading to manage multiple client connections simultaneously.
- **Load Balancing**: Basic load balancing support across multiple server instances.
- **Security**: Implements basic security features such as HTTPS (using OpenSSL) and input validation.

### Key Features

- **Custom Caching**: Implements content caching with support for dynamic caching strategies.
- **Thread Pooling**: Efficiently manages tasks with a thread pool to handle concurrent client requests.
- **Configurable Ports**: Easily configurable ports through a simple configuration file.

### Performance
**Benchmarking Results:** Achieved an average of 2,481 requests per second with zero failures during ApacheBench tests. The server maintained a mean response time of 20.1 milliseconds under a concurrency level of 50.


### Architecture

**Distributed Setup:** The server can run multiple instances simultaneously, with a load balancer distributing incoming traffic across these instances to ensure efficient handling of requests.

**Caching Mechanisms:** Utilizes content caching and dynamic caching strategies to manage frequently requested resources and reduce load times.

## Getting Started

### Prerequisites

Ensure you have the following installed:

- **g++**: The GNU C++ compiler
- **OpenSSL**: For SSL/TLS support
- **ApacheBench**: For benchmarking (optional)

### Installation

1. **Clone the Repository**:
    ```bash
    git clone https://github.com/yourusername/Scalable-Multithreaded-Web-Server.git
    cd Scalable-Multithreaded-Web-Server
    ```

2. **Build the Project**:
    Run the `make` command to compile the project. The binary executable will be created with the name `my_server`.

3. **Install the Binary (Optional)**:
    To install the binary system-wide, run:
    ```bash
    sudo make install
    ```

4. **Clean Up Build Files (Optional)**:
    To remove the compiled files and clean up the directory, run:
    ```bash
    make clean
    ```

### Configuration

The server ports are specified in the `config.txt` file. By default, it includes the following ports:

- 8080
- 8081
- 8082

You can modify this file to use different ports if needed.

### Running the Server

**Start the Server**
Run the compiled server executable:
```bash
./my_server
```
The server will start on the ports specified in the `config.txt` file. You will see output indicating that the server is running on the specified ports.

### Benchmarking

You can test the server performance using ApacheBench. For example:


```bash
ab -n 1000 -c 50 http://localhost:8080/

```
This command sends `1000` requests with a concurrency level of `50` to port `8080`.

### Advanced Features

**Scalability and Load Balancing:** Future enhancements include implementing distributed architecture and horizontal scaling to manage multiple server instances across different machines or containers.

**Advanced Caching:** Planned features include implementing content caching and dynamic caching strategies with time-to-live (TTL) settings.

### Contributing
Contributions are welcome! Please fork the repository and submit a pull request with your changes. Ensure that your changes adhere to the project’s coding standards and include relevant tests.
