#ifndef NEXUSDB_INDEX_MANAGER_H
#define NEXUSDB_INDEX_MANAGER_H

#include <string>
#include <optional>
#include <mutex>
#include <memory>
#include <unordered_map>
#include <vector>
#include <map>

namespace nexusdb {

class StorageEngine;

struct IndexEntry {
    std::string key;
    uint64_t record_id;
};

class IndexManager {
public:
    explicit IndexManager(std::shared_ptr<StorageEngine> storage_engine);
    ~IndexManager();

    std::optional<std::string> initialize();
    void shutdown();

    std::optional<std::string> create_index(const std::string& table_name, const std::string& column_name);
    std::optional<std::string> drop_index(const std::string& table_name, const std::string& column_name);
    std::optional<std::vector<uint64_t>> search_index(const std::string& table_name, const std::string& column_name, const std::string& value);

private:
    std::shared_ptr<StorageEngine> storage_engine_;
    mutable std::mutex mutex_;
    std::unordered_map<std::string, std::map<std::string, std::vector<uint64_t>>> indexes_;

    std::string get_index_key(const std::string& table_name, const std::string& column_name) const;
};

} // namespace nexusdb

#endif // NEXUSDB_INDEX_MANAGER_H