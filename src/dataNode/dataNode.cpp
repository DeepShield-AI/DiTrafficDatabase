#include "dataNode.hpp"

DataNode::DataNode(){
    this->components = std::vector<std::unique_ptr<DataNodeComponent>>();
    this->componentThreads = std::vector<std::thread>();
}

void DataNode::init(std::unordered_map<std::string, std::string>& attrs){
    try{
        PipeRing* serverEnginePipe = new PipeRing(stoull(attrs["serverEnginePipeCapacity"]));
        PipeRing* workerFlusherPipe = new PipeRing(stoull(attrs["workerFlusherPipeCapacity"]));
        u_int64_t workerCount = stoull(attrs["workerCount"]);
        PipeRing** engineWorkerPipes = new PipeRing*[workerCount];
        for (u_int64_t i = 0; i<workerCount; ++i){
            engineWorkerPipes[i] = new PipeRing(stoull(attrs["engineWorkerPipeCapacity"]));
        }
        DataNodeContext cfg = {
            .serverEnginePipe = serverEnginePipe,
            .workerFlusherPipe = workerFlusherPipe,
            .workerCount = workerCount,
            .engineWorkerPipes = engineWorkerPipes,
        };
        
        this->components.push_back(std::make_unique<RegionServer>(attrs["regionServerName"],attrs["logPath"],0));
        this->components.back()->init(cfg);

        this->components.push_back(std::make_unique<MitoEngine>(attrs["MitoEngineName"],attrs["logPath"],0));
        this->components.back()->init(cfg);

        for (u_int64_t i = 0; i<workerCount; ++i){
            this->components.push_back(std::make_unique<RegionWorker>(attrs["RegionWorkerName"],attrs["logPath"],i));
            this->components.back()->init(cfg);
        }

        this->components.push_back(std::make_unique<Flusher>(attrs["FlusherName"],attrs["logPath"],0));
        this->components.back()->init(cfg);

    } catch (const std::invalid_argument& e){
        std::cout << "Data node init failed with error " << std::string(e.what()) << std::endl;
    }
}

void DataNode::run(){
    this->componentThreads.clear();
    this->componentThreads.reserve(components.size());
    for (auto it = this->components.rbegin(); it != this->components.rend(); ++it) {
        DataNodeComponent* comp = it->get();

        this->componentThreads.emplace_back([comp]() {
            comp->run();
        });
    }    
}

void DataNode::stop(){
    for (auto& comp : this->components) {
        comp->stop();
    }

    // 等待所有线程退出
    for (auto& t : this->componentThreads) {
        if (t.joinable()) {
            t.join();
        }
    }

    this->componentThreads.clear();
}

void DataNode::clean(){
    this->components.clear();
}