#pragma once
#include <string>
#include <atomic>
#include <unordered_map>
#include <iostream>
#include "memtable.hpp"
#include "SST.hpp"
#include "region.hpp"

struct DataNodeContext{
    u_int64_t pipeSize; // in unit
};

class DataNodeComponent {
private:
    std::string componentType;
    std::string componentName;
    std::atomic_bool running;
public:
    DataNodeComponent(const std::string& type,const std::string& name) : componentType(type), componentName(name), running(false){}
    virtual ~DataNodeComponent() = default;
    virtual void init(DataNodeContext& cfg)=0;
    virtual void run()=0;
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
};
