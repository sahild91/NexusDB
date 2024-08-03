#ifndef NEXUSDB_MEMORY_MAPPED_FILE_H
#define NEXUSDB_MEMORY_MAPPED_FILE_H

#include <string>
#include <cstdint>

namespace nexusdb {

class MemoryMappedFile {
public:
    MemoryMappedFile(const std::string& filename);
    ~MemoryMappedFile();

    void* map(std::size_t offset, std::size_t length);
    void unmap(void* addr, std::size_t length);
    void flush(void* addr, std::size_t length);

    std::size_t get_file_size() const;

private:
    int fd_;
    std::string filename_;
    std::size_t file_size_;
};

} // namespace nexusdb

#endif // NEXUSDB_MEMORY_MAPPED_FILE_H