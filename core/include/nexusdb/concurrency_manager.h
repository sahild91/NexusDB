#ifndef NEXUSDB_CONCURRENCY_MANAGER_H
#define NEXUSDB_CONCURRENCY_MANAGER_H

#include <string>
#include <optional>
#include <mutex>
#include <unordered_map>
#include <shared_mutex>

namespace nexusdb {

class ConcurrencyManager {
public:
    ConcurrencyManager();
    ~ConcurrencyManager();

    std::optional<std::string> initialize();
    void shutdown();

    std::optional<std::string> acquire_lock(const std::string& resource, bool exclusive = false);
    std::optional<std::string> release_lock(const std::string& resource);

private:
    std::mutex mutex_;
    std::unordered_map<std::string, std::shared_mutex> locks_;
};

} // namespace nexusdb

#endif // NEXUSDB_CONCURRENCY_MANAGER_H