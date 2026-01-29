#pragma once
#include "dataNodeLib/dataNodeComponent.hpp"
#include "../lib/pipeRing.hpp"
#include "dataNodeLib/dataNodeSignal.hpp"


class Flusher: public DataNodeComponent{
private:
    PipeRing* workerPipe;
    FlushSignal* getSignal();
    void handleFlush(FlushSignal* signal);
public:
    Flusher(const std::string& name, const std::string& logPath, const u_int64_t id);
    ~Flusher() = default;
    void init(DataNodeContext& cfg) override;
    void run() override;
};
