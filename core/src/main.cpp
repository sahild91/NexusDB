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
        
        if (!db.initialize("./data")) {
            LOG_ERROR("Failed to initialize NexusDB");
            return 1;
        }
        
        LOG_INFO("NexusDB is ready");
        
        // Create a table
        
        std::vector<std::string> schema = {"id INTEGER", "name TEXT", "age INTEGER"};
        if (db.create_table("users", schema)) {
            LOG_INFO("Created 'users' table");
        } else {
            LOG_ERROR("Failed to create 'users' table");
        }

        // Insert a record
        std::vector<std::string> record = {"1", "John Doe", "30"};
        if (db.insert_record("users", record)) {
            LOG_INFO("Inserted record into 'users' table");
        } else {
            LOG_ERROR("Failed to insert record into 'users' table");
        }

        // Read a record
        std::vector<std::string> retrieved_record;
        if (db.read_record("users", 0)) {
            LOG_INFO("Retrieved record: id=" + retrieved_record[0] + 
                     ", name=" + retrieved_record[1] + 
                     ", age=" + retrieved_record[2]);
        } else {
            LOG_ERROR("Failed to retrieve record from 'users' table");
        }

        db.shutdown();
        LOG_INFO("NexusDB shut down successfully");
    } catch (const std::exception& e) {
        LOG_FATAL("Exception caught: " + std::string(e.what()));
        return 1;
    }

    return 0;
}