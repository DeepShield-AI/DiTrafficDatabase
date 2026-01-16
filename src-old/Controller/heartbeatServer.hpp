#ifndef HEARTBEAT_SERVER_HPP_
#define HEARTBEAT_SERVER_HPP_
#include "../lib/httplib.hpp"

#include <iostream>
#include <unordered_map>
#include <mutex>
#include <ctime>

using namespace httplib;

struct NodeStatus {
    std::time_t last_heartbeat;
    bool alive;
};

class HeartbeatServer{
private:
    std::unordered_map<std::string, NodeStatus> node_table;
    std::mutex node_mutex;

    u_int32_t heartbeatThreshold;
    bool stop;

public:
    HeartbeatServer(u_int32_t heartbeatThreshold);
    ~HeartbeatServer() = default;

    void handle_heartbeat(const Request& req, Response& res);
    void health_check_thread();
    void run();
    void asynchronousStop();
};

#endif