#include "nexusdb/system_manager.h"
#include "nexusdb/storage_engine.h"
#include "nexusdb/utils/logger.h"
#include <openssl/sha.h>
#include <sstream>
#include <iomanip>

namespace nexusdb {

SystemManager::SystemManager(std::shared_ptr<StorageEngine> storage_engine)
    : storage_engine_(storage_engine) {}

SystemManager::~SystemManager() {
    shutdown();
}

std::optional<std::string> SystemManager::initialize() {
    LOG_INFO("Initializing SystemManager...");
    return create_system_tables();
}

void SystemManager::shutdown() {
    LOG_INFO("Shutting down SystemManager...");
}

std::optional<std::string> SystemManager::create_system_tables() {
    std::vector<std::string> users_schema = {"username TEXT", "password_hash TEXT"};
    auto result = storage_engine_->create_table(USERS_TABLE, users_schema);
    if (result.has_value()) {
        LOG_ERROR("Failed to create system users table: " + result.value());
        return result;
    }

    std::vector<std::string> user_tables_schema = {"username TEXT", "table_name TEXT"};
    result = storage_engine_->create_table(USER_TABLES_TABLE, user_tables_schema);
    if (result.has_value()) {
        LOG_ERROR("Failed to create system user tables table: " + result.value());
        return result;
    }

    return std::nullopt;
}

std::optional<std::string> SystemManager::create_user(const std::string& username, const std::string& password) {
    std::lock_guard<std::mutex> lock(mutex_);
    std::string password_hash = hash_password(password);
    std::vector<std::string> user_record = {username, password_hash};
    return storage_engine_->insert_record(USERS_TABLE, user_record);
}

std::optional<std::string> SystemManager::authenticate_user(const std::string& username, const std::string& password) {
    std::lock_guard<std::mutex> lock(mutex_);
    std::vector<std::string> record;
    // Assuming the first record with matching username is the correct one
    auto result = storage_engine_->read_record(USERS_TABLE, 0, record);
    if (!result.has_value()) {
        if (record[0] == username && record[1] == hash_password(password)) {
            return std::nullopt; // Authentication successful
        }
    }
    return "Authentication failed";
}

std::optional<std::string> SystemManager::add_user_table(const std::string& username, const std::string& table_name) {
    std::lock_guard<std::mutex> lock(mutex_);
    std::vector<std::string> record = {username, table_name};
    return storage_engine_->insert_record(USER_TABLES_TABLE, record);
}

std::optional<std::string> SystemManager::remove_user_table(const std::string& username, const std::string& table_name) {
    std::lock_guard<std::mutex> lock(mutex_);
    // This is a simplified implementation. In a real system, you'd need to find the correct record ID.
    // For now, we'll assume the first matching record is the one we want to delete.
    std::vector<std::string> record;
    uint64_t record_id = 0;
    while (true) {
        auto result = storage_engine_->read_record(USER_TABLES_TABLE, record_id, record);
        if (result.has_value()) {
            // No more records or error occurred
            return "Table not found for user";
        }
        if (record[0] == username && record[1] == table_name) {
            return storage_engine_->delete_record(USER_TABLES_TABLE, record_id);
        }
        record_id++;
    }
}

std::optional<std::vector<std::string>> SystemManager::get_user_tables(const std::string& username) {
    std::lock_guard<std::mutex> lock(mutex_);
    std::vector<std::string> user_tables;
    std::vector<std::string> record;
    uint64_t record_id = 0;
    while (true) {
        auto result = storage_engine_->read_record(USER_TABLES_TABLE, record_id, record);
        if (result.has_value()) {
            // No more records or error occurred
            break;
        }
        if (record[0] == username) {
            user_tables.push_back(record[1]);
        }
        record_id++;
    }
    return user_tables;
}

std::string SystemManager::hash_password(const std::string& password) const {
    unsigned char hash[SHA256_DIGEST_LENGTH];
    SHA256_CTX sha256;
    SHA256_Init(&sha256);
    SHA256_Update(&sha256, password.c_str(), password.length());
    SHA256_Final(hash, &sha256);
    std::stringstream ss;
    for (int i = 0; i < SHA256_DIGEST_LENGTH; i++) {
        ss << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(hash[i]);
    }
    return ss.str();
}

} // namespace nexusdb