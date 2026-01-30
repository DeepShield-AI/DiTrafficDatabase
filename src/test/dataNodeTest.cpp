#include <fstream>
#include <thread>
#include <chrono>
#include <nlohmann/json.hpp>

using json = nlohmann::json;

std::string createRegion(){
    
}

int main() {
    std::ofstream out("/tmp/datanode_input");

    out << R"({"op":"create_region","name":"r1"})" << std::endl;
    out << R"({"op":"write","region":"r1","key":"k1","value":"v1"})" << std::endl;
    out << R"({"op":"write","region":"r1","key":"k2","value":"v2"})" << std::endl;
    out << R"({"op":"flush","region":"r1"})" << std::endl;
    out << R"({"op":"stop"})" << std::endl;

    out.flush();
    return 0;
}
