#include "shardHandler.hpp"

ShardHandler::ShardHandler(uint16_t listen_port, std::unordered_map<u_int64_t, std::string>* shard_map, u_int64_t buffer_size){
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

    this->shard_map = shard_map;
    this->buffer_size = buffer_size;
    this->buffer = new char[buffer_size];
    this->stop = true;
}

ShardHandler::~ShardHandler(){
    close(sockfd);
}

void ShardHandler::handleShardID(ShardMessage* msg){
    u_int64_t shard_id = msg->shard_id;
    u_int64_t pod_id = msg->pod_id;

    // 更新 shard_map
    (*shard_map)[shard_id] = this->index_prefix + std::to_string(pod_id);
    printf("[Receiver] Updated shard_id %lu to pod_id %lu\n", shard_id, pod_id);
}

void ShardHandler::run(){
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

        ShardMessage* msg = reinterpret_cast<ShardMessage*>(buffer);
        if (msg->type != MessageType::SHARD_INFO){
            printf("[Receiver] Received unknown message type %d\n", msg->type);
            continue;
        }

        handleShardID(msg);
    }
}

void ShardHandler::asynchronousStop() {
    this->stop = true;
}