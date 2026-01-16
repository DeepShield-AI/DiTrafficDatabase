#include "udpIndexReceiver.hpp"

std::string UDPReceiver::printIPv4(const void* addr_ptr) {
    char str[INET_ADDRSTRLEN];  // IPv4 字符串长度
    const char* res = inet_ntop(AF_INET, addr_ptr, str, sizeof(str));
    return std::string(res);
}

// 打印 IPv6 地址
std::string UDPReceiver::printIPv6(const void* addr_ptr) {
    char str[INET6_ADDRSTRLEN]; // IPv6 字符串长度
    const char* res = inet_ntop(AF_INET6, addr_ptr, str, sizeof(str));
    return std::string(res);
}

UDPReceiver::UDPReceiver(uint16_t listen_port, PointerRingBuffer* ring_buffer, u_int64_t buffer_size) {
    sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd < 0) {
        perror("socket creation failed");
        exit(EXIT_FAILURE);
    }

    memset(&local_addr, 0, sizeof(local_addr));
    local_addr.sin_family = AF_INET;
    local_addr.sin_addr.s_addr = INADDR_ANY; // 监听所有本地 IP
    local_addr.sin_port = htons(listen_port);

    if (bind(sockfd, (struct sockaddr*)&local_addr, sizeof(local_addr)) < 0) {
        perror("bind failed");
        close(sockfd);
        exit(EXIT_FAILURE);
    }
    printf("[Receiver] Listening on port %d ...\n", listen_port);

    this->ring = ring_buffer;
    this->buffer_size = buffer_size;
    this->buffer = new char[buffer_size];
    this->stop = true;
}

UDPReceiver::~UDPReceiver() {
    close(sockfd);
}

// 接收数据
void UDPReceiver::run() {
    // char buffer[2048];
    struct sockaddr_in src_addr;
    socklen_t addr_len = sizeof(src_addr);
    this->stop = false;

    while (!this->stop) {
        ssize_t recv_len = recvfrom(sockfd, buffer, buffer_size - 1, 0,
                                    (struct sockaddr*)&src_addr, &addr_len);
        if (recv_len < 0) {
            perror("recvfrom failed");
            break;
        }

        for(u_int64_t offset = 0;offset < (u_int64_t)recv_len;){
            FlowIndex* index = new FlowIndex();
            IndexIPv4* idv4 = (IndexIPv4*)(buffer + offset);
            index->position = idv4->position;
            index->ts = idv4->ts;
            index->node_id = idv4->node_id;
            if(idv4->len == sizeof(IndexIPv4)){
                index->key = std::string((char*)&(idv4->key), sizeof(QuarTurpleIPv4));
                printf("[Receiver] Received IPv4 FlowInfo: srcip=%s, dstip=%s, srcport=%u, dstport=%u\n",
                       printIPv4(&(idv4->key.srcip)).c_str(),
                       printIPv4(&(idv4->key.dstip)).c_str(),
                       ntohs(idv4->key.srcport),
                       ntohs(idv4->key.dstport));
            }else{
                IndexIPv6* idv6 = (IndexIPv6*)(buffer + offset);
                index->key = std::string((char*)&(idv6->key), sizeof(QuarTurpleIPv6));
                printf("[Receiver] Received IPv4 Index: srcip=%s, dstip=%s, srcport=%u, dstport=%u\n",
                       printIPv6(&(idv6->key.srcip)).c_str(),
                       printIPv6(&(idv6->key.dstip)).c_str(),
                       ntohs(idv4->key.srcport),
                       ntohs(idv4->key.dstport));
            }
            this->ring->put((void*)index);
            offset += (idv4->len);
            this->index_count ++;
            printf("[Receiver] Received indexes: %lu\n", this->index_count);
        }
    }
}

void UDPReceiver::asynchronousStop() {
    this->stop = true;
}