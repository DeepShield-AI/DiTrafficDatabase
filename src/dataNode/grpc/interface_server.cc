#include <grpcpp/ext/proto_server_reflection_plugin.h>
#include <grpcpp/grpcpp.h>
#include <grpcpp/health_check_service_interface.h>

#include <iostream>
#include <memory>
#include <string>
#include <fstream>

#include "build/interface.grpc.pb.h"

using grpc::CallbackServerContext;
using grpc::Server;
using grpc::ServerBuilder;
using grpc::ServerUnaryReactor;
using interface::Interface;
using interface::InterfaceRequest;
using interface::InterfaceReply;

class InterfaceServiceImpl final : public Interface::CallbackService {
  ServerUnaryReactor* SendRequest(CallbackServerContext* context, 
                                 const InterfaceRequest* request,
                                 InterfaceReply* reply) override {
    // server 端结果处理
    std::ofstream outfile;
    outfile.open("received_requests.log", std::ios_base::app);
    if (outfile.is_open()) {
        outfile << "Client IP: " << context->peer() << " | ";
        outfile << "Content: " << request->request() << std::endl;
        outfile.close();
    }
    std::cout << "Received request from: " << context->peer() << std::endl;

    // client 端结果处理
    std::string prefix("Server Received (Async): ");
    reply->set_reply(prefix + request->request());

    ServerUnaryReactor* reactor = context->DefaultReactor();
    reactor->Finish(grpc::Status::OK);
    return reactor;
  }
};

void RunServer() {
  std::string server_address("0.0.0.0:50051");
  InterfaceServiceImpl service;

  grpc::EnableDefaultHealthCheckService(true);
  grpc::reflection::InitProtoReflectionServerBuilderPlugin();
  ServerBuilder builder;
  // Listen on the given address without any authentication mechanism.
  builder.AddListeningPort(server_address, grpc::InsecureServerCredentials());
  // Register "service" as the instance through which we'll communicate with
  // clients. In this case it corresponds to an *synchronous* service.
  builder.RegisterService(&service);
  // Finally assemble the server.
  std::unique_ptr<Server> server(builder.BuildAndStart());
  std::cout << "Interface Server listening on " << server_address << std::endl;

  // Wait for the server to shutdown. Note that some other thread must be
  // responsible for shutting down the server for this call to ever return.
  server->Wait();
}

int main(int argc, char** argv) {
  RunServer();
  return 0;
}
