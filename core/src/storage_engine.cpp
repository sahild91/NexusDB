#include "nexusdb/storage_engine.h"
#include "nexusdb/utils/logger.h"
#include <sstream>
#include <algorithm>

namespace nexusdb {

StorageEngine::StorageEngine(const StorageConfig& config) 
    : config_(config), consistency_level_(ConsistencyLevel::ONE) {
    LOG_DEBUG("StorageEngine constructor called");
}

StorageEngine::~StorageEngine() {
    LOG_DEBUG("StorageEngine destructor called");
    shutdown();
}

std::optional<std::string> StorageEngine::initialize(const std::string& data_directory) {
    try {
        std::lock_guard<std::mutex> lock(mutex_);
        LOG_INFO("Initializing StorageEngine...");
        data_directory_ = data_directory;
        file_manager_ = std::make_unique<FileManager>(data_directory_);
        
        index_manager_ = std::make_shared<IndexManager>(shared_from_this());
        auto index_init_result = index_manager_->initialize();
        if (index_init_result.has_value()) {
            return index_init_result;
        }

        recovery_manager_ = std::make_shared<RecoveryManager>(shared_from_this());
        auto recovery_init_result = recovery_manager_->initialize(data_directory_ + "/recovery.log");
        if (recovery_init_result.has_value()) {
            return recovery_init_result;
        }

        LOG_INFO("StorageEngine initialized successfully");
        return std::nullopt;
    } catch (const std::exception& e) {
        LOG_ERROR("Failed to initialize StorageEngine: " + std::string(e.what()));
        return "Failed to initialize StorageEngine: " + std::string(e.what());
    }
}

void StorageEngine::shutdown() {
    std::lock_guard<std::mutex> lock(mutex_);
    LOG_INFO("Shutting down StorageEngine...");
    for (const auto& [table_name, file_name] : table_files_) {
        file_manager_->close_file(file_name);
    }
    table_files_.clear();
    file_manager_.reset();
    index_manager_->shutdown();
    recovery_manager_->shutdown();
    LOG_INFO("StorageEngine shut down successfully");
}

std::optional<std::string> StorageEngine::create_table(const std::string& table_name, const std::vector<std::string>& schema) {
    std::lock_guard<std::mutex> lock(mutex_);
    LOG_INFO("Creating table: " + table_name);
    std::string file_name = get_table_file_name(table_name);
    if (table_files_.find(table_name) != table_files_.end()) {
        return "Table already exists";
    }

    if (!file_manager_->create_file(file_name)) {
        return "Failed to create file for table";
    }

    table_files_[table_name] = file_name;

    auto page = allocate_page(table_name);
    if (!page) {
        return "Failed to allocate page for schema";
    }

    std::string schema_str = std::accumulate(schema.begin(), schema.end(), std::string(),
        [](const std::string& a, const std::string& b) { return a + (a.empty() ? "" : "\n") + b; });

    if (page->add_record(std::vector<char>(schema_str.begin(), schema_str.end())) == -1) {
        return "Failed to add schema to page";
    }

    if (config_.use_compression) {
        page->compress();
    }

    auto write_result = write_page(table_name, *page);
    if (write_result.has_value()) {
        return write_result;
    }

    LOG_INFO("Table created successfully: " + table_name);
    return std::nullopt;
}

std::optional<std::string> StorageEngine::delete_table(const std::string& table_name) {
    std::lock_guard<std::mutex> lock(mutex_);
    LOG_INFO("Deleting table: " + table_name);
    auto it = table_files_.find(table_name);
    if (it == table_files_.end()) {
        return "Table doesn't exist";
    }

    file_manager_->close_file(it->second);
    // Here we should also delete the file, but that's not implemented in our FileManager yet
    // For now, we'll just remove it from our map
    table_files_.erase(it);

    // Remove all indexes for this table
    index_manager_->drop_all_indexes(table_name);

    LOG_INFO("Table deleted successfully: " + table_name);
    return std::nullopt;
}

std::optional<std::string> StorageEngine::insert_record(const std::string& table_name, const std::vector<std::string>& record) {
    std::lock_guard<std::mutex> lock(mutex_);
    LOG_INFO("Inserting record into table: " + table_name);
    auto it = table_files_.find(table_name);
    if (it == table_files_.end()) {
        return "Table doesn't exist";
    }

    std::string record_str = std::accumulate(record.begin(), record.end(), std::string(),
        [](const std::string& a, const std::string& b) { return a + (a.empty() ? "" : "\n") + b; });
    std::vector<char> record_data(record_str.begin(), record_str.end());

    uint64_t page_id = 1; // Start from the second page (first page is for schema)
    while (true) {
        auto page = read_page(table_name, page_id);
        if (!page) {
            page = allocate_page(table_name);
            if (!page) {
                return "Failed to allocate new page";
            }
        }

        int offset = page->add_record(record_data);
        if (offset != -1) {
            if (config_.use_compression) {
                page->compress();
            }
            auto write_result = write_page(table_name, *page);
            if (write_result.has_value()) {
                return write_result;
            }
            
            // Update indexes
            uint64_t record_id = (page_id - 1) * (Page::PAGE_SIZE / sizeof(uint64_t)) + offset;
            update_indexes(table_name, record, record_id);

            // Log the operation
            LogRecord log_record{
                LogRecordType::INSERT,
                0, // transaction_id (0 for now, as we're not handling transactions in this example)
                table_name,
                record_id,
                "", // before_image (empty for insert)
                record_str // after_image
            };
            recovery_manager_->log_operation(log_record);

            LOG_INFO("Record inserted successfully into table: " + table_name);
            return std::nullopt;
        }

        ++page_id;
    }
}

std::optional<std::string> StorageEngine::read_record(const std::string& table_name, uint64_t record_id, std::vector<std::string>& record) const {
    std::lock_guard<std::mutex> lock(mutex_);
    LOG_INFO("Reading record from table: " + table_name + ", record_id: " + std::to_string(record_id));
    auto it = table_files_.find(table_name);
    if (it == table_files_.end()) {
        return "Table doesn't exist";
    }

    uint64_t page_id = record_id / (Page::PAGE_SIZE / sizeof(uint64_t)) + 1;
    uint64_t offset = record_id % (Page::PAGE_SIZE / sizeof(uint64_t));

    auto page = read_page(table_name, page_id);
    if (!page) {
        return "Record not found";
    }

    std::vector<char> record_data = page->get_record(offset);
    if (record_data.empty()) {
        return "Record not found";
    }

    std::istringstream record_stream(std::string(record_data.begin(), record_data.end()));
    std::string field;
    record.clear();
    while (std::getline(record_stream, field)) {
        record.push_back(field);
    }

    LOG_INFO("Record read successfully from table: " + table_name);
    return std::nullopt;
}

std::optional<std::string> StorageEngine::update_record(const std::string& table_name, uint64_t record_id, const std::vector<std::string>& new_record) {
    std::lock_guard<std::mutex> lock(mutex_);
    LOG_INFO("Updating record in table: " + table_name + ", record_id: " + std::to_string(record_id));
    auto it = table_files_.find(table_name);
    if (it == table_files_.end()) {
        return "Table doesn't exist";
    }

    uint64_t page_id = record_id / (Page::PAGE_SIZE / sizeof(uint64_t)) + 1;
    uint64_t offset = record_id % (Page::PAGE_SIZE / sizeof(uint64_t));

    auto page = read_page(table_name, page_id);
    if (!page) {
        return "Record not found";
    }

    std::vector<char> old_record_data = page->get_record(offset);
    if (old_record_data.empty()) {
        return "Record not found";
    }

    std::string new_record_str = std::accumulate(new_record.begin(), new_record.end(), std::string(),
        [](const std::string& a, const std::string& b) { return a + (a.empty() ? "" : "\n") + b; });
    std::vector<char> new_record_data(new_record_str.begin(), new_record_str.end());

    if (page->update_record(offset, new_record_data)) {
        if (config_.use_compression) {
            page->compress();
        }
        auto write_result = write_page(table_name, *page);
        if (write_result.has_value()) {
            return write_result;
        }

        // Update indexes
        std::vector<std::string> old_record;
        std::istringstream old_record_stream(std::string(old_record_data.begin(), old_record_data.end()));
        std::string field;
        while (std::getline(old_record_stream, field)) {
            old_record.push_back(field);
        }
        remove_from_indexes(table_name, old_record, record_id);
        update_indexes(table_name, new_record, record_id);

        // Log the operation
        LogRecord log_record{
            LogRecordType::UPDATE,
            0, // transaction_id (0 for now, as we're not handling transactions in this example)
            table_name,
            record_id,
            std::string(old_record_data.begin(), old_record_data.end()), // before_image
            new_record_str // after_image
        };
        recovery_manager_->log_operation(log_record);

        LOG_INFO("Record updated successfully in table: " + table_name);
        return std::nullopt;
    } else {
        return "Failed to update record";
    }
}

std::optional<std::string> StorageEngine::delete_record(const std::string& table_name, uint64_t record_id) {
    std::lock_guard<std::mutex> lock(mutex_);
    LOG_INFO("Deleting record from table: " + table_name + ", record_id: " + std::to_string(record_id));
    auto it = table_files_.find(table_name);
    if (it == table_files_.end()) {
        return "Table doesn't exist";
    }

    uint64_t page_id = record_id / (Page::PAGE_SIZE / sizeof(uint64_t)) + 1;
    uint64_t offset = record_id % (Page::PAGE_SIZE / sizeof(uint64_t));

    auto page = read_page(table_name, page_id);
    if (!page) {
        return "Record not found";
    }

    std::vector<char> record_data = page->get_record(offset);
    if (record_data.empty()) {
        return "Record not found";
    }

    if (page->delete_record(offset)) {
        if (config_.use_compression) {
            page->compress();
        }
        auto write_result = write_page(table_name, *page);
        if (write_result.has_value()) {
            return write_result;
        }

        // Remove from indexes
        std::vector<std::string> old_record;
        std::istringstream old_record_stream(std::string(record_data.begin(), record_data.end()));
        std::string field;
        while (std::getline(old_record_stream, field)) {
            old_record.push_back(field);
        }
        remove_from_indexes(table_name, old_record, record_id);

        // Log the operation
        LogRecord log_record{
            LogRecordType::DELETE,
            0, // transaction_id (0 for now, as we're not handling transactions in this example)
            table_name,
            record_id,
            std::string(record_data.begin(), record_data.end()), // before_image
            "" // after_image (empty for delete)
        };
        recovery_manager_->log_operation(log_record);

        LOG_INFO("Record deleted successfully from table: " + table_name);
        return std::nullopt;
    } else {
        return "Failed to delete record";
    }
}

std::optional<std::vector<std::string>> StorageEngine::get_table_schema(const std::string& table_name) const {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = table_files_.find(table_name);
    if (it == table_files_.end()) {
        return std::nullopt;
    }

    auto page = read_page(table_name, 0);  // Schema is stored in the first page
    if (!page) {
        return std::nullopt;
    }

    std::vector<char> schema_data = page->get_record(0);
    if (schema_data.empty()) {
        return std::nullopt;
    }

    std::vector<std::string> schema;
    std::istringstream schema_stream(std::string(schema_data.begin(), schema_data.end()));
    std::string field;
    while (std::getline(schema_stream, field)) {
        schema.push_back(field);
    }

    return schema;
}

std::optional<std::string> StorageEngine::create_index(const std::string& table_name, const std::string& column_name) {
    std::lock_guard<std::mutex> lock(mutex_);
    LOG_INFO("Creating index on table: " + table_name + ", column: " + column_name);

    auto schema = get_table_schema(table_name);
    if (!schema.has_value()) {
        return "Table doesn't exist or schema not found";
    }

    size_t column_index = std::distance(schema->begin(), std::find(schema->begin(), schema->end(), column_name));
    if (column_index == schema->size()) {
        return "Column not found in table schema";
    }

    auto result = index_manager_->create_index(table_name, column_name);
    if (result.has_value()) {
        return result;
    }

    // Populate the index with existing data
    uint64_t page_id = 1;  // Start from the second page (first page is for schema)
    while (true) {
        auto page = read_page(table_name, page_id);
        if (!page) {
            break;  // No more pages
        }

        size_t offset = 0;
        while (offset < Page::PAGE_SIZE) {
            std::vector<char> record_data = page->get_record(offset);
            if (record_data.empty()) {
                break;  // No more records in this page
            }

            std::vector<std::string> record;
            std::istringstream record_stream(std::string(record_data.begin(), record_data.end()));
            std::string field;
            while (std::getline(record_stream, field)) {
                record.push_back(field);
            }

            if (column_index < record.size()) {
                uint64_t record_id = (page_id - 1) * (Page::PAGE_SIZE / sizeof(uint64_t)) + offset;
                index_manager_->insert_into_index(table_name, column_name, record[column_index], record_id);
            }

            offset += sizeof(size_t) + record_data.size();
        }

        ++page_id;
    }

    LOG_INFO("Index created successfully on table: " + table_name + ", column: " + column_name);
    return std::nullopt;
}

std::optional<std::string> StorageEngine::drop_index(const std::string& table_name, const std::string& column_name) {
    std::lock_guard<std::mutex> lock(mutex_);
    LOG_INFO("Dropping index on table: " + table_name + ", column: " + column_name);

    auto result = index_manager_->drop_index(table_name, column_name);
    if (result.has_value()) {
        return result;
    }

    LOG_INFO("Index dropped successfully on table: " + table_name + ", column: " + column_name);
    return std::nullopt;
}

std::optional<std::vector<uint64_t>> StorageEngine::search_index(const std::string& table_name, const std::string& column_name, const std::string& value) const {
    std::lock_guard<std::mutex> lock(mutex_);
    LOG_INFO("Searching index on table: " + table_name + ", column: " + column_name + ", value: " + value);

    return index_manager_->search_index(table_name, column_name, value);
}

std::optional<std::string> StorageEngine::begin_transaction() {
    return recovery_manager_->begin_transaction(0);  // Using 0 as a placeholder for transaction ID
}

std::optional<std::string> StorageEngine::commit_transaction() {
    return recovery_manager_->commit_transaction(0);  // Using 0 as a placeholder for transaction ID
}

std::optional<std::string> StorageEngine::abort_transaction() {
    return recovery_manager_->abort_transaction(0);  // Using 0 as a placeholder for transaction ID
}

std::optional<std::string> StorageEngine::recover() {
    std::lock_guard<std::mutex> lock(mutex_);
    LOG_INFO("Starting recovery process...");

    auto result = recovery_manager_->recover();
    if (result.has_value()) {
        LOG_ERROR("Recovery failed: " + result.value());
        return result;
    }

    LOG_INFO("Recovery process completed successfully");
    return std::nullopt;
}

void StorageEngine::enable_encryption(const EncryptionKey& key) {
    std::lock_guard<std::mutex> lock(mutex_);
    encryptor_ = std::make_unique<Encryptor>(key);
    config_.use_encryption = true;
    LOG_INFO("Encryption enabled");
}

void StorageEngine::disable_encryption() {
    std::lock_guard<std::mutex> lock(mutex_);
    encryptor_.reset();
    config_.use_encryption = false;
    LOG_INFO("Encryption disabled");
}

std::optional<std::string> StorageEngine::add_node(const std::string& node_address, uint32_t port) {
    // This method should be overridden in DistributedStorageEngine
    return "Not implemented in non-distributed mode";
}

std::optional<std::string> StorageEngine::remove_node(const std::string& node_address) {
    // This method should be overridden in DistributedStorageEngine
    return "Not implemented in non-distributed mode";
}

void StorageEngine::set_consistency_level(ConsistencyLevel level) {
    std::lock_guard<std::mutex> lock(mutex_);
    consistency_level_ = level;
    LOG_INFO("Consistency level set to: " + std::to_string(static_cast<int>(level)));
}

std::string StorageEngine::get_table_file_name(const std::string& table_name) const {
    return table_name + ".db";
}

std::unique_ptr<Page> StorageEngine::allocate_page(const std::string& table_name) {
    auto it = table_files_.find(table_name);
    if (it == table_files_.end()) {
        LOG_ERROR("Table doesn't exist");
        return nullptr;
    }

    return file_manager_->allocate_page(it->second);
}

std::optional<std::string> StorageEngine::write_page(const std::string& table_name, const Page& page) {
    auto it = table_files_.find(table_name);
    if (it == table_files_.end()) {
        return "Table doesn't exist";
    }

    std::vector<unsigned char> page_data(page.get_data(), page.get_data() + Page::PAGE_SIZE);
    
    if (config_.use_encryption) {
        page_data = encrypt_page(page_data);
    }

    if (!file_manager_->write_page(it->second, Page(page.get_page_id(), page_data.data()))) {
        return "Failed to write page";
    }

    return std::nullopt;
}

std::unique_ptr<Page> StorageEngine::read_page(const std::string& table_name, uint64_t page_id) const {
    auto it = table_files_.find(table_name);
    if (it == table_files_.end()) {
        LOG_ERROR("Table doesn't exist");
        return nullptr;
    }

    auto page = file_manager_->read_page(it->second, page_id);
    if (!page) {
        return nullptr;
    }

    if (config_.use_encryption) {
        std::vector<unsigned char> decrypted_data = decrypt_page(std::vector<unsigned char>(page->get_data(), page->get_data() + Page::PAGE_SIZE));
        return std::make_unique<Page>(page_id, decrypted_data.data());
    }

    if (config_.use_compression) {
        page->decompress();
    }
    return page;
}

void StorageEngine::update_indexes(const std::string& table_name, const std::vector<std::string>& record, uint64_t record_id) {
    for (size_t i = 0; i < record.size(); ++i) {
        index_manager_->insert_into_index(table_name, std::to_string(i), record[i], record_id);
    }
}

void StorageEngine::remove_from_indexes(const std::string& table_name, const std::vector<std::string>& record, uint64_t record_id) {
    for (size_t i = 0; i < record.size(); ++i) {
        index_manager_->remove_from_index(table_name, std::to_string(i), record[i], record_id);
    }
}

std::vector<unsigned char> StorageEngine::encrypt_page(const std::vector<unsigned char>& page_data) const {
    if (!encryptor_) {
        LOG_ERROR("Encryption is enabled but encryptor is not initialized");
        return page_data;
    }
    return encryptor_->encrypt(page_data);
}

std::vector<unsigned char> StorageEngine::decrypt_page(const std::vector<unsigned char>& encrypted_data) const {
    if (!encryptor_) {
        LOG_ERROR("Encryption is enabled but encryptor is not initialized");
        return encrypted_data;
    }
    return encryptor_->decrypt(encrypted_data);
}

// Utility method to check if a table exists
bool StorageEngine::table_exists(const std::string& table_name) const {
    return table_files_.find(table_name) != table_files_.end();
}

// Utility method to get the number of pages in a table
uint64_t StorageEngine::get_page_count(const std::string& table_name) const {
    if (!table_exists(table_name)) {
        LOG_ERROR("Table does not exist: " + table_name);
        return 0;
    }

    const std::string& file_name = table_files_.at(table_name);
    return file_manager_->get_page_count(file_name);
}

// Method to perform a full table scan (useful for queries without indexes)
std::optional<std::vector<std::pair<uint64_t, std::vector<std::string>>>> StorageEngine::full_table_scan(const std::string& table_name) const {
    std::lock_guard<std::mutex> lock(mutex_);
    if (!table_exists(table_name)) {
        LOG_ERROR("Table does not exist: " + table_name);
        return std::nullopt;
    }

    std::vector<std::pair<uint64_t, std::vector<std::string>>> results;
    uint64_t page_count = get_page_count(table_name);

    for (uint64_t page_id = 1; page_id < page_count; ++page_id) { // Start from 1 as 0 is schema page
        auto page = read_page(table_name, page_id);
        if (!page) {
            LOG_ERROR("Failed to read page " + std::to_string(page_id) + " from table " + table_name);
            continue;
        }

        size_t offset = 0;
        while (offset < Page::PAGE_SIZE) {
            std::vector<char> record_data = page->get_record(offset);
            if (record_data.empty()) {
                break;  // No more records in this page
            }

            std::vector<std::string> record;
            std::istringstream record_stream(std::string(record_data.begin(), record_data.end()));
            std::string field;
            while (std::getline(record_stream, field)) {
                record.push_back(field);
            }

            uint64_t record_id = (page_id - 1) * (Page::PAGE_SIZE / sizeof(uint64_t)) + offset;
            results.emplace_back(record_id, std::move(record));

            offset += sizeof(size_t) + record_data.size();
        }
    }

    return results;
}

// Method to get database statistics
std::optional<DatabaseStats> StorageEngine::get_database_stats() const {
    std::lock_guard<std::mutex> lock(mutex_);
    DatabaseStats stats;

    stats.table_count = table_files_.size();
    stats.total_pages = 0;
    stats.total_records = 0;

    for (const auto& [table_name, file_name] : table_files_) {
        TableStats table_stats;
        table_stats.name = table_name;
        table_stats.page_count = get_page_count(table_name);
        stats.total_pages += table_stats.page_count;

        auto scan_result = full_table_scan(table_name);
        if (scan_result) {
            table_stats.record_count = scan_result->size();
            stats.total_records += table_stats.record_count;
        }

        stats.table_stats.push_back(std::move(table_stats));
    }

    return stats;
}

// Method to compact a table (remove deleted records and defragment pages)
std::optional<std::string> StorageEngine::compact_table(const std::string& table_name) {
    std::lock_guard<std::mutex> lock(mutex_);
    if (!table_exists(table_name)) {
        return "Table does not exist: " + table_name;
    }

    LOG_INFO("Starting table compaction for: " + table_name);

    std::vector<std::pair<uint64_t, std::vector<std::string>>> valid_records;
    auto scan_result = full_table_scan(table_name);
    if (!scan_result) {
        return "Failed to scan table for compaction";
    }
    valid_records = std::move(*scan_result);

    // Create a new file for the compacted table
    std::string compact_file_name = table_name + "_compact.db";
    if (!file_manager_->create_file(compact_file_name)) {
        return "Failed to create compact file";
    }

    // Write schema page
    auto schema_page = read_page(table_name, 0);
    if (!schema_page) {
        return "Failed to read schema page";
    }
    if (!file_manager_->write_page(compact_file_name, *schema_page)) {
        return "Failed to write schema page to compact file";
    }

    // Write compacted records
    uint64_t current_page_id = 1;
    std::unique_ptr<Page> current_page = std::make_unique<Page>(current_page_id);
    for (const auto& [old_record_id, record] : valid_records) {
        std::string record_str = std::accumulate(record.begin(), record.end(), std::string(),
            [](const std::string& a, const std::string& b) { return a + (a.empty() ? "" : "\n") + b; });
        std::vector<char> record_data(record_str.begin(), record_str.end());

        if (current_page->add_record(record_data) == -1) {
            // Page is full, write it and create a new one
            if (!file_manager_->write_page(compact_file_name, *current_page)) {
                return "Failed to write page during compaction";
            }
            current_page_id++;
            current_page = std::make_unique<Page>(current_page_id);
            if (current_page->add_record(record_data) == -1) {
                return "Failed to add record to new page during compaction";
            }
        }
    }

    // Write the last page if it's not empty
    if (current_page->get_free_space() < Page::PAGE_SIZE) {
        if (!file_manager_->write_page(compact_file_name, *current_page)) {
            return "Failed to write last page during compaction";
        }
    }

    // Replace the old file with the new compact file
    file_manager_->close_file(table_files_[table_name]);
    file_manager_->delete_file(table_files_[table_name]);
    file_manager_->rename_file(compact_file_name, table_files_[table_name]);

    // Rebuild indexes
    for (size_t i = 0; i < valid_records[0].second.size(); ++i) {
        index_manager_->drop_index(table_name, std::to_string(i));
        index_manager_->create_index(table_name, std::to_string(i));
        for (const auto& [record_id, record] : valid_records) {
            index_manager_->insert_into_index(table_name, std::to_string(i), record[i], record_id);
        }
    }

    LOG_INFO("Table compaction completed for: " + table_name);
    return std::nullopt;
}

// Error handling method
void StorageEngine::handle_error(const std::string& error_message) {
    LOG_ERROR(error_message);
    // Here you could implement additional error handling logic,
    // such as notifying administrators or triggering a recovery process
}

} // namespace nexusdb