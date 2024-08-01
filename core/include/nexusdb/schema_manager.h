#ifndef NEXUSDB_SCHEMA_MANAGER_H
#define NEXUSDB_SCHEMA_MANAGER_H

#include <string>
#include <optional>
#include <mutex>
#include <unordered_map>
#include <vector>

namespace nexusdb {

struct ColumnDefinition {
    std::string name;
    std::string type;
    bool nullable;
};

struct TableSchema {
    std::string table_name;
    std::vector<ColumnDefinition> columns;
};

class SchemaManager {
public:
    SchemaManager();
    ~SchemaManager();

    std::optional<std::string> initialize();
    void shutdown();

    std::optional<std::string> create_table(const std::string& table_name, const std::vector<ColumnDefinition>& columns);
    std::optional<std::string> drop_table(const std::string& table_name);
    std::optional<TableSchema> get_table_schema(const std::string& table_name);

private:
    std::mutex mutex_;
    std::unordered_map<std::string, TableSchema> schemas_;
};

} // namespace nexusdb

#endif // NEXUSDB_SCHEMA_MANAGER_H