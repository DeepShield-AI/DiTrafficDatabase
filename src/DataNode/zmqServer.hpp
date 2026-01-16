#ifndef ZMQ_SERVER_HPP_
#define ZMQ_SERVER_HPP_
#include <zmq.hpp>
#include <unordered_map>
#include <string>
#include "../lib/singleRingBuffer.hpp"

class ZmqReaderServer {
private:
    PointerRingBuffer* indexRing;
    std::atomic_bool stop;

    void handle_control();
    void send_flow_data();

    zmq::context_t ctx_;
    zmq::socket_t router_;

    std::unordered_map<uint32_t, std::string> shard_map_;
public:
    ZmqReaderServer(int port);
    ~ZmqReaderServer() = default;
    void run();
    void asynchronousStop();
};
#endif