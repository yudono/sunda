#ifndef SUNDA_TCP_SERVER_H
#define SUNDA_TCP_SERVER_H

#include <iostream>
#include <string>
#include <vector>
#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#else
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#endif
#include <fcntl.h>
#include <cstring>
#include <openssl/ssl.h>
#include <openssl/err.h>

namespace WebServer {

class TCPServer {
private:
    int server_fd = -1;
    int port = 3000;
    bool running = false;
    
    // SSL State
    SSL_CTX* ssl_ctx = nullptr;
    bool use_ssl = false;

    void init_openssl() {
        SSL_load_error_strings();
        OpenSSL_add_ssl_algorithms();
    }

    void cleanup_openssl() {
        EVP_cleanup();
    }

    SSL_CTX* create_context() {
        const SSL_METHOD* method = TLS_server_method();
        SSL_CTX* ctx = SSL_CTX_new(method);
        if (!ctx) {
            perror("Unable to create SSL context");
            ERR_print_errors_fp(stderr);
            return nullptr;
        }
        return ctx;
    }

    bool configure_context(SSL_CTX* ctx, const std::string& cert_file, const std::string& key_file) {
        if (SSL_CTX_use_certificate_file(ctx, cert_file.c_str(), SSL_FILETYPE_PEM) <= 0) {
            ERR_print_errors_fp(stderr);
            std::cerr << "Failed to load cert: " << cert_file << std::endl;
            return false;
        }
        if (SSL_CTX_use_PrivateKey_file(ctx, key_file.c_str(), SSL_FILETYPE_PEM) <= 0) {
            ERR_print_errors_fp(stderr);
            std::cerr << "Failed to load key: " << key_file << std::endl;
            return false;
        }
        return true;
    }

public:
    struct Client {
        int fd;
        SSL* ssl;
    };

    TCPServer() { init_openssl(); }
    ~TCPServer() { 
        stop(); 
        if (ssl_ctx) SSL_CTX_free(ssl_ctx);
        cleanup_openssl();
    }
    
    bool initSSL(const std::string& cert, const std::string& key) {
        ssl_ctx = create_context();
        if (!ssl_ctx) return false;
        
        if (!configure_context(ssl_ctx, cert, key)) {
            SSL_CTX_free(ssl_ctx);
            ssl_ctx = nullptr;
            return false;
        }

        use_ssl = true;
        return true;
    }

    bool start(int p) {
        port = p;
        server_fd = socket(AF_INET, SOCK_STREAM, 0);
        if (server_fd < 0) return false;

        // Allow address reuse
        int opt = 1;
#ifdef _WIN32
        setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, (const char*)&opt, sizeof(opt));
#else
        setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
#endif

        struct sockaddr_in address;
        address.sin_family = AF_INET;
        address.sin_addr.s_addr = INADDR_ANY;
        address.sin_port = htons(port);

        if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0) {
            std::cerr << "Bind failed: " << strerror(errno) << std::endl;
            close(server_fd);
            return false;
        }
        
        // Set receive timeout for accept and read
        struct timeval tv;
        tv.tv_sec = 5;
        tv.tv_usec = 0;
#ifdef _WIN32
        setsockopt(server_fd, SOL_SOCKET, SO_RCVTIMEO, (const char*)&tv, sizeof tv);
        setsockopt(server_fd, SOL_SOCKET, SO_SNDTIMEO, (const char*)&tv, sizeof tv);
#else
        setsockopt(server_fd, SOL_SOCKET, SO_RCVTIMEO, (const char*)&tv, sizeof tv);
        setsockopt(server_fd, SOL_SOCKET, SO_SNDTIMEO, (const char*)&tv, sizeof tv);
#endif

        if (listen(server_fd, 10) < 0) {
            close(server_fd);
            return false;
        }

        running = true;
        return true;
    }

    void stop() {
        if (server_fd >= 0) {
            close(server_fd);
            server_fd = -1;
        }
        running = false;
    }

    Client accept_connection() {
        if (!running) return {-1, nullptr};
        struct sockaddr_in client_addr;
        socklen_t client_len = sizeof(client_addr);
        int fd = accept(server_fd, (struct sockaddr *)&client_addr, &client_len);
        
        if (fd < 0) return {-1, nullptr};
        
        SSL* ssl = nullptr;
        if (use_ssl) {
            ssl = SSL_new(ssl_ctx);
            SSL_set_fd(ssl, fd);
            if (SSL_accept(ssl) <= 0) {
                ERR_print_errors_fp(stderr);
                SSL_free(ssl);
                close(fd);
                return {-1, nullptr};
            }
        }
        
        return {fd, ssl};
    }

    std::string read_request(Client client) {
        std::string raw_req;
        char buffer[4096];
        int valread = 0;
        
        // 1. Read Headers
        while (true) {
            memset(buffer, 0, sizeof(buffer));
            if (client.ssl) valread = SSL_read(client.ssl, buffer, sizeof(buffer) - 1);
            else valread = read(client.fd, buffer, sizeof(buffer) - 1);

            if (valread <= 0) break; 
            raw_req.append(buffer, valread);
            
            if (raw_req.find("\r\n\r\n") != std::string::npos) break;
        }
        
        // 2. Parse Content-Length
        size_t header_end = raw_req.find("\r\n\r\n");
        if (header_end != std::string::npos) {
            std::string headers = raw_req.substr(0, header_end);
            size_t cl_pos = headers.find("Content-Length: ");
            if (cl_pos != std::string::npos) {
                size_t start = cl_pos + 16;
                size_t end = headers.find("\r\n", start);
                int content_len = std::stoi(headers.substr(start, end - start));
                
                // 3. Read Remaining Body
                int body_received = raw_req.length() - (header_end + 4);
                int to_read = content_len - body_received;
                
                while (to_read > 0) {
                     memset(buffer, 0, sizeof(buffer));
                     if (client.ssl) valread = SSL_read(client.ssl, buffer, sizeof(buffer) - 1);
                     else valread = read(client.fd, buffer, sizeof(buffer) - 1);
                     
                     if (valread <= 0) break;
                     raw_req.append(buffer, valread);
                     to_read -= valread;
                }
            }
        }
        
        return raw_req;
    }
    
    void send_response(Client client, const std::string& response) {
        if (client.ssl) {
             SSL_write(client.ssl, response.c_str(), response.length());
        } else {
             write(client.fd, response.c_str(), response.length());
        }
    }

    void close_client(Client client) {
        if (client.ssl) {
             SSL_shutdown(client.ssl);
             SSL_free(client.ssl);
        }
        close(client.fd);
    }
};

} // namespace WebServer

#endif
