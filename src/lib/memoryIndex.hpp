#ifndef MEMORY_INDEX_HPP_
#define MEMORY_INDEX_HPP_
#include <iostream>
#include <arpa/inet.h>
#include <limits>
#include "prefixBloomFilter.hpp"
#include "skipList.hpp"


class MemoryIndex{
    PrefixBloomFilter bloomFilterMeta;
    SkipList skipList[IndexType::TOTAL_INDEX];

    std::string printIPv4(const void* addr_ptr) {
        char str[INET_ADDRSTRLEN];  // IPv4 字符串长度
        const char* res = inet_ntop(AF_INET, addr_ptr, str, sizeof(str));
        return std::string(res);
    }

    // 打印 IPv6 地址
    std::string printIPv6(const void* addr_ptr) {
        char str[INET6_ADDRSTRLEN]; // IPv6 字符串长度
        const char* res = inet_ntop(AF_INET6, addr_ptr, str, sizeof(str));
        return std::string(res);
    }   
public:
    MemoryIndex(BitMap* bitmap, size_t numHashFunctions):
        bloomFilterMeta(bitmap, numHashFunctions),
        skipList{
            SkipList(sizeof(u_int32_t) * 8, sizeof(u_int32_t), sizeof(u_int64_t)), // SRCIP
            SkipList(sizeof(u_int32_t) * 8, sizeof(u_int32_t), sizeof(u_int64_t)), // DSTIP
            SkipList(sizeof(u_int16_t) * 8, sizeof(u_int16_t), sizeof(u_int64_t)), // SRCPORT
            SkipList(sizeof(u_int16_t) * 8 ,sizeof(u_int16_t), sizeof(u_int64_t)), // DSTPORT
            SkipList(sizeof(IPv6Address) * 8, sizeof(IPv6Address), sizeof(u_int64_t)), // SRCIPv6
            SkipList(sizeof(IPv6Address) * 8, sizeof(IPv6Address), sizeof(u_int64_t))  // DSTIPv6
        } 
        {
        this->bloomFilterMeta.setWritingCol(0);
        this->bloomFilterMeta.setReadingCol(0);
    }
    ~MemoryIndex(){}
    void insertIndex(FlowIndex* index){
        if(index->key.size() == sizeof(QuarTurpleIPv4)){
            QuarTurpleIPv4* key = (QuarTurpleIPv4*)&(index->key[0]);
            
            if(!bloomFilterMeta.insertIPv4(key->srcip, IndexType::SRCIP)){
                printf("MemoryIndex error: insert srcip %s failed!\n", printIPv4(&(key->srcip)).c_str());
                return;
            }
            skipList[IndexType::SRCIP].insert(std::string((char*)&key->srcip,sizeof(key->srcip)), index->position, std::numeric_limits<u_int64_t>::max());
            // printf("Insert srcipv4: %s\n",this->printIPv4(&(key->srcip)).c_str());

            if(!bloomFilterMeta.insertIPv4(key->dstip, IndexType::DSTIP)){
                printf("MemoryIndex error: insert dstip %s failed!\n", printIPv4(&(key->dstip)).c_str());
                return;
            }
            skipList[IndexType::DSTIP].insert(std::string((char*)&key->dstip,sizeof(key->dstip)), index->position, std::numeric_limits<u_int64_t>::max());
            // printf("Insert dstipv4: %s\n",this->printIPv4(&(key->dstip)).c_str());

            if(!bloomFilterMeta.insertPort(key->srcport, IndexType::SRCPORT)){
                printf("MemoryIndex error: insert srcport %u failed!\n", key->srcport);
                return;
            }
            skipList[IndexType::SRCPORT].insert(std::string((char*)&key->srcport,sizeof(key->srcport)), index->position, std::numeric_limits<u_int64_t>::max());

            if(!bloomFilterMeta.insertPort(key->dstport, IndexType::DSTPORT)){
                printf("MemoryIndex error: insert dstport %u failed!\n", key->dstport);
                return;
            }
            skipList[IndexType::DSTPORT].insert(std::string((char*)&key->dstport,sizeof(key->dstport)), index->position, std::numeric_limits<u_int64_t>::max());
        }else if(index->key.size() == sizeof(QuarTurpleIPv6)){
            QuarTurpleIPv6* key = (QuarTurpleIPv6*)&(index->key[0]);

            if(!bloomFilterMeta.insertIPv6(key->srcip, IndexType::SRCIPv6)){
                printf("MemoryIndex error: insert srcipv6 %s failed!\n", printIPv6(&key->srcip).c_str());
                return;
            }
            skipList[IndexType::SRCIPv6].insert(std::string((char*)&key->srcip,sizeof(key->srcip)), index->position, std::numeric_limits<u_int64_t>::max());
            // printf("Insert srcipv6: %s\n",this->printIPv6(&(key->srcip)).c_str());

            if(!bloomFilterMeta.insertIPv6(key->dstip, IndexType::DSTIPv6)){
                printf("MemoryIndex error: insert dstipv6 %s failed!\n", printIPv6(&key->dstip).c_str());
                return;
            }
            skipList[IndexType::DSTIPv6].insert(std::string((char*)&key->dstip,sizeof(key->dstip)), index->position, std::numeric_limits<u_int64_t>::max());
            // printf("Insert dstipv6: %s\n",this->printIPv6(&(key->dstip)).c_str());

            if(!bloomFilterMeta.insertPort(key->srcport, IndexType::SRCPORT)){
                printf("MemoryIndex error: insert srcport %u failed!\n", key->srcport);
                return;
            }
            skipList[IndexType::SRCPORT].insert(std::string((char*)&key->srcport,sizeof(key->srcport)), index->position, std::numeric_limits<u_int64_t>::max());

            if(!bloomFilterMeta.insertPort(key->dstport, IndexType::DSTPORT)){
                printf("MemoryIndex error: insert dstport %u failed!\n", key->dstport);
                return;
            }
            skipList[IndexType::DSTPORT].insert(std::string((char*)&key->dstport,sizeof(key->dstport)), index->position, std::numeric_limits<u_int64_t>::max());
        }
    }
    std::list<u_int32_t> searchIndex(IndexType type, const std::string& key){
        if(type == IndexType::SRCIP || type == IndexType::DSTIP){
            // printf("Search ipv4: %s\n",this->printIPv4(key.c_str()).c_str());
            if(!bloomFilterMeta.getIPv4(*((u_int32_t*)&(key[0])), type)){
                return std::list<u_int32_t>();
            }
            return skipList[type].findByKey(key);
        }else if(type == IndexType::SRCPORT || type == IndexType::DSTPORT){
            if(!bloomFilterMeta.getPort(*((u_int16_t*)&(key[0])), type)){
                return std::list<u_int32_t>();
            }
            return skipList[type].findByKey(key);
        }else if(type == IndexType::SRCIPv6 || type == IndexType::DSTIPv6){
            // printf("Search ipv6: %s\n",this->printIPv6(key.c_str()).c_str());
            if(!bloomFilterMeta.getIPv6(*((IPv6Address*)&(key[0])), type)){
                return std::list<u_int32_t>();
            }
            return skipList[type].findByKey(key);
        }
        return std::list<u_int32_t>();
    }
};

#endif