#ifndef UDP_SENDER_HPP_
#define UDP_SENDER_HPP_
#include <iostream>
#include <cstring>
#include <arpa/inet.h>
#include <unistd.h>

class UDPSender {
public:
    UDPSender(uint16_t dst_port) {
        sockfd = socket(AF_INET, SOCK_DGRAM, 0);
        if (sockfd < 0) {
            perror("socket creation failed");
            exit(EXIT_FAILURE);
        }

        memset(&dest_addr, 0, sizeof(dest_addr));
        dest_addr.sin_family = AF_INET;
        dest_addr.sin_port = htons(dst_port);
    }

    ~UDPSender() {
        close(sockfd);
    }

    // 发送数据
    void sendData(const std::string& data, const std::string& dst_ip) {
        if (inet_pton(AF_INET, dst_ip.c_str(), &dest_addr.sin_addr) <= 0) {
            perror("invalid destination IP address");
            exit(EXIT_FAILURE);
        }
        ssize_t sent = sendto(sockfd, data.data(), data.size(), 0,
                              (struct sockaddr*)&dest_addr, sizeof(dest_addr));
        if (sent < 0) {
            perror("sendto failed");
        } 
        else {
            std::cout << "[Sender] Sent " << sent << " bytes to "
                      << inet_ntoa(dest_addr.sin_addr) << ":"
                      << ntohs(dest_addr.sin_port) << std::endl;
        }
    }

private:
    int sockfd;
    struct sockaddr_in dest_addr;
};
#endif