#ifndef TCP_SERVER_HPP_
#define TCP_SERVER_HPP_
#include <functional>
#include <unordered_map>
#include <atomic>
#include <thread>
#include <mutex>
#include <netinet/in.h>
#include <unistd.h>
#include <cstring>
#include <iostream>

class TcpServer {
public:
    using ConnId = uint64_t;

    using OnConnect    = std::function<void(ConnId)>;
    using OnDisconnect = std::function<void(ConnId)>;
    using OnMessage    = std::function<void(ConnId, const char*, size_t)>;

    TcpServer::TcpServer(uint16_t port): port_(port),listen_fd_(-1),running_(false),next_conn_id_(1) {}
    ~TcpServer(){stop();}

    bool start(){
        listen_fd_ = socket(AF_INET, SOCK_STREAM, 0);
        if (listen_fd_ < 0) return false;

        int opt = 1;
        setsockopt(listen_fd_, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

        sockaddr_in addr{};
        addr.sin_family = AF_INET;
        addr.sin_addr.s_addr = INADDR_ANY;
        addr.sin_port = htons(port_);

        if (bind(listen_fd_, (sockaddr*)&addr, sizeof(addr)) < 0) return false;
        if (listen(listen_fd_, 128) < 0) return false;

        running_ = true;
        std::thread(&TcpServer::acceptLoop, this).detach();
        return true;
    }
    void stop(){
        running_ = false;
        if (listen_fd_ >= 0) close(listen_fd_);
    }

    bool sendMessage(ConnId id, const void* data, size_t len){
        std::lock_guard<std::mutex> lk(conns_mtx_);
        auto it = conns_.find(id);
        if (it == conns_.end()) return false;
        return sendAll(it->second.fd, data, len);
    }

    void setOnConnect(OnConnect cb){
        on_connect_ = std::move(cb);
    }
    void setOnDisconnect(OnDisconnect cb){
        on_disconnect_ = std::move(cb);
    }
    void setOnMessage(OnMessage cb){
        on_message_ = std::move(cb);
    }
private:
    struct Connection {
        int fd;
    };

    uint16_t port_;
    int listen_fd_;
    std::atomic<bool> running_;
    std::atomic<ConnId> next_conn_id_;

    std::unordered_map<ConnId, Connection> conns_;
    std::mutex conns_mtx_;

    OnConnect on_connect_;
    OnDisconnect on_disconnect_;
    OnMessage on_message_;

    void acceptLoop(){
        while (running_) {
            int fd = accept(listen_fd_, nullptr, nullptr);
            if (fd < 0) continue;

            ConnId id = next_conn_id_++;

            {
                std::lock_guard<std::mutex> lk(conns_mtx_);
                conns_[id] = {fd};
            }

            if (on_connect_) on_connect_(id);

            std::thread(&TcpServer::clientLoop, this, id).detach();
        }
    }
    void clientLoop(ConnId id){
        char buf[4096];

        int fd;
        {
            std::lock_guard<std::mutex> lk(conns_mtx_);
            fd = conns_[id].fd;
        }

        while (running_) {
            size_t len = sizeof(buf);
            if (!recvSome(fd, buf, len)) break;

            if (on_message_) {
                on_message_(id, buf, len);
            }
        }

        {
            std::lock_guard<std::mutex> lk(conns_mtx_);
            close(conns_[id].fd);
            conns_.erase(id);
        }

        if (on_disconnect_) on_disconnect_(id);
    }

    static bool recvSome(int fd, char* buf, size_t& len){
        ssize_t n = recv(fd, buf, len, 0);
        if (n <= 0) return false;
        len = static_cast<size_t>(n);
        return true;
    }
    static bool sendAll(int fd, const void* buf, size_t len){
        const char* p = static_cast<const char*>(buf);
        while (len > 0) {
            ssize_t n = send(fd, p, len, 0);
            if (n <= 0) return false;
            p += n;
            len -= n;
        }
        return true;
    } 
};



#endif