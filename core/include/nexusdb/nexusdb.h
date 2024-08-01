#ifndef NEXUSDB_H
#define NEXUSDB_H

#include "storage_engine.h"
#include "buffer_manager.h"
#include "query_processor.h"
#include "transaction_manager.h"
#include "recovery_manager.h"
#include "schema_manager.h"
#include "index_manager.h"
#include "concurrency_manager.h"
#include "system_manager.h"
#include <memory>
#include <string>
#include <vector>
#include <optional>

namespace nexusdb {

class NexusDB {
public:
    NexusDB();
    ~NexusDB();

    std::optional<std::string> initialize(const std::string& data_directory);
    void shutdown();

    std::optional<std::string> create_user(const std::string& username, const std::string& password);
    std::optional<std::string> login(const std::string& username, const std::string& password);
    void logout();

    std::optional<std::string> create_table(const std::string& table_name, const std::vector<std::string>& schema);
    std::optional<std::string> drop_table(const std::string& table_name);
    std::optional<std::string> insert_record(const std::string& table_name, const std::vector<std::string>& record);
    std::optional<std::vector<std::string>> read_record(const std::string& table_name, uint64_t record_id);
    std::optional<std::string> update_record(const std::string& table_name, uint64_t record_id, const std::vector<std::string>& new_record);
    std::optional<std::string> delete_record(const std::string& table_name, uint64_t record_id);

    std::optional<std::vector<std::string>> get_user_tables();
    
    std::optional<QueryResult> execute_query(const std::string& query);

private:
    std::shared_ptr<StorageEngine> storage_engine_;
    std::unique_ptr<BufferManager> buffer_manager_;
    std::unique_ptr<QueryProcessor> query_processor_;
    std::unique_ptr<TransactionManager> transaction_manager_;
    std::unique_ptr<RecoveryManager> recovery_manager_;
    std::unique_ptr<SchemaManager> schema_manager_;
    std::unique_ptr<IndexManager> index_manager_;
    std::unique_ptr<ConcurrencyManager> concurrency_manager_;
    std::unique_ptr<SystemManager> system_manager_;

    std::string current_user_;
    bool is_authenticated_;

    bool check_authentication();
    bool check_table_ownership(const std::string& table_name);
};

} // namespace nexusdb

#endif // NEXUSDB_H