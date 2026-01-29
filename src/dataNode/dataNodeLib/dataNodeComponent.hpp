#pragma once
#include <string>
#include <atomic>
#include <unordered_map>
#include <iostream>
#include "memtable.hpp"
#include "SST.hpp"
#include "region.hpp"

struct DataNodeContext{
    PipeRing* egineWorkerPipe;
    PipeRing* workerFlusherPipe;
};

class DataNodeComponent {
private:
    std::string componentType;
    std::string componentName;
    std::atomic_bool running;
    std::string logPath;
    std::ofstream logFile;
public:
    DataNodeComponent(const std::string& type,const std::string& name, const std::string& logPath) : componentType(type), componentName(name), logPath(logPath), running(false){
        this->logFile.open(this->logPath + "/" + this->componentType + "_log.txt", std::ios::app);
        if(!this->logFile.is_open()){
            throw std::runtime_error("Failed to open " + this->componentName + " log file");
        }
    }
    virtual ~DataNodeComponent(){
        if(this->logFile.is_open()){
            this->logFile.close();
        }
    }
    virtual void init(DataNodeContext& cfg)=0;
    virtual void run()=0;
    void start(){
        this->running.store(true);
    }
    void stop(){
        this->running.store(false);
    }
    std::string type() const{
        return this->componentType;
    }
    std::string name() const{
        return this->componentName;
    }
    bool isRunning() const{
        return this->running.load();
    }
    void log(const std::string& message){
        std::string log_message = "[" + this->componentName + "] " + message;
        if(this->logFile.is_open()){
            this->logFile << log_message << std::endl;
        }
    }
};
