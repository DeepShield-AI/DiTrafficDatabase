#include "libpcapReader.hpp"
#include "flowSender.hpp"
#include <thread>

u_int64_t node_id = 1;

struct Args {
    std::string input_iface = "ens192";
    std::string output_file = "./data/test.pcap";
    u_int64_t offset_threshold = 1024 * 1024;
    std::string dstip = "10.10.10.108";
    u_int16_t dstport = 9000;
};

Args parse_args(int argc, char* argv[]) {
    Args args;
    for (int i = 1; i < argc; i++) {
        std::string key = argv[i];

        if (key == "--input" && i + 1 < argc) {
            args.input_iface = argv[++i];
        } else if (key == "--output" && i + 1 < argc) {
            args.output_file = argv[++i];
        } else if (key == "--offset" && i + 1 < argc) {
            args.offset_threshold = std::stoull(argv[++i]);
        } else if (key == "--dstip" && i + 1 < argc) {
            args.dstip = argv[++i];
        } else if (key == "--dstport" && i + 1 < argc) {
            args.dstport = static_cast<u_int16_t>(std::stoi(argv[++i]));
        } else {
            std::cerr << "Unknown argument: " << key << std::endl;
        }
    }
    return args;
}

LibpcapReader* g_reader = nullptr;
PointerRingBuffer* g_ring = nullptr;
UDPFlowSender* g_sender = nullptr;

// 信号处理函数
void signalHandler(int signum) {
    std::cout << "Received signal " << signum << ", stopping..." << std::endl;
    if (g_reader) g_reader->asynchronousStop();
    if (g_ring) g_ring->asynchronousStop();
    if (g_sender) g_sender->asynchronousStop();
}

int main(int argc, char* argv[]){
    Args args = parse_args(argc, argv);
    PointerRingBuffer* ring = new PointerRingBuffer(1024*1024);
    g_ring = ring;
    LibpcapReader reader(args.input_iface, args.output_file, args.offset_threshold, node_id, ring);
    g_reader = &reader;
    UDPFlowSender sender(ring, args.dstip, args.dstport, sizeof(IndexIPv6) + 2, node_id);
    g_sender = &sender;

    std::signal(SIGTERM, signalHandler);
    std::signal(SIGINT, signalHandler);

    std::thread st(&UDPFlowSender::run, &sender);
    std::thread rt(&LibpcapReader::run, &reader);
    printf("wait.\n");
    char stop;
    std::cin>>stop;
    
    rt.join();
    printf("pcap reader stopped.\n");
    st.join();
    delete ring;
    return 0;
}