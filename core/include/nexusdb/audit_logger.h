#ifndef NEXUSDB_AUDIT_LOGGER_H
#define NEXUSDB_AUDIT_LOGGER_H

#include <string>
#include <chrono>
#include <memory>

namespace nexusdb {

enum class AuditEventType {
    LOGIN,
    LOGOUT,
    QUERY_EXECUTION,
    SCHEMA_CHANGE,
    DATA_ACCESS,
    CONFIGURATION_CHANGE,
    SECURITY_EVENT
};

class AuditLogger {
public:
    AuditLogger(const std::string& log_file);
    ~AuditLogger();

    void log_event(AuditEventType event_type, 
                   const std::string& user, 
                   const std::string& details);

    void set_retention_period(std::chrono::days period);
    void rotate_logs();

private:
    class AuditLoggerImpl;
    std::unique_ptr<AuditLoggerImpl> pimpl_;
};

} // namespace nexusdb

#endif // NEXUSDB_AUDIT_LOGGER_H