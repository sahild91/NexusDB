#include "nexusdb/utils/logger.h"
#include <iostream>
#include <chrono>
#include <iomanip>
#include <sstream>

namespace nexusdb {
namespace utils {

Logger& Logger::get_instance() {
    static Logger instance;
    return instance;
}

bool Logger::initialize(const std::string& log_file, bool console_output) {
    console_output_ = console_output;
    log_file_.open(log_file, std::ios::app);
    return log_file_.is_open();
}

Logger::~Logger() {
    if (log_file_.is_open()) {
        log_file_.close();
    }
}

void Logger::log(Level level, const std::string& message) {
    std::lock_guard<std::mutex> lock(mutex_);

    auto now = std::chrono::system_clock::now();
    auto now_c = std::chrono::system_clock::to_time_t(now);
    auto now_tm = std::localtime(&now_c);

    std::ostringstream ss;  // Changed from std::stringstream to std::ostringstream
    ss << std::put_time(now_tm, "%Y-%m-%d %H:%M:%S")
       << " [" << level_to_string(level) << "] "
       << message;

    if (log_file_.is_open()) {
        log_file_ << ss.str() << std::endl;
    }

    if (console_output_) {
        std::cout << ss.str() << std::endl;
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

} // namespace utils
} // namespace nexusdb