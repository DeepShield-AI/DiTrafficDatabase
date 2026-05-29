#pragma once
#include <cstdlib>
#include <variant>

enum class RequestType{
    WRITE,
    QUERY,
    REGION_ADMIN,
};

enum class RegionOperation{
    CREATE,
    OPEN,
    CLOSE,
    DROP,
};

struct WriteRequest {
    u_int64_t timestamp;
    std::string jsonData;
};

struct CreateRegion {
    std::string name;
    std::string patition;
    std::string attrs_json;
};

struct OpenRegion {
    bool force;
};

struct CloseRegion{
    bool force;
};

struct DropRegion{
    bool keep_files;
};

using RegionOpPayload = std::variant<CreateRegion, OpenRegion, CloseRegion, DropRegion>;

struct RegionAdminRequest {
    enum RegionOperation op;
    // 不同 op 可能需要不同参数
    RegionOpPayload paras;
};

struct RequestHeader {
    u_int64_t request_id;     // 全局唯一，用于追踪
    u_int64_t timestamp_ns;   // 请求产生时间
};

struct Request {
    RequestHeader header;
    RequestType   type;
    std::variant<WriteRequest,RegionAdminRequest> payload;
};
