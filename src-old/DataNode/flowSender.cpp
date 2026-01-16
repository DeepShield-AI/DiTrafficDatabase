#include "flowSender.hpp"

std::string UDPFlowSender::printIPv4(const void* addr_ptr) {
    char str[INET_ADDRSTRLEN];  // IPv4 字符串长度
    const char* res = inet_ntop(AF_INET, addr_ptr, str, sizeof(str));
    return std::string(res);
}

// 打印 IPv6 地址
std::string UDPFlowSender::printIPv6(const void* addr_ptr) {
    char str[INET6_ADDRSTRLEN]; // IPv6 字符串长度
    const char* res = inet_ntop(AF_INET6, addr_ptr, str, sizeof(str));
    return std::string(res);
}

UDPFlowSender::UDPFlowSender(PointerRingBuffer* ring, std::unordered_map<u_int64_t, std::string>* shard_map, u_int16_t dst_port, u_int64_t threshold, u_int64_t node_id):
    indexRing(ring), threshold(threshold), stop(false), node_id(node_id) {
    this->udpSender = new UDPSender(dst_port);
    this->buffer = new char[threshold];
    this->data_len = 0;
    this->stop = true;
    this->index_count = 0;
}
UDPFlowSender::~UDPFlowSender(){
    delete this->udpSender;
    delete[] this->buffer;
}

u_int64_t UDPFlowSender::calShardID(const Index* index){
    // TODO: implement shard ID calculation logic
    return 0;
}

Index* UDPFlowSender::readIndexFromBuffer(){
    void* data = this->indexRing->get();
    Index* index = (Index*)data;
    return index;
}
void UDPFlowSender::processIndex(Index* index){
    if(index->meta.sourceAddress.size() == 4){
        // IPv4
        IndexIPv4 idx_ipv4;
        idx_ipv4.len = sizeof(IndexIPv4);
        idx_ipv4.ts = index->ts;
        idx_ipv4.position = index->position;
        idx_ipv4.node_id = index->node_id;
        idx_ipv4.key.srcport = index->meta.sourcePort;
        idx_ipv4.key.dstport = index->meta.destinationPort;
        idx_ipv4.key.srcip = *(u_int32_t*)index->meta.sourceAddress.data();
        idx_ipv4.key.dstip = *(u_int32_t*)index->meta.destinationAddress.data();

        memcpy(this->buffer + this->data_len, &idx_ipv4, sizeof(IndexIPv4));
        this->data_len += sizeof(IndexIPv4);
        if(this->data_len + sizeof(IndexIPv4) > this->threshold){
            u_int64_t shard_id = this->calShardID(index);
            this->udpSender->sendData(std::string(this->buffer, this->data_len), (*shard_map)[shard_id]);
            this->data_len = 0;
        }
        printf("UDP Index Sender log: Sent IPv4 index %s:%u -> %s:%u position %lu\n", printIPv4(&idx_ipv4.key.srcip).c_str(), ntohs(idx_ipv4.key.srcport), printIPv4(&idx_ipv4.key.dstip).c_str(), ntohs(idx_ipv4.key.dstport), idx_ipv4.position);
    }else if(index->meta.sourceAddress.size() == 16){
        // IPv6
        IndexIPv6 idx_ipv6;
        idx_ipv6.len = sizeof(IndexIPv6);
        idx_ipv6.ts = index->ts;
        idx_ipv6.position = index->position;
        idx_ipv6.node_id = index->node_id;
        idx_ipv6.key.srcport = index->meta.sourcePort;
        idx_ipv6.key.dstport = index->meta.destinationPort;
        idx_ipv6.key.srcip = *(IPv6Address*)index->meta.sourceAddress.data();
        idx_ipv6.key.dstip = *(IPv6Address*)index->meta.destinationAddress.data();

        memcpy(this->buffer + this->data_len, &idx_ipv6, sizeof(IndexIPv6));
        this->data_len += sizeof(IndexIPv6);
        if(this->data_len + sizeof(IndexIPv6) > this->threshold){
            u_int64_t shard_id = this->calShardID(index);
            this->udpSender->sendData(std::string(this->buffer, this->data_len), (*shard_map)[shard_id]);
            this->data_len = 0;
        }

        printf("UDP Index Sender log: Sent IPv6 index %s:%u -> %s:%u position %lu\n", printIPv6(&idx_ipv6.key.srcip).c_str(), ntohs(idx_ipv6.key.srcport), printIPv6(&idx_ipv6.key.dstip).c_str(), ntohs(idx_ipv6.key.dstport), idx_ipv6.position);
    }
    this->index_count ++;
}
void UDPFlowSender::run(){
    this->stop = false;
    Index* data = nullptr;
    while (true){
        data = this->readIndexFromBuffer();
        if(this->stop){
            break;
        }
        if(data == nullptr){
            continue;
        }
        this->processIndex(data);
    }
    printf("UDP Index Sender log: stopping...\n");
    while(true){
        data = this->readIndexFromBuffer();
        if(data == nullptr){
            break;
        }
        this->processIndex(data);
    }
    
    u_int64_t shard_id = this->calShardID(data);
    if(this->data_len > 0){
        this->udpSender->sendData(std::string(this->buffer, this->data_len), (*shard_map)[shard_id]);
    }

    printf("UDP Index Sender log: sent %lu indexes\n", this->index_count);
}
void UDPFlowSender::asynchronousStop(){
    this->stop = true;
}