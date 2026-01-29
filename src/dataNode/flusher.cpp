#include "flusher.hpp"

Flusher::Flusher(const std::string& name, const std::string& logPath):DataNodeComponent("Flusher", name, logPath){
    this->workerPipe = nullptr;
}

FlushSignal* Flusher::getSignal(){
    void* data = this->workerPipe->get();
    if(data == nullptr){
        return nullptr;
    }
    FlushSignal* signal = (FlushSignal*)data;
    this->log("Received flush signal for region " + std::to_string(signal->regionID));
    return signal;
}

void Flusher::handleFlush(FlushSignal* signal){
    try{
        signal->sst->appendMemtable(signal->memtable, signal->startTime, signal->endTime, signal->regionID);
        this->log("Flushed memtable of region " +  std::to_string(signal->regionID) + " to SST successfully.");
    } catch (const std::runtime_error& e){
        this->log("Flusher error: " + std::string(e.what()));
        std::cerr << "Flusher error: " << e.what() << std::endl;
    }
    delete signal;
}

void Flusher::init(DataNodeContext& cfg){
    this->workerPipe = cfg.workerFlusherPipe;
    // this->sst = cfg.sst;
}

void Flusher::run(){
    this->start();
    while(this->isRunning()){
        FlushSignal* signal = this->getSignal();
        if(signal == nullptr){
            continue;
        }
        this->handleFlush(signal);
    }
}