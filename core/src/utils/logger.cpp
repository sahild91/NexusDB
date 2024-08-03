// File: src/utils/logger.cpp
#include "nexusdb/utils/logger.h"
#include <chrono>
#include <iomanip>
#include <sstream>
#include <iostream>

namespace nexusdb {
namespace utils {

FileLogDestination::FileLogDestination(const std::string& filename) {
    file_.open(filename, std::ios::app);
    if (!file_.is_open()) {
        throw std::runtime_error("Failed to open log file: " + filename);
    }
}

void FileLogDestination::write(const std::string& message) {
    std::lock_guard<std::mutex> lock(mutex_);
    file_ << message << std::endl;
    file_.flush();
}

NetworkLogDestination::NetworkLogDestination(const std::string& host, int port) {
    // Initialize network connection here
    // For simplicity, we'll just print to cout in this example
    std::cout << "Port" << port << std::endl;
    LOG_INFO("Inside NetworkLogDestination " + host);
}

void NetworkLogDestination::write(const std::string& message) {
    std::lock_guard<std::mutex> lock(mutex_);
    // Send message over network here
    // For simplicity, we'll just print to cout in this example
    std::cout << "Network Log: " << message << std::endl;
}

Logger& Logger::get_instance() {
    static Logger instance;
    return instance;
}

Logger::Logger() : current_level_(Level::INFO) {}

Logger::~Logger() = default;

void Logger::add_destination(std::unique_ptr<LogDestination> destination) {
    std::lock_guard<std::mutex> lock(mutex_);
    destinations_.push_back(std::move(destination));
}

void Logger::remove_all_destinations() {
    std::lock_guard<std::mutex> lock(mutex_);
    destinations_.clear();
}

void Logger::set_level(Level level) {
    std::lock_guard<std::mutex> lock(mutex_);
    current_level_ = level;
}

Logger::Level Logger::get_level() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return current_level_;
}

void Logger::log(Level level, const std::string& message) {
    if (level < current_level_) return;

    std::string formatted_message = get_timestamp() + " [" + level_to_string(level) + "] " + message;

    std::lock_guard<std::mutex> lock(mutex_);
    for (const auto& destination : destinations_) {
        destination->write(formatted_message);
    }
}

void Logger::debug(const std::string& message) {
    log(Level::DEBUG, message);
}

void Logger::info(const std::string& message) {
    log(Level::INFO, message);
}

void Logger::warning(const std::string& message) {
    log(Level::WARNING, message);
}

void Logger::error(const std::string& message) {
    log(Level::ERROR, message);
}

void Logger::fatal(const std::string& message) {
    log(Level::FATAL, message);
}

void Logger::log_network_operation(const std::string& operation, const std::string& details) {
    std::string message = "Network Operation: " + operation + " - " + details;
    log(Level::INFO, message);
}

void Logger::log_distributed_transaction(uint64_t transaction_id, const std::string& status) {
    std::string message = "Distributed Transaction " + std::to_string(transaction_id) + ": " + status;
    log(Level::INFO, message);
}

std::string Logger::level_to_string(Level level) {
    switch (level) {
        case Level::DEBUG:   return "DEBUG";
        case Level::INFO:    return "INFO";
        case Level::WARNING: return "WARNING";
        case Level::ERROR:   return "ERROR";
        case Level::FATAL:   return "FATAL";
        default:             return "UNKNOWN";
    }
}

std::string Logger::get_timestamp() const {
    auto now = std::chrono::system_clock::now();
    auto now_c = std::chrono::system_clock::to_time_t(now);
    auto now_ms = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()) % 1000;

    std::tm local_time;
    #ifdef _WIN32
    localtime_s(&local_time, &now_c); // Windows-specific version
    #else
    localtime_r(&now_c, &local_time); // POSIX version
    #endif
    
    std::stringstream ss;
    ss << std::put_time(&local_time, "%F %T");
    ss << '.' << std::setfill('0') << std::setw(3) << now_ms.count();
    return ss.str();
}

} // namespace utils
} // namespace nexusdb