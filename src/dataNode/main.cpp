#include "dataNode.hpp"

int main(int argc, char* argv[]){
    std::unordered_map<std::string,std::string> attrs = std::unordered_map<std::string,std::string>();

    attrs["serverEnginePipeCapacity"] = "1024";
    attrs["workerFlusherPipeCapacity"] = "1024";
    attrs["engineWorkerPipeCapacity"] = "1024";
    attrs["workerCount"] = "2";

    attrs["regionServerName"] = "server";
    attrs["MitoEngineName"] = "engine";
    attrs["RegionWorkerName"] = "worker";
    attrs["FlusherName"] = "flusher";
    attrs["logPath"] = "log/dataNode";
    attrs["fifo"] = "/tmp/datanode_input";

    DataNode node = DataNode();
    node.init(attrs);
    node.run();

    while (true){
        std::string op;
        std::cin >> op;
        if (op[0] =='q'){
            break;
        }
    }
    
    
    node.stop();
    node.clean();
    return 0;
}