CXX = g++
CPPFLAGS += `pkg-config --cflags protobuf grpc grpc++` -I../../protos -I.
CXXFLAGS += -std=c++14
LDFLAGS += `pkg-config --libs protobuf grpc++ grpc` -lgrpc++_reflection -ldl

PROTO_SRC_DIR = ../../protos
BUILD_DIR = build

OBJS = $(BUILD_DIR)/interface.pb.o \
       $(BUILD_DIR)/interface.grpc.pb.o \
       $(BUILD_DIR)/interface_client.o

GEN_SOURCES = $(BUILD_DIR)/interface.pb.cc $(BUILD_DIR)/interface.grpc.pb.cc
GEN_HEADERS = $(BUILD_DIR)/interface.pb.h $(BUILD_DIR)/interface.grpc.pb.h

TARGET = interface_client

all: $(BUILD_DIR) $(TARGET)

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

$(GEN_SOURCES) $(GEN_HEADERS): $(PROTO_SRC_DIR)/interface.proto
	protoc -I $(PROTO_SRC_DIR) --cpp_out=$(BUILD_DIR) $(PROTO_SRC_DIR)/interface.proto
	protoc -I $(PROTO_SRC_DIR) --grpc_out=$(BUILD_DIR) --plugin=protoc-gen-grpc=`which grpc_cpp_plugin` $(PROTO_SRC_DIR)/interface.proto

$(BUILD_DIR)/%.o: %.cc $(GEN_HEADERS)
	$(CXX) $(CXXFLAGS) $(CPPFLAGS) -c $< -o $@

$(BUILD_DIR)/%.o: $(BUILD_DIR)/%.cc $(GEN_HEADERS)
	$(CXX) $(CXXFLAGS) $(CPPFLAGS) -c $< -o $@

$(TARGET): $(OBJS)
	$(CXX) $(OBJS) $(LDFLAGS) -o $@

clean:
	rm -rf $(BUILD_DIR)
	rm -f $(TARGET)
	@echo "Cleaned."