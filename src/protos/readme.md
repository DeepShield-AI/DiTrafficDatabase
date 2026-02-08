# gRPC 通信协议

## 第一版
* 异步回调的一元rpc
* request、reply均为字符串形式，拼接与解析均在interface与dataNode模块内部完成

## 第二版（后续完成）
* 以对象为传递单位，需将dataNode中regionServer的逻辑合并到gRPC通信的server中
* 可解决高速情况下json解析速度慢的潜在问题