#include "nexusdb/recovery_manager.h"
#include "nexusdb/storage_engine.h"
#include "nexusdb/utils/logger.h"
#include <algorithm>
#include <unordered_set>

namespace nexusdb {

RecoveryManager::RecoveryManager(std::shared_ptr<StorageEngine> storage_engine)
    : storage_engine_(storage_engine) {
    LOG_DEBUG("RecoveryManager constructor called");
}

RecoveryManager::~RecoveryManager() {
    LOG_DEBUG("RecoveryManager destructor called");
    shutdown();
}

std::optional<std::string> RecoveryManager::initialize() {
    std::lock_guard<std::mutex> lock(mutex_);
    LOG_INFO("Initializing Recovery Manager...");
    // Add any necessary initialization logic here
    LOG_INFO("Recovery Manager initialized successfully");
    return std::nullopt;
}

void RecoveryManager::shutdown() {
    std::lock_guard<std::mutex> lock(mutex_);
    LOG_INFO("Shutting down Recovery Manager...");
    log_.clear();
    LOG_INFO("Recovery Manager shut down successfully");
}

std::optional<std::string> RecoveryManager::write_log_record(const LogRecord& record) {
    std::lock_guard<std::mutex> lock(mutex_);
    log_.push_back(record);
    // In a real implementation, you would write this to disk
    LOG_INFO("Wrote log record for transaction " + std::to_string(record.transaction_id));
    return std::nullopt;
}

std::optional<std::string> RecoveryManager::recover() {
    std::lock_guard<std::mutex> lock(mutex_);
    LOG_INFO("Starting recovery process...");
    
    auto redo_result = redo();
    if (redo_result.has_value()) {
        return redo_result;
    }

    auto undo_result = undo();
    if (undo_result.has_value()) {
        return undo_result;
    }

    LOG_INFO("Recovery process completed successfully");
    return std::nullopt;
}

std::optional<std::string> RecoveryManager::redo() {
    LOG_INFO("Starting redo phase...");
    
    for (const auto& record : log_) {
        if (record.type == LogRecord::Type::UPDATE) {
            // Redo the update operation
            auto update_result = storage_engine_->update_record(record.table_name, record.record_id, record.new_values);
            if (update_result.has_value()) {
                return "Redo failed: " + update_result.value();
            }
        }
        // Handle other types of log records as needed
    }
    
    return std::nullopt;
}

std::optional<std::string> RecoveryManager::undo() {
    LOG_INFO("Starting undo phase...");
    
    std::unordered_set<uint64_t> committed_transactions;
    std::vector<LogRecord> undo_list;

    // Scan the log backwards to determine which transactions need to be undone
    for (auto it = log_.rbegin(); it != log_.rend(); ++it) {
        if (it->type == LogRecord::Type::COMMIT) {
            committed_transactions.insert(it->transaction_id);
        } else if (it->type == LogRecord::Type::BEGIN) {
            if (committed_transactions.find(it->transaction_id) == committed_transactions.end()) {
                // This transaction was not committed, add its records to the undo list
                undo_list.push_back(*it);
            }
        } else {
            if (committed_transactions.find(it->transaction_id) == committed_transactions.end()) {
                undo_list.push_back(*it);
            }
        }
    }

    // Perform the undo operations
    for (const auto& record : undo_list) {
        if (record.type == LogRecord::Type::UPDATE) {
            // Undo the update operation
            auto update_result = storage_engine_->update_record(record.table_name, record.record_id, record.old_values);
            if (update_result.has_value()) {
                return "Undo failed: " + update_result.value();
            }
        }
        // Handle other types of log records as needed
    }

    return std::nullopt;
}

} // namespace nexusdb