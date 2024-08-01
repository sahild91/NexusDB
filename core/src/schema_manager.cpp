#include "nexusdb/schema_manager.h"
#include "nexusdb/utils/logger.h"

namespace nexusdb {

SchemaManager::SchemaManager() {
    LOG_DEBUG("SchemaManager constructor called");
}

SchemaManager::~SchemaManager() {
    LOG_DEBUG("SchemaManager destructor called");
    shutdown();
}

std::optional<std::string> SchemaManager::initialize() {
    LOG_INFO("Initializing Schema Manager...");
    // Any necessary initialization can be done here
    LOG_INFO("Schema Manager initialized successfully");
    return std::nullopt;
}

void SchemaManager::shutdown() {
    LOG_INFO("Shutting down Schema Manager...");
    std::lock_guard<std::mutex> lock(mutex_);
    schemas_.clear();
    LOG_INFO("Schema Manager shut down successfully");
}

std::optional<std::string> SchemaManager::create_table(const std::string& table_name, const std::vector<ColumnDefinition>& columns) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    if (schemas_.find(table_name) != schemas_.end()) {
        LOG_ERROR("Table already exists: " + table_name);
        return "Table already exists: " + table_name;
    }
    
    TableSchema new_schema;
    new_schema.table_name = table_name;
    new_schema.columns = columns;
    
    schemas_[table_name] = new_schema;
    
    LOG_INFO("Created schema for table: " + table_name);
    return std::nullopt;
}

std::optional<std::string> SchemaManager::drop_table(const std::string& table_name) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    auto it = schemas_.find(table_name);
    if (it == schemas_.end()) {
        LOG_ERROR("Table does not exist: " + table_name);
        return "Table does not exist: " + table_name;
    }
    
    schemas_.erase(it);
    
    LOG_INFO("Dropped schema for table: " + table_name);
    return std::nullopt;
}

std::optional<TableSchema> SchemaManager::get_table_schema(const std::string& table_name) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    auto it = schemas_.find(table_name);
    if (it == schemas_.end()) {
        LOG_ERROR("Table does not exist: " + table_name);
        return std::nullopt;
    }
    
    return it->second;
}

} // namespace nexusdb