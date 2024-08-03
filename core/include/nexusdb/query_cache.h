#ifndef NEXUSDB_QUERY_CACHE_H
#define NEXUSDB_QUERY_CACHE_H

#include <string>
#include <unordered_map>
#include <list>
#include <memory>
#include <optional>
#include "nexusdb/query_processor.h" // For QueryResult

namespace nexusdb {

class QueryCache {
public:
    QueryCache(size_t max_size);

    void insert(const std::string& query, const QueryResult& result);
    std::optional<QueryResult> get(const std::string& query);
    void clear();

private:
    size_t max_size_;
    std::unordered_map<std::string, std::pair<QueryResult, std::list<std::string>::iterator>> cache_;
    std::list<std::string> lru_list_;

    void evict();
};

} // namespace nexusdb

#endif // NEXUSDB_QUERY_CACHE_H