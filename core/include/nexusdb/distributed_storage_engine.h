#ifndef NEXUSDB_DISTRIBUTED_STORAGE_ENGINE_H
#define NEXUSDB_DISTRIBUTED_STORAGE_ENGINE_H

#include "nexusdb/storage_engine.h"
#include "nexusdb/query_processor.h"
#include <vector>
#include <string>
#include <optional>
#include <unordered_map>
#include <memory>
#include <mutex>
#include <future>

namespace nexusdb {

struct NodeInfo {
    std::string address;
    uint32_t port;
    bool is_active;
};

enum class ConsistencyLevel {
    ONE,
    QUORUM,
    ALL
};

class DistributedStorageEngine : public StorageEngine {
public:
    DistributedStorageEngine(const StorageConfig& config);
    ~DistributedStorageEngine() override;

    // Node management
    std::optional<std::string> add_node(const std::string& node_address, uint32_t port);
    std::optional<std::string> remove_node(const std::string& node_address);
    std::vector<NodeInfo> get_nodes() const;

    // Table operations
    std::optional<std::string> create_table(const std::string& table_name, const std::vector<std::string>& schema) override;
    std::optional<std::string> delete_table(const std::string& table_name) override;

    // Record operations
    std::optional<std::string> insert_record(const std::string& table_name, const std::vector<std::string>& record) override;
    std::optional<std::string> read_record(const std::string& table_name, uint64_t record_id, std::vector<std::string>& record) const override;
    std::optional<std::string> update_record(const std::string& table_name, uint64_t record_id, const std::vector<std::string>& new_record) override;
    std::optional<std::string> delete_record(const std::string& table_name, uint64_t record_id) override;

    // Distributed query execution
    std::optional<QueryResult> execute_distributed_query(const std::string& query, ConsistencyLevel consistency_level = ConsistencyLevel::QUORUM);

    // Partitioning
    std::optional<std::string> create_partition(const std::string& table_name, const std::string& partition_key);
    std::vector<std::string> get_partitions(const std::string& table_name) const;

    // Replication
    void set_replication_factor(uint32_t factor);
    uint32_t get_replication_factor() const;

    // Consistency management
    void set_consistency_level(ConsistencyLevel level);
    ConsistencyLevel get_consistency_level() const;

    // Data synchronization
    std::optional<std::string> synchronize_node(const std::string& node_address);
    std::optional<std::string> perform_anti_entropy();

    // Failure handling
    std::optional<std::string> handle_node_failure(const std::string& node_address);
    std::optional<std::string> recover_node(const std::string& node_address);

private:
    std::vector<NodeInfo> nodes_;
    std::unordered_map<std::string, std::vector<std::string>> table_partitions_;
    uint32_t replication_factor_;
    ConsistencyLevel consistency_level_;
    mutable std::mutex mutex_;

    // Helper methods
    std::vector<NodeInfo> select_nodes_for_operation(const std::string& table_name, const std::string& partition_key) const;
    std::optional<std::string> distribute_operation(const std::string& operation, const std::vector<NodeInfo>& target_nodes);
    std::future<QueryResult> async_execute_query_on_node(const std::string& query, const NodeInfo& node);
    std::optional<std::string> resolve_conflicts(const std::vector<std::string>& conflicting_records);
    void update_partition_metadata(const std::string& table_name, const std::string& partition_key, const std::string& node_address);
    
    // Gossip protocol for metadata dissemination
    void start_gossip_protocol();
    void gossip_with_node(const NodeInfo& node);

    // Load balancing
    void rebalance_data();
    
    // Monitoring and statistics
    void update_node_statistics(const NodeInfo& node, const std::string& operation, uint64_t latency);
    std::unordered_map<std::string, std::unordered_map<std::string, uint64_t>> node_statistics_;
};

} // namespace nexusdb

#endif // NEXUSDB_DISTRIBUTED_STORAGE_ENGINE_H