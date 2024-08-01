#ifndef NEXUSDB_FILE_MANAGER_H
#define NEXUSDB_FILE_MANAGER_H

#include <string>
#include <fstream>
#include <memory>
#include <unordered_map>
#include "page.h"

namespace nexusdb {

class FileManager {
public:
    FileManager(const std::string& data_directory);
    ~FileManager();

    bool create_file(const std::string& file_name);
    bool open_file(const std::string& file_name);
    void close_file(const std::string& file_name);
    std::unique_ptr<Page> read_page(const std::string& file_name, uint64_t page_id);
    bool write_page(const std::string& file_name, const Page& page);
    std::unique_ptr<Page> allocate_page(const std::string& file_name);

private:
    std::string data_directory_;
    std::unordered_map<std::string, std::fstream> open_files_;

    std::string get_file_path(const std::string& file_name) const;
    bool is_file_open(const std::string& file_name) const;
    bool file_exists(const std::string& file_name) const;
};

} // namespace nexusdb

#endif // NEXUSDB_FILE_MANAGER_H