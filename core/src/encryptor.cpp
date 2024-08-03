// File: src/encryptor.cpp
#include "nexusdb/encryptor.h"
#include <openssl/evp.h>
#include <openssl/rand.h>
#include <openssl/err.h>
#include <stdexcept>
#include <cstring>

namespace nexusdb {

// EncryptionKey implementation
EncryptionKey::EncryptionKey(const std::string& key) : key_(key.begin(), key.end()) {}

const std::vector<unsigned char>& EncryptionKey::get_raw_key() const {
    return key_;
}

// Encryptor implementation
class Encryptor::EncryptorImpl {
public:
    EncryptorImpl(const EncryptionKey& key) : key_(key) {
        OpenSSL_add_all_algorithms();
    }

    ~EncryptorImpl() {
        EVP_cleanup();
    }

    std::vector<unsigned char> encrypt(const std::vector<unsigned char>& data) {
        EVP_CIPHER_CTX* ctx = EVP_CIPHER_CTX_new();
        if (!ctx) {
            throw std::runtime_error("Failed to create cipher context");
        }

        std::vector<unsigned char> iv(EVP_MAX_IV_LENGTH);
        if (RAND_bytes(iv.data(), EVP_MAX_IV_LENGTH) != 1) {
            EVP_CIPHER_CTX_free(ctx);
            throw std::runtime_error("Failed to generate IV");
        }

        if (EVP_EncryptInit_ex(ctx, EVP_aes_256_cbc(), nullptr, key_.get_raw_key().data(), iv.data()) != 1) {
            EVP_CIPHER_CTX_free(ctx);
            throw std::runtime_error("Failed to initialize encryption");
        }

        std::vector<unsigned char> ciphertext(data.size() + EVP_MAX_BLOCK_LENGTH);
        int len;
        if (EVP_EncryptUpdate(ctx, ciphertext.data(), &len, data.data(), data.size()) != 1) {
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

        std::vector<unsigned char> result;
        result.reserve(iv.size() + ciphertext_len);
        result.insert(result.end(), iv.begin(), iv.end());
        result.insert(result.end(), ciphertext.begin(), ciphertext.begin() + ciphertext_len);

        return result;
    }

    std::vector<unsigned char> decrypt(const std::vector<unsigned char>& encrypted_data) {
        if (encrypted_data.size() <= EVP_MAX_IV_LENGTH) {
            throw std::runtime_error("Encrypted data is too short");
        }

        EVP_CIPHER_CTX* ctx = EVP_CIPHER_CTX_new();
        if (!ctx) {
            throw std::runtime_error("Failed to create cipher context");
        }

        std::vector<unsigned char> iv(encrypted_data.begin(), encrypted_data.begin() + EVP_MAX_IV_LENGTH);
        
        if (EVP_DecryptInit_ex(ctx, EVP_aes_256_cbc(), nullptr, key_.get_raw_key().data(), iv.data()) != 1) {
            EVP_CIPHER_CTX_free(ctx);
            throw std::runtime_error("Failed to initialize decryption");
        }

        std::vector<unsigned char> plaintext(encrypted_data.size() - EVP_MAX_IV_LENGTH);
        int len;
        if (EVP_DecryptUpdate(ctx, plaintext.data(), &len, encrypted_data.data() + EVP_MAX_IV_LENGTH, encrypted_data.size() - EVP_MAX_IV_LENGTH) != 1) {
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

        plaintext.resize(plaintext_len);
        return plaintext;
    }

private:
    EncryptionKey key_;
};

Encryptor::Encryptor(const EncryptionKey& key) : pimpl_(std::make_unique<EncryptorImpl>(key)) {}

Encryptor::~Encryptor() = default;

std::vector<unsigned char> Encryptor::encrypt(const std::vector<unsigned char>& data) {
    return pimpl_->encrypt(data);
}

std::vector<unsigned char> Encryptor::decrypt(const std::vector<unsigned char>& encrypted_data) {
    return pimpl_->decrypt(encrypted_data);
}

EncryptionKey Encryptor::generate_key() {
    std::vector<unsigned char> key(32); // 256 bits
    if (RAND_bytes(key.data(), key.size()) != 1) {
        throw std::runtime_error("Failed to generate random key");
    }
    return EncryptionKey(std::string(key.begin(), key.end()));
}

} // namespace nexusdb