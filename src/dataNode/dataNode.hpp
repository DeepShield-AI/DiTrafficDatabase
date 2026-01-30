#pragma once
#include "regionServer.hpp"
#include "regionWorker.hpp"
#include "mitoEngine.hpp"
#include "flusher.hpp"
#include <thread>
#include <fstream>

class DataNode{
private:
    std::vector<std::unique_ptr<DataNodeComponent>> components;
    std::vector<std::thread> componentThreads;
    DataNodeContext cfg;
public:
    DataNode();
    ~DataNode() = default;
    void init(std::unordered_map<std::string, std::string>& attrs);
    void run();
    void stop();
    void clean();
};
