#include "MetaServer.hpp"

grpc::Status IndexerServiceImpl::DoWork(grpc::ServerContext* context,
                                                    const demo::Empty* request,
                                                    demo::WorkResponse* response) {
    response->set_message("Hello from Indexer gRPC server!");
    return grpc::Status::OK;
}

GRPCServer::GRPCServer(const std::string& address)
    : server_address_(address), stop_(false) {}

GRPCServer::~GRPCServer() {
    asynchronousStop();
}

void GRPCServer::run() {
    IndexerServiceImpl service;
    grpc::ServerBuilder builder;
    builder.AddListeningPort(server_address_, grpc::InsecureServerCredentials());
    builder.RegisterService(&service);

    server_ = builder.BuildAndStart();
    std::cout << "gRPC Server listening on " << server_address_ << std::endl;

    if (server_) {
        server_->Wait();
    }
}

void GRPCServer::asynchronousStop() {
    if (server_) {
        server_->Shutdown();
    }
}
