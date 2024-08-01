#ifndef NEXUSDB_QUERY_PROCESSOR_H
#define NEXUSDB_QUERY_PROCESSOR_H

#include <string>
#include <optional>
#include <mutex>
#include <memory>
#include <vector>

namespace nexusdb {

class StorageEngine;

struct QueryResult {
    std::vector<std::vector<std::string>> rows;
    std::vector<std::string> column_names;
    std::optional<std::string> error;
};

class QueryProcessor {
public:
    explicit QueryProcessor(std::shared_ptr<StorageEngine> storage_engine);
    ~QueryProcessor();

    std::optional<std::string> initialize();
    void shutdown();

    std::optional<QueryResult> execute_query(const std::string& query);

private:
    std::shared_ptr<StorageEngine> storage_engine_;
    mutable std::mutex mutex_;

    std::optional<QueryResult> execute_select(const std::string& query);
    std::optional<QueryResult> execute_insert(const std::string& query);
    std::optional<QueryResult> execute_update(const std::string& query);
    std::optional<QueryResult> execute_delete(const std::string& query);

    std::optional<std::string> parse_query(const std::string& query);
};

} // namespace nexusdb

#endif // NEXUSDB_QUERY_PROCESSOR_H