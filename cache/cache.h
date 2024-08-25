#ifndef CACHE_H
#define CACHE_H

#include <string>
#include <unordered_map>
#include <mutex>
#include "../Exception/exception.h"

class Cache {
public:
    Cache() = default;
    ~Cache() = default;

    std::string get(const std::string& key);
    void set(const std::string& key, const std::string& value);
    void clear();


private:
    std::unordered_map<std::string, std::string> cache_map;
    std::mutex cache_mutex;
};

#endif 
