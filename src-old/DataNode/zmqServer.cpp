#include "zmqServer.hpp"

void ZmqReaderServer::handle_control()
{
    zmq::message_t identity;
    zmq::message_t msg;

    router_.recv(identity);
    router_.recv(msg);

    // TODO:
    // parse REGISTER message
    // extract shard_id
    // shard_map_[shard_id] = identity.to_string();
}

void ZmqReaderServer::send_flow_data()
{
    // TODO:
    // auto flow = ring->get();
    // if no flow, return

    uint32_t shard_id = 0; // calc from flow

    auto it = shard_map_.find(shard_id);
    if (it == shard_map_.end())
        return;

    zmq::message_t identity(it->second.data(), it->second.size());
    zmq::message_t data; // serialize flow

    router_.send(identity, zmq::send_flags::sndmore);
    router_.send(data, zmq::send_flags::none);
}

ZmqReaderServer::ZmqReaderServer(int port)
    : ctx_(1), router_(ctx_, ZMQ_ROUTER)
{
    router_.bind("tcp://*:" + std::to_string(port));
}

void ZmqReaderServer::run()
{
    zmq::pollitem_t items[] = {
        { router_, 0, ZMQ_POLLIN, 0 }
    };

    while (true) {
        zmq::poll(items, 1, std::chrono::milliseconds(10));

        if (items[0].revents & ZMQ_POLLIN) {
            handle_control();
        }

        send_flow_data();
    }
}

void ZmqReaderServer::asynchronousStop(){
    this->stop = true;
}

