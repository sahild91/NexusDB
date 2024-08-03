#ifndef NEXUSDB_STORAGE_ENGINE_H
#define NEXUSDB_STORAGE_ENGINE_H

#include <string>
#include <vector>
#include <unordered_map>
#include <memory>
#include <optional>
#include <mutex>
#include "nexusdb/index_manager.h"
#include "nexusdb/recovery_manager.h"
#include "nexusdb/file_manager.h"
#include "nexusdb/page.h"
#include "nexusdb/encryptor.h"

namespace nexusdb {

struct StorageConfig {
    static constexpr size_t DEFAULT_PAGE_SIZE = 4096;
    size_t page_size = DEFAULT_PAGE_SIZE;
    bool use_compression = true;
    bool use_encryption = false;
};

enum class ConsistencyLevel {
    ONE,
    QUORUM,
    ALL
};

class StorageEngine : public std::enable_shared_from_this<StorageEngine> {
public:
    explicit StorageEngine(const StorageConfig& config = StorageConfig());
    virtual ~StorageEngine();

    virtual std::optional<std::string> initialize(const std::string& data_directory);
    virtual void shutdown();

    // Table and record operations
    virtual std::optional<std::string> create_table(const std::string& table_name, const std::vector<std::string>& schema);
    virtual std::optional<std::string> delete_table(const std::string& table_name);
    virtual std::optional<std::string> insert_record(const std::string& table_name, const std::vector<std::string>& record);
    virtual std::optional<std::string> read_record(const std::string& table_name, uint64_t record_id, std::vector<std::string>& record) const;
    virtual std::optional<std::string> update_record(const std::string& table_name, uint64_t record_id, const std::vector<std::string>& new_record);
    virtual std::optional<std::string> delete_record(const std::string& table_name, uint64_t record_id);

    // Schema and index operations
    virtual std::optional<std::vector<std::string>> get_table_schema(const std::string& table_name) const;
    virtual std::optional<std::string> create_index(const std::string& table_name, const std::string& column_name);
    virtual std::optional<std::string> drop_index(const std::string& table_name, const std::string& column_name);
    virtual std::optional<std::vector<uint64_t>> search_index(const std::string& table_name, const std::string& column_name, const std::string& value) const;

    // Transaction operations
    virtual std::optional<std::string> begin_transaction();
    virtual std::optional<std::string> commit_transaction();
    virtual std::optional<std::string> abort_transaction();

    // Recovery operation
    virtual std::optional<std::string> recover();

    // Encryption operations
    virtual void enable_encryption(const EncryptionKey& key);
    virtual void disable_encryption();

    // New methods for distributed operations
    virtual std::optional<std::string> add_node(const std::string& node_address, uint32_t port);
    virtual std::optional<std::string> remove_node(const std::string& node_address);
    virtual void set_consistency_level(ConsistencyLevel level);

    std::shared_ptr<IndexManager> get_index_manager() { return index_manager_; }
    std::shared_ptr<RecoveryManager> get_recovery_manager() { return recovery_manager_; }

protected:
    StorageConfig config_;
    std::string data_directory_;
    std::unique_ptr<FileManager> file_manager_;
    std::shared_ptr<IndexManager> index_manager_;
    std::shared_ptr<RecoveryManager> recovery_manager_;
    std::unordered_map<std::string, std::string> table_files_;
    mutable std::mutex mutex_;
    std::unique_ptr<Encryptor> encryptor_;
    ConsistencyLevel consistency_level_;

    std::string get_table_file_name(const std::string& table_name) const;
    std::unique_ptr<Page> allocate_page(const std::string& table_name);
    std::optional<std::string> write_page(const std::string& table_name, const Page& page);
    std::unique_ptr<Page> read_page(const std::string& table_name, uint64_t page_id) const;
    void update_indexes(const std::string& table_name, const std::vector<std::string>& record, uint64_t record_id);
    void remove_from_indexes(const std::string& table_name, const std::vector<std::string>& record, uint64_t record_id);

    virtual std::vector<unsigned char> encrypt_page(const std::vector<unsigned char>& page_data) const;
    virtual std::vector<unsigned char> decrypt_page(const std::vector<unsigned char>& encrypted_data) const;
};

} // namespace nexusdb

#endif // NEXUSDB_STORAGE_ENGINE_H