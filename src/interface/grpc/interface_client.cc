#include <grpcpp/grpcpp.h>

#include <iostream>
#include <memory>
#include <string>
#include <thread>
#include <chrono>

#include "build/interface.grpc.pb.h"

// gRPC Server ip及端口
const std::string ServerTarget = "localhost:50051";

using grpc::Channel;
using grpc::ClientContext;
using grpc::Status;
using interface::Interface;
using interface::InterfaceReply;
using interface::InterfaceRequest;

class InterfaceClient {
 public:
  InterfaceClient(std::shared_ptr<Channel> channel)
      : stub_(Interface::NewStub(channel)) {}

  // 异步：调用后立即返回，不阻塞。
  void SendRequest(const std::string& req_text) {
    // 为每一次RPC调用分配独立数据空间（保存在堆上）
    struct AsyncCall {
      InterfaceReply reply;
      ClientContext context;
      std::string original_req;
    };
    
    AsyncCall* call = new AsyncCall;
    call->original_req = req_text;

    InterfaceRequest request;
    request.set_request(req_text);

    stub_->async()->SendRequest(&call->context, &request, &call->reply, 
      [call](Status s) {
        // 回调逻辑：服务器响应时gRPC内部线程执行此处
        if (s.ok()) {
          std::cout << "\n[Response] For '" << call->original_req 
                    << "': " << call->reply.reply() << std::endl;
        } else {
          std::cout << "\n[RPC Failed] " << s.error_message() << std::endl;
        }       
        // 手动释放堆内存
        delete call;
        std::cout << "> " << std::flush;
      });
  }

 private:
  std::unique_ptr<Interface::Stub> stub_;
};

int main(int argc, char** argv) {
  // 实例化client
  InterfaceClient interface_client(
      grpc::CreateChannel(ServerTarget, grpc::InsecureChannelCredentials()));

  std::string req;
  std::cout << "Connected to " << ServerTarget << " (Async Callback Mode)" << std::endl;
  std::cout << "Enter request (type 'quit' to exit):" << std::endl;

  while (true) {
      std::cout << "> " << std::flush;
      if (!std::getline(std::cin, req) || req == "quit") {
          break;
      }
      if (req.empty()) continue;
      interface_client.SendRequest(req);
  }

  std::cout << "Waiting for pending callbacks to finish..." << std::endl;
  std::this_thread::sleep_for(std::chrono::seconds(1));
  std::cout << "Client exits" << std::endl;

  return 0;
}