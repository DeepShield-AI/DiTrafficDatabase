#include <fstream>
#include <thread>
#include <chrono>
#include <nlohmann/json.hpp>
#include "../dataNode/dataNodeLib/memtable.hpp"

using json = nlohmann::json;

json createRegion(u_int64_t regionID, std::string name, std::string expr){
    json request;
    request["type"] = "ADMIN";
    request["regionID"] = std::to_string(regionID);
    request["operation"] = "CREATE";
    request["regionName"] = name;
    request["expr"] = expr;
    
    json attrs;

    attrs["memTableUsageThreshold"] = std::to_string(1024*1024);
    attrs["sstPath"] = "./data";

    json column_names;
    column_names["id"] = (u_int64_t)ColumnType::UINT64;
    column_names["value"] = (u_int64_t)ColumnType::DOUBLE;
    column_names["description"] = (u_int64_t)ColumnType::STRING;

    attrs["columnNames"] = column_names.dump();

    request["attrs"] = attrs.dump();

    return request;
}

json openRegion(u_int64_t regionID){
    json request;
    request["type"] = "ADMIN";
    request["regionID"] = std::to_string(regionID);
    request["operation"] = "OPEN";
    return request;
}

json closeRegion(u_int64_t regionID){
    json request;
    request["type"] = "ADMIN";
    request["regionID"] = std::to_string(regionID);
    request["operation"] = "CLOSE";
    return request;
}

json dropRegion(u_int64_t regionID){
    json request;
    request["type"] = "ADMIN";
    request["regionID"] = std::to_string(regionID);
    request["operation"] = "DROP";
    return request;
}

int main() {
    std::ofstream out("/tmp/datanode_input");

    // out << R"({"op":"create_region","name":"r1"})" << std::endl;
    // out << R"({"op":"write","region":"r1","key":"k1","value":"v1"})" << std::endl;
    // out << R"({"op":"write","region":"r1","key":"k2","value":"v2"})" << std::endl;
    // out << R"({"op":"flush","region":"r1"})" << std::endl;
    // out << R"({"op":"stop"})" << std::endl;
    
    json json_array;
    json_array.push_back(createRegion(1,"region1","test1"));
    json_array.push_back(createRegion(2,"region2","test2"));
    out << json_array.dump() << std::endl;

    json_array.clear();
    json_array.push_back(openRegion(1));
    json_array.push_back(openRegion(2));
    out << json_array.dump() << std::endl;

    json_array.clear();
    json_array.push_back(closeRegion(1));
    json_array.push_back(closeRegion(2));
    out << json_array.dump() << std::endl;

    json_array.clear();
    json_array.push_back(dropRegion(1));
    json_array.push_back(dropRegion(2));
    out << json_array.dump() << std::endl;

    out.flush();
    return 0;
}
