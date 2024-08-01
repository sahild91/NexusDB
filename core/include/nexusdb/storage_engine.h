#ifndef NEXUSDB_STORAGE_ENGINE_H
#define NEXUSDB_STORAGE_ENGINE_H

#include <string>
#include <vector>
#include <unordered_map>
#include <cstdint>
#include <memory>
#include <mutex>
#include <optional>

namespace nexusdb {

// Forward declarations
class Page;
class FileManager;

// Configurable constants
struct StorageConfig {
    static constexpr size_t DEFAULT_PAGE_SIZE = 4096;
    size_t page_size = DEFAULT_PAGE_SIZE;
};

/// Storage Engine class
class StorageEngine {
public:
    /// Constructor
    explicit StorageEngine(const StorageConfig& config = StorageConfig());

    /// Destructor
    ~StorageEngine();

    /// Initialize the storage engine
    /// @param data_directory The directory where database files will be stored
    /// @return std::optional<std::string> containing an error message if initialization failed
    std::optional<std::string> initialize(const std::string& data_directory);

    /// Shutdown the storage engine
    void shutdown();

    /// Create a new table
    /// @param table_name The name of the table to create
    /// @param schema The schema of the table (simplified for now)
    /// @return std::optional<std::string> containing an error message if table creation failed
    std::optional<std::string> create_table(const std::string& table_name, const std::vector<std::string>& schema);

    /// Delete a table
    /// @param table_name The name of the table to delete
    /// @return std::optional<std::string> containing an error message if table deletion failed
    std::optional<std::string> delete_table(const std::string& table_name);

    /// Insert a record into a table
    /// @param table_name The name of the table to insert into
    /// @param record The record to insert (simplified for now)
    /// @return std::optional<std::string> containing an error message if insertion failed
    std::optional<std::string> insert_record(const std::string& table_name, const std::vector<std::string>& record);

    /// Read a record from a table
    /// @param table_name The name of the table to read from
    /// @param record_id The ID of the record to read
    /// @param record The record data will be stored here
    /// @return std::optional<std::string> containing an error message if reading failed
    std::optional<std::string> read_record(const std::string& table_name, uint64_t record_id, std::vector<std::string>& record) const;

    /// Update a record in a table
    /// @param table_name The name of the table to update
    /// @param record_id The ID of the record to update
    /// @param new_record The new record data
    /// @return std::optional<std::string> containing an error message if update failed
    std::optional<std::string> update_record(const std::string& table_name, uint64_t record_id, const std::vector<std::string>& new_record);

    /// Delete a record from a table
    /// @param table_name The name of the table to delete from
    /// @param record_id The ID of the record to delete
    /// @return std::optional<std::string> containing an error message if deletion failed
    std::optional<std::string> delete_record(const std::string& table_name, uint64_t record_id);

private:
    StorageConfig config_;
    std::string data_directory_;
    std::unique_ptr<FileManager> file_manager_;
    std::unordered_map<std::string, std::string> table_files_; // Maps table names to file names
    mutable std::mutex mutex_; // For thread-safety

    /// Get the file name for a table
    /// @param table_name The name of the table
    /// @return The file name for the table
    std::string get_table_file_name(const std::string& table_name) const;

    /// Allocate a new page for a table
    /// @param table_name The name of the table
    /// @return A pointer to the new page, or nullptr if allocation failed
    std::unique_ptr<Page> allocate_page(const std::string& table_name);

    /// Write a page to disk
    /// @param table_name The name of the table
    /// @param page The page to write
    /// @return std::optional<std::string> containing an error message if writing failed
    std::optional<std::string> write_page(const std::string& table_name, const Page& page);

    /// Read a page from disk
    /// @param table_name The name of the table
    /// @param page_id The ID of the page to read
    /// @return A pointer to the read page, or nullptr if reading failed
    std::unique_ptr<Page> read_page(const std::string& table_name, uint64_t page_id) const;
};

} // namespace nexusdb

#endif // NEXUSDB_STORAGE_ENGINE_H