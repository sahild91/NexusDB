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

    // Create a new B-tree index
    indexes_[index_key] = std::make_unique<BTree<std::string, std::vector<uint64_t>>>(10); // Degree of 10 as an example

    // Scan the table and build the index
    // This is a placeholder implementation and should be replaced with actual table scanning logic
    // storage_engine_->scan_table(table_name, [this, &index_key, &column_name](const Record& record) {
    //     std::string value = record.get_value(column_name);
    //     uint64_t record_id = record.get_id();
    //     insert_into_index(index_key, value, record_id);
    // });
    
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

std::string IndexManager::get_index_key(const std::string& table_name, const std::string& column_name) const {
    return table_name + "." + column_name;
}

} // namespace nexusdb