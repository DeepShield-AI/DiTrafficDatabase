#ifndef FLOW_SENDER_HPP_
#define FLOW_SENDER_HPP_

#include <unordered_map>
#include "../lib/udpSender.hpp"
#include "../lib/singleRingBuffer.hpp"
#include "../lib/util.hpp"

class UDPFlowSender {
    PointerRingBuffer* indexRing;
    UDPSender* udpSender;

    std::unordered_map<u_int64_t, std::string>* shard_map; // read only

    u_int64_t threshold;

    std::atomic_bool stop;
    const u_int64_t node_id;

    char* buffer;
    u_int64_t data_len;
    u_int64_t index_count;

    std::string printIPv4(const void* addr_ptr);
    std::string printIPv6(const void* addr_ptr);

    u_int64_t calShardID(const Index* index);
    Index* readIndexFromBuffer();
    void processIndex(Index* index);
public:
    UDPFlowSender(PointerRingBuffer* ring, std::unordered_map<u_int64_t, std::string>* shard_map, u_int16_t dst_port, u_int64_t threshold, u_int64_t node_id);
    ~UDPFlowSender();
    void run();
    void asynchronousStop();
};

#endif