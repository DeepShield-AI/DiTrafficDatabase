#ifndef SHARD_HANDLER_HPP_
#define SHARD_HANDLER_HPP_

#include <iostream>
#include <cstring>
#include <arpa/inet.h>
#include <unistd.h>
#include <unordered_map>
#include "../lib/util.hpp"

class ShardHandler{
private:
    const std::string index_prefix = "shard_index_";
    std::unordered_map<u_int64_t, std::string>* shard_map;

    int sockfd;
    struct sockaddr_in local_addr;
    char* buffer;
    u_int64_t buffer_size;

    bool stop;

    void handleShardID(ShardMessage* msg);
public:
    ShardHandler(uint16_t listen_port, std::unordered_map<u_int64_t, std::string>* shard_map, u_int64_t buffer_size);
    ~ShardHandler();
    void run();
    void asynchronousStop();
};

#endif