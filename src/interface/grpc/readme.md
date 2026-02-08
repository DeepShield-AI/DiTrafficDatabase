# gRPC Client

## 依赖安装
``` bash
sudo apt update
sudo apt install -y \
    build-essential \
    cmake \
    pkg-config \
    libgrpc++-dev \
    libprotobuf-dev \
    protobuf-compiler-grpc
```

## Client端编译与测试
``` bash
cd src/interface/grpc
make
./interface_client
```
* 键入字符串模拟发送请求（Request）
* 输出Server端异步回复（Reply）

## 后续对齐
* interface_client.cc 中修改目标服务器ip及端口（ServerTarget）
* 测试默认值为 "localhost:50051"
* 根据需求修改Server发回reply时的回调处理逻辑