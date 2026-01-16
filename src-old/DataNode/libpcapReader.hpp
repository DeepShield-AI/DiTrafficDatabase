#ifndef LIBPCAP_READER_HPP_
#define LIBPCAP_READER_HPP_

#include <iostream>
#include <string>
#include <fstream>
#include <chrono>
#include <atomic>

#include <stdint.h>
#include <net/ethernet.h>
#include <netinet/ip.h>
#include <netinet/udp.h>
#include <arpa/inet.h>

#include <pcap.h>
#include <iostream>
#include <csignal>
#include <cstdint>
#include <cstring>

#include "../lib/packetAggregator.hpp"
#include "../lib/singleRingBuffer.hpp"
#include "../lib/header.hpp"
#include "../lib/util.hpp"

class LibpcapReader{
    std::string interface_name;
    std::string file_name;

    pcap_t* handle;
    pcap_dumper_t* dumper;
    FILE* file;

    u_int64_t byte_count;
    u_int64_t offset_threshold;
    const u_int64_t node_id;

    PacketAggregator* packetAggregator;
    PointerRingBuffer* indexRing;

    std::string printIPv4(const void* addr_ptr);
    std::string printIPv6(const void* addr_ptr);
    static void packetCallback(u_char* user, const struct pcap_pkthdr* hdr, const u_char* pkt);
    void processPacket(const struct pcap_pkthdr* hdr, const u_char* pkt);
    FlowMetadata getFlowMetaData(const struct pcap_pkthdr* hdr, const u_char* pkt);
    u_int64_t calDiff(u_int64_t offset, u_int64_t last_offset);
    void writeBefore(u_int64_t packet_header_offset, uint32_t new_value);
    u_int64_t calValue(u_int64_t _offset);
    bool writeIndexToRing(u_int64_t value, FlowMetadata& meta, u_int64_t ts);

public:
    LibpcapReader(const std::string& iface, const std::string& fname, u_int64_t offset_threshold, u_int64_t node_id, PointerRingBuffer* ring);
    ~LibpcapReader();
    void run();
    void asynchronousStop();
};


#endif