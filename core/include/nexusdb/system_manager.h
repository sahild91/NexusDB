#ifndef NEXUSDB_SYSTEM_MANAGER_H
#define NEXUSDB_SYSTEM_MANAGER_H

#include <string>
#include <vector>
#include <memory>
#include <optional>
#include <mutex>

namespace nexusdb {

class StorageEngine;

class SystemManager {
public:
    explicit SystemManager(std::shared_ptr<StorageEngine> storage_engine);
    ~SystemManager();

    std::optional<std::string> initialize();
    void shutdown();

    std::optional<std::string> create_user(const std::string& username, const std::string& password);
    std::optional<std::string> authenticate_user(const std::string& username, const std::string& password);
    std::optional<std::string> add_user_table(const std::string& username, const std::string& table_name);
    std::optional<std::string> remove_user_table(const std::string& username, const std::string& table_name);
    std::optional<std::vector<std::string>> get_user_tables(const std::string& username);

private:
    std::shared_ptr<StorageEngine> storage_engine_;
    mutable std::mutex mutex_;
    const std::string USERS_TABLE = "system_users";
    const std::string USER_TABLES_TABLE = "system_user_tables";

    std::optional<std::string> create_system_tables();
    std::string hash_password(const std::string& password) const;
};

} // namespace nexusdb

#endif // NEXUSDB_SYSTEM_MANAGER_H