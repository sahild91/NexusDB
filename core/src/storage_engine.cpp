#include "nexusdb/storage_engine.h"
#include "nexusdb/file_manager.h"
#include "nexusdb/page.h"
#include "nexusdb/utils/logger.h"
#include <stdexcept>
#include <sstream>
#include <algorithm>
#include <numeric>

namespace nexusdb {

StorageEngine::StorageEngine(const StorageConfig& config) : config_(config), file_manager_(nullptr) {}

StorageEngine::~StorageEngine() {
    shutdown();
}

std::optional<std::string> StorageEngine::initialize(const std::string& data_directory) {
    try {
        std::lock_guard<std::mutex> lock(mutex_);
        data_directory_ = data_directory;
        file_manager_ = std::make_unique<FileManager>(data_directory_);
        return std::nullopt;
    } catch (const std::exception& e) {
        LOG_ERROR("Failed to initialize StorageEngine: " + std::string(e.what()));
        return "Failed to initialize StorageEngine: " + std::string(e.what());
    }
}

void StorageEngine::shutdown() {
    std::lock_guard<std::mutex> lock(mutex_);
    for (const auto& [table_name, file_name] : table_files_) {
        file_manager_->close_file(file_name);
    }
    table_files_.clear();
    file_manager_.reset();
}

std::optional<std::string> StorageEngine::create_table(const std::string& table_name, const std::vector<std::string>& schema) {
    std::lock_guard<std::mutex> lock(mutex_);
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

    return write_page(table_name, *page);
}

std::optional<std::string> StorageEngine::delete_table(const std::string& table_name) {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = table_files_.find(table_name);
    if (it == table_files_.end()) {
        return "Table doesn't exist";
    }

    file_manager_->close_file(it->second);
    // Here we should also delete the file, but that's not implemented in our FileManager yet
    // For now, we'll just remove it from our map
    table_files_.erase(it);
    return std::nullopt;
}

std::optional<std::string> StorageEngine::insert_record(const std::string& table_name, const std::vector<std::string>& record) {
    std::lock_guard<std::mutex> lock(mutex_);
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
            return write_page(table_name, *page);
        }

        ++page_id;
    }
}

std::optional<std::string> StorageEngine::read_record(const std::string& table_name, uint64_t record_id, std::vector<std::string>& record) const {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = table_files_.find(table_name);
    if (it == table_files_.end()) {
        return "Table doesn't exist";
    }

    uint64_t page_id = 1; // Start from the second page (first page is for schema)
    size_t records_scanned = 0;
    while (true) {
        auto page = read_page(table_name, page_id);
        if (!page) {
            return "Record not found";
        }

        size_t offset = 0;
        while (offset < config_.page_size) {
            std::vector<char> record_data = page->get_record(offset);
            if (record_data.empty()) {
                break;  // No more records in this page
            }

            if (records_scanned == record_id) {
                // Found the record we're looking for
                std::istringstream record_stream(std::string(record_data.begin(), record_data.end()));
                std::string field;
                record.clear();
                while (std::getline(record_stream, field)) {
                    record.push_back(field);
                }
                return std::nullopt;
            }

            ++records_scanned;
            offset += sizeof(size_t) + record_data.size();
        }

        ++page_id;
    }
}

std::optional<std::string> StorageEngine::update_record(const std::string& table_name, uint64_t record_id, const std::vector<std::string>& new_record) {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = table_files_.find(table_name);
    if (it == table_files_.end()) {
        return "Table doesn't exist";
    }

    std::string record_str = std::accumulate(new_record.begin(), new_record.end(), std::string(),
        [](const std::string& a, const std::string& b) { return a + (a.empty() ? "" : "\n") + b; });
    std::vector<char> new_record_data(record_str.begin(), record_str.end());

    uint64_t page_id = 1; // Start from the second page (first page is for schema)
    size_t records_scanned = 0;
    while (true) {
        auto page = read_page(table_name, page_id);
        if (!page) {
            return "Record not found";
        }

        size_t offset = 0;
        while (offset < config_.page_size) {
            std::vector<char> record_data = page->get_record(offset);
            if (record_data.empty()) {
                break;  // No more records in this page
            }

            if (records_scanned == record_id) {
                // Found the record we're looking for
                if (page->update_record(offset, new_record_data)) {
                    return write_page(table_name, *page);
                } else {
                    // If update fails (e.g., not enough space), we need to delete and re-insert
                    if (page->delete_record(offset) && write_page(table_name, *page)) {
                        return insert_record(table_name, new_record);
                    } else {
                        return "Failed to update record";
                    }
                }
            }

            ++records_scanned;
            offset += sizeof(size_t) + record_data.size();
        }

        ++page_id;
    }
}

std::optional<std::string> StorageEngine::delete_record(const std::string& table_name, uint64_t record_id) {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = table_files_.find(table_name);
    if (it == table_files_.end()) {
        return "Table doesn't exist";
    }

    uint64_t page_id = 1; // Start from the second page (first page is for schema)
    size_t records_scanned = 0;
    while (true) {
        auto page = read_page(table_name, page_id);
        if (!page) {
            return "Record not found";
        }

        size_t offset = 0;
        while (offset < config_.page_size) {
            std::vector<char> record_data = page->get_record(offset);
            if (record_data.empty()) {
                break;  // No more records in this page
            }

            if (records_scanned == record_id) {
                // Found the record we're looking for
                if (page->delete_record(offset)) {
                    return write_page(table_name, *page);
                } else {
                    return "Failed to delete record";
                }
            }

            ++records_scanned;
            offset += sizeof(size_t) + record_data.size();
        }

        ++page_id;
    }
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

    if (!file_manager_->write_page(it->second, page)) {
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

    return file_manager_->read_page(it->second, page_id);
}

} // namespace nexusdb