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

std::optional<std::string> RecoveryManager::initialize(const std::string& log_file_path) {
    std::lock_guard<std::mutex> lock(mutex_);
    LOG_INFO("Initializing Recovery Manager...");
    log_file_.open(log_file_path, std::ios::app | std::ios::binary);
    if (!log_file_.is_open()) {
        return "Failed to open log file: " + log_file_path;
    }
    LOG_INFO("Recovery Manager initialized successfully");
    return std::nullopt;
}

void RecoveryManager::shutdown() {
    std::lock_guard<std::mutex> lock(mutex_);
    LOG_INFO("Shutting down Recovery Manager...");
    if (log_file_.is_open()) {
        log_file_.close();
    }
    in_memory_log_.clear();
    LOG_INFO("Recovery Manager shut down successfully");
}

std::optional<std::string> RecoveryManager::begin_transaction(uint64_t transaction_id) {
    LogRecord record{LogRecordType::BEGIN, transaction_id, "", 0, "", ""};
    return log_operation(record);
}

std::optional<std::string> RecoveryManager::commit_transaction(uint64_t transaction_id) {
    LogRecord record{LogRecordType::COMMIT, transaction_id, "", 0, "", ""};
    return log_operation(record);
}

std::optional<std::string> RecoveryManager::abort_transaction(uint64_t transaction_id) {
    LogRecord record{LogRecordType::ABORT, transaction_id, "", 0, "", ""};
    return log_operation(record);
}

std::optional<std::string> RecoveryManager::log_operation(const LogRecord& record) {
    std::lock_guard<std::mutex> lock(mutex_);
    write_log_to_disk(record);
    in_memory_log_.push_back(record);
    LOG_INFO("Logged operation for transaction " + std::to_string(record.transaction_id));
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

void RecoveryManager::write_log_to_disk(const LogRecord& record) {
    log_file_.write(reinterpret_cast<const char*>(&record), sizeof(LogRecord));
    log_file_.flush();
}

void RecoveryManager::apply_log_record(const LogRecord& record) {
    switch (record.type) {
        case LogRecordType::UPDATE:
            storage_engine_->update_record(record.table_name, record.record_id, {record.after_image});
            break;
        case LogRecordType::INSERT:
            storage_engine_->insert_record(record.table_name, {record.after_image});
            break;
        case LogRecordType::DELETE:
            storage_engine_->delete_record(record.table_name, record.record_id);
            break;
        default:
            // BEGIN, COMMIT, and ABORT don't require direct action on the storage engine
            break;
    }
}

std::optional<std::string> RecoveryManager::redo() {
    LOG_INFO("Starting redo phase...");
    
    for (const auto& record : in_memory_log_) {
        if (record.type != LogRecordType::BEGIN && record.type != LogRecordType::COMMIT && record.type != LogRecordType::ABORT) {
            apply_log_record(record);
        }
    }
    
    return std::nullopt;
}

std::optional<std::string> RecoveryManager::undo() {
    LOG_INFO("Starting undo phase...");
    
    std::unordered_set<uint64_t> committed_transactions;
    std::vector<LogRecord> undo_list;

    // Scan the log backwards to determine which transactions need to be undone
    for (auto it = in_memory_log_.rbegin(); it != in_memory_log_.rend(); ++it) {
        if (it->type == LogRecordType::COMMIT) {
            committed_transactions.insert(it->transaction_id);
        } else if (it->type == LogRecordType::BEGIN) {
            if (committed_transactions.find(it->transaction_id) == committed_transactions.end()) {
                // This transaction was not committed, add its records to the undo list
                undo_list.push_back(*it);
            }
        } else if (it->type != LogRecordType::ABORT) {
            if (committed_transactions.find(it->transaction_id) == committed_transactions.end()) {
                undo_list.push_back(*it);
            }
        }
    }

    // Perform the undo operations
    for (const auto& record : undo_list) {
        if (record.type == LogRecordType::UPDATE) {
            storage_engine_->update_record(record.table_name, record.record_id, {record.before_image});
        } else if (record.type == LogRecordType::INSERT) {
            storage_engine_->delete_record(record.table_name, record.record_id);
        } else if (record.type == LogRecordType::DELETE) {
            storage_engine_->insert_record(record.table_name, {record.before_image});
        }
    }

    return std::nullopt;
}

} // namespace nexusdb