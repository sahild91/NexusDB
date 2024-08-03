#ifndef NEXUSDB_QUERY_PROCESSOR_H
#define NEXUSDB_QUERY_PROCESSOR_H

#include "nexusdb/storage_engine.h"
#include "nexusdb/prepared_statement.h"
#include "nexusdb/distributed_storage_engine.h"
#include <string>
#include <optional>
#include <mutex>
#include <memory>
#include <vector>

namespace nexusdb {

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
    std::optional<QueryResult> execute_prepared_statement(const std::shared_ptr<PreparedStatement>& stmt);

    // New method for distributed query execution
    std::optional<QueryResult> execute_distributed_query(const std::string& query, ConsistencyLevel consistency_level);

private:
    std::shared_ptr<StorageEngine> storage_engine_;
    mutable std::mutex mutex_;

    std::optional<QueryResult> execute_select(const std::string& query);
    std::optional<QueryResult> execute_insert(const std::string& query);
    std::optional<QueryResult> execute_update(const std::string& query);
    std::optional<QueryResult> execute_delete(const std::string& query);

    std::optional<std::string> parse_query(const std::string& query);

    // New helper method for prepared statements
    std::optional<QueryResult> execute_prepared_select(const PreparedStatement& stmt);
};

} // namespace nexusdb

#endif // NEXUSDB_QUERY_PROCESSOR_H