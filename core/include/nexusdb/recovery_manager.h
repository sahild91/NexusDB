#ifndef NEXUSDB_RECOVERY_MANAGER_H
#define NEXUSDB_RECOVERY_MANAGER_H

#include <string>
#include <optional>
#include <mutex>
#include <memory>
#include <vector>

namespace nexusdb {

class StorageEngine;

struct LogRecord {
    enum class Type {
        BEGIN,
        COMMIT,
        ABORT,
        UPDATE,
        INSERT,
        DELETE
    };

    Type type;
    uint64_t transaction_id;
    std::string table_name;
    uint64_t record_id;
    std::vector<std::string> old_values;
    std::vector<std::string> new_values;
};

class RecoveryManager {
public:
    explicit RecoveryManager(std::shared_ptr<StorageEngine> storage_engine);
    ~RecoveryManager();

    std::optional<std::string> initialize();
    void shutdown();

    std::optional<std::string> write_log_record(const LogRecord& record);
    std::optional<std::string> recover();

private:
    std::shared_ptr<StorageEngine> storage_engine_;
    mutable std::mutex mutex_;
    std::vector<LogRecord> log_;

    std::optional<std::string> redo();
    std::optional<std::string> undo();
};

} // namespace nexusdb

#endif // NEXUSDB_RECOVERY_MANAGER_H