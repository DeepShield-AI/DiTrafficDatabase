#ifndef ZMQ_SERVER_HPP_
#define ZMQ_SERVER_HPP_
#include <zmq.hpp>
#include <string>
#include <unordered_map>
#include <iostream>

class ZmqServer {
private:
    zmq::context_t ctx_;
    zmq::socket_t router_;
public:
    ZmqServer(int io_threads): ctx_(io_threads),router_(ctx_, zmq::socket_type::router) {
        router_.set(zmq::sockopt::router_mandatory, 1);
        router_.set(zmq::sockopt::linger, 0);
    }
    ~ZmqServer() {
        router_.close();
        ctx_.close();
    }

    void bind(const std::string& endpoint){
        router_.bind(endpoint);
        std::cout << "[ZMQ] bind on " << endpoint << std::endl;
    }
    void poll_once(int timeout_ms = 0){
        zmq::pollitem_t items[] = {
            { router_, 0, ZMQ_POLLIN, 0 }
        };
        zmq::poll(items, 1, std::chrono::milliseconds(timeout_ms));
    }

    bool has_message() const {
        return true; // poll 后直接 recv（ZMQ 内部处理）
    }
    std::string recv(std::string& client_id){
        zmq::message_t identity;
        zmq::message_t msg;

        router_.recv(identity);
        router_.recv(msg);

        client_id.assign(static_cast<char*>(identity.data()), identity.size());
        return std::string(static_cast<char*>(msg.data()), msg.size());
    }
    void send(const std::string& client_id, const std::string& data){
        zmq::message_t identity(client_id.data(), client_id.size());
        zmq::message_t msg(data.data(), data.size());

        router_.send(identity, zmq::send_flags::sndmore);
        router_.send(msg, zmq::send_flags::none);
    }
};
#endif