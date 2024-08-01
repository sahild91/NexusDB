#include "page.h"
#include <cstring>
#include <algorithm>

namespace nexusdb {

Page::Page(uint64_t page_id) : page_id_(page_id), data_(PAGE_SIZE, 0), free_space_(PAGE_SIZE) {}

uint64_t Page::get_page_id() const {
    return page_id_;
}

char* Page::get_data() {
    return data_.data();
}

const char* Page::get_data() const {
    return data_.data();
}

size_t Page::get_free_space() const {
    return free_space_;
}

int Page::add_record(const std::vector<char>& record) {
    if (record.size() + sizeof(size_t) > free_space_) {
        return -1; // Not enough space
    }

    size_t offset = PAGE_SIZE - free_space_;
    size_t record_size = record.size();

    // Write record size
    std::memcpy(data_.data() + offset, &record_size, sizeof(size_t));
    offset += sizeof(size_t);

    // Write record data
    std::memcpy(data_.data() + offset, record.data(), record_size);

    free_space_ -= (record_size + sizeof(size_t));
    return static_cast<int>(offset - sizeof(size_t));
}

std::vector<char> Page::get_record(size_t offset) const {
    if (offset >= PAGE_SIZE - free_space_) {
        return {}; // Invalid offset
    }

    size_t record_size;
    std::memcpy(&record_size, data_.data() + offset, sizeof(size_t));

    if (offset + sizeof(size_t) + record_size > PAGE_SIZE) {
        return {}; // Corrupted record
    }

    return std::vector<char>(data_.begin() + offset + sizeof(size_t),
                             data_.begin() + offset + sizeof(size_t) + record_size);
}

bool Page::update_record(size_t offset, const std::vector<char>& new_record) {
    if (offset >= PAGE_SIZE - free_space_) {
        return false; // Invalid offset
    }

    size_t old_record_size;
    std::memcpy(&old_record_size, data_.data() + offset, sizeof(size_t));

    if (offset + sizeof(size_t) + old_record_size > PAGE_SIZE) {
        return false; // Corrupted record
    }

    if (new_record.size() <= old_record_size) {
        // New record fits in the old space
        std::memcpy(data_.data() + offset + sizeof(size_t), new_record.data(), new_record.size());
        free_space_ += (old_record_size - new_record.size());
        size_t new_record_size = new_record.size();
        std::memcpy(data_.data() + offset, &new_record_size, sizeof(size_t));
    } else {
        // New record doesn't fit, need to delete old and add new
        delete_record(offset);
        int new_offset = add_record(new_record);
        return new_offset != -1;
    }

    return true;
}

bool Page::delete_record(size_t offset) {
    if (offset >= PAGE_SIZE - free_space_) {
        return false; // Invalid offset
    }

    size_t record_size;
    std::memcpy(&record_size, data_.data() + offset, sizeof(size_t));

    if (offset + sizeof(size_t) + record_size > PAGE_SIZE) {
        return false; // Corrupted record
    }

    // Mark the space as free
    std::memset(data_.data() + offset, 0, sizeof(size_t) + record_size);
    free_space_ += sizeof(size_t) + record_size;

    compact();
    return true;
}

void Page::compact() {
    std::vector<char> temp_data(PAGE_SIZE, 0);
    size_t write_offset = 0;
    size_t read_offset = 0;

    while (read_offset < PAGE_SIZE - free_space_) {
        size_t record_size;
        std::memcpy(&record_size, data_.data() + read_offset, sizeof(size_t));

        if (record_size == 0) {
            // Skip free space
            read_offset += sizeof(size_t);
        } else {
            // Copy record to temp buffer
            std::memcpy(temp_data.data() + write_offset, 
                        data_.data() + read_offset, 
                        sizeof(size_t) + record_size);
            write_offset += sizeof(size_t) + record_size;
            read_offset += sizeof(size_t) + record_size;
        }
    }

    // Update data and free space
    data_ = std::move(temp_data);
    free_space_ = PAGE_SIZE - write_offset;
}

} // namespace nexusdb