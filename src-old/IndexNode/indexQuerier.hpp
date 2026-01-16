#ifndef INDEX_QUERIER_HPP_
#define INDEX_QUERIER_HPP_
#include <vector>
#include <string>
#include <list>
#include <chrono>
#include <iostream>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <string.h>
#include <sstream>
#include <regex>
#include <arpa/inet.h>
#include "../lib/util.hpp"
#include "../lib/memoryIndex.hpp"

struct Query{
    IndexType type;
    std::string key;  
};




class IndexQuerier{
    MemoryIndex* memoryIndex;
    bool stop;

    IndexType parseField(const std::string& field);
    u_int32_t ipv4ToUint32(const std::string& ip);
    IPv6Address ipv6ToStruct(const std::string& ip);
    bool isIPv4(const std::string& ip);
    bool isIPv6(const std::string& ip);
    void printResult(std::list<u_int32_t>& result_list);
    bool parseQuery(std::string& query, Query& outQuery);
    void handleQuery(std::string& query);
public:
    IndexQuerier(MemoryIndex* memoryIndex);
    ~IndexQuerier();
    void run();
    void asynchronousStop();
};
#endif