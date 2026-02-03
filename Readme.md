# Readme

## DataNode编译运行方式

* 下列指令均在项目根目录中执行
* DataNode编译

```
cd src/dataNode
make clean
make
```

* DataNodeTest编译

```
cd src/test
g++ -std=c++17 -O2 -Wall dataNodeTest.cpp -o ../../build/test/dataNodeTest
```

* 运行
	* 正常情况下，dataNodeTest会直接退出，dataNode输入quit后退出
	* dataNode输出在`log/dataNode`文件夹下

```
mkfifo /tmp/datanode_input
./build/dataNode/dataNode
./build/test/dataNodeTest
```