CXX = g++
CPPFLAGS += `pkg-config --cflags protobuf grpc grpc++` -I../../protos
CXXFLAGS += -std=c++14
LDFLAGS += `pkg-config --libs protobuf grpc++ grpc` -lgrpc++_reflection -ldl

PROTO_SRC_DIR = ../../protos
PROTOS_OBJ = interface.pb.o interface.grpc.pb.o

all: interface_client

interface.pb.cc interface.grpc.pb.cc: $(PROTO_SRC_DIR)/interface.proto
	protoc -I $(PROTO_SRC_DIR) --cpp_out=. $(PROTO_SRC_DIR)/interface.proto
	protoc -I $(PROTO_SRC_DIR) --grpc_out=. --plugin=protoc-gen-grpc=`which grpc_cpp_plugin` $(PROTO_SRC_DIR)/interface.proto

%.o: %.cc
	$(CXX) $(CXXFLAGS) $(CPPFLAGS) -c $< -o $@

interface_client: $(PROTOS_OBJ) interface_client.o
	$(CXX) $^ $(LDFLAGS) -o $@

clean:
	rm -f *.o *.pb.cc *.pb.h interface_client