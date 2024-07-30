#ifndef NEXUSDB_LOGGER_H
#define NEXUSDB_LOGGER_H

#include <string>
#include <fstream>
#include <mutex>

namespace nexusdb {
namespace utils {

/// Logger class for NexusDB
class Logger {
public:
    /// Log levels
    enum class Level {
        DEBUG,
        INFO,
        WARNING,
        ERROR,
        FATAL
    };

    /// Get the singleton instance of the logger
    /// @return Reference to the logger instance
    static Logger& get_instance();

    /// Initialize the logger
    /// @param log_file Path to the log file
    /// @param console_output Whether to output logs to console
    /// @return true if initialization was successful, false otherwise
    bool initialize(const std::string& log_file, bool console_output = true);

    /// Log a message
    /// @param level Log level
    /// @param message Message to log
    void log(Level level, const std::string& message);

    /// Convenience method for debug logging
    /// @param message Message to log
    void debug(const std::string& message);

    /// Convenience method for info logging
    /// @param message Message to log
    void info(const std::string& message);

    /// Convenience method for warning logging
    /// @param message Message to log
    void warning(const std::string& message);

    /// Convenience method for error logging
    /// @param message Message to log
    void error(const std::string& message);

    /// Convenience method for fatal logging
    /// @param message Message to log
    void fatal(const std::string& message);

private:
    Logger() = default;  // Private constructor for singleton
    ~Logger();

    Logger(const Logger&) = delete;
    Logger& operator=(const Logger&) = delete;

    std::ofstream log_file_;
    bool console_output_ = false;
    std::mutex mutex_;

    /// Convert log level to string
    /// @param level Log level
    /// @return String representation of the log level
    std::string level_to_string(Level level);
};

} // namespace utils
} // namespace nexusdb

#define LOG_DEBUG(message) nexusdb::utils::Logger::get_instance().debug(message)
#define LOG_INFO(message) nexusdb::utils::Logger::get_instance().info(message)
#define LOG_WARNING(message) nexusdb::utils::Logger::get_instance().warning(message)
#define LOG_ERROR(message) nexusdb::utils::Logger::get_instance().error(message)
#define LOG_FATAL(message) nexusdb::utils::Logger::get_instance().fatal(message)

#endif // NEXUSDB_LOGGER_H