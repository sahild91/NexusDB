#include "nexusdb/transaction_manager.h"
#include "nexusdb/utils/logger.h"

namespace nexusdb {

TransactionManager::TransactionManager() : next_transaction_id_(1) {
    LOG_DEBUG("TransactionManager constructor called");
}

TransactionManager::~TransactionManager() {
    LOG_DEBUG("TransactionManager destructor called");
    shutdown();
}

std::optional<std::string> TransactionManager::initialize() {
    std::lock_guard<std::mutex> lock(mutex_);
    LOG_INFO("Initializing Transaction Manager...");
    // Any necessary initialization
    LOG_INFO("Transaction Manager initialized successfully");
    return std::nullopt;
}

void TransactionManager::shutdown() {
    std::lock_guard<std::mutex> lock(mutex_);
    LOG_INFO("Shutting down Transaction Manager...");
    transactions_.clear();
    transaction_logs_.clear();
    LOG_INFO("Transaction Manager shut down successfully");
}

std::optional<transaction_id_t> TransactionManager::begin_transaction() {
    std::lock_guard<std::mutex> lock(mutex_);
    transaction_id_t txn_id = next_transaction_id_++;
    transactions_[txn_id] = TransactionState::ACTIVE;
    transaction_logs_[txn_id] = TransactionLog();
    LOG_INFO("Transaction " + std::to_string(txn_id) + " started");
    return txn_id;
}

std::optional<std::string> TransactionManager::commit_transaction(transaction_id_t txn_id) {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = transactions_.find(txn_id);
    if (it == transactions_.end()) {
        return "Transaction not found";
    }
    if (it->second != TransactionState::ACTIVE) {
        return "Transaction is not active";
    }
    it->second = TransactionState::COMMITTED;
    LOG_INFO("Transaction " + std::to_string(txn_id) + " committed");
    // Here you would apply the changes to the database
    return std::nullopt;
}

std::optional<std::string> TransactionManager::abort_transaction(transaction_id_t txn_id) {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = transactions_.find(txn_id);
    if (it == transactions_.end()) {
        return "Transaction not found";
    }
    if (it->second != TransactionState::ACTIVE) {
        return "Transaction is not active";
    }
    it->second = TransactionState::ABORTED;
    LOG_INFO("Transaction " + std::to_string(txn_id) + " aborted");
    // Here you would undo any changes made by this transaction
    return std::nullopt;
}

std::optional<std::string> TransactionManager::log_operation(transaction_id_t txn_id, const std::string& operation) {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = transactions_.find(txn_id);
    if (it == transactions_.end()) {
        return "Transaction not found";
    }
    if (it->second != TransactionState::ACTIVE) {
        return "Transaction is not active";
    }
    transaction_logs_[txn_id].operations.push_back(operation);
    return std::nullopt;
}

} // namespace nexusdb