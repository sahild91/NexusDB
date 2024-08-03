#include "nexusdb/index_manager.h"
#include "nexusdb/storage_engine.h"
#include "nexusdb/utils/logger.h"
#include <algorithm>

namespace nexusdb {

IndexManager::IndexManager(std::shared_ptr<StorageEngine> storage_engine)
    : storage_engine_(storage_engine) {
    LOG_DEBUG("IndexManager constructor called");
}

IndexManager::~IndexManager() {
    LOG_DEBUG("IndexManager destructor called");
    shutdown();
}

std::optional<std::string> IndexManager::initialize() {
    std::lock_guard<std::mutex> lock(mutex_);
    LOG_INFO("Initializing Index Manager...");
    // Add any necessary initialization logic here
    LOG_INFO("Index Manager initialized successfully");
    return std::nullopt;
}

void IndexManager::shutdown() {
    std::lock_guard<std::mutex> lock(mutex_);
    LOG_INFO("Shutting down Index Manager...");
    indexes_.clear();
    LOG_INFO("Index Manager shut down successfully");
}

std::optional<std::string> IndexManager::create_index(const std::string& table_name, const std::string& column_name) {
    std::lock_guard<std::mutex> lock(mutex_);
    std::string index_key = get_index_key(table_name, column_name);
    
    if (indexes_.find(index_key) != indexes_.end()) {
        return "Index already exists for this table and column";
    }

    indexes_[index_key] = std::make_unique<BTree<std::string, std::vector<uint64_t>>>(10); // Degree of 10 as an example

    // Populate the index with existing data
    auto table_scan = storage_engine_->full_table_scan(table_name);
    if (table_scan.has_value()) {
        for (const auto& [record_id, record] : *table_scan) {
            if (column_name < record.size()) {
                insert_into_index(table_name, column_name, record[std::stoi(column_name)], record_id);
            }
        }
    }
    
    LOG_INFO("Created index for " + table_name + "." + column_name);
    return std::nullopt;
}

std::optional<std::string> IndexManager::drop_index(const std::string& table_name, const std::string& column_name) {
    std::lock_guard<std::mutex> lock(mutex_);
    std::string index_key = get_index_key(table_name, column_name);
    
    if (indexes_.find(index_key) == indexes_.end()) {
        return "Index does not exist for this table and column";
    }

    indexes_.erase(index_key);
    
    LOG_INFO("Dropped index for " + table_name + "." + column_name);
    return std::nullopt;
}

std::optional<std::vector<uint64_t>> IndexManager::search_index(const std::string& table_name, const std::string& column_name, const std::string& value) {
    std::lock_guard<std::mutex> lock(mutex_);
    std::string index_key = get_index_key(table_name, column_name);
    
    auto it = indexes_.find(index_key);
    if (it == indexes_.end()) {
        return std::nullopt; // Index doesn't exist
    }

    auto search_result = it->second->search(value);
    if (search_result) {
        return *search_result;
    }

    return std::vector<uint64_t>{}; // No matches found
}

std::optional<std::string> IndexManager::insert_into_index(const std::string& table_name, const std::string& column_name, const std::string& value, uint64_t record_id) {
    std::lock_guard<std::mutex> lock(mutex_);
    std::string index_key = get_index_key(table_name, column_name);
    
    auto it = indexes_.find(index_key);
    if (it == indexes_.end()) {
        return "Index does not exist for this table and column";
    }

    auto search_result = it->second->search(value);
    if (search_result) {
        search_result->push_back(record_id);
        it->second->insert(value, *search_result);
    } else {
        it->second->insert(value, {record_id});
    }

    return std::nullopt;
}

std::optional<std::string> IndexManager::remove_from_index(const std::string& table_name, const std::string& column_name, const std::string& value, uint64_t record_id) {
    std::lock_guard<std::mutex> lock(mutex_);
    std::string index_key = get_index_key(table_name, column_name);
    
    auto it = indexes_.find(index_key);
    if (it == indexes_.end()) {
        return "Index does not exist for this table and column";
    }

    auto search_result = it->second->search(value);
    if (search_result) {
        auto& records = *search_result;
        records.erase(std::remove(records.begin(), records.end(), record_id), records.end());
        if (records.empty()) {
            it->second->remove(value);
        } else {
            it->second->insert(value, records);
        }
    }

    return std::nullopt;
}

std::optional<std::string> IndexManager::drop_all_indexes(const std::string& table_name) {
    std::lock_guard<std::mutex> lock(mutex_);
    LOG_INFO("Dropping all indexes for table: " + table_name);

    std::string prefix = table_name + ".";
    auto it = indexes_.begin();
    while (it != indexes_.end()) {
        if (it->first.compare(0, prefix.length(), prefix) == 0) {
            it = indexes_.erase(it);
        } else {
            ++it;
        }
    }

    LOG_INFO("All indexes dropped for table: " + table_name);
    return std::nullopt;
}

std::optional<std::string> IndexManager::sync_index(const std::string& table_name, const std::string& column_name, const std::string& remote_node) {
    // This is a placeholder implementation. In a real system, you'd need to implement
    // network communication and data transfer with the remote node.
    LOG_INFO("Syncing index for " + table_name + "." + column_name + " with node: " + remote_node);
    return std::nullopt;
}

std::optional<std::string> IndexManager::merge_index(const std::string& table_name, const std::string& column_name, const BTree<std::string, std::vector<uint64_t>>& remote_index) {
    std::lock_guard<std::mutex> lock(mutex_);
    std::string index_key = get_index_key(table_name, column_name);
    
    auto it = indexes_.find(index_key);
    if (it == indexes_.end()) {
        return "Index does not exist for this table and column";
    }

    // Merge the remote index into the local index
    remote_index.traverse([&](const std::string& key, const std::vector<uint64_t>& remote_records) {
        auto local_result = it->second->search(key);
        if (local_result) {
            std::vector<uint64_t> merged_records = *local_result;
            resolve_conflicts(merged_records, remote_records);
            it->second->insert(key, merged_records);
        } else {
            it->second->insert(key, remote_records);
        }
    });

    LOG_INFO("Merged index for " + table_name + "." + column_name);
    return std::nullopt;
}

std::optional<std::string> IndexManager::bulk_load_index(const std::string& table_name, const std::string& column_name, const std::vector<std::pair<std::string, uint64_t>>& data) {
    std::lock_guard<std::mutex> lock(mutex_);
    std::string index_key = get_index_key(table_name, column_name);
    
    if (indexes_.find(index_key) != indexes_.end()) {
        return "Index already exists for this table and column";
    }

    auto new_index = std::make_unique<BTree<std::string, std::vector<uint64_t>>>(10);
    
    // Sort the data for efficient bulk loading
    std::vector<std::pair<std::string, uint64_t>> sorted_data = data;
    std::sort(sorted_data.begin(), sorted_data.end());

    // Bulk load the index
    for (const auto& [value, record_id] : sorted_data) {
        auto search_result = new_index->search(value);
        if (search_result) {
            search_result->push_back(record_id);
            new_index->insert(value, *search_result);
        } else {
            new_index->insert(value, {record_id});
        }
    }

    indexes_[index_key] = std::move(new_index);
    
    LOG_INFO("Bulk loaded index for " + table_name + "." + column_name);
    return std::nullopt;
}

std::optional<IndexManager::IndexStats> IndexManager::get_index_stats(const std::string& table_name, const std::string& column_name) {
    std::lock_guard<std::mutex> lock(mutex_);
    std::string index_key = get_index_key(table_name, column_name);
    
    auto it = indexes_.find(index_key);
    if (it == indexes_.end()) {
        return std::nullopt;
    }

    IndexStats stats;
    stats.num_entries = it->second->size();
    stats.height = it->second->height();
    stats.num_nodes = it->second->node_count();

    return stats;
}

std::string IndexManager::get_index_key(const std::string& table_name, const std::string& column_name) const {
    return table_name + "." + column_name;
}

void IndexManager::resolve_conflicts(std::vector<uint64_t>& local_records, const std::vector<uint64_t>& remote_records) {
    // This is a simple conflict resolution strategy that takes the union of local and remote records
    std::vector<uint64_t> merged_records;
    std::set_union(local_records.begin(), local_records.end(),
                   remote_records.begin(), remote_records.end(),
                   std::back_inserter(merged_records));
    local_records = merged_records;
}

} // namespace nexusdb