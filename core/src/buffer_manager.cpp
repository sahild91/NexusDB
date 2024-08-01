#include "nexusdb/buffer_manager.h"
#include "nexusdb/utils/logger.h"
#include <stdexcept>
#include <cstdlib>

#ifdef _WIN32
#include <windows.h>
#else
#include <unistd.h>
#endif

namespace nexusdb {

BufferManager::BufferManager(const BufferConfig& config) : config_(config) {
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
        buffer_.resize(buffer_size);
        LOG_INFO("Buffer Manager initialized successfully with size: " + std::to_string(buffer_size) + " bytes");
        return std::nullopt;
    } catch (const std::exception& e) {
        LOG_ERROR("Failed to initialize Buffer Manager: " + std::string(e.what()));
        return "Failed to initialize Buffer Manager: " + std::string(e.what());
    }
}

void BufferManager::shutdown() {
    LOG_INFO("Shutting down Buffer Manager...");
    std::lock_guard<std::mutex> lock(mutex_);
    buffer_.clear();
    buffer_.shrink_to_fit();
    LOG_INFO("Buffer Manager shut down successfully");
}

size_t BufferManager::get_buffer_size() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return buffer_.size();
}

std::optional<std::string> BufferManager::resize_buffer(size_t new_size) {
    try {
        std::lock_guard<std::mutex> lock(mutex_);
        buffer_.resize(new_size);
        LOG_INFO("Buffer resized to: " + std::to_string(new_size) + " bytes");
        return std::nullopt;
    } catch (const std::exception& e) {
        LOG_ERROR("Failed to resize buffer: " + std::string(e.what()));
        return "Failed to resize buffer: " + std::string(e.what());
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

} // namespace nexusdb