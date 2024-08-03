// File: src/memory_mapped_file.cpp
#include "nexusdb/memory_mapped_file.h"
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdexcept>

namespace nexusdb {

MemoryMappedFile::MemoryMappedFile(const std::string& filename)
    : filename_(filename), fd_(-1), file_size_(0) {
    fd_ = open(filename.c_str(), O_RDWR);
    if (fd_ == -1) {
        throw std::runtime_error("Failed to open file: " + filename);
    }

    struct stat sb;
    if (fstat(fd_, &sb) == -1) {
        close(fd_);
        throw std::runtime_error("Failed to get file size");
    }
    file_size_ = sb.st_size;
}

MemoryMappedFile::~MemoryMappedFile() {
    if (fd_ != -1) {
        close(fd_);
    }
}

void* MemoryMappedFile::map(std::size_t offset, std::size_t length) {
    void* addr = mmap(nullptr, length, PROT_READ | PROT_WRITE, MAP_SHARED, fd_, offset);
    if (addr == MAP_FAILED) {
        throw std::runtime_error("Failed to map file");
    }
    return addr;
}

void MemoryMappedFile::unmap(void* addr, std::size_t length) {
    if (munmap(addr, length) == -1) {
        throw std::runtime_error("Failed to unmap file");
    }
}

void MemoryMappedFile::flush(void* addr, std::size_t length) {
    if (msync(addr, length, MS_SYNC) == -1) {
        throw std::runtime_error("Failed to flush mapped memory");
    }
}

std::size_t MemoryMappedFile::get_file_size() const {
    return file_size_;
}

} // namespace nexusdb