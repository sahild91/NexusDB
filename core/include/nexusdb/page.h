#ifndef NEXUSDB_PAGE_H
#define NEXUSDB_PAGE_H

#include <vector>
#include <cstdint>
#include "nexusdb/data_compression.h"

namespace nexusdb {

class Page {
public:
    static const size_t PAGE_SIZE = 4096; // 4KB page size

    Page(uint64_t page_id);

    uint64_t get_page_id() const;
    char* get_data();
    const char* get_data() const;
    size_t get_free_space() const;

    int add_record(const std::vector<char>& record);
    std::vector<char> get_record(size_t offset) const;
    bool update_record(size_t offset, const std::vector<char>& new_record);
    bool delete_record(size_t offset);

    void compress();
    void decompress();
    bool is_compressed() const { return is_compressed_; }

private:
    uint64_t page_id_;
    std::vector<char> data_;
    size_t free_space_;
    bool is_compressed_;
    
    void compact();
    void ensure_decompressed() const;
};

} // namespace nexusdb

#endif // NEXUSDB_PAGE_H