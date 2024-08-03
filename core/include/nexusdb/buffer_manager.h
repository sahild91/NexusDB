#ifndef NEXUSDB_BUFFER_MANAGER_H
#define NEXUSDB_BUFFER_MANAGER_H

#include <vector>
#include <unordered_map>
#include <cstddef>
#include <optional>
#include <mutex>
#include <memory>
#include "nexusdb/page.h"

namespace nexusdb {

struct BufferConfig {
    size_t initial_size = 0;  // 0 means auto-detect based on system memory
    float memory_usage_fraction = 0.25;  // Use 25% of available memory by default
    bool distributed_mode = false;
};

class BufferManager {
public:
    explicit BufferManager(const BufferConfig& config = BufferConfig());
    ~BufferManager();

    std::optional<std::string> initialize();
    void shutdown();

    size_t get_buffer_size() const;
    std::optional<std::string> resize_buffer(size_t new_size);

    // Page management methods
    std::shared_ptr<Page> get_page(const std::string& table_name, uint64_t page_id);
    void release_page(const std::string& table_name, uint64_t page_id);
    void flush_page(const std::string& table_name, uint64_t page_id);
    void flush_all_pages();

    // New methods for distributed operations
    void invalidate_page(const std::string& table_name, uint64_t page_id);
    void prefetch_pages(const std::string& table_name, const std::vector<uint64_t>& page_ids);

private:
    struct CacheEntry {
        std::shared_ptr<Page> page;
        bool is_dirty;
        size_t last_access_time;
    };

    BufferConfig config_;
    std::unordered_map<std::string, std::unordered_map<uint64_t, CacheEntry>> buffer_;
    mutable std::mutex mutex_;
    size_t current_size_;
    size_t access_counter_;

    size_t determine_buffer_size() const;
    void evict_page();
    void write_page_to_disk(const std::string& table_name, uint64_t page_id, const Page& page);
    std::shared_ptr<Page> read_page_from_disk(const std::string& table_name, uint64_t page_id);
};

} // namespace nexusdb

#endif // NEXUSDB_BUFFER_MANAGER_H