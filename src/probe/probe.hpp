#pragma once
#include <thread>
#include <fstream>
#include <vector>
#include <unordered_map>
#include "probeLib/probeComponent.hpp"

class Probe{
private:
    std::vector<std::unique_ptr<ProbeComponent>> components;
    std::vector<std::thread> componentThreads;
    ProbeContext cfg;
public:
    Probe();
    ~Probe() = default;
    void init(std::unordered_map<std::string, std::string>& attrs);
    void run();
    void stop();
    void clean();
};
