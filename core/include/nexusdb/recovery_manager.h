#ifndef NEXUSDB_RECOVERY_MANAGER_H
#define NEXUSDB_RECOVERY_MANAGER_H

#include <string>
#include <optional>
#include <mutex>
#include <memory>
#include <vector>
#include <fstream>
#include <unordered_set>

namespace nexusdb {

class StorageEngine;

enum class LogRecordType {
    BEGIN, COMMIT, ABORT, UPDATE, INSERT, DELETE
};

struct LogRecord {
    LogRecordType type;
    uint64_t transaction_id;
    std::string table_name;
    uint64_t record_id;
    std::string before_image;
    std::string after_image;
};

class RecoveryManager {
public:
    explicit RecoveryManager(std::shared_ptr<StorageEngine> storage_engine);
    ~RecoveryManager();

    std::optional<std::string> initialize(const std::string& log_file_path);
    void shutdown();

    std::optional<std::string> begin_transaction(uint64_t transaction_id);
    std::optional<std::string> commit_transaction(uint64_t transaction_id);
    std::optional<std::string> abort_transaction(uint64_t transaction_id);
    std::optional<std::string> log_operation(const LogRecord& record);
    std::optional<std::string> recover();

private:
    std::shared_ptr<StorageEngine> storage_engine_;
    mutable std::mutex mutex_;
    std::ofstream log_file_;
    std::vector<LogRecord> in_memory_log_;

    void write_log_to_disk(const LogRecord& record);
    void apply_log_record(const LogRecord& record);
    std::optional<std::string> redo();
    std::optional<std::string> undo();
};

} // namespace nexusdb

#endif // NEXUSDB_RECOVERY_MANAGER_H