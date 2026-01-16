#include "heartbeatServer.hpp"

HeartbeatServer::HeartbeatServer(u_int32_t heartbeatThreshold)
    : heartbeatThreshold(heartbeatThreshold), stop(true) {}

void HeartbeatServer::handle_heartbeat(const Request& req, Response& res) {
    std::string node_id = req.body;

    if (node_id.empty()) {
        res.status = 400;
        res.set_content("empty node_id", "text/plain");
        return;
    }
    {
        std::lock_guard<std::mutex> lock(node_mutex);
        auto& node = node_table[node_id];
        node.last_heartbeat = std::time(nullptr);
        node.alive = true;
    }

    std::cout << "[Heartbeat] received from " << node_id << std::endl;

    res.set_content("{\"status\":\"ok\"}", "application/json");
}

void HeartbeatServer::health_check_thread() {

    while (!stop) {
        {
            std::lock_guard<std::mutex> lock(node_mutex);
            auto now = std::time(nullptr);

            for (auto& [node_id, status] : node_table) {
                if (now - status.last_heartbeat > this->heartbeatThreshold) {
                    if (status.alive) {
                        std::cout << "[HealthCheck] node timeout: "
                                  << node_id << std::endl;
                    }
                    status.alive = false;
                }
            }
        }
        std::this_thread::sleep_for(std::chrono::seconds(5));
    }

    std::cout << "[HealthCheck] thread stopped" << std::endl;
}

void HeartbeatServer::run() {
    this->stop = false;
    Server server;

    /* 注册路由 */
    server.Post("/heartbeat",
        [this](const Request& req, Response& res) {
            this->handle_heartbeat(req, res);
        });

    server.Get("/nodes",
        [this](const Request&, Response& res) {
            std::lock_guard<std::mutex> lock(node_mutex);

            std::string body = "{ \"nodes\": [";
            bool first = true;
            for (const auto& [id, status] : node_table) {
                if (!first) body += ",";
                first = false;

                body += "{";
                body += "\"id\":\"" + id + "\",";
                body += "\"alive\":" + std::string(status.alive ? "true" : "false");
                body += "}";
            }
            body += "] }";

            res.set_content(body, "application/json");
        });

    
    /* 启动健康检查线程 */
    std::thread(&HeartbeatServer::health_check_thread, this).detach();

    std::cout << "[Controller] listening on 0.0.0.0:8080" << std::endl;

    server.listen("0.0.0.0", 8080);
}

/* =========================
 * 异步停止
 * ========================= */
void HeartbeatServer::asynchronousStop() {
    stop = true;
}
