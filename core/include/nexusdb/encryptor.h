#ifndef NEXUSDB_ENCRYPTOR_H
#define NEXUSDB_ENCRYPTOR_H

#include <vector>
#include <string>
#include <memory>

namespace nexusdb {

class EncryptionKey {
public:
    EncryptionKey(const std::string& key);
    const std::vector<unsigned char>& get_raw_key() const;

private:
    std::vector<unsigned char> key_;
};

class Encryptor {
public:
    Encryptor(const EncryptionKey& key);
    ~Encryptor();

    std::vector<unsigned char> encrypt(const std::vector<unsigned char>& data);
    std::vector<unsigned char> decrypt(const std::vector<unsigned char>& encrypted_data);

    static EncryptionKey generate_key();

private:
    class EncryptorImpl;
    std::unique_ptr<EncryptorImpl> pimpl_;
};

} // namespace nexusdb

#endif // NEXUSDB_ENCRYPTOR_H