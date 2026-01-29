#include "mitoEngine.hpp"

MitoEngine::MitoEngine(const std::string& name, const std::string& logPath, const u_int64_t id):DataNodeComponent("MitoEngine", name, logPath, id){
    this->serverPipe = nullptr;
    this->workerPipes = std::vector<PipeRing*>();
}
u_int64_t MitoEngine::regionIDToWorkerID(u_int64_t regionID, u_int64_t workerCount){
    return regionID % workerCount;
}
MitoSignal* MitoEngine::getSignal(){
    void* data = this->serverPipe->get();
    if(data == nullptr){
        return nullptr;
    }
    MitoSignal* signal = (MitoSignal*)data;
    this->log("Received send signal for region " + std::to_string(signal->regionID));
    return signal;
}
void MitoEngine::handleSignal(MitoSignal* signal){
    auto workerID = this->regionIDToWorkerID(signal->regionID, this->workerPipes.size());
    WorkSignal* workSignal = new WorkSignal({
        .regionID = signal->regionID,
        .request = signal->request,
    });
    this->workerPipes[workerID]->put((void*)signal);
    this->log("Dispatched signal for region " + std::to_string(signal->regionID) + " to worker " + std::to_string(workerID));
    delete signal;
}
void MitoEngine::init(DataNodeContext& cfg){
    this->serverPipe = cfg.serverEnginePipe;
    for(u_int64_t i = 0;i<cfg.workerCount;++i){
        this->workerPipes.push_back(cfg.engineWorkerPipes[i]);
    }
}
void MitoEngine::run(){
    this->start();
    while(this->isRunning()){
        MitoSignal* signal = this->getSignal();
        if(signal == nullptr){
            continue;
        }
        this->handleSignal(signal);
    }
}