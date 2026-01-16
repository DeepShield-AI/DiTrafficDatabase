#include "heartbeatServer.hpp"

int main(){
    HeartbeatServer server(15);
    server.run();
    return 0;
}