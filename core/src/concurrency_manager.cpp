#include "nexusdb/concurrency_manager.h"
#include "nexusdb/utils/logger.h"

namespace nexusdb {

ConcurrencyManager::ConcurrencyManager() {
    LOG_DEBUG("ConcurrencyManager constructor called");
}

ConcurrencyManager::~ConcurrencyManager() {
    LOG_DEBUG("ConcurrencyManager destructor called");
    shutdown();
}

std::optional<std::string> ConcurrencyManager::initialize() {
    LOG_INFO("Initializing Concurrency Manager...");
    // Any necessary initialization can be done here
    LOG_INFO("Concurrency Manager initialized successfully");
    return std::nullopt;
}

void ConcurrencyManager::shutdown() {
    LOG_INFO("Shutting down Concurrency Manager...");
    std::lock_guard<std::mutex> lock(mutex_);
    locks_.clear();
    LOG_INFO("Concurrency Manager shut down successfully");
}

std::optional<std::string> ConcurrencyManager::acquire_lock(const std::string& resource, bool exclusive) {
    std::lock_guard<std::mutex> lock(mutex_);
    auto& resource_lock = locks_[resource];
    
    try {
        if (exclusive) {
            resource_lock.lock();
        } else {
            resource_lock.lock_shared();
        }
        LOG_DEBUG("Lock acquired for resource: " + resource + (exclusive ? " (exclusive)" : " (shared)"));
        return std::nullopt;
    } catch (const std::system_error& e) {
        LOG_ERROR("Failed to acquire lock for resource: " + resource);
        return "Failed to acquire lock: " + std::string(e.what());
    }
}

std::optional<std::string> ConcurrencyManager::release_lock(const std::string& resource) {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = locks_.find(resource);
    if (it == locks_.end()) {
        LOG_ERROR("Attempted to release non-existent lock for resource: " + resource);
        return "Lock does not exist for resource: " + resource;
    }
    
    try {
        it->second.unlock();
        LOG_DEBUG("Lock released for resource: " + resource);
        return std::nullopt;
    } catch (const std::system_error& e) {
        LOG_ERROR("Failed to release lock for resource: " + resource);
        return "Failed to release lock: " + std::string(e.what());
    }
}

} // namespace nexusdb