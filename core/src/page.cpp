#include "nexusdb/page.h"
#include "nexusdb/utils/logger.h"
#include <algorithm>
#include <cstring>
#include <stdexcept>
#include <openssl/evp.h>
#include <openssl/sha.h>

namespace nexusdb {

Page::Page(uint64_t page_id) 
    : page_id_(page_id), data_(PAGE_SIZE, 0), free_space_(PAGE_SIZE), 
      is_compressed_(false), is_encrypted_(false), checksum_(0) {
    update_checksum();
}

Page::Page(uint64_t page_id, const char* data)
    : page_id_(page_id), data_(data, data + PAGE_SIZE), free_space_(0), 
      is_compressed_(false), is_encrypted_(false), checksum_(0) {
    update_checksum();
}

uint64_t Page::get_page_id() const {
    return page_id_;
}

char* Page::get_data() {
    ensure_decompressed();
    return data_.data();
}

const char* Page::get_data() const {
    ensure_decompressed();
    return data_.data();
}

size_t Page::get_free_space() const {
    ensure_decompressed();
    return free_space_;
}

int Page::add_record(const std::vector<char>& record) {
    ensure_decompressed();
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
    update_checksum();
    return static_cast<int>(offset - sizeof(size_t));
}

std::vector<char> Page::get_record(size_t offset) const {
    ensure_decompressed();
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
    ensure_decompressed();
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

    update_checksum();
    return true;
}

bool Page::delete_record(size_t offset) {
    ensure_decompressed();
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
    update_checksum();
    return true;
}

void Page::compress() {
    if (!is_compressed_) {
        std::vector<uint8_t> compressed_data = Compression::compress_rle(std::vector<uint8_t>(data_.begin(), data_.end()));
        data_ = std::vector<char>(compressed_data.begin(), compressed_data.end());
        is_compressed_ = true;
        update_checksum();
    }
}

void Page::decompress() {
    if (is_compressed_) {
        std::vector<uint8_t> decompressed_data = Compression::decompress_rle(std::vector<uint8_t>(data_.begin(), data_.end()));
        data_ = std::vector<char>(decompressed_data.begin(), decompressed_data.end());
        is_compressed_ = false;
        update_checksum();
    }
}

void Page::encrypt(const std::vector<unsigned char>& key) {
    if (!is_encrypted_) {
        EVP_CIPHER_CTX* ctx = EVP_CIPHER_CTX_new();
        if (!ctx) {
            throw std::runtime_error("Failed to create cipher context");
        }

        std::vector<unsigned char> iv(EVP_MAX_IV_LENGTH);
        if (RAND_bytes(iv.data(), EVP_MAX_IV_LENGTH) != 1) {
            EVP_CIPHER_CTX_free(ctx);
            throw std::runtime_error("Failed to generate IV");
        }

        if (EVP_EncryptInit_ex(ctx, EVP_aes_256_cbc(), nullptr, key.data(), iv.data()) != 1) {
            EVP_CIPHER_CTX_free(ctx);
            throw std::runtime_error("Failed to initialize encryption");
        }

        std::vector<unsigned char> ciphertext(data_.size() + EVP_MAX_BLOCK_LENGTH);
        int len;
        if (EVP_EncryptUpdate(ctx, ciphertext.data(), &len, reinterpret_cast<unsigned char*>(data_.data()), data_.size()) != 1) {
            EVP_CIPHER_CTX_free(ctx);
            throw std::runtime_error("Failed to encrypt data");
        }
        int ciphertext_len = len;

        if (EVP_EncryptFinal_ex(ctx, ciphertext.data() + len, &len) != 1) {
            EVP_CIPHER_CTX_free(ctx);
            throw std::runtime_error("Failed to finalize encryption");
        }
        ciphertext_len += len;

        EVP_CIPHER_CTX_free(ctx);

        data_ = std::vector<char>(ciphertext.begin(), ciphertext.begin() + ciphertext_len);
        is_encrypted_ = true;
        update_checksum();
    }
}

void Page::decrypt(const std::vector<unsigned char>& key) {
    if (is_encrypted_) {
        EVP_CIPHER_CTX* ctx = EVP_CIPHER_CTX_new();
        if (!ctx) {
            throw std::runtime_error("Failed to create cipher context");
        }

        std::vector<unsigned char> iv(data_.begin(), data_.begin() + EVP_MAX_IV_LENGTH);
        
        if (EVP_DecryptInit_ex(ctx, EVP_aes_256_cbc(), nullptr, key.data(), iv.data()) != 1) {
            EVP_CIPHER_CTX_free(ctx);
            throw std::runtime_error("Failed to initialize decryption");
        }

        std::vector<unsigned char> plaintext(data_.size());
        int len;
        if (EVP_DecryptUpdate(ctx, plaintext.data(), &len, reinterpret_cast<unsigned char*>(data_.data()) + EVP_MAX_IV_LENGTH, data_.size() - EVP_MAX_IV_LENGTH) != 1) {
            EVP_CIPHER_CTX_free(ctx);
            throw std::runtime_error("Failed to decrypt data");
        }
        int plaintext_len = len;

        if (EVP_DecryptFinal_ex(ctx, plaintext.data() + len, &len) != 1) {
            EVP_CIPHER_CTX_free(ctx);
            throw std::runtime_error("Failed to finalize decryption");
        }
        plaintext_len += len;

        EVP_CIPHER_CTX_free(ctx);

        data_ = std::vector<char>(plaintext.begin(), plaintext.begin() + plaintext_len);
        is_encrypted_ = false;
        update_checksum();
    }
}

std::vector<char> Page::serialize() const {
    std::vector<char> serialized;
    serialized.reserve(sizeof(uint64_t) + sizeof(size_t) + sizeof(bool) * 2 + sizeof(uint32_t) + data_.size());

    // Serialize page_id_
    serialized.insert(serialized.end(), reinterpret_cast<const char*>(&page_id_), reinterpret_cast<const char*>(&page_id_) + sizeof(uint64_t));

    // Serialize free_space_
    serialized.insert(serialized.end(), reinterpret_cast<const char*>(&free_space_), reinterpret_cast<const char*>(&free_space_) + sizeof(size_t));

    // Serialize is_compressed_ and is_encrypted_
    serialized.push_back(is_compressed_);
    serialized.push_back(is_encrypted_);

    // Serialize checksum_
    serialized.insert(serialized.end(), reinterpret_cast<const char*>(&checksum_), reinterpret_cast<const char*>(&checksum_) + sizeof(uint32_t));

    // Serialize data_
    serialized.insert(serialized.end(), data_.begin(), data_.end());

    return serialized;
}

Page Page::deserialize(const std::vector<char>& data) {
    if (data.size() < sizeof(uint64_t) + sizeof(size_t) + sizeof(bool) * 2 + sizeof(uint32_t)) {
        throw std::runtime_error("Insufficient data for deserialization");
    }

    const char* ptr = data.data();

    // Deserialize page_id_
    uint64_t page_id;
    std::memcpy(&page_id, ptr, sizeof(uint64_t));
    ptr += sizeof(uint64_t);

    // Deserialize free_space_
    size_t free_space;
    std::memcpy(&free_space, ptr, sizeof(size_t));
    ptr += sizeof(size_t);

    // Deserialize is_compressed_ and is_encrypted_
    bool is_compressed = *ptr++;
    bool is_encrypted = *ptr++;

    // Deserialize checksum_
    uint32_t checksum;
    std::memcpy(&checksum, ptr, sizeof(uint32_t));
    ptr += sizeof(uint32_t);

    // Create Page object
    Page page(page_id);
    page.free_space_ = free_space;
    page.is_compressed_ = is_compressed;
    page.is_encrypted_ = is_encrypted;
    page.checksum_ = checksum;

    // Copy remaining data
    page.data_.assign(ptr, data.end());

    if (!page.verify_checksum()) {
        throw std::runtime_error("Checksum verification failed during deserialization");
    }

    return page;
}

uint32_t Page::calculate_checksum() const {
    unsigned char hash[SHA256_DIGEST_LENGTH];
    SHA256_CTX sha256;
    SHA256_Init(&sha256);
    SHA256_Update(&sha256, data_.data(), data_.size());
    SHA256_Final(hash, &sha256);

    uint32_t checksum;
    std::memcpy(&checksum, hash, sizeof(uint32_t));
    return checksum;
}

bool Page::verify_checksum() const {
    return calculate_checksum() == checksum_;
}

void Page::compact() {
    ensure_decompressed();
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
    update_checksum();
}

void Page::ensure_decompressed() const {
    if (is_compressed_) {
        const_cast<Page*>(this)->decompress();
    }
}

void Page::update_checksum() {
    checksum_ = calculate_checksum();
}

} // namespace nexusdb