#ifndef UDP_INDEX_RECEIVER_HPP_
#define UDP_INDEX_RECEIVER_HPP_

#include <iostream>
#include <cstring>
#include <arpa/inet.h>
#include <unistd.h>
#include "../lib/singleRingBuffer.hpp"
#include "../lib/util.hpp"

class UDPReceiver {
private:
    int sockfd;
    struct sockaddr_in local_addr;
    PointerRingBuffer* ring;
    char* buffer;
    u_int64_t buffer_size;

    bool stop;
    u_int64_t index_count = 0;

    std::string printIPv4(const void* addr_ptr);
    std::string printIPv6(const void* addr_ptr);

public:
    UDPReceiver(uint16_t listen_port, PointerRingBuffer* ring_buffer, u_int64_t buffer_size);
    ~UDPReceiver();
    void run();
    void asynchronousStop();
};

#endif