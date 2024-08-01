#include "nexusdb/nexusdb.h"
#include "nexusdb/utils/logger.h"
#include <algorithm>

namespace nexusdb {

NexusDB::NexusDB() : is_authenticated_(false) {
    storage_engine_ = std::make_shared<StorageEngine>();
    buffer_manager_ = std::make_unique<BufferManager>();
    query_processor_ = std::make_unique<QueryProcessor>(storage_engine_);
    transaction_manager_ = std::make_unique<TransactionManager>();
    recovery_manager_ = std::make_unique<RecoveryManager>(storage_engine_);
    schema_manager_ = std::make_unique<SchemaManager>();
    index_manager_ = std::make_unique<IndexManager>(storage_engine_);
    concurrency_manager_ = std::make_unique<ConcurrencyManager>();
}

NexusDB::~NexusDB() {
    shutdown();
}

std::optional<std::string> NexusDB::initialize(const std::string& data_directory) {
    LOG_INFO("Initializing NexusDB...");
    
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

    // Create admin user if it doesn't exist
    auto create_admin_result = system_manager_->create_user("admin", "admin_password");
    if (create_admin_result.has_value()) {
        LOG_WARNING("Admin user already exists or failed to create: " + create_admin_result.value());
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
    return query_processor_->execute_query(query);
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