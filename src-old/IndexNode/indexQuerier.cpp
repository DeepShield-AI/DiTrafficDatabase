#include "indexQuerier.hpp"

IndexQuerier::IndexQuerier(MemoryIndex* memoryIndex){
    this->memoryIndex = memoryIndex;
    this->stop = true;
}
IndexQuerier::~IndexQuerier(){}

void IndexQuerier::printResult(std::list<u_int32_t>& result_list){
    printf("Find %lu flows.\n",result_list.size());
    for(auto x:result_list){
        printf("%u ",x);
    }
    printf("\n");
}
u_int32_t IndexQuerier::ipv4ToUint32(const std::string& ip) {
    struct in_addr addr{};
    if (inet_pton(AF_INET, ip.c_str(), &addr) != 1) {
        throw std::runtime_error("Invalid IPv4 address: " + ip);
    }
    // 网络字节序转为主机字节序
    return addr.s_addr;
}
IPv6Address IndexQuerier::ipv6ToStruct(const std::string& ip) {
    struct in6_addr addr6{};
    if (inet_pton(AF_INET6, ip.c_str(), &addr6) != 1) {
        throw std::runtime_error("Invalid IPv6 address: " + ip);
    }
    IPv6Address ipv6{};
    const uint8_t* bytes = reinterpret_cast<const uint8_t*>(&addr6);
    // 高 64 位
    for (int i = 15; i >= 8; --i) {
        ipv6.high = (ipv6.high << 8) | bytes[i];
    }
    // 低 64 位
    for (int i = 8; i >= 0 ; --i) {
        ipv6.low = (ipv6.low << 8) | bytes[i];
    }
    return ipv6;
}
bool IndexQuerier::isIPv4(const std::string& ip) {
    struct in_addr addr;
    return inet_pton(AF_INET, ip.c_str(), &addr) == 1;
}
bool IndexQuerier::isIPv6(const std::string& ip) {
    struct in6_addr addr6;
    return inet_pton(AF_INET6, ip.c_str(), &addr6) == 1;
}
IndexType IndexQuerier::parseField(const std::string& field) {
    if (field == "srcip") return IndexType::SRCIP;
    if (field == "dstip") return IndexType::DSTIP;
    if (field == "srcport") return IndexType::SRCPORT;
    if (field == "dstport") return IndexType::DSTPORT;
    return IndexType::TOTAL_INDEX;
}
bool IndexQuerier::parseQuery(std::string& query, Query& outQuery){
    std::regex pattern(R"(^\s*(srcip|dstip|srcport|dstport)\s*==\s*([^\s]+)\s*$)");
    std::smatch match;

    if (!std::regex_match(query, match, pattern)) {
        std::cerr << "Invalid query format: " << query << std::endl;
        return false;
    }

    std::string field_str = match[1];
    std::string value_str = match[2];

    outQuery.type = parseField(field_str);
    if (outQuery.type == IndexType::TOTAL_INDEX) {
        std::cerr << "Unknown field: " << field_str << std::endl;
        return false;
    }

    // 判断值类型
    if (outQuery.type == IndexType::SRCIP || outQuery.type == IndexType::DSTIP) {
        if (!isIPv4(value_str)) {
            if (isIPv6(value_str)) {
                outQuery.type = outQuery.type == IndexType::SRCIP ? IndexType::SRCIPv6 : IndexType::DSTIPv6;
                IPv6Address key = this->ipv6ToStruct(value_str);
                outQuery.key = std::string((char*)(&key),sizeof(key));
            }else {
                std::cerr << "Invalid IP address: " << value_str << std::endl;
                return false;
            }
        } else {
            u_int32_t key = this->ipv4ToUint32(value_str);
            outQuery.key = std::string((char*)(&key),sizeof(key));
        }
    } else {
        // 检查端口号
        try {
            int port = std::stoi(value_str);
            if (port < 0 || port > 65535) {
                std::cerr << "Invalid port range: " << port << std::endl;
                return false;
            }
            u_int16_t key = std::stoul(value_str);
            outQuery.key = std::string((char*)(&key),sizeof(key));
        } catch (...) {
            std::cerr << "Invalid port value: " << value_str << std::endl;
            return false;
        }
    }

    // outQuery.key = value_str;
    return true;
}
void IndexQuerier::handleQuery(std::string& query){
    Query q;
    if(!this->parseQuery(query,q)){
        // printf("error\n");
        return;
    }
    // printf("search\n");
    auto result = this->memoryIndex->searchIndex(q.type, q.key);
    this->printResult(result);
}
void IndexQuerier::run(){
    std::string query;
    this->stop = false;
    while(true){
        // std::cin >> query;
        std::getline(std::cin, query);
        if (query[0] == 'q'){
            this->stop = true;
            break;
        }
        this->handleQuery(query);
    }
    printf("Query stop.\n");
}
void IndexQuerier::asynchronousStop(){
    this->stop = true;
}
