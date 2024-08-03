#include "nexusdb/nexusdb.h"
#include "nexusdb/utils/logger.h"
#include <algorithm>

namespace nexusdb {

NexusDB::NexusDB() : is_authenticated_(false), is_distributed_(false) {
    buffer_manager_ = std::make_unique<BufferManager>();
    query_processor_ = std::make_unique<QueryProcessor>();
    transaction_manager_ = std::make_unique<TransactionManager>();
    recovery_manager_ = std::make_unique<RecoveryManager>();
    schema_manager_ = std::make_unique<SchemaManager>();
    index_manager_ = std::make_unique<IndexManager>();
    concurrency_manager_ = std::make_unique<ConcurrencyManager>();
    query_cache_ = std::make_unique<QueryCache>(1000); // Default cache size of 1000 queries
}

NexusDB::~NexusDB() {
    shutdown();
}

std::optional<std::string> NexusDB::initialize(const std::string& data_directory, bool distributed) {
    LOG_INFO("Initializing NexusDB...");
    is_distributed_ = distributed;
    
    if (is_distributed_) {
        storage_engine_ = std::make_shared<DistributedStorageEngine>(StorageConfig());
    } else {
        storage_engine_ = std::make_shared<StorageEngine>(StorageConfig());
    }

    auto storage_result = storage_engine_->initialize(data_directory);
    if (storage_result.has_value()) {
        LOG_ERROR("Failed to initialize StorageEngine: " + storage_result.value());
        return storage_result;
    }

    auto buffer_result = buffer_manager_->initialize();
    if (buffer_result.has_value()) {
        LOG_ERROR("Failed to initialize BufferManager: " + buffer_result.value());
        return buffer_result;
    }

    auto query_result = query_processor_->initialize();
    if (query_result.has_value()) {
        LOG_ERROR("Failed to initialize QueryProcessor: " + query_result.value());
        return query_result;
    }

    auto transaction_result = transaction_manager_->initialize();
    if (transaction_result.has_value()) {
        LOG_ERROR("Failed to initialize TransactionManager: " + transaction_result.value());
        return transaction_result;
    }

    auto recovery_result = recovery_manager_->initialize();
    if (recovery_result.has_value()) {
        LOG_ERROR("Failed to initialize RecoveryManager: " + recovery_result.value());
        return recovery_result;
    }

    auto schema_result = schema_manager_->initialize();
    if (schema_result.has_value()) {
        LOG_ERROR("Failed to initialize SchemaManager: " + schema_result.value());
        return schema_result;
    }

    auto index_result = index_manager_->initialize();
    if (index_result.has_value()) {
        LOG_ERROR("Failed to initialize IndexManager: " + index_result.value());
        return index_result;
    }

    auto concurrency_result = concurrency_manager_->initialize();
    if (concurrency_result.has_value()) {
        LOG_ERROR("Failed to initialize ConcurrencyManager: " + concurrency_result.value());
        return concurrency_result;
    }

    system_manager_ = std::make_unique<SystemManager>(storage_engine_);
    auto system_result = system_manager_->initialize();
    if (system_result.has_value()) {
        LOG_ERROR("Failed to initialize SystemManager: " + system_result.value());
        return system_result;
    }

    LOG_INFO("NexusDB initialized successfully");
    return std::nullopt;
}

void NexusDB::shutdown() {
    LOG_INFO("Shutting down NexusDB...");
    system_manager_->shutdown();
    concurrency_manager_->shutdown();
    index_manager_->shutdown();
    schema_manager_->shutdown();
    recovery_manager_->shutdown();
    transaction_manager_->shutdown();
    query_processor_->shutdown();
    buffer_manager_->shutdown();
    storage_engine_->shutdown();
    LOG_INFO("NexusDB shut down successfully");
}

std::optional<std::string> NexusDB::create_user(const std::string& username, const std::string& password) {
    if (!check_authentication()) {
        return "Not authenticated. Please login first.";
    }
    return system_manager_->create_user(username, password);
}

std::optional<std::string> NexusDB::login(const std::string& username, const std::string& password) {
    auto auth_result = system_manager_->authenticate_user(username, password);
    if (!auth_result.has_value()) {
        current_user_ = username;
        is_authenticated_ = true;
        LOG_INFO("User " + username + " logged in successfully");
        return std::nullopt;
    }
    LOG_ERROR("Login failed for user " + username);
    return "Login failed: " + auth_result.value();
}

void NexusDB::logout() {
    current_user_ = "";
    is_authenticated_ = false;
    LOG_INFO("User logged out");
}

std::optional<std::string> NexusDB::create_table(const std::string& table_name, const std::vector<std::string>& schema) {
    if (!check_authentication()) {
        return "Not authenticated. Please login first.";
    }
    LOG_INFO("Creating table: " + table_name);
    auto create_result = storage_engine_->create_table(table_name, schema);
    if (!create_result.has_value()) {
        return system_manager_->add_user_table(current_user_, table_name);
    }
    return create_result;
}

std::optional<std::string> NexusDB::drop_table(const std::string& table_name) {
    if (!check_authentication()) {
        return "Not authenticated. Please login first.";
    }
    if (!check_table_ownership(table_name)) {
        return "User does not have permission to access this table";
    }
    LOG_INFO("Dropping table: " + table_name);
    auto drop_result = storage_engine_->delete_table(table_name);
    if (!drop_result.has_value()) {
        return system_manager_->remove_user_table(current_user_, table_name);
    }
    return drop_result;
}

std::optional<std::string> NexusDB::insert_record(const std::string& table_name, const std::vector<std::string>& record) {
    if (!check_authentication()) {
        return "Not authenticated. Please login first.";
    }
    if (!check_table_ownership(table_name)) {
        return "User does not have permission to access this table";
    }
    LOG_INFO("Inserting record into table: " + table_name);
    return storage_engine_->insert_record(table_name, record);
}

std::optional<std::vector<std::string>> NexusDB::read_record(const std::string& table_name, uint64_t record_id) {
    if (!check_authentication()) {
        return std::vector<std::string>{"Not authenticated. Please login first."};
    }
    if (!check_table_ownership(table_name)) {
        return std::vector<std::string>{"User does not have permission to access this table"};
    }
    LOG_INFO("Reading record from table: " + table_name + ", record_id: " + std::to_string(record_id));
    std::vector<std::string> record;
    auto read_result = storage_engine_->read_record(table_name, record_id, record);
    if (read_result.has_value()) {
        return std::vector<std::string>{read_result.value()};
    }
    return record;
}

std::optional<std::string> NexusDB::update_record(const std::string& table_name, uint64_t record_id, const std::vector<std::string>& new_record) {
    if (!check_authentication()) {
        return "Not authenticated. Please login first.";
    }
    if (!check_table_ownership(table_name)) {
        return "User does not have permission to access this table";
    }
    LOG_INFO("Updating record in table: " + table_name + ", record_id: " + std::to_string(record_id));
    return storage_engine_->update_record(table_name, record_id, new_record);
}

std::optional<std::string> NexusDB::delete_record(const std::string& table_name, uint64_t record_id) {
    if (!check_authentication()) {
        return "Not authenticated. Please login first.";
    }
    if (!check_table_ownership(table_name)) {
        return "User does not have permission to access this table";
    }
    LOG_INFO("Deleting record from table: " + table_name + ", record_id: " + std::to_string(record_id));
    return storage_engine_->delete_record(table_name, record_id);
}

std::optional<std::vector<std::string>> NexusDB::get_user_tables() {
    if (!check_authentication()) {
        return std::vector<std::string>{"Not authenticated. Please login first."};
    }
    return system_manager_->get_user_tables(current_user_);
}

std::optional<QueryResult> NexusDB::execute_query(const std::string& query) {
    if (!check_authentication()) {
        QueryResult error_result;
        error_result.error = "Not authenticated. Please login first.";
        return error_result;
    }

    // Check query cache first
    auto cached_result = query_cache_->get(query);
    if (cached_result) {
        LOG_INFO("Query result retrieved from cache");
        return *cached_result;
    }

    auto result = query_processor_->execute_query(query);
    if (result && !result->error) {
        // Cache the successful query result
        query_cache_->insert(query, *result);
    }
    return result;
}

std::optional<std::string> NexusDB::add_node(const std::string& node_address, uint32_t port) {
    if (!is_distributed_) {
        return "Not in distributed mode";
    }
    auto distributed_engine = std::dynamic_pointer_cast<DistributedStorageEngine>(storage_engine_);
    return distributed_engine->add_node(node_address, port);
}

std::optional<std::string> NexusDB::remove_node(const std::string& node_address) {
    if (!is_distributed_) {
        return "Not in distributed mode";
    }
    auto distributed_engine = std::dynamic_pointer_cast<DistributedStorageEngine>(storage_engine_);
    return distributed_engine->remove_node(node_address);
}

std::vector<NodeInfo> NexusDB::get_nodes() const {
    if (!is_distributed_) {
        return {};
    }
    auto distributed_engine = std::dynamic_pointer_cast<DistributedStorageEngine>(storage_engine_);
    return distributed_engine->get_nodes();
}

void NexusDB::set_replication_factor(uint32_t factor) {
    if (is_distributed_) {
        auto distributed_engine = std::dynamic_pointer_cast<DistributedStorageEngine>(storage_engine_);
        distributed_engine->set_replication_factor(factor);
        LOG_INFO("Replication factor set to " + std::to_string(factor));
    } else {
        LOG_WARNING("Attempted to set replication factor in non-distributed mode");
    }
}

void NexusDB::set_consistency_level(ConsistencyLevel level) {
    if (is_distributed_) {
        auto distributed_engine = std::dynamic_pointer_cast<DistributedStorageEngine>(storage_engine_);
        distributed_engine->set_consistency_level(level);
        LOG_INFO("Consistency level set to " + std::to_string(static_cast<int>(level)));
    } else {
        LOG_WARNING("Attempted to set consistency level in non-distributed mode");
    }
}

std::optional<std::string> NexusDB::start_secure_server(int port, const std::string& cert_file, const std::string& key_file) {
    try {
        secure_connection_manager_ = std::make_unique<SecureConnectionManager>(cert_file, key_file);
        secure_connection_manager_->start_server(port, [this](std::unique_ptr<SecureSocket> socket) {
            // Handle incoming secure connections
            // This is where you would implement your protocol for handling database requests over a secure connection
            LOG_INFO("Secure connection established");
            // Example: Read a command from the socket
            auto data = socket->receive();
            std::string command(data.begin(), data.end());
            LOG_INFO("Received command: " + command);
            // Process the command and send a response
            // This is just a placeholder - you'd implement actual command processing here
            std::string response = "Command processed: " + command;
            socket->send(std::vector<unsigned char>(response.begin(), response.end()));
        });
        LOG_INFO("Secure server started on port " + std::to_string(port));
        return std::nullopt;
    } catch (const std::exception& e) {
        LOG_ERROR("Failed to start secure server: " + std::string(e.what()));
        return "Failed to start secure server: " + std::string(e.what());
    }
}

std::shared_ptr<PreparedStatement> NexusDB::prepare_statement(const std::string& query) {
    if (!check_authentication()) {
        LOG_ERROR("Not authenticated. Please login first.");
        return nullptr;
    }
    try {
        auto stmt = std::make_shared<PreparedStatement>(query);
        LOG_INFO("Prepared statement created for query: " + query);
        return stmt;
    } catch (const std::exception& e) {
        LOG_ERROR("Failed to prepare statement: " + std::string(e.what()));
        return nullptr;
    }
}

std::optional<QueryResult> NexusDB::execute_prepared_statement(
    const std::shared_ptr<PreparedStatement>& stmt,
    const std::vector<std::variant<int, double, std::string, std::vector<unsigned char>>>& params) {
    if (!check_authentication()) {
        QueryResult error_result;
        error_result.error = "Not authenticated. Please login first.";
        return error_result;
    }
    if (!stmt) {
        QueryResult error_result;
        error_result.error = "Invalid prepared statement";
        return error_result;
    }
    try {
        // Bind parameters to the prepared statement
        for (size_t i = 0; i < params.size(); ++i) {
            std::visit([&](auto&& arg) {
                using T = std::decay_t<decltype(arg)>;
                if constexpr (std::is_same_v<T, int>)
                    stmt->bind_param(i, arg);
                else if constexpr (std::is_same_v<T, double>)
                    stmt->bind_param(i, arg);
                else if constexpr (std::is_same_v<T, std::string>)
                    stmt->bind_param(i, arg);
                else if constexpr (std::is_same_v<T, std::vector<unsigned char>>)
                    stmt->bind_param(i, arg);
            }, params[i]);
        }
        
        // Execute the prepared statement
        auto result = query_processor_->execute_prepared_statement(stmt);
        LOG_INFO("Executed prepared statement");
        return result;
    } catch (const std::exception& e) {
        LOG_ERROR("Failed to execute prepared statement: " + std::string(e.what()));
        QueryResult error_result;
        error_result.error = "Failed to execute prepared statement: " + std::string(e.what());
        return error_result;
    }
}

void NexusDB::set_query_cache_size(size_t size) {
    query_cache_ = std::make_unique<QueryCache>(size);
    LOG_INFO("Query cache size set to " + std::to_string(size));
}

void NexusDB::enable_encryption(const EncryptionKey& key) {
    encryptor_ = std::make_unique<Encryptor>(key);
    LOG_INFO("Encryption enabled");
}

bool NexusDB::check_authentication() {
    if (!is_authenticated_) {
        LOG_ERROR("Not authenticated. Please login first.");
        return false;
    }
    return true;
}

bool NexusDB::check_table_ownership(const std::string& table_name) {
    auto user_tables = system_manager_->get_user_tables(current_user_);
    if (!user_tables.has_value() || std::find(user_tables.value().begin(), user_tables.value().end(), table_name) == user_tables.value().end()) {
        LOG_ERROR("User " + current_user_ + " does not have permission to access table " + table_name);
        return false;
    }
    return true;
}

} // namespace nexusdb