#ifndef EXCEPTION_H
#define EXCEPTION_H

#include <exception>
#include <string>

class ConfigException : public std::exception {
public:
    explicit ConfigException(const std::string& message) : message_(message) {}
    const char* what() const noexcept override {
        return message_.c_str();
    }
private:
    std::string message_;
};

class CacheException : public std::exception {
public:
    explicit CacheException(const std::string& message) : message_(message) {}
    const char* what() const noexcept override {
        return message_.c_str();
    }
private:
    std::string message_;
};

class ServerException : public std::exception {
public:
    explicit ServerException(const std::string& message) : message_(message) {}
    const char* what() const noexcept override {
        return message_.c_str();
    }
private:
    std::string message_;
};

#endif 

