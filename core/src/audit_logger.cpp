// File: src/audit_logger.cpp
#include "nexusdb/audit_logger.h"
#include <fstream>
#include <mutex>
#include <chrono>
#include <ctime>
#include <iomanip>
#include <filesystem>
#include <sstream>
#include <thread>

namespace nexusdb {

class AuditLogger::AuditLoggerImpl {
public:
    AuditLoggerImpl(const std::string& log_file)
        : log_file_(log_file), retention_period_(std::chrono::days(30)) {
        log_stream_.open(log_file, std::ios::app);
        if (!log_stream_.is_open()) {
            throw std::runtime_error("Failed to open log file: " + log_file);
        }
    }

    ~AuditLoggerImpl() {
        if (log_stream_.is_open()) {
            log_stream_.close();
        }
    }

    void log_event(AuditEventType event_type, const std::string& user, const std::string& details) {
        std::lock_guard<std::mutex> lock(mutex_);
        
        auto now = std::chrono::system_clock::now();
        auto now_c = std::chrono::system_clock::to_time_t(now);
        
        std::ostringstream oss;
        oss << std::put_time(std::localtime(&now_c), "%Y-%m-%d %H:%M:%S")
            << " | " << event_type_to_string(event_type)
            << " | User: " << user
            << " | " << details
            << std::endl;
        
        log_stream_ << oss.str();
        log_stream_.flush();
    }

    void set_retention_period(std::chrono::days period) {
        std::lock_guard<std::mutex> lock(mutex_);
        retention_period_ = period;
    }

    void rotate_logs() {
        std::lock_guard<std::mutex> lock(mutex_);
        
        // Close current log file
        log_stream_.close();
        
        // Generate new log file name
        auto now = std::chrono::system_clock::now();
        auto now_c = std::chrono::system_clock::to_time_t(now);
        std::ostringstream oss;
        oss << std::put_time(std::localtime(&now_c), "%Y%m%d_%H%M%S");
        std::string new_log_file = log_file_ + "." + oss.str();
        
        // Rename current log file
        std::filesystem::rename(log_file_, new_log_file);
        
        // Open new log file
        log_stream_.open(log_file_, std::ios::app);
        if (!log_stream_.is_open()) {
            throw std::runtime_error("Failed to open new log file: " + log_file_);
        }
        
        // Delete old log files
        delete_old_logs();
    }

private:
    std::string log_file_;
    std::ofstream log_stream_;
    std::mutex mutex_;
    std::chrono::days retention_period_;

    std::string event_type_to_string(AuditEventType event_type) {
        switch (event_type) {
            case AuditEventType::LOGIN: return "LOGIN";
            case AuditEventType::LOGOUT: return "LOGOUT";
            case AuditEventType::QUERY_EXECUTION: return "QUERY_EXECUTION";
            case AuditEventType::SCHEMA_CHANGE: return "SCHEMA_CHANGE";
            case AuditEventType::DATA_ACCESS: return "DATA_ACCESS";
            case AuditEventType::CONFIGURATION_CHANGE: return "CONFIGURATION_CHANGE";
            case AuditEventType::SECURITY_EVENT: return "SECURITY_EVENT";
            default: return "UNKNOWN";
        }
    }

    void delete_old_logs() {
        auto now = std::chrono::system_clock::now();
        for (const auto& entry : std::filesystem::directory_iterator(std::filesystem::path(log_file_).parent_path())) {
            if (entry.is_regular_file() && entry.path().string().find(log_file_) != std::string::npos) {
                auto last_write_time = std::chrono::system_clock::from_time_t(std::filesystem::last_write_time(entry).time_since_epoch().count());
                if (now - last_write_time > retention_period_) {
                    std::filesystem::remove(entry.path());
                }
            }
        }
    }
};

AuditLogger::AuditLogger(const std::string& log_file)
    : pimpl_(std::make_unique<AuditLoggerImpl>(log_file)) {}

AuditLogger::~AuditLogger() = default;

void AuditLogger::log_event(AuditEventType event_type, const std::string& user, const std::string& details) {
    pimpl_->log_event(event_type, user, details);
}

void AuditLogger::set_retention_period(std::chrono::days period) {
    pimpl_->set_retention_period(period);
}

void AuditLogger::rotate_logs() {
    pimpl_->rotate_logs();
}

} // namespace nexusdb