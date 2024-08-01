#ifndef NEXUSDB_TRANSACTION_MANAGER_H
#define NEXUSDB_TRANSACTION_MANAGER_H

#include <cstdint>
#include <optional>
#include <mutex>
#include <unordered_map>
#include <vector>

namespace nexusdb {

typedef uint64_t transaction_id_t;

enum class TransactionState {
    ACTIVE,
    COMMITTED,
    ABORTED
};

struct TransactionLog {
    std::vector<std::string> operations;
};

class TransactionManager {
public:
    TransactionManager();
    ~TransactionManager();

    std::optional<std::string> initialize();
    void shutdown();

    std::optional<transaction_id_t> begin_transaction();
    std::optional<std::string> commit_transaction(transaction_id_t txn_id);
    std::optional<std::string> abort_transaction(transaction_id_t txn_id);
    
    std::optional<std::string> log_operation(transaction_id_t txn_id, const std::string& operation);

private:
    mutable std::mutex mutex_;
    std::unordered_map<transaction_id_t, TransactionState> transactions_;
    std::unordered_map<transaction_id_t, TransactionLog> transaction_logs_;
    transaction_id_t next_transaction_id_;
};

} // namespace nexusdb

#endif // NEXUSDB_TRANSACTION_MANAGER_H