#include "cache.h"
#include <stdexcept>

std::string Cache::get(const std::string &key)
{
    std::lock_guard<std::mutex> lock(cache_mutex);
    auto it = cache_map.find(key);
    if (it != cache_map.end())
    {
        return it->second;
    }
    throw CacheException("Key not found in cache");
}

void Cache::set(const std::string &key, const std::string &value)
{
    std::lock_guard<std::mutex> lock(cache_mutex);
    cache_map[key] = value;
}
void Cache::clear()
{
    std::lock_guard<std::mutex> lock(cache_mutex);
    cache_map.clear();
}
