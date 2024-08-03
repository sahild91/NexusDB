#ifndef NEXUSDB_INDEX_MANAGER_H
#define NEXUSDB_INDEX_MANAGER_H

#include "btree.h"
#include <string>
#include <optional>
#include <mutex>
#include <memory>
#include <unordered_map>
#include <vector>
#include <functional>

namespace nexusdb {

class StorageEngine;

class IndexManager {
public:
    explicit IndexManager(std::shared_ptr<StorageEngine> storage_engine);
    ~IndexManager();

    std::optional<std::string> initialize();
    void shutdown();

    std::optional<std::string> create_index(const std::string& table_name, const std::string& column_name);
    std::optional<std::string> drop_index(const std::string& table_name, const std::string& column_name);
    std::optional<std::string> drop_all_indexes(const std::string& table_name);
    std::optional<std::vector<uint64_t>> search_index(const std::string& table_name, const std::string& column_name, const std::string& value);
    std::optional<std::string> insert_into_index(const std::string& table_name, const std::string& column_name, const std::string& value, uint64_t record_id);
    std::optional<std::string> remove_from_index(const std::string& table_name, const std::string& column_name, const std::string& value, uint64_t record_id);

    // New methods for distributed operations
    std::optional<std::string> sync_index(const std::string& table_name, const std::string& column_name, const std::string& remote_node);
    std::optional<std::string> merge_index(const std::string& table_name, const std::string& column_name, const BTree<std::string, std::vector<uint64_t>>& remote_index);

    // New method for bulk loading
    std::optional<std::string> bulk_load_index(const std::string& table_name, const std::string& column_name, const std::vector<std::pair<std::string, uint64_t>>& data);

    // New method for index statistics
    struct IndexStats {
        size_t num_entries;
        size_t height;
        size_t num_nodes;
    };
    std::optional<IndexStats> get_index_stats(const std::string& table_name, const std::string& column_name);

private:
    std::shared_ptr<StorageEngine> storage_engine_;
    mutable std::mutex mutex_;
    std::unordered_map<std::string, std::unique_ptr<BTree<std::string, std::vector<uint64_t>>>> indexes_;

    std::string get_index_key(const std::string& table_name, const std::string& column_name) const;
    
    // New method for distributed conflict resolution
    void resolve_conflicts(std::vector<uint64_t>& local_records, const std::vector<uint64_t>& remote_records);
};

} // namespace nexusdb

#endif // NEXUSDB_INDEX_MANAGER_H