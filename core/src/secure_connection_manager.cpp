// File: src/secure_connection_manager.cpp
#include "nexusdb/secure_connection_manager.h"
#include <openssl/ssl.h>
#include <openssl/err.h>
#include <stdexcept>
#include <cstring>
#include <thread>
#include <arpa/inet.h>
#include <unistd.h>

namespace nexusdb {

class OpenSSLSocket : public SecureSocket {
public:
    OpenSSLSocket(SSL* ssl, int socket_fd) : ssl_(ssl), socket_fd_(socket_fd) {}
    ~OpenSSLSocket() override {
        close();
    }

    void send(const std::vector<unsigned char>& data) override {
        if (SSL_write(ssl_, data.data(), data.size()) <= 0) {
            throw std::runtime_error("Failed to send data");
        }
    }

    std::vector<unsigned char> receive() override {
        std::vector<unsigned char> buffer(4096);
        int bytes_read = SSL_read(ssl_, buffer.data(), buffer.size());
        if (bytes_read <= 0) {
            throw std::runtime_error("Failed to receive data");
        }
        buffer.resize(bytes_read);
        return buffer;
    }

    void close() override {
        if (ssl_) {
            SSL_shutdown(ssl_);
            SSL_free(ssl_);
            ssl_ = nullptr;
        }
        if (socket_fd_ != -1) {
            ::close(socket_fd_);
            socket_fd_ = -1;
        }
    }

private:
    SSL* ssl_;
    int socket_fd_;
};

class SecureConnectionManager::SecureConnectionManagerImpl {
public:
    SecureConnectionManagerImpl(const std::string& cert_file, const std::string& key_file) 
        : ctx_(nullptr) {
        SSL_library_init();
        OpenSSL_add_all_algorithms();
        SSL_load_error_strings();

        ctx_ = SSL_CTX_new(TLS_method());
        if (!ctx_) {
            throw std::runtime_error("Failed to create SSL context");
        }

        if (SSL_CTX_use_certificate_file(ctx_, cert_file.c_str(), SSL_FILETYPE_PEM) <= 0) {
            throw std::runtime_error("Failed to load certificate file");
        }

        if (SSL_CTX_use_PrivateKey_file(ctx_, key_file.c_str(), SSL_FILETYPE_PEM) <= 0) {
            throw std::runtime_error("Failed to load private key file");
        }

        if (!SSL_CTX_check_private_key(ctx_)) {
            throw std::runtime_error("Private key does not match the certificate");
        }
    }

    ~SecureConnectionManagerImpl() {
        if (ctx_) {
            SSL_CTX_free(ctx_);
        }
        EVP_cleanup();
    }

    void start_server(int port, std::function<void(std::unique_ptr<SecureSocket>)> connection_handler) {
        int server_fd = socket(AF_INET, SOCK_STREAM, 0);
        if (server_fd < 0) {
            throw std::runtime_error("Failed to create socket");
        }

        sockaddr_in server_addr;
        std::memset(&server_addr, 0, sizeof(server_addr));
        server_addr.sin_family = AF_INET;
        server_addr.sin_addr.s_addr = INADDR_ANY;
        server_addr.sin_port = htons(port);

        if (bind(server_fd, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
            ::close(server_fd);
            throw std::runtime_error("Failed to bind to port");
        }

        if (listen(server_fd, SOMAXCONN) < 0) {
            ::close(server_fd);
            throw std::runtime_error("Failed to listen on socket");
        }

        while (true) {
            sockaddr_in client_addr;
            socklen_t client_len = sizeof(client_addr);
            int client_fd = accept(server_fd, (struct sockaddr*)&client_addr, &client_len);
            if (client_fd < 0) {
                continue;
            }

            SSL* ssl = SSL_new(ctx_);
            SSL_set_fd(ssl, client_fd);

            if (SSL_accept(ssl) <= 0) {
                SSL_free(ssl);
                ::close(client_fd);
                continue;
            }

            std::thread([this, ssl, client_fd, connection_handler]() {
                auto socket = std::make_unique<OpenSSLSocket>(ssl, client_fd);
                connection_handler(std::move(socket));
            }).detach();
        }
    }

    std::unique_ptr<SecureSocket> connect_to_server(const std::string& host, int port) {
        int socket_fd = socket(AF_INET, SOCK_STREAM, 0);
        if (socket_fd < 0) {
            throw std::runtime_error("Failed to create socket");
        }

        sockaddr_in server_addr;
        std::memset(&server_addr, 0, sizeof(server_addr));
        server_addr.sin_family = AF_INET;
        server_addr.sin_port = htons(port);

        if (inet_pton(AF_INET, host.c_str(), &server_addr.sin_addr) <= 0) {
            ::close(socket_fd);
            throw std::runtime_error("Invalid address");
        }

        if (connect(socket_fd, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
            ::close(socket_fd);
            throw std::runtime_error("Connection failed");
        }

        SSL* ssl = SSL_new(ctx_);
        SSL_set_fd(ssl, socket_fd);

        if (SSL_connect(ssl) <= 0) {
            SSL_free(ssl);
            ::close(socket_fd);
            throw std::runtime_error("SSL connection failed");
        }

        return std::make_unique<OpenSSLSocket>(ssl, socket_fd);
    }

    void set_cipher_list(const std::string& ciphers) {
        if (SSL_CTX_set_cipher_list(ctx_, ciphers.c_str()) <= 0) {
            throw std::runtime_error("Failed to set cipher list");
        }
    }

    void set_protocol_version(const std::string& version) {
        if (version == "TLSv1.2") {
            SSL_CTX_set_min_proto_version(ctx_, TLS1_2_VERSION);
        } else if (version == "TLSv1.3") {
            SSL_CTX_set_min_proto_version(ctx_, TLS1_3_VERSION);
        } else {
            throw std::runtime_error("Unsupported TLS version");
        }
    }

private:
    SSL_CTX* ctx_;
};

SecureConnectionManager::SecureConnectionManager(const std::string& cert_file, const std::string& key_file)
    : pimpl_(std::make_unique<SecureConnectionManagerImpl>(cert_file, key_file)) {}

SecureConnectionManager::~SecureConnectionManager() = default;

void SecureConnectionManager::start_server(int port, std::function<void(std::unique_ptr<SecureSocket>)> connection_handler) {
    pimpl_->start_server(port, std::move(connection_handler));
}

std::unique_ptr<SecureSocket> SecureConnectionManager::connect_to_server(const std::string& host, int port) {
    return pimpl_->connect_to_server(host, port);
}

void SecureConnectionManager::set_cipher_list(const std::string& ciphers) {
    pimpl_->set_cipher_list(ciphers);
}

void SecureConnectionManager::set_protocol_version(const std::string& version) {
    pimpl_->set_protocol_version(version);
}

} // namespace nexusdb