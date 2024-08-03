#ifndef NEXUSDB_LOGGER_H
#define NEXUSDB_LOGGER_H

#include <string>
#include <fstream>
#include <mutex>
#include <vector>
#include <memory>

namespace nexusdb {
namespace utils {

class LogDestination {
public:
    virtual ~LogDestination() = default;
    virtual void write(const std::string& message) = 0;
};

class FileLogDestination : public LogDestination {
public:
    FileLogDestination(const std::string& filename);
    void write(const std::string& message) override;

private:
    std::ofstream file_;
    std::mutex mutex_;
};

class NetworkLogDestination : public LogDestination {
public:
    NetworkLogDestination(const std::string& host, int port);
    void write(const std::string& message) override;

private:
    // Network socket or client would be here
    std::mutex mutex_;
};

class Logger {
public:
    enum class Level {
        DEBUG,
        INFO,
        WARNING,
        ERROR,
        FATAL
    };

    static Logger& get_instance();

    void add_destination(std::unique_ptr<LogDestination> destination);
    void remove_all_destinations();

    void set_level(Level level);
    Level get_level() const;

    void log(Level level, const std::string& message);

    void debug(const std::string& message);
    void info(const std::string& message);
    void warning(const std::string& message);
    void error(const std::string& message);
    void fatal(const std::string& message);

    // New methods for distributed logging
    void log_network_operation(const std::string& operation, const std::string& details);
    void log_distributed_transaction(uint64_t transaction_id, const std::string& status);

private:
    Logger();
    ~Logger();

    Logger(const Logger&) = delete;
    Logger& operator=(const Logger&) = delete;

    std::vector<std::unique_ptr<LogDestination>> destinations_;
    Level current_level_;
    mutable std::mutex mutex_;

    std::string level_to_string(Level level);
    std::string get_timestamp() const;
};

} // namespace utils
} // namespace nexusdb

#define LOG_DEBUG(message) nexusdb::utils::Logger::get_instance().debug(message)
#define LOG_INFO(message) nexusdb::utils::Logger::get_instance().info(message)
#define LOG_WARNING(message) nexusdb::utils::Logger::get_instance().warning(message)
#define LOG_ERROR(message) nexusdb::utils::Logger::get_instance().error(message)
#define LOG_FATAL(message) nexusdb::utils::Logger::get_instance().fatal(message)

#endif // NEXUSDB_LOGGER_H