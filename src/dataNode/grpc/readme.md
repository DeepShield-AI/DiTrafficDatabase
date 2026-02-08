# gRPC Server
## 依赖安装
同 `src/interface/grpc/readme.md`

## Server端编译与测试
``` bash
cd src/dataNode/grpc
make
./interface_server
```

## 后续对齐
* 修改接收到请求时，dataNode中的处理逻辑以及发回回复的内容
* 参考 `src/protos/readme.md` 说明，后续与regionServer合并，将传送字符串修改为传送对象