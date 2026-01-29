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
    const void* jsonData;
    // std::unique_ptr<const u_int8_t[]> jsonData;
    u_int64_t jsonSize;
    ~WriteRequest(){
        delete[] (char*)jsonData;
    }
};

struct RegionAdminRequest {
    enum RegionOperation op;
    // 不同 op 可能需要不同参数
    union {
        struct {
            const void* name;
            u_int64_t name_size;
            const void* partition_expr;
            u_int64_t partition_expr_size;
            const void* attrs_json;
            u_int64_t attrs_json_size;
        } create;
        // unused now
        struct {
            bool force;
        } open;
        struct {
            bool force;
        } close;
        struct {
            bool keep_files;
        } drop;
    };
    ~RegionAdminRequest(){
        switch(op){
            case RegionOperation::CREATE:
                delete[] (char*)create.name;
                delete[] (char*)create.partition_expr;
                delete[] (char*)create.attrs_json;
                break;
            default:
                break;
        }
    }
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
