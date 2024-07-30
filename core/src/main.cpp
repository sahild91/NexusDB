#include "nexusdb/nexusdb.h"
#include "nexusdb/utils/logger.h"
#include <iostream>
#include <stdexcept>

int main() {
    // Initialize logger
    if (!nexusdb::utils::Logger::get_instance().initialize("nexusdb.log", true)) {
        std::cerr << "Failed to initialize logger" << std::endl;
        return 1;
    }

    try {
        LOG_INFO("Starting NexusDB");
        
        nexusdb::NexusDB db;
        
        if (!db.initialize()) {
            LOG_ERROR("Failed to initialize NexusDB");
            return 1;
        }
        
        LOG_INFO("NexusDB is ready");
        
        // Add your test code or REPL here
        
        db.shutdown();
        LOG_INFO("NexusDB shut down successfully");
    } catch (const std::exception& e) {
        LOG_FATAL("Exception caught: " + std::string(e.what()));
        return 1;
    }

    return 0;
}