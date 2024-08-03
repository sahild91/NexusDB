#ifndef NEXUSDB_H
#define NEXUSDB_H

#include "nexusdb/storage_engine.h"
#include "nexusdb/buffer_manager.h"
#include "nexusdb/query_processor.h"
#include "nexusdb/transaction_manager.h"
#include "nexusdb/recovery_manager.h"
#include "nexusdb/schema_manager.h"
#include "nexusdb/index_manager.h"
#include "nexusdb/concurrency_manager.h"
#include "nexusdb/system_manager.h"
#include "nexusdb/distributed_storage_engine.h"
#include "nexusdb/query_cache.h"
#include "nexusdb/secure_connection_manager.h"
#include "nexusdb/prepared_statement.h"
#include "nexusdb/encryptor.h"
#include <memory>
#include <string>
#include <vector>
#include <optional>

namespace nexusdb {

class NexusDB {
public:
    NexusDB();
    ~NexusDB();

    std::optional<std::string> initialize(const std::string& data_directory, bool distributed = false);
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

    // New methods for distributed operations
    std::optional<std::string> add_node(const std::string& node_address, uint32_t port);
    std::optional<std::string> remove_node(const std::string& node_address);
    std::vector<NodeInfo> get_nodes() const;
    void set_replication_factor(uint32_t factor);
    void set_consistency_level(ConsistencyLevel level);

    // New method for secure connections
    std::optional<std::string> start_secure_server(int port, const std::string& cert_file, const std::string& key_file);

    // New methods for prepared statements
    std::shared_ptr<PreparedStatement> prepare_statement(const std::string& query);
    std::optional<QueryResult> execute_prepared_statement(const std::shared_ptr<PreparedStatement>& stmt, const std::vector<std::variant<int, double, std::string, std::vector<unsigned char>>>& params);

    // New method for query caching
    void set_query_cache_size(size_t size);

    // New method for encryption
    void enable_encryption(const EncryptionKey& key);

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
    std::unique_ptr<QueryCache> query_cache_;
    std::unique_ptr<SecureConnectionManager> secure_connection_manager_;
    std::unique_ptr<Encryptor> encryptor_;

    std::string current_user_;
    bool is_authenticated_;
    bool is_distributed_;

    bool check_authentication();
    bool check_table_ownership(const std::string& table_name);
};

} // namespace nexusdb

#endif // NEXUSDB_H