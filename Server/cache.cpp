#include "cache.h"

void CacheClass::CacheData(const std::string &key, const std::string &data){
    cache[key] = data;
}

std::string CacheClass::GetCachedData(const std::string &key){
    auto it = cache.find(key);
    if (it != cache.end()) {
        return it->second;
    }
    return "empty";
}

void CacheClass::ClearCache(){
    cache.clear();
}
