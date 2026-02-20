#include "indexer.hpp"

Indexer::Indexer(const std::string& name, const std::string& logPath, const u_int64_t id):DataNodeComponent("Flusher", name, logPath, id){
    this->workerPipe = nullptr;
}

IndexSignal* Indexer::getSignal(){
    void* data = this->workerPipe->get();
    if(data == nullptr){
        return nullptr;
    }
    IndexSignal* signal = (IndexSignal*)data;
    this->log("Received flush signal for region " + std::to_string(signal->regionID));
    return signal;
}

void Indexer::handleIndex(IndexSignal* signal){
    try{
        signal->index->appendMemtable(signal->memtable, signal->startTime, signal->endTime, signal->regionID);
        this->log("Build memtable of region " +  std::to_string(signal->regionID) + " to index successfully.");
    } catch (const std::runtime_error& e){
        this->log("Indexer error: " + std::string(e.what()));
        // std::cerr << "Flusher error: " << e.what() << std::endl;
    }
    delete signal;
}

void Indexer::init(DataNodeContext& cfg){
    this->workerPipe = cfg.workerFlusherPipe;
    this->log("init.");
    // this->sst = cfg.sst;
}

void Indexer::run(){
    this->start();
    this->log("run.");
    while(this->isRunning()){
        IndexSignal* signal = this->getSignal();
        if(signal == nullptr){
            continue;
        }
        this->handleIndex(signal);
    }
}