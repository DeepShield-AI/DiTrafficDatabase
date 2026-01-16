#pragma once
#include <string>
#include <atomic>
#include <unordered_map>
#include <iostream>

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
    virtual void stop()=0;
};
