#ifndef NEXUSDB_STORAGE_ENGINE_H
#define NEXUSDB_STORAGE_ENGINE_H

#include <string>
#include <vector>
#include <unordered_map>
#include <cstdint>
#include <memory>
#include <mutex>
#include <optional>
#include <numeric>
#include "nexusdb/index_manager.h"
#include "nexusdb/recovery_manager.h"
#include "nexusdb/file_manager.h"
#include "nexusdb/page.h"

namespace nexusdb {

struct StorageConfig {
    static constexpr size_t DEFAULT_PAGE_SIZE = 4096;
    size_t page_size = DEFAULT_PAGE_SIZE;
    bool use_compression = true;
};

class StorageEngine : public std::enable_shared_from_this<StorageEngine> {
public:
    explicit StorageEngine(const StorageConfig& config = StorageConfig());
    ~StorageEngine();

    std::optional<std::string> initialize(const std::string& data_directory);
    void shutdown();

    // Table and record operations
    std::optional<std::string> create_table(const std::string& table_name, const std::vector<std::string>& schema);
    std::optional<std::string> delete_table(const std::string& table_name);
    std::optional<std::string> insert_record(const std::string& table_name, const std::vector<std::string>& record);
    std::optional<std::string> read_record(const std::string& table_name, uint64_t record_id, std::vector<std::string>& record) const;
    std::optional<std::string> update_record(const std::string& table_name, uint64_t record_id, const std::vector<std::string>& new_record);
    std::optional<std::string> delete_record(const std::string& table_name, uint64_t record_id);

    // Schema and index operations
    std::optional<std::vector<std::string>> get_table_schema(const std::string& table_name) const;
    std::optional<std::string> create_index(const std::string& table_name, const std::string& column_name);
    std::optional<std::string> drop_index(const std::string& table_name, const std::string& column_name);
    std::optional<std::vector<uint64_t>> search_index(const std::string& table_name, const std::string& column_name, const std::string& value) const;

    // Transaction operations
    std::optional<std::string> begin_transaction();
    std::optional<std::string> commit_transaction();
    std::optional<std::string> abort_transaction();

    // Recovery operation
    std::optional<std::string> recover();

    std::shared_ptr<IndexManager> get_index_manager() { return index_manager_; }
    std::shared_ptr<RecoveryManager> get_recovery_manager() { return recovery_manager_; }

private:
    StorageConfig config_;
    std::string data_directory_;
    std::unique_ptr<FileManager> file_manager_;
    std::shared_ptr<IndexManager> index_manager_;
    std::shared_ptr<RecoveryManager> recovery_manager_;
    std::unordered_map<std::string, std::string> table_files_;
    mutable std::mutex mutex_;

    std::string get_table_file_name(const std::string& table_name) const;
    std::unique_ptr<Page> allocate_page(const std::string& table_name);
    std::optional<std::string> write_page(const std::string& table_name, const Page& page);
    std::unique_ptr<Page> read_page(const std::string& table_name, uint64_t page_id) const;
    void update_indexes(const std::string& table_name, const std::vector<std::string>& record, uint64_t record_id);
    void remove_from_indexes(const std::string& table_name, const std::vector<std::string>& record, uint64_t record_id);
};

} // namespace nexusdb

#endif // NEXUSDB_STORAGE_ENGINE_H