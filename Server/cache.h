#ifndef CACHE_H
#define CACHE_H

#include <unordered_map>
#include <string>

class CacheClass
{
protected:
    std::unordered_map<std::string, std::string> cache;

public:
    void CacheData(const std::string& key, const std::string& data);

    std::string GetCachedData(const std::string& key);

    void ClearCache();
};

#endif // CACHE_H
