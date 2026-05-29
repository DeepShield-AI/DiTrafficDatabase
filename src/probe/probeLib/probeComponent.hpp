#pragma once
#include <string>
#include <atomic>
#include <unordered_map>
#include <iostream>
#include <fstream>
#include "../../lib/pipeRing.hpp"

#include "diskAgent.hpp"
#include "dataBlockBuffer.hpp"
#include "dpdk.hpp"
#include "packetAggregator.hpp"
#include "packetParser.hpp"

struct ProbeContext{
    PipeRing* serverEnginePipe;
    PipeRing* workerFlusherPipe;
    u_int64_t workerCount;
    PipeRing** engineWorkerPipes;
    std::istream* in;
};

class ProbeComponent {
private:
    std::string componentType;
    std::string componentName;
    u_int64_t componentID;
    std::string logPath;
    std::atomic_bool running;
    std::ofstream logFile;
public:
    ProbeComponent(const std::string& type,const std::string& name, const std::string& logPath, const u_int64_t id) : componentType(type), componentName(name),componentID(id), logPath(logPath), running(false){
        this->logFile.open(this->logPath + "/" + this->componentType + ".log", std::ios::app);
        // this->logFile.open(this->logPath + "/" + this->componentType + ".log");
        if(!this->logFile.is_open()){
            throw std::runtime_error("Failed to open " + this->componentName + " log file");
        }
    }
    virtual ~ProbeComponent(){
        if(this->logFile.is_open()){
            this->logFile.close();
        }
    }
    virtual void init(ProbeContext& cfg)=0;
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
    std::string path() const {
        return this->logPath;
    }
    u_int64_t id() const{
        return this->componentID;
    }
    bool isRunning() const{
        return this->running.load();
    }
    void log(const std::string& message){
        std::string log_message = "[" + this->componentName + std::to_string(this->componentID) + "] " + message;
        if(this->logFile.is_open()){
            this->logFile << log_message << std::endl;
        }
    }
};
