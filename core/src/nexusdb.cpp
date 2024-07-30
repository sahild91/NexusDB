#include "nexusdb/nexusdb.h"
#include "nexusdb/storage_engine.h"
#include "nexusdb/query_processor.h"
#include "nexusdb/transaction_manager.h"
#include "nexusdb/index_manager.h"
#include "nexusdb/buffer_manager.h"
#include "nexusdb/recovery_manager.h"
#include "nexusdb/concurrency_manager.h"
#include "nexusdb/schema_manager.h"
#include <iostream>

namespace nexusdb {

NexusDB::NexusDB() {
    storage_engine_ = std::make_unique<StorageEngine>();
    query_processor_ = std::make_unique<QueryProcessor>();
    transaction_manager_ = std::make_unique<TransactionManager>();
    index_manager_ = std::make_unique<IndexManager>();
    buffer_manager_ = std::make_unique<BufferManager>();
    recovery_manager_ = std::make_unique<RecoveryManager>();
    concurrency_manager_ = std::make_unique<ConcurrencyManager>();
    schema_manager_ = std::make_unique<SchemaManager>();
}

NexusDB::~NexusDB() {
    shutdown();
}

bool NexusDB::initialize() {
    std::cout << "Initializing NexusDB..." << std::endl;
    
    if (!storage_engine_->initialize() ||
        !query_processor_->initialize() ||
        !transaction_manager_->initialize() ||
        !index_manager_->initialize() ||
        !buffer_manager_->initialize() ||
        !recovery_manager_->initialize() ||
        !concurrency_manager_->initialize() ||
        !schema_manager_->initialize()) {
        std::cerr << "Failed to initialize one or more components" << std::endl;
        return false;
    }
    
    std::cout << "NexusDB initialized successfully" << std::endl;
    return true;
}

bool NexusDB::shutdown() {
    std::cout << "Shutting down NexusDB..." << std::endl;
    // Perform cleanup and shutdown procedures
    std::cout << "NexusDB shut down successfully" << std::endl;
    return true;
}

} // namespace nexusdb