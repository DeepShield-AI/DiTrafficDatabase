#ifndef META_SERVER_HPP_
#define META_SERVER_HPP_

#include <grpcpp/grpcpp.h>
#include "../lib/demo.grpc.pb.h"
#include <memory>
#include <thread>
#include <atomic>
#include <iostream>

class IndexerServiceImpl final : public demo::Indexer::Service {
public:
    grpc::Status DoWork(grpc::ServerContext* context,
                        const demo::Empty* request,
                        demo::WorkResponse* response) override;
};

class GRPCServer {
private:
    std::string server_address_;
    std::unique_ptr<grpc::Server> server_;
    std::atomic<bool> stop_;

public:
    GRPCServer(const std::string& address);
    ~GRPCServer();

    // 启动服务
    void run();

    // 停止服务
    void asynchronousStop();
};

#endif
