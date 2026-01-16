#ifndef UTIL_HPP_
#define UTIL_HPP_
#include <vector>
#include <iostream>
#include <string>
#include "header.hpp"
#include "packetAggregator.hpp"

enum IndexType{
    SRCIP = 0,
    DSTIP,
    SRCPORT,
    DSTPORT,
    SRCIPv6,
    DSTIPv6,
    // QUARTURPLEIPv4,
    // QUARTURPLEIPv6,
    TOTAL_INDEX,
};

struct IPv6Address{
    u_int64_t low;
    u_int64_t high;

    bool operator<(const IPv6Address& other) const {
        if (high < other.high) {
            return true;
        } else if (high > other.high) {
            return false;
        }
        // 如果高位相等，比较低位
        return low < other.low;
    }
    // 重载比较运算符 ">"
    bool operator>(const IPv6Address& other) const {
        if (high > other.high) {
            return true;
        } else if (high < other.high) {
            return false;
        }
        // 如果高位相等，比较低位
        return low > other.low;
    }
    // 重载比较运算符 "=="
    bool operator==(const IPv6Address& other) const {
        return high == other.high && low == other.low;
    }
    // 重载比较运算符 "!="
    bool operator!=(const IPv6Address& other) const {
        return !(*this == other);
    }
    // 重载比较运算符 "<="
    bool operator<=(const IPv6Address& other) const {
        return !(*this > other);
    }
    // 重载比较运算符 ">="
    bool operator>=(const IPv6Address& other) const {
        return !(*this < other);
    }

    IPv6Address operator>>(unsigned int shift) const {
        IPv6Address result{0, 0};
        if (shift == 0) {
            result = *this;
        } else if (shift < 64) {
            result.low = (low >> shift) | (high << (64 - shift));
            result.high = high >> shift;
        } else if (shift < 128) {
            result.low = high >> (shift - 64);
            result.high = 0;
        } else {
            result.low = 0;
            result.high = 0;
        }
        return result;
    }

    IPv6Address& operator>>=(unsigned int shift) {
        *this = *this >> shift;
        return *this;
    }
};

struct QuarTurpleIPv4{
    u_int16_t dstport;
    u_int16_t srcport;
    u_int32_t dstip;
    u_int32_t srcip;

    bool operator<(const QuarTurpleIPv4& other) const {
        if (srcip < other.srcip) {
            return true;
        } else if (srcip > other.srcip) {
            return false;
        }

        if (dstip < other.dstip) {
            return true;
        } else if (dstip > other.dstip) {
            return false;
        }

        if (srcport < other.srcport) {
            return true;
        } else if (srcport > other.srcport) {
            return false;
        }
        // 如果高位相等，比较低位
        return dstport < other.dstport;
    }
    // 重载比较运算符 ">"
     bool operator>(const QuarTurpleIPv4& other) const {
        if (srcip > other.srcip) {
            return true;
        } else if (srcip < other.srcip) {
            return false;
        }

        if (dstip > other.dstip) {
            return true;
        } else if (dstip < other.dstip) {
            return false;
        }

        if (srcport > other.srcport) {
            return true;
        } else if (srcport < other.srcport) {
            return false;
        }
        // 如果高位相等，比较低位
        return dstport > other.dstport;
    }
    // 重载比较运算符 "=="
    bool operator==(const QuarTurpleIPv4& other) const {
        return srcip == other.srcip && dstip == other.dstip && srcport == other.srcport && dstport == other.dstport;
    }
    // 重载比较运算符 "!="
    bool operator!=(const QuarTurpleIPv4& other) const {
        return !(*this == other);
    }
    // 重载比较运算符 "<="
    bool operator<=(const QuarTurpleIPv4& other) const {
        return !(*this > other);
    }
    // 重载比较运算符 ">="
    bool operator>=(const QuarTurpleIPv4& other) const {
        return !(*this < other);
    }
};

struct QuarTurpleIPv6{
    u_int16_t dstport;
    u_int16_t srcport;
    IPv6Address dstip;
    IPv6Address srcip;

    bool operator<(const QuarTurpleIPv6& other) const {
        if (srcip < other.srcip) {
            return true;
        } else if (srcip > other.srcip) {
            return false;
        }

        if (dstip < other.dstip) {
            return true;
        } else if (dstip > other.dstip) {
            return false;
        }

        if (srcport < other.srcport) {
            return true;
        } else if (srcport > other.srcport) {
            return false;
        }
        // 如果高位相等，比较低位
        return dstport < other.dstport;
    }
    // 重载比较运算符 ">"
     bool operator>(const QuarTurpleIPv6& other) const {
        if (srcip > other.srcip) {
            return true;
        } else if (srcip < other.srcip) {
            return false;
        }

        if (dstip > other.dstip) {
            return true;
        } else if (dstip < other.dstip) {
            return false;
        }

        if (srcport > other.srcport) {
            return true;
        } else if (srcport < other.srcport) {
            return false;
        }
        // 如果高位相等，比较低位
        return dstport > other.dstport;
    }
    // 重载比较运算符 "=="
    bool operator==(const QuarTurpleIPv6& other) const {
        return srcip == other.srcip && dstip == other.dstip && srcport == other.srcport && dstport == other.dstport;
    }
    // 重载比较运算符 "!="
    bool operator!=(const QuarTurpleIPv6& other) const {
        return !(*this == other);
    }
    // 重载比较运算符 "<="
    bool operator<=(const QuarTurpleIPv6& other) const {
        return !(*this > other);
    }
    // 重载比较运算符 ">="
    bool operator>=(const QuarTurpleIPv6& other) const {
        return !(*this < other);
    }
};

#pragma pack(push,1)
struct IndexIPv4{
    u_int64_t len;
    u_int64_t ts;
    u_int64_t position;
    u_int64_t node_id;
    QuarTurpleIPv4 key;
};
#pragma pack(pop)

#pragma pack(push,1)
struct IndexIPv6{
    u_int64_t len;
    u_int64_t ts;
    u_int64_t position;
    u_int64_t node_id;
    QuarTurpleIPv6 key;
};
#pragma pack(pop)


struct Index{
    u_int64_t ts;
    u_int64_t position;
    u_int64_t node_id;
    FlowMetadata meta;
};

struct FlowIndex{
    u_int64_t ts;
    u_int64_t position;
    u_int64_t node_id;
    std::string key;
};

enum MessageType{
    SHARD_INFO = 0,
    ERROR_FLOW,
    TOTAL_MESSAGE,
};

struct ShardMessage{
    MessageType type = SHARD_INFO;
    u_int64_t shard_id;
    u_int64_t pod_id;
};

#endif