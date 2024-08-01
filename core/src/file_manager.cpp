#include "file_manager.h"
#include <stdexcept>
#include <cstdio>
#include <cerrno>

#ifdef _WIN32
#include <direct.h>
#include <io.h>
#define MKDIR(dir) _mkdir(dir)
#define ACCESS(path, mode) _access(path, mode)
#define F_OK 0
#else
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#define MKDIR(dir) mkdir(dir, 0777)
#define ACCESS(path, mode) access(path, mode)
#endif

namespace nexusdb {

FileManager::FileManager(const std::string& data_directory) : data_directory_(data_directory) {
    // Create directory if it doesn't exist
    if (MKDIR(data_directory_.c_str()) != 0 && errno != EEXIST) {
        throw std::runtime_error("Failed to create data directory");
    }
}

FileManager::~FileManager() {
    for (auto& file : open_files_) {
        file.second.close();
    }
}

bool FileManager::create_file(const std::string& file_name) {
    std::string full_path = get_file_path(file_name);
    if (file_exists(full_path)) {
        return false; // File already exists
    }

    std::ofstream file(full_path, std::ios::binary);
    return file.is_open();
}

bool FileManager::open_file(const std::string& file_name) {
    if (is_file_open(file_name)) {
        return true; // File is already open
    }

    std::string full_path = get_file_path(file_name);
    if (!file_exists(full_path)) {
        return false; // File doesn't exist
    }

    open_files_[file_name].open(full_path, std::ios::in | std::ios::out | std::ios::binary);
    return open_files_[file_name].is_open();
}

void FileManager::close_file(const std::string& file_name) {
    auto it = open_files_.find(file_name);
    if (it != open_files_.end()) {
        it->second.close();
        open_files_.erase(it);
    }
}

std::unique_ptr<Page> FileManager::read_page(const std::string& file_name, uint64_t page_id) {
    if (!is_file_open(file_name) && !open_file(file_name)) {
        return nullptr;
    }

    auto& file = open_files_[file_name];
    file.seekg(page_id * Page::PAGE_SIZE, std::ios::beg);

    auto page = std::make_unique<Page>(page_id);
    file.read(page->get_data(), Page::PAGE_SIZE);

    if (file.gcount() != Page::PAGE_SIZE) {
        return nullptr; // Failed to read full page
    }

    return page;
}

bool FileManager::write_page(const std::string& file_name, const Page& page) {
    if (!is_file_open(file_name) && !open_file(file_name)) {
        return false;
    }

    auto& file = open_files_[file_name];
    file.seekp(page.get_page_id() * Page::PAGE_SIZE, std::ios::beg);
    file.write(page.get_data(), Page::PAGE_SIZE);
    file.flush();

    return !file.fail();
}

std::unique_ptr<Page> FileManager::allocate_page(const std::string& file_name) {
    if (!is_file_open(file_name) && !open_file(file_name)) {
        return nullptr;
    }

    auto& file = open_files_[file_name];
    file.seekp(0, std::ios::end);
    uint64_t file_size = file.tellp();
    uint64_t new_page_id = file_size / Page::PAGE_SIZE;

    auto new_page = std::make_unique<Page>(new_page_id);
    if (!write_page(file_name, *new_page)) {
        return nullptr;
    }

    return new_page;
}

std::string FileManager::get_file_path(const std::string& file_name) const {
    return data_directory_ + "/" + file_name;
}

bool FileManager::is_file_open(const std::string& file_name) const {
    return open_files_.find(file_name) != open_files_.end();
}

bool FileManager::file_exists(const std::string& file_name) const {
    return ACCESS(file_name.c_str(), F_OK) == 0;
}

} // namespace nexusdb