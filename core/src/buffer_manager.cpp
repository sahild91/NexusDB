#include "nexusdb/buffer_manager.h"
#include "nexusdb/utils/logger.h"
#include <stdexcept>
#include <algorithm>
#include <chrono>

#ifdef _WIN32
#include <windows.h>
#else
#include <unistd.h>
#endif

namespace nexusdb {

BufferManager::BufferManager(const BufferConfig& config) 
    : config_(config), current_size_(0), access_counter_(0) {
    LOG_DEBUG("BufferManager constructor called");
}

BufferManager::~BufferManager() {
    LOG_DEBUG("BufferManager destructor called");
    shutdown();
}

std::optional<std::string> BufferManager::initialize() {
    LOG_INFO("Initializing Buffer Manager...");
    try {
        std::lock_guard<std::mutex> lock(mutex_);
        size_t buffer_size = determine_buffer_size();
        current_size_ = 0;  // Start with 0 and grow as needed
        LOG_INFO("Buffer Manager initialized successfully with max size: " + std::to_string(buffer_size) + " bytes");
        return std::nullopt;
    } catch (const std::exception& e) {
        LOG_ERROR("Failed to initialize Buffer Manager: " + std::string(e.what()));
        return "Failed to initialize Buffer Manager: " + std::string(e.what());
    }
}

void BufferManager::shutdown() {
    LOG_INFO("Shutting down Buffer Manager...");
    std::lock_guard<std::mutex> lock(mutex_);
    flush_all_pages();
    buffer_.clear();
    current_size_ = 0;
    LOG_INFO("Buffer Manager shut down successfully");
}

size_t BufferManager::get_buffer_size() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return current_size_;
}

std::optional<std::string> BufferManager::resize_buffer(size_t new_size) {
    try {
        std::lock_guard<std::mutex> lock(mutex_);
        while (current_size_ > new_size) {
            evict_page();
        }
        LOG_INFO("Buffer resized to: " + std::to_string(new_size) + " bytes");
        return std::nullopt;
    } catch (const std::exception& e) {
        LOG_ERROR("Failed to resize buffer: " + std::string(e.what()));
        return "Failed to resize buffer: " + std::string(e.what());
    }
}

std::shared_ptr<Page> BufferManager::get_page(const std::string& table_name, uint64_t page_id) {
    std::lock_guard<std::mutex> lock(mutex_);
    auto& table_buffer = buffer_[table_name];
    auto it = table_buffer.find(page_id);

    if (it != table_buffer.end()) {
        // Page is in buffer
        it->second.last_access_time = ++access_counter_;
        return it->second.page;
    }

    // Page is not in buffer, need to load it
    auto page = read_page_from_disk(table_name, page_id);
    if (!page) {
        LOG_ERROR("Failed to read page from disk: " + table_name + ", page_id: " + std::to_string(page_id));
        return nullptr;
    }

    // If buffer is full, evict a page
    if (current_size_ >= determine_buffer_size()) {
        evict_page();
    }

    // Add new page to buffer
    table_buffer[page_id] = {page, false, ++access_counter_};
    current_size_ += Page::PAGE_SIZE;

    return page;
}

void BufferManager::release_page(const std::string& table_name, uint64_t page_id) {
    std::lock_guard<std::mutex> lock(mutex_);
    auto& table_buffer = buffer_[table_name];
    auto it = table_buffer.find(page_id);

    if (it != table_buffer.end()) {
        if (it->second.is_dirty) {
            write_page_to_disk(table_name, page_id, *(it->second.page));
            it->second.is_dirty = false;
        }
    }
}

void BufferManager::flush_page(const std::string& table_name, uint64_t page_id) {
    std::lock_guard<std::mutex> lock(mutex_);
    auto& table_buffer = buffer_[table_name];
    auto it = table_buffer.find(page_id);

    if (it != table_buffer.end() && it->second.is_dirty) {
        write_page_to_disk(table_name, page_id, *(it->second.page));
        it->second.is_dirty = false;
    }
}

void BufferManager::flush_all_pages() {
    std::lock_guard<std::mutex> lock(mutex_);
    for (auto& table_entry : buffer_) {
        for (auto& page_entry : table_entry.second) {
            if (page_entry.second.is_dirty) {
                write_page_to_disk(table_entry.first, page_entry.first, *(page_entry.second.page));
                page_entry.second.is_dirty = false;
            }
        }
    }
}

void BufferManager::invalidate_page(const std::string& table_name, uint64_t page_id) {
    std::lock_guard<std::mutex> lock(mutex_);
    auto& table_buffer = buffer_[table_name];
    auto it = table_buffer.find(page_id);

    if (it != table_buffer.end()) {
        if (it->second.is_dirty) {
            write_page_to_disk(table_name, page_id, *(it->second.page));
        }
        table_buffer.erase(it);
        current_size_ -= Page::PAGE_SIZE;
    }
}

void BufferManager::prefetch_pages(const std::string& table_name, const std::vector<uint64_t>& page_ids) {
    for (const auto& page_id : page_ids) {
        get_page(table_name, page_id);
    }
}

size_t BufferManager::determine_buffer_size() const {
    if (config_.initial_size > 0) {
        return config_.initial_size;
    }

    size_t total_memory = 0;

#ifdef _WIN32
    MEMORYSTATUSEX status;
    status.dwLength = sizeof(status);
    GlobalMemoryStatusEx(&status);
    total_memory = status.ullTotalPhys;
#else
    long pages = sysconf(_SC_PHYS_PAGES);
    long page_size = sysconf(_SC_PAGE_SIZE);
    total_memory = pages * page_size;
#endif

    return static_cast<size_t>(total_memory * config_.memory_usage_fraction);
}

void BufferManager::evict_page() {
    std::string table_to_evict;
    uint64_t page_to_evict = 0;
    size_t oldest_access_time = std::numeric_limits<size_t>::max();

    for (const auto& table_entry : buffer_) {
        for (const auto& page_entry : table_entry.second) {
            if (page_entry.second.last_access_time < oldest_access_time) {
                oldest_access_time = page_entry.second.last_access_time;
                table_to_evict = table_entry.first;
                page_to_evict = page_entry.first;
            }
        }
    }

    if (!table_to_evict.empty()) {
        auto& page_entry = buffer_[table_to_evict][page_to_evict];
        if (page_entry.is_dirty) {
            write_page_to_disk(table_to_evict, page_to_evict, *(page_entry.page));
        }
        buffer_[table_to_evict].erase(page_to_evict);
        current_size_ -= Page::PAGE_SIZE;
    }
}

void BufferManager::write_page_to_disk(const std::string& table_name, uint64_t page_id, const Page& page) {
    // This is a placeholder. In a real implementation, you would write the page to disk.
    LOG_INFO("Writing page to disk: " + table_name + ", page_id: " + std::to_string(page_id));
}

std::shared_ptr<Page> BufferManager::read_page_from_disk(const std::string& table_name, uint64_t page_id) {
    // This is a placeholder. In a real implementation, you would read the page from disk.
    LOG_INFO("Reading page from disk: " + table_name + ", page_id: " + std::to_string(page_id));
    return std::make_shared<Page>(page_id);
}

} // namespace nexusdb