#ifndef NEXUSDB_SECURE_CONNECTION_MANAGER_H
#define NEXUSDB_SECURE_CONNECTION_MANAGER_H

#include <string>
#include <memory>
#include <vector>
#include <functional>

namespace nexusdb {

class SecureSocket {
public:
    virtual ~SecureSocket() = default;
    virtual void send(const std::vector<unsigned char>& data) = 0;
    virtual std::vector<unsigned char> receive() = 0;
    virtual void close() = 0;
};

class SecureConnectionManager {
public:
    SecureConnectionManager(const std::string& cert_file, const std::string& key_file);
    ~SecureConnectionManager();

    void start_server(int port, std::function<void(std::unique_ptr<SecureSocket>)> connection_handler);
    std::unique_ptr<SecureSocket> connect_to_server(const std::string& host, int port);

    void set_cipher_list(const std::string& ciphers);
    void set_protocol_version(const std::string& version);

private:
    class SecureConnectionManagerImpl;
    std::unique_ptr<SecureConnectionManagerImpl> pimpl_;
};

} // namespace nexusdb

#endif // NEXUSDB_SECURE_CONNECTION_MANAGER_H