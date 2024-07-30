#ifndef NEXUSDB_H
#define NEXUSDB_H

#include <memory>

namespace nexusdb {

class StorageEngine;
class QueryProcessor;
class TransactionManager;
class IndexManager;
class BufferManager;
class RecoveryManager;
class ConcurrencyManager;
class SchemaManager;

/// Main class for NexusDB
class NexusDB {
public:
    /// Constructor
    NexusDB();

    /// Destructor
    ~NexusDB();

    /// Initialize the database
    /// @return true if initialization was successful, false otherwise
    bool initialize();

    /// Shutdown the database
    /// @return true if shutdown was successful, false otherwise
    bool shutdown();

private:
    std::unique_ptr<StorageEngine> storage_engine_;
    std::unique_ptr<QueryProcessor> query_processor_;
    std::unique_ptr<TransactionManager> transaction_manager_;
    std::unique_ptr<IndexManager> index_manager_;
    std::unique_ptr<BufferManager> buffer_manager_;
    std::unique_ptr<RecoveryManager> recovery_manager_;
    std::unique_ptr<ConcurrencyManager> concurrency_manager_;
    std::unique_ptr<SchemaManager> schema_manager_;
};

} // namespace nexusdb

#endif // NEXUSDB_H