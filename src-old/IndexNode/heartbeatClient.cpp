#include "heartbeatClient.hpp"

using namespace httplib;


HeartbeatClient::HeartbeatClient(const std::string& host,
                                 int port,
                                 const std::string& node_id,
                                 u_int32_t interval_sec)
    : controller_host(host),
      controller_port(port),
      node_id(node_id),
      interval_sec(interval_sec) {
        this->stop = true;
      }

HeartbeatClient::~HeartbeatClient() {
    this->asynchronousStop();
    if (heartbeat_thread.joinable()) {
        heartbeat_thread.join();
    }
}

void HeartbeatClient::asynchronousStop() {
    this->stop = true;
}

void HeartbeatClient::run() {
    this->stop = false;
    Client cli(controller_host, controller_port);

    cli.set_connection_timeout(2); // 秒
    cli.set_read_timeout(2);
    cli.set_write_timeout(2);

    while (!stop.load()) {
        auto res = cli.Post("/heartbeat",
                            node_id,
                            "text/plain");

        if (res && res->status == 200) {
            std::cout << "[HeartbeatClient] heartbeat ok" << std::endl;
        } else {
            std::cout << "[HeartbeatClient] heartbeat failed" << std::endl;
        }

        for (u_int32_t i = 0; i < interval_sec && !stop.load(); ++i) {
            std::this_thread::sleep_for(std::chrono::seconds(1));
        }
    }

    std::cout << "[HeartbeatClient] stopped" << std::endl;
}
