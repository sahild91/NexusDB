// File: src/query_cache.cpp
#include "nexusdb/query_cache.h"
#include <mutex>

namespace nexusdb {

QueryCache::QueryCache(size_t max_size) : max_size_(max_size) {}

void QueryCache::insert(const std::string& query, const QueryResult& result) {
    std::lock_guard<std::mutex> lock(mutex_);

    auto it = cache_.find(query);
    if (it != cache_.end()) {
        // Query already in cache, update it
        lru_list_.erase(it->second.second);
    } else if (cache_.size() >= max_size_) {
        // Cache is full, evict least recently used item
        evict();
    }

    // Insert new item at the front of the list
    lru_list_.push_front(query);
    cache_[query] = std::make_pair(result, lru_list_.begin());
}

std::optional<QueryResult> QueryCache::get(const std::string& query) {
    std::lock_guard<std::mutex> lock(mutex_);

    auto it = cache_.find(query);
    if (it == cache_.end()) {
        return std::nullopt;
    }

    // Move accessed item to front of LRU list
    lru_list_.erase(it->second.second);
    lru_list_.push_front(query);
    it->second.second = lru_list_.begin();

    return it->second.first;
}

void QueryCache::clear() {
    std::lock_guard<std::mutex> lock(mutex_);
    cache_.clear();
    lru_list_.clear();
}

void QueryCache::evict() {
    if (!lru_list_.empty()) {
        auto last = lru_list_.back();
        cache_.erase(last);
        lru_list_.pop_back();
    }
}

} // namespace nexusdb