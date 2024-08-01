#ifndef NEXUSDB_PAGE_H
#define NEXUSDB_PAGE_H

#include <vector>
#include <cstdint>

namespace nexusdb {

class Page {
public:
    static const size_t PAGE_SIZE = 4096; // 4KB page size

    Page(uint64_t page_id);

    uint64_t get_page_id() const;
    
    // Get a pointer to the raw data
    char* get_data();
    const char* get_data() const;

    // Get the amount of free space in the page
    size_t get_free_space() const;

    // Add a record to the page
    // Returns the offset of the record within the page, or -1 if there's not enough space
    int add_record(const std::vector<char>& record);

    // Get a record from the page
    // Returns the record data, or an empty vector if the record doesn't exist
    std::vector<char> get_record(size_t offset) const;

    // Update a record in the page
    // Returns true if the update was successful, false otherwise
    bool update_record(size_t offset, const std::vector<char>& new_record);

    // Delete a record from the page
    // Returns true if the deletion was successful, false otherwise
    bool delete_record(size_t offset);

private:
    uint64_t page_id_;
    std::vector<char> data_;
    size_t free_space_;
    
    // Helper method to compact the page after deletions
    void compact();
};

} // namespace nexusdb

#endif // NEXUSDB_PAGE_H