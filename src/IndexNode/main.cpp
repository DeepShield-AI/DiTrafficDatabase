#include "heartbeatClient.hpp"
#include <unistd.h>
#include <thread>

struct HeartbeatArgs {
    std::string domain = "10.10.10.108";
    u_int16_t dstport = 8080;
    std::string node_id = "indexnode-worker-1";
    u_int32_t interval_sec = 5;
};

HeartbeatArgs parse_args(int argc, char* argv[]) {
    HeartbeatArgs args;
    for (int i = 1; i < argc; i++) {
        std::string key = argv[i];

        if (key == "--domain" && i + 1 < argc) {
            args.domain = argv[++i];
        } else if (key == "--dstport" && i + 1 < argc) {
            args.dstport = static_cast<u_int16_t>(std::stoi(argv[++i]));
        } else if (key == "--nodeid" && i + 1 < argc) {
            args.node_id = argv[++i];
        } else if (key == "--interval" && i + 1 < argc) {
            args.interval_sec = std::stoul(argv[++i]);
        } else {
            std::cerr << "Unknown argument: " << key << std::endl;
        }
    }
    return args;
}

int main(int argc, char* argv[]) {
    HeartbeatArgs args;

    args = parse_args(argc, argv);

    HeartbeatClient hb(
        args.domain,
        args.dstport,
        args.node_id,
        args.interval_sec
    );

    std::thread hb_thread(&HeartbeatClient::run, &hb);

    printf("heartbeat client started to send heartbeats to %s:%d.\n",args.domain.c_str(), args.dstport);

    while (true){
        sleep(1);
    }

    hb.asynchronousStop();
    printf("stopping heartbeat client...\n");
    hb_thread.join();

    return 0;
}


// #include "udpIndexReceiver.hpp"
// #include "indexGenerator.hpp"
// #include "indexQuerier.hpp"
// #include <thread>

// // std::string iface = "ens192";
// // std::string filename = "./data/test.pcap";
// // u_int64_t offset_threshold = 1024 * 1024;
// // u_int64_t node_id = 1;
// // std::string dstip = "192.0.4.1";
// u_int16_t dstport = 9000;

// int main(){
//     PointerRingBuffer* ring = new PointerRingBuffer(1024*1024);
//     UDPReceiver receiver(dstport, ring, 1024*64);
//     BitMap* bitmap = new BitMap((PORT_BIT_LEN + IPV4_BIT_LEN + IPV6_BIT_LEN) * 2, 15, 1);
//     MemoryIndex* memoryIndex = new MemoryIndex(bitmap, 3);
//     IndexGenerator indexGen(ring, memoryIndex);
//     IndexQuerier indexQue(memoryIndex);

//     std::thread rt(&UDPReceiver::run, &receiver);
//     std::thread it(&IndexGenerator::run, &indexGen);
//     std::thread qt(&IndexQuerier::run, &indexQue);
    
//     printf("wait.\n");
//     qt.join();
//     // char stop;
//     // std::cin>>stop;
//     receiver.asynchronousStop();
//     rt.join();
//     printf("pcap reader stopped.\n");
//     ring->asynchronousStop();
//     indexGen.asynchronousStop();
//     it.join();
//     // sender.asynchronousStop();
//     // st.join();
//     // delete ring;
//     return 0;
// }