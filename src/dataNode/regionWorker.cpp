#include "regionWorker.hpp"

RegionWorker::RegionWorker(const std::string& name, const std::string& logPath, const u_int64_t id):DataNodeComponent("RegionWorker", name, logPath, id){
    this->regions = std::unordered_map<u_int64_t, std::unique_ptr<Region>>();
    this->enginePipe = nullptr;
    this->flusherPipe = nullptr;
}

WorkSignal* RegionWorker::getSignal(){
    void* data = this->enginePipe->get();
    if(data == nullptr){
        return nullptr;
    }
    WorkSignal* signal = (WorkSignal*)data;
    this->log("Received send signal for region " + std::to_string(signal->regionID));
    return signal;
}

void RegionWorker::handleWrite(WriteRequest& request, u_int64_t regionID, u_int64_t requestID){
    json j = json::parse(std::string((const char*)request.jsonData, request.jsonSize));
    std::unordered_map<std::string, Value> fields;
    for (auto& el : j.items()){
        const std::string& key = el.key();
        if (el.value().is_number_unsigned()){
            fields[key] = el.value().get<u_int64_t>();
        } else if (el.value().is_number_integer()){
            fields[key] = el.value().get<int64_t>();
        } else if (el.value().is_number_float()){
            fields[key] = el.value().get<double>();
        } else if (el.value().is_string()){
            fields[key] = el.value().get<std::string>();
        } else {
            this->log("[Request " + std::to_string(requestID) + "]Unsupported data type for field " + key + " in region " + std::to_string(regionID));
            return;
        }
    }
    auto rowSize = this->regions[regionID]->getMemtable()->calRowSize(fields);
    if(rowSize.fixed_size + rowSize.var_size > std::stoull(this->regions[regionID]->getAtrr("memTableUsageThreshold"))){
        this->log("[Request " + std::to_string(requestID) + "]Write request row size " + std::to_string(rowSize.fixed_size + rowSize.var_size) + " exceeds memtable usage threshold for region " + std::to_string(regionID));
        return;
    }

    // TODO: WAL
    // ...

    if(this->checkRegionMemUsage(regionID) + rowSize.fixed_size + rowSize.var_size > std::stoull(this->regions[regionID]->getAtrr("memTableUsageThreshold"))){
        this->sendFlushSignal(regionID);
    }
    this->regions[regionID]->getMemtable()->write(request.timestamp, fields);
    this->regions[regionID]->getMemtable()->addRowSize(rowSize);
    this->log("[Request " + std::to_string(requestID) + "]Handled region write request for region " + std::to_string(regionID));
}

void RegionWorker::handleStatus(RegionAdminRequest& request, u_int64_t regionID, u_int64_t requestID){
    if(request.op == RegionOperation::CREATE){
        if (this->regions.find(regionID) != this->regions.end() && this->closedRegions.find(regionID) != this->closedRegions.end()){
            this->log("[Request " + std::to_string(requestID) + "]Region " + std::to_string(regionID) + " already exists.");
            return;
        }
        auto paras = request.create;
        json attrs_json = json::parse(std::string((const char*)paras.attrs_json, paras.attrs_json_size));
        std::unordered_map<std::string, std::string> attrs;
        for (auto& el : attrs_json.items()){
            attrs[el.key()] = el.value().get<std::string>();
        }
        std::unique_ptr<Region> newRegion = std::make_unique<Region>(
            regionID,
            std::string((const char*)paras.name, paras.name_size),
            std::string((const char*)paras.partition_expr, paras.partition_expr_size),
            attrs
        );
        std::shared_ptr<SST> sst = std::make_shared<SST>(newRegion->getAtrr("sstPath"));
        newRegion->setSST(sst);
        this->closedRegions[regionID] = std::move(newRegion);
        this->log("[Request " + std::to_string(requestID) + "]Created region " + std::to_string(regionID));
    } else if(request.op == RegionOperation::OPEN){
        if (this->regions.find(regionID) != this->regions.end()){
            this->log("[Request " + std::to_string(requestID) + "]Region " + std::to_string(regionID) + " opened yet.");
            return;
        }
        if (this->closedRegions.find(regionID) == this->closedRegions.end()){
            this->log("[Request " + std::to_string(requestID) + "]Region " + std::to_string(regionID) + " not exist.");
            return;
        }
        json colNames = json::parse(this->closedRegions[regionID]->getAtrr("columnNames"));
        std::unordered_map<std::string, ColumnType> columnNames;
        for (auto& el : colNames.items()){
            const std::string& key = el.key();
            const enum ColumnType& typeStr = (enum ColumnType)el.value().get<u_int64_t>();
        }
        std::shared_ptr<Memtable> memtable = std::make_shared<Memtable>(columnNames);
        this->closedRegions[regionID]->setMemtable(memtable);
        this->regions[regionID] = std::move(this->closedRegions[regionID]);
        this->closedRegions.erase(regionID);
        this->log("[Request " + std::to_string(requestID) + "]Opened region " + std::to_string(regionID));
    }else if(request.op == RegionOperation::CLOSE){
        if (this->regions.find(regionID) == this->regions.end()){
            this->log("[Request " + std::to_string(requestID) + "]Region " + std::to_string(regionID) + " not open or exist yet.");
            return;
        }
        auto memtable = this->regions[regionID]->getMemtable();
        if(memtable != nullptr && memtable->getRowCount() > 0){
            this->sendFlushSignal(regionID);
        }
        this->regions[regionID]->setMemtable(nullptr);
        this->closedRegions[regionID] = std::move(this->regions[regionID]);
        this->regions.erase(regionID);
        this->log("[Request " + std::to_string(requestID) + "]Closed region " + std::to_string(regionID));
    }else if(request.op == RegionOperation::DROP){
        if (this->regions.find(regionID) != this->regions.end()){
            this->log("[Request " + std::to_string(requestID) + "]Region " + std::to_string(regionID) + " opened yet.");
            return;
        }
        if (this->closedRegions.find(regionID) == this->closedRegions.end()){
            this->log("[Request " + std::to_string(requestID) + "]Region " + std::to_string(regionID) + " not exist.");
            return;
        }
        this->dropingRegions[regionID] = std::move(this->closedRegions[regionID]);
        this->closedRegions.erase(regionID);
        this->log("[Request " + std::to_string(requestID) + "]Dropping region " + std::to_string(regionID));
    }else{
        this->log("[Request " + std::to_string(requestID) + "]Unknown region operation for region " + std::to_string(regionID));
        return;
    }
    this->log("[Request " + std::to_string(requestID) + "]Handled region admin request for region " + std::to_string(regionID));
}

u_int64_t RegionWorker::checkRegionMemUsage(u_int64_t regionID){
    return this->regions[regionID]->getMemtable()->capacity() + this->regions[regionID]->getMemtable()->varCapacity();
}

void RegionWorker::sendFlushSignal(u_int64_t regionID){
    auto region = this->regions.find(regionID);
    if(region == this->regions.end()){
        this->log("Region " + std::to_string(regionID) + " not found for flush signal.");
        return;
    }
    auto memtable = region->second->getMemtable();
    if (memtable == nullptr){
        this->log("Region " + std::to_string(regionID) + " has no memtable to flush.");
        return;
    }
    auto sst = region->second->getSST();
    if (sst == nullptr){
        this->log("Region " + std::to_string(regionID) + " has no SST to flush to.");
        return;
    }
    memtable->freeze();
    std::shared_ptr<Memtable> newMemtable = std::make_shared<Memtable>(memtable->getColumnNames());
    region->second->setMemtable(newMemtable);
    FlushSignal* signal = new FlushSignal{
        .regionID = regionID,
        .startTime = memtable->getMinTime(),
        .endTime = memtable->getMaxTime(),
        .memtable = memtable,
        // .sst = sst,
    };
    this->flusherPipe->put((void*)signal);
    this->log("Sent flush signal for region " + std::to_string(regionID));
}

void RegionWorker::handleSignal(WorkSignal* signal){
    u_int64_t regionID = signal->regionID;
    Request& request = signal->request;
    auto region = this->regions.find(regionID);
    if(region == this->regions.end()){
        if (request.type == RequestType::REGION_ADMIN){
            auto exsitingClosedRegion = this->closedRegions.find(regionID);
            if (exsitingClosedRegion != this->closedRegions.end()){
                RegionAdminRequest adminRequest = std::get<RegionAdminRequest>(request.payload);
                this->handleStatus(adminRequest, regionID, request.header.request_id);
                delete signal;
                return;
            }
            RegionAdminRequest adminRequest = std::get<RegionAdminRequest>(request.payload);
            if (adminRequest.op == RegionOperation::CREATE){
                this->handleStatus(adminRequest, regionID, request.header.request_id);
                delete signal;
                return;
            }
        } 
        this->log("[Request " + std::to_string(request.header.request_id) + "]Region " + std::to_string(regionID) + " not found in working regions.");
        delete signal;
        return;
    }
    switch(request.type){
        case RequestType::WRITE:
            WriteRequest writeRequest = std::get<WriteRequest>(request.payload);
            this->handleWrite(writeRequest, regionID, request.header.request_id);
            break;
        case RequestType::QUERY:
            this->log("[Request " + std::to_string(request.header.request_id) + "]Query request handling not implemented yet for region " + std::to_string(regionID));
            break;
        case RequestType::REGION_ADMIN:
            RegionAdminRequest adminRequest = std::get<RegionAdminRequest>(request.payload);
            this->handleStatus(adminRequest, regionID, request.header.request_id);
            break;
        default:
            this->log("[Request " + std::to_string(request.header.request_id) + "]Unknown request type for region " + std::to_string(regionID));
            break;
    }
}

void RegionWorker::init(DataNodeContext& cfg){
    this->enginePipe = cfg.engineWorkerPipes[this->id()];
    this->flusherPipe = cfg.workerFlusherPipe;
    // this->memTableUsageThreshold = cfg.memTableUsageThreshold;
}

void RegionWorker::run(){
    this->start();
    while(this->isRunning()){
        WorkSignal* signal = this->getSignal();
        if(signal == nullptr){
            continue;
        }
        this->handleSignal(signal);
    }
}