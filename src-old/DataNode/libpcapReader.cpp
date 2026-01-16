#include "libpcapReader.hpp"

static uint64_t swap_endianness(uint64_t value) {
    return ((value >> 56) & 0x00000000000000FFULL) | // byte 0
           ((value >> 40) & 0x000000000000FF00ULL) | // byte 1
           ((value >> 24) & 0x00000000FF000000ULL) | // byte 2
           ((value >> 8)  & 0x00FF000000000000ULL) | // byte 3
           ((value << 8)  & 0xFF00000000000000ULL) | // byte 4
           ((value << 24) & 0x0000FF0000000000ULL) | // byte 5
           ((value << 40) & 0x000000FF00000000ULL) | // byte 6
           ((value << 56) & 0x00000000000000FFULL);   // byte 7
}

std::string LibpcapReader::printIPv4(const void* addr_ptr) {
    char str[INET_ADDRSTRLEN];  // IPv4 字符串长度
    const char* res = inet_ntop(AF_INET, addr_ptr, str, sizeof(str));
    return std::string(res);
}

// 打印 IPv6 地址
std::string LibpcapReader::printIPv6(const void* addr_ptr) {
    char str[INET6_ADDRSTRLEN]; // IPv6 字符串长度
    const char* res = inet_ntop(AF_INET6, addr_ptr, str, sizeof(str));
    return res;
}

LibpcapReader::LibpcapReader(const std::string& iface, const std::string& fname, u_int64_t offset_threshold, u_int64_t node_id, PointerRingBuffer* ring):
    interface_name(iface), file_name(fname), handle(nullptr), dumper(nullptr), file(nullptr), node_id(node_id), indexRing(ring) {
    char errbuf[PCAP_ERRBUF_SIZE];
    this->handle = pcap_open_live(interface_name.c_str(), BUFSIZ, 1, 1000, errbuf);
    if (handle == nullptr) {
        std::cerr << "Could not open device " << interface_name << ": " << errbuf << std::endl;
        exit(1);
    }
    this->dumper = pcap_dump_open(handle, file_name.c_str());
    if (this->dumper == nullptr) {
        std::cerr << "Could not open dump file " << file_name << ": " << pcap_geterr(handle) << std::endl;
        pcap_close(handle);
        exit(1); 
    }
    this->file = (FILE*)pcap_dump_file(this->dumper);
    this->byte_count = 0;

    this->packetAggregator = new PacketAggregator(offset_threshold, std::numeric_limits<uint64_t>::max());
    // this->indexRing = ring;
    this->offset_threshold = offset_threshold;
    
}

LibpcapReader::~LibpcapReader(){
    if (dumper) {
        pcap_dump_close(dumper);
    }
    if (handle) {
        pcap_close(handle);
    }
    this->file = nullptr;
    delete this->packetAggregator;
}

void LibpcapReader::packetCallback(u_char* user, const struct pcap_pkthdr* hdr, const u_char* pkt) {
    auto* self = reinterpret_cast<LibpcapReader*>(user);
    if (!self || !self->dumper) {
        return;
    }
    self->processPacket(hdr, pkt);
}

void LibpcapReader::processPacket(const struct pcap_pkthdr* hdr, const u_char* pkt){
    if (!this->dumper || !this->file) return;
    u_int64_t offset = ftell(file);  // 当前包在文件中的偏移

    FlowMetadata flow_meta = this->getFlowMetaData(hdr, pkt);

    if(flow_meta.sourceAddress.size() == 0){
        printf("Libpcap Reader error: Non-IP L3 protocol!\n");
        return;
    }

    u_int64_t timestamp = static_cast<u_int64_t>(hdr->ts.tv_sec) * 1000000 + hdr->ts.tv_usec;

    u_int64_t last = this->packetAggregator->addPacket(flow_meta,offset,timestamp, std::numeric_limits<u_int64_t>::max());
    if(last != std::numeric_limits<uint64_t>::max()){
                
        u_int32_t diff = (u_int32_t)this->calDiff(offset,last);
        this->writeBefore(diff,last);
    }else{

        u_int64_t value = this->calValue(offset);
        if(!this->writeIndexToRing(value,flow_meta,timestamp)){
            printf("Libpcap Reader error: write index to ring failed!\n");
        }
    }

    pcap_dump((u_char*)dumper, hdr, pkt);

    printf("Libpcap Reader log: Processed packet original offset %lu, last offset %lu\n", offset, last);
    if(flow_meta.sourceAddress.size()==4){
        printf("Libpcap Reader log: Processed packet %s:%u -> %s:%u original offset %lu\n",  printIPv4(flow_meta.sourceAddress.data()).c_str(), flow_meta.sourcePort, printIPv4(flow_meta.destinationAddress.data()).c_str(), flow_meta.destinationPort, offset);
    }else if(flow_meta.sourceAddress.size()==16){
        printf("Libpcap Reader log: Processed packet %s:%u -> %s:%u original offset %lu\n",  printIPv6(flow_meta.sourceAddress.data()).c_str(), flow_meta.sourcePort, printIPv6(flow_meta.destinationAddress.data()).c_str(), flow_meta.destinationPort, offset);
    }

    this->byte_count += hdr->caplen + sizeof(struct pcap_pkthdr);
    if (this->byte_count >= this->offset_threshold){
        fflush(file);
        this->byte_count = 0;
    }
    // fflush(file);
}

FlowMetadata LibpcapReader::getFlowMetaData(const struct pcap_pkthdr* hdr, const u_char* pkt){
    FlowMetadata meta;
    uint8_t version = (*(u_int8_t*)(pkt + ETHER_HEADER_LEN) >> 4) & 0x0F;
    if(version == 4){
        const struct ip_header* ip_protocol = (const struct ip_header *)(pkt + ETHER_HEADER_LEN);
        const u_int16_t* sport = (const u_int16_t*)(pkt + ETHER_HEADER_LEN + ip_protocol->ip_header_length * 4);
        const u_int16_t* dport = sport + 1;
        u_int32_t srcip = htonl(ip_protocol->ip_source_address);
        u_int32_t dstip = htonl(ip_protocol->ip_destination_address);
        FlowMetadata flow_meta = {
            .sourceAddress = std::string((char*)&srcip,sizeof(srcip)),
            .destinationAddress = std::string((char*)&dstip,sizeof(dstip)),
            .sourcePort = htons(*sport),
            .destinationPort = htons(*dport),
        };
        // printf("Parse packet %s:%u -> %s:%u \n",  printIPv4(flow_meta.sourceAddress.data()).c_str(), flow_meta.sourcePort, printIPv4(flow_meta.destinationAddress.data()).c_str(), flow_meta.destinationPort);
        return flow_meta;
    }else if(version == 6){
        const u_int16_t* sport = (const u_int16_t*)(pkt + ETHER_HEADER_LEN + IPV6_HEADER_LEN);
        const u_int16_t* dport = sport + 1;
        IPv6Address srcip = {
            .low = swap_endianness(*(u_int64_t*)(pkt + ETHER_HEADER_LEN + 16)),
            .high = swap_endianness(*(u_int64_t*)(pkt + ETHER_HEADER_LEN + 8)),
        };
        IPv6Address dstip = {
            .low = swap_endianness(*(u_int64_t*)(pkt + ETHER_HEADER_LEN + 32)),
            .high = swap_endianness(*(u_int64_t*)(pkt + ETHER_HEADER_LEN + 24)),
        };
        FlowMetadata flow_meta = {
            .sourceAddress = std::string((char*)&srcip,sizeof(srcip)),
            .destinationAddress = std::string((char*)&dstip,sizeof(dstip)),
            .sourcePort = htons(*sport),
            .destinationPort = htons(*dport),
        };
        return flow_meta;
    }
    FlowMetadata flow_meta = {
        .sourceAddress = std::string(),
        .destinationAddress = std::string(),
        .sourcePort = 0,
        .destinationPort = 0,
    };
    return flow_meta;
}

u_int64_t LibpcapReader::calDiff(u_int64_t offset, u_int64_t last_offset){
    return offset - last_offset;
}
void LibpcapReader::writeBefore(u_int64_t packet_header_offset, uint32_t new_value){
    u_int64_t orig_len_offset = packet_header_offset + 12; // record header 中 orig_len 偏移
    if (fseek(this->file, orig_len_offset, SEEK_SET) != 0) return;
    fseek(this->file, 0, SEEK_END);
    return;
}
u_int64_t LibpcapReader::calValue(u_int64_t _offset){
    return (this->node_id << 48) | (_offset & 0x0000FFFFFFFFFFFFULL);
}
bool LibpcapReader::writeIndexToRing(u_int64_t value, FlowMetadata& meta, u_int64_t ts){
    Index* index = new Index();
    index->ts = ts;
    index->position = value;
    index->meta = meta;
    index->node_id = this->node_id;

    if(!this->indexRing->put((void*)index)){
        return false;
    }
    return true;
}

void LibpcapReader::run(){
    if (!this->handle) return;
    pcap_loop(this->handle, 0, LibpcapReader::packetCallback, reinterpret_cast<u_char*>(this));
    fflush(this->file);
}

void LibpcapReader::asynchronousStop(){
    if (this->handle) {
        pcap_breakloop(this->handle);
    }
}