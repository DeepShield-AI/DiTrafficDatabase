#ifndef HEARTBEAT_CLIENT_HPP
#define HEARTBEAT_CLIENT_HPP

#include <string>
#include <atomic>
#include <chrono>
#include <iostream>
#include "../lib/httplib.hpp"


class HeartbeatClient {
private:
    std::string controller_host;
    int controller_port;
    std::string node_id;

    std::atomic_bool stop;
    std::thread heartbeat_thread;

    u_int32_t interval_sec;  // 心跳周期（秒）

public:
    HeartbeatClient(const std::string& host,
                    int port,
                    const std::string& node_id,
                    u_int32_t interval_sec = 5);

    ~HeartbeatClient();

    void run();
    void asynchronousStop();
};

#endif